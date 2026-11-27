#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FocusConfig g_config = { true, true, true, 0x02u, 0.0f };
static char *trim(char *s) {
    while (isspace((unsigned char)*s)) ++s;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = '\0';
    return s;
}
static void set_value(const char *key, const char *value) {
    if (!strcmp(key, "Enabled")) g_config.enabled = atoi(value) != 0;
    else if (!strcmp(key, "OnlyInQuest")) g_config.only_in_quest = atoi(value) != 0;
    else if (!strcmp(key, "PauseInMenus")) g_config.pause_in_menus = atoi(value) != 0;
    else if (!strcmp(key, "HoldKey")) {
        unsigned long v = strtoul(value, NULL, 0);
        if (v <= 0xffu) g_config.hold_key = (unsigned)v;
    } else if (!strcmp(key, "YawOffsetDegrees")) {
        float v = (float)atof(value);
        if (v >= -180.0f && v <= 180.0f) g_config.yaw_offset_degrees = v;
    }
}
bool fm_config_load(const char *module_dir) {
    char path[1024];
#ifdef _WIN32
    snprintf(path, sizeof path, "%s\\FocusMode.ini", module_dir ? module_dir : ".");
#else
    snprintf(path, sizeof path, "%s/FocusMode.ini", module_dir ? module_dir : ".");
#endif
    FILE *f = fopen(path, "r");
    if (!f) return false;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char *s = trim(line);
        if (!*s || *s == ';' || *s == '#' || *s == '[') continue;
        char *eq = strchr(s, '='); if (!eq) continue;
        *eq++ = '\0'; set_value(trim(s), trim(eq));
    }
    fclose(f);
    return true;
}
