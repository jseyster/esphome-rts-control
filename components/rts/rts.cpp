#include <array>
#include <iomanip>
#include <sstream>

#include "rts.h"
#include "esphome/core/log.h"

/**
 * Huge thanks to the author of the protocol description
 * https://pushstack.wordpress.com/somfy-rts-protocol/
 * and the respective authors of these reference implementations
 * https://github.com/Nickduino/Somfy_Remote
 * https://github.com/dmslabsbr/esphome-somfy
 */

namespace esphome {
namespace rts {

static const char *const TAG = "rts.cover";

namespace {

class RTSPacketBody {
 public:
  using Data = std::array<uint8_t, 7>;

  RTSPacketBody(RTS::RTSControlCode control_code, uint32_t channel_id, uint16_t rolling_code) {
    encoded_data_[0] = default_nonce;
    encoded_data_[1] = control_code << 4;
    encoded_data_[2] = rolling_code >> 8;  // Big-ending rolling code
    encoded_data_[3] = 0xff & rolling_code;

    // Little-endian channel id
    for (size_t i = 4; i < encoded_data_.size(); i++) {
      encoded_data_[i] = 0xff & channel_id;
      channel_id >>= 8;
    }

    // Compute 4-bit checksum
    uint8_t checksum = 0;
    for (uint8_t byte : encoded_data_) {
      checksum ^= byte ^ (byte >> 4);
    }
    encoded_data_[1] |= 0xf & checksum;

    // Compute the obfuscated data that gets transmitted.
    for (size_t i = 0; i < obfuscated_data_.size(); i++) {
      obfuscated_data_[i] = encoded_data_[i] ^ (i > 0 ? obfuscated_data_[i - 1] : 0);
    }
  }

  const Data &encoded_data() const { return encoded_data_; }
  const Data &obfuscated_data() const { return obfuscated_data_; }
  std::string encoded_data_as_string_() const { return data_to_string(encoded_data_); }
  std::string obfuscated_data_as_string_() const { return data_to_string(obfuscated_data_); }

 private:
  // Each RTS packet begins with an "encryption key" whose purpose is unclear. Tested devices work
  // the same regardless of the value used, but it is possible that the byte is reserved for future
  // devices. It is also possible that the byte is a perfunctory means to obscure the operation of
  // the obfuscation encoding.
  static constexpr uint8_t default_nonce = 0xa7;

  static std::string data_to_string(const Data &data) {
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(data[0]);
    for (size_t i = 1; i < data.size(); i++) {
      stream << " " << std::setfill('0') << std::setw(2) << static_cast<int>(data[i]);
    }
    return stream.str();
  }

