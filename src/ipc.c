#include "ipc.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
//  HELPERS
// =============================================================================

// Build an absolute path to the running executable. Children are always
// spawned by re-launching this same binary with a different subcommand.
static bool BuildSelfExePath(char *out, size_t outSize)
{
    DWORD n = GetModuleFileNameA(NULL, out, (DWORD)outSize);
    return n > 0 && n < outSize;
}

// Build a CreateProcess-ready command line of the form:
//     "<self.exe>" <subcommand> <extraArgs>
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

// =============================================================================
//  IPC IMPLEMENTATION
// =============================================================================

bool IpcSpawnMenu(MenuProcess *mp, int posX, int posY)
{
    mp->hProcess = NULL;
    mp->hRead    = NULL;
    mp->running  = false;

    // -- Create the anonymous pipe used by the child's stdout --
    SECURITY_ATTRIBUTES sa = {sizeof sa, NULL, TRUE}; // inheritable handles
    HANDLE              hRead  = NULL;
    HANDLE              hWrite = NULL;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
        return false;
    // The parent's read end must NOT leak into the child.
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    // -- Build the command line: re-launch this exe with the "menu" subcommand --
    char extra[64];
    snprintf(extra, sizeof extra, "%d %d", posX, posY);

    char cmdline[MAX_PATH + 128];
    if (!BuildSelfCommandLine("menu", extra, cmdline, sizeof cmdline))
    {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return false;
    }

    // -- Spawn the child with its stdout wired to our pipe --
    STARTUPINFOA si = {0};
    si.cb           = sizeof si;
    si.dwFlags      = STARTF_USESTDHANDLES;
    si.hStdOutput   = hWrite;
    si.hStdError    = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdInput    = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi = {0};
    BOOL ok = CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

    // We never write into the pipe; close our copy so the child's exit closes it.
    CloseHandle(hWrite);

    if (!ok)
    {
        CloseHandle(hRead);
        return false;
    }
    CloseHandle(pi.hThread);

    mp->hProcess = pi.hProcess;
    mp->hRead    = hRead;
    mp->running  = true;
    return true;
}

bool IpcPollMenu(MenuProcess *mp, int *outId)
{
    if (!mp->running)
        return false;

    bool gotId = false;

    // -- Non-blocking read of whatever the child has written so far --
    DWORD avail = 0;
    if (PeekNamedPipe(mp->hRead, NULL, 0, NULL, &avail, NULL) && avail > 0)
    {
        char  buf[64];
        DWORD toRead = (avail < sizeof buf - 1) ? avail : (DWORD)(sizeof buf - 1);
        DWORD read   = 0;
        if (ReadFile(mp->hRead, buf, toRead, &read, NULL) && read > 0)
        {
            buf[read] = '\0';
            *outId    = atoi(buf);
            gotId     = true;
        }
    }

    // -- Reap the child if it has exited (either after a click or by user close) --
    if (WaitForSingleObject(mp->hProcess, 0) == WAIT_OBJECT_0)
        IpcCloseMenu(mp);

    return gotId;
}

void IpcCloseMenu(MenuProcess *mp)
{
    if (mp->hProcess != NULL)
    {
        // If still alive (user cancelled from parent side), force it to quit.
        if (WaitForSingleObject(mp->hProcess, 0) != WAIT_OBJECT_0)
            TerminateProcess(mp->hProcess, 0);
        CloseHandle(mp->hProcess);
    }
    if (mp->hRead != NULL)
        CloseHandle(mp->hRead);

    mp->hProcess = NULL;
    mp->hRead    = NULL;
    mp->running  = false;
}

// =============================================================================
//  BIDIRECTIONAL CHILD (parent side)
// =============================================================================

// Try to extract a newline-terminated line from cp->rxBuf into `line`.
// Shifts any remaining bytes to the start of the buffer.
static bool DrainLine(ChildPipe *cp, char *line, int lineCap)
{
    for (int i = 0; i < cp->rxLen; i++)
    {
        if (cp->rxBuf[i] == '\n')
        {
            int copy = i;
            if (copy > lineCap - 1)
                copy = lineCap - 1;
            memcpy(line, cp->rxBuf, copy);
            line[copy] = '\0';

            int remaining = cp->rxLen - (i + 1);
            if (remaining > 0)
                memmove(cp->rxBuf, cp->rxBuf + i + 1, remaining);
            cp->rxLen = remaining;
            return true;
        }
    }
    return false;
}

