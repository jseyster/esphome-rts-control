#pragma once

#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include "esphome/core/component.h"

namespace esphome {
namespace rts {

class RTS : public Component {
 public:
  enum RTSControlCode {
    STOP = 0x1,
    OPEN = 0x2,
    // SET_OPEN_MOTOR_LIMIT = 0x3,
    CLOSE = 0x4,
    // SET_CLOSED_MOTOR_LIMIT = 0x5,
    // MOTOR_PROGRAMMING = 0x6,
    // UNUSED = 0x7,
    PROGRAM = 0x8,
    // ENABLE_SUN_DETECTOR = 0x9,
    // DISABLE_SUN_DETECTOR = 0xa,
  };

  void transmit_rts_command(RTSControlCode control_code, uint32_t channel_id, uint16_t rolling_code) const;

  void set_transmitter(remote_transmitter::RemoteTransmitterComponent *transmitter) { transmitter_ = transmitter; }

 protected:
  remote_transmitter::RemoteTransmitterComponent *transmitter_;
};

}  // namespace rts
}  // namespace esphome
