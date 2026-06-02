#ifndef __MOTOR_H
#define __MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define MOTOR_A   0U
#define MOTOR_B   1U
#define MOTOR_C   2U
#define MOTOR_D   3U

#define MOTOR_00  0U
#define MOTOR_01  1U
#define MOTOR_10  2U
#define MOTOR_11  3U

void motor_init(void);
void motor_run(uint8_t motor, uint8_t direction, uint8_t duty_percent);
void motor_stop_all(void);

#ifdef __cplusplus
}
#endif

#endif
