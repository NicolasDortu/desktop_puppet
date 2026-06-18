#ifndef IPC_H
#define IPC_H

#include <stdbool.h>
#include <stddef.h>

// =============================================================================
//  IPC OVERVIEW
// =============================================================================
//
//  Every child process is just THIS same .exe re-launched with a different
//  first argument (e.g. `main.exe item 300 400`). Parent and children share
//  state through a single named shared-memory region instead of pipes:
//
//      - The parent (puppet) creates the region and maps it.
//      - Each child opens the same named region and reads/writes its slot.
//
//  There is no message protocol: the shared struct IS the synced state. The
//  parent publishes its data; children publish theirs; both sides just read
//  the latest values. Torn reads are harmless because every field is rewritten
//  the next frame.
//
//  This header stays platform-agnostic (no <windows.h>, which conflicts with
//  raylib symbol names) and knows nothing about the payload struct: callers
//  pass a byte size and cast `view` to their own type. The Win32 backend lives
//  in ipc_win32.c and the POSIX backend in ipc_posix.c.
// =============================================================================

// Handles are stored as `void *` so this header does not pull in <windows.h>.

// A mapped named shared-memory region. `view` points at the shared bytes.
typedef struct
{
    void  *handle; // Win32 file-mapping HANDLE / POSIX fd
    void  *view;   // MapViewOfFile / mmap pointer (the shared payload)
    size_t size;   // size of the mapped region in bytes
} ShmRegion;

// A spawned child process. The parent keeps the handle to poll liveness.
typedef struct
{
    void *hProcess; // Win32 HANDLE / POSIX pid wrapper
    bool  running;  // false once the child has exited or was killed
} ChildProc;

// =============================================================================
//  SESSION NAMING
// =============================================================================
//
// The whole session keys off the parent's PID: the parent passes its PID to
// each child via argv, and both sides derive the same shared-region name from
// it. The name format (e.g. the Win32 "Local\" prefix) is platform-specific, so
// it lives in the backend rather than in app code.

unsigned long IpcSelfPid(void);                                  // this process's PID
void          IpcShmName(unsigned long pid, char *out, size_t cap); // session region name for a PID

// =============================================================================
//  SHARED MEMORY
// =============================================================================

// Parent: create (or replace) a named region of `size` bytes, zero-initialized.
bool IpcShmCreate(ShmRegion *r, const char *name, size_t size);
// Child: open an existing named region of `size` bytes.
bool IpcShmOpen(ShmRegion *r, const char *name, size_t size);
void IpcShmClose(ShmRegion *r);

// =============================================================================
//  CHILD PROCESSES (parent side)
// =============================================================================

// All children are launched as `main.exe <subcommand> [extraArgs]`. The
// dispatcher in main.c then routes the child into the matching RunX entry
// point. `extraArgs` is appended verbatim to the command line.
bool IpcSpawnChild(ChildProc *c, const char *subcommand, const char *extraArgs);
bool IpcChildRunning(ChildProc *c); // false once the child has exited
void IpcKillChild(ChildProc *c);    // terminate if alive, then close the handle

// =============================================================================
//  PARENT LIVENESS (child side)
// =============================================================================
//
// A child opens a handle to its parent once (by PID, passed via argv) and polls
// it each frame; when the parent is gone the child exits. This replaces the old
// "broken pipe => parent died" signal.

void *IpcOpenProcess(unsigned long pid); // NULL on failure
bool  IpcProcessAlive(void *handle);      // true while the process is running

#endif
