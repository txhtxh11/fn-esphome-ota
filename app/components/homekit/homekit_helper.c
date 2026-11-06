#include "homekit_helper.h"
#include <string.h>

void my_identify(homekit_value_t _value) {}

homekit_characteristic_t* new_cha_name(const char* name) {
    return NEW_HOMEKIT_CHARACTERISTIC(NAME, (char*)name);
}

homekit_characteristic_t* new_cha_on() {
    return NEW_HOMEKIT_CHARACTERISTIC(ON, false);
}

homekit_accessory_t* new_switch_accessory(unsigned int id, const char* name, homekit_characteristic_t* cha_on) {
    return NEW_HOMEKIT_ACCESSORY(.id=id, .category=homekit_accessory_category_switch, .services=(homekit_service_t*[]) {
        NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            new_cha_name(name),
            NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ESPHome"),
            NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "HK-SW"),
            NEW_HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
            NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, my_identify),
            NULL
        }),
        NEW_HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            cha_on,
            new_cha_name(name),
            NULL
        }),
        NULL
    });
}

homekit_characteristic_t* new_cha_rotation_speed() {
    return NEW_HOMEKIT_CHARACTERISTIC(ROTATION_SPEED, 100);
}

homekit_accessory_t* new_fan_accessory(unsigned int id, const char* name, 
    homekit_characteristic_t* cha_on, homekit_characteristic_t* cha_speed) {
    return NEW_HOMEKIT_ACCESSORY(.id=id, .category=homekit_accessory_category_fan, .services=(homekit_service_t*[]) {
        NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            new_cha_name(name),
            NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ESPHome"),
            NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "HK-FAN"),
            NEW_HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
            NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, my_identify),
            NULL
        }),
        NEW_HOMEKIT_SERVICE(FAN, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            cha_on,
            cha_speed,
            new_cha_name(name),
            NULL
        }),
        NULL
    });
}

homekit_characteristic_t* new_cha_brightness() {
    return NEW_HOMEKIT_CHARACTERISTIC(BRIGHTNESS, 100);
}

homekit_accessory_t* new_light_accessory(unsigned int id, const char* name, 
    homekit_characteristic_t* cha_on, homekit_characteristic_t* cha_brightness) {
    return NEW_HOMEKIT_ACCESSORY(.id=id, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]) {
        NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            new_cha_name(name),
            NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ESPHome"),
            NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "HK-LIGHT"),
            NEW_HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
            NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, my_identify),
            NULL
        }),
        NEW_HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            cha_on,
            cha_brightness,
            new_cha_name(name),
            NULL
        }),
        NULL
    });
}

homekit_characteristic_t* new_cha_temperature() {
    return NEW_HOMEKIT_CHARACTERISTIC(CURRENT_TEMPERATURE, 0);
}

homekit_characteristic_t* new_cha_humidity() {
    return NEW_HOMEKIT_CHARACTERISTIC(CURRENT_RELATIVE_HUMIDITY, 0);
}

homekit_characteristic_t* new_cha_light() {
    return NEW_HOMEKIT_CHARACTERISTIC(CURRENT_AMBIENT_LIGHT_LEVEL, 0.0001);
}

// 关键修复：根据传感器类型创建对应的服务
homekit_accessory_t* new_sensor_accessory(unsigned int id, const char* name, 
    homekit_characteristic_t* cha_value, const char* sensor_type) {
    
    homekit_accessory_category_t category = homekit_accessory_category_sensor;
    
    if (strcmp(sensor_type, "temperature") == 0) {
        // 温度传感器
        return NEW_HOMEKIT_ACCESSORY(.id=id, .category=category, .services=(homekit_service_t*[]) {
            NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
                new_cha_name(name),
                NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ESPHome"),
                NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "HK-TEMP"),
                NEW_HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
                NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
                NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, my_identify),
                NULL
            }),
            NEW_HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]){
                cha_value,
                new_cha_name(name),
                NULL
            }),
            NULL
        });
    } else if (strcmp(sensor_type, "humidity") == 0) {
        // 关键修复：湿度传感器使用 HUMIDITY_SENSOR 服务
        return NEW_HOMEKIT_ACCESSORY(.id=id, .category=category, .services=(homekit_service_t*[]) {
            NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
                new_cha_name(name),
                NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ESPHome"),
                NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "HK-HUM"),
                NEW_HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
                NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
                NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, my_identify),
                NULL
            }),
            NEW_HOMEKIT_SERVICE(HUMIDITY_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]){
                cha_value,
                new_cha_name(name),
                NULL
            }),
            NULL
        });
    } else if (strcmp(sensor_type, "illuminance") == 0) {
        // 光照传感器使用 LIGHT_SENSOR 服务
        return NEW_HOMEKIT_ACCESSORY(.id=id, .category=category, .services=(homekit_service_t*[]) {
            NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
                new_cha_name(name),
                NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ESPHome"),
                NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "HK-LIGHT"),
                NEW_HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
                NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
                NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, my_identify),
                NULL
            }),
            NEW_HOMEKIT_SERVICE(LIGHT_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]){
                cha_value,
                new_cha_name(name),
                NULL
            }),
            NULL
        });
    } else {
        // 默认温度
        return NEW_HOMEKIT_ACCESSORY(.id=id, .category=category, .services=(homekit_service_t*[]) {
            NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
                new_cha_name(name),
                NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ESPHome"),
                NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "HK-DEF"),
                NEW_HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
                NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
                NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, my_identify),
                NULL
            }),
            NEW_HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]){
                cha_value,
                new_cha_name(name),
                NULL
            }),
            NULL
        });
    }
}

