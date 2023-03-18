import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import cover
from esphome.const import (
    CONF_ASSUMED_STATE,
    CONF_CLOSE_ACTION,
    CONF_ID,
    CONF_LAMBDA,
    CONF_OPEN_ACTION,
    CONF_OPTIMISTIC,
    CONF_RESTORE_MODE,
    CONF_STOP_ACTION,
)
from .. import rts_ns

DEPENDENCIDES = ["rts"]

RTSCover = rts_ns.class_("RTSCover", cover.Cover, cg.Component)

RTSRestoreMode = rts_ns.enum("RTSRestoreMode")
RESTORE_MODES = {
    "NO_RESTORE": RTSRestoreMode.COVER_NO_RESTORE,
    "RESTORE": RTSRestoreMode.COVER_RESTORE,
    "RESTORE_AND_CALL": RTSRestoreMode.COVER_RESTORE_AND_CALL,
}

CONFIG_SCHEMA = cover.COVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(RTSCover),
        cv.Optional(CONF_ASSUMED_STATE, default=True): cv.boolean,
        cv.Optional(CONF_OPTIMISTIC, default=True): cv.boolean,
        cv.Optional(CONF_LAMBDA): cv.returning_lambda,
        cv.Optional(CONF_OPEN_ACTION): automation.validate_automation(single=True),
        cv.Optional(CONF_CLOSE_ACTION): automation.validate_automation(single=True),
        cv.Optional(CONF_STOP_ACTION): automation.validate_automation(single=True),
        cv.Optional(CONF_RESTORE_MODE, default="NO_RESTORE"): cv.enum(
            RESTORE_MODES, upper=True
        ),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cover.register_cover(var, config)
    if CONF_LAMBDA in config:
        template_ = await cg.process_lambda(
            config[CONF_LAMBDA], [], return_type=cg.optional.template(float)
        )
        cg.add(var.set_state_lambda(template_))
    if CONF_OPEN_ACTION in config:
        await automation.build_automation(
            var.get_open_trigger(), [], config[CONF_OPEN_ACTION]
        )
    if CONF_CLOSE_ACTION in config:
        await automation.build_automation(
            var.get_close_trigger(), [], config[CONF_CLOSE_ACTION]
        )
    if CONF_STOP_ACTION in config:
        await automation.build_automation(
            var.get_stop_trigger(), [], config[CONF_STOP_ACTION]
        )
    cg.add(var.set_restore_mode(config[CONF_RESTORE_MODE]))
