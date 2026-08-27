#ifndef INPUT_H
#define INPUT_H

#include "physics.h"

#include "raylib.h"

// =============================================================================
//  FUNCTIONS
// =============================================================================

// Left-click drag for any Body: grab a particle, drag it, throw it on release.
void DragBody(Body *body);

#endif