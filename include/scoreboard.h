#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include "typedefs.h"

void loadHighScores(GameState* state);
void saveHighScores(GameState* state);
void addHighScore(GameState* state, int score, int wave);
void renderScoreboard(const GameState* state, int x, int y, int width);

#endif // SCOREBOARD_H