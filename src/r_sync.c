#include "r_sync.h"
#include "ipc.h"

// =============================================================================
//  SHARED-MEMORY REGION SETUP
// =============================================================================
//
//  The whole session keys off the puppet's PID: the puppet creates a region
//  named after its own PID and passes that PID to each child, which derives the
//  same name to map the same region. These two helpers are the only places that
//  touch the IPC layer for the shared region.
// =============================================================================

bool SyncHostCreate(ShmRegion *shm, SharedState **shared, unsigned long *selfPid)
{
    unsigned long pid = IpcSelfPid();

    char name[64];
    IpcShmName(pid, name, sizeof name);
    if (!IpcShmCreate(shm, name, sizeof(SharedState)))
        return false;

    *shared  = (SharedState *)shm->view;
    *selfPid = pid;
    return true;
}

bool SyncChildAttach(unsigned long parentPid, ShmRegion *shm,
                     SharedState **shared, void **parentH)
{
    char name[64];
    IpcShmName(parentPid, name, sizeof name);
    if (!IpcShmOpen(shm, name, sizeof(SharedState)))
        return false;

    *shared  = (SharedState *)shm->view;
    *parentH = IpcOpenProcess(parentPid);
    return true;
}
