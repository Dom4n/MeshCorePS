#ifdef RXPS_TEST_RIG
#include "RXPSRig.h"
#include "MyMesh.h"
#include "target.h"
#include <string.h>

extern RADIO_CLASS radio;
static const uint8_t symbols[] = {6, 10, 16, 32};

void RXPSRig::begin(MyMesh& mesh) {
  _mesh = &mesh;
  auto* p = mesh.getNodePrefs();
  bool changed = strcmp(p->node_name, "RXPSTestingRig") || strcmp(p->password, "testpass") ||
      !p->disable_fwd || p->powersaving_enabled || p->rxps.enabled;
  strcpy(p->node_name, "RXPSTestingRig");
  strcpy(p->password, "testpass");
  p->disable_fwd = 1;
  p->powersaving_enabled = 0;
  p->rxps.enabled = 0;
  if (changed) mesh.savePrefs();
  p->getRadioPrefs()->applyRxPowerSaving(p->sf, p->bw);
  char reply[160];
  if (p->advert_interval) { char c[] = "set advert.interval 0"; mesh.handleCommand(0, c, reply); }
  if (p->flood_advert_interval) { char c[] = "set flood.advert.interval 0"; mesh.handleCommand(0, c, reply); }
  user_btn.begin();
  display.turnOn();
  render();
}

void RXPSRig::render() {
  char line[48];
  display.startFrame();
  display.setTextSize(1);
  display.setColor(UIColor::primary_txt);
  display.setCursor(0, 0); display.print("RXPSTestingRig");
  auto* p = _mesh->getNodePrefs();
  snprintf(line, sizeof(line), "SF%u BW%.1f TX%d", p->sf, (double)p->bw, (int)p->tx_power_dbm);
  display.setCursor(0, 13); display.print(line);
  snprintf(line, sizeof(line), "%u symbols / no data", symbols[_mode]);
  display.setCursor(0, 25); display.print(line);
  display.setTextSize(2);
  display.setCursor(0, 37);
  display.print(_state == WAITING ? "waiting" : _state == SENDING ? "sending" : _state == SENT ? "sent" : "error");
  display.setTextSize(1);
  display.setCursor(0, 56);
  if (_state == ERROR) { snprintf(line, sizeof(line), "Radio error %d", _error); display.print(line); }
  else display.print("Tap:send Hold:mode");
  display.endFrame();
}

void RXPSRig::send() {
  if (_mesh->isTransmitting()) return;
  auto* p = _mesh->getNodePrefs();
  const uint32_t duration = (uint32_t)(((uint64_t)symbols[_mode] * (1UL << p->sf) * 1000UL) / p->bw);
  _state = SENDING; render();
  radio_driver.idle();
  // No sync word or payload can be reached within a pulse of at most 32 symbols.
  _error = radio.setPreambleLength(65535);
  bool hooked = false;
  if (_error == RADIOLIB_ERR_NONE) {
    board.onBeforeTransmit(); hooked = true;
    _error = radio.startRigPreamble(duration);
    if (_error == RADIOLIB_ERR_NONE) {
      // RadioLib returns after BUSY falls: PA ramp-up done, preamble started.
      const uint32_t started = micros();
      while ((uint32_t)(micros() - started) < duration) { }
    }
  }
  if (_error == RADIOLIB_ERR_NONE && radio.checkIrq(RADIOLIB_IRQ_TIMEOUT)) _error = RADIOLIB_ERR_TX_TIMEOUT;
  const int16_t stopped = radio.finishTransmit();
  if (_error == RADIOLIB_ERR_NONE) _error = stopped;
  if (hooked) board.onAfterTransmit();
  const int16_t restored = radio.setPreambleLength(RadioLibWrapper::preambleLengthForSF(p->sf));
  if (_error == RADIOLIB_ERR_NONE) _error = restored;
  radio_driver.idle();
  _sent_at = millis();
  _state = _error == RADIOLIB_ERR_NONE ? SENT : ERROR;
  render();
  Serial.printf("Rig: symbols=%u nominal_us=%lu result=%d\n", symbols[_mode], (unsigned long)duration, _error);
}

void RXPSRig::loop() {
  // Keep this dedicated generator awake and out of packet forwarding.
  _mesh->getNodePrefs()->powersaving_enabled = 0;
  _mesh->getNodePrefs()->disable_fwd = 1;
  const int event = user_btn.check();
  if (_state != WAITING) {
    if ((uint32_t)(millis() - _sent_at) >= 1000) { _state = WAITING; render(); }
    return;
  }
  if (event == BUTTON_EVENT_LONG_PRESS) {
    user_btn.cancelClick(); _mode = (_mode + 1) % 4; render();
  } else if (event == BUTTON_EVENT_CLICK) send();
}
#endif
