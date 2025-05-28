#ifndef POWERUPS_H
#define POWERUPS_H

#include "typedefs.h"

void spawnHealthPowerup(GameState* state, float x, float y);
void spawnLifePowerup(GameState* state, float x, float y);
void spawnShotgunPowerup(GameState* state, float x, float y);
void spawnGrenadePowerup(GameState* state, float x, float y);
void updatePowerups(GameState* state, float deltaTime);
void unloadPowerupTextures(GameState* state);

#endif // POWERUPS_H