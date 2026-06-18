#if !defined(_WIN32)

#include "ipc.h"

#include <stdio.h>
#include <unistd.h>

// =============================================================================
//  POSIX IPC BACKEND  (TODO)
// =============================================================================
//
//  Sibling of ipc_win32.c for Linux/macOS. Intended implementation:
//    - IpcShmCreate / IpcShmOpen : shm_open + ftruncate + mmap
//                                  (https://www.youtube.com/watch?v=rPV6b8BUwxM)
//    - IpcSpawnChild             : fork + execv of /proc/self/exe (or argv[0])
//    - IpcChildRunning           : waitpid(pid, ..., WNOHANG)
//    - IpcOpenProcess/Alive      : store the pid; kill(pid, 0) to probe liveness
//
//  Stubs below let the build link on non-Windows platforms. The Makefile's
//  `wildcard src/*.c` compiles every backend; the platform guards make the
//  non-matching one compile to nothing.
// =============================================================================

unsigned long IpcSelfPid(void)
{
    return (unsigned long)getpid();
}

// Leading "/" is required for shm_open names.
void IpcShmName(unsigned long pid, char *out, size_t cap)
{
    snprintf(out, cap, "/desktop_puppet_%lu", pid);
}

bool IpcShmCreate(ShmRegion *r, const char *name, size_t size)
{
    (void)name; (void)size;
    *r = (ShmRegion){0};
    return false;
}

bool IpcShmOpen(ShmRegion *r, const char *name, size_t size)
{
    (void)name; (void)size;
    *r = (ShmRegion){0};
    return false;
}

void IpcShmClose(ShmRegion *r)
{
    *r = (ShmRegion){0};
}

bool IpcSpawnChild(ChildProc *c, const char *subcommand, const char *extraArgs)
{
    (void)subcommand; (void)extraArgs;
    *c = (ChildProc){0};
    return false;
}

bool IpcChildRunning(ChildProc *c)
{
    return c->running;
}

void IpcKillChild(ChildProc *c)
{
    *c = (ChildProc){0};
}

void *IpcOpenProcess(unsigned long pid)
{
    (void)pid;
    return NULL;
}

bool IpcProcessAlive(void *handle)
{
    (void)handle;
    return false;
}

#endif // !_WIN32
