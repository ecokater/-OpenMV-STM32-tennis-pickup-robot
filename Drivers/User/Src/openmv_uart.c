#include "openmv_uart.h"
#include <string.h>

extern UART_HandleTypeDef huart4;

/* 定义环形缓冲区大小，建议是 DMA 搬运长度的 2 倍以上 */
#define UART_RX_BUFFER_SIZE 64 

/* DMA 接收缓冲区 - 放入 DMA 专用段以避免 Cache 问题 */
static uint8_t rx_buffer[UART_RX_BUFFER_SIZE] __attribute__((section(".dma_buffer")));

/* 环形缓冲区读指针 */
static volatile uint16_t rx_read_index = 0;

/* 解析出的数据 */
static openmv_uart_data_t parsed_data = {0};
/* UART 数据更新计数器 */
static volatile uint32_t uart_seq = 0;

void openmv_uart_init(void)
{
    /* 开启 UART4 空闲中断接收 (IDLE Line Detection) + DMA */
    /* 注意：如果不使用 IDLE 中断，仅使用循环 DMA 也可以，这里为了简单演示循环 DMA 轮询 */
    HAL_UART_Receive_DMA(&huart4, rx_buffer, UART_RX_BUFFER_SIZE);
}

void openmv_uart_get_data(openmv_uart_data_t *out)
{
    if (out != NULL)
    {
        *out = parsed_data;
    }
}

/* 从缓冲区读取一个字节 */
static int read_byte(uint8_t *byte)
{
    /* 计算当前 DMA 写入到了哪里 */
    /* CNDTR 是倒计数的，所以用 BUFFER_SIZE - CNDTR 得到当前写入位置 */
    uint16_t write_index = UART_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart4.hdmarx);

    if (rx_read_index == write_index)
    {
        return 0; // 没有新数据
    }

    *byte = rx_buffer[rx_read_index];
    rx_read_index++;
    if (rx_read_index >= UART_RX_BUFFER_SIZE)
    {
        rx_read_index = 0;
    }
    return 1;
}

/* 状态机状态 */
typedef enum
{
    STATE_WAIT_AA,
    STATE_WAIT_BB,
    STATE_READ_X_L,
    STATE_READ_X_H,
    STATE_READ_Y_L,
    STATE_READ_Y_H,
    STATE_WAIT_0D,
    STATE_WAIT_0A
} parse_state_t;

static parse_state_t state = STATE_WAIT_AA;
static uint8_t temp_x_l, temp_x_h, temp_y_l, temp_y_h;

uint32_t openmv_uart_get_seq(void)
{
    return uart_seq;
}

void openmv_uart_process(void)
{
    uint8_t byte;
    
    /* 循环读取所有新数据 */
    while (read_byte(&byte))
    {
        switch (state)
        {
            case STATE_WAIT_AA:
                if (byte == 0xAA) state = STATE_WAIT_BB;
                break;
                
            case STATE_WAIT_BB:
                if (byte == 0xBB) state = STATE_READ_X_L;
                else if (byte == 0xAA) state = STATE_WAIT_BB; // 重复 AA 的情况
                else state = STATE_WAIT_AA;
                break;
                
            case STATE_READ_X_L:
                temp_x_l = byte;
                state = STATE_READ_X_H;
                break;
                
            case STATE_READ_X_H:
                temp_x_h = byte;
                state = STATE_READ_Y_L;
                break;
                
            case STATE_READ_Y_L:
                temp_y_l = byte;
                state = STATE_READ_Y_H;
                break;
                
            case STATE_READ_Y_H:
                temp_y_h = byte;
                state = STATE_WAIT_0D;
                break;
                
            case STATE_WAIT_0D:
                if (byte == 0x0D) state = STATE_WAIT_0A;
                else state = STATE_WAIT_AA;
                break;
                
            case STATE_WAIT_0A:
                if (byte == 0x0A)
                {
                    /* 解析成功，更新数据 */
                    parsed_data.ball_x = (uint16_t)temp_x_l | ((uint16_t)temp_x_h << 8);
                    parsed_data.ball_y = (uint16_t)temp_y_l | ((uint16_t)temp_y_h << 8);
                    parsed_data.last_update_tick = HAL_GetTick();
                    uart_seq++; /* 更新序号 */
                }
                state = STATE_WAIT_AA; // 回到初始状态
                break;
                
            default:
                state = STATE_WAIT_AA;
                break;
        }
    }
}
