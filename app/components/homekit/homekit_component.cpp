#include "homekit_component.h"
#include "esphome/core/application.h"
#include <arduino_homekit_server.h>
#include "esphome/core/log.h"

extern "C" void homekit_server_reset();

namespace esphome {
namespace homekit {

static const char *const TAG = "homekit";

void HomekitComponent::setup() {
  std::string mac_str = get_mac_address();
  this->setup_id_ = mac_str.length() >= 4 ? mac_str.substr(mac_str.length() - 4) : "ESPH";
  for (auto &c : this->setup_id_) c = toupper(c);

  if (this->generated_code_.empty()) {
    uint8_t mac[6];
    esphome::get_mac_address_raw(mac);
    uint32_t seed = (mac[3] << 16) | (mac[4] << 8) | mac[5];
    srand(seed);
    
    char code_buf[12];
    sprintf(code_buf, "%03d-%02d-%03d", rand()%900+100, rand()%90+10, rand()%900+100);
    this->generated_code_ = std::string(code_buf);
  }

  this->started_ = false;
}

void HomekitComponent::update() {
  // 检查是否所有设备都为空，如果都为空则不启动
  bool no_switches = this->switches_.empty();
  bool no_fans = this->fans_.empty();
  bool no_lights = this->lights_.empty();
  bool no_sensors = this->sensors_.empty();
  bool no_bin_sensors = this->bin_sensors_.empty();
  
  if (this->started_ || (no_switches && no_fans && no_lights && no_sensors && no_bin_sensors)) {
    return;
  }

  uint32_t aid = 1;
  
  // 1. 处理开关（可选）
  for (auto *obj : this->switches_) {
    HomekitSwitch* hk = new HomekitSwitch(obj);
    this->homekit_switches_.push_back(hk);
    this->accessories_.push_back(new_switch_accessory(aid++, obj->get_name().c_str(), hk->get_cha_on()));
  }
  
  // 2. 处理风扇（可选）
  for (auto *obj : this->fans_) {
    HomekitFan* hk = new HomekitFan(obj);
    this->homekit_fans_.push_back(hk);
    this->accessories_.push_back(new_fan_accessory(aid++, obj->get_name().c_str(), 
        hk->get_cha_on(), hk->get_cha_speed()));
  }
  
  // 3. 处理灯（可选）
  for (auto *obj : this->lights_) {
    HomekitLight* hk = new HomekitLight(obj);
    this->homekit_lights_.push_back(hk);
    this->accessories_.push_back(new_light_accessory(aid++, obj->get_name().c_str(), 
        hk->get_cha_on(), hk->get_cha_bright()));
  }
  
  // 4. 处理数值传感器（温度/湿度/光照）（可选）
  for (auto& pair : this->sensors_) {
    sensor::Sensor* obj = pair.first;
    const char* type = pair.second;
    HomekitSensor* hk = new HomekitSensor(obj, type);
    this->homekit_sensors_.push_back(hk);
    this->accessories_.push_back(new_sensor_accessory(aid++, obj->get_name().c_str(), 
        hk->get_cha_value(), type));
  }
  
  // 5. 处理二进制传感器（门磁/人体等）（可选）
  for (auto& pair : this->bin_sensors_) {
    binary_sensor::BinarySensor* obj = pair.first;
    const char* type = pair.second;
    HomekitBinarySensor* hk = new HomekitBinarySensor(obj, type);
    this->homekit_bin_sensors_.push_back(hk);
    this->accessories_.push_back(new_binary_sensor_accessory(aid++, obj->get_name().c_str(), 
        hk->get_cha_detected(), type));
  }
  
  // 启动 HomeKit 服务
  if (!this->accessories_.empty()) {
    this->accessories_.push_back(NULL);
    static homekit_server_config_t config;
    config.accessories = this->accessories_.data();
    config.password = (char*)this->generated_code_.c_str();
    config.setupId = (char*)this->setup_id_.c_str();
    
    arduino_homekit_setup(&config);
    this->started_ = true;
    ESP_LOGI(TAG, "HomeKit Started. Code: %s, SW:%d F:%d L:%d SE:%d B:%d", 
        config.password, this->switches_.size(), this->fans_.size(), 
        this->lights_.size(), this->sensors_.size(), this->bin_sensors_.size());
  }
}

void HomekitComponent::loop() {
  if (this->started_) {
    arduino_homekit_loop();
    
    // 轮询检查灯状态（如果配了灯）
    for (auto* light : this->homekit_lights_) {
      light->tick();
    }
    
    // 轮询检查数值传感器状态（如果配了传感器）
    for (auto* sensor : this->homekit_sensors_) {
      sensor->tick();
    }
    
    // 关键新增：轮询检查二进制传感器状态（防止回调丢失）
    for (auto* bin_sensor : this->homekit_bin_sensors_) {
      bin_sensor->tick();
    }
  }
}

void HomekitComponent::set_setup_code(const char* sc) {
  if (sc && strlen(sc) > 0) {
    this->generated_code_ = std::string(sc);
  }
}

void HomekitComponent::reset_storage() {
  homekit_server_reset();
  App.reboot();
}

void HomekitComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Homekit Setup Code: %s", this->generated_code_.c_str());
}

} 
}