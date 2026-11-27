#include "focus.h"
#include "config.h"
#include "guest.h"
#include "host.h"
#include "platform.h"
#include "focus_math.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define FM_PI 3.14159265358979323846f
static uint32_t g_hooked_address;
static uint32_t g_hook_id;
static int g_was_active;
static unsigned g_logged_bad_pose;

#ifdef _WIN32
#include <windows.h>
bool fm_guest_readable(uint32_t address, uint32_t length, bool write_access) {
    if (!address || !length) return false;
    const unsigned char *p = (const unsigned char *)(uintptr_t)(FM_GUEST_BASE + address);
    size_t done = 0;
    while (done < length) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(p + done, &mbi, sizeof mbi) || mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return false;
        if (write_access) {
            DWORD prot = mbi.Protect & 0xffu;
            if (prot != PAGE_READWRITE && prot != PAGE_WRITECOPY && prot != PAGE_EXECUTE_READWRITE && prot != PAGE_EXECUTE_WRITECOPY) return false;
        }
        size_t off = (size_t)((p + done) - (const unsigned char *)mbi.BaseAddress);
        done = off + mbi.RegionSize;
    }
    return true;
}
#else
#include <sys/mman.h>
bool fm_guest_readable(uint32_t address, uint32_t length, bool write_access) {
    (void)write_access;
    if (!address || !length) return false;
    const unsigned char *p = (const unsigned char *)(uintptr_t)(FM_GUEST_BASE + address);
    uintptr_t page = (uintptr_t)p & ~(uintptr_t)0xfffu;
    size_t span = ((uintptr_t)p + length - page + 0xfffu) & ~(size_t)0xfffu;
    return msync((void *)page, span, MS_ASYNC) == 0;
}
#endif

static bool is_main_camera(uint32_t eye, uint32_t target, uint32_t up) {
    const uint32_t eye_off = 0x40u, up_off = 0x50u, target_off = 0x60u;
    if (eye < 0x1000u + eye_off || (eye & 3u)) return false;
    uint32_t cam = eye - eye_off;
    if (target != cam + target_off || up != cam + up_off || !fm_guest_readable(cam, 0x70u, false)) return false;
    float far_z = fm_ldf(cam + 0x30u), near_z = fm_ldf(cam + 0x34u);
    float aspect = fm_ldf(cam + 0x38u), fov = fm_ldf(cam + 0x3cu);
    return isfinite(far_z) && isfinite(near_z) && isfinite(aspect) && isfinite(fov) &&
           near_z > 0.0f && far_z > near_z && far_z < 1.0e8f &&
           aspect >= 1.2f && aspect <= 4.0f && fov > 5.0f && fov < 150.0f;
}
static bool finite3(uint32_t p) {
    if (!fm_guest_readable(p, 12u, false)) return false;
    for (unsigned i = 0; i < 3; ++i) {
        float v = fm_ldf(p + 4u * i);
        if (!isfinite(v) || fabsf(v) > 1000000.0f) return false;
    }
    return true;
}
static bool menu_blocks_focus(void) {
    if (!g_config.pause_in_menus) return false;
    return (g_host.overlay_menu_open && g_host.overlay_menu_open()) ||
           (g_host.chat_active && g_host.chat_active()) ||
           (g_host.textinput_active && g_host.textinput_active());
}
static bool local_player_rotation(uint32_t *rotation) {
    if (!fm_guest_readable(FM_G_SGAMECONTROL, 0x80u, false) || !fm_guest_readable(FM_G_SPLAYER, 0x40u, false)) return false;
    uint32_t gc = fm_ld32(FM_G_SGAMECONTROL);
    uint32_t sp = fm_ld32(FM_G_SPLAYER);
    if (!gc || !sp || !fm_guest_readable(gc, 0x80u, false) || !fm_guest_readable(sp, 0x40u, false)) return false;
    uint32_t slot = fm_ld32(gc + FM_GC_SLOT);
    if (slot >= 4u) return false;
    uint32_t slot_addr = sp + FM_SP_PLAYERS + slot * 4u;
    if (!fm_guest_readable(slot_addr, 4u, false)) return false;
    uint32_t player = fm_ld32(slot_addr);
    if (!player || !fm_guest_readable(player + FM_PL_ZONE, 1u, false)) return false;
    uint8_t zone = fm_ld8(player + FM_PL_ZONE);
    if (g_config.only_in_quest && (zone == 0u || zone == 0xffu)) return false;
    *rotation = player + FM_PL_ROT;
    return fm_guest_readable(*rotation, 16u, true);
}

