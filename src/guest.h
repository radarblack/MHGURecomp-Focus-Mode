#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define FM_GUEST_BASE 0x10000000000ull
#define FM_GPTR(a) ((void *)(uintptr_t)(FM_GUEST_BASE + (uint32_t)(a)))
#define FM_FN_VIEW_BUILDER 0x9c5e4cu
#define FM_VIEW_BUILDER_PROLOGUE 0xe92d4070u
#define FM_G_SGAMECONTROL 0x1a8ca48u
#define FM_GC_SLOT 0x30u
#define FM_G_SPLAYER 0x1a85b58u
#define FM_SP_PLAYERS 0x1cu
#define FM_PL_ZONE 0x1054u
#define FM_PL_ROT 0x50u

bool fm_guest_readable(uint32_t address, uint32_t length, bool write_access);
static inline uint8_t fm_ld8(uint32_t a) { return *(const uint8_t *)FM_GPTR(a); }
static inline uint32_t fm_ld32(uint32_t a) { uint32_t v; memcpy(&v, FM_GPTR(a), sizeof v); return v; }
static inline float fm_ldf(uint32_t a) { float v; memcpy(&v, FM_GPTR(a), sizeof v); return v; }
static inline void fm_stf(uint32_t a, float v) { memcpy(FM_GPTR(a), &v, sizeof v); }
