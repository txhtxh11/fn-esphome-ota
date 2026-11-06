#pragma once
#include "homekit_helper.h"
#include "esphome/components/sensor/sensor.h"
#include <string.h>
#include <math.h>

namespace esphome { namespace homekit {

class HomekitSensor {
 protected:
    sensor::Sensor* bind_sensor_;
    homekit_characteristic_t* cha_value_;
    float last_value_ = 0;
    uint32_t last_update_ = 0;
    
    enum SensorType { TEMPERATURE, HUMIDITY, ILLUMINANCE };
    SensorType type_;

    static homekit_value_t getter(const homekit_characteristic_t *ch) {
        HomekitSensor* i = (HomekitSensor*)ch->context;
        if (!i || !i->bind_sensor_) {
            return HOMEKIT_FLOAT_CPP(0.0);
        }
        float val = i->bind_sensor_->get_state();
        if (isnan(val)) val = 0.0;
        return HOMEKIT_FLOAT_CPP(val);
    }

 public:
    HomekitSensor(sensor::Sensor* s, const char* type_str) : bind_sensor_(s) {
        // 根据类型判断
        if (strcmp(type_str, "temperature") == 0) {
            type_ = TEMPERATURE;
            cha_value_ = new_cha_temperature();
        } else if (strcmp(type_str, "humidity") == 0) {
            type_ = HUMIDITY;
            cha_value_ = new_cha_humidity();
        } else if (strcmp(type_str, "illuminance") == 0) {
            type_ = ILLUMINANCE;
            cha_value_ = new_cha_light();
        } else {
            type_ = TEMPERATURE;
            cha_value_ = new_cha_temperature();
        }
        
        cha_value_->getter_ex = getter;
        cha_value_->context = this;
        
        // 关键：设置初始值，避免HomeKit未响应
        float initial = s->get_state();
        if (!isnan(initial)) {
            cha_value_->value.float_value = initial;
            last_value_ = initial;
        } else {
            cha_value_->value.float_value = 0.0;
            last_value_ = 0.0;
        }
    }
    
    // 轮询检查更新
    void tick() {
        uint32_t now = millis();
        // 不同传感器不同刷新频率
        uint32_t min_interval = 5000; // 默认5秒
        
        if (type_ == HUMIDITY) min_interval = 10000; // 湿度10秒
        else if (type_ == ILLUMINANCE) min_interval = 30000; // 光照30秒
        
        if (now - last_update_ < min_interval) return;
        
        float current = bind_sensor_->get_state();
        if (isnan(current)) return;
        
        // 判断是否需要更新
        bool should_update = false;
        float threshold = 0.5; // 默认0.5
        
        if (type_ == TEMPERATURE && fabs(current - last_value_) >= 0.5) should_update = true;
        else if (type_ == HUMIDITY && fabs(current - last_value_) >= 2.0) should_update = true; // 湿度变化2%就更新
        else if (type_ == ILLUMINANCE && fabs(current - last_value_) >= 10.0) should_update = true;
        else if (now - last_update_ > 60000) should_update = true; // 强制每分钟更新
        
        if (should_update) {
            cha_value_->value.float_value = current;
            homekit_characteristic_notify(cha_value_, cha_value_->value);
            last_value_ = current;
            last_update_ = now;
        }
    }
    
    homekit_characteristic_t* get_cha_value() { return cha_value_; }
    const char* get_sensor_type() { 
        if (type_ == TEMPERATURE) return "temperature";
        if (type_ == HUMIDITY) return "humidity";
        if (type_ == ILLUMINANCE) return "illuminance";
        return "unknown";
    }
};

}}
