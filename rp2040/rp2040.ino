#include "SerialKeyboardClient.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "FreeSansBold7pt7b.h"
#define FONT_DEF FreeSansBold7pt7b
#define FONT_OFFSET_Y 10

#define BLE_RESET 4
#define TX 0
#define RX 1
#define SDL 2
#define SCL 3

#define SHOW_CONNECTION_TIMEOUT 5000
#define CONN_INFO_BG_UPDATE_INTERVAL 2000
#define CONN_INFO_FG_UPDATE_INTERVAL 500

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define CONN_IND_WIDTH (SCREEN_WIDTH / MAX_CONNECTIONS)

#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define BAUDRATE 115200

#define MAX_UPDATE_WAIT 500
#define INITIAL_UPDATE_WAIT 10

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);

SerialPIO Serial3(TX, RX, 256);

pin_size_t cols[] = {28, 27, 26};
pin_size_t rows[] = {10, 9, 8};

static semaphore_t queueSem;

typedef struct _DisplayInfo
{
  ConnectionInfo *connectionInfo;
  uint8_t batteryLevel;
  bool writerReady;
} DisplayInfo;

DisplayInfo displayInfo;

void resetBLEModule()
{
  pinMode(BLE_RESET, OUTPUT);
  digitalWrite(BLE_RESET, LOW);
  delay(200);
  digitalWrite(BLE_RESET, HIGH);
}

void setup()
{
  Serial.begin(BAUDRATE);
  delay(1000);
  sem_init(&queueSem, 1, 1);

  resetBLEModule();

  Serial3.begin(BAUDRATE);
  SerialKeyboardClient.setStream(&Serial3);
  SerialKeyboardClient.setSemaphore(&queueSem);

  Serial.println("Com initialized");

  for (int i = 0; i < sizeof(cols); i++)
  {
    pinMode(cols[i], OUTPUT);
    digitalWrite(cols[i], LOW);
  }

  for (int i = 0; i < sizeof(rows); i++)
  {
    pinMode(rows[i], INPUT_PULLDOWN);
  }

  Serial.println("Matrix initialized");

  // NormalPacket pac;
  // pac.command = NOP;
  // pac.bodyLength = 0;
  // pac.body = NULL;

  // SerialKeyboardClient.send(&pac);
  // delay(2000);
  SerialKeyboardClient.begin();
  delay(3000);
  SerialKeyboardClient.update();
  delay(2000);
  // SerialKeyboardClient.startAdv();
  // delay(1000);
}

void setup1()
{
  delay(500);
  Wire1.setSDA(SDL);
  Wire1.setSCL(SCL);
  Wire1.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(0x1);
  display.clearDisplay();
  display.setFont(&FONT_DEF);
  display.setTextWrap(false);
  display.display();
  Serial.println("display started");
}

// const char* str = "TestInput !\"#$%&'()-=^~\\|@`[{;+:*]},<.>/?_";

unsigned long loop0LastTime = 0;
unsigned long interval = 30;
unsigned long updateWait = 10;

bool writerMode = false;

void loop()
{
  if (writerMode)
  {
    return;
  }
  if (SerialKeyboardClient.update())
  {
    updateWait = INITIAL_UPDATE_WAIT;
  }
  else
  {
    updateWait += 50;
    if (updateWait > MAX_UPDATE_WAIT)
    {
      updateWait = MAX_UPDATE_WAIT;
    }
  }
  delay(updateWait);

  unsigned long currentMillis = millis();
  if (currentMillis - loop0LastTime < interval)
  {
    return;
  }
  loop0LastTime = currentMillis;

  KeyboardUsageId idxKeycodeMap[3][3] = {
      {KEY_A, KEY_B, KEY_C},
      {KEY_LEFT_ALT, KEY_NONE, KEY_LEFT_GUI},
      {KEY_G, KEY_H, KEY_NONE}};
  size_t changeKey = 0;

  for (size_t colIdx = 0; colIdx < sizeof(cols) / sizeof(cols[0]); colIdx++)
  {
    digitalWrite(cols[colIdx], HIGH);
    for (size_t rowIdx = 0; rowIdx < sizeof(rows) / sizeof(rows[0]); rowIdx++)
    {
      KeyboardUsageId keycode = idxKeycodeMap[rowIdx][colIdx];
      if (keycode == KEY_NONE)
      {
        continue;
      }
      if (digitalRead(rows[rowIdx]) == HIGH)
      {
        changeKey += SerialKeyboardClient.add(keycode);
      }
      else
      {
        changeKey += SerialKeyboardClient.remove(keycode);
      }
    }
    digitalWrite(cols[colIdx], LOW);
    delay(0);
  }

  if (changeKey > 0)
  {
    SerialKeyboardClient.send();
  }
}

