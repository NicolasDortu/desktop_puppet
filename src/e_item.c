#include "e_item.h"
#include "e_puppet.h"
#include "config.h"
#include "input.h"
#include "physics.h"
#include "renderer.h"
#include "ipc.h"

#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"

// =============================================================================
//  ITEM ROLE
// =============================================================================
//
//  Standalone draggable ball in its own transparent, undecorated, topmost
//  window. The file has two halves:
//
//    PARENT SIDE (runs inside the puppet process):
//      - ItemRegistry / SpawnItem / CloseAllItems : lifecycle of child procs
//      - UpdateItems                              : per-frame IPC + collisions
//
//    CHILD SIDE (runs inside each spawned `main.exe item ...` process):
//      - InitItemBall / InitItemBody              : build the 1-particle body
//      - ParseLimbsMessage / SendBallState        : IPC plumbing
//      - DrawItem                                 : rendering
//      - RunItem                                  : entry point + main loop
//
//  Usage (set by the parent via argv): main.exe item <screenX> <screenY>
// =============================================================================

// =============================================================================
//  PARENT SIDE: REGISTRY
// =============================================================================

ItemRegistry CreateItemRegistry(void)
{
    ItemRegistry reg = {0};
    return reg;
}

// Spawn a new item child of the requested kind. Returns false if all slots
// are busy or spawning failed.
bool SpawnItem(ItemRegistry *reg, ItemType type, int posX, int posY)
{
    for (int i = 0; i < MAX_ITEMS; i++)
    {
        Item *it = &reg->items[i];
        if (it->proc.running)
            continue;

        // Command line: main.exe item <type> <x> <y>
        char args[64];
        snprintf(args, sizeof args, "%d %d %d", (int)type, posX, posY);

        if (!IpcSpawnBidi(&it->proc, "item", args))
            return false;

        it->type     = type;
        it->hasState = false;
        return true;
    }
    return false;
}

// Terminate every live item child (called on shutdown).
void CloseAllItems(ItemRegistry *reg)
{
    for (int i = 0; i < MAX_ITEMS; i++)
    {
        if (reg->items[i].proc.running)
            IpcCloseChild(&reg->items[i].proc);
    }
}

// =============================================================================
//  PARENT SIDE: PER-FRAME UPDATE
// =============================================================================

// Build the LIMBS snapshot message sent to every live item this frame.
// Format: "LIMBS <n> <x0> <y0> <r0> <x1> <y1> <r1> ...\n"
static int BuildLimbsMessage(const Puppet *pup, char *out, int outCap)
{
    int written = snprintf(out, outCap, "LIMBS %d", LIMB_COUNT);
    for (int i = 0; i < LIMB_COUNT && written > 0 && written < outCap; i++)
    {
        const PuppetLimb *L = &pup->limbs[i];
        written += snprintf(out + written, outCap - written,
                            " %.2f %.2f %.2f", L->pos.x, L->pos.y, L->radius);
    }
    if (written > 0 && written < outCap - 1)
        written += snprintf(out + written, outCap - written, "\n");
    return written;
}

// Push every limb away from `it->particle` if they overlap. Each side's local
// snapshot of the other particle is non-dragged (we don't sync isDragged over
// IPC), so ResolveCirclesCollisions splits the overlap 50/50; the foreign
// move is harmless because the next IPC message overwrites that snapshot.
static void CollideItemAgainstPuppet(Item *it, Puppet *pup)
{
    for (int j = 0; j < LIMB_COUNT; j++)
    {
        ResolveCirclesCollisions(&it->particle, &pup->limbs[j]);
    }
}

// Poll each item's pipe, run collisions, then broadcast the puppet state.
void UpdateItems(ItemRegistry *reg, Puppet *pup)
{
    char limbsMsg[IPC_LINE_CAP];
    BuildLimbsMessage(pup, limbsMsg, sizeof limbsMsg);

    bool puppetTouched = false;

    for (int s = 0; s < MAX_ITEMS; s++)
    {
        Item *it = &reg->items[s];
        if (!it->proc.running)
            continue;

        // -- Drain incoming STATE messages; keep the latest one --
        char line[IPC_LINE_CAP];
        while (IpcReadLine(&it->proc, line, sizeof line))
        {
            float x, y, ox, oy, r;
            if (sscanf(line, "STATE %f %f %f %f %f", &x, &y, &ox, &oy, &r) == 5)
            {
                it->particle.pos    = (Vector2){ x, y };
                it->particle.oldPos = (Vector2){ ox, oy };
                it->particle.radius = r;
                it->hasState        = true;
            }
        }

        if (!it->proc.running) // exited mid-drain
            continue;

        // -- Apply collision against the puppet --
        if (it->hasState)
        {
            CollideItemAgainstPuppet(it, pup);
            puppetTouched = true;
        }

        // -- Broadcast puppet state to this item --
        if (!IpcWriteLine(&it->proc, limbsMsg))
            IpcCloseChild(&it->proc);
    }

    // Limb displacements above invalidate the cached bounds used for window sizing.
    if (puppetTouched)
        pup->body.bounds = ComputeBoundBox(&pup->body);
}

// =============================================================================
//  CHILD SIDE: SPECS
// =============================================================================
//
// All item kinds currently share the same physics (a single Particle in a
// 1-particle Body); they differ only in size and visuals. Add a new ItemType
// in e_item.h and a matching entry below to introduce a new item kind.

