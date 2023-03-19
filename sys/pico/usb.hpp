#pragma once

#include <cstdint>

void init_usb();
#ifdef __cplusplus
extern "C" {
#endif
void update_usb();
#ifdef __cplusplus
}
#endif

void usb_debug(const char *message);

bool is_multiplayer_connected();
void set_multiplayer_enabled(bool enabled);
void send_multiplayer_message(const uint8_t *data, uint16_t len);
