import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_ADDRESS, CONF_ID, CONF_TARGET

from .. import meshmesh_direct_ns
from .. import MeshMeshDirectComponent

CONF_MMDIRECT = "mmdirect"

MeshMeshSwitch = meshmesh_direct_ns.class_(
    "MeshMeshSwitch", switch.Switch, cg.Component
)

CONFIG_SCHEMA_BASE = {
    cv.GenerateID(): cv.declare_id(MeshMeshSwitch),
    cv.GenerateID(CONF_MMDIRECT): cv.use_id(MeshMeshDirectComponent),
    cv.Required(CONF_TARGET): cv.positive_int,
    cv.Required(CONF_ADDRESS): cv.positive_int,
}

CONFIG_SCHEMA = switch.switch_schema(MeshMeshSwitch).extend(CONFIG_SCHEMA_BASE).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_MMDIRECT])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

    cg.add(var.set_target(config[CONF_TARGET], config[CONF_ADDRESS]))
    cg.add(var.set_parent(parent))
