from ..meshmesh import meshmesh_ns
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_ADDRESS, CONF_TIMEOUT


CODEOWNERS = ["@persuader72"]
DEPENDENCIES = ["meshmesh"]
CONF_REPEATERS = "repeaters"
CONF_COORDINATOR_ADDRESS = 2**32 - 2

PingComponent = meshmesh_ns.class_("PingComponent", cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PingComponent),
        cv.Optional(CONF_ADDRESS, default=CONF_COORDINATOR_ADDRESS): cv.positive_int,
        cv.Optional(CONF_REPEATERS, cv.UNDEFINED): cv.ensure_list(cv.positive_int),
    }
).extend(cv.COMPONENT_SCHEMA).extend(cv.polling_component_schema("60s"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_target_address(config[CONF_ADDRESS]))
    if CONF_REPEATERS in config:    
        cg.add(var.set_repeaters(config[CONF_REPEATERS]))
