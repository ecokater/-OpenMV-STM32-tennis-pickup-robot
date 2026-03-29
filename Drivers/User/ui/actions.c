#include "actions.h"
#include "vars.h"
#include <string.h>

static char ui_wifi_name[64];
static char ui_password[64];

const char *get_var_ui_wifi_name()
{
    return ui_wifi_name;
}

void set_var_ui_wifi_name(const char *value)
{
    if (value == NULL) {
        ui_wifi_name[0] = '\0';
        return;
    }
    strncpy(ui_wifi_name, value, sizeof(ui_wifi_name) - 1);
    ui_wifi_name[sizeof(ui_wifi_name) - 1] = '\0';
}

const char *get_var_ui_password()
{
    return ui_password;
}

void set_var_ui_password(const char *value)
{
    if (value == NULL) {
        ui_password[0] = '\0';
        return;
    }
    strncpy(ui_password, value, sizeof(ui_password) - 1);
    ui_password[sizeof(ui_password) - 1] = '\0';
}

void action_connect_btn_click(lv_event_t *e) {
    // TODO: Implement action connect_btn_click here
}
