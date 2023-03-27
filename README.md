# ESPHome Integration for RTS Window Coverings

This project provides ESPHome components for controlling RTS devices like
curtains and shades. You can import the new components into your ESPHome config
directly from this repository:

```
external_components:
  - source: github://jseyster/esphome-rts-control@master
    components: [ rts ]
```

## Precautions

Be sure to carefully read the configuration instructions before attempting to
pair an ESPHome device as a remote control for your window coverings. Although
there's not much potential to accidentally damange your devices, they could end
up in a state where they are paired to an RTS channel that you cannot control or
unpair. That's not an unrecoverable state, but I haven't tested what happens if
you use up too many of an RTS device's channels.

## Hardware

This integration is tested with an ESP32 connected to an MX-FS-03V radio
transmitter, but it should also work with an ESP8266 and with any similar radio
transmitter. If using an ESP8266, _make sure it is configured to save
preferences to flash_ (`esp8266_restore_from_flash`).

The MX-FS-03V is a 433MHz radio that takes 3.3V power and broadcasts a
continuous radio output while its single input pin is held high, making it
suitable for On-Off Keying (OOK) protocols like RTS. It can be powered directly
from an ESP microcontroller by connecting its VCC and GND pins to the 3.3V and
GND pins on the ESP. The input pin can connect to any of the ESP's digital
output pins.

Note that most small radio modules, including the MS-FS-03V, don't come with an
antenna and will need to have one soldered on in order to broadcast a strong
enough signal to work with any RTS devices.

These modules also have the problem that almost all of them, again including the
MS-FS-03V, broadcast on a frequency that is _slightly_ different from the
433.42MHZ frequency that RTS uses. Radio transmission _may_ still succeed at
shorter ranges, but it may be necessary to replace the oscillator with a
433.42MHz oscillator, which requires skill at soldering.

This Raspberry Pi project provides excellent instructions for setting up the
radio, which also apply for this project:
  - https://github.com/Nickduino/Pi-Somfy

Alternatively, the popular CC1101 transceiver can be programmed in software to
transmit at 433.42MHz. It uses a different control protocol, though, and is not
supported by this project. There is a standalone ESP32 RTS controller that uses
the CC1101 and can also integrate with Home Assistant:
  - https://github.com/rstrouse/ESPSomfy-RTS

## Configuration

In your ESPHome configuration, add config blocks for the `remote_transmitter`,
the `rts` platform, and the list of `cover` devices you want to control.

```
# The transmitter must be configured for 100% duty cycle.
remote_transmitter:
  id: rts_transmitter
  pin: 5  # Use the number for the output pin connected to your radio.
  carrier_duty_percent: 100

rts:
  transmitter_id: rts_transmitter

  # Optional: you can configure how many times you want commands to
  # repeat to compensate for poor signal or interference. RTS devices
  # are designed to ignore any repeated commands that follow a
  # successful transmission.
  command_repetitions: 2  # The default.

cover:
  - platform: rts
    id: curtain_lv
    name: Living room curtain

    # Optional: Home Assistant uses the device class to style the
    # components user interface.
    device_class: curtain
  - platform: rts
    id: shade_lv
    name: Living room shade
    device_class: shade

# Expose internal controller state for backup in case of data loss on
# the ESP microcontroller.
sensor:
  - platform: rts
    rts_cover_id: curtain_lv
    channel_id:
      name: Channel id of living room curtain controller
    rolling_code:
      name: Next rolling code value for living room curtain
  - platform: rts
    rts_cover_id: shade0
    channel_id:
      name: Channel id of living room shade controller
    rolling_code:
      name: Next rolling code value for living room shade

# Pairing buttons that can be removed after all "cover" devices are
# paired as desired.
button:
  - platform: template
    name: Program living room curtain
    on_press:
      - rts.program: curtain_lv
  - platform: template
    name: Program living room shade
    on_press:
      - rts.program: shade_lv
```

### Pairing

The first time the RTS integration loads an RTS cover component, it assigns that
component a random RTS channel that RTS devices can be "programmed" to listen
to. To pair the new cover entity using an RTS remote that is already programmed:
  1. Select the channel on the existing remote that controls the target device
     and confirm that the device responds to commands.
  2. Press and hold the "program" button on the existing remote until the target
     device's motor jogs back-and-forth or up-and-down, indicating that it is in
     "programming" mode.
  3. Send a "program" command from the ESPHome entity you want to pair with the
     device. One way to do that is to include a `button` entity in your ESPHome
     configuration that sends the `rts.program` action to the cover entity, as
     shown in the example configuration above.
  4. If the target device received the program command, it will jog again to
     indicate successful pairing.
  5. Test the cover entity. The paired device should now respond to any commands
     that it sends!
    - If pairing was successful, make sure to leave your ESP microcontroller on
      long enough for it to save the new channel id in its flash storage.
    - If pairing does not succeed, wait for the target device to jog its motor
      again, indicating that it has exited programming mode, before sending any
      further commands. You can retry the "program" command if you think
      pairing failed because of poor radio reception.
  6. Check your ESPHome log output to ensure that a "saving preferences to
     flash" message occurs within a few minutes. If you see an error in the logs
     that RTS could not persist channel state, unpair your cover entity right
     away.
    - The unpairing procedure is identical to the pairing procedure. Use your
      original remote to initiate programming mode in the target device and
      send the "program" command from the entity that you want to unpair.

### Backup

Each RTS cover entity saves a copy of its unique 24-bit channel id and 16-bit
rolling code. The channel id identifies the controller to paired devices, and
the rolling code provides rudimentary security against replay attacks. If either
of these values is lost, devices will no longer recognize commands from the
cover entity that was using them.

The RTS sensor provided with this integration exposes these values to Home
Assistant so that they are also available in Home Assistant's sensor history in
the event of a device failure. There are several Home Assistant add-ons that can
properly back up sensor values to off-site storage.

Use the `rts.config_channel` action to directly set the channel id and rolling
code values for an RTS cover when restoring from backup.

### Rolling Code

Each time an a controller sends a command on an RTS channel, it increments the
rolling code value associated with that channel, because the receiving device
only accepts a rolling code value once. After receiving a valid command on a
paired channel, an RTS device stores the command's rolling code and requires any
new commands to have a rolling code of greater value.

If your ESPHome fails to persist its updated rolling code values because of an
unexpecrted shutdown, commands from affected entities will not work until their
rolling codes catch up. I.e., try spamming a few commands on each device if you
find they're not working after a restart :).

Configuring multiple remotes with the same channel id will result in one or more
desynchronized remotes that are behind on the rolling code and unable to send
valid commands. Cloning your existing remote's channel is not a recommended
configuration. Instead, pair as many remotes and ESPHome cover entities as you
want to each device.

## Acknowledgements

Huge thanks to the author of PushStack, for an excellent and well written
analysis of the RTS protocol.
  - https://pushstack.wordpress.com/somfy-rts-protocol/

Also thanks to the authors of these implementations that I referenced to build
my implementation. Consider them as well if ESPHome is not your preferred home
automation platform.
  - https://github.com/Nickduino/Somfy_Remote
  - https://github.com/dmslabsbr/esphome-somfy

Finally, thanks to the ESPHome team for a well engineered platform that is great
fun to work with.
  - https://esphome.io
