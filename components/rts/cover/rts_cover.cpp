#include "rts_cover.h"
#include "esphome/core/log.h"

namespace esphome {
namespace rts {

static const char *const TAG = "rts.cover";

void RTSCover::setup() {
  ESP_LOGCONFIG(TAG, "Setting up RTS cover '%s'...", this->name_.c_str());

  this->rtc_ = global_preferences->make_preference<ChannelState>(this->get_object_id_hash());
  if (!this->rtc_.load(&this->rts_channel_)) {
    ESP_LOGW(TAG, "Failed to load channel information (INCLUDING ROLLING CODE) for RTS cover: %s", this->name_.c_str());
    ESP_LOGW(TAG, "Entity will be initialized as a NEW REMOTE with a random channel id");

    // Choose a random channel and random start point for the rolling code. The code always starts
    // from the lower half of possible values, because I haven't tested that rollover works :).
    this->rts_channel_.channel_id = static_cast<uint16_t>(random_uint32());
    this->rts_channel_.rolling_code = 0x7fff & static_cast<uint16_t>(random_uint32());
  }
  ESP_LOGI(TAG, "Initialized RTS cover %s with channel id 0x%x; next rolling code value is %u", this->name_.c_str(),
           this->rts_channel_.channel_id, this->rts_channel_.rolling_code);

  switch (this->restore_mode_) {
    case COVER_NO_RESTORE:
      break;
    case COVER_RESTORE: {
      auto restore = this->restore_state_();
      if (restore.has_value())
        restore->apply(this);
      break;
    }
    case COVER_RESTORE_AND_CALL: {
      auto restore = this->restore_state_();
      if (restore.has_value()) {
        restore->to_call(this).perform();
      }
      break;
    }
  }
}

cover::CoverTraits RTSCover::get_traits() {
  cover::CoverTraits traits;
  traits.set_is_assumed_state(true);
  traits.set_supports_position(false);
  traits.set_supports_tilt(false);
  traits.set_supports_toggle(false);
  return traits;
}

void RTSCover::send_program_command() {
  this->rts_parent_->transmit_rts_command(RTS::PROGRAM, this->rts_channel_.channel_id,
                                          this->consume_rolling_code_value_());
}

void RTSCover::config_channel(optional<uint16_t> channel_id, optional<uint16_t> rolling_code) {
  if (channel_id.has_value()) {
    ESP_LOGI(TAG, "Assigning new channel id to RTS cover component %s: previously %x, now %x", this->name_.c_str(),
             this->rts_channel_.channel_id, channel_id.value());
    this->rts_channel_.channel_id = channel_id.value();
  }
  if (rolling_code.has_value()) {
    ESP_LOGI(TAG, "Updating next rolling code value for RTS cover component %s: previously %u, now %u",
             this->name_.c_str(), this->rts_channel_.rolling_code, rolling_code.value());
    this->rts_channel_.rolling_code = rolling_code.value();
  }
  if (!channel_id.has_value() && !rolling_code.has_value()) {
    ESP_LOGW(TAG, "RTS cover component %s received no-op 'config_channel' action", this->name_.c_str());
  }

  this->persist_channel_state_();
}

void RTSCover::control(const cover::CoverCall &call) {
  auto position = call.get_position();
  RTS::RTSControlCode control_code;
  if (position && *position == cover::COVER_OPEN) {
    control_code = RTS::OPEN;
  } else if (position && *position == cover::COVER_CLOSED) {
    control_code = RTS::CLOSE;
  } else if (call.get_stop()) {
    control_code = RTS::STOP;
  } else {
    ESP_LOGW(TAG, "Invalid call to RTS cover component");
    return;
  }

  this->rts_parent_->transmit_rts_command(control_code, this->rts_channel_.channel_id,
                                          this->consume_rolling_code_value_());
}

uint16_t RTSCover::consume_rolling_code_value_() {
  // I have not exhaustively tested using 0 as the rolling code, but I observed it failing at
  // least once.
  uint16_t consumedCode = this->rts_channel_.rolling_code != 0 ? this->rts_channel_.rolling_code : 1;
  this->rts_channel_.rolling_code = consumedCode + 1;

  this->persist_channel_state_();

  return consumedCode;
}

void RTSCover::persist_channel_state_() {
  if (!this->rtc_.save(&this->rts_channel_)) {
    ESP_LOGE(TAG, "Failed to persist channel state for RTS cover %s", this->name_.c_str());
    ESP_LOGE(TAG, "  RTS CONTROL WILL DESYNCHRONIZE IF THIS DEVICE SHUTS DOWN OR RESTARTS");
  }
}

}  // namespace rts
}  // namespace esphome
