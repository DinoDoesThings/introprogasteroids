#ifndef INPUT_H
#define INPUT_H

// Custom headers
#include "typedefs.h"

void handleMenuInput(GameState* state);
void handlePauseInput(GameState* state);
void handleOptionsInput(GameState* state);
void handleGameOverInput(GameState* state);
void handleInfoInput(GameState* state);
void handleInput(GameState* state);

#endif // INPUT_H