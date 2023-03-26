#pragma once

#include <queue>

#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include "esphome/core/component.h"
#include "rts_channel.h"

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

  void schedule_rts_command(RTSControlCode control_code, RTSChannel *rts_channel, int max_repetitions = 16);

  void set_transmitter(remote_transmitter::RemoteTransmitterComponent *transmitter) { transmitter_ = transmitter; }

  void set_command_repetitions(int command_repetitions) { this->command_repetitions_ = command_repetitions; }

 protected:
  struct ScheduledCommand {
    RTSControlCode control_code;
    uint32_t channel_id;
    uint16_t rolling_code;

    int num_repetitions;
    int num_completed_repetitions;
  };

  void process_one_scheduled_command(bool abbreviated_sync = false);

  // Transmits the wakeup signal and returns the length of time to wait before further
  // transmissions in milliseconds. Sets failure_observed_ on error.
  uint32_t transmit_wakeup();

  // Transmits one RTS command packet, including synchronization signals, and returns the length
  // of time to wait before further transmissions in milliseconds. Sets failure_observed_ on
  // error.
  uint32_t transmit_command(RTSControlCode control_code, uint32_t channel_id, uint16_t rolling_code,
                            bool abbreviated_sync = false);

  // Before sending a command, the transmitter sends a long wakeup signal followed by radio
  // silence.
  static constexpr uint32_t wakeup_signal_high_micros = 9415;
  static constexpr uint32_t wakeup_signal_low_millis = 90;  // 89565 microsends per the spec

  // After sending one wakeup, don't send another one for another 10 seconds.
  static constexpr uint32_t wakeup_cooldown_millis = 10000;

  // Total time to transmit a Manchester-encoded bit.
  static constexpr uint32_t symbol_micros = 1208;

  // Each data packet is preceded by a square-wave sync signal.
  static constexpr uint32_t hardware_sync_high_micros = 2 * symbol_micros;
  static constexpr uint32_t hardware_sync_low_micros = 2 * symbol_micros;

  // After the hardware sync and before data bits get transmitted, there is one last
  // synchronization signal that ends with half a signal of silence.
  static constexpr uint32_t software_sync_high_micros = 4550;
  static constexpr uint32_t software_sync_low_micros = symbol_micros / 2;

  static constexpr uint32_t inter_frame_gap_millis = 30;  // 30415 microseconds per the spec

  // Approximate number of data items needed to send a single command. Used to reserve space when
  // building a TransmitCall.
  static constexpr uint32_t transmit_items_per_command_upper_bound = 130;

  remote_transmitter::RemoteTransmitterComponent *transmitter_;
  int command_repetitions_ = 2;

  std::queue<ScheduledCommand> scheduled_commands_;
  bool is_transmit_task_scheduled_{false};
  bool needs_wakeup_{true};
  bool failure_observed_{false};
};

}  // namespace rts
}  // namespace esphome
