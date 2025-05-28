#ifndef INITIALIZE_H
#define INITIALIZE_H

// custom headers
#include "typedefs.h"

void startWave(GameState* state);
void initGameState(GameState* state);
void initMenuAsteroids(GameState* state);
void resetGameData(GameState* state);
void resetShip(GameState* state);

#endif // INITIALIZE_H