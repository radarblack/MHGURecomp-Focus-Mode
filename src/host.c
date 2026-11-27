#include "host.h"
#include "platform.h"
#include <string.h>
BcHost g_host;
static void *resolve(const char *name, bool required, bool *ok) {
    void *p = bc_find_symbol(name);
    if (!p && required) *ok = false;
    return p;
}
#define RESOLVE_FN(destination, symbol, required, okptr) do { \
    void *resolved_address = resolve((symbol), (required), (okptr)); \
    _Static_assert(sizeof(destination) == sizeof(resolved_address), "function and data pointers must match on this platform"); \
    memcpy(&(destination), &resolved_address, sizeof(destination)); \
} while (0)
bool bc_host_resolve(void) {
    bool ok = true;
    memset(&g_host, 0, sizeof g_host);
    RESOLVE_FN(g_host.fnhook_set, "fnhook_set", true, &ok);
    RESOLVE_FN(g_host.fnhook_get, "fnhook_get", true, &ok);
    RESOLVE_FN(g_host.fnhook_call_original, "fnhook_call_original", true, &ok);
    RESOLVE_FN(g_host.hle_register, "hle_register", true, &ok);
    RESOLVE_FN(g_host.dispatch_lookup, "dispatch_lookup", false, &ok);
    RESOLVE_FN(g_host.overlay_menu_open, "overlay_menu_open", false, &ok);
    RESOLVE_FN(g_host.chat_active, "chat_active", false, &ok);
    RESOLVE_FN(g_host.textinput_active, "textinput_active", false, &ok);
    return ok;
}
