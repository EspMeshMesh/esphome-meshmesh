from ..meshmesh import (
    meshmesh_ns, 
    CONF_MESH_ADDRESS, 
    CONF_PROTOCOL,
    CONF_REPEATERS, 
    MESH_ADDRESS_SCHEMA,
    MESH_PROTOCOL,
)
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_ADDRESS, CONF_TIMEOUT


CODEOWNERS = ["@persuader72"]
DEPENDENCIES = ["meshmesh"]

PingComponent = meshmesh_ns.class_("PingComponent", cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PingComponent),
        cv.Required(CONF_MESH_ADDRESS): MESH_ADDRESS_SCHEMA,
    }
).extend(cv.COMPONENT_SCHEMA).extend(cv.polling_component_schema("60s"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_prefered_protocol(MESH_PROTOCOL[config[CONF_MESH_ADDRESS][CONF_PROTOCOL]]))
    cg.add(var.set_target_address(config[CONF_MESH_ADDRESS][CONF_ADDRESS]))
    if CONF_REPEATERS in config[CONF_MESH_ADDRESS]:    
        cg.add(var.set_repeaters(config[CONF_MESH_ADDRESS][CONF_REPEATERS]))
