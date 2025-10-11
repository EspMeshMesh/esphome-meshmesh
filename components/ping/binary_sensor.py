import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_PRESENCE,
    ENTITY_CATEGORY_NONE,
)

from . import PingComponent

DEPENDENCIES = ["ping"]
CONF_PRESENCE = "presence"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(PingComponent),
    cv.Optional(CONF_PRESENCE): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PRESENCE,
        entity_category=ENTITY_CATEGORY_NONE,
    ),
})


async def to_code(config):
    parent = await cg.get_variable(config[CONF_ID])

    if CONF_PRESENCE in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_PRESENCE])
        cg.add(parent.set_presence_sensor(sens))
