#include "openmv_spi.h"
#include <string.h>

extern SPI_HandleTypeDef hspi1;

#define OPENMV_MAGIC 0xDEADBEEFUL
#define OPENMV_HEADER_BYTES 8
/* OPENMV_IMG_BYTES is defined in header */
#define OPENMV_PACKET_BYTES (OPENMV_HEADER_BYTES + OPENMV_IMG_BYTES)
#define DUMMY_BYTES 4
/* 暂时只接收 Header + Dummy，不接收图像 */
/* 恢复接收图像 */
#define RX_BUFFER_SIZE (OPENMV_PACKET_BYTES + DUMMY_BYTES)

/* 定义双缓冲 */
uint8_t spi_rx_buffer[2][RX_BUFFER_SIZE] __attribute__((section(".dma_buffer")));
/* 双缓冲控制变量 */
static volatile uint8_t dma_idx = 0;      // 当前 DMA 正在写入的 buffer
static volatile uint8_t process_idx = 0;  // 当前 CPU 应该处理的 buffer
static volatile uint8_t data_ready_flag = 0; // 新数据标志

uint8_t user_spi_rx_buffer[16];

/* 图像指针直接指向 spi_rx_buffer 中的正确位置 */
static uint8_t *openmv_image = NULL; 
/* 接收状态与帧序号 */
static volatile uint32_t openmv_frame_seq = 0;
static volatile uint32_t data_seq = 0;
/* 最新解析出的元数据 */
static openmv_meta_t openmv_meta;
/* 上一次收到有效帧的时间戳 */
static uint32_t last_frame_tick = 0;

static uint16_t openmv_u16_le(const uint8_t *p)
{
  return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static void openmv_spi_start_header(void)
{
  HAL_SPI_Receive_DMA(&hspi1, spi_rx_buffer[dma_idx], RX_BUFFER_SIZE);
}

void openmv_spi_init(void)
{
  openmv_spi_start_header();
  last_frame_tick = HAL_GetTick(); // 初始化 tick
}

void openmv_spi_restart(void)
{
    /* 1. 停止当前传输 */
    HAL_SPI_Abort(&hspi1);
    
    /* 2. 清除可能的 OVR 错误标志 */
    /* 读 DR 和 SR 寄存器是清除 OVR 的标准步骤 */
    __HAL_SPI_CLEAR_OVRFLAG(&hspi1);
    
    /* 3. 确保 SPI 不在忙状态 */
    /* 如果卡在 BUSY，可能需要强制复位外设 (HAL_SPI_DeInit/Init)，但这里先试试软复位 */
    
    /* 4. 重启接收 */
    openmv_spi_start_header();
    
    /* 5. 重置超时 tick */
    last_frame_tick = HAL_GetTick();
}

/* 检查是否超时（ms），如果超时则重启 SPI */
void openmv_check_timeout(uint32_t timeout_ms)
{
    if ((HAL_GetTick() - last_frame_tick) > timeout_ms)
    {
        /* 超时了！重启 SPI */
        openmv_spi_restart();
    }
}

uint32_t openmv_get_frame_seq(void)
{
  /* 读取帧序号 */
  return openmv_frame_seq;
}

uint32_t openmv_get_data_seq(void)
{
  /* 读取数据序号 */
  return data_seq;
}
void openmv_get_meta(openmv_meta_t *out)
{
  /* 读取最新元数据 */
  if (out != NULL)
  {
    *out = openmv_meta;
  }
}

void openmv_get_raw_header(uint8_t *out)
{
  if (out != NULL)
  {
    /* 拷贝前16字节用于调试 */
    for(int i=0; i<16; i++) out[i] = user_spi_rx_buffer[i];
  }
}

uint8_t *openmv_get_image(void)
{
  /* 返回图像缓冲区指针 */
  return openmv_image;
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance != SPI1)
  {
    return;
  }
  
  /* 1. 记录当前已完成的 buffer 索引给主循环处理 */
  process_idx = dma_idx;
  data_ready_flag = 1;
  
  /* 2. 切换 DMA 到下一个 buffer */
  dma_idx = !dma_idx;
  
  /* 3. 立即重启接收，尽可能减少间隙 */
  openmv_spi_start_header();
}

