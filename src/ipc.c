#include "ipc.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
//  IPC OVERVIEW
// =============================================================================
//
//  Every child process is just THIS same .exe re-launched with a different
//  first argument (e.g. `main.exe item 300 400`). Parent and child talk
//  through anonymous Win32 pipes wired to the child's stdin and stdout:
//
//      parent  ---- (pipe to child stdin)  ---->  child
//      parent  <--- (pipe to child stdout) ----   child
//
//  Two patterns are supported:
//
//    1. ONE-SHOT (menu): the child only writes a single result to its stdout
//       and exits. The parent reads it via IpcPollMenu. No parent->child pipe.
//
//    2. BIDIRECTIONAL (items): a long-lived child that keeps reading
//       newline-terminated commands from its stdin and writing its state back
//       to its stdout. Parent calls IpcReadLine / IpcWriteLine, never blocks.
//
//  Reads are always NON-BLOCKING: PeekNamedPipe first, ReadFile only what is
//  already buffered, then look for the next '\n' in a small per-pipe line
//  buffer. The same low-level helpers (MakePipe / ReadAvailable / DrainLine)
//  are shared by the parent and child sides.
// =============================================================================

// =============================================================================
//  LOW-LEVEL HELPERS
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

// Create an anonymous pipe. CreatePipe with bInheritHandle=TRUE makes BOTH
// ends inheritable; we then clear the inherit flag on the end the PARENT
// keeps, so the spawned child only inherits the end it actually needs.
//
//   parentReadsThisPipe = true  -> *outRead  stays in parent (non-inheritable)
//                                  *outWrite is handed to the child
//   parentReadsThisPipe = false -> *outWrite stays in parent (non-inheritable)
//                                  *outRead  is handed to the child
static bool MakePipe(HANDLE *outRead, HANDLE *outWrite, bool parentReadsThisPipe)
{
    SECURITY_ATTRIBUTES sa = { sizeof sa, NULL, TRUE };
    if (!CreatePipe(outRead, outWrite, &sa, 0))
        return false;
    HANDLE keep = parentReadsThisPipe ? *outRead : *outWrite;
    SetHandleInformation(keep, HANDLE_FLAG_INHERIT, 0);
    return true;
}

// Non-blocking append: peek the pipe, then read at most what is already
// buffered into `buf[*len .. cap]`. If the buffer is full WITHOUT a newline
// we drop it to avoid deadlocking on a runaway peer.
static void ReadAvailable(HANDLE pipe, char *buf, int *len, int cap)
{
    DWORD avail = 0;
    if (!PeekNamedPipe(pipe, NULL, 0, NULL, &avail, NULL) || avail == 0)
        return;

    int space = cap - *len;
    if (space <= 0)
    {
        *len  = 0;     // discard: no '\n' was found in a full buffer
        space = cap;
    }

    DWORD toRead = (avail < (DWORD)space) ? avail : (DWORD)space;
    DWORD read   = 0;
    if (ReadFile(pipe, buf + *len, toRead, &read, NULL) && read > 0)
        *len += (int)read;
}

// If `buf` contains a newline, copy the line (without the '\n') into `out`,
// shift remaining bytes to the front of `buf`, and return true.
static bool DrainLine(char *buf, int *len, char *out, int outCap)
{
    for (int i = 0; i < *len; i++)
    {
        if (buf[i] != '\n') continue;

        int copy = (i > outCap - 1) ? outCap - 1 : i;
        memcpy(out, buf, copy);
        out[copy] = '\0';

        int remaining = *len - (i + 1);
        if (remaining > 0)
            memmove(buf, buf + i + 1, remaining);
        *len = remaining;
        return true;
    }
    return false;
}

