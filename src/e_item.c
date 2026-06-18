#include "e_item.h"
#include "config.h"
#include "physics.h"

#include <stddef.h>

#include "raylib.h"

// =============================================================================
//  ENTITY
// =============================================================================

typedef struct
{
    float radius;
    Color fillColor;
    Color outlineColor;
} ItemSpec;

static const ItemSpec ITEM_SPECS[ITEM_TYPE_COUNT] = {
    [ITEM_BALL] = { .radius = 30.0f, .fillColor = GRAY, .outlineColor = BLACK },
};


void CreateItem(Particle *p, Body *body, ItemType type, Vector2 startPos)
{
    ItemSpec spec = ITEM_SPECS[type];

    *p = (Particle){
        .pos    = startPos,
        .oldPos = startPos,
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


void DrawItem(ItemType type)
{
    ItemSpec spec = ITEM_SPECS[type];
    ClearBackground(BLANK);
    DrawCircle     ((int)spec.radius, (int)spec.radius, spec.radius, spec.fillColor);
    DrawCircleLines((int)spec.radius, (int)spec.radius, spec.radius, spec.outlineColor);
}

int ItemWindowSize(ItemType type)
{
    return (int)(2 * ITEM_SPECS[type].radius);
}
