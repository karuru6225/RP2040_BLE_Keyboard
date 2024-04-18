#include "SerialKeyboardClient.h"

#ifdef DEBUG
void debugPrint(const char *label, const uint8_t *data, size_t size)
{
  Serial.println(label);
  Serial.print("millis: ");
  Serial.println(millis());
  Serial.print("HEX: ");
  for (int i = 0; i < size; i++)
  {
    char buf[2];
    sprintf(buf, "%02X", data[i]);
    Serial.print(buf);
  }
  Serial.println();
  // Serial.print("ASCII: ");
  // for (int i = 0; i < size; i++)
  // {
  //   if (data[i] >= 0x20 && data[i] <= 0x7E)
  //   {
  //     Serial.print((char)data[i]);
  //   }
  //   else
  //   {
  //     Serial.print(" ");
  //   }
  // }
  // Serial.println();
  Serial.println("============");
}
#endif

void packet_callback(const uint8_t *buffer, size_t size)
{

#ifdef DEBUG
  debugPrint("recieve", buffer, size);
#endif

  SerialKeyboardClient._recievePacket(buffer, size);
}

#ifdef DEBUG
void SerialKeyboardClient_::printPQueue(const char *label)
{
  Serial.print(label);
  for (int i = 0; i < PACKET_QUEUE_SIZE; i++)
  {
    char buf[4];
    if (i == pqueueHead)
    {
      sprintf(buf, "[%02X]", packetQueue[i].command);
    }
    else if (i == pqueueTail)
    {
      sprintf(buf, "<%02X>", packetQueue[i].command);
    }
    else
    {
      sprintf(buf, " %02X ", packetQueue[i].command);
    }
    Serial.print(buf);
  }
  Serial.println();
}
#endif

void SerialKeyboardClient_::_recievePacket(const uint8_t *data, size_t size)
{
  if (size < 3)
  {
    return;
  }
  NormalPacket packet;
  packet.command = (Command)data[0];
  packet.sequenceNum = data[1];
  packet.bodyLength = data[2];
  packet.body = (uint8_t *)data + 3;
  if (packet.bodyLength + 4 != size)
  {
    return;
  }

  // debugPrint("command", (uint8_t *)&packet.command, 1);
  // debugPrint("body", packet.body, packet.bodyLength);

  if (packet.command != ACK)
  {
    return;
  }

  AckPacket ack;
  ack.ackBodyLength = packet.bodyLength - 3;
  ack.requestCommand = (Command)packet.body[0];
  ack.result = (Result)packet.body[1];
  ack.body = packet.body + 2;

  if (ack.requestCommand == START)
  {
    bootAckReceived = true;
    // Serial.println("boot ack received");
  }
  else
  {
    if (packet.sequenceNum >= SEQUENCE_START)
    {
      for (int i = 0; i < SEND_WINDOW_SIZE; i++)
      {
        if (sendWindow[i] == packet.sequenceNum)
        {
          sendWindow[i] = EMPTY;
          break;
        }
      }
    }
  }

  switch (ack.requestCommand)
  {
  case GET_ADVERTISING_STATE:
    if (ack.result == SUCCESS)
    {
      if (ack.body[0] == 0)
      {
        Serial.println("Advertising stopped");
        advertising = NOT_ADVERTISING;
      }
      else
      {
        Serial.println("Advertising started");
        advertising = ADVERTISING;
      }
    }
    break;
  case START_ADV:
    if (ack.result == SUCCESS)
    {
      advertising = ADVERTISING;
    }
    break;
  case STOP_ADV:
    if (ack.result == SUCCESS)
    {
      advertising = NOT_ADVERTISING;
    }
    break;
  case GET_CONNECTION_INFO:
    if (ack.result == SUCCESS)
    {
      semaphoreAcquire();
      maxConnectionCount = ack.body[0];
      if (maxConnectionCount > MAX_CONNECTIONS)
      {
        maxConnectionCount = MAX_CONNECTIONS;
      }
      connectionState = ack.body[1];
      for (int i = 0; i < maxConnectionCount; i++)
      {
        connectionNames[i][0] = '\0';
      }
      int offset = 2;
      for (int i = 0; i < maxConnectionCount; i++)
      {
        if (offset >= ack.ackBodyLength)
        {
          break;
        }
        if (connectionState & (1 << i) == 0)
        {
          continue;
        }

        memcpy(connectionNames[i], ack.body + offset, CONNECTION_NAME_LENGTH);
        offset += CONNECTION_NAME_LENGTH;
      }
#ifdef DEBUG
      Serial.print("Max connection count: ");
      Serial.println(maxConnectionCount);
      Serial.print("Current connectionId: ");
      Serial.println(connectionID);
      for (int i = 0; i < maxConnectionCount; i++)
      {
        if (connectionNames[i][0] != '\0')
        {
          if (connectionState & (1 << i))
          {
            if (i == connectionID)
            {
              Serial.print("*");
            }
            else
            {
              Serial.print("-");
            }
          }
          else
          {
            Serial.print(" ");
          }
          Serial.print("Connection ");
          Serial.print(i);
          Serial.print(": ");
          Serial.println(connectionNames[i]);
        }
      }
#endif
      semaphoreRelease();
    }
    break;
  case SWITCH_CONNECTION:
    if (ack.result == SUCCESS)
    {
      connectionID = (uint8_t)ack.body[0];
    }
    break;
  case GET_LED_STATE:
    if (ack.result == SUCCESS)
    {
      ledState = ack.body[0];
    }
    break;
  default:
    break;
  }
}

