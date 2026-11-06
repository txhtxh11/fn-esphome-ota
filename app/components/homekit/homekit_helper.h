#pragma once
#include <homekit/homekit.h>
#include <homekit/characteristics.h>

#ifdef __cplusplus
extern "C" {
#endif

void my_identify(homekit_value_t _value);
homekit_characteristic_t* new_cha_name(const char* name);
homekit_characteristic_t* new_cha_on();
homekit_accessory_t* new_switch_accessory(unsigned int id, const char* name, homekit_characteristic_t* cha_on);

// 风扇
homekit_characteristic_t* new_cha_rotation_speed();
homekit_accessory_t* new_fan_accessory(unsigned int id, const char* name, 
    homekit_characteristic_t* cha_on, homekit_characteristic_t* cha_speed);

// 灯
homekit_characteristic_t* new_cha_brightness();
homekit_accessory_t* new_light_accessory(unsigned int id, const char* name, 
    homekit_characteristic_t* cha_on, homekit_characteristic_t* cha_brightness);

// 传感器
homekit_characteristic_t* new_cha_temperature();
homekit_characteristic_t* new_cha_humidity();
homekit_characteristic_t* new_cha_light();
homekit_accessory_t* new_sensor_accessory(unsigned int id, const char* name, 
    homekit_characteristic_t* cha_value, const char* sensor_type);

// 二进制传感器
homekit_characteristic_t* new_cha_contact();      // 门窗
homekit_characteristic_t* new_cha_motion();       // 人体移动
homekit_characteristic_t* new_cha_smoke();        // 烟雾
homekit_characteristic_t* new_cha_leak();         // 漏水
homekit_characteristic_t* new_cha_occupancy();    // 占用检测
homekit_accessory_t* new_binary_sensor_accessory(unsigned int id, const char* name, 
    homekit_characteristic_t* cha_detected, const char* sensor_type);

#ifdef __cplusplus
}
#endif