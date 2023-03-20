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

  void transmit_rts_command(RTSControlCode control_code, uint32_t channel_id, uint16_t rolling_code,
                            int max_repetitions = 16) const;

  void set_transmitter(remote_transmitter::RemoteTransmitterComponent *transmitter) { transmitter_ = transmitter; }

  void set_command_repetitions(int command_repetitions) { this->command_repetitions_ = command_repetitions; }

 protected:
  // Before sending the command, the transmitter sends a long wakeup signal followed by radio
  // silence.
  static constexpr uint32_t wakeup_signal_high_micros = 9415;
  static constexpr uint32_t wakeup_signal_low_micros = 89565;

  // Total time to transmit a Manchester-encoded bit.
  static constexpr uint32_t symbol_micros = 1208;

  // Each data packet is preceded by a square-wave sync signal.
  static constexpr uint32_t hardware_sync_high_micros = 2 * symbol_micros;
  static constexpr uint32_t hardware_sync_low_micros = 2 * symbol_micros;

  // After the hardware sync and before data bits get transmitted, there is one last
  // synchronization signal that ends with half a signal of silence.
  static constexpr uint32_t software_sync_high_micros = 4550;
  static constexpr uint32_t software_sync_low_micros = symbol_micros / 2;

  static constexpr uint32_t inter_frame_gap_micros = 30415;

  // Approximate number of data items needed to send a single command. Used to reserve space when
  // building a TransmitCall.
  static constexpr uint32_t transmit_items_per_command_upper_bound = 130;

  remote_transmitter::RemoteTransmitterComponent *transmitter_;

  int command_repetitions_ = 2;
};

}  // namespace rts
}  // namespace esphome
