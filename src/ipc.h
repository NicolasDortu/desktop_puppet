#ifndef IPC_H
#define IPC_H

#include <stdbool.h>
#include <stddef.h>

// =============================================================================
//  DECLARATIONS
// =============================================================================

// Bidirectional child process: parent owns one read-end (child stdout) and
// one write-end (child stdin), plus a small internal line buffer used by
// IpcReadLine. One-shot children (e.g. the menu) simply ignore the stdin
// pipe and let the parent read their single result via IpcReadLine before
// the child exits. Handles are stored as `void *` so this header does not
// have to pull in <windows.h> (which conflicts with raylib symbol names).
#define IPC_LINE_CAP 512

typedef struct
{
    void *hProcess;    // Win32 HANDLE to the child process
    void *hReadStdout; // read end of the child's stdout pipe
    void *hWriteStdin; // write end of the child's stdin pipe (may stay NULL for one-shot children)
    bool  running;     // false once the child has exited or the pipe broke
    char  rxBuf[IPC_LINE_CAP];
    int   rxLen;       // bytes currently buffered awaiting a newline
} ChildPipe;

// =============================================================================
//  PARENT-SIDE FUNCTIONS (puppet process)
// =============================================================================

// All children are launched as `main.exe <subcommand> [extraArgs]`. The
// dispatcher in main.c then routes the child into the matching RunX entry
// point. `extraArgs` is appended verbatim to the command line.
bool IpcSpawnBidi(ChildPipe *cp, const char *subcommand, const char *extraArgs);
bool IpcReadLine(ChildPipe *cp, char *line, int lineCap); // non-blocking, true if a full line was returned
bool IpcWriteLine(ChildPipe *cp, const char *line);       // false on broken pipe; marks running=false
void IpcCloseChild(ChildPipe *cp);

// =============================================================================
//  CHILD-SIDE FUNCTIONS (used inside item.exe / similar workers)
// =============================================================================

void IpcChildInit(void);
bool IpcChildReadLine(char *line, int lineCap);
bool IpcChildWriteLine(const char *line);

#endif
