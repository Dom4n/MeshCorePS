#pragma once
#ifdef RXPS_TEST_RIG
#include <stdint.h>
class MyMesh;
class RXPSRig {
  MyMesh* _mesh = nullptr;
  uint8_t _mode = 0;
  enum State { WAITING, SENDING, SENT, ERROR } _state = WAITING;
  uint32_t _sent_at = 0;
  int16_t _error = 0;
  void render();
  void send();
public:
  void begin(MyMesh& mesh);
  void loop();
};
#endif
