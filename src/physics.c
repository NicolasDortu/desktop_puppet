#include "physics.h"
#include "pet.h"

#include "raylib.h"

void ApplyPhysics(Pet *pet, int screenWidth, int screenHeight)
{
    pet->velocity.y += pet->physics.gravity; // apply gravity
    pet->position.x += pet->velocity.x;      // apply velocity
    pet->position.y += pet->velocity.y;
    pet->velocity.x *= pet->physics.friction; // apply friction

    // Bounce on screen edges
    if (pet->position.y + pet->radius > screenHeight)
    {
        pet->position.y = screenHeight - pet->radius;
        pet->velocity.y *= pet->physics.bounce;
    }
    if (pet->position.y - pet->radius < 0)
    {
        pet->position.y = pet->radius;
        pet->velocity.y *= pet->physics.bounce;
    }
    if (pet->position.x + pet->radius > screenWidth)
    {
        pet->position.x = screenWidth - pet->radius;
        pet->velocity.x *= pet->physics.bounce;
    }
    if (pet->position.x - pet->radius < 0)
    {
        pet->position.x = pet->radius;
        pet->velocity.x *= pet->physics.bounce;
    }

    // Stop tiny bounces
    if (pet->position.y + pet->radius >= screenHeight - 1 &&
        pet->velocity.y > 0 && pet->velocity.y < pet->physics.minBounceVel)
    {
        pet->velocity.y = 0;
    }
}
