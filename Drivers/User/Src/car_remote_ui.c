#include "car_remote_ui.h"

#include "carcontrol.h"
#include "esp32_link.h"
#include "lvgl.h"
#include "screens.h"

#define CAR_REMOTE_PWM  70U
#define REMOTE_MODE_SYNC_PERIOD_MS  100U

typedef enum
{
    REMOTE_CMD_STOP = 0,
    REMOTE_CMD_FORWARD,
    REMOTE_CMD_BACKWARD,
    REMOTE_CMD_TURN_LEFT,
    REMOTE_CMD_TURN_RIGHT,
    REMOTE_CMD_SPIN_LEFT,
    REMOTE_CMD_SPIN_RIGHT,
    REMOTE_CMD_TEST
} remote_cmd_t;

static lv_obj_t *g_remote_panel;
static lv_obj_t *g_mode_label;
static lv_obj_t *g_manual_mode_btn;
static lv_obj_t *g_pickup_mode_btn;
static lv_timer_t *g_mode_sync_timer;
static lv_timer_t *g_test_timer;
static uint8_t g_test_step;

static lv_obj_t *remote_get_parent_screen(void)
{
    if (objects.show_view != NULL)
    {
        return objects.show_view;
    }

    return lv_scr_act();
}

static void remote_stop_test(void)
{
    if (g_test_timer != NULL)
    {
        lv_timer_del(g_test_timer);
        g_test_timer = NULL;
    }

    g_test_step = 0U;
}

static void remote_apply_command(remote_cmd_t cmd)
{
    switch (cmd)
    {
    case REMOTE_CMD_FORWARD:
        car_forward(CAR_REMOTE_PWM);
        break;
    case REMOTE_CMD_BACKWARD:
        car_backward(CAR_REMOTE_PWM);
        break;
    case REMOTE_CMD_TURN_LEFT:
        car_turn_left(CAR_REMOTE_PWM);
        break;
    case REMOTE_CMD_TURN_RIGHT:
        car_turn_right(CAR_REMOTE_PWM);
        break;
    case REMOTE_CMD_SPIN_LEFT:
        car_spin_left(CAR_REMOTE_PWM);
        break;
    case REMOTE_CMD_SPIN_RIGHT:
        car_spin_right(CAR_REMOTE_PWM);
        break;
    case REMOTE_CMD_STOP:
    default:
        car_stop();
        break;
    }
}

