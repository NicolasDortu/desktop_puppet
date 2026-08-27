#include "e_puppet.h"
#include "config.h"
#include "physics.h"
#include "renderer.h"

#include <math.h>

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

    // Limbs sit partly embedded in the body (Interactive Buddy look): hands
    // ride the rim at the upper sides (15 degrees above mid-height), feet ride
    // the rim at the bottom, and the head overlaps the top by half its radius.
    float feetSpread = 0.55f;          // horizontal spread of the feet along the rim
    Vector2 armRest  = { bodyR * 0.966f, -bodyR * 0.259f };

    Vector2 c = startPos;

    // --- Per-limb anchor positions (body is at the center) ---
    Vector2 bodyPos  = c;
    Vector2 headPos  = { c.x,                      c.y - (bodyR + headR * 0.5f) };
    Vector2 armLPos  = { c.x - armRest.x,          c.y + armRest.y              };
    Vector2 armRPos  = { c.x + armRest.x,          c.y + armRest.y              };
    Vector2 footLPos = { c.x - bodyR * feetSpread, c.y + bodyR                  };
    Vector2 footRPos = { c.x + bodyR * feetSpread, c.y + bodyR                  };

    // --- Build the limbs (oldPos = pos => zero initial velocity) ---
    pup->limbs[LIMB_BODY]   = (Particle){ .pos = bodyPos,  .oldPos = bodyPos,  .radius = bodyR };
    pup->limbs[LIMB_HEAD]   = (Particle){ .pos = headPos,  .oldPos = headPos,  .radius = headR };
    pup->limbs[LIMB_ARM_L]  = (Particle){ .pos = armLPos,  .oldPos = armLPos,  .radius = limbR };
    pup->limbs[LIMB_ARM_R]  = (Particle){ .pos = armRPos,  .oldPos = armRPos,  .radius = limbR };
    pup->limbs[LIMB_FOOT_L] = (Particle){ .pos = footLPos, .oldPos = footLPos, .radius = limbR };
    pup->limbs[LIMB_FOOT_R] = (Particle){ .pos = footRPos, .oldPos = footRPos, .radius = limbR };

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
//
// HARD bones fix the structure: every limb stays at its distance from the
// body (so the embedded look holds) and the feet cannot cross. SOFT bones are
// the springs: the diagonals to the head let arms and feet swing with lag and
// bounce back (PHYS_STIFFNESS sets how snappy). Distance springs are drift-safe
// — they only push along the line between two particles and have a true rest
// point — unlike a positional pull toward a rotating target, which pumps
// energy and made the puppet walk on its own.
static void InitPuppetBones(Puppet *pup)
{
    pup->bones[BODY_HEAD]   = (Bone){ LIMB_BODY,   LIMB_HEAD,   ParticlesDistance(&pup->limbs[LIMB_BODY],   &pup->limbs[LIMB_HEAD]),   false };
    pup->bones[BODY_ARM_L]  = (Bone){ LIMB_BODY,   LIMB_ARM_L,  ParticlesDistance(&pup->limbs[LIMB_BODY],   &pup->limbs[LIMB_ARM_L]),  false };
    pup->bones[BODY_ARM_R]  = (Bone){ LIMB_BODY,   LIMB_ARM_R,  ParticlesDistance(&pup->limbs[LIMB_BODY],   &pup->limbs[LIMB_ARM_R]),  false };
    pup->bones[BODY_FOOT_L] = (Bone){ LIMB_BODY,   LIMB_FOOT_L, ParticlesDistance(&pup->limbs[LIMB_BODY],   &pup->limbs[LIMB_FOOT_L]), false };
    pup->bones[BODY_FOOT_R] = (Bone){ LIMB_BODY,   LIMB_FOOT_R, ParticlesDistance(&pup->limbs[LIMB_BODY],   &pup->limbs[LIMB_FOOT_R]), false };
    pup->bones[FOOT_FOOT]   = (Bone){ LIMB_FOOT_L, LIMB_FOOT_R, ParticlesDistance(&pup->limbs[LIMB_FOOT_L], &pup->limbs[LIMB_FOOT_R]), false };

    pup->bones[HEAD_FOOT_L] = (Bone){ LIMB_HEAD,   LIMB_FOOT_L, ParticlesDistance(&pup->limbs[LIMB_HEAD],   &pup->limbs[LIMB_FOOT_L]), true  };
    pup->bones[HEAD_FOOT_R] = (Bone){ LIMB_HEAD,   LIMB_FOOT_R, ParticlesDistance(&pup->limbs[LIMB_HEAD],   &pup->limbs[LIMB_FOOT_R]), true  };
    pup->bones[HEAD_ARM_L]  = (Bone){ LIMB_HEAD,   LIMB_ARM_L,  ParticlesDistance(&pup->limbs[LIMB_HEAD],   &pup->limbs[LIMB_ARM_L]),  true  };
    pup->bones[HEAD_ARM_R]  = (Bone){ LIMB_HEAD,   LIMB_ARM_R,  ParticlesDistance(&pup->limbs[LIMB_HEAD],   &pup->limbs[LIMB_ARM_R]),  true  };
}

