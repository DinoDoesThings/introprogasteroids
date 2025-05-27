#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <float.h>

// custom headers
#include "typedefs.h"

bool checkCollision(GameObject* a, GameObject* b) {
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float distance = sqrt(dx * dx + dy * dy);
    
    return distance < (a->radius + b->radius);
}

void checkPowerupCollisions(GameState* state) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (state->powerups[i].base.active) {
            if (checkCollision(&state->ship.base, &state->powerups[i].base)) {
                // Apply powerup effect
                switch (state->powerups[i].type) {
                    case POWERUP_HEALTH:
                        state->health += HEALTH_POWERUP_HEAL_AMOUNT;
                        if (state->health > MAX_HEALTH) {
                            state->health = MAX_HEALTH;
                        }
                        
                        // Play pickup sound 
                        if (state->soundLoaded) {
                            PlaySound(state->sounds[SOUND_POWERUP_PICKUP]);
                        }
                        break;
                }
                
                // Remove the powerup
                state->powerups[i].base.active = false;
            }
        }
    }
}