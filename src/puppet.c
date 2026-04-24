#include "config.h"
#include "puppet.h"

#include "string.h"

#include "raylib.h"

Puppet CreatePuppet(const char *name, enum PuppetType type, Color color, float radius, Vector2 startPos)
{
    Puppet pup;
    strncpy(pup.name, name, sizeof(pup.name) - 1);
    pup.name[sizeof(pup.name) - 1] = '\0';
    pup.puppetType = type;
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
    return pup;
}
