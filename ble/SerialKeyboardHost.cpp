#include <bluefruit.h>
#include "SerialKeyboardHost.h"

BLEDis bledis;
BLEHidAdafruit blehid;

void indicateDebug(void)
{
  digitalWrite(11, HIGH);
  delay(200);
  digitalWrite(11, LOW);
  delay(200);
  digitalWrite(11, HIGH);
  delay(100);
  digitalWrite(11, LOW);
  delay(100);
  digitalWrite(11, HIGH);
  delay(200);
  digitalWrite(11, LOW);
}

void SerialKeyboardHost_::_connect(uint16_t conn_handle)
{
  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    if (conn_handles[i] == DISCONNECTED)
    {
      conn_handles[i] = conn_handle;
      if (current_conn_idx == -1)
      {
        current_conn_idx = i;
      }
      // indicateDebug();
      respondConnectionInfo(0);
      if (getConnectionCount() < MAX_CONNECTIONS)
      {
        startAdv();
      }
      else
      {
        stopAdv();
      }
      return;
    }
  }
}

void SerialKeyboardHost_::_disconnect(uint16_t conn_handle)
{
  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    if (conn_handles[i] == conn_handle)
    {
      conn_handles[i] = DISCONNECTED;
      startAdv();
      respondConnectionInfo(0);
      return;
    }
  }
}

void SerialKeyboardHost_::_setLedState(uint16_t conn_handle, uint8_t led_state)
{
  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    if (conn_handles[i] == conn_handle)
    {
      ledState[i] = led_state;
      return;
    }
  }
}

void packet_callback(const uint8_t *buffer, size_t size)
{
  SerialKeyboardHost._recievePacket(buffer, size);
}

void led_callback(uint16_t conn_handle, uint8_t led_state)
{
  SerialKeyboardHost._setLedState(conn_handle, led_state);
}

void connect_callback(uint16_t conn_handle)
{
  SerialKeyboardHost._connect(conn_handle);
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  SerialKeyboardHost._disconnect(conn_handle);
}

SerialKeyboardHost_::SerialKeyboardHost_() : HID_Keyboard()
{
  packetSerial.setPacketHandler(&packet_callback);
}

void SerialKeyboardHost_::setStream(Stream *stream)
{
  packetSerial.setStream(stream);
}

int sendDebug = 0;

void SerialKeyboardHost_::update(void)
{
  packetSerial.update();
  popPacket();
}

void SerialKeyboardHost_::begin()
{
  HID_Keyboard::begin();
  initBle();

  pinMode(11, OUTPUT);

  // indicateDebug();
}

void SerialKeyboardHost_::initBle(void)
{
  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    conn_handles[i] = DISCONNECTED;
    ledState[i] = 0;
  }

  Bluefruit.autoConnLed(false);
  Bluefruit.configAttrTableSize(1280);
  Bluefruit.begin(MAX_CONNECTIONS, 0);
  // Bluefruit.begin();

  Bluefruit.setName("SerialKeyboard");
  Bluefruit.setTxPower(4); // Check bluefruit.h for supported values

  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather52");
  bledis.begin();

  blehid.begin();

  blehid.setKeyboardLedCallback(led_callback);
}

void SerialKeyboardHost_::respondAck(AckPacket *ack)
{
  uint8_t buf[6 + ack->ackBodyLength];
  buf[0] = ACK;
  buf[1] = ack->sequenceNum;
  buf[2] = ack->ackBodyLength + 2;
  buf[3] = ack->requestCommand;
  buf[4] = ack->result;
  if (ack->ackBodyLength > 0)
  {
    memcpy(buf + 5, ack->body, ack->ackBodyLength);
  }
  uint8_t sum = 0;
  for (int i = 0; i < 5 + ack->ackBodyLength; i++)
  {
    sum += buf[i];
  }
  buf[5 + ack->ackBodyLength] = sum;

  packetSerial.send(buf, 6 + ack->ackBodyLength);
}

