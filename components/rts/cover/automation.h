#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/optional.h"
#include "rts_cover.h"

namespace esphome {
namespace rts {

template<typename... Ts> class ProgramAction : public Action<Ts...> {
 public:
  explicit ProgramAction(RTSCover *cover) : cover_(cover) {}

  void play(Ts... x) override { cover_->send_program_command(); }

 protected:
  RTSCover *cover_;
};

template<typename... Ts> class ConfigAction : public Action<Ts...> {
 public:
  explicit ConfigAction(RTSCover *cover) : cover_(cover) {}

  TEMPLATABLE_VALUE(int, channel_id)
  TEMPLATABLE_VALUE(int, rolling_code)

  void play(Ts... x) override {
    optional<uint16_t> channel_id;
    if (this->channel_id_.has_value()) {
      channel_id = static_cast<uint16_t>(this->channel_id_.value(x...));
    }

    optional<uint16_t> rolling_code;
    if (this->rolling_code_.has_value()) {
      rolling_code = static_cast<uint16_t>(this->rolling_code_.value(x...));
    }

    cover_->config_channel(channel_id, rolling_code);
  }

 protected:
  RTSCover *cover_;
};

}  // namespace rts
}  // namespace esphome