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

void RTS::transmit_rts_command(RTSControlCode control_code, uint32_t channel_id, uint16_t rolling_code) const {
  RTSPacketBody packet(control_code, channel_id, rolling_code);

  ESP_LOGD(TAG, "Transmitting RTS command -- Control code: %x, Channel id: %x, Rolling code value: %d", control_code,
           channel_id, rolling_code);
  ESP_LOGV(TAG, "Cleartext RTS packet:  %s", packet.encoded_data_as_string_().c_str());
  ESP_LOGV(TAG, "Obfuscated RTS packet: %s", packet.obfuscated_data_as_string_().c_str());
}

}  // namespace rts
}  // namespace esphome
