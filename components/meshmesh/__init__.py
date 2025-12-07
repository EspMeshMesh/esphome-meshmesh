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
from esphome.core import CORE, coroutine_with_priority

CODEOWNERS = ["@persuader72"]
AUTO_LOAD = ["network", "md5"]

UART0 = "UART0"
UART1 = "UART1"
UART2 = "UART2"
DEFAULT = "DEFAULT"

CONF_IS_COORDINATOR = "is_coordinator"

meshmesh_ns = cg.esphome_ns.namespace("meshmesh")
MeshmeshComponent = meshmesh_ns.class_("MeshmeshComponent", cg.Component)

HARDWARE_UART_TO_UART_SELECTION = {
    UART0: meshmesh_ns.UART_SELECTION_UART0,
    UART1: meshmesh_ns.UART_SELECTION_UART1,
    UART2: meshmesh_ns.UART_SELECTION_UART2,
    DEFAULT: meshmesh_ns.UART_SELECTION_DEFAULT,
}

CONF_BONDING_MODE = "bonding_mode"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(MeshmeshComponent),
        cv.Optional(CONF_IS_COORDINATOR, default=False): cv.boolean,
        cv.Optional(CONF_HARDWARE_UART, default=0): cv.positive_int,
        cv.Optional(CONF_BAUD_RATE, default=460800): cv.positive_int,
        cv.Optional(CONF_RX_BUFFER_SIZE, default=2048): cv.validate_bytes,
        cv.Optional(CONF_TX_BUFFER_SIZE, default=4096): cv.validate_bytes,
        cv.Required(CONF_CHANNEL): cv.positive_int,
        cv.Required(CONF_PASSWORD): cv.string,
        cv.Optional(CONF_BONDING_MODE, default=False): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)


@coroutine_with_priority(40.0)
async def to_code(config):
    cg.add_define("USE_MESH_MESH")
    if CONF_BONDING_MODE in config and config[CONF_BONDING_MODE]:
        cg.add_define("USE_BONDING_MODE")

    if CORE.is_esp8266:
        cg.add_build_flag("-Wl,-wrap=ppEnqueueRxq")

    #cg.add_build_flag("-DUSE_POLITE_BROADCAST_PROTOCOL")

    var = cg.Pvariable(
        config[CONF_ID],
        MeshmeshComponent.new(
            config[CONF_BAUD_RATE],
            config[CONF_TX_BUFFER_SIZE],
            config[CONF_RX_BUFFER_SIZE],
        ),
    )
    cg.add(var.setChannel(config[CONF_CHANNEL]))
    cg.add(var.setAesPassword(config[CONF_PASSWORD]))
    if config[CONF_IS_COORDINATOR]:
        cg.add(var.setIsCoordinator())
    if CONF_HARDWARE_UART in config:
        cg.add(var.set_uart_selection(HARDWARE_UART_TO_UART_SELECTION["UART0"]))
    cg.add(var.pre_setup())
    if CORE.is_esp8266:
        cg.add_library("ESP8266WiFi", None)

    cg.add_library(
        name="ESPMeshMesh-dev",
        version="1.3.2",
        repository="persuader72/ESPMeshMesh-dev",
        #repository="file:///home/stefano/Sviluppo/Stefano/Meshmesh/workspace/espmeshmesh/",
    )

    await cg.register_component(var, config)
