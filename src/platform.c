#define _POSIX_C_SOURCE 200809L
#include "platform.h"
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
void bc_sleep_ms(unsigned ms) { Sleep(ms); }
uint64_t bc_now_ms(void) { return GetTickCount64(); }
void *bc_find_symbol(const char *name) {
    HMODULE exe = GetModuleHandleA(NULL);
    return exe ? (void *)GetProcAddress(exe, name) : NULL;
}
typedef struct ThreadArgs { void (*fn)(void *); void *arg; } ThreadArgs;
static DWORD WINAPI thread_entry(LPVOID p) {
    ThreadArgs *a = (ThreadArgs *)p;
    a->fn(a->arg);
    free(a);
    return 0;
}
bool bc_thread_start(BcThread *t, void (*fn)(void *), void *arg) {
    ThreadArgs *a = (ThreadArgs *)malloc(sizeof *a);
    if (!a) return false;
    a->fn = fn; a->arg = arg;
    t->handle = CreateThread(NULL, 0, thread_entry, a, 0, NULL);
    t->started = t->handle != NULL;
    if (!t->started) free(a);
    return t->started;
}
void bc_thread_join(BcThread *t, unsigned timeout_ms) {
    if (!t->started) return;
    (void)timeout_ms;
    WaitForSingleObject(t->handle, INFINITE);
    CloseHandle(t->handle); t->started = false;
}
bool bc_key_down(unsigned vk) { return (GetAsyncKeyState((int)vk) & 0x8000) != 0; }
#else
#include <dlfcn.h>
#include <time.h>
#include <unistd.h>
void bc_sleep_ms(unsigned ms) { struct timespec req = { (time_t)(ms / 1000u), (long)(ms % 1000u) * 1000000L }; nanosleep(&req, NULL); }
uint64_t bc_now_ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000u + (uint64_t)t.tv_nsec / 1000000u; }
void *bc_find_symbol(const char *name) { return dlsym(RTLD_DEFAULT, name); }
typedef struct ThreadArgs { void (*fn)(void *); void *arg; } ThreadArgs;
static void *thread_entry(void *p) { ThreadArgs *a = (ThreadArgs *)p; a->fn(a->arg); free(a); return NULL; }
bool bc_thread_start(BcThread *t, void (*fn)(void *), void *arg) {
    ThreadArgs *a = (ThreadArgs *)malloc(sizeof *a); if (!a) return false;
    a->fn = fn; a->arg = arg; t->started = pthread_create(&t->handle, NULL, thread_entry, a) == 0;
    if (!t->started) free(a);
    return t->started;
}
void bc_thread_join(BcThread *t, unsigned timeout_ms) { (void)timeout_ms; if (t->started) { pthread_join(t->handle, NULL); t->started = false; } }
bool bc_key_down(unsigned vk) { (void)vk; return false; }
#endif
