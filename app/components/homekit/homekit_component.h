#pragma once
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/fan/fan.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "homekit_switch.h"
#include "homekit_fan.h"
#include "homekit_light.h"
#include "homekit_sensor.h"
#include "homekit_binary_sensor.h"
#include "homekit_helper.h"
#include <vector>
#include <string>

namespace esphome {
namespace homekit {

class HomekitComponent : public PollingComponent {
 public:
  HomekitComponent() : PollingComponent(2000) {}

  void set_setup_code(const char* sc);
  void set_name(const char* n) { this->name_ = n; }
  
  // 所有add方法都是累积式的，YAML中没有的类型不会调用，vector保持为空
  void add_switch(switch_::Switch* s) { this->switches_.push_back(s); }
  void add_fan(fan::Fan* f) { this->fans_.push_back(f); }
  void add_light(light::LightState* l) { this->lights_.push_back(l); }
  void add_sensor(sensor::Sensor* s, const char* type) { this->sensors_.push_back({s, type}); }
  void add_binary_sensor(binary_sensor::BinarySensor* s, const char* type) { this->bin_sensors_.push_back({s, type}); }

  std::string get_setup_code() { return this->generated_code_; }

  void reset_storage();
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

 protected:
  const char* name_;
  std::string setup_id_;
  std::string generated_code_;
  bool started_ = false;
  
  // 这些vector初始都是空的，YAML中配了才会填充
  std::vector<switch_::Switch*> switches_;
  std::vector<HomekitSwitch*> homekit_switches_;
  std::vector<fan::Fan*> fans_;
  std::vector<HomekitFan*> homekit_fans_;
  std::vector<light::LightState*> lights_;
  std::vector<HomekitLight*> homekit_lights_;
  std::vector<std::pair<sensor::Sensor*, const char*>> sensors_;
  std::vector<HomekitSensor*> homekit_sensors_;
  std::vector<std::pair<binary_sensor::BinarySensor*, const char*>> bin_sensors_;
  std::vector<HomekitBinarySensor*> homekit_bin_sensors_;
  std::vector<homekit_accessory_t*> accessories_;
};

template<typename... Ts> class HomekitResetAction : public Action<Ts...> {
 public:
  HomekitResetAction(HomekitComponent *p) : p_(p) {}
  void play(Ts... x) override { p_->reset_storage(); }
  HomekitComponent *p_;
};

} 
}