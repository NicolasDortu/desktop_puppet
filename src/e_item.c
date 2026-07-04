#include "e_item.h"
#include "config.h"
#include "physics.h"
#include "renderer.h"

#include <math.h>
#include <stddef.h>

#include "raylib.h"

// =============================================================================
//  ITEM ENTITY
// =============================================================================
//
//  An item is a small physics Body (particles + bones), exactly like the
//  puppet. This file only defines the entity (its specs, how to build it, how
//  to draw it); the process/window/IPC orchestration lives in r_item.c.
//
//    ITEM_BALL : 1 particle, no bones                 -> circle
//    ITEM_BAT  : 2 particles + 1 rigid bone (segment) -> capsule, drawn as a bar
// =============================================================================

// =============================================================================
//  SPECS
// =============================================================================

typedef struct
{
    ItemShape shape;
    float     radius;       // circle radius, or capsule half-thickness
    float     length;       // capsule end-to-end length (unused for circles)
    Color     fillColor;
    Color     outlineColor;
} ItemSpec;

static const ItemSpec ITEM_SPECS[ITEM_TYPE_COUNT] = {
    [ITEM_BALL] = { .shape = ITEM_SHAPE_CIRCLE,  .radius = 30.0f, .length =  0.0f, .fillColor = BLACK, .outlineColor = DARKGRAY },
    [ITEM_BAT]  = { .shape = ITEM_SHAPE_CAPSULE, .radius =  11.0f, .length = 186.0f, .fillColor = BROWN, .outlineColor = BLACK },
};

ItemShape ItemShapeOf(ItemType type)
{
    return ITEM_SPECS[type].shape;
}

// =============================================================================
//  CONSTRUCTION
// =============================================================================

void CreateItem(Item *item, ItemType type, Vector2 startPos)
{
    ItemSpec spec = ITEM_SPECS[type];

    *item = (Item){0};
    item->type = type;

    int particleCount = 0;
    int boneCount     = 0;

    if (spec.shape == ITEM_SHAPE_CAPSULE)
    {
        // Two endpoints `length` apart (laid out horizontally), joined by a
        // rigid bone so the bar keeps its length while it swings.
        item->particles[0] = (Particle){ .pos = startPos, .oldPos = startPos, .radius = spec.radius };
        Vector2 end        = { startPos.x + spec.length, startPos.y };
        item->particles[1] = (Particle){ .pos = end, .oldPos = end, .radius = spec.radius };
        item->bones[0]     = (Bone){ .particle1 = 0, .particle2 = 1,
                                     .length = ParticlesDistance(&item->particles[0], &item->particles[1]),
                                     .soft = false };
        particleCount = 2;
        boneCount     = 1;
    }
    else // ITEM_SHAPE_CIRCLE
    {
        item->particles[0] = (Particle){ .pos = startPos, .oldPos = startPos, .radius = spec.radius };
        particleCount = 1;
    }

    item->body = (Body){
        .particles     = item->particles,
        .particleCount = particleCount,
        .bones         = item->bones,
        .boneCount     = boneCount,
        .cfg           = DEFAULT_PHYSICS_CONFIG,
    };
    item->body.bounds = ComputeBoundBox(&item->body);
}

// =============================================================================
//  RENDERING
// =============================================================================

// Drawn relative to the bounding-box origin (the window is positioned
// WINDOW_MARGIN up-left of the box, see UpdateWindow), like DrawPuppet.
void DrawItem(const Item *item)
{
    ItemSpec spec = ITEM_SPECS[item->type];

    BoundBox b = item->body.bounds;
    float ox = b.x - WINDOW_MARGIN;
    float oy = b.y - WINDOW_MARGIN;

    ClearBackground(BLANK);

    if (spec.shape == ITEM_SHAPE_CAPSULE)
    {
        // Bat skin, loaded lazily on first draw (LoadTexture needs the window open).
        static Texture2D texBat;
        static bool texLoaded = false;
        if (!texLoaded)
        {
            texBat    = LoadAssetTexture("i_bat.png");
            texLoaded = true;
        }

        Vector2 a = { item->particles[0].pos.x - ox, item->particles[0].pos.y - oy };
        Vector2 c = { item->particles[1].pos.x - ox, item->particles[1].pos.y - oy };
        if (texBat.id)
        {
            // Sprite is horizontal, handle on the left: stretch it over the full
            // capsule (end caps included) with the handle at particle 0, rotated
            // around that end to follow the bar.
            float dx = c.x - a.x, dy = c.y - a.y;
            float len = sqrtf(dx * dx + dy * dy);
            Rectangle src = { 0, 0, (float)texBat.width, (float)texBat.height };
            Rectangle dst = { a.x, a.y, len + 2 * spec.radius, 2 * spec.radius };
            DrawTexturePro(texBat, src, dst, (Vector2){ spec.radius, spec.radius },
                           atan2f(dy, dx) * RAD2DEG, WHITE);
        }
        else
            DrawLineEx(a, c, 2 * spec.radius, spec.fillColor); // skin missing on disk
    }
    else // ITEM_SHAPE_CIRCLE: bowling ball with finger holes that roll with it
    {
        // No stored orientation on a 1-particle body: integrate a roll angle
        // from horizontal velocity (rolling without slipping, vx / r).
        // ponytail: static is fine, each item child is its own process.
        static float roll = 0.0f;
        roll += (item->particles[0].pos.x - item->particles[0].oldPos.x) / spec.radius;

        Vector2 c = { item->particles[0].pos.x - ox, item->particles[0].pos.y - oy };
        DrawCircleV    (c, spec.radius, spec.fillColor);
        DrawCircleLines((int)c.x, (int)c.y, spec.radius, spec.outlineColor);

        // Three finger holes clustered above center (fractions of the radius).
        static const Vector2 HOLES[3] = { {-0.22f, -0.30f}, {0.22f, -0.30f}, {0.0f, 0.02f} };
        float cs = cosf(roll), sn = sinf(roll);
        for (int i = 0; i < 3; i++)
        {
            Vector2 h = { HOLES[i].x * spec.radius, HOLES[i].y * spec.radius };
            Vector2 p = { c.x + h.x * cs - h.y * sn, c.y + h.x * sn + h.y * cs };
            DrawCircleV(p, spec.radius * 0.12f, DARKGRAY);
        }
    }
}
