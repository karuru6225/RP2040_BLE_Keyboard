#pragma once

#include <Arduino.h>
#include <pico/multicore.h>
#include <PacketSerial.h>
#include "SerialKeyboard_common.h"
#include "HID_Keyboard.h"

// #define DEBUG
#define PACKET_QUEUE_SIZE 16
#define SEND_WINDOW_SIZE 1
#define SEQUENCE_START 4

enum SpecialSequenceNum : uint8_t
{
  EMPTY = 0,
};

enum AdvertisingState : uint8_t
{
  UNKNOWN = 0,
  ADVERTISING,
  NOT_ADVERTISING,
};

class SerialKeyboardClient_ : public HID_Keyboard
{
private:
  PacketSerial packetSerial;
  uint8_t ledState;
  uint8_t connectionID;
  uint8_t maxConnectionCount;
  uint8_t connectionState;
  unsigned long ledStateLastUpdate;
  unsigned long connectionStateLastUpdate;
  unsigned long lastDebugOutput = 0;
  semaphore_t *queueSemaphore;
  AdvertisingState advertising = UNKNOWN;
  char connectionNames[MAX_CONNECTIONS][CONNECTION_NAME_LENGTH];
  uint8_t pqueueHead = 0;
  uint8_t pqueueTail = 0;
  uint8_t sequenceNum = SEQUENCE_START;
  uint8_t sendWindow[SEND_WINDOW_SIZE];
  bool bootAckReceived = false;
  NormalPacket packetQueue[PACKET_QUEUE_SIZE];
  bool pushPacket(NormalPacket *);
  void popPacket(void);
  bool sendPacket(NormalPacket *);
  uint8_t getSendingCount(void);
  void semaphoreAcquire(void);
  void semaphoreRelease(void);

#ifdef DEBUG
  void printPQueue(const char *);
#endif

public:
  SerialKeyboardClient_();
  void setStream(Stream *);
  void begin();
  void _recievePacket(const uint8_t *data, size_t size);
  void update(void);
  AdvertisingState getAdvertisingState(void);
  void startAdv(void);
  void stopAdv(void);
  void getConnectionInfo(void);
  void switchConnection(uint8_t);
  virtual int send(void);
  void getConnectionName(uint8_t, char *);
  void setSemaphore(semaphore_t *);
};

extern SerialKeyboardClient_ SerialKeyboardClient;