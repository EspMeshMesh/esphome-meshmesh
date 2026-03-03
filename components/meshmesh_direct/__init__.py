from esphome import automation, core
import esphome.codegen as cg
from esphome.components.udp import CONF_ON_RECEIVE
import esphome.config_validation as cv
from esphome.const import CONF_ADDRESS, CONF_DATA, CONF_ID, CONF_TRIGGER_ID
from esphome.types import ConfigType
from esphome.core import coroutine_with_priority
from esphome.coroutine import CoroPriority

CODEOWNERS = ["@persuader72"]
DEPENDENCIES = ["meshmesh"]

byte_vector = cg.std_vector.template(cg.uint8)

meshmesh_direct_ns = cg.esphome_ns.namespace("meshmesh")
MeshMeshDirectComponent = meshmesh_direct_ns.class_(
    "MeshMeshDirectComponent", cg.Component
)

# Handler interfaces that other components can use to register callbacks
MeshMeshDirectReceivedPacketHandler = meshmesh_direct_ns.class_(
    "MeshMeshDirectReceivedPacketHandler"
)

MeshMeshDirectHandlerTrigger = automation.Trigger.template(
    cg.uint32,
    cg.uint8.operator("const").operator("ptr"),
    cg.uint8,
)

OnReceiveTrigger = meshmesh_direct_ns.class_(
    "OnReceiveTrigger",
    MeshMeshDirectHandlerTrigger,
    MeshMeshDirectReceivedPacketHandler,
)


SendAction = meshmesh_direct_ns.class_("SendAction", automation.Action)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(MeshMeshDirectComponent),
        cv.Optional(CONF_ON_RECEIVE): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(OnReceiveTrigger),
            }
        ),
    }
)


async def _trigger_to_code(config):
    if address := config.get(CONF_ADDRESS):
        address = address.parts
    trigger = cg.new_Pvariable(config[CONF_TRIGGER_ID], address)
    await automation.build_automation(
        trigger,
        [
            (cg.uint32, "from"),
            (cg.uint8.operator("const").operator("ptr"), "data"),
            (cg.uint8, "size"),
        ],
        config,
    )
    return trigger


@coroutine_with_priority(CoroPriority.NETWORK_SERVICES)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add_define("USE_MESHMESH_DIRECT")

    for on_receive in config.get(CONF_ON_RECEIVE, []):
        trigger = await _trigger_to_code(on_receive)
        cg.add(var.register_received_handler(trigger))


# ========================================== A C T I O N S ================================================

MAX_MESHMESH_PACKET_SIZE = 1024  # Maximum size of the payload in bytes


def _validate_raw_data(value):
    if isinstance(value, str):
        if len(value) >= MAX_MESHMESH_PACKET_SIZE:
            raise cv.Invalid(
                f"'{CONF_DATA}' must be less than {MAX_MESHMESH_PACKET_SIZE} characters long, got {len(value)}"
            )
        return value
    if isinstance(value, list):
        if len(value) > MAX_MESHMESH_PACKET_SIZE:
            raise cv.Invalid(
                f"'{CONF_DATA}' must be less than {MAX_MESHMESH_PACKET_SIZE} bytes long, got {len(value)}"
            )
        return cv.Schema([cv.hex_uint8_t])(value)
    raise cv.Invalid(
        f"'{CONF_DATA}' must either be a string wrapped in quotes or a list of bytes"
    )


PEER_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(MeshMeshDirectComponent),
        cv.Required(CONF_ADDRESS): cv.templatable(cv.positive_int),
    }
)

SEND_SCHEMA = PEER_SCHEMA.extend(
    {
        cv.Required(CONF_DATA): cv.templatable(_validate_raw_data),
    }
)


def _validate_send_action(config):
    return config


SEND_SCHEMA.add_extra(_validate_send_action)


@automation.register_action(
    "meshmesh_direct.send",
    SendAction,
    SEND_SCHEMA,
)
async def send_action(
    config: ConfigType,
    action_id: core.ID,
    template_arg: cg.TemplateArguments,
    args: list[tuple],
):
    print(f"send_action: {config} {args}")
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])

    data = config.get(CONF_DATA, [])
    if isinstance(data, str):
        data = [cg.RawExpression(f"'{c}'") for c in data]
    templ = await cg.templatable(data, args, byte_vector, byte_vector)
    cg.add(var.set_data(templ))

    peer = config[CONF_ADDRESS]
    templ = await cg.templatable(peer, args, cg.uint32)
    cg.add(var.set_address(templ))

    return var
