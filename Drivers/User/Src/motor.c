#include "motor.h"

extern TIM_HandleTypeDef htim2;

static uint32_t motor_percent_to_compare(uint8_t duty_percent)
{
    uint32_t period;

    if (duty_percent > 100U)
    {
        duty_percent = 100U;
    }

    period = __HAL_TIM_GET_AUTORELOAD(&htim2) + 1U;
    return (period * duty_percent) / 100U;
}

static int motor_is_valid(uint8_t motor)
{
    return motor <= MOTOR_D;
}

void motor_run(uint8_t motor, uint8_t direction, uint8_t duty_percent)
{
    GPIO_PinState in1 = GPIO_PIN_RESET;
    GPIO_PinState in2 = GPIO_PIN_RESET;
    uint32_t channel = 0U;

    if (!motor_is_valid(motor))
    {
        return;
    }

    switch (direction)
    {
    case MOTOR_01:
        in1 = GPIO_PIN_RESET;
        in2 = GPIO_PIN_SET;
        break;
    case MOTOR_10:
        in1 = GPIO_PIN_SET;
        in2 = GPIO_PIN_RESET;
        break;
    case MOTOR_11:
        in1 = GPIO_PIN_SET;
        in2 = GPIO_PIN_SET;
        break;
    case MOTOR_00:
    default:
        in1 = GPIO_PIN_RESET;
        in2 = GPIO_PIN_RESET;
        break;
    }

    switch (motor)
    {
    case MOTOR_A:
        channel = TIM_CHANNEL_1;
        HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, in1);
        HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, in2);
        break;
    case MOTOR_B:
        channel = TIM_CHANNEL_2;
        HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, in1);
        HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, in2);
        break;
    case MOTOR_C:
        channel = TIM_CHANNEL_3;
        HAL_GPIO_WritePin(CIN1_GPIO_Port, CIN1_Pin, in1);
        HAL_GPIO_WritePin(CIN2_GPIO_Port, CIN2_Pin, in2);
        break;
    case MOTOR_D:
        channel = TIM_CHANNEL_4;
        HAL_GPIO_WritePin(DIN1_GPIO_Port, DIN1_Pin, in1);
        HAL_GPIO_WritePin(DIN2_GPIO_Port, DIN2_Pin, in2);
        break;
    default:
        break;
    }

    __HAL_TIM_SET_COMPARE(&htim2, channel, motor_percent_to_compare(duty_percent));
}

void motor_stop_all(void)
{
    motor_run(MOTOR_A, MOTOR_00, 0U);
    motor_run(MOTOR_B, MOTOR_00, 0U);
    motor_run(MOTOR_C, MOTOR_00, 0U);
    motor_run(MOTOR_D, MOTOR_00, 0U);
}

void motor_init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);

    motor_stop_all();
}
