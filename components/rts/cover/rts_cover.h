#pragma once

#include "esphome/components/cover/cover.h"
#include "esphome/components/cover/cover_traits.h"
#include "esphome/core/component.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/helpers.h"
#include "esphome/core/optional.h"
#include "esphome/core/preferences.h"
#include "../rts.h"

namespace esphome {
namespace rts {

enum RTSRestoreMode {
  COVER_NO_RESTORE,
  COVER_RESTORE,
  COVER_RESTORE_AND_CALL,
};

class RTSCover : public cover::Cover, public Component {
 public:
  RTSCover() {}
  virtual ~RTSCover() = default;

  void setup() override final;
  cover::CoverTraits get_traits() override final;

  void send_program_command();
  void config_channel(optional<uint16_t> channel_id, optional<uint16_t> rolling_code);

  void set_rts_parent(RTS *rts_parent) { rts_parent_ = rts_parent; }
  void set_restore_mode(RTSRestoreMode restore_mode) { restore_mode_ = restore_mode; }

  void add_on_channel_update_callback(std::function<void(uint32_t, uint16_t)> &&f) {
    this->channel_update_callback_.add(std::move(f));
  }

 protected:
  void control(const cover::CoverCall &call) override final;

  // Returns an unused "rolling code" value and increments the stored rolling code value as a side
  // effect, saving it to persistent storage.
  uint16_t consume_rolling_code_value_();

  void persist_channel_state_();

  RTSRestoreMode restore_mode_{COVER_NO_RESTORE};

  struct ChannelState {
    uint32_t channel_id;
    uint16_t rolling_code;
  } __attribute__((packed)) rts_channel_;

  RTS *rts_parent_;
  ESPPreferenceObject rtc_;

  CallbackManager<void(uint32_t, uint16_t)> channel_update_callback_{};
};

}  // namespace rts
}  // namespace esphome
