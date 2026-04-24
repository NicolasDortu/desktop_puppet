#include "config.h"
#include "pet.h"

#include "string.h"

#include "raylib.h"

Pet CreatePet(const char *name, enum PetType type, Color color, float radius, Vector2 startPos)
{
    Pet pet;
    strncpy(pet.name, name, sizeof(pet.name) - 1);
    pet.name[sizeof(pet.name) - 1] = '\0';
    pet.petType = type;
    pet.color = color;
    pet.radius = radius;
    pet.position = startPos;
    pet.velocity = (Vector2){0.0f, 0.0f};
    pet.isDragging = false;
    pet.physics = (PhysicsConfig){
        .gravity = DEFAULT_GRAVITY,
        .friction = DEFAULT_FRICTION,
        .bounce = DEFAULT_BOUNCE,
        .minBounceVel = DEFAULT_MIN_BOUNCE_VEL};
    return pet;
}
