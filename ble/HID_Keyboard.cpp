#include "HID_Keyboard.h"
#include <strings.h>

//================================================================================
//================================================================================
//  Keyboard

HID_Keyboard::HID_Keyboard(void) {
    bzero(&_keyReport, sizeof(_keyReport));
    _ledCB = nullptr;
}

size_t HID_Keyboard::releaseAll(void) {
		bzero(&_keyReport, sizeof(_keyReport));
		return 1;
}

void HID_Keyboard::begin(void) {
	releaseAll();
	send();
}

void HID_Keyboard::end(void) {
	releaseAll();
}

size_t HID_Keyboard::write(KeyboardUsageId k) {
	auto ret = press(k);
	if (ret) {
		release(k);
	};
	return ret;
}

size_t HID_Keyboard::press(KeyboardUsageId k) {
	auto ret = add(k);
	if (ret) {
		send();
	}
	return ret;
}

size_t HID_Keyboard::release(KeyboardUsageId k) {
	auto ret = remove(k);
	if (ret) {
		send();
	}
	return ret;
}

size_t HID_Keyboard::add(KeyboardUsageId k) {
	return set(k, true);
}

size_t HID_Keyboard::remove(KeyboardUsageId k) {
	return set(k, false);
}

size_t HID_Keyboard::set(KeyboardUsageId k, bool on) {
	if (k >= KEY_LEFT_CTRL && k <= KEY_RIGHT_GUI) {
		// Modifier key
		if (on) {
			_keyReport.modifiers |= (1 << (k - KEY_LEFT_CTRL));
		} else {
			_keyReport.modifiers &= ~(1 << (k - KEY_LEFT_CTRL));
		}
		return 1;
	} 

	const uint8_t keysSize = sizeof(_keyReport.keys) / sizeof(_keyReport.keys[0]);

	for (uint8_t i = 0; i < keysSize; i++) {
		if (_keyReport.keys[i] == k) {
			if (!on) {
				_keyReport.keys[i] = KEY_RESERVED;
			}
			return 1;
		}
	}
	if (!on) {
		return 0;
	}
	for (uint8_t i = 0; i < keysSize; i++) {
		if (_keyReport.keys[i] == KEY_RESERVED) {
			if (on) {
				_keyReport.keys[i] = k;
			}
			return 1;
		}
	}
	return 0;
}

// size_t HID_Keyboard::write(ConsumerUsageId k) {
// 	auto ret = press(k);
// 	if (ret) {
// 		release(k);
// 	};
// 	return ret;
// }

// size_t HID_Keyboard::press(ConsumerUsageId k) {
// 	auto ret = add(k);
// 	if (ret) {
// 		send();
// 	}
// 	return ret;
// }

// size_t HID_Keyboard::release(ConsumerUsageId k) {
// 	auto ret = remove(k);
// 	if (ret) {
// 		send();
// 	}
// 	return ret;
// }

// size_t HID_Keyboard::add(ConsumerUsageId k) {
// 	return set(k, true);
// }

// size_t HID_Keyboard::remove(ConsumerUsageId k) {
// 	return set(k, false);
// }

// size_t HID_Keyboard::set(ConsumerUsageId k, bool on) {
// 	if (k >= CONSUMER_USAGE_ID_POWER && k <= CONSUMER_USAGE_ID_CONSUMER_CONTROL) {
// 		// Consumer key
// 		if (on) {
// 			_keyReport.consumer |= (1 << (k - CONSUMER_USAGE_ID_POWER));
// 		} else {
// 			_keyReport.consumer &= ~(1 << (k - CONSUMER_USAGE_ID_POWER));
// 		}
// 		return 1;
// 	} 
// 	return 0;
// }

size_t HID_Keyboard::write(uint8_t k) {
	auto ret = press(k);
	if (ret) {
		release(k);
	};
	return ret;
}

size_t HID_Keyboard::press(uint8_t k) {
	auto ret = add(k);
	if (ret) {
		send();
	}
	return ret;
}

size_t HID_Keyboard::release(uint8_t k) {
	auto ret = remove(k);
	if (ret) {
		send();
	}
	return ret;
}

size_t HID_Keyboard::add(uint8_t k) {
	return set(k, true);
}

size_t HID_Keyboard::remove(uint8_t k) {
	return set(k, false);
}

size_t HID_Keyboard::set(uint8_t k, bool on) {
	if (sizeof(_asciimap)/sizeof(_asciimap[0]) <= k) {
		return 0;
	}

	uint16_t keycodeWithMods = pgm_read_word(_asciimap + k);
	KeyboardUsageId keycode = KeyboardUsageId((uint8_t)(keycodeWithMods & 0xFF));
	auto ret = set(keycode, on);

  // キーを押すのに成功したときだけ、modifier キーを押す
	// 離すときは常に処理する
	if (!ret && on) {
		return ret;
	}

	if (keycodeWithMods & MOD_LEFT_CTRL) {
		ret += set(KEY_LEFT_CTRL, on);
	}
	if (keycodeWithMods & MOD_LEFT_SHIFT) {
		ret += set(KEY_LEFT_SHIFT, on);
	}
	if (keycodeWithMods & MOD_LEFT_ALT) {
		ret += set(KEY_LEFT_ALT, on);
	}
	if (keycodeWithMods & MOD_LEFT_GUI) {
		ret += set(KEY_LEFT_GUI, on);
	}
	if (keycodeWithMods & MOD_RIGHT_CTRL) {
		ret += set(KEY_RIGHT_CTRL, on);
	}
	if (keycodeWithMods & MOD_RIGHT_SHIFT) {
		ret += set(KEY_RIGHT_SHIFT, on);
	}
	if (keycodeWithMods & MOD_RIGHT_ALT) {
		ret += set(KEY_RIGHT_ALT, on);
	}
	if (keycodeWithMods & MOD_RIGHT_GUI) {
		ret += set(KEY_RIGHT_GUI, on);
	}

	return ret;
}

void HID_Keyboard::onLED(LedCallbackFcn fcn, void *cbData) {
    _ledCB = fcn;
    _ledCBdata = cbData;
}