// 二进制传感器（保持之前修复的版本）
homekit_characteristic_t* new_cha_contact() {
    return NEW_HOMEKIT_CHARACTERISTIC(CONTACT_SENSOR_STATE, 0);
}

homekit_characteristic_t* new_cha_motion() {
    return NEW_HOMEKIT_CHARACTERISTIC(MOTION_DETECTED, 0);
}

homekit_characteristic_t* new_cha_smoke() {
    return NEW_HOMEKIT_CHARACTERISTIC(SMOKE_DETECTED, 0);
}

homekit_characteristic_t* new_cha_leak() {
    return NEW_HOMEKIT_CHARACTERISTIC(LEAK_DETECTED, 0);
}

homekit_characteristic_t* new_cha_occupancy() {
    return NEW_HOMEKIT_CHARACTERISTIC(OCCUPANCY_DETECTED, 0);
}

homekit_accessory_t* new_binary_sensor_accessory(unsigned int id, const char* name, 
    homekit_characteristic_t* cha_detected, const char* sensor_type) {
    
    homekit_accessory_category_t category = homekit_accessory_category_sensor;
    
    if (strcmp(sensor_type, "motion") == 0) {
        return NEW_HOMEKIT_ACCESSORY(.id=id, .category=category, .services=(homekit_service_t*[]) {
            NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
                new_cha_name(name),
                NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ESPHome"),
                NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "HK-MOTION"),
                NEW_HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
                NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
                NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, my_identify),
                NULL
            }),
            NEW_HOMEKIT_SERVICE(MOTION_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]){
                cha_detected,
                new_cha_name(name),
                NULL
            }),
            NULL
        });
    } else if (strcmp(sensor_type, "smoke") == 0) {
        return NEW_HOMEKIT_ACCESSORY(.id=id, .category=category, .services=(homekit_service_t*[]) {
            NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
                new_cha_name(name),
                NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ESPHome"),
                NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "HK-SMOKE"),
                NEW_HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
                NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
                NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, my_identify),
                NULL
            }),
            NEW_HOMEKIT_SERVICE(SMOKE_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]){
                cha_detected,
                new_cha_name(name),
                NULL
            }),
            NULL
        });
    } else if (strcmp(sensor_type, "leak") == 0) {
        return NEW_HOMEKIT_ACCESSORY(.id=id, .category=category, .services=(homekit_service_t*[]) {
            NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
                new_cha_name(name),
                NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ESPHome"),
                NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "HK-LEAK"),
                NEW_HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
                NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
                NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, my_identify),
                NULL
            }),
            NEW_HOMEKIT_SERVICE(LEAK_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]){
                cha_detected,
                new_cha_name(name),
                NULL
            }),
            NULL
        });
    } else if (strcmp(sensor_type, "occupancy") == 0) {
        return NEW_HOMEKIT_ACCESSORY(.id=id, .category=category, .services=(homekit_service_t*[]) {
            NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
                new_cha_name(name),
                NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ESPHome"),
                NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "HK-OCCUPANCY"),
                NEW_HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
                NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
                NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, my_identify),
                NULL
            }),
            NEW_HOMEKIT_SERVICE(OCCUPANCY_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]){
                cha_detected,
                new_cha_name(name),
                NULL
            }),
            NULL
        });
    } else {
        // 默认Contact Sensor（门磁）
        return NEW_HOMEKIT_ACCESSORY(.id=id, .category=category, .services=(homekit_service_t*[]) {
            NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
                new_cha_name(name),
                NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "ESPHome"),
                NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "HK-CONTACT"),
                NEW_HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
                NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
                NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, my_identify),
                NULL
            }),
            NEW_HOMEKIT_SERVICE(CONTACT_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]){
                cha_detected,
                new_cha_name(name),
                NULL
            }),
            NULL
        });
    }
}