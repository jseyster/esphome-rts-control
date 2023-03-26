#include "rts_channel_sensor.h"
#include "esphome/core/log.h"

static const char *const TAG = "rts.sensor";

namespace esphome {
namespace rts {

void RTSChannelSensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up RTS channel sensor for cover '%s'...", this->rts_cover_->get_name().c_str());

  this->rts_cover_->rts_channel().add_on_channel_update_callback([this](uint32_t channel_id, uint16_t rolling_code) {
    if (this->channel_id_sensor_ != nullptr) {
      this->channel_id_sensor_->publish_state(channel_id);
    }
    if (this->rolling_code_sensor_ != nullptr) {
      this->rolling_code_sensor_->publish_state(rolling_code);
    }
  });
}

}  // namespace rts
}  // namespace esphome
