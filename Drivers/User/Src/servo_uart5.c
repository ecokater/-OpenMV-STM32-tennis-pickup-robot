#include "servo_uart5.h"
#include "openmv_uart.h"

extern UART_HandleTypeDef huart5;

static pid_t g_pid_pan = {0.10f, 0.01f, 0.01f, 0.0f, 0.0f};
static pid_t g_pid_tilt = {0.15f, 0.01f, 0.01f, 0.0f, 0.0f};
static int32_t g_current_pan = 1500;
static int32_t g_current_tilt = 1500;
static uint32_t g_last_uart_seq = 0;

float pid_step(pid_t *pid, float error, float scaler)
{
    float derivative;
    float output;

    pid->sum += error;
    if (pid->sum > 100.0f)
    {
        pid->sum = 100.0f;
    }
    else if (pid->sum < -100.0f)
    {
        pid->sum = -100.0f;
    }

    derivative = error - pid->last;
    pid->last = error;

    output = (error * pid->kp) + (pid->sum * pid->ki) + (derivative * pid->kd);
    return output * scaler;
}

void send_servo_command(const char *cmd)
{
    uint16_t len;
    if (cmd == NULL) return;
    len = (uint16_t)strlen(cmd);
    HAL_UART_Transmit(&huart5, (uint8_t *)cmd, len, 100);
}

void move_servo(uint16_t servo_id, uint16_t position, uint16_t time_ms)
{
    char cmd[24];
    char cmd_crlf[28];
    if (position < 500) position = 500;
    if (position > 2500) position = 2500;

    snprintf(cmd, sizeof(cmd), "#%03uP%04uT%04u!", servo_id, position, time_ms);
    send_servo_command(cmd);
    snprintf(cmd_crlf, sizeof(cmd_crlf), "#%03uP%04uT%04u!\r\n", servo_id, position, time_ms);
    send_servo_command(cmd_crlf);
}

void servo_pid_init(void)
{
    g_pid_pan.sum = 0.0f;
    g_pid_pan.last = 0.0f;
    g_pid_tilt.sum = 0.0f;
    g_pid_tilt.last = 0.0f;
    g_current_pan = 1500;
    g_current_tilt = 1500;
    g_last_uart_seq = openmv_uart_get_seq();
    move_servo(0, (uint16_t)g_current_pan, 50);
    move_servo(1, (uint16_t)g_current_tilt, 50);
}

void servo_pid_track_xy(uint16_t ball_x, uint16_t ball_y)
{
    float error_x = (float)ball_x - 320.0f;
    float error_y = (float)ball_y - 240.0f;
    float pan_output = pid_step(&g_pid_pan, error_x, 1.0f);
    float tilt_output = pid_step(&g_pid_tilt, error_y, 1.0f);

    g_current_pan -= (int32_t)pan_output;
    g_current_tilt -= (int32_t)tilt_output;

    if (g_current_pan < 500) g_current_pan = 500;
    if (g_current_pan > 2500) g_current_pan = 2500;
    if (g_current_tilt < 500) g_current_tilt = 500;
    if (g_current_tilt > 2500) g_current_tilt = 2500;

    move_servo(0, (uint16_t)g_current_pan, 20);
    move_servo(1, (uint16_t)g_current_tilt, 20);
}

void servo_pid_track_from_uart(void)
{
    uint32_t seq = openmv_uart_get_seq();
    openmv_uart_data_t data;

    if (seq == g_last_uart_seq)
    {
        return;
    }

    g_last_uart_seq = seq;
    openmv_uart_get_data(&data);
    servo_pid_track_xy(data.ball_x, data.ball_y);
}
