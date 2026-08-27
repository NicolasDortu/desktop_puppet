#include "r_item.h"
#include "r_sync.h"
#include "e_puppet.h"
#include "e_item.h"
#include "ipc.h"
#include "input.h"
#include "physics.h"
#include "renderer.h"
#include "config.h"

#include <math.h>
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

        // Stagger spawns by slot: items spawned back-to-back would otherwise
        // start at identical coordinates, and the collision resolvers bail on
        // coincident centers, leaving them perfectly stacked forever.
        posX += slot * 15;

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

// Resolve MY particles against a peer item's snapshot, dispatching on both
// collision shapes. The caller passes a throwaway copy of the peer: only my
// side of each correction is kept, the peer applies its own share in its own
// process (same split as the puppet <-> item collision).
static void CollideItemWithItem(Particle *mine, ItemType myType,
                                Particle *theirs, ItemType theirType)
{
    bool meCapsule  = ItemShapeOf(myType)    == ITEM_SHAPE_CAPSULE;
    bool othCapsule = ItemShapeOf(theirType) == ITEM_SHAPE_CAPSULE;

    if (meCapsule && othCapsule)
        ResolveCapsulesCollision(&mine[0], &mine[1], &theirs[0], &theirs[1]);
    else if (meCapsule)
        ResolveCapsuleCircleCollision(&mine[0], &mine[1], &theirs[0]);
    else if (othCapsule)
        ResolveCapsuleCircleCollision(&theirs[0], &theirs[1], &mine[0]);
    else
        ResolveCirclesCollisions(&mine[0], &theirs[0]);
}

// Push every limb away from the item if they overlap. We collide LOCAL copies of
// the item's particles (owned by the child via shared memory) so only the limb
// side of the resolution is kept; the item's own correction is discarded and
// redone authoritatively by the child next frame.
// Returns the biggest displacement an item inflicted on a limb (0 = no contact).
static float CollideItemAgainstPuppet(const ItemSlot *slot, ItemType type, Puppet *pup)
{
    // Item velocity (particle average), for transferring momentum on contact.
    // A DRAGGED item transfers nothing: shoving the puppet with a held ball
    // is free, so rolling/throwing it is the rewarded move.
    int   n       = (ItemShapeOf(type) == ITEM_SHAPE_CAPSULE) ? 2 : 1;
    bool  dragged = false;
    float ivx = 0.0f, ivy = 0.0f;
    for (int i = 0; i < n; i++)
    {
        dragged = dragged || slot->particles[i].isDragged;
        ivx += slot->particles[i].pos.x - slot->particles[i].oldPos.x;
        ivy += slot->particles[i].pos.y - slot->particles[i].oldPos.y;
    }
    ivx /= n;
    ivy /= n;

    float punch   = ItemPunchOf(type);
    float maxPush = 0.0f;
    for (int j = 0; j < LIMB_COUNT; j++)
    {
        Particle copy[ITEM_MAX_PARTICLES];
        for (int i = 0; i < ITEM_MAX_PARTICLES; i++)
            copy[i] = slot->particles[i];

        Vector2 before = pup->limbs[j].pos;
        CollideItemWithCircle(copy, type, &pup->limbs[j]);

        float dx   = pup->limbs[j].pos.x - before.x;
        float dy   = pup->limbs[j].pos.y - before.y;
        float push = sqrtf(dx * dx + dy * dy);
        if (push > maxPush)
            maxPush = push;

        if (push > 0.0f && !dragged)
        {
            // The positional resolve only shares the overlap, which barely
            // moves the puppet (a rolling ball just nudged it). Real impact =
            // amplified overlap share + the item's own speed INTO the limb,
            // both scaled by punch and injected as Verlet velocity. A resting
            // item has ~zero speed, so this adds nothing to settled contact.
            float nx     = dx / push;
            float ny     = dy / push;
            float vAlong = ivx * nx + ivy * ny;      // item speed toward the limb
            if (vAlong < 0.0f) vAlong = 0.0f;        // never a pull

            float kick = push * (punch - 1.0f) + vAlong * punch;
            pup->limbs[j].oldPos.x -= nx * kick;
            pup->limbs[j].oldPos.y -= ny * kick;
        }
    }
    return maxPush;
}

// Publish the puppet limbs, collide each live item against them, reap children
// that have exited. Returns the biggest limb displacement an item caused.
float UpdateItems(ItemRegistry *reg, SharedState *shared, Puppet *pup)
{
    // -- Publish the puppet limbs for every child to read --
    shared->limbCount = LIMB_COUNT;
    for (int i = 0; i < LIMB_COUNT; i++)
        shared->limbs[i] = pup->limbs[i];

    bool  puppetTouched = false;
    float maxPush       = 0.0f;

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
            float push = CollideItemAgainstPuppet(&shared->items[slot], meta->type, pup);
            if (push > maxPush)
                maxPush = push;
            puppetTouched = true;
        }
    }

    // Limb displacements above invalidate the cached bounds used for window sizing.
    if (puppetTouched)
        pup->body.bounds = ComputeBoundBox(&pup->body);

    return maxPush;
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

