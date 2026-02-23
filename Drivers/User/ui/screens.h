#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *show_view;
    lv_obj_t *main;
    lv_obj_t *lvgl_password;
    lv_obj_t *lvgl_connect_btn;
    lv_obj_t *obj0;
    lv_obj_t *video_panel;
    lv_obj_t *video_pic;
    lv_obj_t *lvgl_ssid;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_SHOW_VIEW = 1,
    SCREEN_ID_MAIN = 2,
};

void create_screen_show_view();
void tick_screen_show_view();

void create_screen_main();
void tick_screen_main();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/