uint8_t connectionId = 0;
unsigned long bootSelPushStartTime = 0;
unsigned long bootSelLastPushTime = 0;
unsigned long startShowConnection = 0;
unsigned long loop1LastTime = 0;
unsigned long lastConnInfoUpdated = 0;

typedef enum _PushType
{
  NO_PUSH,
  SHORT_PUSH,
  LONG_PUSH,
} PushType;

void loop1()
{
  unsigned long currentMillis = millis();
  if (currentMillis - loop1LastTime < interval)
  {
    return;
  }
  loop1LastTime = currentMillis;

  PushType pushType = NO_PUSH;
  if (bootSelPushStartTime != 0)
  {
    unsigned long pushTime = currentMillis - bootSelPushStartTime;
    if (pushTime < 500)
    {
      pushType = SHORT_PUSH;
    }
    else if (pushTime > 3000)
    {
      pushType = LONG_PUSH;
    }
  }

  if (BOOTSEL)
  {
    if (bootSelPushStartTime == 0 && currentMillis - bootSelLastPushTime > 100)
    {
      bootSelPushStartTime = currentMillis;
    }
  }
  else
  {
    if (bootSelPushStartTime != 0)
    {
      switch (pushType)
      {
      case SHORT_PUSH:
        Serial.println("short push");
        break;
      case LONG_PUSH:
        Serial.println("long push");
        break;
      default:
        break;
      }
      bootSelPushStartTime = 0;
      bootSelLastPushTime = currentMillis;
    }

    if (startShowConnection != 0)
    {
      // 接続切り替えモード
      if (pushType == SHORT_PUSH)
      {
        // bootボタン 短押し

        // 接続先切り替え
        startShowConnection = currentMillis;
        Serial.println("switch connection");
        connectionId = (connectionId + 1) % MAX_CONNECTIONS;
        SerialKeyboardClient.switchConnection(connectionId);
        displayConnectionInfo(&displayInfo, currentMillis - startShowConnection);
      }
      // else if (pushType == LONG_PUSH)
      // {
      //   // bootボタン 長押し

      //   // advertising開始
      //   Serial.println("start adv");
      //   AdvertisingState adv = SerialKeyboardClient.getAdvertisingState();
      //   if (adv == NOT_ADVERTISING)
      //   {
      //     Serial.println("start adv");
      //     SerialKeyboardClient.startAdv();
      //   }
      // }
    }
    else if (writerMode)
    {
      // writerモード
      if (pushType == SHORT_PUSH)
      {
        // bootボタン 短押し

        // writerモード終了
        writerMode = false;
        resetBLEModule();
        delay(2000);
        Serial3.begin(BAUDRATE);
        SerialKeyboardClient.resumeUart();
      }
    }
    else
    {
      // 通常モード
      if (pushType == SHORT_PUSH)
      {
        // bootボタン 短押し
        // 接続切り替えモード開始
        startShowConnection = currentMillis;
        SerialKeyboardClient.refreshConnectionInfo();
        display.ssd1306_command(SSD1306_SETCONTRAST);
        display.ssd1306_command(0xFF);
      }
      else if (pushType == LONG_PUSH)
      {
        // bootボタン 長押し
        // writerModeを有効化
        writerMode = true;
        SerialKeyboardClient.pauseUart();
        Serial3.end();
        pinMode(TX, INPUT);
        pinMode(RX, INPUT);
        pinMode(BLE_RESET, INPUT);
      }
    }
  }

  // if (startShowConnection == 0 && currentMillis - lastConnInfoUpdated > CONN_INFO_BG_UPDATE_INTERVAL ||
  //     startShowConnection != 0 && currentMillis - lastConnInfoUpdated > CONN_INFO_FG_UPDATE_INTERVAL)
  if (startShowConnection != 0 && currentMillis - lastConnInfoUpdated > CONN_INFO_FG_UPDATE_INTERVAL)
  {
    lastConnInfoUpdated = currentMillis;
    SerialKeyboardClient.refreshConnectionInfo();
  }

  if (startShowConnection == 0)
  {
    // 通常モード
    // writerモード

    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
      SerialKeyboardClient.getConnectionName(i, displayInfo.connectionName[i]);
    }
    displayInfo.writerReady = pushType == LONG_PUSH && !writerMode && startShowConnection == 0;
    displayMainInfo(&displayInfo);
  }
  else
  {
    if (currentMillis - startShowConnection > SHOW_CONNECTION_TIMEOUT)
    {
      startShowConnection = 0;
      display.ssd1306_command(SSD1306_SETCONTRAST);
      display.ssd1306_command(0x1);
      display.clearDisplay();
      display.display();
    }
    else
    {
      for (int i = 0; i < MAX_CONNECTIONS; i++)
      {
        SerialKeyboardClient.getConnectionName(i, displayInfo.connectionName[i]);
      }
      displayConnectionInfo(&displayInfo, currentMillis - startShowConnection);
    }
  }
}

