#include "car_ball_pid.h"

#include "carcontrol.h"
#include "openmv_uart.h"
#include "servo_uart5.h"

typedef struct
{
    float kp;
    float ki;
    float kd;
    float sum;
    float last;
} car_pid_t;

#define CAR_TRACK_INVALID_COORD            0xFFFFU
#define CAR_TRACK_FRAME_CENTER_X           320.0f
#define CAR_TRACK_PICKUP_TARGET_Y          365.0f
#define CAR_TRACK_PICKUP_STOP_Y            360.0f
#define CAR_TRACK_PICKUP_STOP_HEADING_ERR  70.0f
#define CAR_TRACK_TIMEOUT_MS               250U
#define CAR_TRACK_CONTROL_PERIOD_MS        20U
#define CAR_TRACK_PICKUP_SIGNAL_ENABLE     1U
#define CAR_TRACK_PICKUP_HOLD_MS           5000U
#define CAR_TRACK_SEARCH_ROTATE_DELAY_MS   10000U
#define CAR_TRACK_SEARCH_ROTATE_PWM        12.0f

#define CAR_TRACK_SPEED_KP                 0.34f
#define CAR_TRACK_SPEED_KI                 0.0012f
#define CAR_TRACK_SPEED_KD                 0.015f
#define CAR_TRACK_MAX_BASE_PWM             38.0f
#define CAR_TRACK_MIN_BASE_PWM             18.0f
#define CAR_TRACK_CLOSE_PICKUP_Y           320.0f
#define CAR_TRACK_CLOSE_FORWARD_HOLD_MS    2000U
#define CAR_TRACK_CLOSE_FORWARD_PWM        22.0f
#define CAR_TRACK_MAX_TURN_PWM             24.0f
#define CAR_TRACK_MIN_TURN_PWM             6.0f
#define CAR_TRACK_FIXED_TURN_PWM           14.0f
#define CAR_TRACK_FORWARD_PAN_ALIGN_ERR    140.0f
#define CAR_TRACK_FORWARD_HEADING_START    110.0f
#define CAR_TRACK_FORWARD_HEADING_STOP     280.0f
#define CAR_TRACK_TURN_DEADBAND            40.0f
#define CAR_TRACK_OUTPUT_SLEW_PER_UPDATE   2.0f
#define CAR_TRACK_MAX_WHEEL_PWM            34.0f
#define CAR_TRACK_TARGET_FILTER_ALPHA      0.28f
#define CAR_TRACK_PAN_FILTER_ALPHA         0.15f
#define CAR_TRACK_IMG_TO_PAN_GAIN          0.55f
#define CAR_TRACK_HEADING_BLEND_PAN        1.00f
#define CAR_TRACK_HEADING_BLEND_IMG        0.45f

static car_pid_t g_car_speed_pid = {CAR_TRACK_SPEED_KP, CAR_TRACK_SPEED_KI, CAR_TRACK_SPEED_KD, 0.0f, 0.0f};
static uint32_t g_last_uart_seq = 0;
static uint32_t g_last_target_tick = 0;
static uint32_t g_last_control_tick = 0;
static uint32_t g_pickup_hold_until_tick = 0;
static uint32_t g_close_forward_until_tick = 0;
static uint32_t g_no_target_since_tick = 0;
static uint8_t g_target_valid = 0;
static float g_filtered_ball_x = CAR_TRACK_FRAME_CENTER_X;
static float g_filtered_ball_y = 0.0f;
static float g_filtered_pan_error = 0.0f;
static float g_left_cmd = 0.0f;
static float g_right_cmd = 0.0f;

static float car_ball_absf(float value)
{
    if (value < 0.0f)
    {
        return -value;
    }

    return value;
}

static float car_ball_clampf(float value, float min_value, float max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }

    return value;
}

static float car_ball_pid_step(car_pid_t *pid, float error)
{
    float derivative;

    pid->sum += error;
    if (pid->sum > 200.0f)
    {
        pid->sum = 200.0f;
    }
    else if (pid->sum < -200.0f)
    {
        pid->sum = -200.0f;
    }

    derivative = error - pid->last;
    pid->last = error;
    return (error * pid->kp) + (pid->sum * pid->ki) + (derivative * pid->kd);
}

static void car_ball_pid_reset_state(void)
{
    g_car_speed_pid.sum = 0.0f;
    g_car_speed_pid.last = 0.0f;
}

static float car_ball_apply_slew(float current, float target, float max_step)
{
    float delta = target - current;

    if (delta > max_step)
    {
        delta = max_step;
    }
    else if (delta < -max_step)
    {
        delta = -max_step;
    }

    return current + delta;
}

static void car_ball_set_pickup_signal(uint8_t detected)
{
    (void)detected;
#if CAR_TRACK_PICKUP_SIGNAL_ENABLE
    HAL_GPIO_WritePin(PICKUP_EN_GPIO_Port, PICKUP_EN_Pin, detected ? GPIO_PIN_SET : GPIO_PIN_RESET);
#else
    HAL_GPIO_WritePin(PICKUP_EN_GPIO_Port, PICKUP_EN_Pin, GPIO_PIN_RESET);
#endif
}