static void remote_update_mode_label(void)
{
    esp32_control_mode_t mode = esp32_control_get_mode();

    if (g_mode_label == NULL)
    {
        return;
    }

    if (mode == ESP32_CONTROL_MODE_PICKUP)
    {
        lv_label_set_text(g_mode_label, "Mode: Pickup");
    }
    else
    {
        lv_label_set_text(g_mode_label, "Mode: Manual");
    }

    if (g_manual_mode_btn != NULL)
    {
        lv_obj_set_style_bg_color(g_manual_mode_btn,
                                  mode == ESP32_CONTROL_MODE_MANUAL ? lv_palette_main(LV_PALETTE_BLUE)
                                                                    : lv_palette_darken(LV_PALETTE_GREY, 1),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (g_pickup_mode_btn != NULL)
    {
        lv_obj_set_style_bg_color(g_pickup_mode_btn,
                                  mode == ESP32_CONTROL_MODE_PICKUP ? lv_palette_main(LV_PALETTE_GREEN)
                                                                    : lv_palette_darken(LV_PALETTE_GREY, 1),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void remote_mode_sync_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    car_remote_ui_init();
    remote_update_mode_label();
}

static void remote_test_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    switch (g_test_step)
    {
    case 0:
        car_forward(CAR_REMOTE_PWM);
        break;
    case 1:
        car_backward(CAR_REMOTE_PWM);
        break;
    case 2:
        car_turn_left(CAR_REMOTE_PWM);
        break;
    case 3:
        car_turn_right(CAR_REMOTE_PWM);
        break;
    case 4:
        car_spin_left(CAR_REMOTE_PWM);
        break;
    case 5:
        car_spin_right(CAR_REMOTE_PWM);
        break;
    default:
        remote_stop_test();
        car_stop();
        return;
    }

    g_test_step++;
}

static void remote_start_test(void)
{
    remote_stop_test();
    esp32_control_set_mode(ESP32_CONTROL_MODE_MANUAL);
    remote_update_mode_label();
    g_test_step = 0U;
    g_test_timer = lv_timer_create(remote_test_timer_cb, 1200, NULL);
    remote_test_timer_cb(g_test_timer);
}

static void remote_mode_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    esp32_control_mode_t mode = (esp32_control_mode_t)(uintptr_t)lv_event_get_user_data(e);

    if (code != LV_EVENT_CLICKED)
    {
        return;
    }

    remote_stop_test();
    car_stop();
    esp32_control_set_mode(mode);
    remote_update_mode_label();
}

static void remote_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    remote_cmd_t cmd = (remote_cmd_t)(uintptr_t)lv_event_get_user_data(e);

    if (cmd == REMOTE_CMD_TEST)
    {
        if (code == LV_EVENT_CLICKED)
        {
            remote_start_test();
        }
        return;
    }

    if (code == LV_EVENT_PRESSED)
    {
        remote_stop_test();
        esp32_control_set_mode(ESP32_CONTROL_MODE_MANUAL);
        remote_update_mode_label();
        remote_apply_command(cmd);
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        car_stop();
    }
}

static lv_obj_t *remote_create_button(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y, remote_cmd_t cmd)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_t *label = lv_label_create(btn);

    lv_obj_set_size(btn, 110, 60);
    lv_obj_set_pos(btn, x, y);
    lv_obj_add_event_cb(btn, remote_btn_event_cb, LV_EVENT_ALL, (void *)(uintptr_t)cmd);

    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

static lv_obj_t *remote_create_mode_button(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y,
                                           esp32_control_mode_t mode)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_t *label = lv_label_create(btn);

    lv_obj_set_size(btn, 110, 42);
    lv_obj_set_pos(btn, x, y);
    lv_obj_add_event_cb(btn, remote_mode_event_cb, LV_EVENT_ALL, (void *)(uintptr_t)mode);

    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

void car_remote_ui_init(void)
{
    lv_obj_t *screen;

    screen = remote_get_parent_screen();
    if (g_remote_panel != NULL)
    {
        if (lv_obj_get_parent(g_remote_panel) == screen)
        {
            remote_update_mode_label();
            return;
        }

        lv_obj_del(g_remote_panel);
        g_remote_panel = NULL;
        g_mode_label = NULL;
        g_manual_mode_btn = NULL;
        g_pickup_mode_btn = NULL;
    }

    g_remote_panel = lv_obj_create(screen);
    lv_obj_set_size(g_remote_panel, 360, 310);
    lv_obj_align(g_remote_panel, LV_ALIGN_TOP_RIGHT, -16, 16);
    lv_obj_set_style_bg_opa(g_remote_panel, LV_OPA_70, 0);
    lv_obj_set_style_pad_all(g_remote_panel, 10, 0);

    g_mode_label = lv_label_create(g_remote_panel);
    lv_obj_set_pos(g_mode_label, 10, 8);
    remote_update_mode_label();

    g_manual_mode_btn = remote_create_mode_button(g_remote_panel, "Manual", 125, 4, ESP32_CONTROL_MODE_MANUAL);
    g_pickup_mode_btn = remote_create_mode_button(g_remote_panel, "Pickup", 240, 4, ESP32_CONTROL_MODE_PICKUP);

    remote_create_button(g_remote_panel, "Forward", 125, 55, REMOTE_CMD_FORWARD);
    remote_create_button(g_remote_panel, "Backward", 125, 215, REMOTE_CMD_BACKWARD);
    remote_create_button(g_remote_panel, "Left", 10, 135, REMOTE_CMD_TURN_LEFT);
    remote_create_button(g_remote_panel, "Right", 240, 135, REMOTE_CMD_TURN_RIGHT);
    remote_create_button(g_remote_panel, "SpinL", 10, 215, REMOTE_CMD_SPIN_LEFT);
    remote_create_button(g_remote_panel, "SpinR", 240, 215, REMOTE_CMD_SPIN_RIGHT);
    remote_create_button(g_remote_panel, "Stop", 125, 135, REMOTE_CMD_STOP);
    remote_create_button(g_remote_panel, "Test", 240, 55, REMOTE_CMD_TEST);

    if (g_mode_sync_timer == NULL)
    {
        g_mode_sync_timer = lv_timer_create(remote_mode_sync_timer_cb, REMOTE_MODE_SYNC_PERIOD_MS, NULL);
    }

    remote_update_mode_label();
}
