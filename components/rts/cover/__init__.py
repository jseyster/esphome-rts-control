import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.automation import maybe_simple_id
from esphome.components import cover
from esphome.const import (CONF_ID, CONF_RESTORE_MODE)
from .. import RTS, rts_ns

DEPENDENCIES = ["rts"]

RTSCover = rts_ns.class_("RTSCover", cover.Cover, cg.Component)
ProgramAction = rts_ns.class_("ProgramAction", automation.Action)
ConfigAction = rts_ns.class_("ConfigAction", automation.Action)

RTSRestoreMode = rts_ns.enum("RTSRestoreMode")
RESTORE_MODES = {
    "NO_RESTORE": RTSRestoreMode.COVER_NO_RESTORE,
    "RESTORE": RTSRestoreMode.COVER_RESTORE,
    "RESTORE_AND_CALL": RTSRestoreMode.COVER_RESTORE_AND_CALL,
}

CONF_CHANNEL_ID = "channel_id"
CONF_ROLLING_CODE = "rolling_code"
CONF_RTS_ID = "rts_id"

CONFIG_SCHEMA = cover.COVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(RTSCover),
        cv.GenerateID(CONF_RTS_ID): cv.use_id(RTS),
        cv.Optional(CONF_RESTORE_MODE, default="NO_RESTORE"): cv.enum(
            RESTORE_MODES, upper=True
        ),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cover.register_cover(var, config)

    paren = await cg.get_variable(config[CONF_RTS_ID])
    cg.add(var.set_rts_parent(paren))
    cg.add(var.set_restore_mode(config[CONF_RESTORE_MODE]))

@automation.register_action(
    "rts.program",
    ProgramAction,
    maybe_simple_id(
        {
            cv.Required(CONF_ID): cv.use_id(RTSCover),
        }
    )
)
async def rts_program_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    return var

@automation.register_action(
    "rts.config_channel",
    ConfigAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(RTSCover),
            cv.Optional(CONF_CHANNEL_ID): cv.templatable(cv.int_range(min=0, max=0xffffff)),
            cv.Optional(CONF_ROLLING_CODE): cv.templatable(cv.int_range(min=0, max=0xffff)),
        }
    )
)
async def rts_config_channel_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    if CONF_CHANNEL_ID in config:
        template_ = await cg.templatable(config[CONF_CHANNEL_ID], args, int)
        cg.add(var.set_channel_id(template_))
    if CONF_ROLLING_CODE in config:
        template_ = await cg.templatable(config[CONF_ROLLING_CODE], args, int)
        cg.add(var.set_rolling_code(template_))
    return var
