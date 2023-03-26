#pragma once

#include "esphome/components/cover/cover.h"
#include "esphome/components/cover/cover_traits.h"
#include "esphome/core/component.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/helpers.h"
#include "esphome/core/optional.h"
#include "esphome/core/preferences.h"
#include "../rts.h"
#include "../rts_channel.h"

namespace esphome {
namespace rts {

enum RTSRestoreMode {
  COVER_NO_RESTORE,
  COVER_RESTORE,
  COVER_RESTORE_AND_CALL,
};

class RTSCover : public cover::Cover, public Component {
 public:
  void setup() override final;
  void dump_config() override final;
  cover::CoverTraits get_traits() override final;

  void send_program_command();

  void set_rts_parent(RTS *rts_parent) { rts_parent_ = rts_parent; }
  void set_restore_mode(RTSRestoreMode restore_mode) { restore_mode_ = restore_mode; }

  RTSChannel &rts_channel() { return rts_channel_; }

 protected:
  void control(const cover::CoverCall &call) override final;

  RTSRestoreMode restore_mode_{COVER_NO_RESTORE};

  RTS *rts_parent_;
  RTSChannel rts_channel_;

  CallbackManager<void(uint32_t, uint16_t)> channel_update_callback_{};
};

}  // namespace rts
}  // namespace esphome
