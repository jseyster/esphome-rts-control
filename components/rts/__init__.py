import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import remote_transmitter
from esphome.components.remote_base import CONF_TRANSMITTER_ID
from esphome.const import CONF_ID

rts_ns = cg.esphome_ns.namespace("rts")
RTS = rts_ns.class_("RTS", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(RTS),
        cv.Required(CONF_TRANSMITTER_ID): cv.use_id(remote_transmitter.RemoteTransmitterComponent),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    transmitter = await cg.get_variable(config[CONF_TRANSMITTER_ID])
    cg.add(var.set_transmitter(transmitter))
