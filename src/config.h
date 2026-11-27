#pragma once
#include <stdbool.h>

typedef struct FocusConfig {
    bool enabled;
    bool only_in_quest;
    bool pause_in_menus;
    unsigned hold_key;
    float yaw_offset_degrees;
} FocusConfig;
extern FocusConfig g_config;
bool fm_config_load(const char *module_dir);