SerialKeyboardClient_::SerialKeyboardClient_() : HID_Keyboard()
{
  packetSerial.setPacketHandler(&packet_callback);
  ledState = 0;
  connectionID = 0;
  maxConnectionCount = 0;
  connectionState = 0;
  ledStateLastUpdate = 0;
  connectionStateLastUpdate = 0;
  queueSemaphore = NULL;
  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    connectionNames[i][0] = '\0';
  }
  for (int i = 0; i < PACKET_QUEUE_SIZE; i++)
  {
    packetQueue[i].command = NOP;
    packetQueue[i].body = NULL;
  }
  for (int i = 0; i < SEND_WINDOW_SIZE; i++)
  {
    sendWindow[i] = EMPTY;
  }
}

void SerialKeyboardClient_::setSemaphore(semaphore_t *semaphore)
{
  queueSemaphore = semaphore;
}

void SerialKeyboardClient_::semaphoreAcquire(void)
{
  if (queueSemaphore == NULL)
  {
    return;
  }
  sem_acquire_blocking(queueSemaphore);
}

void SerialKeyboardClient_::semaphoreRelease(void)
{
  if (queueSemaphore == NULL)
  {
    return;
  }
  sem_release(queueSemaphore);
}

void SerialKeyboardClient_::setStream(Stream *stream)
{
  packetSerial.setStream(stream);
}

void SerialKeyboardClient_::begin()
{
  NormalPacket boot;
  boot.command = START;
  boot.bodyLength = 0;
  boot.body = NULL;
  boot.sequenceNum = 0;

  while (!bootAckReceived)
  {
    sendPacket(&boot);
    delay(1000);
    update();
  }
  delay(200);
  HID_Keyboard::begin();
  delay(200);
  update();
}

int SerialKeyboardClient_::send(void)
{
  uint8_t body[7];
  NormalPacket packet;
  packet.command = KEY_REPORT;
  packet.bodyLength = 7;
  packet.body = body;
  packet.body[0] = _keyReport.modifiers;
  memcpy(packet.body + 1, _keyReport.keys, 6);

  // sendPacket(&packet);

  while (!pushPacket(&packet))
  {
    delay(1);
    update();
  }

  // while (!sendPacket(&packet)) {
  //   delay(1);
  //   update();
  // }

  return 0;
}

bool SerialKeyboardClient_::pushPacket(NormalPacket *packet)
{
  semaphoreAcquire();
  if (pqueueHead == (pqueueTail + 1) % PACKET_QUEUE_SIZE)
  {
    semaphoreRelease();
    return false;
  }
  if (!bootAckReceived)
  {
    semaphoreRelease();
    return true;
  }

#ifdef DEBUG
  printPQueue("push ");
#endif

  NormalPacket *p = &packetQueue[pqueueTail];
  p->command = packet->command;
  p->sequenceNum = sequenceNum++;
  if (sequenceNum == 0)
  {
    sequenceNum = SEQUENCE_START;
  }
  if (packet->bodyLength == 0)
  {
    p->body = NULL;
    p->bodyLength = 0;
  }
  else
  {
    p->body = (uint8_t *)malloc(packet->bodyLength);
    p->bodyLength = packet->bodyLength;
    memcpy(p->body, packet->body, packet->bodyLength);
  }

  pqueueTail = (pqueueTail + 1) % PACKET_QUEUE_SIZE;
  semaphoreRelease();
  return true;
}

