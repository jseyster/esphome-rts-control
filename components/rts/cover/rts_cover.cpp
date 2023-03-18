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
    this->rts_channel_.channelId = static_cast<uint16_t>(random_uint32());
    this->rts_channel_.rollingCode = 0x7fff & static_cast<uint16_t>(random_uint32());
  }
  ESP_LOGI(TAG, "Initialized RTS cover %s with channel id 0x%x; next rolling code value is %u", this->name_.c_str(),
           this->rts_channel_.channelId, this->rts_channel_.rollingCode);

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

void RTSCover::control(const cover::CoverCall &call) {
  auto position = call.get_position();
  if (position && *position == cover::COVER_OPEN) {
    auto code = consumeRollingCodeValue();
    ESP_LOGW(TAG, "Open call with code %u to RTS cover component %s", code, this->name_.c_str());
  } else if (position && *position == cover::COVER_CLOSED) {
    auto code = consumeRollingCodeValue();
    ESP_LOGW(TAG, "Close call with code %u to RTS cover component %s", code, this->name_.c_str());
  } else if (call.get_stop()) {
    auto code = consumeRollingCodeValue();
    ESP_LOGW(TAG, "Stop call with code %u to RTS cover component %s", code, this->name_.c_str());
  } else {
    ESP_LOGW(TAG, "Invalid call to RTS cover component");
  }
}

uint16_t RTSCover::consumeRollingCodeValue() {
  // I have not exhaustively tested using 0 as the rolling code, but I observed it failing at
  // least once.
  uint16_t consumedCode = this->rts_channel_.rollingCode != 0 ? this->rts_channel_.rollingCode : 1;
  this->rts_channel_.rollingCode = consumedCode + 1;

  this->rtc_.save(&this->rts_channel_);

  return consumedCode;
}

}  // namespace rts
}  // namespace esphome