static void focus_view_builder(BcCpu *cpu) {
    bool active = g_config.enabled && bc_key_down(g_config.hold_key) && !menu_blocks_focus();
    if (active != (g_was_active != 0)) {
        g_was_active = active ? 1 : 0;
        if (g_mh.log) g_mh.log("[focusmode] Focus Mode %s\n", active ? "engaged" : "released");
    }
    if (active && cpu) {
        uint32_t eye = cpu->r[1], target = cpu->r[2], up = cpu->r[3], rotation = 0;
        if (is_main_camera(eye, target, up) && finite3(eye) && finite3(target) && local_player_rotation(&rotation)) {
            float dx = fm_ldf(target) - fm_ldf(eye);
            float dz = fm_ldf(target + 8u) - fm_ldf(eye + 8u);
            if (isfinite(dx) && isfinite(dz) && dx * dx + dz * dz > 0.0001f) {
                float yaw = atan2f(dx, dz) + g_config.yaw_offset_degrees * (FM_PI / 180.0f);
                float q[4];
                fm_quaternion_from_yaw(yaw, q);
                for (unsigned i = 0; i < 4; ++i) fm_stf(rotation + 4u * i, q[i]);
            }
        } else if (g_logged_bad_pose < 3u) {
            ++g_logged_bad_pose;
            if (g_mh.log) g_mh.log("[focusmode] player/camera data not ready or not writable (check skipped)\n");
        }
    }
    if (g_host.fnhook_call_original) g_host.fnhook_call_original(cpu, FM_FN_VIEW_BUILDER);
}

bool fm_focus_install(void) {
    if (!g_host.fnhook_set || !g_host.fnhook_get || !g_host.fnhook_call_original || !g_host.hle_register) {
        if (g_mh.log) g_mh.log("[focusmode] required host hook functions missing; module inactive\n");
        return false;
    }
    if (!fm_guest_readable(FM_FN_VIEW_BUILDER, 16u, false) || fm_ld32(FM_FN_VIEW_BUILDER) != FM_VIEW_BUILDER_PROLOGUE) {
        if (g_mh.log) g_mh.log("[focusmode] view-builder signature mismatch; refusing to hook this game build\n");
        return false;
    }
    uint32_t existing = g_host.fnhook_get(FM_FN_VIEW_BUILDER);
    if (existing != 0u) {
        if (g_mh.log) g_mh.log("[focusmode] view-builder already hooked (id %u); disable ModernCamera/other camera hooks first\n", existing);
        return false;
    }
    g_hook_id = g_host.hle_register("focusmode:viewbuilder", focus_view_builder);
    if (!g_hook_id || !g_host.fnhook_set(FM_FN_VIEW_BUILDER, g_hook_id)) {
        if (g_mh.log) g_mh.log("[focusmode] failed to install view-builder hook\n");
        return false;
    }
    if (g_host.fnhook_get(FM_FN_VIEW_BUILDER) != g_hook_id) {
        g_hook_id = 0;
        if (g_mh.log) g_mh.log("[focusmode] hook verification failed; hook rolled back\n");
        return false;
    }
    g_hooked_address = FM_FN_VIEW_BUILDER;
    if (g_mh.log) g_mh.log("[focusmode] camera-relative hunter facing hook installed (hold VK 0x%02X)\n", g_config.hold_key);
    return true;
}
void fm_focus_remove(void) {
    if (g_hooked_address && g_host.fnhook_set) {
        if (g_host.fnhook_get && g_host.fnhook_get(g_hooked_address) == g_hook_id)
            g_host.fnhook_set(g_hooked_address, 0u);
        g_hooked_address = 0;
        g_hook_id = 0;
        if (g_mh.log) g_mh.log("[focusmode] hook removed\n");
    }
}
