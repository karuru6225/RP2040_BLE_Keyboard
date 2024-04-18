#pragma once

#include <Arduino.h>

enum LEDFlags : uint8_t {
    LED_FLAG_NUM_LOCK = (uint8_t)0x01,
    LED_FLAG_CAPS_LOCK = (uint8_t)0x02,
    LED_FLAG_SCROLL_LOCK = (uint8_t)0x04,
    LED_FLAG_COMPOSE = (uint8_t)0x08,
    LED_FLAG_KANA = (uint8_t)0x10,
    LED_FLAG_POWER = (uint8_t)0x20,
    LED_FLAG_SHIFT = (uint8_t)0x40,
    LED_FLAG_DO_NOT_DISTURB = (uint8_t)0x80
};

// LED Page 0x08

enum LEDUsageId {
    // usage type = OOC
    LED_UNDEFINED = 0x00,
    LED_NUM_LOCK = 0x01,
    LED_CAPS_LOCK = 0x02,
    LED_SCROLL_LOCK = 0x03,
    LED_COMPOSE = 0x04,
    LED_KANA = 0x05,
    LED_POWER = 0x06,
    LED_SHIFT = 0x07,
    LED_DO_NOT_DISTURB = 0x08,
    LED_MUTE = 0x09,
    
    // 0x0A - 0x68 are in use
    // 0x69 - 0xFFFF are reserved
};