/*
  Keyboard.h

  Modified by Earle F. Philhower, III <earlephilhower@yahoo.com>
  Main Arduino Library Copyright (c) 2015, Arduino LLC
  Original code (pre-library): Copyright (c) 2011, Peter Barrett

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include <Arduino.h>
#include "Usage_Pages/Usage_Pages.h"
#include "KeyboardLayouts/Layouts_JP.h"

// Low level key report: up to 6 keys and shift, ctrl etc at once
typedef struct
{
  uint8_t modifiers;
  ConsumerUsageId consumer;
  KeyboardUsageId keys[6];
} KeyReport;

class HID_Keyboard : public Print
{
protected:
  KeyReport _keyReport;

public:
  HID_Keyboard(void);
  void begin(void);
  void end(void);

  // by keycodes
  size_t write(KeyboardUsageId k);
  size_t press(KeyboardUsageId k);
  size_t release(KeyboardUsageId k);
  size_t add(KeyboardUsageId k);
  size_t remove(KeyboardUsageId k);
  size_t set(KeyboardUsageId k, bool on);

  // size_t write(ConsumerUsageId k);
  // size_t press(ConsumerUsageId k);
  // size_t release(ConsumerUsageId k);
  // size_t add(ConsumerUsageId k);
  // size_t remove(ConsumerUsageId k);
  // size_t set(ConsumerUsageId k, bool on);

  // by ascii
  size_t write(uint8_t k) override;
  size_t press(uint8_t k);
  size_t release(uint8_t k);
  size_t add(uint8_t k);
  size_t remove(uint8_t k);
  size_t set(uint8_t k, bool on);

  size_t releaseAll(void);
  virtual int send(void) = 0;

  typedef void (*LedCallbackFcn)(bool numlock, bool capslock, bool scrolllock, bool compose, bool kana, void *cbData);
  void onLED(LedCallbackFcn fcn, void *cbData = nullptr);
  LedCallbackFcn _ledCB;
  void *_ledCBdata;
};
