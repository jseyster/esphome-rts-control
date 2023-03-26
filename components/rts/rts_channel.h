#pragma once

#include "esphome/core/preferences.h"

namespace esphome {
namespace rts {

class RTSChannel {
 public:
  void init(uint32_t preference_id, const std::string &component_name);
  void config_channel(optional<uint16_t> channel_id, optional<uint16_t> rolling_code);

  // Returns an unused "rolling code" value and increments the stored rolling code value as a side
  // effect, saving it to persistent storage.
  uint16_t consume_rolling_code_value();

  uint32_t id() const { return state_.channel_id; }
  uint16_t rolling_code() const { return state_.rolling_code; }

  void add_on_channel_update_callback(std::function<void(uint32_t, uint16_t)> &&f) {
    this->channel_update_callback_.add(std::move(f));
  }

 protected:
  struct ChannelState {
    uint32_t channel_id;
    uint16_t rolling_code;
  } __attribute__((packed)) state_;

  void persist_channel_state_();

  std::string component_name_;
  ESPPreferenceObject rtc_;

  CallbackManager<void(uint32_t, uint16_t)> channel_update_callback_{};
};

}  // namespace rts
}  // namespace esphome
