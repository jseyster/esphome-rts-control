#include "rts_cover.h"
#include "esphome/core/log.h"

namespace esphome {
namespace rts {

static const char *const TAG = "rts.cover";

void RTSCover::setup() {
  ESP_LOGCONFIG(TAG, "Setting up RTS cover '%s'...", this->name_.c_str());

  this->rts_channel_.init(this->get_object_id_hash(), this->name_);

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

void RTSCover::dump_config() {
  LOG_COVER("", "RTS Cover", this);
  ESP_LOGCONFIG(TAG, "  RTS Cover:");
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
  // Note: A long repeated stretch of PROGRAM commands may trigger a motor's programming mode, which
  // is untested and almost certainly not what the user wants.
  // When transmission are unreliable, users are advised to place the radio transmitter as close as
  // possible to the motor when pairing, even if the transmitter will be in a different location
  // during normal operation.
  this->rts_parent_->schedule_rts_command(RTS::PROGRAM, &this->rts_channel_, 2 /* Max repetitions */);
}

void RTSCover::control(const cover::CoverCall &call) {
  auto position = call.get_position();
  RTS::RTSControlCode control_code;
  if (position && *position == cover::COVER_OPEN) {
    control_code = RTS::OPEN;
    this->position = cover::COVER_OPEN;
  } else if (position && *position == cover::COVER_CLOSED) {
    control_code = RTS::CLOSE;
    this->position = cover::COVER_CLOSED;
  } else if (call.get_stop()) {
    control_code = RTS::STOP;
  } else {
    ESP_LOGE(TAG, "Invalid call to RTS cover component: 0x%x", control_code);
    return;
  }

  this->rts_parent_->schedule_rts_command(control_code, &this->rts_channel_);

  this->publish_state();
}

}  // namespace rts
}  // namespace esphome
