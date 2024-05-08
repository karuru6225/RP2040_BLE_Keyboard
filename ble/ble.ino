#include "SerialKeyboardHost.h"

void setup()
{
  Serial.begin(115200);
  delay(100);

  SerialKeyboardHost.setStream(&Serial);
  SerialKeyboardHost.begin();

  delay(100);

  SerialKeyboardHost.startAdv();
}

void loop()
{
  SerialKeyboardHost.update();
  delay(1);
}
