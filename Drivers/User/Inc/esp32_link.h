#ifndef ESP32_LINK_H
#define ESP32_LINK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
    ESP32_CONTROL_MODE_PICKUP = 0,
    ESP32_CONTROL_MODE_MANUAL = 1
} esp32_control_mode_t;

void esp32_control_init(void);
void esp32_control_process(void);
esp32_control_mode_t esp32_control_get_mode(void);
void esp32_control_set_mode(esp32_control_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif
