#pragma once

#include <Arduino.h>
#include <PacketSerial.h>
#include <bluefruit.h>
#include "SerialKeyboard_common.h"
#include "HID_Keyboard.h"

#define PACKET_QUEUE_SIZE ((uint8_t)4)

class SerialKeyboardHost_ : public HID_Keyboard
{
private:
  PacketSerial packetSerial;
  uint8_t ledState[MAX_CONNECTIONS];
  int16_t conn_handles[MAX_CONNECTIONS];
  int8_t current_conn_idx = -1;
  bool ble_adv_initialized = false;
  virtual int send(void);
  void respondAck(AckPacket *);
  void respondFailure(Command requestCommand, uint8_t sequenceNum);
  void respondSuccess(Command requestCommand, uint8_t sequenceNum);
  void initBle(void);
  uint8_t pqueueHead = 0;
  uint8_t pqueueTail = 0;
  NormalPacket packetQueue[PACKET_QUEUE_SIZE];
  bool pushPacket(NormalPacket *packet);
  void popPacket(void);
  void processPacket(NormalPacket *packet);
  void respondConnectionInfo(uint8_t sequenceNum);

public:
  SerialKeyboardHost_();
  void setStream(Stream *);
  void update(void);

  void begin();
  void _recievePacket(const uint8_t *data, size_t size);

  void startAdv(void);
  void stopAdv(void);

  void switchConnection(uint8_t idx);
  bool isConnected(uint8_t idx);
  void disconnect(uint8_t idx);
  uint8_t getConnectionCount(void);

  void _connect(uint16_t conn_handle);
  void _disconnect(uint16_t conn_handle);
  void _setLedState(uint16_t conn_handle, uint8_t led_state);
};

extern SerialKeyboardHost_ SerialKeyboardHost;