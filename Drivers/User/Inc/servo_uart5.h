#ifndef SERVO_UART5_H
#define SERVO_UART5_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
    float kp;
    float ki;
    float kd;
    float sum;
    float last;
} pid_t;

float pid_step(pid_t *pid, float error, float scaler);
void send_servo_command(const char *cmd);
void move_servo(uint16_t servo_id, uint16_t position, uint16_t time_ms);
void servo_pid_init(void);
void servo_pid_track_xy(uint16_t ball_x, uint16_t ball_y);
void servo_pid_track_from_uart(void);





#ifdef __cplusplus
}
#endif

#endif
