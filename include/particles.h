#ifndef PARTICLES_H
#define PARTICLES_H

// Custom headers
#include "typedefs.h"

void updateParticles(GameState* state, float deltaTime);
void emitParticles(GameState* state, int count);

#endif // PARTICLES_H