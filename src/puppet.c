#include "config.h"
#include "puppet.h"

#include "string.h"
#include <math.h>

#include "raylib.h"

// Initialize the limbs at world-space positions around the puppet's current center.
static PuppetBounds InitPuppetLimbs(Puppet *pup, Vector2 startPos)
{
    float R = pup->radius;
    float bodyR = R * 0.60f;
    float headR = R * 0.40f;
    float limbR = R * 0.25f;
    float feetOffset = 0.5f;
    float feetGround = R - bodyR - limbR; // This is the missing distance for the feet touching the ground

    float minX = 1e30f, minY = 1e30f, maxX = -1e30f, maxY = -1e30f; // Initialize min/max bounds

    // Color c = pup->color;
    Vector2 center = startPos;

    // Body sits at the puppet's anchor.
    Vector2 bodyPos = center;
    // Head rests on top of the body (touching).
    Vector2 headPos = {center.x, center.y - (bodyR + headR)};
    // Arms
    Vector2 armLPos = {center.x - (bodyR + limbR), center.y};
    Vector2 armRPos = {center.x + (bodyR + limbR), center.y};
    // Feet
    Vector2 footLPos = {center.x - bodyR * feetOffset, center.y + bodyR + feetGround};
    Vector2 footRPos = {center.x + bodyR * feetOffset, center.y + bodyR + feetGround};

    pup->limbs[LIMB_BODY] = (PuppetLimb){
        .pos = bodyPos, .oldPos = bodyPos, .radius = bodyR, .color = YELLOW};
    pup->limbs[LIMB_HEAD] = (PuppetLimb){
        .pos = headPos, .oldPos = headPos, .radius = headR, .color = BLUE};
    pup->limbs[LIMB_ARM_L] = (PuppetLimb){.pos = armLPos, .oldPos = armLPos, .radius = limbR, .color = GREEN};
    pup->limbs[LIMB_ARM_R] = (PuppetLimb){.pos = armRPos, .oldPos = armRPos, .radius = limbR, .color = GREEN};
    pup->limbs[LIMB_FOOT_L] = (PuppetLimb){.pos = footLPos, .oldPos = footLPos, .radius = limbR, .color = RED};
    pup->limbs[LIMB_FOOT_R] = (PuppetLimb){.pos = footRPos, .oldPos = footRPos, .radius = limbR, .color = RED};

    // Return the boundaries of the puppet
    for (int i = 0; i < LIMB_COUNT; i++)
    {
        if (pup->limbs[i].pos.x - pup->limbs[i].radius < minX)
            minX = pup->limbs[i].pos.x - pup->limbs[i].radius;
        if (pup->limbs[i].pos.y - pup->limbs[i].radius < minY)
            minY = pup->limbs[i].pos.y - pup->limbs[i].radius;
        if (pup->limbs[i].pos.x + pup->limbs[i].radius > maxX)
            maxX = pup->limbs[i].pos.x + pup->limbs[i].radius;
        if (pup->limbs[i].pos.y + pup->limbs[i].radius > maxY)
            maxY = pup->limbs[i].pos.y + pup->limbs[i].radius;
    }

    return (PuppetBounds){.x = minX, .y = minY, .w = maxX - minX, .h = maxY - minY};
}

// Compute the distance between two limbs.
float LimbsDistance(const Puppet *pup, LimbId limb1, LimbId limb2)
{
    float dx = pup->limbs[limb2].pos.x - pup->limbs[limb1].pos.x;
    float dy = pup->limbs[limb2].pos.y - pup->limbs[limb1].pos.y;
    return sqrtf(dx * dx + dy * dy);
}

// Add the bones/sticks to glue the limbs together.
static void InitPuppetBones(Puppet *pup)
{
    pup->bones[BODY_HEAD] = (PuppetBone){LIMB_BODY, LIMB_HEAD, LimbsDistance(pup, LIMB_BODY, LIMB_HEAD)};
    pup->bones[BODY_ARM_L] = (PuppetBone){LIMB_BODY, LIMB_ARM_L, LimbsDistance(pup, LIMB_BODY, LIMB_ARM_L)};
    pup->bones[BODY_ARM_R] = (PuppetBone){LIMB_BODY, LIMB_ARM_R, LimbsDistance(pup, LIMB_BODY, LIMB_ARM_R)};
    pup->bones[BODY_FOOT_L] = (PuppetBone){LIMB_BODY, LIMB_FOOT_L, LimbsDistance(pup, LIMB_BODY, LIMB_FOOT_L)};
    pup->bones[BODY_FOOT_R] = (PuppetBone){LIMB_BODY, LIMB_FOOT_R, LimbsDistance(pup, LIMB_BODY, LIMB_FOOT_R)};
}

// Create a puppet with limbs initialized in a default pose around the given start position.
Puppet CreatePuppet(const char *name, Color color, float radius, Vector2 startPos)
{
    Puppet pup = {0};
    strncpy(pup.name, name, sizeof(pup.name) - 1);
    pup.name[sizeof(pup.name) - 1] = '\0';
    pup.color = color;
    pup.radius = radius;
    pup.draggedLimb = -1;
    pup.physics = (PhysicsConfig){
        .gravity = DEFAULT_GRAVITY,
        .friction = DEFAULT_FRICTION,
        .bounce = DEFAULT_BOUNCE,
        .minBounce = DEFAULT_MIN_BOUNCE};
    pup.bounds = InitPuppetLimbs(&pup, startPos);
    InitPuppetBones(&pup);
    return pup;
}