void SerialKeyboardHost_::respondSuccess(Command requestCommand, uint8_t sequenceNum)
{
  AckPacket ack;
  ack.ackBodyLength = 1;
  ack.requestCommand = requestCommand;
  ack.sequenceNum = sequenceNum;
  ack.result = SUCCESS;
  // ack.body = NULL;
  uint8_t body[1] = {static_cast<uint8_t>((pqueueHead - pqueueTail + PACKET_QUEUE_SIZE) % PACKET_QUEUE_SIZE)};
  ack.body = body;
  respondAck(&ack);
}

void SerialKeyboardHost_::respondFailure(Command requestCommand, uint8_t sequenceNum)
{
  AckPacket ack;
  ack.ackBodyLength = 1;
  ack.requestCommand = requestCommand;
  ack.sequenceNum = sequenceNum;
  ack.result = FAILURE;
  uint8_t body[1] = {static_cast<uint8_t>((pqueueHead - pqueueTail + PACKET_QUEUE_SIZE) % PACKET_QUEUE_SIZE)};
  ack.body = body;
  respondAck(&ack);
}

bool SerialKeyboardHost_::pushPacket(NormalPacket *packet)
{
  if ((pqueueTail + 1) % PACKET_QUEUE_SIZE == pqueueHead)
  {
    return false;
  }
  NormalPacket *p = &packetQueue[pqueueTail];

  p->command = packet->command;
  p->sequenceNum = packet->sequenceNum;
  p->bodyLength = packet->bodyLength;
  p->body = (uint8_t *)malloc(packet->bodyLength);
  memcpy(p->body, packet->body, packet->bodyLength);
  pqueueTail = (pqueueTail + 1) % PACKET_QUEUE_SIZE;
  return true;
}

void SerialKeyboardHost_::popPacket(void)
{
  if (pqueueHead == pqueueTail)
  {
    return;
  }
  NormalPacket packet = packetQueue[pqueueHead];
  processPacket(&packet);
  free(packet.body);
  packet.body = NULL;
  pqueueHead = (pqueueHead + 1) % PACKET_QUEUE_SIZE;
}

void SerialKeyboardHost_::respondConnectionInfo(uint8_t sequenceNum)
{
  AckPacket ack;

  uint8_t connBitmap = 0;

  // uint8_t macAddresses[MAX_CONNECTIONS * MAC_ADDRESS_LENGTH];
  // char names[MAX_CONNECTIONS * CONNECTION_NAME_LENGTH];
  ConnectionInfo connInfo[MAX_CONNECTIONS];
  memset(connInfo, 0, MAX_CONNECTIONS * (MAC_ADDRESS_LENGTH + CONNECTION_NAME_LENGTH));
  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    // memset(names + i * CONNECTION_NAME_LENGTH, 0, CONNECTION_NAME_LENGTH);
    // memset(macAddresses + i * MAC_ADDRESS_LENGTH, 0, MAC_ADDRESS_LENGTH);
    if (conn_handles[i] != DISCONNECTED)
    {
      connBitmap |= (1 << i);
      BLEConnection *conn = Bluefruit.Connection((uint16_t)conn_handles[i]);
      if (conn)
      {
        // uint16_t nameLength = conn->getPeerName(names + i * CONNECTION_NAME_LENGTH, CONNECTION_NAME_LENGTH - 1);
        // names[i * CONNECTION_NAME_LENGTH + nameLength] = '\0';
        // conn->getPeerAddr(macAddresses + i * MAC_ADDRESS_LENGTH);
        ble_gap_addr_t gap = conn->getPeerAddr();
        memcpy(connInfo[i].mac, gap.addr, MAC_ADDRESS_LENGTH);
        conn->getPeerName(connInfo[i].name, CONNECTION_NAME_LENGTH - 1);
      }
      else
      {
        // names[i * CONNECTION_NAME_LENGTH] = '\0';
      }
    }
    else
    {
      // names[i * CONNECTION_NAME_LENGTH] = '\0';
    }
  }

  uint8_t body[2 + MAX_CONNECTIONS * (MAC_ADDRESS_LENGTH + CONNECTION_NAME_LENGTH)];

  body[0] = MAX_CONNECTIONS;
  body[1] = connBitmap;
  memcpy(body + 2, connInfo, MAX_CONNECTIONS * (MAC_ADDRESS_LENGTH + CONNECTION_NAME_LENGTH));

  ack.ackBodyLength = sizeof(body);
  ack.result = SUCCESS;
  ack.requestCommand = GET_CONNECTION_INFO;
  ack.sequenceNum = sequenceNum;
  ack.body = body;
  respondAck(&ack);
}

