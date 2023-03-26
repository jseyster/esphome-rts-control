#include "rts_channel.h"
#include "esphome/core/log.h"

namespace esphome {
namespace rts {

static const char *const TAG = "rts.channel";

void RTSChannel::init(uint32_t preference_id, const std::string &component_name) {
  this->component_name_ = component_name;
  this->rtc_ = global_preferences->make_preference<ChannelState>(preference_id);
  if (!this->rtc_.load(&this->state_)) {
    ESP_LOGW(TAG, "Failed to load channel information (INCLUDING ROLLING CODE) for RTS component: %s",
             this->component_name_.c_str());
    ESP_LOGW(TAG, "Entity will be initialized as a NEW REMOTE with a random channel id");

    // Choose a random channel and random start point for the rolling code. The code always starts
    // from the lower half of possible values, because I haven't tested that rollover works :).
    this->state_.channel_id = 0xffffff & random_uint32();
    this->state_.rolling_code = 0x7fff & static_cast<uint16_t>(random_uint32());
  }
  ESP_LOGI(TAG, "Initialized RTS component %s with channel id 0x%x; next rolling code value is %u",
           this->component_name_.c_str(), this->state_.channel_id, this->state_.rolling_code);
}

void RTSChannel::config_channel(optional<uint16_t> channel_id, optional<uint16_t> rolling_code) {
  if (channel_id.has_value()) {
    ESP_LOGI(TAG, "Assigning new channel id to RTS component %s: previously %x, now %x", this->component_name_.c_str(),
             this->state_.channel_id, channel_id.value());
    this->state_.channel_id = channel_id.value();
  }
  if (rolling_code.has_value()) {
    ESP_LOGI(TAG, "Updating next rolling code value for RTS component %s: previously %u, now %u",
             this->component_name_.c_str(), this->state_.rolling_code, rolling_code.value());
    this->state_.rolling_code = rolling_code.value();
  }
  if (!channel_id.has_value() && !rolling_code.has_value()) {
    ESP_LOGW(TAG, "RTS cover component %s received no-op 'config_channel' action", this->component_name_.c_str());
  }

  this->persist_channel_state_();
}

uint16_t RTSChannel::consume_rolling_code_value() {
  // I have not exhaustively tested using 0 as the rolling code, but I observed it failing at
  // least once.
  uint16_t consumedCode = this->state_.rolling_code != 0 ? this->state_.rolling_code : 1;
  this->state_.rolling_code = consumedCode + 1;

  this->persist_channel_state_();

  return consumedCode;
}

void RTSChannel::persist_channel_state_() {
  if (!this->rtc_.save(&this->state_)) {
    ESP_LOGE(TAG, "Failed to persist channel state for RTS component %s", this->component_name_.c_str());
    ESP_LOGE(TAG, "  RTS CONTROL WILL DESYNCHRONIZE IF ESPHOME DEVICE SHUTS DOWN OR RESTARTS");
  }

  this->channel_update_callback_.call(this->state_.channel_id, this->state_.rolling_code);
}

}  // namespace rts
}  // namespace esphome
