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
    float     punch;        // velocity multiplier inflicted on puppet limbs (1 = neutral, heavy items > 1)
    Color     fillColor;
    Color     outlineColor;
} ItemSpec;

static const ItemSpec ITEM_SPECS[ITEM_TYPE_COUNT] = {
    [ITEM_BALL]    = { .shape = ITEM_SHAPE_CIRCLE,  .radius = 30.0f, .length =   0.0f, .punch = 3.0f, .fillColor = BLACK,     .outlineColor = DARKGRAY },
    [ITEM_BAT]     = { .shape = ITEM_SHAPE_CAPSULE, .radius = 11.0f, .length = 186.0f, .punch = 1.0f, .fillColor = BROWN,     .outlineColor = BLACK    },
    [ITEM_BOMB]    = { .shape = ITEM_SHAPE_CIRCLE,  .radius = 22.0f, .length =   0.0f, .punch = 1.0f, .fillColor = BLACK,     .outlineColor = DARKGRAY },
    [ITEM_MISSILE] = { .shape = ITEM_SHAPE_CIRCLE,  .radius = 13.0f, .length =   0.0f, .punch = 1.0f, .fillColor = LIGHTGRAY, .outlineColor = DARKGRAY },
};

ItemShape ItemShapeOf(ItemType type)
{
    return ITEM_SPECS[type].shape;
}

float ItemPunchOf(ItemType type)
{
    return ITEM_SPECS[type].punch;
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
    };
    item->body.bounds = ComputeBoundBox(&item->body);
}

// =============================================================================
//  RENDERING
// =============================================================================

// Window box for an item: body bounds plus headroom for decorations drawn
// outside the physics shape (the bomb's fuse sticks out of the top), plus
// speed slack.
BoundBox ItemWindowBounds(const Item *item)
{
    BoundBox b = item->body.bounds;
    if (item->type == ITEM_BOMB)
    {
        float room = ITEM_SPECS[ITEM_BOMB].radius; // fuse + spark headroom
        b.y -= room;
        b.h += room;
    }
    else if (item->type == ITEM_MISSILE)
    {
        // Nose, fins and flame overdraw the collision circle in any direction.
        float room = 1.6f * ITEM_SPECS[ITEM_MISSILE].radius;
        b.x -= room;
        b.y -= room;
        b.w += 2.0f * room;
        b.h += 2.0f * room;
    }
    return AddSpeedSlack(&item->body, b);
}

// Cartoon bomb at center `c` with body radius `r`: shaded iron sphere with a
// sheen and glint, metal collar, drooping fuse, blinking spark star. Stays
// within the fuse headroom ItemWindowBounds reserves (r above the body).
static void DrawBomb(Vector2 c, float r)
{
    // -- Fuse: droops from the collar up-right to the spark (drawn first so
    // the collar covers its base) --
    Vector2 base = { c.x, c.y - 1.05f * r };
    Vector2 tip  = { c.x + 0.55f * r, c.y - 1.55f * r };
    DrawLineBezier(base, tip, 0.14f * r, (Color){ 150, 110, 70, 255 });

    // -- Body: dark sphere, soft up-left sheen, bright glint, crisp rim --
    DrawCircleV(c, r, (Color){ 35, 38, 46, 255 });
    DrawCircleGradient((int)(c.x - 0.30f * r), (int)(c.y - 0.35f * r), 0.75f * r,
                       (Color){ 120, 130, 150, 140 }, (Color){ 120, 130, 150, 0 });
    DrawCircleV((Vector2){ c.x - 0.38f * r, c.y - 0.42f * r }, 0.16f * r,
                (Color){ 225, 232, 245, 190 });
    DrawCircleLines((int)c.x, (int)c.y, r, BLACK);

    // -- Metal collar the fuse plugs into --
    Rectangle cap = { c.x - 0.22f * r, c.y - 1.12f * r, 0.44f * r, 0.28f * r };
    DrawRectangleRounded(cap, 0.6f, 6, (Color){ 105, 112, 125, 255 });

    // -- Spark: blinking 4-point star with a hot core --
    bool  on = ((int)(GetTime() * 10.0) % 2) == 0;
    Color sc = on ? (Color){ 255, 230, 90, 255 } : (Color){ 255, 150, 40, 255 };
    float s  = (on ? 0.30f : 0.22f) * r;
    float d  = s * 0.6f;
    DrawLineEx((Vector2){ tip.x - s, tip.y }, (Vector2){ tip.x + s, tip.y }, 0.09f * r, sc);
    DrawLineEx((Vector2){ tip.x, tip.y - s }, (Vector2){ tip.x, tip.y + s }, 0.09f * r, sc);
    DrawLineEx((Vector2){ tip.x - d, tip.y - d }, (Vector2){ tip.x + d, tip.y + d }, 0.07f * r, sc);
    DrawLineEx((Vector2){ tip.x - d, tip.y + d }, (Vector2){ tip.x + d, tip.y - d }, 0.07f * r, sc);
    DrawCircleV(tip, 0.13f * r, (Color){ 255, 255, 210, 255 });
}

