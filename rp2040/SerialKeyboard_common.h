#pragma once

#define MAX_CONNECTIONS 3
#define CONNECTION_NAME_LENGTH 32
#define DISCONNECTED -1
#define PACKET_HEADER_SIZE 3

typedef enum _Command : uint8_t
{
  NOP = 0x00,

  START = 0x01,

  GET_ADVERTISING_STATE = 0x10,
  START_ADV = 0x11,
  STOP_ADV = 0x12,

  GET_CONNECTION_INFO = 0x20,
  SWITCH_CONNECTION = 0x21,

  KEY_REPORT = 0x30,

  PUSH_CONSUMER = 0x40,
  RELEASE_CONSUMER,

  GET_LED_STATE = 0x50,

  ACK = 0xFF
} Command;

typedef enum _Result : uint8_t
{
  SUCCESS = 0x00,
  FAILURE,
  CHECKSUM_ERROR,
} Result;

typedef enum _LED : uint8_t
{
  NUMLOCK = 0x01,
  CAPSLOCK = 0x02,
  SCROLLLOCK = 0x04,
  COMPOSE = 0x08,
  KANA = 0x10
} LED;

/*
Key Switch <-> Client/pico <- PacketSerial -> Host/ISP1507 <-> BLE

Packet Structure

| Command | sequence num | bodyLength | Body |

0x00 - 0x7F Client -> Host
0x80 - 0xEF Reserved
0xF0 - 0xFF Host -> Client

NOP
Command Body -> none
ACK BODY -> none

GET_CONNECTION_INFO
Command Body -> none
ACK BODY ->
| Max Connection Count | Connection State bitmap | Connection1 Name \0 | ... |

SWITCH_CONNECTION
Command Body -> | Connection ID |
ACK BODY -> | Connection ID |

GET_LED_STATE
Command Body -> none
ACK BODY -> | LED State |

KEY_REPORT
Command Body -> | Modifier | Key1 | Key2 | ... |
ACK BODY -> none

PUSH_CONSUMER / RELEASE_CONSUMER
Command Body -> | Consumer Code |
ACK BODY -> none

ACK
Command Body -> | Request Command | Result | ACK Body |

*/

struct NormalPacket
{
  Command command;
  uint8_t sequenceNum;
  uint8_t bodyLength;
  uint8_t *body;
};

struct AckPacket
{
  uint8_t ackBodyLength;
  Command requestCommand;
  Result result;
  uint8_t sequenceNum;
  uint8_t *body;
};
