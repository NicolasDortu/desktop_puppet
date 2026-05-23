#include "e_item.h"
#include "e_puppet.h"
#include "config.h"
#include "input.h"
#include "renderer.h"
#include "ipc.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"

// =============================================================================
//  ITEM ROLE
// =============================================================================
//
//  Standalone draggable ball in its own transparent, undecorated, topmost
//  window. Every frame it:
//    * reads the parent's LIMBS broadcast from stdin (non-blocking),
//    * runs its own verlet physics + dragging + screen-edge bounces,
//    * resolves collisions against the received puppet limbs,
//    * sends its updated state to the parent via stdout.
//
//  Usage (set by the parent via argv): main.exe item <screenX> <screenY>
// =============================================================================

// =============================================================================
//  REGISTRY
// =============================================================================

ItemRegistry CreateItemRegistry(void)
{
    ItemRegistry reg = {0};
    return reg;
}

// Spawn a new item child. Returns false if all slots are busy or spawning failed.
bool SpawnItem(ItemRegistry *reg, int posX, int posY)
{
    for (int i = 0; i < MAX_ITEMS; i++)
    {
        Item *it = &reg->items[i];
        if (it->proc.running)
            continue;

        char args[64];
        snprintf(args, sizeof args, "%d %d", posX, posY);

        if (!IpcSpawnBidi(&it->proc, "item", args))
            return false;

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
//  PER-FRAME UPDATE
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

// Push every limb away from `it->ball` if they overlap. Each side's local
// snapshot of the other particle is non-dragged (we don't sync isDragged over
// IPC), so ResolveCirclesCollisions splits the overlap 50/50; the foreign
// move is harmless because the next IPC message overwrites that snapshot.
static void CollideItemAgainstPuppet(Item *it, Puppet *pup)
{
    for (int j = 0; j < LIMB_COUNT; j++)
    {
        ResolveCirclesCollisions(&it->ball, &pup->limbs[j]);
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

        // -- Drain incoming BALL messages; keep the latest one --
        char line[IPC_LINE_CAP];
        while (IpcReadLine(&it->proc, line, sizeof line))
        {
            float x, y, ox, oy, r;
            if (sscanf(line, "BALL %f %f %f %f %f", &x, &y, &ox, &oy, &r) == 5)
            {
                it->ball.pos    = (Vector2){ x, y };
                it->ball.oldPos = (Vector2){ ox, oy };
                it->ball.radius = r;
                it->hasState    = true;
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

// Local physics tuning. Ball reuses ApplyPhysics with a 1-particle Body so a
// thrown ball feels consistent with the puppet's limbs.
#define ITEM_RADIUS  30.0f
#define MAX_LIMBS    16

int RunItem(int argc, char **argv)
{
    float startX = (argc > 2) ? (float)atoi(argv[2]) : 300.0f;
    float startY = (argc > 3) ? (float)atoi(argv[3]) : 300.0f;

    // raylib's stdout logs would corrupt the IPC pipe; silence them entirely.
    SetTraceLogLevel(LOG_NONE);

    int winSize = (int)(2 * ITEM_RADIUS);
    InitOverlayWindow(winSize, winSize);
    SetTargetFPS(TARGET_FPS);

    IpcChildInit();

    ScreenWidthHeight screen = GetScreenSize();

    // -- Ball as a 1-particle Body so it reuses ApplyPhysics (verlet + walls) --
    Particle ball = {
        .pos    = { startX, startY },
        .oldPos = { startX, startY },
        .radius = ITEM_RADIUS,
    };
    Body body = {
        .particles       = &ball,
        .particleCount   = 1,
        .bones           = NULL,
        .boneCount       = 0,
        .cfg = (PhysicsConfig){
            .gravity   = DEFAULT_GRAVITY,
            .friction  = DEFAULT_FRICTION,
            .bounce    = DEFAULT_BOUNCE,
            .minBounce = DEFAULT_MIN_BOUNCE,
            .stiffness = 0.0f,
        },
    };

    // -- Latest puppet snapshot received from parent --
    // Zero-init so each limb's isDragged stays false (we never sync drag state).
    int      limbCount = 0;
    Particle limbs[MAX_LIMBS] = {0};

    while (!WindowShouldClose())
    {
        // -- 1. Drain incoming messages; keep the most recent LIMBS line --
        char line[IPC_LINE_CAP];
        while (IpcChildReadLine(line, sizeof line))
        {
            int n, consumed;
            if (sscanf(line, "LIMBS %d%n", &n, &consumed) == 1 && n >= 0 && n <= MAX_LIMBS)
            {
                limbCount = n;
                const char *p = line + consumed;
                for (int i = 0; i < n; i++)
                {
                    float xi, yi, ri;
                    int   c2;
                    if (sscanf(p, " %f %f %f%n", &xi, &yi, &ri, &c2) == 3)
                    {
                        limbs[i].pos    = (Vector2){ xi, yi };
                        limbs[i].oldPos = (Vector2){ xi, yi };
                        limbs[i].radius = ri;
                        p += c2;
                    }
                    else
                    {
                        limbCount = i;
                        break;
                    }
                }
            }
        }

        // -- 2. Dragging input (generic body-drag against our 1-particle body) --
        DragBody(&body);

        // -- 3. Verlet integration + wall bounces --
        ApplyPhysics(&body, screen.screenWidth, screen.screenHeight);

        // -- 4. Collide against received puppet limbs. Neither side's snapshot
        //       is flagged dragged, so the overlap is split 50/50; the parent
        //       does the symmetric half against its real limbs.
        for (int i = 0; i < limbCount; i++)
            ResolveCirclesCollisions(&ball, &limbs[i]);

        // -- 5. Move the OS window so the ball stays centered in it --
        SetWindowPosition((int)(ball.pos.x - ITEM_RADIUS), (int)(ball.pos.y - ITEM_RADIUS));

        // -- 6. Send our state to the parent. Broken pipe => parent died. --
        char out[128];
        snprintf(out, sizeof out, "BALL %.2f %.2f %.2f %.2f %.2f\n",
                 ball.pos.x, ball.pos.y, ball.oldPos.x, ball.oldPos.y, ball.radius);
        if (!IpcChildWriteLine(out))
            break;

        // -- 7. Render --
        BeginDrawing();
            ClearBackground(BLANK);
            DrawCircle((int)ITEM_RADIUS, (int)ITEM_RADIUS, ITEM_RADIUS, GRAY);
            DrawCircleLines((int)ITEM_RADIUS, (int)ITEM_RADIUS, ITEM_RADIUS, BLACK);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
