#pragma once
#include "homekit_helper.h"
#include "esphome/components/fan/fan.h"
#include <string.h>

namespace esphome { namespace homekit {

class HomekitFan {
 protected:
    fan::Fan* bind_fan_;
    homekit_characteristic_t* cha_on_;
    homekit_characteristic_t* cha_speed_;

    static void setter(homekit_characteristic_t *ch, homekit_value_t value) {
        HomekitFan* i = (HomekitFan*)ch->context;
        if (strcmp(ch->type, HOMEKIT_CHARACTERISTIC_ON) == 0) {
            if (value.bool_value) 
                i->bind_fan_->turn_on().perform(); 
            else 
                i->bind_fan_->turn_off().perform();
        } else if (strcmp(ch->type, HOMEKIT_CHARACTERISTIC_ROTATION_SPEED) == 0) {
            auto call = i->bind_fan_->make_call();
            call.set_speed(value.float_value);
            call.perform();
        }
        homekit_characteristic_notify(ch, value);
    }
    
    static homekit_value_t getter(const homekit_characteristic_t *ch) {
        HomekitFan* i = (HomekitFan*)ch->context;
        if (strcmp(ch->type, HOMEKIT_CHARACTERISTIC_ON) == 0) {
            return HOMEKIT_BOOL_CPP(i->bind_fan_->state);
        } else if (strcmp(ch->type, HOMEKIT_CHARACTERISTIC_ROTATION_SPEED) == 0) {
            float speed = i->bind_fan_->state ? (i->bind_fan_->speed > 0 ? i->bind_fan_->speed : 100) : 0;
            return HOMEKIT_FLOAT_CPP(speed);
        }
        return HOMEKIT_NULL_CPP();
    }
    
    void on_esphome_state_change() {
        this->cha_on_->value.bool_value = this->bind_fan_->state;
        homekit_characteristic_notify(this->cha_on_, this->cha_on_->value);
        
        if (this->bind_fan_->speed > 0) {
            this->cha_speed_->value.float_value = this->bind_fan_->speed;
            homekit_characteristic_notify(this->cha_speed_, this->cha_speed_->value);
        }
    }

 public:
    HomekitFan(fan::Fan* f) : bind_fan_(f) {
        cha_on_ = new_cha_on();
        cha_on_->getter_ex = getter;
        cha_on_->setter_ex = setter;
        cha_on_->context = this;
        
        cha_speed_ = new_cha_rotation_speed();
        cha_speed_->getter_ex = getter;
        cha_speed_->setter_ex = setter;
        cha_speed_->context = this;
        
        bind_fan_->add_on_state_callback([this]() {
            this->on_esphome_state_change();
        });
    }
    
    homekit_characteristic_t* get_cha_on() { return cha_on_; }
    homekit_characteristic_t* get_cha_speed() { return cha_speed_; }
};

}}