#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/helpers.h"
#include "../cover/rts_cover.h"

namespace esphome {
namespace rts {

class RTSChannelSensor : public Component {
  SUB_SENSOR(channel_id)
  SUB_SENSOR(rolling_code)

 public:
  void setup() override final;
  void dump_config() override final;

  void set_rts_cover(RTSCover *rts_cover) { this->rts_cover_ = rts_cover; }

 protected:
  RTSCover *rts_cover_;
};

}  // namespace rts
}  // namespace esphome
