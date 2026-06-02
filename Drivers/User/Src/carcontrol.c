#include "carcontrol.h"

static uint8_t car_limit_pwm(int16_t pwm)
{
    if (pwm < 0)
    {
        pwm = -pwm;
    }

    if (pwm > 100)
    {
        pwm = 100;
    }

    return (uint8_t)pwm;
}

static uint8_t car_reverse_dir(uint8_t dir)
{
    if (dir == MOTOR_10)
    {
        return MOTOR_01;
    }
    if (dir == MOTOR_01)
    {
        return MOTOR_10;
    }
    return dir;
}

static void car_drive_motor(uint8_t motor, uint8_t forward_dir, int16_t pwm)
{
    if (pwm > 0)
    {
        motor_run(motor, forward_dir, car_limit_pwm(pwm));
    }
    else if (pwm < 0)
    {
        motor_run(motor, car_reverse_dir(forward_dir), car_limit_pwm(pwm));
    }
    else
    {
        motor_run(motor, MOTOR_00, 0);
    }
}

void car_init(void)
{
    car_stop();
}

void car_stop(void)
{
    motor_stop_all();
}

void car_forward(uint8_t pwm)
{
    car_set_lr((int16_t)pwm, (int16_t)pwm);
}

void car_backward(uint8_t pwm)
{
    car_set_lr(-(int16_t)pwm, -(int16_t)pwm);
}

void car_turn_left(uint8_t pwm)
{
    car_set_lr(-(int16_t)(pwm / 2U), (int16_t)pwm);
}

void car_turn_right(uint8_t pwm)
{
    car_set_lr((int16_t)pwm, -(int16_t)(pwm / 2U));
}

void car_spin_left(uint8_t pwm)
{
    car_set_lr((int16_t)pwm, -(int16_t)pwm);
}

void car_spin_right(uint8_t pwm)
{
    car_set_lr(-(int16_t)pwm, (int16_t)pwm);
}

void car_set_lr(int16_t left_pwm, int16_t right_pwm)
{
    car_drive_motor(CAR_RF_MOTOR, CAR_RF_FORWARD_DIR, right_pwm);
    car_drive_motor(CAR_RR_MOTOR, CAR_RR_FORWARD_DIR, right_pwm);
    car_drive_motor(CAR_LR_MOTOR, CAR_LR_FORWARD_DIR, left_pwm);
    car_drive_motor(CAR_LF_MOTOR, CAR_LF_FORWARD_DIR, left_pwm);
}

void car_set_wheels(int16_t rf_pwm, int16_t rr_pwm, int16_t lr_pwm, int16_t lf_pwm)
{
    car_drive_motor(CAR_RF_MOTOR, CAR_RF_FORWARD_DIR, rf_pwm);
    car_drive_motor(CAR_RR_MOTOR, CAR_RR_FORWARD_DIR, rr_pwm);
    car_drive_motor(CAR_LR_MOTOR, CAR_LR_FORWARD_DIR, lr_pwm);
    car_drive_motor(CAR_LF_MOTOR, CAR_LF_FORWARD_DIR, lf_pwm);
}
