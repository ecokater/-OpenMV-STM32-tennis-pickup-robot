#include "servo_uart5.h"
#include "openmv_uart.h"

extern UART_HandleTypeDef huart5;

#define SERVO_PAN_ID                     000
#define SERVO_TILT_ID                    001
#define PID_PAN_KP                       0.06f
#define PID_PAN_KI                       0.0f
#define PID_PAN_KD                       0.008f
#define PID_TILT_KP                      0.10f
#define PID_TILT_KI                      0.0f
#define PID_TILT_KD                      0.010f
#define PID_PAN_SCALE                    1.0f
#define PID_TILT_SCALE                   1.0f
#define FRAME_CENTER_X                   320.0f
#define FRAME_CENTER_Y                   240.0f
#define INVALID_BALL_COORD               0xFFFFU
#define PID_PAN_MIN_STEP_ERR_THRESHOLD   22.0f
#define PID_PAN_MIN_STEP_OUTPUT          1.0f
#define PID_TILT_MIN_STEP_ERR_THRESHOLD  14.0f
#define PID_TILT_MIN_STEP_OUTPUT         2.0f
#define PID_PAN_DEADBAND                 14.0f
#define PID_TILT_DEADBAND                8.0f
#define PID_PAN_MAX_OUTPUT               6.0f
#define PID_TILT_MAX_OUTPUT              10.0f
#define SERVO_TILT_MIN_LIMIT             1100
#define SERVO_TILT_HORIZON_LIMIT         1300
#define SERVO_SCAN_LEFT_LIMIT            1100
#define SERVO_SCAN_RIGHT_LIMIT           1900
#define SERVO_SCAN_STEP                  10
#define SERVO_SCAN_MOVE_TIME_MS          70
#define SERVO_SCAN_PERIOD_MS             90U
#define SERVO_TRACK_UPDATE_PERIOD_MS     30U
#define SERVO_TARGET_TIMEOUT_MS          160U
#define SERVO_LOST_TILT_RESET_DELAY_MS   5000U
#define SERVO_TRACK_MOVE_TIME_MS         45
#define SERVO_TRACK_FILTER_ALPHA         0.18f

static pid_t g_pid_pan = {PID_PAN_KP, PID_PAN_KI, PID_PAN_KD, 0.0f, 0.0f};
static pid_t g_pid_tilt = {PID_TILT_KP, PID_TILT_KI, PID_TILT_KD, 0.0f, 0.0f};
static int32_t g_current_pan = 1500;
static int32_t g_current_tilt = 1300;
static uint32_t g_last_uart_seq = 0;
static int32_t g_scan_direction = 1;
static uint32_t g_last_scan_tick = 0;
static uint32_t g_last_track_cmd_tick = 0;
static uint32_t g_last_target_tick = 0;
static uint8_t g_target_valid = 0;
static uint8_t g_tilt_reset_after_lost_done = 0;
static float g_filtered_ball_x = FRAME_CENTER_X;
static float g_filtered_ball_y = FRAME_CENTER_Y;

static float servo_absf(float value)
{
    if (value < 0.0f)
    {
        return -value;
    }

    return value;
}

static int32_t servo_float_to_int(float value)
{
    if (value >= 0.0f)
    {
        return (int32_t)(value + 0.5f);
    }

    return (int32_t)(value - 0.5f);
}

static void servo_pid_reset_state(void)
{
    g_pid_pan.sum = 0.0f;
    g_pid_pan.last = 0.0f;
    g_pid_tilt.sum = 0.0f;
    g_pid_tilt.last = 0.0f;
}

static void servo_scan_step_once(uint32_t now)
{
    if ((now - g_last_scan_tick) < SERVO_SCAN_PERIOD_MS)
    {
        return;
    }

    g_last_scan_tick = now;
    g_current_pan += (g_scan_direction * SERVO_SCAN_STEP);
    if (g_current_pan >= SERVO_SCAN_RIGHT_LIMIT)
    {
        g_current_pan = SERVO_SCAN_RIGHT_LIMIT;
        g_scan_direction = -1;
    }
    else if (g_current_pan <= SERVO_SCAN_LEFT_LIMIT)
    {
        g_current_pan = SERVO_SCAN_LEFT_LIMIT;
        g_scan_direction = 1;
    }

    move_servo(SERVO_PAN_ID, (uint16_t)g_current_pan, SERVO_SCAN_MOVE_TIME_MS);
}

