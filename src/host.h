#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "engine_module_api.h"

typedef struct BcCpu {
    uint32_t r[16];
    uint8_t reserved[192];
    float f[64];
} BcCpu;
_Static_assert(sizeof(BcCpu) == 512, "BcCpu layout must match MHGU Recomp");
_Static_assert(offsetof(BcCpu, f) == 256, "BcCpu VFP register offset mismatch");

typedef void (*BcHostFn)(BcCpu *cpu);
typedef struct BcHost {
    bool (*fnhook_set)(uint32_t address, uint32_t detour_id);
    uint32_t (*fnhook_get)(uint32_t address);
    void (*fnhook_call_original)(BcCpu *cpu, uint32_t address);
    uint32_t (*hle_register)(const char *name, BcHostFn fn);
    void *(*dispatch_lookup)(uint32_t address);
    bool (*overlay_menu_open)(void);
    bool (*chat_active)(void);
    bool (*textinput_active)(void);
} BcHost;
extern BcHost g_host;
extern MhguHost g_mh;
bool bc_host_resolve(void);
