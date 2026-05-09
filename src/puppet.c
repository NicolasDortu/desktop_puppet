#include "config.h"
#include "puppet.h"

#include "string.h"
#include <math.h>

#include "raylib.h"

// Initialize the limbs and their relative positions based on the puppet's radius
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

// Calculate the limb's position based on the puppet's rotation
// When the rotation is 0, cos = 1 and sin = 0, so the limb's position remains unchanged
Vector2 GetLimbsPosition(Puppet *pup, Vector2 limbPos)
{
    float cosAngle = cosf(pup->rotation);
    float sinAngle = sinf(pup->rotation);
    return (Vector2){
        .x = limbPos.x * cosAngle - limbPos.y * sinAngle,
        .y = limbPos.x * sinAngle + limbPos.y * cosAngle};
}

Puppet CreatePuppet(const char *name, Color color, float radius, Vector2 startPos)
{
    Puppet pup;
    strncpy(pup.name, name, sizeof(pup.name) - 1);
    pup.name[sizeof(pup.name) - 1] = '\0';
    pup.color = color;
    pup.radius = radius;
    pup.position = startPos;
    pup.rotation = 0.0f;
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