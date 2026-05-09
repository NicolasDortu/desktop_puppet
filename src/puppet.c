#include "config.h"
#include "puppet.h"

#include "string.h"
#include <math.h>

#include "raylib.h"

// All the limbs' positions are relative to the puppet's center
static void InitPuppetLimbs(Puppet *pup)
{
    float R = pup->radius;
    float bodyR = R * 0.60f;
    float headR = R * 0.40f;
    float limbR = R * 0.25f;
    float feetOffset = 0.5f;
    float feetGround = R - bodyR - limbR; // This is the missing distance for the feet touching the ground

    Color c = pup->color;

    // Body -> There is a small offset to make the body lower than the center
    pup->limbs[LIMB_BODY] = (PuppetLimb){.position = (Vector2){0.0f, R * 0.1f}, .radius = bodyR, .color = YELLOW};

    // Head
    pup->limbs[LIMB_HEAD] = (PuppetLimb){.position = (Vector2){0.0f, -bodyR}, .radius = headR, .color = c};

    // Arms
    pup->limbs[LIMB_ARM_L] = (PuppetLimb){.position = (Vector2){-bodyR, 0.0f}, .radius = limbR, .color = c};
    pup->limbs[LIMB_ARM_R] = (PuppetLimb){.position = (Vector2){bodyR, 0.0f}, .radius = limbR, .color = c};

    // Feet -> There is also an offset so the feet are not at the middle of the body
    pup->limbs[LIMB_FOOT_L] = (PuppetLimb){.position = (Vector2){-bodyR * feetOffset, bodyR + feetGround}, .radius = limbR, .color = c};
    pup->limbs[LIMB_FOOT_R] = (PuppetLimb){.position = (Vector2){bodyR * feetOffset, bodyR + feetGround}, .radius = limbR, .color = c};
}

Puppet CreatePuppet(const char *name, Color color, float radius, Vector2 startPos)
{
    Puppet pup;
    strncpy(pup.name, name, sizeof(pup.name) - 1);
    pup.name[sizeof(pup.name) - 1] = '\0';
    pup.color = color;
    pup.radius = radius;
    pup.position = startPos;
    pup.velocity = (Vector2){0.0f, 0.0f};
    pup.isDragging = false;
    pup.physics = (PhysicsConfig){
        .gravity = DEFAULT_GRAVITY,
        .friction = DEFAULT_FRICTION,
        .bounce = DEFAULT_BOUNCE,
        .minBounceVel = DEFAULT_MIN_BOUNCE_VEL};
    InitPuppetLimbs(&pup);
    return pup;
}