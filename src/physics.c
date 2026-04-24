#include "physics.h"
#include "puppet.h"

#include "raylib.h"

void ApplyPhysics(Puppet *pup, int screenWidth, int screenHeight)
{
    if (pup->isDragging)
        return;

    pup->velocity.y += pup->physics.gravity; // apply gravity
    pup->position.x += pup->velocity.x;      // apply velocity
    pup->position.y += pup->velocity.y;
    pup->velocity.x *= pup->physics.friction; // apply friction

    // Bounce on screen edges
    if (pup->position.y + pup->radius > screenHeight)
    {
        pup->position.y = screenHeight - pup->radius;
        pup->velocity.y *= pup->physics.bounce;
    }
    if (pup->position.y - pup->radius < 0)
    {
        pup->position.y = pup->radius;
        pup->velocity.y *= pup->physics.bounce;
    }
    if (pup->position.x + pup->radius > screenWidth)
    {
        pup->position.x = screenWidth - pup->radius;
        pup->velocity.x *= pup->physics.bounce;
    }
    if (pup->position.x - pup->radius < 0)
    {
        pup->position.x = pup->radius;
        pup->velocity.x *= pup->physics.bounce;
    }

    // Stop tiny bounces
    if (pup->position.y + pup->radius >= screenHeight - 1 &&
        pup->velocity.y > 0 && pup->velocity.y < pup->physics.minBounceVel)
    {
        pup->velocity.y = 0;
    }
}