// One flight step of the guided missile: accelerate toward the cursor (capped
// at cruise speed), then check for impact. Returns true when the missile hit
// a screen border or a puppet limb and must detonate. No gravity, no drag, no
// bouncing — it is powered flight until something stops it.
static bool UpdateMissile(Item *item, const SharedState *shared, BoundBox screen)
{
    Particle *p  = &item->particles[0];
    float     vx = p->pos.x - p->oldPos.x;
    float     vy = p->pos.y - p->oldPos.y;

    // -- Steer toward the cursor --
    float cx, cy;
    IpcCursorPos(&cx, &cy);
    float dx = cx - p->pos.x;
    float dy = cy - p->pos.y;
    float d  = sqrtf(dx * dx + dy * dy);
    if (d > 1.0f)
    {
        vx += dx / d * MISSILE_ACCEL;
        vy += dy / d * MISSILE_ACCEL;
    }
    float speed = sqrtf(vx * vx + vy * vy);
    if (speed > MISSILE_SPEED)
    {
        vx *= MISSILE_SPEED / speed;
        vy *= MISSILE_SPEED / speed;
    }

    p->oldPos = p->pos;
    p->pos.x += vx;
    p->pos.y += vy;

    // -- Impact: screen border --
    if (p->pos.x - p->radius < screen.x || p->pos.x + p->radius > screen.x + screen.w ||
        p->pos.y - p->radius < screen.y || p->pos.y + p->radius > screen.y + screen.h)
        return true;

    // -- Impact: puppet limb --
    for (int i = 0; i < shared->limbCount; i++)
    {
        float lx = shared->limbs[i].pos.x - p->pos.x;
        float ly = shared->limbs[i].pos.y - p->pos.y;
        float r  = shared->limbs[i].radius + p->radius;
        if (lx * lx + ly * ly < r * r)
            return true;
    }
    return false;
}

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

    // -- Setup: window sized to the item's window box (it follows it) --
    BoundBox b0 = ItemWindowBounds(&item);
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

    BoundBox screen = GetScreenArea();

    unsigned int lastBlast = shared->blast.seq; // ignore blasts from before we spawned

    // Items are temporary: a bomb burns its short fuse, a missile its fuel,
    // everything else despawns after ITEM_LIFETIME (the window just closes;
    // the parent reaps).
    double dieAt = GetTime() + ((type == ITEM_BOMB)    ? BOMB_FUSE_TIME
                              : (type == ITEM_MISSILE) ? MISSILE_FUEL_TIME
                                                       : ITEM_LIFETIME);

    // -- Main loop --
    while (!WindowShouldClose() && IpcProcessAlive(parentH))
    {
        if (GetTime() >= dieAt)
            break; // lifetime over: bombs/missiles detonate below, others just exit

        if (type == ITEM_MISSILE)
        {
            // Guided flight: no dragging, no gravity, no peer collisions —
            // it flies at the cursor and detonates on the first contact.
            bool impact = UpdateMissile(&item, shared, screen);
            item.body.bounds = ComputeBoundBox(&item.body);
            if (impact)
                break; // detonation below
        }
        else
        {
            // Input
            DragBody(&item.body);

            // A bomb went off somewhere: kick our own particles (every process
            // applies the blast to the particles it owns).
            if (shared->blast.seq != lastBlast)
            {
                lastBlast = shared->blast.seq;
                ApplyBlastToBody(&item.body, shared->blast.pos, shared->blast.radius, shared->blast.power);
            }

            // Simulation: collide against a local copy of each limb (we must not
            // write into shared->limbs, which the puppet owns).
            ApplyPhysics(&item.body, screen);
            for (int i = 0; i < shared->limbCount; i++)
            {
                Particle limb = shared->limbs[i];
                CollideItemWithCircle(item.particles, type, &limb);
            }

            // Collide against every other live item's published snapshot (local
            // copy; each peer applies its own share from its own process).
            for (int s = 0; s < MAX_ITEMS; s++)
            {
                if (s == slot || !shared->items[s].active)
                    continue;
                ItemSlot peer = shared->items[s];
                CollideItemWithItem(item.particles, type, peer.particles, peer.type);
            }

            // Collisions moved particles after ApplyPhysics computed the bounds; refresh.
            item.body.bounds = ComputeBoundBox(&item.body);
        }

        // Publish our particles for the puppet and the other items to collide against.
        for (int i = 0; i < item.body.particleCount; i++)
            shared->items[slot].particles[i] = item.particles[i];
        shared->items[slot].type   = type;
        shared->items[slot].active = true;

        // Size/position the window to the item's window box, then draw.
        UpdateWindow(ItemWindowBounds(&item));

        BeginDrawing();
            DrawItem(&item);
        EndDrawing();
    }

    // -- Bomb/missile: detonate (fuse or fuel ran out, or the missile hit
    // something — not on manual close) --
    if ((type == ITEM_BOMB || type == ITEM_MISSILE) &&
        !WindowShouldClose() && IpcProcessAlive(parentH))
    {
        Vector2 c = item.particles[0].pos;

        shared->items[slot].active = false; // the bomb itself is gone

        // Publish the blast: parameters first, seq bump last (readers key on seq).
        shared->blast.pos    = c;
        shared->blast.radius = BOMB_BLAST_RADIUS;
        shared->blast.power  = BOMB_BLAST_POWER;
        shared->blast.seq++;

        // Grow the window to cover the blast area and play the visual.
        float R = BOMB_BLAST_RADIUS;
        UpdateWindow((BoundBox){ c.x - R, c.y - R, 2 * R, 2 * R });
        Vector2 local = { R + WINDOW_MARGIN, R + WINDOW_MARGIN };

        double t0 = GetTime();
        while (!WindowShouldClose() && IpcProcessAlive(parentH))
        {
            float progress = (float)((GetTime() - t0) / BOMB_BOOM_TIME);
            if (progress >= 1.0f)
                break;
            BeginDrawing();
                DrawExplosion(local, R, progress);
            EndDrawing();
        }
    }

    // -- Teardown --
    shared->items[slot].active = false;
    IpcShmClose(&shm);
    CloseWindow();
    return 0;
}
