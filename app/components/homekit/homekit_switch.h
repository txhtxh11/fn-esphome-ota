#pragma once
#include "homekit_helper.h"
#include "esphome/components/switch/switch.h"
#include <string.h>

namespace esphome { namespace homekit {

class HomekitSwitch {
 protected:
    switch_::Switch* bind_switch_;
    homekit_characteristic_t* cha_on_;

    static void setter(homekit_characteristic_t *ch, homekit_value_t value) {
        HomekitSwitch* i = (HomekitSwitch*)ch->context;
        if (value.bool_value) i->bind_switch_->turn_on(); else i->bind_switch_->turn_off();
        homekit_characteristic_notify(ch, value);
    }
    
    static homekit_value_t getter(const homekit_characteristic_t *ch) {
        return HOMEKIT_BOOL_CPP(((HomekitSwitch*)ch->context)->bind_switch_->state);
    }
    
    // 关键新增：ESPHome 开关变化时通知 HomeKit
    void on_esphome_state_change(bool state) {
        // 更新特征值
        this->cha_on_->value.bool_value = state;
        // 通知 HomeKit
        homekit_characteristic_notify(this->cha_on_, this->cha_on_->value);
    }

 public:
    HomekitSwitch(switch_::Switch* s) : bind_switch_(s) {
        cha_on_ = new_cha_on();
        cha_on_->getter_ex = getter;
        cha_on_->setter_ex = setter;
        cha_on_->context = this;
        
        // 关键：监听 ESPHome 开关状态变化
        bind_switch_->add_on_state_callback([this](bool state) {
            this->on_esphome_state_change(state);
        });
    }
    
    homekit_characteristic_t* get_cha_on() { return cha_on_; }
};

}}