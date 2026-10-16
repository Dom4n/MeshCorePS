#pragma once
#include <helpers/radiolib/RXPowerSaving.h>
struct ConfigTestRadio : RxPowerSavingControl {
 bool supported=true, enabled=false, invalid=false;
 uint32_t rx=0, sleep=0;
 bool setRxPowerSaving(bool on,uint32_t r,uint32_t s) override {
  if(on && !supported) return false;
  enabled=on;rx=r;sleep=s;return true;
 }
 void setRxPowerSavingConfigError(bool error) override {invalid=error;}
 RxPowerSavingStatus getRxPowerSavingStatus() const override {
  RxPowerSavingStatus s;s.supported=supported;s.armed=enabled;
  s.last_error=invalid ? RX_POWERSAVING_ERR_TIMING : 0;return s;
 }
 bool setRxBoostedGainMode(bool) {return true;}
 void setTxPower(int8_t) {}
 uint32_t getCADBusyCount() const {return 0;}
};
#define WRAPPER_CLASS ConfigTestRadio
inline ConfigTestRadio radio_driver;
