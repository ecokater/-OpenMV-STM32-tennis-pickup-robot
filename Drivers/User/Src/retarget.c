#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "usart.h" 
#include "stm32h7xx_hal.h"

extern UART_HandleTypeDef huart1;

/* 
 * 重定向 GCC 标准库的 _write 函数到 UART
 * 这使得 printf() 可以输出到串口
 */
int _write(int file, char *ptr, int len)
{
  /* 只重定向 stdout (1) 和 stderr (2) */
  if (file == 1 || file == 2)
  {
    /* 发送数据到 UART1 */
    /* 注意：这里使用阻塞模式发送，如果需要高性能，可以改用中断或DMA */
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 0xFFFF);
    return len;
  }
  
  errno = EBADF;
  return -1;
}
