#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;

static void event_handler_cb_main_lvgl_password(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_FOCUSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, -1, 2, e);
    }
}

static void event_handler_cb_main_lvgl_connect_btn(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        action_connect_btn_click(e);
    }
}

static void event_handler_cb_main_obj0(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        action_connect_btn_click(e);
    }
}

void create_screen_show_view() {
    void *flowState = getFlowState(0, 0);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.show_view = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    {
        lv_obj_t *parent_obj = obj;
        {
            // video_panel
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.video_panel = obj;
            lv_obj_set_pos(obj, 80, 0);
            lv_obj_set_size(obj, 640, 480);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // video_pic
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.video_pic = obj;
                    lv_obj_set_pos(obj, -22, -22);
                    lv_obj_set_size(obj, 160, 120);
                }
            }
        }
    }
    
    tick_screen_show_view();
}

void tick_screen_show_view() {
    void *flowState = getFlowState(0, 0);
    (void)flowState;
}

void create_screen_main() {
    void *flowState = getFlowState(0, 1);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    {
        lv_obj_t *parent_obj = obj;
        {
            // LVGL_ssid
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            objects.lvgl_ssid = obj;
            lv_obj_set_pos(obj, 43, 43);
            lv_obj_set_size(obj, 371, 42);
            lv_textarea_set_max_length(obj, 128);
            lv_textarea_set_text(obj, "wifi_name");
            lv_textarea_set_one_line(obj, true);
            lv_textarea_set_password_mode(obj, false);
        }
        {
            // LVGL_password
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            objects.lvgl_password = obj;
            lv_obj_set_pos(obj, 43, 118);
            lv_obj_set_size(obj, 371, 42);
            lv_textarea_set_max_length(obj, 128);
            lv_textarea_set_text(obj, "password");
            lv_textarea_set_one_line(obj, true);
            lv_textarea_set_password_mode(obj, false);
            lv_obj_add_event_cb(obj, event_handler_cb_main_lvgl_password, LV_EVENT_ALL, flowState);
        }
        {
            lv_obj_t *obj = lv_keyboard_create(parent_obj);
            lv_obj_set_pos(obj, 0, 212);
            lv_obj_set_size(obj, 800, 268);
            lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // LVGL_connect_btn
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.lvgl_connect_btn = obj;
            lv_obj_set_pos(obj, 456, 64);
            lv_obj_set_size(obj, 112, 75);
            lv_obj_add_event_cb(obj, event_handler_cb_main_lvgl_connect_btn, LV_EVENT_ALL, flowState);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj0 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_event_cb(obj, event_handler_cb_main_obj0, LV_EVENT_ALL, flowState);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Button");
                }
            }
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
    void *flowState = getFlowState(0, 1);
    (void)flowState;
}


static const char *screen_names[] = { "Show_view", "Main" };
static const char *object_names[] = { "show_view", "main", "lvgl_password", "lvgl_connect_btn", "obj0", "video_panel", "video_pic", "lvgl_ssid" };


typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_show_view,
    tick_screen_main,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    eez_flow_init_screen_names(screen_names, sizeof(screen_names) / sizeof(const char *));
    eez_flow_init_object_names(object_names, sizeof(object_names) / sizeof(const char *));
    
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_show_view();
    create_screen_main();
}
