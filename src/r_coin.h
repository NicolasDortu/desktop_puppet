#ifndef R_COIN_H
#define R_COIN_H

#include "ipc.h"

// =============================================================================
//  COIN ROLE
// =============================================================================
//
//  A tiny short-lived window showing coin.png, spawned by the puppet whenever
//  it earns a coin. It floats up for about a second and exits on its own.
//  Fire-and-forget: the parent keeps a small ring of process handles and reaps
//  finished ones opportunistically on the next spawn.

#define COIN_RING_SIZE 8

typedef struct
{
    ChildProc procs[COIN_RING_SIZE];
    int       next; // ring cursor
} CoinPopups;

// Parent side: pop a coin at screen position (x, y) (coin center).
void SpawnCoinPopup(CoinPopups *ring, unsigned long parentPid, int x, int y);
// Parent side: close every handle (called on shutdown).
void CloseCoinPopups(CoinPopups *ring);

// Child entry point: main.exe coin <x> <y> <parentPid>
int  RunCoin(int argc, char **argv);

#endif
