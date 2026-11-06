#pragma once
#include "homekit_helper.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include <string.h>
#include "esphome/core/log.h"

namespace esphome { namespace homekit {

class HomekitBinarySensor {
 protected:
    enum SensorType { CONTACT, MOTION, SMOKE, LEAK, OCCUPANCY };
    
    binary_sensor::BinarySensor* bind_sensor_;
    homekit_characteristic_t* cha_detected_;
    bool last_state_ = false;
    SensorType sensor_type_;

    static homekit_value_t getter(const homekit_characteristic_t *ch) {
        HomekitBinarySensor* instance = (HomekitBinarySensor*)ch->context;
        if (!instance || !instance->bind_sensor_) {
            // 安全默认值
            return HOMEKIT_BOOL_CPP(false);
        }
        
        bool state = instance->bind_sensor_->has_state() ? instance->bind_sensor_->state : false;
        
        // 关键：MOTION 用 bool，其他用 uint8
        if (instance->sensor_type_ == MOTION) {
            return HOMEKIT_BOOL_CPP(state);
        } else {
            return HOMEKIT_UINT8_CPP(state ? 1 : 0);
        }
    }

 public:
    HomekitBinarySensor(binary_sensor::BinarySensor* s, const char* type_str) : bind_sensor_(s) {
        ESP_LOGI("homekit", "创建传感器: %s 类型:%s", s->get_name().c_str(), type_str);
        
        if (strcmp(type_str, "contact") == 0) {
            sensor_type_ = CONTACT;
            cha_detected_ = new_cha_contact();
        } else if (strcmp(type_str, "motion") == 0) {
            sensor_type_ = MOTION;
            cha_detected_ = new_cha_motion();
        } else if (strcmp(type_str, "smoke") == 0) {
            sensor_type_ = SMOKE;
            cha_detected_ = new_cha_smoke();
        } else if (strcmp(type_str, "leak") == 0) {
            sensor_type_ = LEAK;
            cha_detected_ = new_cha_leak();
        } else if (strcmp(type_str, "occupancy") == 0) {
            sensor_type_ = OCCUPANCY;
            cha_detected_ = new_cha_occupancy();
        } else {
            sensor_type_ = CONTACT;
            cha_detected_ = new_cha_contact();
        }
        
        cha_detected_->getter_ex = getter;
        cha_detected_->context = this;
        
        // 等待传感器有状态后再设初始值，避免设置错误格式
        if (s->has_state()) {
            bool initial = s->state;
            last_state_ = initial;
            
            // 关键：根据类型设值
            if (sensor_type_ == MOTION) {
                cha_detected_->value.bool_value = initial;
            } else {
                cha_detected_->value.uint8_value = initial ? 1 : 0;
            }
            ESP_LOGI("homekit", "传感器 %s 初始值:%d", s->get_name().c_str(), initial);
        } else {
            // 默认关闭
            last_state_ = false;
            if (sensor_type_ == MOTION) {
                cha_detected_->value.bool_value = false;
            } else {
                cha_detected_->value.uint8_value = 0;
            }
        }
        
        s->add_on_state_callback([this](bool state) {
            this->on_state_change(state);
        });
    }
    
    void on_state_change(bool state) {
        if (state != last_state_) {
            if (sensor_type_ == MOTION) {
                cha_detected_->value.bool_value = state;
            } else {
                cha_detected_->value.uint8_value = state ? 1 : 0;
            }
            
            homekit_characteristic_notify(cha_detected_, cha_detected_->value);
            last_state_ = state;
            ESP_LOGD("homekit", "通知HK: %s = %d", bind_sensor_->get_name().c_str(), state);
        }
    }
    
    void tick() {
        if (bind_sensor_->has_state()) {
            bool current = bind_sensor_->state;
            if (current != last_state_) {
                on_state_change(current);
            }
        }
    }
    
    homekit_characteristic_t* get_cha_detected() { return cha_detected_; }
};

}}