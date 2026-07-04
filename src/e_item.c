#include "e_item.h"
#include "config.h"
#include "physics.h"

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
    [ITEM_BALL] = { .shape = ITEM_SHAPE_CIRCLE,  .radius = 30.0f, .length =  0.0f, .fillColor = GRAY,  .outlineColor = BLACK },
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
        Vector2 a = { item->particles[0].pos.x - ox, item->particles[0].pos.y - oy };
        Vector2 c = { item->particles[1].pos.x - ox, item->particles[1].pos.y - oy };
        DrawLineEx(a, c, 2 * spec.radius, spec.fillColor);
    }
    else // ITEM_SHAPE_CIRCLE
    {
        Vector2 c = { item->particles[0].pos.x - ox, item->particles[0].pos.y - oy };
        DrawCircleV    (c, spec.radius, spec.fillColor);
        DrawCircleLines((int)c.x, (int)c.y, spec.radius, spec.outlineColor);
    }
}
