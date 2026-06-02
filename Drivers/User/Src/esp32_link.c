#include "esp32_link.h"

#include "carcontrol.h"

extern UART_HandleTypeDef huart7;

#define ESP32_PACKET_HEADER        0xFFU
#define ESP32_PACKET_TAIL          0xFEU
#define ESP32_CMD_FORWARD          0x01U
#define ESP32_CMD_BACKWARD         0x02U
#define ESP32_CMD_LEFT             0x03U
#define ESP32_CMD_RIGHT            0x04U
#define ESP32_CMD_STOP             0x05U
#define ESP32_CMD_MODE_MANUAL      0x06U
#define ESP32_CMD_MODE_PICKUP      0x07U
#define ESP32_MANUAL_PWM           45U
#define ESP32_MANUAL_TIMEOUT_MS    350U

static volatile uint8_t g_uart7_rx_byte = 0U;
static volatile uint8_t g_parser_state = 0U;
static volatile uint8_t g_pending_code = 0U;
static volatile uint8_t g_manual_command = ESP32_CMD_STOP;
static volatile uint32_t g_manual_command_tick = 0U;
static volatile esp32_control_mode_t g_control_mode = ESP32_CONTROL_MODE_PICKUP;

static void esp32_set_mode_internal(esp32_control_mode_t mode)
{
    g_control_mode = mode;
    g_manual_command = ESP32_CMD_STOP;
    g_manual_command_tick = HAL_GetTick();
}

static void esp32_uart7_start_receive(void)
{
    (void)HAL_UART_Receive_IT(&huart7, (uint8_t *)&g_uart7_rx_byte, 1U);
}

static void esp32_apply_manual_command(uint8_t command)
{
    switch (command)
    {
    case ESP32_CMD_FORWARD:
        car_forward(ESP32_MANUAL_PWM);
        break;
    case ESP32_CMD_BACKWARD:
        car_backward(ESP32_MANUAL_PWM);
        break;
    case ESP32_CMD_LEFT:
        car_turn_left(ESP32_MANUAL_PWM);
        break;
    case ESP32_CMD_RIGHT:
        car_turn_right(ESP32_MANUAL_PWM);
        break;
    case ESP32_CMD_STOP:
    default:
        car_stop();
        break;
    }
}

static void esp32_handle_code(uint8_t code)
{
    uint32_t now = HAL_GetTick();

    switch (code)
    {
    case ESP32_CMD_FORWARD:
    case ESP32_CMD_BACKWARD:
    case ESP32_CMD_LEFT:
    case ESP32_CMD_RIGHT:
    case ESP32_CMD_STOP:
        g_control_mode = ESP32_CONTROL_MODE_MANUAL;
        g_manual_command = code;
        g_manual_command_tick = now;
        break;
    case ESP32_CMD_MODE_MANUAL:
        (void)now;
        esp32_set_mode_internal(ESP32_CONTROL_MODE_MANUAL);
        break;
    case ESP32_CMD_MODE_PICKUP:
        (void)now;
        esp32_set_mode_internal(ESP32_CONTROL_MODE_PICKUP);
        break;
    default:
        break;
    }
}

void esp32_control_init(void)
{
    g_uart7_rx_byte = 0U;
    g_parser_state = 0U;
    g_pending_code = 0U;
    g_manual_command = ESP32_CMD_STOP;
    g_manual_command_tick = HAL_GetTick();
    g_control_mode = ESP32_CONTROL_MODE_PICKUP;
    car_stop();
    esp32_uart7_start_receive();
}

void esp32_control_process(void)
{
    static esp32_control_mode_t last_mode = ESP32_CONTROL_MODE_PICKUP;
    static uint8_t last_command = 0xFFU;
    esp32_control_mode_t mode = g_control_mode;
    uint8_t command = g_manual_command;
    uint32_t now = HAL_GetTick();

    if (mode == ESP32_CONTROL_MODE_MANUAL)
    {
        if ((now - g_manual_command_tick) > ESP32_MANUAL_TIMEOUT_MS)
        {
            command = ESP32_CMD_STOP;
        }

        if (mode != last_mode || command != last_command)
        {
            esp32_apply_manual_command(command);
            last_command = command;
        }
    }
    else
    {
        if (last_mode != mode || last_command != ESP32_CMD_STOP)
        {
            car_stop();
            last_command = ESP32_CMD_STOP;
        }
    }

    last_mode = mode;
}

esp32_control_mode_t esp32_control_get_mode(void)
{
    return g_control_mode;
}

void esp32_control_set_mode(esp32_control_mode_t mode)
{
    if (mode != ESP32_CONTROL_MODE_MANUAL && mode != ESP32_CONTROL_MODE_PICKUP)
    {
        return;
    }

    esp32_set_mode_internal(mode);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint8_t byte;

    if (huart != &huart7)
    {
        return;
    }

    byte = g_uart7_rx_byte;
    switch (g_parser_state)
    {
    case 0U:
        if (byte == ESP32_PACKET_HEADER)
        {
            g_parser_state = 1U;
        }
        break;
    case 1U:
        g_pending_code = byte;
        g_parser_state = 2U;
        break;
    case 2U:
        if (byte == ESP32_PACKET_TAIL)
        {
            esp32_handle_code(g_pending_code);
        }
        g_parser_state = 0U;
        break;
    default:
        g_parser_state = 0U;
        break;
    }

    esp32_uart7_start_receive();
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart != &huart7)
    {
        return;
    }

    __HAL_UART_CLEAR_OREFLAG(&huart7);
    g_parser_state = 0U;
    esp32_uart7_start_receive();
}
