import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID

from .. import rts_ns
from .. import cover as rtscover

DEPENDENCIES = ["rts"]

CONF_CHANNEL_ID = "channel_id"
CONF_ROLLING_CODE = "rolling_code"
CONF_RTS_COVER_ID = "rts_cover_id"

ICON_REMOTE_TV = "mdi:remote-tv"
ICON_PAPER_ROLL = "mdi:paper-roll"

RTSChannelSensor = rts_ns.class_("RTSChannelSensor", cg.Component)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(RTSChannelSensor),
            cv.Required(CONF_RTS_COVER_ID): cv.use_id(rtscover.RTSCover),
            cv.Optional(CONF_CHANNEL_ID): sensor.sensor_schema(icon=ICON_REMOTE_TV),
            cv.Optional(CONF_ROLLING_CODE): sensor.sensor_schema(icon=ICON_PAPER_ROLL),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.has_at_least_one_key(CONF_CHANNEL_ID, CONF_ROLLING_CODE),
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[CONF_RTS_COVER_ID])
    cg.add(var.set_rts_cover(paren))

    if CONF_CHANNEL_ID in config:
        sens = await sensor.new_sensor(config[CONF_CHANNEL_ID])
        cg.add(var.set_channel_id_sensor(sens))
    if CONF_ROLLING_CODE in config:
        sens = await sensor.new_sensor(config[CONF_ROLLING_CODE])
        cg.add(var.set_rolling_code_sensor(sens))
