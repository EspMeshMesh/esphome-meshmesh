import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_INDEX

from esphome.components.meshmesh import meshmesh_ns

MeshmeshTestComponent = meshmesh_ns.class_("MeshmeshTest", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(MeshmeshTestComponent),
        cv.Required(CONF_INDEX): cv.positive_int,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_index(config[CONF_INDEX]))