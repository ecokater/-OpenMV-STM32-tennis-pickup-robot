#ifndef __CARCONTROL_H
#define __CARCONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "motor.h"

/* A/B/C/D: 右前 / 右后 / 左后 / 左前 */
#define CAR_RF_MOTOR          MOTOR_A
#define CAR_RR_MOTOR          MOTOR_B
#define CAR_LR_MOTOR          MOTOR_C
#define CAR_LF_MOTOR          MOTOR_D

/*
 * 前进方向可按实测修改:
 * MOTOR_10: IN1=1 IN2=0
 * MOTOR_01: IN1=0 IN2=1
 */
#define CAR_RF_FORWARD_DIR    MOTOR_10
#define CAR_RR_FORWARD_DIR    MOTOR_10
#define CAR_LR_FORWARD_DIR    MOTOR_10
#define CAR_LF_FORWARD_DIR    MOTOR_10

void car_init(void);
void car_stop(void);
void car_forward(uint8_t pwm);
void car_backward(uint8_t pwm);
void car_turn_left(uint8_t pwm);
void car_turn_right(uint8_t pwm);
void car_spin_left(uint8_t pwm);
void car_spin_right(uint8_t pwm);
void car_set_lr(int16_t left_pwm, int16_t right_pwm);
void car_set_wheels(int16_t rf_pwm, int16_t rr_pwm, int16_t lr_pwm, int16_t lf_pwm);

#ifdef __cplusplus
}
#endif

#endif
