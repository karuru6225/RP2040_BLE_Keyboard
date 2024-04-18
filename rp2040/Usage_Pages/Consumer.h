#pragma once

#include <Arduino.h>

// Consumer Page 0x0C
// OOC, OSC, LC, RTC

enum ConsumerUsageId : uint8_t {
	GEN_POWER	= 0x30, // usage type = OOC
	GEN_SLEEP = 0x32, // usage type = OSC

  TRANSPORT_PLAY = 0xB0, // usage type = OOC
  TRANSPORT_PAUSE = 0xB1, // usage type = OOC
  TRANSPORT_RECORD = 0xB2, // usage type = OOC
  TRANSPORT_FAST_FORWARD = 0xB3, // usage type = OOC
  TRANSPORT_REWIND = 0xB4, // usage type = OOC
  TRANSPORT_NEXT_TRACK = 0xB5, // usage type = OSC
  TRANSPORT_PREV_TRACK = 0xB6, // usage type = OSC
  TRANSPORT_STOP = 0xB7, // usage type = OSC
  TRANSPORT_EJECT = 0xB8, // usage type = OSC
  TRANSPORT_RANDOM_PLAY = 0xB9, // usage type = OOC
  TRANSPORT_RPEAT = 0xBC, // usage type = OSC
  TRANSPORT_PLAY_PAUSE = 0xCD, // usage type = OSC

  AUDIO_VOLUME = 0xE0, // usage type = LC
  AUDIO_MUTE = 0xE2, // usage type = OOC
  AUDIO_VOLUME_INC = 0xE9, // usage type = RTC
  AUDIO_VOLUME_DEC = 0xEA, // usage type = RTC
  
  DISPLAY_BRIGHTNESS_INC = 0x6F, // usage type = RTC
  DISPLAY_BRIGHTNESS_DEC = 0x70, // usage type = RTC
  DISPLAY_BRIGHTNESS = 0x71, // usage type = LC
  DISPLAY_BRIGHTNESS_TOGGLE = 0x72, // usage type = OOC
  DISPLAY_BRIGHTNESS_MIN = 0x73, // usage type = OSC
  DISPLAY_BRIGHTNESS_MAX = 0x74, // usage type = OSC
  DISPLAY_BRIGHTNESS_AUTO = 0x75, // usage type = OOC
  
  KEYBOARD_BRIGHTNESS_INC = 0x79, // usage type = OSC
  KEYBOARD_BRIGHTNESS_DEC = 0x7A, // usage type = OSC
  KEYBOARD_BRIGHTNESS_SET = 0x7B, // usage type = LC
  KEYBOARD_BRIGHTNESS_OOC = 0x7C, // usage type = OOC
  KEYBOARD_BRIGHTNESS_MIN = 0x7D, // usage type = OSC
  KEYBOARD_BRIGHTNESS_MAX = 0x7E, // usage type = OSC
  KEYBOARD_BRIGHTNESS_AUTO = 0x8A, // usage type = OOC
};