void SerialKeyboardHost_::processPacket(NormalPacket *packet)
{
  digitalWrite(11, HIGH);
  switch (packet->command)
  {
  case NOP:
  {
    respondSuccess(NOP, packet->sequenceNum);
  }
  case START:
  {
    respondSuccess(START, packet->sequenceNum);
  }
  break;
  case GET_ADVERTISING_STATE:
  {
    AckPacket ack;
    ack.ackBodyLength = 1;
    ack.requestCommand = GET_ADVERTISING_STATE;
    ack.sequenceNum = packet->sequenceNum;
    ack.result = SUCCESS;
    uint8_t body[1] = {static_cast<uint8_t>(Bluefruit.Advertising.isRunning() ? 1 : 0)};
    ack.body = body;
    respondAck(&ack);
  }
  break;
  case START_ADV:
  {
    startAdv();
    respondSuccess(START_ADV, packet->sequenceNum);
  }
  break;
  case STOP_ADV:
  {
    stopAdv();
    respondSuccess(STOP_ADV, packet->sequenceNum);
  }
  case GET_CONNECTION_INFO:
  {
    respondConnectionInfo(packet->sequenceNum);
    // uint8_t body[2 + MAX_CONNECTIONS * 6];
    // for (int i = 0; i < MAX_CONNECTIONS; i++) {
    //   if (conn_handles[i] == DISCONNECTED) {
    //     memset(body + 2 + i * 6, 0, 6);
    //   } else {
    //     connBitmap |= (1 << i);
    //     BLEConnection* conn = Bluefruit.Connection((uint16_t)conn_handles[i]);
    //     if (conn) {
    //       ble_gap_addr_t addr = conn->getPeerAddr();
    //       memcpy(body + 2 + i * 6, addr.addr, 6);
    //     }
    //   }
    // }
    // ack.requestCommand = GET_CONNECTION_INFO;
    // ack.ackBodyLength = (uint8_t)(2 + MAX_CONNECTIONS * 6);
    // ack.result = SUCCESS;
    // body[0] = MAX_CONNECTIONS;
    // body[1] = connBitmap;
    // ack.body = body;
    // respondAck(&ack);
  }
  break;
  case SWITCH_CONNECTION:
  {
    if (packet->bodyLength < 1)
    {
      respondFailure(SWITCH_CONNECTION, packet->sequenceNum);
      return;
    }
    uint8_t connectionId = packet->body[0];
    if (connectionId < MAX_CONNECTIONS && conn_handles[connectionId] != DISCONNECTED)
    {
      _keyReport.modifiers = (uint8_t)0;
      _keyReport.consumer = (ConsumerUsageId)0;
      for (int i = 0; i < 6; i++)
      {
        _keyReport.keys[i] = (KeyboardUsageId)0;
      }
      send();
      current_conn_idx = connectionId;
      AckPacket ack;
      ack.ackBodyLength = 1;
      ack.requestCommand = SWITCH_CONNECTION;
      ack.result = SUCCESS;
      ack.sequenceNum = packet->sequenceNum;
      uint8_t body[1] = {connectionId};
      ack.body = body;
      respondAck(&ack);
    }
    else
    {
      respondFailure(SWITCH_CONNECTION, packet->sequenceNum);
    }
  }
  break;
  case GET_LED_STATE:
  {
    AckPacket ack;
    ack.ackBodyLength = 1;
    ack.requestCommand = GET_LED_STATE;
    ack.result = SUCCESS;
    ack.sequenceNum = packet->sequenceNum;
    uint8_t body[1];
    body[0] = ledState[current_conn_idx];
    ack.body = body;
    respondAck(&ack);
  }
  break;
  case KEY_REPORT:
  {
    if (packet->bodyLength < 1)
    {
      respondFailure(KEY_REPORT, packet->sequenceNum);
      return;
    }
    if (getConnectionCount() == 0)
    {
      indicateDebug();
      respondFailure(KEY_REPORT, packet->sequenceNum);
      return;
    }
    _keyReport.modifiers = packet->body[0];
    if (packet->bodyLength > 1)
    {
      memcpy(_keyReport.keys, packet->body + 1, packet->bodyLength - 1);
    }
    send();
    respondSuccess(KEY_REPORT, packet->sequenceNum);
  }
  break;
  case PUSH_CONSUMER:
  {
    // todo
    respondSuccess(PUSH_CONSUMER, packet->sequenceNum);
  }
  break;
  case RELEASE_CONSUMER:
  {
    // todo
    respondSuccess(RELEASE_CONSUMER, packet->sequenceNum);
  }
  break;
  default:
    respondFailure(NOP, packet->sequenceNum);
    break;
  }
  digitalWrite(11, LOW);
}

