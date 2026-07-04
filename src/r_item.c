#include "r_item.h"
#include "r_sync.h"
#include "e_puppet.h"
#include "e_item.h"
#include "ipc.h"
#include "input.h"
#include "physics.h"
#include "renderer.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"

// =============================================================================
//  ITEM ROLE
// =============================================================================
//
//  A standalone draggable item (ball, bat, ...) in its own transparent,
//  undecorated, topmost window. This file owns both halves of the item role:
//
//    PARENT SIDE (runs inside the puppet process):
//      SpawnItem / UpdateItems / CloseAllItems -- manage the item children.
//    CHILD SIDE (runs inside each `main.exe item ...` process):
//      RunItem -- window loop that syncs through SharedState.
//
//  Usage (set by the parent via argv): main.exe item <type> <x> <y> <parentPid> <slot>
// =============================================================================

// =============================================================================
//  PARENT SIDE
// =============================================================================

// Spawn a new item child of the requested kind. Returns false if all slots
// are busy or spawning failed.
bool SpawnItem(ItemRegistry *reg, SharedState *shared, unsigned long parentPid,
               ItemType type, int posX, int posY)
{
    for (int slot = 0; slot < MAX_ITEMS; slot++)
    {
        ItemSlotMeta *meta = &reg->items[slot];
        if (meta->active)
            continue;

        // Clear any stale state left by a previous child in this slot before the
        // new child can publish (we read `active` to know it's valid).
        shared->items[slot] = (ItemSlot){0};

        // Command line: main.exe item <type> <x> <y> <parentPid> <slot>
        char args[96];
        snprintf(args, sizeof args, "%d %d %d %lu %d",
                 (int)type, posX, posY, parentPid, slot);

        if (!IpcSpawnChild(&meta->proc, "item", args))
            return false;

        meta->type   = type;
        meta->active = true;
        return true;
    }
    return false;
}

// Resolve an item against a single circle, dispatching on the item's collision
// shape. Used on both sides of the IPC: the puppet passes a real limb as
// `circle` (limb authoritative, item particles are throwaway copies); the child
// passes a limb snapshot (item particles authoritative, snapshot discarded).
static void CollideItemWithCircle(Particle *itemParticles, ItemType type, Particle *circle)
{
    if (ItemShapeOf(type) == ITEM_SHAPE_CAPSULE)
        ResolveCapsuleCircleCollision(&itemParticles[0], &itemParticles[1], circle);
    else
        ResolveCirclesCollisions(&itemParticles[0], circle);
}

// Push every limb away from the item if they overlap. We collide LOCAL copies of
// the item's particles (owned by the child via shared memory) so only the limb
// side of the resolution is kept; the item's own correction is discarded and
// redone authoritatively by the child next frame.
static void CollideItemAgainstPuppet(const ItemSlot *slot, ItemType type, Puppet *pup)
{
    for (int j = 0; j < LIMB_COUNT; j++)
    {
        Particle copy[ITEM_MAX_PARTICLES];
        for (int i = 0; i < ITEM_MAX_PARTICLES; i++)
            copy[i] = slot->particles[i];
        CollideItemWithCircle(copy, type, &pup->limbs[j]);
    }
}

// Publish the puppet limbs, collide each live item against them, reap children
// that have exited.
void UpdateItems(ItemRegistry *reg, SharedState *shared, Puppet *pup)
{
    // -- Publish the puppet limbs for every child to read --
    shared->limbCount = LIMB_COUNT;
    for (int i = 0; i < LIMB_COUNT; i++)
        shared->limbs[i] = pup->limbs[i];

    bool puppetTouched = false;

    for (int slot = 0; slot < MAX_ITEMS; slot++)
    {
        ItemSlotMeta *meta = &reg->items[slot];
        if (!meta->active)
            continue;

        // Reap a child that has exited (closed its window).
        if (!IpcChildRunning(&meta->proc))
        {
            IpcKillChild(&meta->proc);
            meta->active               = false;
            shared->items[slot].active = false;
            continue;
        }

        // Collide the puppet against the item's latest published state.
        if (shared->items[slot].active)
        {
            CollideItemAgainstPuppet(&shared->items[slot], meta->type, pup);
            puppetTouched = true;
        }
    }

    // Limb displacements above invalidate the cached bounds used for window sizing.
    if (puppetTouched)
        pup->body.bounds = ComputeBoundBox(&pup->body);
}

// Terminate every live item child (called on shutdown).
void CloseAllItems(ItemRegistry *reg)
{
    for (int i = 0; i < MAX_ITEMS; i++)
    {
        if (reg->items[i].active)
            IpcKillChild(&reg->items[i].proc);
    }
}

// =============================================================================
//  CHILD SIDE
// =============================================================================

int RunItem(int argc, char **argv)
{
    // -- Parse argv: main.exe item <type> <x> <y> <parentPid> <slot> --
    ItemType      type      = (argc > 2) ? (ItemType)atoi(argv[2]) : ITEM_BALL;
    if (type < 0 || type >= ITEM_TYPE_COUNT) type = ITEM_BALL;
    float         startX    = (argc > 3) ? (float)atoi(argv[3]) : 300.0f;
    float         startY    = (argc > 4) ? (float)atoi(argv[4]) : 300.0f;
    unsigned long parentPid = (argc > 5) ? strtoul(argv[5], NULL, 10) : 0;
    int           slot      = (argc > 6) ? atoi(argv[6]) : 0;

    // -- Build the item body (pure data; no window needed yet) --
    Item item;
    CreateItem(&item, type, (Vector2){ startX, startY });

    // -- Setup: window sized to the item's bounding box (it follows bounds) --
    BoundBox b0 = item.body.bounds;
    InitOverlayWindow((int)b0.w + 2 * WINDOW_MARGIN, (int)b0.h + 2 * WINDOW_MARGIN);
    SetTargetFPS(TARGET_FPS);

    ShmRegion    shm;
    SharedState *shared;
    void        *parentH;
    if (!SyncChildAttach(parentPid, &shm, &shared, &parentH))
    {
        CloseWindow();
        return 1;
    }

    ScreenWidthHeight screen = GetScreenSize();

    // -- Main loop --
    while (!WindowShouldClose() && IpcProcessAlive(parentH))
    {
        // Input
        DragBody(&item.body);

        // Simulation: collide against a local copy of each limb (we must not
        // write into shared->limbs, which the puppet owns).
        ApplyPhysics(&item.body, screen.screenWidth, screen.screenHeight);
        for (int i = 0; i < shared->limbCount; i++)
        {
            Particle limb = shared->limbs[i];
            CollideItemWithCircle(item.particles, type, &limb);
        }

        // Collisions moved particles after ApplyPhysics computed the bounds; refresh.
        item.body.bounds = ComputeBoundBox(&item.body);

        // Publish our particles for the puppet to collide against.
        for (int i = 0; i < item.body.particleCount; i++)
            shared->items[slot].particles[i] = item.particles[i];
        shared->items[slot].active = true;

        // Size/position the window to the item's bounds, then draw.
        UpdateWindow(item.body.bounds);

        BeginDrawing();
            DrawItem(&item);
        EndDrawing();
    }

    // -- Teardown --
    shared->items[slot].active = false;
    IpcShmClose(&shm);
    CloseWindow();
    return 0;
}
