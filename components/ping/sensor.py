import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_DURATION,
    ENTITY_CATEGORY_NONE,
    UNIT_MILLISECOND,
)

from . import PingComponent

DEPENDENCIES = ["ping"]
CONF_LATENCY = "latency"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(PingComponent),
    cv.Optional(CONF_LATENCY): sensor.sensor_schema(
        unit_of_measurement=UNIT_MILLISECOND,
        device_class=DEVICE_CLASS_DURATION,
        entity_category=ENTITY_CATEGORY_NONE,
    ),
})


async def to_code(config):
    parent = await cg.get_variable(config[CONF_ID])

    if CONF_LATENCY in config:
        sens = await sensor.new_sensor(config[CONF_LATENCY])
        cg.add(parent.set_latency_sensor(sens))
