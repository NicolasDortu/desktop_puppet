#ifndef INPUT_H
#define INPUT_H

#include "physics.h"

#include "raylib.h"

// =============================================================================
//  FUNCTIONS
// =============================================================================

// -- Mouse helpers --

Vector2 GetScreenMousePos (void);
bool    MouseInsideCircle (Vector2 mouseScreen, Vector2 center, float radius);

// -- Generic body dragging --

int     GetHoveredParticle(const Body *body, Vector2 mouseScreenPos);
void    DragBody          (Body *body);

#endif