bool IpcSpawnBidi(ChildPipe *cp, const char *subcommand, const char *extraArgs)
{
    cp->hProcess    = NULL;
    cp->hReadStdout = NULL;
    cp->hWriteStdin = NULL;
    cp->running     = false;
    cp->rxLen       = 0;

    SECURITY_ATTRIBUTES sa = {sizeof sa, NULL, TRUE};

    // -- Pipe 1: child stdout -> parent --
    HANDLE childStdoutRead  = NULL;
    HANDLE childStdoutWrite = NULL;
    if (!CreatePipe(&childStdoutRead, &childStdoutWrite, &sa, 0))
        return false;
    SetHandleInformation(childStdoutRead, HANDLE_FLAG_INHERIT, 0);

    // -- Pipe 2: parent -> child stdin --
    HANDLE childStdinRead  = NULL;
    HANDLE childStdinWrite = NULL;
    if (!CreatePipe(&childStdinRead, &childStdinWrite, &sa, 0))
    {
        CloseHandle(childStdoutRead);
        CloseHandle(childStdoutWrite);
        return false;
    }
    SetHandleInformation(childStdinWrite, HANDLE_FLAG_INHERIT, 0);

    // -- Re-launch this same exe with the requested subcommand --
    char cmdline[MAX_PATH + 256];
    if (!BuildSelfCommandLine(subcommand, extraArgs, cmdline, sizeof cmdline))
    {
        CloseHandle(childStdoutRead);  CloseHandle(childStdoutWrite);
        CloseHandle(childStdinRead);   CloseHandle(childStdinWrite);
        return false;
    }

    STARTUPINFOA si = {0};
    si.cb         = sizeof si;
    si.dwFlags    = STARTF_USESTDHANDLES;
    si.hStdInput  = childStdinRead;
    si.hStdOutput = childStdoutWrite;
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

    PROCESS_INFORMATION pi = {0};
    BOOL ok = CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

    // Parent doesn't use the child-side ends of either pipe.
    CloseHandle(childStdoutWrite);
    CloseHandle(childStdinRead);

    if (!ok)
    {
        CloseHandle(childStdoutRead);
        CloseHandle(childStdinWrite);
        return false;
    }
    CloseHandle(pi.hThread);

    cp->hProcess    = pi.hProcess;
    cp->hReadStdout = childStdoutRead;
    cp->hWriteStdin = childStdinWrite;
    cp->running     = true;
    return true;
}

bool IpcReadLine(ChildPipe *cp, char *line, int lineCap)
{
    if (!cp->running || lineCap < 2)
        return false;

    // Reap exited child first so callers see `running=false` promptly.
    if (WaitForSingleObject(cp->hProcess, 0) == WAIT_OBJECT_0)
    {
        // Still try one final drain of buffered + pipe bytes below.
    }

    DWORD avail = 0;
    if (PeekNamedPipe(cp->hReadStdout, NULL, 0, NULL, &avail, NULL) && avail > 0)
    {
        int space = (int)sizeof cp->rxBuf - cp->rxLen;
        if (space > 0)
        {
            DWORD toRead = (avail < (DWORD)space) ? avail : (DWORD)space;
            DWORD read   = 0;
            if (ReadFile(cp->hReadStdout, cp->rxBuf + cp->rxLen, toRead, &read, NULL) && read > 0)
                cp->rxLen += (int)read;
        }
        else
        {
            // Buffer full without a newline: discard to avoid deadlock.
            cp->rxLen = 0;
        }
    }

    if (DrainLine(cp, line, lineCap))
        return true;

    // Mark dead only after we've drained everything we could.
    if (WaitForSingleObject(cp->hProcess, 0) == WAIT_OBJECT_0)
    {
        cp->running = false;
    }
    return false;
}

bool IpcWriteLine(ChildPipe *cp, const char *line)
{
    if (!cp->running)
        return false;

    size_t n = strlen(line);
    DWORD  written = 0;
    if (!WriteFile(cp->hWriteStdin, line, (DWORD)n, &written, NULL) || written != n)
    {
        cp->running = false;
        return false;
    }
    return true;
}

void IpcCloseChild(ChildPipe *cp)
{
    if (cp->hProcess != NULL)
    {
        if (WaitForSingleObject(cp->hProcess, 0) != WAIT_OBJECT_0)
            TerminateProcess(cp->hProcess, 0);
        CloseHandle(cp->hProcess);
    }
    if (cp->hReadStdout != NULL)
        CloseHandle(cp->hReadStdout);
    if (cp->hWriteStdin != NULL)
        CloseHandle(cp->hWriteStdin);

    cp->hProcess    = NULL;
    cp->hReadStdout = NULL;
    cp->hWriteStdin = NULL;
    cp->running     = false;
    cp->rxLen       = 0;
}

// =============================================================================
//  CHILD-SIDE HELPERS
// =============================================================================

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

static bool DrainChildLine(char *line, int lineCap)
{
    for (int i = 0; i < g_childLen; i++)
    {
        if (g_childBuf[i] == '\n')
        {
            int copy = i;
            if (copy > lineCap - 1)
                copy = lineCap - 1;
            memcpy(line, g_childBuf, copy);
            line[copy] = '\0';

            int remaining = g_childLen - (i + 1);
            if (remaining > 0)
                memmove(g_childBuf, g_childBuf + i + 1, remaining);
            g_childLen = remaining;
            return true;
        }
    }
    return false;
}

bool IpcChildReadLine(char *line, int lineCap)
{
    if (g_childIn == NULL || lineCap < 2)
        return false;

    DWORD avail = 0;
    if (PeekNamedPipe(g_childIn, NULL, 0, NULL, &avail, NULL) && avail > 0)
    {
        int space = (int)sizeof g_childBuf - g_childLen;
        if (space > 0)
        {
            DWORD toRead = (avail < (DWORD)space) ? avail : (DWORD)space;
            DWORD read   = 0;
            if (ReadFile(g_childIn, g_childBuf + g_childLen, toRead, &read, NULL) && read > 0)
                g_childLen += (int)read;
        }
        else
        {
            g_childLen = 0;
        }
    }

    return DrainChildLine(line, lineCap);
}

bool IpcChildWriteLine(const char *line)
{
    if (g_childOut == NULL)
        return false;

    size_t n = strlen(line);
    DWORD  written = 0;
    if (!WriteFile(g_childOut, line, (DWORD)n, &written, NULL) || written != n)
        return false;
    return true;
}
