#include "e_puppet.h"
#include "config.h"
#include "physics.h"
#include "renderer.h"

#include <string.h>

#include "raylib.h"

// =============================================================================
//  INITIALIZATION
// =============================================================================

// Place each limb in a default standing pose around `startPos`.
// Limb sizes are expressed as fractions of the puppet's overall radius `R`
static void InitPuppetLimbs(Puppet *pup, Vector2 startPos)
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
    pup->limbs[LIMB_BODY]   = (PuppetLimb){ .pos = bodyPos,  .oldPos = bodyPos,  .radius = bodyR };
    pup->limbs[LIMB_HEAD]   = (PuppetLimb){ .pos = headPos,  .oldPos = headPos,  .radius = headR };
    pup->limbs[LIMB_ARM_L]  = (PuppetLimb){ .pos = armLPos,  .oldPos = armLPos,  .radius = limbR };
    pup->limbs[LIMB_ARM_R]  = (PuppetLimb){ .pos = armRPos,  .oldPos = armRPos,  .radius = limbR };
    pup->limbs[LIMB_FOOT_L] = (PuppetLimb){ .pos = footLPos, .oldPos = footLPos, .radius = limbR };
    pup->limbs[LIMB_FOOT_R] = (PuppetLimb){ .pos = footRPos, .oldPos = footRPos, .radius = limbR };

    // --- Per-limb render colors ---
    pup->limbColors[LIMB_BODY]   = YELLOW;
    pup->limbColors[LIMB_HEAD]   = BLUE;
    pup->limbColors[LIMB_ARM_L]  = GREEN;
    pup->limbColors[LIMB_ARM_R]  = GREEN;
    pup->limbColors[LIMB_FOOT_L] = RED;
    pup->limbColors[LIMB_FOOT_R] = RED;
}

// Wire the limbs together with distance constraints.
// Rest lengths are captured from the freshly-initialized pose,
// so the puppet always tries to return to that shape.
static void InitPuppetBones(Puppet *pup)
{
    // -- Hard bones: rigid skeleton --
    pup->bones[BODY_HEAD]   = (PuppetBone){ LIMB_BODY, LIMB_HEAD,   ParticlesDistance(&pup->limbs[LIMB_BODY], &pup->limbs[LIMB_HEAD]),   false };
    pup->bones[BODY_ARM_L]  = (PuppetBone){ LIMB_BODY, LIMB_ARM_L,  ParticlesDistance(&pup->limbs[LIMB_BODY], &pup->limbs[LIMB_ARM_L]),  false };
    pup->bones[BODY_ARM_R]  = (PuppetBone){ LIMB_BODY, LIMB_ARM_R,  ParticlesDistance(&pup->limbs[LIMB_BODY], &pup->limbs[LIMB_ARM_R]),  false };
    pup->bones[BODY_FOOT_L] = (PuppetBone){ LIMB_BODY, LIMB_FOOT_L, ParticlesDistance(&pup->limbs[LIMB_BODY], &pup->limbs[LIMB_FOOT_L]), false };
    pup->bones[BODY_FOOT_R] = (PuppetBone){ LIMB_BODY, LIMB_FOOT_R, ParticlesDistance(&pup->limbs[LIMB_BODY], &pup->limbs[LIMB_FOOT_R]), false };
    pup->bones[HEAD_FOOT_L] = (PuppetBone){ LIMB_HEAD, LIMB_FOOT_L, ParticlesDistance(&pup->limbs[LIMB_HEAD], &pup->limbs[LIMB_FOOT_L]), false };
    pup->bones[HEAD_FOOT_R] = (PuppetBone){ LIMB_HEAD, LIMB_FOOT_R, ParticlesDistance(&pup->limbs[LIMB_HEAD], &pup->limbs[LIMB_FOOT_R]), false };

    // -- Soft bones: gently restore to their rest angle --
    pup->bones[HEAD_ARM_L]  = (PuppetBone){ LIMB_HEAD, LIMB_ARM_L,  ParticlesDistance(&pup->limbs[LIMB_HEAD], &pup->limbs[LIMB_ARM_L]),  true  };
    pup->bones[HEAD_ARM_R]  = (PuppetBone){ LIMB_HEAD, LIMB_ARM_R,  ParticlesDistance(&pup->limbs[LIMB_HEAD], &pup->limbs[LIMB_ARM_R]),  true  };
}

// =============================================================================
//  ENTRY POINT
// =============================================================================

// Build a fully-initialized puppet (limbs, bones, default physics) at `startPos`.
void CreatePuppet(Puppet *pup, Color color, float radius, Vector2 startPos)
{
    *pup = (Puppet){0};

    // -- Identity --
    pup->color  = color;
    pup->radius = radius;

    // -- Skeleton --
    InitPuppetLimbs(pup, startPos);
    InitPuppetBones(pup);

    // -- Physics wiring: body holds pointers into pup's own arrays --
    pup->body = (Body){
        .particles     = pup->limbs,
        .particleCount = LIMB_COUNT,
        .bones         = pup->bones,
        .boneCount     = BONE_COUNT,
        .cfg           = DEFAULT_PHYSICS_CONFIG,
    };
    pup->body.bounds = ComputeBoundBox(&pup->body);
}

// =============================================================================
//  RENDERING
// =============================================================================

void DrawPuppet(const Puppet *pup)
{
    // Skins, loaded lazily on first draw (LoadTexture needs the window open).
    static Texture2D texBody, texHead, texHand;
    static bool texLoaded = false;
    if (!texLoaded)
    {
        texBody   = LoadAssetTexture("b_body.png");
        texHead   = LoadAssetTexture("b_head.png");
        texHand   = LoadAssetTexture("b_hand.png"); // hands and feet share a skin
        texLoaded = true;
    }

    ClearBackground(BLANK);

    BoundBox b = pup->body.bounds;
    float winOriginX = b.x;
    float winOriginY = b.y;

    for (int i = 0; i < LIMB_COUNT; i++)
    {
        PuppetLimb limb = pup->limbs[i];
        Vector2 local = {limb.pos.x - winOriginX, limb.pos.y - winOriginY};
        Texture2D tex = (i == LIMB_BODY) ? texBody
                      : (i == LIMB_HEAD) ? texHead
                                         : texHand;
        if (tex.id)
        {
            Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
            Rectangle dst = { local.x, local.y, 2 * limb.radius, 2 * limb.radius };
            DrawTexturePro(tex, src, dst, (Vector2){ limb.radius, limb.radius }, 0.0f, WHITE);
        }
        else
            DrawCircleV(local, limb.radius, pup->limbColors[i]); // skin missing on disk
    }
}