static uint8_t car_ball_pickup_hold_active(uint32_t now)
{
    return ((int32_t)(g_pickup_hold_until_tick - now) > 0) ? 1U : 0U;
}

static int16_t car_ball_pid_limit_pwm(float pwm_value)
{
    if (pwm_value > 100.0f)
    {
        pwm_value = 100.0f;
    }
    else if (pwm_value < -100.0f)
    {
        pwm_value = -100.0f;
    }

    return (int16_t)pwm_value;
}

void car_ball_pid_init(void)
{
    car_ball_pid_reset_state();
    g_last_uart_seq = openmv_uart_get_seq();
    g_last_target_tick = 0;
    g_last_control_tick = 0;
    g_pickup_hold_until_tick = 0;
    g_close_forward_until_tick = 0;
    g_no_target_since_tick = 0;
    g_target_valid = 0;
    g_filtered_ball_x = CAR_TRACK_FRAME_CENTER_X;
    g_filtered_ball_y = 0.0f;
    g_filtered_pan_error = 0.0f;
    g_left_cmd = 0.0f;
    g_right_cmd = 0.0f;
    car_ball_set_pickup_signal(0U);
    car_stop();
}

void car_ball_pid_track_from_uart(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t seq = openmv_uart_get_seq();
    openmv_uart_data_t data;
    float image_x_error;
    float pan_error;
    float heading_error;
    float abs_heading_error;
    float distance_error;
    float linear_pwm;
    float angular_pwm;
    float forward_scale;
    float target_left;
    float target_right;

    if (seq != g_last_uart_seq)
    {
        g_last_uart_seq = seq;
        openmv_uart_get_data(&data);

        if ((now - data.last_update_tick) <= CAR_TRACK_TIMEOUT_MS &&
            data.ball_x != CAR_TRACK_INVALID_COORD &&
            data.ball_y != CAR_TRACK_INVALID_COORD)
        {
            if (g_target_valid == 0U)
            {
                g_filtered_ball_x = (float)data.ball_x;
                g_filtered_ball_y = (float)data.ball_y;
            }
            else
            {
                g_filtered_ball_x += (((float)data.ball_x - g_filtered_ball_x) * CAR_TRACK_TARGET_FILTER_ALPHA);
                g_filtered_ball_y += (((float)data.ball_y - g_filtered_ball_y) * CAR_TRACK_TARGET_FILTER_ALPHA);
            }
            g_target_valid = 1U;
            g_last_target_tick = now;
            g_pickup_hold_until_tick = now + CAR_TRACK_PICKUP_HOLD_MS;
            g_no_target_since_tick = 0U;
            if (g_filtered_ball_y >= CAR_TRACK_CLOSE_PICKUP_Y && now >= g_close_forward_until_tick)
            {
                g_close_forward_until_tick = now + CAR_TRACK_CLOSE_FORWARD_HOLD_MS;
            }
            car_ball_set_pickup_signal(1U);
        }
    }

    if (g_target_valid == 0U || (now - g_last_target_tick) > CAR_TRACK_TIMEOUT_MS)
    {
        g_target_valid = 0U;
        if (g_no_target_since_tick == 0U)
        {
            g_no_target_since_tick = now;
        }
        if (car_ball_pickup_hold_active(now) != 0U)
        {
            car_ball_set_pickup_signal(1U);
        }
        else
        {
            car_ball_set_pickup_signal(0U);
        }
        car_ball_pid_reset_state();
        if ((now - g_no_target_since_tick) >= CAR_TRACK_SEARCH_ROTATE_DELAY_MS)
        {
            g_left_cmd = car_ball_apply_slew(g_left_cmd, -CAR_TRACK_SEARCH_ROTATE_PWM, CAR_TRACK_OUTPUT_SLEW_PER_UPDATE);
            g_right_cmd = car_ball_apply_slew(g_right_cmd, CAR_TRACK_SEARCH_ROTATE_PWM, CAR_TRACK_OUTPUT_SLEW_PER_UPDATE);
        }
        else
        {
            g_left_cmd = car_ball_apply_slew(g_left_cmd, 0.0f, CAR_TRACK_OUTPUT_SLEW_PER_UPDATE);
            g_right_cmd = car_ball_apply_slew(g_right_cmd, 0.0f, CAR_TRACK_OUTPUT_SLEW_PER_UPDATE);
        }
        car_set_lr(car_ball_pid_limit_pwm(g_left_cmd), car_ball_pid_limit_pwm(g_right_cmd));
        if ((now - g_no_target_since_tick) < CAR_TRACK_SEARCH_ROTATE_DELAY_MS &&
            car_ball_absf(g_left_cmd) < 1.0f && car_ball_absf(g_right_cmd) < 1.0f)
        {
            car_stop();
        }
        return;
    }

    car_ball_set_pickup_signal(1U);

    if ((now - g_last_control_tick) < CAR_TRACK_CONTROL_PERIOD_MS)
    {
        return;
    }
    g_last_control_tick = now;

    pan_error = (float)servo_get_pan_offset_from_center();
    g_filtered_pan_error += ((pan_error - g_filtered_pan_error) * CAR_TRACK_PAN_FILTER_ALPHA);
    image_x_error = (g_filtered_ball_x - CAR_TRACK_FRAME_CENTER_X) * CAR_TRACK_IMG_TO_PAN_GAIN;
    heading_error = (g_filtered_pan_error * CAR_TRACK_HEADING_BLEND_PAN) + (image_x_error * CAR_TRACK_HEADING_BLEND_IMG);
    abs_heading_error = car_ball_absf(heading_error);

    if (now < g_close_forward_until_tick)
    {
        angular_pwm = 0.0f;
        linear_pwm = CAR_TRACK_CLOSE_FORWARD_PWM;
        target_left = car_ball_clampf(linear_pwm, -CAR_TRACK_MAX_WHEEL_PWM, CAR_TRACK_MAX_WHEEL_PWM);
        target_right = car_ball_clampf(linear_pwm, -CAR_TRACK_MAX_WHEEL_PWM, CAR_TRACK_MAX_WHEEL_PWM);
        g_left_cmd = car_ball_apply_slew(g_left_cmd, target_left, CAR_TRACK_OUTPUT_SLEW_PER_UPDATE);
        g_right_cmd = car_ball_apply_slew(g_right_cmd, target_right, CAR_TRACK_OUTPUT_SLEW_PER_UPDATE);
        car_set_lr(car_ball_pid_limit_pwm(g_left_cmd), car_ball_pid_limit_pwm(g_right_cmd));
        return;
    }

    if (abs_heading_error <= CAR_TRACK_PICKUP_STOP_HEADING_ERR && g_filtered_ball_y >= CAR_TRACK_PICKUP_STOP_Y)
    {
        car_ball_pid_reset_state();
        g_left_cmd = car_ball_apply_slew(g_left_cmd, 0.0f, CAR_TRACK_OUTPUT_SLEW_PER_UPDATE);
        g_right_cmd = car_ball_apply_slew(g_right_cmd, 0.0f, CAR_TRACK_OUTPUT_SLEW_PER_UPDATE);
        car_set_lr(car_ball_pid_limit_pwm(g_left_cmd), car_ball_pid_limit_pwm(g_right_cmd));
        return;
    }

    if (abs_heading_error <= CAR_TRACK_TURN_DEADBAND)
    {
        angular_pwm = 0.0f;
    }
    else
    {
        angular_pwm = (heading_error >= 0.0f) ? CAR_TRACK_FIXED_TURN_PWM : -CAR_TRACK_FIXED_TURN_PWM;
    }

    distance_error = CAR_TRACK_PICKUP_TARGET_Y - g_filtered_ball_y;
    if (distance_error < 0.0f)
    {
        distance_error = 0.0f;
    }

    linear_pwm = car_ball_pid_step(&g_car_speed_pid, distance_error);
    if (linear_pwm < 0.0f)
    {
        linear_pwm = 0.0f;
    }
    linear_pwm = car_ball_clampf(linear_pwm, 0.0f, CAR_TRACK_MAX_BASE_PWM);
    if (linear_pwm > 0.0f && linear_pwm < CAR_TRACK_MIN_BASE_PWM)
    {
        linear_pwm = CAR_TRACK_MIN_BASE_PWM;
    }

    if (car_ball_absf(g_filtered_pan_error) > CAR_TRACK_FORWARD_PAN_ALIGN_ERR)
    {
        forward_scale = 0.0f;
    }
    else if (abs_heading_error >= CAR_TRACK_FORWARD_HEADING_STOP)
    {
        forward_scale = 0.0f;
    }
    else if (abs_heading_error <= CAR_TRACK_FORWARD_HEADING_START)
    {
        forward_scale = 1.0f;
    }
    else
    {
        forward_scale = 1.0f - ((abs_heading_error - CAR_TRACK_FORWARD_HEADING_START) /
                         (CAR_TRACK_FORWARD_HEADING_STOP - CAR_TRACK_FORWARD_HEADING_START));
        forward_scale = forward_scale * forward_scale;
    }

    linear_pwm *= forward_scale;

    target_left = car_ball_clampf(linear_pwm - angular_pwm, -CAR_TRACK_MAX_WHEEL_PWM, CAR_TRACK_MAX_WHEEL_PWM);
    target_right = car_ball_clampf(linear_pwm + angular_pwm, -CAR_TRACK_MAX_WHEEL_PWM, CAR_TRACK_MAX_WHEEL_PWM);
    g_left_cmd = car_ball_apply_slew(g_left_cmd, target_left, CAR_TRACK_OUTPUT_SLEW_PER_UPDATE);
    g_right_cmd = car_ball_apply_slew(g_right_cmd, target_right, CAR_TRACK_OUTPUT_SLEW_PER_UPDATE);

    car_set_lr(car_ball_pid_limit_pwm(g_left_cmd), car_ball_pid_limit_pwm(g_right_cmd));
}