#define BATT_CELL_H 7
#define BATT_CELL_W 4
#define BATT_LEVELS 4
#define BATT_CELL_GAP 1
#define BATT_B_H (BATT_CELL_H + BATT_CELL_GAP * 2 + 2)
#define BATT_HEAD_H 4
#define BATT_HEAD_W 2
#define BATT_B_W (BATT_CELL_W * BATT_LEVELS + BATT_CELL_GAP * (BATT_LEVELS + 1) + 2)
#define BATT_H (BATT_B_H + 2)
#define BATT_W (BATT_B_W + 2 + BATT_HEAD_W)

void drawBattery(uint8_t x, uint8_t y)
{
  display.drawRect(x, y, BATT_B_W, BATT_B_H, SSD1306_WHITE);
  display.fillRect(x + BATT_B_W, y + (BATT_B_H - BATT_HEAD_H) / 2, BATT_HEAD_W, BATT_HEAD_H, SSD1306_WHITE);
  for (int i = 0; i < BATT_LEVELS; i++)
  {
    display.fillRect(x + i * (BATT_CELL_W + BATT_CELL_GAP) + BATT_CELL_GAP + 1, y + BATT_CELL_GAP + 1, BATT_CELL_W, BATT_CELL_H, SSD1306_WHITE);
  }
}

void displayMainInfo(DisplayInfo *displayInfo)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    if (i == connectionId && strlen(displayInfo->connectionName[i]) > 0)
    {
      // display.fillRect(i * CONN_IND_WIDTH, 0, CONN_IND_WIDTH, 2, SSD1306_WHITE);
    }
    // else if (i != MAX_CONNECTIONS - 1)
    // {
    //   uint8_t right = i * CONN_IND_WIDTH + CONN_IND_WIDTH;
    //   display.drawLine(right, 0, right, 1, SSD1306_WHITE);
    //   // display.drawRect(i * CONN_IND_WIDTH, 0, CONN_IND_WIDTH, 2, SSD1306_WHITE);
    // }
  }
  if (displayInfo->writerReady)
  {
    display.setCursor(0, FONT_OFFSET_Y);
    display.print("Long");
  }
  drawBattery(SCREEN_WIDTH - BATT_W - 1, 1);
  display.drawFastHLine(0, BATT_H + 1, SCREEN_WIDTH, SSD1306_WHITE);
  display.setCursor(0, FONT_OFFSET_Y + BATT_H + 2);
  if (writerMode)
  {
    display.print("Writer Mode");
  }
  else
  {
    // display.print("Connection:");
    // display.print(connectionId);
    // display.print(" ");
    display.print(displayInfo->connectionName[connectionId]);
  }
  // display.print(connectionNames[connectionId])
  display.display();
}

void displayConnectionInfo(DisplayInfo *displayInfo, unsigned long showingTime)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, FONT_OFFSET_Y);

  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    if (i == connectionId)
    {
      display.print(">");
    }
    else
    {
      display.print(" ");
    }
    display.print(i);
    display.print(":");
    display.println(displayInfo->connectionName[i]);
  }

  uint16_t lineLen = showingTime * SCREEN_WIDTH / SHOW_CONNECTION_TIMEOUT;
  // display.printf("Timeout: %.1f\n", (SHOW_CONNECTION_TIMEOUT - (showingTime)) / 1000.0);
  uint16_t y = SCREEN_HEIGHT - 1;
  display.drawLine(0, y, SCREEN_WIDTH - lineLen, y, SSD1306_WHITE);
  display.display();
}
