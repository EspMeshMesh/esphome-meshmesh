import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_BAUD_RATE,
    CONF_CHANNEL,
    CONF_HARDWARE_UART,
    CONF_ID,
    CONF_PASSWORD,
    CONF_RX_BUFFER_SIZE,
    CONF_TX_BUFFER_SIZE,
)
from esphome.core import CORE

CODEOWNERS = ["@persuader72"]
AUTO_LOAD = ["network", "md5"]


meshmesh_ns = cg.esphome_ns.namespace("meshmesh")
MeshmeshComponent = meshmesh_ns.class_("MeshmeshComponent", cg.Component)

CONF_UART0 = "UART0"
CONF_UART1 = "UART1"
CONF_UART2 = "UART2"
HARDWARE_UART_TO_UART_SELECTION = {
    CONF_UART0: meshmesh_ns.UART_SELECTION_UART0,
    CONF_UART1: meshmesh_ns.UART_SELECTION_UART1,
    CONF_UART2: meshmesh_ns.UART_SELECTION_UART2,
}

CONF_BONDING_MODE = "bonding_mode"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(MeshmeshComponent),
        cv.Optional(CONF_HARDWARE_UART): cv.enum(
            HARDWARE_UART_TO_UART_SELECTION, upper=True
        ),
        cv.Optional(CONF_BAUD_RATE, default=460800): cv.positive_int,
        cv.Optional(CONF_RX_BUFFER_SIZE, default=2048): cv.validate_bytes,
        cv.Optional(CONF_TX_BUFFER_SIZE, default=4096): cv.validate_bytes,
        cv.Required(CONF_CHANNEL): cv.positive_int,
        cv.Required(CONF_PASSWORD): cv.string,
        cv.Optional(CONF_BONDING_MODE, default=False): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    cg.add_define("USE_MESH_MESH")
    cg.add_define("USE_POLITE_BROADCAST_PROTOCOL")
    cg.add_define("USE_MULTIPATH_PROTOCOL")
    cg.add_define("USE_CONNECTED_PROTOCOL")
    if config.get(CONF_BONDING_MODE, False):
        cg.add_define("USE_BONDING_MODE")

    var = cg.Pvariable(
        config[CONF_ID],
        MeshmeshComponent.new(
            config[CONF_BAUD_RATE],
            config[CONF_TX_BUFFER_SIZE],
            config[CONF_RX_BUFFER_SIZE],
        ),
    )
    await cg.register_component(var, config)

    cg.add(var.setChannel(config[CONF_CHANNEL]))
    cg.add(var.setAesPassword(config[CONF_PASSWORD]))
    uart = config.get(CONF_HARDWARE_UART, CONF_UART0):

    cg.add(var.set_uart_selection(uart))
    cg.add(var.pre_setup())

    if CORE.is_esp8266:
        cg.add_build_flag("-Wl,-wrap=ppEnqueueRxq")
        cg.add_library("ESP8266WiFi", None)

    cg.add_library(
        name="ESPMeshMesh",
        version="1.0.8",
        repository="persuader72/ESPMeshMesh",
    )