void SerialKeyboardHost_::_recievePacket(const uint8_t *data, size_t size)
{
  if (size < 4)
  {
    // respondFailure(NOP, 0);
    return;
  }
  NormalPacket packet;
  packet.command = (Command)data[0];
  packet.sequenceNum = data[1];
  packet.bodyLength = data[2];
  packet.body = (uint8_t *)data + PACKET_HEADER_SIZE;

  uint8_t sum = 0;
  for (int i = 0; i < size - 1; i++)
  {
    sum += data[i];
  }

  if (sum != data[size - 1])
  {
    AckPacket ack;
    ack.ackBodyLength = 0;
    ack.requestCommand = packet.command;
    ack.sequenceNum = packet.sequenceNum;
    ack.result = CHECKSUM_ERROR;
    ack.body = NULL;
    // uint8_t body[2] = {sum, data[size - 1]};
    // ack.body = body;
    respondAck(&ack);
    return;
  }

  while (!pushPacket(&packet))
  {
    update();
  }
}

void SerialKeyboardHost_::startAdv(void)
{
  if (getConnectionCount() >= MAX_CONNECTIONS)
  {
    return;
  }
  if (Bluefruit.Advertising.isRunning())
  {
    return;
  }
  if (!ble_adv_initialized)
  {
    ble_adv_initialized = true;
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);

    Bluefruit.Advertising.addService(blehid);
    Bluefruit.Advertising.addName();

    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244); // in unit of 0.625 ms
    Bluefruit.Advertising.setFastTimeout(30);   // number of seconds in fast mode
  }
  Bluefruit.Advertising.start(0);
}

void SerialKeyboardHost_::stopAdv(void)
{
  Bluefruit.Advertising.stop();
}

void SerialKeyboardHost_::switchConnection(uint8_t idx)
{
  if (idx < MAX_CONNECTIONS)
  {
    current_conn_idx = idx;
  }
}

bool SerialKeyboardHost_::isConnected(uint8_t idx)
{
  if (idx < MAX_CONNECTIONS)
  {
    return conn_handles[idx] != DISCONNECTED;
  }
  return false;
}

void SerialKeyboardHost_::disconnect(uint8_t idx)
{
  if (idx < MAX_CONNECTIONS)
  {
    if (conn_handles[idx] != DISCONNECTED)
    {
      BLEConnection *conn = Bluefruit.Connection((uint16_t)conn_handles[idx]);
      conn->disconnect();
    }
  }
}

uint8_t SerialKeyboardHost_::getConnectionCount(void)
{
  uint8_t count = 0;
  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    if (conn_handles[i] != DISCONNECTED)
    {
      count++;
    }
  }
  return count;
}

int SerialKeyboardHost_::send(void)
{
  if (conn_handles[current_conn_idx] == DISCONNECTED)
  {
    return -1;
  }

  blehid.keyboardReport(conn_handles[current_conn_idx], reinterpret_cast<hid_keyboard_report_t *>(&_keyReport));

  return 0;
}

SerialKeyboardHost_ SerialKeyboardHost;