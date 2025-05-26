#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <string.h>

// Custom headers
#include "typedefs.h"
#include "config.h"

void updateParticles(GameState* state, float deltaTime) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (state->particles[i].active) {
            // Update particle position
            state->particles[i].position.x += state->particles[i].velocity.x;
            state->particles[i].position.y += state->particles[i].velocity.y;
            
            // Update particle life
            state->particles[i].life -= deltaTime;
            
            // Make particles shrink over time
            state->particles[i].radius = 3.0f * (state->particles[i].life / PARTICLE_LIFETIME);
            
            // Fade out particles over time
            state->particles[i].color.a = (unsigned char)(255.0f * (state->particles[i].life / PARTICLE_LIFETIME));
            
            // Deactivate expired particles
            if (state->particles[i].life <= 0) {
                state->particles[i].active = false;
            }
        }
    }
}

void emitParticles(GameState* state, int count) {
    // Calculate ship rear position (opposite to the front)
    float radians = state->ship.base.angle * PI / 180.0f;
    float rearX = state->ship.base.x - sin(radians) * state->ship.base.radius * 1.2f;
    float rearY = state->ship.base.y + cos(radians) * state->ship.base.radius * 1.2f;
    
    // Emit particles
    for (int i = 0; i < count; i++) {
        // Find an inactive particle
        for (int j = 0; j < MAX_PARTICLES; j++) {
            if (!state->particles[j].active) {
                state->particles[j].active = true;
                state->particles[j].life = PARTICLE_LIFETIME;
                
                // Set particle position at the ship's rear
                state->particles[j].position.x = rearX;
                state->particles[j].position.y = rearY;
                
                // Add slight randomness to position
                state->particles[j].position.x += GetRandomValue(-3, 3);
                state->particles[j].position.y += GetRandomValue(-3, 3);
                
                // Set particle velocity in the opposite direction of the ship
                float particleAngle = radians + PI + GetRandomValue(-30, 30) * PI / 180.0f;
                state->particles[j].velocity.x = sin(particleAngle) * PARTICLE_SPEED;
                state->particles[j].velocity.y = -cos(particleAngle) * PARTICLE_SPEED;
                
                // Set particle appearance
                state->particles[j].radius = GetRandomValue(2, 5);
                
                // Different colors for visual interest - orange/red/yellow for engine exhaust
                int colorChoice = GetRandomValue(0, 2);
                if (colorChoice == 0)
                    state->particles[j].color = (Color){ 255, 120, 0, 255 };  // Orange
                else if (colorChoice == 1)
                    state->particles[j].color = (Color){ 255, 50, 0, 255 };   // Red-orange
                else
                    state->particles[j].color = (Color){ 255, 215, 0, 255 };  // Yellow
                
                break;
            }
        }
    }
}