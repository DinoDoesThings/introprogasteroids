#ifndef RENDER_H
#define RENDER_H

// Custom headers
#include "typedefs.h"

void renderGame(const GameState* state);
void renderGameObject(const GameObject* obj, int sides, const GameState* state);
void renderParticles(const GameState* state);
void renderEnemies(const GameState* state);
void renderPowerups(const GameState* state);
void renderMenu(const GameState* state);
void renderInfo(const GameState* state);
void renderPause(const GameState* state);
void renderOptions(const GameState* state);
void renderGameOver(const GameState* state);

#endif // RENDER_H