static void servo_uart5_clear_rx(void)
{
    volatile uint32_t dummy;

    while ((huart5.Instance->ISR & USART_ISR_RXNE_RXFNE) != 0U)
    {
        dummy = huart5.Instance->RDR;
        (void)dummy;
    }

    __HAL_UART_CLEAR_OREFLAG(&huart5);
}

static HAL_StatusTypeDef servo_uart5_recover(void)
{
    HAL_StatusTypeDef init_status;
    HAL_StatusTypeDef txfifo_status;
    HAL_StatusTypeDef rxfifo_status;
    HAL_StatusTypeDef fifo_disable_status;

    (void)HAL_UART_Abort(&huart5);
    (void)HAL_UART_DeInit(&huart5);
    init_status = HAL_UART_Init(&huart5);
    txfifo_status = HAL_UARTEx_SetTxFifoThreshold(&huart5, UART_TXFIFO_THRESHOLD_1_8);
    rxfifo_status = HAL_UARTEx_SetRxFifoThreshold(&huart5, UART_RXFIFO_THRESHOLD_1_8);
    fifo_disable_status = HAL_UARTEx_DisableFifoMode(&huart5);

    if (init_status != HAL_OK || txfifo_status != HAL_OK || rxfifo_status != HAL_OK || fifo_disable_status != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

static HAL_StatusTypeDef servo_uart5_transmit_internal(const char *cmd)
{
    uint16_t len;
    HAL_StatusTypeDef status;
    HAL_StatusTypeDef retry_status;

    if (cmd == NULL)
    {
        return HAL_ERROR;
    }

    len = (uint16_t)strlen(cmd);
    servo_uart5_clear_rx();
    status = HAL_UART_Transmit(&huart5, (uint8_t *)cmd, len, 100);

    if (status == HAL_TIMEOUT)
    {
        if (servo_uart5_recover() == HAL_OK)
        {
            servo_uart5_clear_rx();
            retry_status = HAL_UART_Transmit(&huart5, (uint8_t *)cmd, len, 100);
            return retry_status;
        }
    }

    return status;
}

static int servo_is_digit_char(char c)
{
    return c >= '0' && c <= '9';
}

static HAL_StatusTypeDef servo_parse_angle_response(const char *resp, uint16_t expected_id, uint16_t *out_position)
{
    uint16_t resp_id;
    uint16_t position;

    if (resp == NULL || out_position == NULL)
    {
        return HAL_ERROR;
    }

    if (resp[0] != '#' || resp[4] != 'P' || resp[9] != '!' || resp[10] != '\0')
    {
        return HAL_ERROR;
    }

    if (!servo_is_digit_char(resp[1]) || !servo_is_digit_char(resp[2]) || !servo_is_digit_char(resp[3]) ||
        !servo_is_digit_char(resp[5]) || !servo_is_digit_char(resp[6]) || !servo_is_digit_char(resp[7]) || !servo_is_digit_char(resp[8]))
    {
        return HAL_ERROR;
    }

    resp_id = (uint16_t)((resp[1] - '0') * 100 + (resp[2] - '0') * 10 + (resp[3] - '0'));
    position = (uint16_t)((resp[5] - '0') * 1000 + (resp[6] - '0') * 100 + (resp[7] - '0') * 10 + (resp[8] - '0'));

    if (resp_id != expected_id)
    {
        return HAL_ERROR;
    }

    *out_position = position;
    return HAL_OK;
}

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
    (void)servo_uart5_transmit_internal(cmd);
}

void move_servo(uint16_t servo_id, uint16_t position, uint16_t time_ms)
{
    char cmd[24];
    if (position < 500) position = 500;
    if (position > 2500) position = 2500;
    if (servo_id == SERVO_TILT_ID && position < SERVO_TILT_MIN_LIMIT) position = SERVO_TILT_MIN_LIMIT;
    if (servo_id == SERVO_TILT_ID && position > SERVO_TILT_HORIZON_LIMIT) position = SERVO_TILT_HORIZON_LIMIT;

    snprintf(cmd, sizeof(cmd), "#%03uP%04uT%04u!", servo_id, position, time_ms);
    send_servo_command(cmd);
}

HAL_StatusTypeDef servo_read_angle(uint16_t servo_id, uint16_t *out_position)
{
    char cmd[16];
    char resp[16];
    uint8_t ch;
    uint32_t index = 0;
    HAL_StatusTypeDef status;

    if (out_position == NULL)
    {
        return HAL_ERROR;
    }

    snprintf(cmd, sizeof(cmd), "#%03uPRAD!", servo_id);
    status = servo_uart5_transmit_internal(cmd);
    if (status != HAL_OK)
    {
        return status;
    }

    memset(resp, 0, sizeof(resp));
    while (index < (sizeof(resp) - 1U))
    {
        status = HAL_UART_Receive(&huart5, &ch, 1, 100);
        if (status != HAL_OK)
        {
            return status;
        }

        resp[index++] = (char)ch;
        if (ch == '!')
        {
            break;
        }
    }

    resp[index] = '\0';
    return servo_parse_angle_response(resp, servo_id, out_position);
}

void servo_startup_self_test(void)
{
    move_servo(SERVO_PAN_ID, 1500, 300);
    move_servo(SERVO_TILT_ID, 1300, 300);
    HAL_Delay(500);

    move_servo(SERVO_PAN_ID, 1800, 450);
    HAL_Delay(500);
    move_servo(SERVO_PAN_ID, 1200, 450);
    HAL_Delay(500);
    move_servo(SERVO_PAN_ID, 1500, 300);
    HAL_Delay(500);

    move_servo(SERVO_TILT_ID, 1100, 450);
    HAL_Delay(500);
    move_servo(SERVO_TILT_ID, 1300, 450);
    HAL_Delay(500);
    move_servo(SERVO_TILT_ID, 1300, 300);
    HAL_Delay(500);
}

void servo_pid_init(void)
{
    servo_pid_reset_state();
    g_current_pan = 1500;
    g_current_tilt = 1300;
    g_last_uart_seq = openmv_uart_get_seq();
    g_scan_direction = 1;
    g_last_scan_tick = HAL_GetTick();
    g_last_track_cmd_tick = 0;
    g_last_target_tick = 0;
    g_target_valid = 0;
    g_tilt_reset_after_lost_done = 0;
    g_filtered_ball_x = FRAME_CENTER_X;
    g_filtered_ball_y = FRAME_CENTER_Y;
    move_servo(SERVO_PAN_ID, (uint16_t)g_current_pan, 50);
    move_servo(SERVO_TILT_ID, (uint16_t)g_current_tilt, 50);
}

void servo_pid_track_xy(uint16_t ball_x, uint16_t ball_y)
{
    float error_x = (float)ball_x - FRAME_CENTER_X;
    float error_y = (float)ball_y - FRAME_CENTER_Y;
    float pan_output = pid_step(&g_pid_pan, error_x, PID_PAN_SCALE);
    float tilt_output = pid_step(&g_pid_tilt, error_y, PID_TILT_SCALE);
    if (servo_absf(error_x) <= PID_PAN_DEADBAND) pan_output = 0.0f;
    if (servo_absf(error_y) <= PID_TILT_DEADBAND) tilt_output = 0.0f;
    if (error_x > PID_PAN_MIN_STEP_ERR_THRESHOLD && pan_output < PID_PAN_MIN_STEP_OUTPUT) pan_output = PID_PAN_MIN_STEP_OUTPUT;
    if (error_x < -PID_PAN_MIN_STEP_ERR_THRESHOLD && pan_output > -PID_PAN_MIN_STEP_OUTPUT) pan_output = -PID_PAN_MIN_STEP_OUTPUT;
    if (error_y > PID_TILT_MIN_STEP_ERR_THRESHOLD && tilt_output < PID_TILT_MIN_STEP_OUTPUT) tilt_output = PID_TILT_MIN_STEP_OUTPUT;
    if (error_y < -PID_TILT_MIN_STEP_ERR_THRESHOLD && tilt_output > -PID_TILT_MIN_STEP_OUTPUT) tilt_output = -PID_TILT_MIN_STEP_OUTPUT;
    if (pan_output > PID_PAN_MAX_OUTPUT) pan_output = PID_PAN_MAX_OUTPUT;
    if (pan_output < -PID_PAN_MAX_OUTPUT) pan_output = -PID_PAN_MAX_OUTPUT;
    if (tilt_output > PID_TILT_MAX_OUTPUT) tilt_output = PID_TILT_MAX_OUTPUT;
    if (tilt_output < -PID_TILT_MAX_OUTPUT) tilt_output = -PID_TILT_MAX_OUTPUT;

    g_current_pan -= servo_float_to_int(pan_output);
    g_current_tilt -= servo_float_to_int(tilt_output);

    if (g_current_pan < 500) g_current_pan = 500;
    if (g_current_pan > 2500) g_current_pan = 2500;
    if (g_current_tilt < SERVO_TILT_MIN_LIMIT) g_current_tilt = SERVO_TILT_MIN_LIMIT;
    if (g_current_tilt > SERVO_TILT_HORIZON_LIMIT) g_current_tilt = SERVO_TILT_HORIZON_LIMIT;

    move_servo(SERVO_PAN_ID, (uint16_t)g_current_pan, SERVO_TRACK_MOVE_TIME_MS);
    move_servo(SERVO_TILT_ID, (uint16_t)g_current_tilt, SERVO_TRACK_MOVE_TIME_MS);
}

void servo_pid_track_from_uart(void)
{
    uint32_t seq = openmv_uart_get_seq();
    openmv_uart_data_t data;
    uint32_t now = HAL_GetTick();

    if (seq != g_last_uart_seq)
    {
        g_last_uart_seq = seq;
        openmv_uart_get_data(&data);
        if (data.ball_x != INVALID_BALL_COORD && data.ball_y != INVALID_BALL_COORD)
        {
            if (g_target_valid == 0U)
            {
                g_filtered_ball_x = (float)data.ball_x;
                g_filtered_ball_y = (float)data.ball_y;
            }
            else
            {
                g_filtered_ball_x += (((float)data.ball_x - g_filtered_ball_x) * SERVO_TRACK_FILTER_ALPHA);
                g_filtered_ball_y += (((float)data.ball_y - g_filtered_ball_y) * SERVO_TRACK_FILTER_ALPHA);
            }
            g_target_valid = 1U;
            g_tilt_reset_after_lost_done = 0U;
            g_last_target_tick = now;
        }
        else
        {
            g_target_valid = 0U;
        }
    }

    if (g_target_valid != 0U && (now - g_last_target_tick) <= SERVO_TARGET_TIMEOUT_MS)
    {
        if ((now - g_last_track_cmd_tick) >= SERVO_TRACK_UPDATE_PERIOD_MS)
        {
            g_last_track_cmd_tick = now;
            servo_pid_track_xy((uint16_t)g_filtered_ball_x, (uint16_t)g_filtered_ball_y);
        }
        return;
    }

    g_target_valid = 0U;
    servo_pid_reset_state();
    if (g_tilt_reset_after_lost_done == 0U &&
        (now - g_last_target_tick) >= SERVO_LOST_TILT_RESET_DELAY_MS)
    {
        g_current_tilt = 1300;
        move_servo(SERVO_TILT_ID, (uint16_t)g_current_tilt, SERVO_TRACK_MOVE_TIME_MS);
        g_tilt_reset_after_lost_done = 1U;
    }
    servo_scan_step_once(now);
}

uint16_t servo_get_pan_position(void)
{
    return (uint16_t)g_current_pan;
}

int16_t servo_get_pan_offset_from_center(void)
{
    return (int16_t)(g_current_pan - 1500);
}
