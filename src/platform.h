#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
typedef HANDLE BcThreadHandle;
#else
#include <pthread.h>
typedef pthread_t BcThreadHandle;
#endif

typedef struct BcThread {
    BcThreadHandle handle;
    bool started;
} BcThread;
void bc_sleep_ms(unsigned ms);
uint64_t bc_now_ms(void);
void *bc_find_symbol(const char *name);
bool bc_thread_start(BcThread *thread, void (*fn)(void *), void *arg);
void bc_thread_join(BcThread *thread, unsigned timeout_ms);
bool bc_key_down(unsigned virtual_key);
