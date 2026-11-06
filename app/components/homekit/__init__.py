import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID, CONF_TYPE
from esphome.components import switch, fan, light, sensor, binary_sensor

# 关键修改：添加 AUTO_LOAD，确保所有组件头文件都可用
# 这样 YAML 中没有配置 fan，头文件依然存在，不会编译报错
AUTO_LOAD = ['switch', 'fan', 'light', 'sensor', 'binary_sensor']
DEPENDENCIES = ['network']

homekit_ns = cg.esphome_ns.namespace('homekit')
HomekitComponent = homekit_ns.class_('HomekitComponent', cg.Component)
HomekitResetAction = homekit_ns.class_("HomekitResetAction", automation.Action)

CONF_SWITCHES = "switches"
CONF_FANS = "fans"
CONF_LIGHTS = "lights"
CONF_SENSORS = "sensors"
CONF_BINARY_SENSORS = "binary_sensors"

SENSOR_SCHEMA = cv.Schema({
    cv.Required(CONF_ID): cv.use_id(sensor.Sensor),
    cv.Optional(CONF_TYPE, default="temperature"): cv.enum({
        "temperature": "temperature",
        "humidity": "humidity", 
        "illuminance": "illuminance"
    }),
})

BINARY_SENSOR_SCHEMA = cv.Schema({
    cv.Required(CONF_ID): cv.use_id(binary_sensor.BinarySensor),
    cv.Optional(CONF_TYPE, default="contact"): cv.enum({
        "contact": "contact",
        "motion": "motion",
        "smoke": "smoke",
        "leak": "leak",
        "occupancy": "occupancy",
    }),
})

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(HomekitComponent),
    cv.Optional("setup_code", default="111-11-111"): cv.string,
    cv.Optional(CONF_SWITCHES): cv.ensure_list(cv.use_id(switch.Switch)),
    cv.Optional(CONF_FANS): cv.ensure_list(cv.use_id(fan.Fan)),
    cv.Optional(CONF_LIGHTS): cv.ensure_list(cv.use_id(light.LightState)),
    cv.Optional(CONF_SENSORS): cv.ensure_list(SENSOR_SCHEMA),
    cv.Optional(CONF_BINARY_SENSORS): cv.ensure_list(BINARY_SENSOR_SCHEMA),
}).extend(cv.COMPONENT_SCHEMA)

def to_code(config):
    # Pinned to a fork that builds under ESPHome's -DNO_GLOBAL_MDNS (Arduino core 3.1+).
    # Upstream Mixiaoxiao/Arduino-HomeKit-ESP8266 relies on the global `MDNS`
    # instance, which ESPHome's mdns component compiles out.
    cg.add_library("HomeKit-ESP8266", "2d25affe20205af44752fc5db7c62b6687c04285",
        "https://hk.gh-proxy.org/https://github.com/txhtxh11/Arduino-HomeKit-ESP8266.git")
    cg.add_build_flag("-DARDUINO_HOMEKIT_LOWROM")
    cg.add_build_flag("-DHOMEKIT_LOG_LEVEL=0")
    
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_setup_code(config["setup_code"]))
    
    # 只有配置了才生成代码，没配置的不会占用内存
    if CONF_SWITCHES in config:
        for sw in config[CONF_SWITCHES]:
            s = yield cg.get_variable(sw)
            cg.add(var.add_switch(s))
    
    if CONF_FANS in config:
        for f in config[CONF_FANS]:
            fan_obj = yield cg.get_variable(f)
            cg.add(var.add_fan(fan_obj))
    
    if CONF_LIGHTS in config:
        for lt in config[CONF_LIGHTS]:
            light_obj = yield cg.get_variable(lt)
            cg.add(var.add_light(light_obj))
    
    if CONF_SENSORS in config:
        for sen in config[CONF_SENSORS]:
            sensor_obj = yield cg.get_variable(sen[CONF_ID])
            cg.add(var.add_sensor(sensor_obj, sen[CONF_TYPE]))
    
    if CONF_BINARY_SENSORS in config:
        for bs in config[CONF_BINARY_SENSORS]:
            bin_obj = yield cg.get_variable(bs[CONF_ID])
            cg.add(var.add_binary_sensor(bin_obj, bs[CONF_TYPE]))
            
    yield cg.register_component(var, config)

@automation.register_action("homekit.reset_storage", HomekitResetAction, automation.maybe_simple_id({
    cv.Required(CONF_ID): cv.use_id(HomekitComponent),
}))
async def homekit_reset_to_code(config, action_id, template_arg, args):
    var = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, var)