#ifndef COLLISIONS_H
#define COLLISIONS_H

// Custom headers
#include "typedefs.h"

bool checkCollision(GameObject* a, GameObject* b);
void checkPowerupCollisions(GameState* state);

#endif // COLLISIONS_H