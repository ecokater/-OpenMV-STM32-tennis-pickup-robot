#ifndef OPENMV_UART_H
#define OPENMV_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef struct
{
  uint16_t ball_x;
  uint16_t ball_y;
  uint32_t last_update_tick; // 最后一次收到有效数据的时间戳
} openmv_uart_data_t;

void openmv_uart_init(void);
void openmv_uart_process(void);
void openmv_uart_get_data(openmv_uart_data_t *out);
uint32_t openmv_uart_get_seq(void); /* 获取 UART 数据更新序号 */

#ifdef __cplusplus
}
#endif

#endif
