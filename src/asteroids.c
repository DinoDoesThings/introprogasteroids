#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <float.h>

// Include custom headers
#include "typedefs.h"
#include "config.h"

void createAsteroids(GameState* state, int count) {
    int created = 0;
    
    for (int i = 0; i < MAX_ASTEROIDS && created < count; i++) {
        if (!state->asteroids[i].base.active) {
            state->asteroids[i].base.active = true;
            state->asteroids[i].size = 3; // Start with large asteroids
            state->asteroids[i].base.radius = 20.0f * state->asteroids[i].size;
            
            // Place asteroid away from the ship but within map bounds
            do {
                state->asteroids[i].base.x = GetRandomValue(
                    state->asteroids[i].base.radius, 
                    MAP_WIDTH - state->asteroids[i].base.radius
                );
                
                state->asteroids[i].base.y = GetRandomValue(
                    state->asteroids[i].base.radius, 
                    MAP_HEIGHT - state->asteroids[i].base.radius
                );
                
            } while (sqrt(pow(state->asteroids[i].base.x - state->ship.base.x, 2) + 
                         pow(state->asteroids[i].base.y - state->ship.base.y, 2)) < 200);
            
            // Random velocity
            float angle = GetRandomValue(0, 359) * PI / 180.0f;
            float speed = 1.0f + GetRandomValue(0, 100) / 100.0f;
            state->asteroids[i].base.dx = sin(angle) * speed;
            state->asteroids[i].base.dy = -cos(angle) * speed;
            state->asteroids[i].base.angle = GetRandomValue(0, 359);
            
            created++;
        }
    }
}

void splitAsteroid(GameState* state, int index) {
    // Get the asteroid properties before deactivating it
    float x = state->asteroids[index].base.x;
    float y = state->asteroids[index].base.y;
    int size = state->asteroids[index].size;
    
    // Deactivate the hit asteroid
    state->asteroids[index].base.active = false;
    
    // If it's not the smallest size, split into two smaller asteroids
    if (size > 1) {
        int newSize = size - 1;
        int created = 0;
        
        for (int i = 0; i < MAX_ASTEROIDS && created < 2; i++) {
            if (!state->asteroids[i].base.active) {
                state->asteroids[i].base.active = true;
                state->asteroids[i].size = newSize;
                state->asteroids[i].base.radius = 20.0f * newSize;
                state->asteroids[i].base.x = x;
                state->asteroids[i].base.y = y;
                
                // Random velocity
                float angle = GetRandomValue(0, 359) * PI / 180.0f;
                float speed = 1.5f + GetRandomValue(0, 100) / 100.0f;
                state->asteroids[i].base.dx = sin(angle) * speed;
                state->asteroids[i].base.dy = -cos(angle) * speed;
                state->asteroids[i].base.angle = GetRandomValue(0, 359);
                
                created++;
            }
        }
    }
    
    // Decrement asteroids remaining when an asteroid is destroyed
    // but only if it's split into smaller ones or completely destroyed
    if (state->asteroids[index].size <= 1) {
        state->asteroidsRemaining--;
    }
}
