import logging

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ADDRESS,
    CONF_BAUD_RATE,
    CONF_CHANNEL,
    CONF_HARDWARE_UART,
    CONF_ID,
    CONF_PASSWORD,
    CONF_RX_BUFFER_SIZE,
    CONF_TX_BUFFER_SIZE,
)
from esphome.core import CORE, coroutine_with_priority
from esphome.coroutine import CoroPriority
from esphome.components.logger import request_log_listener

_LOGGER = logging.getLogger(__name__)

CODEOWNERS = ["@persuader72"]
AUTO_LOAD = ["network", "md5"]

UART0 = "UART0"
UART1 = "UART1"
UART2 = "UART2"
DEFAULT = "DEFAULT"

CONF_MESH_ADDRESS = "mesh_address"
CONF_IS_COORDINATOR = "is_coordinator"
CONF_NODE_TYPE = "node_type"
CONF_REPEATERS = "repeaters"
CONF_COORDINATOR_ADDRESS = "coordinator"
CONF_PROTOCOL = "protocol"

meshmesh_ns = cg.esphome_ns.namespace("meshmesh")
MeshmeshComponent = meshmesh_ns.class_("MeshmeshComponent", cg.Component)

MESH_PROTOCOL = {
    "none": 0,
    "polite_broadcast": 5,
}

MESH_SPECIAL_ADDRESSES = {
    "broadcast": 2**32 - 1,
    "coordinator": 2**32 - 2,
    "polite_broadcast": 2**32 - 3,
    "invalid": 0,
    "server": 1,
}

MESH_NODE_TYPE = {
    "backbone": 0,
    "coordinator": 1,
    "edge": 2,
}

HARDWARE_UART_TO_UART_SELECTION = {
    UART0: meshmesh_ns.UART_SELECTION_UART0,
    UART1: meshmesh_ns.UART_SELECTION_UART1,
    UART2: meshmesh_ns.UART_SELECTION_UART2,
    DEFAULT: meshmesh_ns.UART_SELECTION_DEFAULT,
}

CONF_USE_STARPATH = "use_starpath"
CONF_USE_POLITE_BROADCAST_PROTOCOL = "use_polite_broadcast"

MESH_ADDRESS_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_ADDRESS, default=CONF_COORDINATOR_ADDRESS): cv.Any(cv.enum(MESH_SPECIAL_ADDRESSES), cv.positive_int),
        cv.Optional(CONF_REPEATERS, cv.UNDEFINED): cv.ensure_list(cv.positive_int),
        cv.Optional(CONF_PROTOCOL, default="none"): cv.enum(MESH_PROTOCOL),
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(MeshmeshComponent),
        cv.Optional(CONF_IS_COORDINATOR, default=False): cv.boolean,
        cv.Optional(CONF_NODE_TYPE, default="backbone"): cv.enum(MESH_NODE_TYPE),
        cv.Optional(CONF_HARDWARE_UART, default=0): cv.positive_int,
        cv.Optional(CONF_BAUD_RATE, default=460800): cv.positive_int,
        cv.Optional(CONF_RX_BUFFER_SIZE, default=2048): cv.validate_bytes,
        cv.Optional(CONF_TX_BUFFER_SIZE, default=4096): cv.validate_bytes,
        cv.Required(CONF_CHANNEL): cv.positive_int,
        cv.Required(CONF_PASSWORD): cv.string,
        cv.Optional(CONF_USE_STARPATH, default=False): cv.boolean,
        cv.Optional(CONF_USE_POLITE_BROADCAST_PROTOCOL, default=False): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)



# IDF components the ESPMeshMesh PIO library needs (raw 802.11 TX/RX).
# ESPHome's PIO→IDF converter only puts library.json PIO deps in REQUIRES;
# without these, coordinator builds (no wifi:) exclude esp_wifi and fail with
# "esp_wifi.h: No such file" / missing REQUIRES.
_ESPMESHMESH_IDF_REQUIRES = (
    "esp_wifi",
    "esp_phy",
    "esp_event",
    "esp_netif",
    "esp_timer",
    "nvs_flash",
)


