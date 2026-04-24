#ifndef PUPPET_H
#define PUPPET_H
#include "config.h"

#include "raylib.h"

// -- Declarations --
enum PuppetType
{
    PUPPET_STANDARD,
};

typedef struct Puppet
{
    char name[32];
    enum PuppetType puppetType;
    Color color;
    float radius;
    Vector2 position;
    Vector2 velocity;
    bool isDragging;
    PhysicsConfig physics;
} Puppet;

// -- Functions --
Puppet CreatePuppet(const char *name, enum PuppetType type, Color color, float radius, Vector2 startPos);

#endif