// Cartoon missile at center `c` (collision radius `r`), pointing along its
// velocity `v`: gray body capsule, red rounded nose and swept fins, cockpit
// dot, flickering exhaust flame. Circles and thick lines only (no winding
// worries), all sized as fractions of r.
static void DrawMissile(Vector2 c, Vector2 v, float r)
{
    float   ang = atan2f(v.y, v.x); // zero velocity at spawn: points right, fine
    Vector2 f   = { cosf(ang), sinf(ang) }; // forward
    Vector2 s   = { -f.y, f.x };            // side

    // -- Exhaust: flickering two-tone flame behind the tail --
    bool  hot = ((int)(GetTime() * 20.0) % 2) == 0;
    float fl  = (hot ? 0.55f : 0.40f) * r;
    DrawCircleV((Vector2){ c.x - f.x * 2.0f * r, c.y - f.y * 2.0f * r }, fl, ORANGE);
    DrawCircleV((Vector2){ c.x - f.x * 1.7f * r, c.y - f.y * 1.7f * r }, fl * 0.7f, YELLOW);

    // -- Fins: two swept-back strokes at the tail --
    Vector2 tail = { c.x - f.x * 1.1f * r, c.y - f.y * 1.1f * r };
    for (int e = -1; e <= 1; e += 2)
    {
        Vector2 tip = { tail.x - f.x * 0.9f * r + s.x * 1.2f * r * e,
                        tail.y - f.y * 0.9f * r + s.y * 1.2f * r * e };
        DrawLineEx(tail, tip, 0.45f * r, RED);
    }

    // -- Body: gray capsule, red rounded nose, cockpit dot --
    Vector2 noseB = { c.x + f.x * 1.2f * r, c.y + f.y * 1.2f * r };
    DrawLineEx(tail, noseB, 1.4f * r, (Color){ 200, 205, 215, 255 });
    DrawCircleV(noseB, 0.7f * r, RED);
    DrawCircleV((Vector2){ c.x + f.x * 0.5f * r, c.y + f.y * 0.5f * r }, 0.28f * r,
                (Color){ 60, 70, 90, 255 });
}

// Drawn relative to the window box origin (the window is positioned
// WINDOW_MARGIN up-left of the box, see UpdateWindow), like DrawPuppet.
void DrawItem(const Item *item)
{
    ItemSpec spec = ITEM_SPECS[item->type];

    BoundBox b = ItemWindowBounds(item);
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
    else if (item->type == ITEM_BOMB)
    {
        Vector2 c = { item->particles[0].pos.x - ox, item->particles[0].pos.y - oy };
        DrawBomb(c, spec.radius);
    }
    else if (item->type == ITEM_MISSILE)
    {
        Vector2 c = { item->particles[0].pos.x - ox, item->particles[0].pos.y - oy };
        Vector2 v = { item->particles[0].pos.x - item->particles[0].oldPos.x,
                      item->particles[0].pos.y - item->particles[0].oldPos.y };
        DrawMissile(c, v, spec.radius);
    }
    else // ITEM_SHAPE_CIRCLE
    {
        Vector2 c = { item->particles[0].pos.x - ox, item->particles[0].pos.y - oy };
        DrawCircleV    (c, spec.radius, spec.fillColor);
        DrawCircleLines((int)c.x, (int)c.y, spec.radius, spec.outlineColor);

        if (item->type == ITEM_BALL) // bowling ball: finger holes that roll with it
        {
            // No stored orientation on a 1-particle body: integrate a roll angle
            // from horizontal velocity (rolling without slipping, vx / r).
            // ponytail: static is fine, each item child is its own process.
            static float roll = 0.0f;
            roll += (item->particles[0].pos.x - item->particles[0].oldPos.x) / spec.radius;

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
}

// Cartoon blast: an expanding shell with a bright core, fading out as
// `progress` runs 0 -> 1. Drawn in window-local coordinates.
void DrawExplosion(Vector2 center, float maxRadius, float progress)
{
    ClearBackground(BLANK);

    float ease  = 1.0f - (1.0f - progress) * (1.0f - progress); // fast start, soft end
    float r     = maxRadius * ease;
    unsigned char a = (unsigned char)(200.0f * (1.0f - progress));

    DrawCircleV(center, r,         (Color){ 255, 120,  30, a });
    DrawCircleV(center, r * 0.6f,  (Color){ 255, 200,  60, a });
    DrawCircleV(center, r * 0.3f,  (Color){ 255, 255, 160, a });
}