void SerialKeyboardClient_::popPacket(void)
{
  semaphoreAcquire();
  if (pqueueHead == pqueueTail)
  {
    semaphoreRelease();
    return;
  }

#ifdef DEBUG
  printPQueue("pop  ");
#endif

  NormalPacket *packet = &packetQueue[pqueueHead];
  if (!sendPacket(packet))
  {
    semaphoreRelease();
    return;
  }
  packet->command = NOP;
  free(packet->body);
  packet->body = NULL;
  pqueueHead = (pqueueHead + 1) % PACKET_QUEUE_SIZE;
  semaphoreRelease();
}

bool SerialKeyboardClient_::sendPacket(NormalPacket *packet)
{
  uint8_t i = 0;
  if (packet->sequenceNum >= SEQUENCE_START)
  {
    while (i < SEND_WINDOW_SIZE)
    {
      if (sendWindow[i] == EMPTY)
      {
        sendWindow[i] = packet->sequenceNum;
        break;
      }
      i++;
    }
    if (i == SEND_WINDOW_SIZE)
    {
      return false;
    }
  }

  uint8_t buffer[packet->bodyLength + 3];
  buffer[0] = packet->command;
  buffer[1] = packet->sequenceNum;
  buffer[2] = packet->bodyLength;
  memcpy(buffer + 3, packet->body, packet->bodyLength);
  const uint8_t m = {_keyReport.modifiers};
  uint8_t sum = 0;
  for (int i = 0; i < packet->bodyLength + 3; i++)
  {
    sum += buffer[i];
  }
  buffer[packet->bodyLength + 3] = sum;

#ifdef DEBUG
  debugPrint("keyReportkeys", (uint8_t *)_keyReport.keys, 6);
  debugPrint("send", buffer, packet->bodyLength + 4);
#endif

  packetSerial.send(buffer, packet->bodyLength + 4);

  return true;
}

void SerialKeyboardClient_::update(void)
{
  popPacket();
  packetSerial.update();
  delay(10);
  // if (millis() - lastDebugOutput > 1000)
  // {
  //   Serial.print(millis() - lastDebugOutput);
  //   Serial.print("queue: ");
  //   Serial.print(pqueueHead);
  //   Serial.print(" ");
  //   Serial.println(pqueueTail);
  //   lastDebugOutput = millis();
  // }
}

AdvertisingState SerialKeyboardClient_::getAdvertisingState(void)
{
  advertising = UNKNOWN;
  NormalPacket pac;
  pac.command = GET_ADVERTISING_STATE;
  pac.bodyLength = 0;
  pac.body = NULL;

  pushPacket(&pac);
  while (advertising == UNKNOWN)
  {
    update();
  }
  return advertising;
  // sendPacket(&pac);
}

void SerialKeyboardClient_::startAdv(void)
{
  getAdvertisingState();
  if (advertising == ADVERTISING)
  {
    return;
  }
  Serial.println("send start adv");
  NormalPacket pac;
  pac.command = START_ADV;
  pac.bodyLength = 0;
  pac.body = NULL;

  pushPacket(&pac);
  // sendPacket(&pac);
}

void SerialKeyboardClient_::stopAdv(void)
{
  getAdvertisingState();
  if (advertising == NOT_ADVERTISING)
  {
    return;
  }
  Serial.println("send stop adv");
  NormalPacket pac;
  pac.command = STOP_ADV;
  pac.bodyLength = 0;
  pac.body = NULL;

  pushPacket(&pac);
  // sendPacket(&pac);
}

void SerialKeyboardClient_::getConnectionInfo(void)
{
  NormalPacket pac;
  pac.command = GET_CONNECTION_INFO;
  pac.bodyLength = 0;
  pac.body = NULL;

  pushPacket(&pac);
  // sendPacket(&pac);
}

void SerialKeyboardClient_::switchConnection(uint8_t id)
{
  NormalPacket pac;
  pac.command = SWITCH_CONNECTION;
  pac.bodyLength = 1;
  uint8_t body[1] = {id};
  pac.body = body;

  pushPacket(&pac);

  // sendPacket(&pac);
}

void SerialKeyboardClient_::getConnectionName(uint8_t id, char *name)
{
  if (id >= maxConnectionCount)
  {
    return;
  }
  semaphoreAcquire();
  memcpy(name, connectionNames[id], CONNECTION_NAME_LENGTH);
  semaphoreRelease();
}
SerialKeyboardClient_ SerialKeyboardClient;