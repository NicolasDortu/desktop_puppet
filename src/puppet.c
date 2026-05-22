#include "config.h"
#include "puppet.h"

#include <string.h>
#include <math.h>

#include "raylib.h"

// =============================================================================
//  GEOMETRY QUERIES
// =============================================================================

// Axis-aligned bounding box around every limb (accounting for their radii).
PuppetBounds ComputePuppetBounds(const Puppet *pup)
{
    float minX =  1e30f, minY =  1e30f;
    float maxX = -1e30f, maxY = -1e30f;

    for (int i = 0; i < LIMB_COUNT; i++)
    {
        const PuppetLimb *limb = &pup->limbs[i];

        float lx = limb->pos.x - limb->radius;
        float ly = limb->pos.y - limb->radius;
        float rx = limb->pos.x + limb->radius;
        float ry = limb->pos.y + limb->radius;

        if (lx < minX) minX = lx;
        if (ly < minY) minY = ly;
        if (rx > maxX) maxX = rx;
        if (ry > maxY) maxY = ry;
    }

    return (PuppetBounds){.x = minX, .y = minY, .w = maxX - minX, .h = maxY - minY};
}

// Euclidean distance between the centers of two limbs.
float LimbsDistance(const Puppet *pup, LimbId limb1, LimbId limb2)
{
    float dx = pup->limbs[limb2].pos.x - pup->limbs[limb1].pos.x;
    float dy = pup->limbs[limb2].pos.y - pup->limbs[limb1].pos.y;
    return sqrtf(dx * dx + dy * dy);
}

// =============================================================================
//  INITIALIZATION
// =============================================================================

// Place each limb in a default standing pose around `startPos`.
// Limb sizes are expressed as fractions of the puppet's overall radius `R`,
// so the whole figure scales uniformly with `pup->radius`.
static PuppetBounds InitPuppetLimbs(Puppet *pup, Vector2 startPos)
{
    // --- Limb radii (fractions of overall puppet radius) ---
    float R     = pup->radius;
    float bodyR = R * 0.60f;
    float headR = R * 0.40f;
    float limbR = R * 0.25f;

    // --- Feet placement helpers ---
    float feetOffset = 0.5f;              // horizontal spread of feet around body center
    float feetGround = R - bodyR - limbR; // vertical drop so feet sit on the "ground"

    Vector2 c = startPos;

    // --- Per-limb anchor positions (body is at the center) ---
    Vector2 bodyPos  = c;
    Vector2 headPos  = { c.x,                       c.y - (bodyR + headR)       };
    Vector2 armLPos  = { c.x - (bodyR + limbR),     c.y                         };
    Vector2 armRPos  = { c.x + (bodyR + limbR),     c.y                         };
    Vector2 footLPos = { c.x - bodyR * feetOffset,  c.y + bodyR + feetGround    };
    Vector2 footRPos = { c.x + bodyR * feetOffset,  c.y + bodyR + feetGround    };

    // --- Build the limbs (oldPos = pos => zero initial velocity) ---
    pup->limbs[LIMB_BODY]   = (PuppetLimb){ .pos = bodyPos,  .oldPos = bodyPos,  .radius = bodyR, .color = pup->color };
    pup->limbs[LIMB_HEAD]   = (PuppetLimb){ .pos = headPos,  .oldPos = headPos,  .radius = headR, .color = BLUE   };
    pup->limbs[LIMB_ARM_L]  = (PuppetLimb){ .pos = armLPos,  .oldPos = armLPos,  .radius = limbR, .color = GREEN  };
    pup->limbs[LIMB_ARM_R]  = (PuppetLimb){ .pos = armRPos,  .oldPos = armRPos,  .radius = limbR, .color = GREEN  };
    pup->limbs[LIMB_FOOT_L] = (PuppetLimb){ .pos = footLPos, .oldPos = footLPos, .radius = limbR, .color = RED    };
    pup->limbs[LIMB_FOOT_R] = (PuppetLimb){ .pos = footRPos, .oldPos = footRPos, .radius = limbR, .color = RED    };

    return ComputePuppetBounds(pup);
}

// Wire the limbs together with distance constraints.
// Rest lengths are captured from the freshly-initialized pose,
// so the puppet always tries to return to that shape.
static void InitPuppetBones(Puppet *pup)
{
    // -- Hard bones: rigid skeleton --
    pup->bones[BODY_HEAD]   = (PuppetBone){ LIMB_BODY, LIMB_HEAD,   LimbsDistance(pup, LIMB_BODY, LIMB_HEAD)   };
    pup->bones[BODY_ARM_L]  = (PuppetBone){ LIMB_BODY, LIMB_ARM_L,  LimbsDistance(pup, LIMB_BODY, LIMB_ARM_L)  };
    pup->bones[BODY_ARM_R]  = (PuppetBone){ LIMB_BODY, LIMB_ARM_R,  LimbsDistance(pup, LIMB_BODY, LIMB_ARM_R)  };
    pup->bones[BODY_FOOT_L] = (PuppetBone){ LIMB_BODY, LIMB_FOOT_L, LimbsDistance(pup, LIMB_BODY, LIMB_FOOT_L) };
    pup->bones[BODY_FOOT_R] = (PuppetBone){ LIMB_BODY, LIMB_FOOT_R, LimbsDistance(pup, LIMB_BODY, LIMB_FOOT_R) };
    pup->bones[HEAD_FOOT_L] = (PuppetBone){ LIMB_HEAD, LIMB_FOOT_L, LimbsDistance(pup, LIMB_HEAD, LIMB_FOOT_L) };
    pup->bones[HEAD_FOOT_R] = (PuppetBone){ LIMB_HEAD, LIMB_FOOT_R, LimbsDistance(pup, LIMB_HEAD, LIMB_FOOT_R) };

    // -- Soft bones: gently restore arms to their rest angle --
    pup->bones[HEAD_ARM_L]  = (PuppetBone){ LIMB_HEAD, LIMB_ARM_L,  LimbsDistance(pup, LIMB_HEAD, LIMB_ARM_L)  };
    pup->bones[HEAD_ARM_R]  = (PuppetBone){ LIMB_HEAD, LIMB_ARM_R,  LimbsDistance(pup, LIMB_HEAD, LIMB_ARM_R)  };
}

// =============================================================================
//  PUPPET ENTRY POINT
// =============================================================================

// Build a fully-initialized puppet (limbs, bones, default physics) at `startPos`.
Puppet CreatePuppet(const char *name, Color color, float radius, Vector2 startPos)
{
    Puppet pup = {0};

    // -- Identity --
    strncpy(pup.name, name, sizeof(pup.name) - 1);
    pup.name[sizeof(pup.name) - 1] = '\0';
    pup.color       = color;
    pup.radius      = radius;
    pup.draggedLimb = -1;

    // -- Physics tuning --
    pup.physics = (PhysicsConfig){
        .gravity   = DEFAULT_GRAVITY,
        .friction  = DEFAULT_FRICTION,
        .bounce    = DEFAULT_BOUNCE,
        .minBounce = DEFAULT_MIN_BOUNCE,
        .stiffness = DEFAULT_STIFFNESS,
    };

    // -- Skeleton --
    pup.bounds = InitPuppetLimbs(&pup, startPos);
    InitPuppetBones(&pup);

    return pup;
}