#include "item.h"
#include "puppet.h"
#include "ipc.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

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

// Push every limb away from `it` if they overlap. Each side resolves half the
// overlap; combined with the item's own half-resolution this yields a normal
// non-overlapping collision response.
static void CollideItemAgainstPuppet(const Item *it, Puppet *pup)
{
    for (int j = 0; j < LIMB_COUNT; j++)
    {
        if (pup->draggedLimb == j)
            continue;

        PuppetLimb *L  = &pup->limbs[j];
        float       dx = L->pos.x - it->x;
        float       dy = L->pos.y - it->y;
        float       d2 = dx * dx + dy * dy;
        float       minD = L->radius + it->r;

        if (d2 >= minD * minD || d2 < 1e-6f)
            continue;

        float d    = sqrtf(d2);
        float push = (minD - d) * 0.5f;
        L->pos.x += (dx / d) * push;
        L->pos.y += (dy / d) * push;
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
                it->x = x; it->y = y;
                it->oldX = ox; it->oldY = oy;
                it->r = r;
                it->hasState = true;
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
        pup->bounds = ComputePuppetBounds(pup);
}