// CreateProcess wrapper. `hStdin` may be NULL to inherit our own stdin.
// Returns the child process handle on success, NULL on failure.
static HANDLE SpawnChild(char *cmdline, HANDLE hStdin, HANDLE hStdout)
{
    STARTUPINFOA si = {0};
    si.cb         = sizeof si;
    si.dwFlags    = STARTF_USESTDHANDLES;
    si.hStdInput  = (hStdin != NULL) ? hStdin : GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hStdout;
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

    PROCESS_INFORMATION pi = {0};
    if (!CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        return NULL;
    CloseHandle(pi.hThread);
    return pi.hProcess;
}

// Zero-wait "has it exited?" poll.
static bool ProcessExited(HANDLE hProcess)
{
    return hProcess != NULL && WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0;
}

// Terminate-if-alive then close, in one place.
static void KillAndClose(HANDLE hProcess)
{
    if (hProcess == NULL) return;
    if (!ProcessExited(hProcess))
        TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
}

// =============================================================================
//  BIDIRECTIONAL CHILD  (parent side)
// =============================================================================
//
//  Used for both long-lived workers (items) AND one-shot children (the menu).
//  One-shot children simply never read from their stdin and write a single
//  line of result to their stdout before exiting; the parent reads it via
//  IpcReadLine, then ProcessExited flips `running` to false and the slot
//  is reaped via IpcCloseChild.

bool IpcSpawnBidi(ChildPipe *cp, const char *subcommand, const char *extraArgs)
{
    *cp = (ChildPipe){0};

    // Pipe 1: child stdout -> parent
    HANDLE childStdoutRead, childStdoutWrite;
    if (!MakePipe(&childStdoutRead, &childStdoutWrite, /*parentReadsThisPipe=*/true))
        return false;

    // Pipe 2: parent -> child stdin
    HANDLE childStdinRead, childStdinWrite;
    if (!MakePipe(&childStdinRead, &childStdinWrite, /*parentReadsThisPipe=*/false))
    {
        CloseHandle(childStdoutRead); CloseHandle(childStdoutWrite);
        return false;
    }

    char cmdline[MAX_PATH + 256];
    if (!BuildSelfCommandLine(subcommand, extraArgs, cmdline, sizeof cmdline))
    {
        CloseHandle(childStdoutRead); CloseHandle(childStdoutWrite);
        CloseHandle(childStdinRead);  CloseHandle(childStdinWrite);
        return false;
    }

    HANDLE hProcess = SpawnChild(cmdline, childStdinRead, childStdoutWrite);
    // Parent never uses the child-side ends. Closing them now means each pipe
    // automatically EOFs / breaks when the child exits.
    CloseHandle(childStdoutWrite);
    CloseHandle(childStdinRead);
    if (hProcess == NULL)
    {
        CloseHandle(childStdoutRead);
        CloseHandle(childStdinWrite);
        return false;
    }

    cp->hProcess    = hProcess;
    cp->hReadStdout = childStdoutRead;
    cp->hWriteStdin = childStdinWrite;
    cp->running     = true;
    return true;
}

bool IpcReadLine(ChildPipe *cp, char *line, int lineCap)
{
    if (!cp->running || lineCap < 2)
        return false;

    // 1. Pull whatever bytes the child has written into our line buffer.
    // 2. Try to extract one complete line from the buffer.
    ReadAvailable(cp->hReadStdout, cp->rxBuf, &cp->rxLen, (int)sizeof cp->rxBuf);
    if (DrainLine(cp->rxBuf, &cp->rxLen, line, lineCap))
        return true;

    // No complete line AND the child has exited: nothing more is coming.
    if (ProcessExited(cp->hProcess))
        cp->running = false;
    return false;
}

bool IpcWriteLine(ChildPipe *cp, const char *line)
{
    if (!cp->running)
        return false;

    DWORD n       = (DWORD)strlen(line);
    DWORD written = 0;
    if (!WriteFile(cp->hWriteStdin, line, n, &written, NULL) || written != n)
    {
        cp->running = false; // broken pipe -> child is gone
        return false;
    }
    return true;
}

void IpcCloseChild(ChildPipe *cp)
{
    KillAndClose(cp->hProcess);
    if (cp->hReadStdout != NULL) CloseHandle(cp->hReadStdout);
    if (cp->hWriteStdin != NULL) CloseHandle(cp->hWriteStdin);
    *cp = (ChildPipe){0};
}

// =============================================================================
//  CHILD-SIDE HELPERS  (used from inside a spawned child process)
// =============================================================================
//
// The child sees its own stdin/stdout as plain HANDLEs. It runs the exact
// same Peek+Read+DrainLine dance as the parent's IpcReadLine, just against
// the global child buffer instead of a ChildPipe-owned one.

static HANDLE g_childIn  = NULL;
static HANDLE g_childOut = NULL;
static char   g_childBuf[IPC_LINE_CAP];
static int    g_childLen = 0;

void IpcChildInit(void)
{
    g_childIn  = GetStdHandle(STD_INPUT_HANDLE);
    g_childOut = GetStdHandle(STD_OUTPUT_HANDLE);
    g_childLen = 0;
}

bool IpcChildReadLine(char *line, int lineCap)
{
    if (g_childIn == NULL || lineCap < 2)
        return false;

    ReadAvailable(g_childIn, g_childBuf, &g_childLen, (int)sizeof g_childBuf);
    return DrainLine(g_childBuf, &g_childLen, line, lineCap);
}

bool IpcChildWriteLine(const char *line)
{
    if (g_childOut == NULL)
        return false;

    DWORD n       = (DWORD)strlen(line);
    DWORD written = 0;
    return WriteFile(g_childOut, line, n, &written, NULL) && written == n;
}