def _ensure_espmeshmesh_idf_wifi_stack() -> None:
    """Keep the Wi-Fi IDF stack in the build and in ESPMeshMesh REQUIRES."""
    from esphome.components.esp32 import (
        include_builtin_idf_component,
        request_wifi,
    )

    # Soft-disable SoftAP-only opts stay under reconciler; we only need the radio.
    request_wifi(ap=False)
    for name in ("esp_wifi", "esp_phy", "wpa_supplicant", "esp_coex"):
        include_builtin_idf_component(name)

    try:
        from esphome.espidf import component as idf_comp
    except ImportError:
        return
    if getattr(idf_comp, "_meshmesh_idf_requires_patched", False):
        return

    _orig = idf_comp.generate_cmakelists_txt

    def _generate_cmakelists_txt(component):
        content = _orig(component)
        path = str(getattr(component, "path", ""))
        name = str(getattr(component, "name", ""))
        if "ESPMeshMesh" not in name and "ESPMeshMesh" not in path and "espmeshmesh" not in path.lower():
            return content
        # First REQUIRES line only
        lines = content.splitlines(keepends=True)
        out = []
        for line in lines:
            if line.startswith("  REQUIRES ") and "esp_wifi" not in line:
                # Insert after "REQUIRES "
                rest = line[len("  REQUIRES ") :]
                line = "  REQUIRES " + " ".join(_ESPMESHMESH_IDF_REQUIRES) + " " + rest
            out.append(line)
        return "".join(out)

    idf_comp.generate_cmakelists_txt = _generate_cmakelists_txt
    idf_comp._meshmesh_idf_requires_patched = True


@coroutine_with_priority(CoroPriority.COMMUNICATION)
async def to_code(config):
    # Request a log listener slot for API log streaming
    request_log_listener()

    cg.add_define("USE_MESH_MESH")

    if CONF_USE_STARPATH in config and config[CONF_USE_STARPATH]:
        cg.add_build_flag("-DESPMESH_STARPATH_ENABLED")
    if CONF_USE_POLITE_BROADCAST_PROTOCOL in config and config[CONF_USE_POLITE_BROADCAST_PROTOCOL]:
        cg.add_build_flag("-DUSE_POLITE_BROADCAST_PROTOCOL")
    if config[CONF_NODE_TYPE] == "coordinator":
        cg.add_build_flag("-DESPMESH_RECV_DUP_TABLE_SIZE=0x40")
    else:
        cg.add_build_flag("-DESPMESH_RECV_DUP_TABLE_SIZE=0x10")
    if config[CONF_NODE_TYPE] == "coordinator":
        cg.add_build_flag("-DESPMESH_CONNPATH_MAX_CONNECTIONS=0x40")
    else:
        cg.add_build_flag("-DESPMESH_CONNPATH_MAX_CONNECTIONS=0x10")
    if CORE.is_esp8266:
        cg.add_build_flag("-Wl,-wrap=ppEnqueueRxq")
    if CORE.is_esp32:
        # Allow overriding ieee80211_raw_frame_sanity_check (see espmeshmesh wifi_raw_tx_bypass.c)
        cg.add_build_flag("-Wl,-zmuldefs")
        _ensure_espmeshmesh_idf_wifi_stack()

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
        _LOGGER.warning("is_coordinator is deprecated. Use node_type instead.")
        cg.add(var.setIsCoordinator())
    if CONF_NODE_TYPE in config:
        if config[CONF_IS_COORDINATOR]:
            raise cv.Invalid("is_coordinator and node_type cannot be used together")
        cg.add(var.setNodeType(MESH_NODE_TYPE[config[CONF_NODE_TYPE]]))
    if CONF_HARDWARE_UART in config:
        cg.add(var.set_uart_selection(HARDWARE_UART_TO_UART_SELECTION["UART0"]))
    cg.add(var.pre_setup())
    if CORE.is_esp8266:
        cg.add_library("ESP8266WiFi", None)

    # Use local espmeshmesh while testing IDF 5.5 raw-TX bypass; flip back to registry for release.
    import os
    _espmeshmesh_path = os.environ.get(
        "ESPMESHMESH_PATH", "/home/stefano/Sviluppo/Meshmesh/espmeshmesh"
    )

    # Local file:// breaks ESP-IDF component names under ESPHome 2026.7+
    # (path becomes the component key). Use registry for builds; keep path helper
    # for future workarounds.
    _ = _espmeshmesh_path
    # cg.add_library(
    #     name="ESPMeshMesh",
    #     version="1.6.8",
    #     repository="file://" + _espmeshmesh_path,
    # )
    cg.add_library("ESPMeshMesh", "1.6.8")  # PlatformIO registry
    # <-- End of local copy of the espmeshmesh library sections

    # ESPHome 2026.7+ treats repository= as a git URL; use owner/name for registry.
    cg.add_library("nanopb/Nanopb", "^0.4.91")

    await cg.register_component(var, config)