// =============================================================================
//  ENTRY POINT
// =============================================================================

// Build a fully-initialized puppet (limbs, bones, default physics) at `startPos`.
void CreatePuppet(Puppet *pup, float radius, Vector2 startPos)
{
    *pup = (Puppet){0};

    // -- Identity --
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
    };
    pup->body.bounds = ComputeBoundBox(&pup->body);
}

// =============================================================================
//  WINDOW BOX
// =============================================================================

// Window box for the puppet: the body bounds plus speed slack. Pure function
// of the puppet state, so UpdateWindow and DrawPuppet compute the identical box.
BoundBox PuppetWindowBounds(const Puppet *pup)
{
    return AddSpeedSlack(&pup->body, pup->body.bounds);
}

// =============================================================================
//  POSE ENFORCEMENT
// =============================================================================

// Chirality guard, run once per frame after physics. Distance constraints
// cannot tell left from right (a mirrored pose satisfies every bone length),
// so a hard spin can settle an arm on the wrong side of the body or below the
// feet. Reflect any limb found on the wrong side of the body->head axis back
// onto its own side; paired limbs are identical circles, so the snap is
// invisible on screen.
void EnforcePuppetPose(Puppet *pup)
{
    Vector2 body = pup->limbs[LIMB_BODY].pos;
    Vector2 head = pup->limbs[LIMB_HEAD].pos;
    float   dx   = head.x - body.x;
    float   dy   = head.y - body.y;
    float   len  = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f)
        return;

    // `n` is the puppet's local left-right axis; rest-pose LEFT limbs have a
    // negative coordinate along it, RIGHT limbs a positive one.
    Vector2 n = { -dy / len, dx / len };

    static const struct { LimbId limb; float side; } SIDES[] = {
        { LIMB_ARM_L,  -1.0f }, { LIMB_ARM_R,  1.0f },
        { LIMB_FOOT_L, -1.0f }, { LIMB_FOOT_R, 1.0f },
    };

    // A limb is only corrected when CALM relative to the body. Reflecting a
    // fast limb reverses its lateral momentum every frame, which turned the
    // body-head axis into an invisible wall: a puppet thrown by an arm or a
    // foot stopped dead mid-air. Wrong sides in flight are invisible (paired
    // limbs are identical); the guard cleans up once the motion settles.
    const float calmSpeed = 2.0f; // px/frame of limb speed relative to the body
    Particle    bodyP     = pup->limbs[LIMB_BODY];
    Vector2     bodyVel   = { bodyP.pos.x - bodyP.oldPos.x, bodyP.pos.y - bodyP.oldPos.y };

    for (int i = 0; i < 4; i++)
    {
        Particle *p = &pup->limbs[SIDES[i].limb];
        if (p->isDragged)
            continue; // respect the user's grab; corrected on release

        float rvx = (p->pos.x - p->oldPos.x) - bodyVel.x;
        float rvy = (p->pos.y - p->oldPos.y) - bodyVel.y;
        if (rvx * rvx + rvy * rvy > calmSpeed * calmSpeed)
            continue; // limb in motion: leave it alone this frame

        float d = (p->pos.x - body.x) * n.x + (p->pos.y - body.y) * n.y;
        if (d * SIDES[i].side >= -1.0f)
            continue; // on its own side (1px of axis tolerance against jitter)

        // Mirror pos across the axis, then keep the limb moving WITH the body.
        // (Mirroring oldPos too reflected the limb's ABSOLUTE velocity: during
        // a throw the calm-check passes — limbs fly with the body — so the
        // reflection flipped the limb's lateral flight velocity, the bones
        // fought it, and the puppet stopped dead or veered. The limb is calm
        // relative to the body here, so continuing at the body's velocity
        // discards at most calmSpeed px/frame — invisible.)
        p->pos.x    -= 2.0f * d * n.x;
        p->pos.y    -= 2.0f * d * n.y;
        p->oldPos.x  = p->pos.x - bodyVel.x;
        p->oldPos.y  = p->pos.y - bodyVel.y;
    }

    // Reflections move particles: refresh the cached bounds.
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

    // Same box UpdateWindow used this frame (the window sits WINDOW_MARGIN
    // up-left of it, like the items).
    BoundBox b = PuppetWindowBounds(pup);
    float winOriginX = b.x - WINDOW_MARGIN;
    float winOriginY = b.y - WINDOW_MARGIN;

    for (int i = 0; i < LIMB_COUNT; i++)
    {
        Particle limb = pup->limbs[i];
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

    // -- Eyes: ride on the head — X eyes while hurt, plain dots otherwise.
    // The puppet has no stored orientation, so "up" comes from the body->head
    // axis; the eyes swing around the head center as the puppet tumbles.
    Vector2 headPos = pup->limbs[LIMB_HEAD].pos;
    Vector2 bodyPos = pup->limbs[LIMB_BODY].pos;
    float   headR   = pup->limbs[LIMB_HEAD].radius;
    float   len     = sqrtf((headPos.x - bodyPos.x) * (headPos.x - bodyPos.x) +
                            (headPos.y - bodyPos.y) * (headPos.y - bodyPos.y));
    if (len > 0.001f)
    {
        Vector2 up   = { (headPos.x - bodyPos.x) / len, (headPos.y - bodyPos.y) / len };
        Vector2 side = { -up.y, up.x };
        bool    hurt = pup->hurtFrames > 0;

        for (int s = -1; s <= 1; s += 2)
        {
            Vector2 eye = {
                headPos.x - winOriginX + up.x * headR * 0.20f + side.x * headR * 0.35f * s,
                headPos.y - winOriginY + up.y * headR * 0.20f + side.y * headR * 0.35f * s,
            };
            float er = headR * 0.13f;

            if (hurt) // X eyes, tilted with the head
            {
                float   k  = er * 1.1f;
                Vector2 d1 = { (up.x + side.x) * k, (up.y + side.y) * k };
                Vector2 d2 = { (up.x - side.x) * k, (up.y - side.y) * k };
                DrawLineEx((Vector2){ eye.x - d1.x, eye.y - d1.y },
                           (Vector2){ eye.x + d1.x, eye.y + d1.y }, er * 0.55f, BLACK);
                DrawLineEx((Vector2){ eye.x - d2.x, eye.y - d2.y },
                           (Vector2){ eye.x + d2.x, eye.y + d2.y }, er * 0.55f, BLACK);
            }
            else // calm: plain dots
                DrawCircleV(eye, er, BLACK);
        }
    }
}
