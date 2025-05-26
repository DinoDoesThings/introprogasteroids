#ifndef ASTEROIDS_H
#define ASTEROIDS_H

// Custom headers
#include "typedefs.h"

void createAsteroids(GameState* state, int count);
void splitAsteroid(GameState* state, int index);

#endif // ASTEROIDS_H