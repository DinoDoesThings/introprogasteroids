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
#include "enemies.h"

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

// Function to get particle configuration for each enemy type
EnemyParticleConfig getEnemyParticleConfig(EnemyType type) {
    EnemyParticleConfig config;
    
    switch (type) {
        case ENEMY_TANK:
            config.minRadius = TANK_PARTICLE_MIN_RADIUS;
            config.maxRadius = TANK_PARTICLE_MAX_RADIUS;
            config.randomRange = TANK_PARTICLE_RANDOM_RANGE;
            config.rearOffset = TANK_PARTICLE_REAR_OFFSET;
            config.speedMultiplier = TANK_PARTICLE_SPEED_MULTIPLIER;
            // Red/orange exhaust for tank enemies
            config.colors[0] = (Color){ 255, 100, 50, 255 };  // Red-orange
            config.colors[1] = (Color){ 255, 50, 20, 255 };   // Reddish
            config.colors[2] = (Color){ 255, 150, 50, 255 };  // Orange
            break;
            
        case ENEMY_SCOUT:
            config.minRadius = SCOUT_PARTICLE_MIN_RADIUS;
            config.maxRadius = SCOUT_PARTICLE_MAX_RADIUS;
            config.randomRange = SCOUT_PARTICLE_RANDOM_RANGE;
            config.rearOffset = SCOUT_PARTICLE_REAR_OFFSET;
            config.speedMultiplier = SCOUT_PARTICLE_SPEED_MULTIPLIER;
            // Blue/cyan exhaust for scout enemies
            config.colors[0] = (Color){ 50, 150, 255, 255 };  // Blue
            config.colors[1] = (Color){ 0, 200, 255, 255 };   // Cyan
            config.colors[2] = (Color){ 100, 200, 255, 255 }; // Light blue
            break;
            
        default:
            // Default configuration (same as scout)
            config.minRadius = SCOUT_PARTICLE_MIN_RADIUS;
            config.maxRadius = SCOUT_PARTICLE_MAX_RADIUS;
            config.randomRange = SCOUT_PARTICLE_RANDOM_RANGE;
            config.rearOffset = SCOUT_PARTICLE_REAR_OFFSET;
            config.speedMultiplier = SCOUT_PARTICLE_SPEED_MULTIPLIER;
            config.colors[0] = (Color){ 255, 255, 255, 255 };  // White
            config.colors[1] = (Color){ 200, 200, 200, 255 };  // Light gray
            config.colors[2] = (Color){ 150, 150, 150, 255 };  // Gray
            break;
    }
    
    return config;
}

void emitEnemyThrustParticles(GameState* state, Enemy* enemy, int count) {
    // Get particle configuration for this enemy type
    EnemyParticleConfig config = getEnemyParticleConfig(enemy->type);
    
    // Calculate enemy rear position (opposite to facing direction)
    float radians = enemy->base.angle * PI / 180.0f;
    
    // Get the texture scale for this enemy type to position particles correctly
    float textureScale = getEnemyTextureScale(enemy->type);
    
    // Calculate effective visual radius based on texture scale
    float visualRadius = enemy->base.radius;
    if (enemy->texture.id > 0) {
        // Use texture dimensions if available
        float textureRadius = (enemy->texture.height * textureScale) / 2.0f;
        visualRadius = fmaxf(textureRadius * 0.8f, enemy->base.radius);
    }
    
    // Position particles at the rear of the visual ship using config offset
    float rearX = enemy->base.x - sin(radians) * visualRadius * config.rearOffset;
    float rearY = enemy->base.y + cos(radians) * visualRadius * config.rearOffset;
    
    // Emit particles
    for (int i = 0; i < count; i++) {
        // Find an inactive particle
        for (int j = 0; j < MAX_PARTICLES; j++) {
            if (!state->particles[j].active) {
                state->particles[j].active = true;
                state->particles[j].life = PARTICLE_LIFETIME * 0.7f;
                
                // Set particle position at the enemy's rear
                state->particles[j].position.x = rearX;
                state->particles[j].position.y = rearY;
                
                // Add randomness using config range
                state->particles[j].position.x += GetRandomValue(-config.randomRange, config.randomRange);
                state->particles[j].position.y += GetRandomValue(-config.randomRange, config.randomRange);
                
                // Set particle velocity in the opposite direction of the enemy
                float particleAngle = radians + PI + GetRandomValue(-20, 20) * PI / 180.0f;
                float particleSpeed = PARTICLE_SPEED * config.speedMultiplier;
                state->particles[j].velocity.x = sin(particleAngle) * particleSpeed;
                state->particles[j].velocity.y = -cos(particleAngle) * particleSpeed;
                
                // Set particle size using config
                state->particles[j].radius = GetRandomValue(config.minRadius, config.maxRadius);
                
                // Set color using config colors
                int colorChoice = GetRandomValue(0, 2);
                state->particles[j].color = config.colors[colorChoice];
                
                break;
            }
        }
    }
}