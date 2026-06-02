#ifndef OPENMV_SPI_H
#define OPENMV_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define OPENMV_IMG_W 80
#define OPENMV_IMG_H 80
#define OPENMV_IMG_BYTES (OPENMV_IMG_W * OPENMV_IMG_H * 2)



typedef struct
{
  uint16_t roi_x;  /* 裁剪 ROI 左上角 x */
  uint16_t roi_y;  /* 裁剪 ROI 左上角 y */
} openmv_meta_t;

void openmv_spi_init(void);            /* 启动一次 SPI DMA 接收 */
void openmv_process(void);             /* 主循环调用，处理接收到的数据 */
void openmv_check_timeout(uint32_t timeout_ms); /* 检查超时并自动重启 */
uint32_t openmv_get_frame_seq(void);   /* 读取帧序号，变化表示新帧 */
uint32_t openmv_get_data_seq(void);    /* 读取数据序号，变化表示新数据 */
void openmv_get_meta(openmv_meta_t *out); /* 读取最新元数据 */
void openmv_get_raw_header(uint8_t *out); /* 调试：读取原始帧头 */
uint8_t *openmv_get_image(void);       /* 获取图像缓冲区指针 */

#ifdef __cplusplus
}
#endif

#endif