  Data encoded_data_;
  Data obfuscated_data_;
};

}  // namespace

void RTS::schedule_rts_command(RTSControlCode control_code, RTSChannel *rts_channel, int max_repetitions) {
  ScheduledCommand command;
  command.control_code = control_code;
  command.channel_id = rts_channel->id();
  command.rolling_code = rts_channel->consume_rolling_code_value();
  command.num_repetitions = std::min(this->command_repetitions_, max_repetitions);
  command.num_completed_repetitions = 0;
  this->scheduled_commands_.push(command);

  if (!this->is_transmit_task_scheduled_) {
    ESP_LOGD(TAG, "Scheduling RTS transmission handler");
    this->defer([this]() { this->process_one_scheduled_command(); });
    this->is_transmit_task_scheduled_ = true;
  } else {
    // The already scheduled transmission handler will reschedule itself to handle newly enqueued
    // commands.
    ESP_LOGV(TAG, "RTS transmission handler already active");
  }
}

void RTS::process_one_scheduled_command(bool abbreviated_sync) {
  if (this->scheduled_commands_.empty()) {
    this->is_transmit_task_scheduled_ = false;
    ESP_LOGD(TAG, "Completed all scheduled RTS commands");
    return;
  } else if (this->failure_observed_) {
    this->is_transmit_task_scheduled_ = false;
    this->failure_observed_ = false;
    ESP_LOGE(TAG, "Canceling scheduled RTS commands");
    this->scheduled_commands_ = {};
    return;
  }

  uint32_t transmission_delay = 0;
  if (this->needs_wakeup_) {
    ESP_LOGD(TAG, "Transmitting wakeup signal");
    transmission_delay = this->transmit_wakeup();
  } else {
    auto &command = this->scheduled_commands_.front();

    if (command.num_completed_repetitions == 0) {
      ESP_LOGD(TAG, "Transmitting RTS command -- Control code: 0x%x, Channel id: 0x%x, Rolling code value: %d",
               command.control_code, command.channel_id, command.rolling_code);
    } else {
      ESP_LOGV(TAG, "Repeating RTS command on channel 0x%x", command.channel_id);
    }

    transmission_delay =
        this->transmit_command(command.control_code, command.channel_id, command.rolling_code, abbreviated_sync);

    if (++command.num_completed_repetitions >= command.num_repetitions) {
      this->scheduled_commands_.pop();
    }
  }

  this->set_timeout(transmission_delay, [this]() { this->process_one_scheduled_command(true); });
}

uint32_t RTS::transmit_wakeup() {
  auto transmit_call = this->transmitter_->transmit();
  auto transmit_data = transmit_call.get_data();

  if (transmit_data->get_carrier_frequency() != 0) {
    ESP_LOGE(TAG, "Cannot transmit RTS commands over radio configured with a carrier frequency");

    // Return control to the transmission loop without delay.
    return 0;
  }

  transmit_data->mark(wakeup_signal_high_micros);
  transmit_call.perform();

  // Begin the 10 second cooldown period for sending wakeup signals.
  this->set_timeout(wakeup_cooldown_millis, [this]() { this->needs_wakeup_ = true; });
  this->needs_wakeup_ = false;

  // Delay further transmission for 90ms.
  return wakeup_signal_low_millis;
}

uint32_t RTS::transmit_command(RTSControlCode control_code, uint32_t channel_id, uint16_t rolling_code,
                               bool abbreviated_sync) {
  RTSPacketBody packet(control_code, channel_id, rolling_code);

  ESP_LOGVV(TAG, "Cleartext RTS packet:  %s", packet.encoded_data_as_string_().c_str());
  ESP_LOGVV(TAG, "Obfuscated RTS packet: %s", packet.obfuscated_data_as_string_().c_str());

  auto transmit_call = this->transmitter_->transmit();
  auto transmit_data = transmit_call.get_data();

  if (transmit_data->get_carrier_frequency() != 0) {
    ESP_LOGE(TAG, "Cannot transmit RTS commands over radio configured with a carrier frequency");

    // Return control to the transmission loop without delay.
    return 0;
  }

  transmit_data->reserve(transmit_items_per_command_upper_bound);

  // Hardware sync: sent twice immediately after a wakeup signal or 7 times otherwise.
  for (int j = 0; j < (abbreviated_sync ? 2 : 7); j++) {
    transmit_data->item(hardware_sync_high_micros, hardware_sync_low_micros);
  }

  // One last sync signal.
  transmit_data->item(software_sync_high_micros, software_sync_low_micros);

  // Transmit the data with Manchester encoding.
  for (auto byte_to_transmit : packet.obfuscated_data()) {
    for (int bit_index = 0; bit_index < 8; bit_index++) {
      if ((byte_to_transmit & 0x80) == 0) {
        // A 0 bit is transmitted as a falling edge.
        transmit_data->mark(symbol_micros / 2);
        transmit_data->space(symbol_micros / 2);
      } else {
        // A 1 bit is transmitted as a rising edge.
        transmit_data->space(symbol_micros / 2);
        transmit_data->mark(symbol_micros / 2);
      }
      byte_to_transmit <<= 1;
    }
  }

  transmit_call.perform();

  // Delay further transmission for 30ms.
  return inter_frame_gap_millis;
}

}  // namespace rts
}  // namespace esphome
