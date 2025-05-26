#ifndef INPUT_H
#define INPUT_H

// Custom headers
#include "typedefs.h"

void handleInput(GameState* state);
void handleMenuInput(GameState* state);
void handlePauseInput(GameState* state);
void handleOptionsInput(GameState* state);
void handleGameOverInput(GameState* state);

#endif // INPUT_H