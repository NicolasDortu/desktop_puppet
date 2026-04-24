#ifndef PET_H
#define PET_H
#include "config.h"

#include "raylib.h"

enum PetType
{
    PET_DOG,
    PET_CAT,
    PET_BIRD,
    PET_FISH
};

typedef struct
{
    char name[32];
    enum PetType petType;
    Color color;
    float radius;
    Vector2 position;
    Vector2 velocity;
    bool isDragging;
    PhysicsConfig physics;
} Pet;

Pet CreatePet(const char *name, enum PetType type, Color color, float radius, Vector2 startPos);

#endif