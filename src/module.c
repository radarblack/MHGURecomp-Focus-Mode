#include "engine_module_api.h"
#include "config.h"
#include "focus.h"
#include "guest.h"
#include "host.h"
#include "platform.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#ifndef FM_VERSION
#define FM_VERSION "0.1.0"
#endif

MhguHost g_mh;
static BcThread g_worker;
static atomic_bool g_shutdown_requested = ATOMIC_VAR_INIT(false);

static bool game_ready(void) {
    if (g_host.dispatch_lookup) return g_host.dispatch_lookup(FM_FN_VIEW_BUILDER) != NULL;
    return fm_guest_readable(FM_FN_VIEW_BUILDER, 16u, false) &&
           fm_ld32(FM_FN_VIEW_BUILDER) == FM_VIEW_BUILDER_PROLOGUE;
}
static void worker(void *unused) {
    (void)unused;
    if (!bc_host_resolve()) {
        if (g_mh.log) g_mh.log("[focusmode] required host symbols unavailable; game startup unaffected\n");
        return;
    }
    if (g_mh.log) g_mh.log("[focusmode] waiting for MHGU 1.4.0 game code\n");
    uint64_t start = bc_now_ms();
    while (!atomic_load_explicit(&g_shutdown_requested, memory_order_acquire) && bc_now_ms() - start < 60000u) {
        if (game_ready()) {
            if (atomic_load_explicit(&g_shutdown_requested, memory_order_acquire)) return;
            if (fm_focus_install()) {
                if (g_mh.log) g_mh.log("[focusmode] ready; hold the configured key to align hunter facing with camera\n");
            } else {
                if (g_mh.log) g_mh.log("[focusmode] unavailable on this session; game startup unaffected\n");
            }
            return;
        }
        bc_sleep_ms(100);
    }
    if (!atomic_load_explicit(&g_shutdown_requested, memory_order_acquire)) {
        if (g_mh.log) g_mh.log("[focusmode] timed out waiting for game code; module inactive\n");
    }
}
static void module_shutdown(void) {
    atomic_store_explicit(&g_shutdown_requested, true, memory_order_release);
    bc_thread_join(&g_worker, 5000u);
    fm_focus_remove();
}

MHGU_ENGINE_MODULE_EXPORT bool mhgu_engine_module_entry(const MhguHost *host, MhguEngineModule *out) {
    if (!host || !out || !host->log) return false;
    if (host->api < 1u || host->api > MHGU_ENGINE_MODULE_API) {
        host->log("[focusmode] module API mismatch (built %u, host %u); not loaded\n",
                  (unsigned)MHGU_ENGINE_MODULE_API, (unsigned)host->api);
        return false;
    }
    memset(out, 0, sizeof *out);
    g_mh = *host;
    atomic_store_explicit(&g_shutdown_requested, false, memory_order_release);
    bool cfg = fm_config_load(host->module_dir);
    host->log("[focusmode] Focus Mode v%s (API %u, port %s); config %s; enabled=%u hold-key=0x%02X\n",
              FM_VERSION, (unsigned)host->api, host->port_version ? host->port_version : "?",
              cfg ? "loaded" : "defaults", g_config.enabled ? 1u : 0u, g_config.hold_key);
    out->name = "Focus Mode";
    out->shutdown = module_shutdown;
    if (!bc_thread_start(&g_worker, worker, NULL)) {
        host->log("[focusmode] worker thread could not start; module inactive\n");
    }
    return true;
}
MHGU_ENGINE_MODULE_EXPORT const uint32_t mhgu_engine_module_api = MHGU_ENGINE_MODULE_API;
