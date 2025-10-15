import esphome.codegen as cg
from esphome.components.packet_transport import (
    PacketTransport,
    new_packet_transport,
    transport_schema,
)
import esphome.config_validation as cv
from esphome.const import CONF_ADDRESS
from esphome.cpp_types import PollingComponent

from .. import MeshmeshComponent, meshmesh_ns

CONF_MESHMESH_ID = "meshmesh_id"
CONF_REPEATERS = "repeaters"

MeshmeshTransport = meshmesh_ns.class_(
    "MeshmeshTransport", PacketTransport, PollingComponent
)

CONFIG_SCHEMA = transport_schema(MeshmeshTransport).extend(
    {
        cv.GenerateID(CONF_MESHMESH_ID): cv.use_id(MeshmeshComponent),
        cv.Required(CONF_ADDRESS): cv.positive_int,
        cv.Optional(CONF_REPEATERS, cv.UNDEFINED): cv.ensure_list(cv.positive_int),
    }
)


async def to_code(config):
    var, _ = await new_packet_transport(config)
    meshmesh = await cg.get_variable(config[CONF_MESHMESH_ID])
    cg.add(var.set_parent(meshmesh))
    cg.add(var.set_address(config[CONF_ADDRESS]))
    if CONF_REPEATERS in config:
        cg.add(var.set_repeaters(config[CONF_REPEATERS]))