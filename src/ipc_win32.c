#include "ipc.h"

#include <windows.h>
#include <stdio.h>

// =============================================================================
//  WIN32 IPC BACKEND
// =============================================================================
//
//  Shared memory via a named file mapping backed by the page file
//  (CreateFileMapping with INVALID_HANDLE_VALUE). Children re-launch this same
//  .exe with a different subcommand and open the same named mapping. Parent
//  liveness is exposed to children through an OpenProcess handle.
// =============================================================================

// =============================================================================
//  SESSION NAMING
// =============================================================================

unsigned long IpcSelfPid(void)
{
    return (unsigned long)GetCurrentProcessId();
}

// "Local\" keeps the mapping private to the current session.
void IpcShmName(unsigned long pid, char *out, size_t cap)
{
    snprintf(out, cap, "Local\\desktop_puppet_%lu", pid);
}

// =============================================================================
//  SHARED MEMORY
// =============================================================================

bool IpcShmCreate(ShmRegion *r, const char *name, size_t size)
{
    *r = (ShmRegion){0};

    HANDLE h = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                  0, (DWORD)size, name);
    if (h == NULL)
        return false;

    void *view = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (view == NULL)
    {
        CloseHandle(h);
        return false;
    }

    ZeroMemory(view, size);
    r->handle = h;
    r->view   = view;
    r->size   = size;
    return true;
}

bool IpcShmOpen(ShmRegion *r, const char *name, size_t size)
{
    *r = (ShmRegion){0};

    HANDLE h = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);
    if (h == NULL)
        return false;

    void *view = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (view == NULL)
    {
        CloseHandle(h);
        return false;
    }

    r->handle = h;
    r->view   = view;
    r->size   = size;
    return true;
}

void IpcShmClose(ShmRegion *r)
{
    if (r->view   != NULL) UnmapViewOfFile(r->view);
    if (r->handle != NULL) CloseHandle((HANDLE)r->handle);
    *r = (ShmRegion){0};
}

// =============================================================================
//  CHILD PROCESS SPAWNING
// =============================================================================

// Build an absolute path to the running executable. Children are spawned by
// re-launching this same binary with a different subcommand.
static bool BuildSelfExePath(char *out, size_t outSize)
{
    DWORD n = GetModuleFileNameA(NULL, out, (DWORD)outSize);
    return n > 0 && n < outSize;
}

// Build a CreateProcess-ready command line:  "<self.exe>" <subcommand> <extraArgs>
static bool BuildSelfCommandLine(const char *subcommand,
                                 const char *extraArgs,
                                 char       *out,
                                 size_t      outSize)
{
    char selfPath[MAX_PATH];
    if (!BuildSelfExePath(selfPath, sizeof selfPath))
        return false;

    int written = snprintf(out, outSize, "\"%s\" %s %s",
                           selfPath,
                           (subcommand != NULL) ? subcommand : "",
                           (extraArgs  != NULL) ? extraArgs  : "");
    return written > 0 && (size_t)written < outSize;
}

// CreateProcess wrapper. The child inherits our standard handles; no pipes.
static HANDLE SpawnChild(char *cmdline)
{
    STARTUPINFOA si = {0};
    si.cb = sizeof si;

    PROCESS_INFORMATION pi = {0};
    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        return NULL;
    CloseHandle(pi.hThread);
    return pi.hProcess;
}

bool IpcSpawnChild(ChildProc *c, const char *subcommand, const char *extraArgs)
{
    *c = (ChildProc){0};

    char cmdline[MAX_PATH + 256];
    if (!BuildSelfCommandLine(subcommand, extraArgs, cmdline, sizeof cmdline))
        return false;

    HANDLE hProcess = SpawnChild(cmdline);
    if (hProcess == NULL)
        return false;

    c->hProcess = hProcess;
    c->running  = true;
    return true;
}

bool IpcChildRunning(ChildProc *c)
{
    if (!c->running)
        return false;
    if (WaitForSingleObject((HANDLE)c->hProcess, 0) == WAIT_OBJECT_0)
        c->running = false;
    return c->running;
}

void IpcKillChild(ChildProc *c)
{
    if (c->hProcess != NULL)
    {
        if (WaitForSingleObject((HANDLE)c->hProcess, 0) != WAIT_OBJECT_0)
            TerminateProcess((HANDLE)c->hProcess, 0);
        CloseHandle((HANDLE)c->hProcess);
    }
    *c = (ChildProc){0};
}

// =============================================================================
//  PARENT LIVENESS (child side)
// =============================================================================

void *IpcOpenProcess(unsigned long pid)
{
    return OpenProcess(SYNCHRONIZE, FALSE, (DWORD)pid);
}

bool IpcProcessAlive(void *handle)
{
    if (handle == NULL)
        return false;
    return WaitForSingleObject((HANDLE)handle, 0) == WAIT_TIMEOUT;
}
