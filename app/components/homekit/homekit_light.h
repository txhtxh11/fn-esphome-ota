#pragma once
#include "homekit_helper.h"
#include "esphome/components/light/light_state.h"

namespace esphome {
namespace homekit {

class HomekitLight {
 protected:
    light::LightState* bind_light_;
    homekit_characteristic_t* cha_on_;
    homekit_characteristic_t* cha_bright_;
    
    // 用于检测变化
    bool last_state_ = false;
    int last_brightness_ = 100;

    static void cha_on_setter(homekit_characteristic_t *ch, homekit_value_t value) {
        HomekitLight* instance = (HomekitLight*)ch->context;
        auto call = instance->bind_light_->make_call();
        call.set_state(value.bool_value);
        call.perform();
    }

    static homekit_value_t cha_on_getter(const homekit_characteristic_t *ch) {
        HomekitLight* instance = (HomekitLight*)ch->context;
        return HOMEKIT_BOOL_CPP(instance->bind_light_->remote_values.is_on());
    }

    static void cha_bright_setter(homekit_characteristic_t *ch, homekit_value_t value) {
        HomekitLight* instance = (HomekitLight*)ch->context;
        auto call = instance->bind_light_->make_call();
        call.set_brightness((float)value.int_value / 100.0f);
        call.perform();
    }

    static homekit_value_t cha_bright_getter(const homekit_characteristic_t *ch) {
        HomekitLight* instance = (HomekitLight*)ch->context;
        int brightness = (int)(instance->bind_light_->remote_values.get_brightness() * 100);
        return HOMEKIT_INT_CPP(brightness);
    }

 public:
    HomekitLight(light::LightState* bind_light) : bind_light_(bind_light) {
        // 初始化当前状态
        auto values = bind_light_->remote_values;
        last_state_ = values.is_on();
        last_brightness_ = (int)(values.get_brightness() * 100);

        this->cha_on_ = new_cha_on();
        this->cha_on_->getter_ex = cha_on_getter;
        this->cha_on_->setter_ex = cha_on_setter;
        this->cha_on_->context = this;

        this->cha_bright_ = new_cha_brightness();
        this->cha_bright_->getter_ex = cha_bright_getter;
        this->cha_bright_->setter_ex = cha_bright_setter;
        this->cha_bright_->context = this;
    }

    // 关键：轮询检查变化并通知 HomeKit
    void tick() {
        auto values = this->bind_light_->remote_values;
        bool current_on = values.is_on();
        int current_bright = (int)(values.get_brightness() * 100);

        if (current_on != last_state_) {
            this->cha_on_->value.bool_value = current_on;
            homekit_characteristic_notify(this->cha_on_, this->cha_on_->value);
            last_state_ = current_on;
        }

        if (current_bright != last_brightness_) {
            this->cha_bright_->value.int_value = current_bright;
            homekit_characteristic_notify(this->cha_bright_, this->cha_bright_->value);
            last_brightness_ = current_bright;
        }
    }

    homekit_characteristic_t* get_cha_on() { return this->cha_on_; }
    homekit_characteristic_t* get_cha_bright() { return this->cha_bright_; }
};

}
}