/* 主循环中调用的处理函数 */
void openmv_process(void)
{
  if (!data_ready_flag)
  {
      return;
  }
  
  /* 清除标志，避免重复处理 */
  data_ready_flag = 0;
  
  /* 获取当前要处理的 buffer 指针 */
  uint8_t *current_buffer = spi_rx_buffer[process_idx];
  
  /* 使 Cache 无效（虽然 MPU 已配置，但双保险） */
  SCB_InvalidateDCache_by_Addr((uint32_t*)current_buffer, RX_BUFFER_SIZE);
  
  /* 智能查找帧头与Bit Shift修复 */
  uint8_t *p_data = NULL;
  uint8_t bit_shift_detected = 0;

  /* 1. 优先搜索完美对齐的帧头 (EF BE) */
  for(int i = 0; i <= DUMMY_BYTES; i++)
  {
    if(current_buffer[i] == 0xEF && current_buffer[i+1] == 0xBE)
    {
      p_data = &current_buffer[i];
      bit_shift_detected = 0;
      break;
    }
  }

  /* 2. 如果没找到，搜索右移1位的帧头 (77 DF) */
  if(p_data == NULL)
  {
    for(int i = 0; i <= DUMMY_BYTES; i++)
    {
      if(current_buffer[i] == 0x77 && current_buffer[i+1] == 0xDF)
      {
        p_data = &current_buffer[i];
        bit_shift_detected = 1;
        break;
      }
    }
  }

  if (p_data != NULL)
  {
    /* 如果检测到 Bit Shift，需要手动修复数据 */
    if (bit_shift_detected)
    {
      /* 严重警告：如果检测到 Bit Shift，意味着后面的图像数据全部错位了！ */
      /* 策略：只解析 Header 里的元数据，但丢弃图像数据 */
      for(int k = 0; k < OPENMV_HEADER_BYTES; k++)
      {
        user_spi_rx_buffer[k] = (p_data[k] << 1) | (p_data[k+1] >> 7);
      }
      /* 图像指针置空 */
      openmv_image = NULL; 
    }
    else
    {
      /* 完美！没移位，直接拷贝 Header */
      memcpy(user_spi_rx_buffer, p_data, OPENMV_HEADER_BYTES);
      
      /* 图像指针直接指向 Header 后面的数据 */
      openmv_image = p_data + OPENMV_HEADER_BYTES;
    }

    if (1)
    {
      /* 解析 metadata */
      uint8_t *p_header;
      
      if (bit_shift_detected)
      {
          p_header = user_spi_rx_buffer;
      }
      else
      {
          p_header = p_data;
      }
      
      /* 只有 ROI 坐标了，偏移量也变了 */
      /* Magic(4) + ROI_X(2) + ROI_Y(2) */
      openmv_meta.roi_x = openmv_u16_le(&p_header[4]);
      openmv_meta.roi_y = openmv_u16_le(&p_header[6]);
      
      /* 只有当图像可用（没移位）时，才更新帧序号 */
      if (!bit_shift_detected)
      {
          openmv_frame_seq++;
          last_frame_tick = HAL_GetTick(); // 更新 tick，表示还活着
      }
    }
    data_seq++;
  }
  /* 注意：如果帧头无效，我们不再 Abort，因为 DMA 已经在中断里重启了 */
  if (p_data == NULL)
  {
      /* 严重错误：找不到帧头，说明 SPI 接收已经完全错位 */
      /* 强制 Abort 并重启，以实现物理层面的重新同步 */
      HAL_SPI_Abort(&hspi1);
      
      /* 重启接收 */
      openmv_spi_start_header();
  }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance != SPI1)
  {
    return;
  }
  
  /* SPI 发生错误（如 OVR）时，必须清除错误标志并重启 */
  /* 如果不清除 OVR，SPI 将无法接收任何新数据 */
  __HAL_SPI_CLEAR_OVRFLAG(hspi);
  
  /* 强制重启流程 */
  openmv_spi_restart();
}
