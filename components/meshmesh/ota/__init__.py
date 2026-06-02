import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PASSWORD, CONF_VERSION, CONF_PORT
from esphome.core import coroutine_with_priority
from esphome.coroutine import CoroPriority

from .. import meshmesh_ns

CODEOWNERS = ["@persuader72"]
DEPENDENCIES = ["meshmesh", "ota"]
AUTO_LOAD = ["sha256"]

MeshmeshOtaComponent = meshmesh_ns.class_("MeshmeshOtaComponent", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(MeshmeshOtaComponent),
        cv.Optional(CONF_VERSION, default=2): cv.one_of(1, 2, int=True),
        cv.Optional(CONF_PASSWORD): cv.string,
        cv.SplitDefault(CONF_PORT, esp8266=3232, esp32=3232, rp2040=2040, bk72xx=8892, ln882x=8820, rtl87xx=8892, meshmesh=3230): cv.port,
    }
).extend(cv.COMPONENT_SCHEMA)


@coroutine_with_priority(CoroPriority.OTA_UPDATES)
async def to_code(config):
    cg.add_define("USE_OTA")
    cg.add_define("USE_OTA_VERSION", config[CONF_VERSION])

    var = cg.new_Pvariable(config[CONF_ID])
    if config.get(CONF_PASSWORD):
        cg.add(var.set_auth_password(config[CONF_PASSWORD]))
        cg.add_define("USE_OTA_PASSWORD")

    await cg.register_component(var, config)
