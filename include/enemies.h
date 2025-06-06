#ifndef ENEMIES_H
#define ENEMIES_H

// Custom headers
#include "typedefs.h"

void spawnEnemy(GameState* state, EnemyType type);
void updateEnemies(GameState* state, float deltaTime);
void fireEnemyWeapon(GameState* state, Enemy* enemy);
void explodeGrenade(GameState* state, int grenadeIndex);
float getEnemyTextureScale(EnemyType type);

#endif // ENEMIES_H