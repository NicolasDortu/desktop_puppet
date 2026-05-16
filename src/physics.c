#include "physics.h"
#include "puppet.h"

#include "raylib.h"

// -- Physics Implementation --

// Update the positions of the limbs connected by bones to maintain their lengths.
static void updateBones(Puppet *pup)
{
    for (int i = 0; i < BONE_COUNT; i++)
    {
        PuppetBone *bone = &pup->bones[i];
        PuppetLimb *a = &pup->limbs[bone->limb1];
        PuppetLimb *b = &pup->limbs[bone->limb2];

        float dx = b->pos.x - a->pos.x;
        float dy = b->pos.y - a->pos.y;
        float distance = LimbsDistance(pup, bone->limb1, bone->limb2);
        if (distance < 1e-6f)
            continue;

        float percent = (distance - bone->length) / distance * 0.5f;
        float offsetX = dx * percent;
        float offsetY = dy * percent;

        if (pup->draggedLimb != (int)bone->limb1)
        {
            a->pos.x += offsetX;
            a->pos.y += offsetY;
        }
        if (pup->draggedLimb != (int)bone->limb2)
        {
            b->pos.x -= offsetX;
            b->pos.y -= offsetY;
        }
    }
}

float minX = 1e30f, minY = 1e30f, maxX = -1e30f, maxY = -1e30f; // first declaration of puppet bounds
// Verlet integration applied independently to each limb.
void ApplyPhysics(Puppet *pup, int screenWidth, int screenHeight)
{
    for (int i = 0; i < LIMB_COUNT; i++)
    {
        PuppetLimb *limb = &pup->limbs[i];

        if (pup->draggedLimb != i)
        {

            // Verlet: derive velocity from the position delta, then integrate.
            float vx = (limb->pos.x - limb->oldPos.x) * pup->physics.friction;
            float vy = (limb->pos.y - limb->oldPos.y) * pup->physics.friction;

            // Minimum bounce treshold before stopping the puppet from moving
            float threshold = pup->physics.minBounce;
            if (vx * vx + vy * vy < threshold * threshold)
            {
                vx = 0.0f;
                vy = 0.0f;
            }

            limb->oldPos = limb->pos;
            limb->pos.x += vx;
            limb->pos.y += vy;
            limb->pos.y += pup->physics.gravity;

            // Bounce against screen edges using the limb's own radius.
            if (limb->pos.y + limb->radius > screenHeight)
            {
                limb->pos.y = screenHeight - limb->radius;
                limb->oldPos.y = limb->pos.y - vy * pup->physics.bounce;
            }
            if (limb->pos.y - limb->radius < 0)
            {
                limb->pos.y = limb->radius;
                limb->oldPos.y = limb->pos.y - vy * pup->physics.bounce;
            }
            if (limb->pos.x + limb->radius > screenWidth)
            {
                limb->pos.x = screenWidth - limb->radius;
                limb->oldPos.x = limb->pos.x - vx * pup->physics.bounce;
            }
            if (limb->pos.x - limb->radius < 0)
            {
                limb->pos.x = limb->radius;
                limb->oldPos.x = limb->pos.x - vx * pup->physics.bounce;
            }
        }
    }

    // Update bones
    for (int iter = 0; iter < 3; iter++)
        updateBones(pup);

    // Return the bounds
    float minX = 1e30f, minY = 1e30f, maxX = -1e30f, maxY = -1e30f; // reset
    for (int i = 0; i < LIMB_COUNT; i++)
    {
        PuppetLimb *limb = &pup->limbs[i];
        float lx = limb->pos.x - limb->radius;
        float ly = limb->pos.y - limb->radius;
        float rx = limb->pos.x + limb->radius;
        float ry = limb->pos.y + limb->radius;
        if (lx < minX)
            minX = lx;
        if (ly < minY)
            minY = ly;
        if (rx > maxX)
            maxX = rx;
        if (ry > maxY)
            maxY = ry;
    }
    pup->bounds = (PuppetBounds){minX, minY, maxX - minX, maxY - minY};
}