typedef struct
{
    float radius;
    Color fillColor;
    Color outlineColor;
} ItemSpec;

static const ItemSpec ITEM_SPECS[ITEM_TYPE_COUNT] = {
    [ITEM_BALL] = { .radius = 30.0f, .fillColor = GRAY, .outlineColor = BLACK },
};

// =============================================================================
//  CHILD SIDE: INITIALIZATION
// =============================================================================

// Build the particle at `startPos` and wire a 1-particle Body around it so it
// reuses ApplyPhysics (verlet + walls) just like the puppet's limbs.
static void CreateItem(Particle *p, Body *body, ItemType type, Vector2 startPos)
{
    ItemSpec spec = ITEM_SPECS[type];

    *p = (Particle){
        .pos    = startPos,
        .oldPos = startPos,   // oldPos == pos => zero initial velocity
        .radius = spec.radius,
    };
    *body = (Body){
        .particles     = p,
        .particleCount = 1,
        .bones         = NULL,
        .boneCount     = 0,
        .cfg           = DEFAULT_PHYSICS_CONFIG,
    };
}

// =============================================================================
//  CHILD SIDE: IPC MESSAGES
// =============================================================================

// Parse a "LIMBS n x0 y0 r0 x1 y1 r1 ..." line into `limbs`.
// Writes the number of limbs actually parsed (clamped to `cap`) to `*outCount`.
static void ParseLimbsMessage(const char *line, Particle *limbs, int cap, int *outCount)
{
    int n, consumed;
    if (sscanf(line, "LIMBS %d%n", &n, &consumed) != 1 || n < 0)
        return;
    if (n > cap) n = cap;

    const char *p = line + consumed;
    for (int i = 0; i < n; i++)
    {
        float x, y, r;
        int   c2;
        if (sscanf(p, " %f %f %f%n", &x, &y, &r, &c2) != 3)
        {
            n = i; // truncated/corrupted message
            break;
        }
        limbs[i].pos    = (Vector2){ x, y };
        limbs[i].oldPos = (Vector2){ x, y }; // zero implicit velocity
        limbs[i].radius = r;
        p += c2;
    }
    *outCount = n;
}

// Send "STATE x y oldX oldY r\n" back to the parent. Returns false on broken pipe.
static bool SendItemState(const Particle *p)
{
    char out[128];
    snprintf(out, sizeof out, "STATE %.2f %.2f %.2f %.2f %.2f\n",
             p->pos.x, p->pos.y,
             p->oldPos.x, p->oldPos.y,
             p->radius);
    return IpcChildWriteLine(out);
}

// =============================================================================
//  CHILD SIDE: RENDERING
// =============================================================================

// The particle is always centered in its own (2R x 2R) window; the window
// itself is moved each frame so it appears at its world position on screen.
static void DrawItem(ItemType type)
{
    ItemSpec spec = ITEM_SPECS[type];
    ClearBackground(BLANK);
    DrawCircle     ((int)spec.radius, (int)spec.radius, spec.radius, spec.fillColor);
    DrawCircleLines((int)spec.radius, (int)spec.radius, spec.radius, spec.outlineColor);
}

// =============================================================================
//  CHILD SIDE: ITEM ROLE
// =============================================================================

int RunItem(int argc, char **argv)
{
    // -- Parse argv: main.exe item <type> <x> <y> --
    ItemType type   = (argc > 2) ? (ItemType)atoi(argv[2]) : ITEM_BALL;
    if (type < 0 || type >= ITEM_TYPE_COUNT) type = ITEM_BALL;
    float    startX = (argc > 3) ? (float)atoi(argv[3]) : 300.0f;
    float    startY = (argc > 4) ? (float)atoi(argv[4]) : 300.0f;

    ItemSpec spec    = ITEM_SPECS[type];
    int      winSize = (int)(2 * spec.radius);

    // -- Setup --
    SetTraceLogLevel(LOG_NONE); // raylib stdout logs would corrupt the IPC pipe
    InitOverlayWindow(winSize, winSize);
    SetTargetFPS(TARGET_FPS);
    IpcChildInit();

    ScreenWidthHeight screen = GetScreenSize();

    Particle particle;
    Body     body;
    CreateItem(&particle, &body, type, (Vector2){ startX, startY });

    // Latest puppet snapshot received over IPC. Zero-init so each limb's
    // isDragged stays false (we never sync drag state across processes).
    int      limbCount         = 0;
    Particle limbs[LIMB_COUNT] = {0};

    // -- Main loop --
    while (!WindowShouldClose())
    {
        // Input (drain every queued LIMBS message; keep only the latest one)
        char line[IPC_LINE_CAP];
        while (IpcChildReadLine(line, sizeof line))
            ParseLimbsMessage(line, limbs, LIMB_COUNT, &limbCount);
        DragBody(&body);

        // Simulation
        ApplyPhysics(&body, screen.screenWidth, screen.screenHeight);
        for (int i = 0; i < limbCount; i++)
            ResolveCirclesCollisions(&particle, &limbs[i]);

        // Move the OS window so the particle stays centered in it,
        // then ship our new state back to the parent.
        SetWindowPosition((int)(particle.pos.x - spec.radius),
                          (int)(particle.pos.y - spec.radius));
        if (!SendItemState(&particle))
            break; // broken pipe => parent died

        // Render
        BeginDrawing();
            DrawItem(type);
        EndDrawing();
    }

    // -- Teardown --
    CloseWindow();
    return 0;
}
