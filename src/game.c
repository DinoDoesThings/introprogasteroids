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
#include "collisions.h"
#include "particles.h"
#include "enemies.h"
#include "asteroids.h"
#include "particles.h"
#include "powerups.h"
#include "initialize.h"
#include "resources.h"

void updateGame(GameState* state, float deltaTime) {
    // Update fire timers
    if (state->fireTimer > 0) {
        state->fireTimer -= deltaTime;
    }
    
    if (state->shotgunFireTimer > 0) {
        state->shotgunFireTimer -= deltaTime;
    }
    
    if (state->grenadeFireTimer > 0) {
        state->grenadeFireTimer -= deltaTime;
    }
    
    // Update wave message timer if active
    if (state->waveMessageTimer > 0) {
        state->waveMessageTimer -= deltaTime;
    }
    
    // Handle wave transitions
    if (state->inWaveTransition) {
        state->waveDelayTimer -= deltaTime;
        if (state->waveDelayTimer <= 0) {
            state->currentWave++; // Increment wave count
            startWave(state); // Start the new wave
            return; // Skip the rest of the update during wave transition
        }
    }
    
    // Update ship
    float newX = state->ship.base.x + state->ship.base.dx;
    float newY = state->ship.base.y + state->ship.base.dy;
    
    // Block ship at boundaries instead of teleporting
    if (newX - state->ship.base.radius >= 0 && newX + state->ship.base.radius <= MAP_WIDTH) {
        state->ship.base.x = newX;
    } else {
        state->ship.base.dx *= -0.5f; // Bounce with reduced speed
    }
    
    if (newY - state->ship.base.radius >= 0 && newY + state->ship.base.radius <= MAP_HEIGHT) {
        state->ship.base.y = newY;
    } else {
        state->ship.base.dy *= -0.5f; // Bounce with reduced speed
    }
    
    // Apply friction
    state->ship.base.dx *= FRICTION;
    state->ship.base.dy *= FRICTION;
    
    // Update camera to follow the ship
    state->camera.target = (Vector2){ state->ship.base.x, state->ship.base.y };
    
    // Handle reloading (only for normal weapon)
    if (state->isReloading) {
        // Cancel reload if weapon changed away from normal
        if (state->currentWeapon != WEAPON_NORMAL) {
            state->isReloading = false;
            state->reloadTimer = 0.0f;
        } else {
            state->reloadTimer -= deltaTime;
            if (state->reloadTimer <= 0) {
                state->isReloading = false;
                state->normalAmmo = MAX_AMMO;
                state->currentAmmo = MAX_AMMO; // Keep for compatibility
                
                // Play reload finish sound
                if (state->soundLoaded) {
                    PlaySound(state->sounds[SOUND_RELOAD_FINISH]);
                }
            }
        }
    }
    
    // Update bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (state->bullets[i].active) {
            state->bullets[i].x += state->bullets[i].dx;
            state->bullets[i].y += state->bullets[i].dy;
            
            // Check if bullet is out of bounds
            if (state->bullets[i].x < 0 || state->bullets[i].x > MAP_WIDTH || 
                state->bullets[i].y < 0 || state->bullets[i].y > MAP_HEIGHT) {
                state->bullets[i].active = false;
                continue;
            }
            
            // Check for collision with asteroids
            for (int j = 0; j < MAX_ASTEROIDS; j++) {
                if (state->asteroids[j].base.active && checkCollision((GameObject*)&state->bullets[i], &state->asteroids[j].base)) {
                    state->bullets[i].active = false;
                    splitAsteroid(state, j);
                    
                    // Update score based on asteroid size
                    state->score += (4 - state->asteroids[j].size) * 100;
                    
                    break;
                }
            }
        }
    }
    
    // Update asteroids
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (state->asteroids[i].base.active) {
            // Update position
            state->asteroids[i].base.x += state->asteroids[i].base.dx;
            state->asteroids[i].base.y += state->asteroids[i].base.dy;
            
            // Bounce asteroids off boundaries
            if (state->asteroids[i].base.x - state->asteroids[i].base.radius < 0) {
                state->asteroids[i].base.x = state->asteroids[i].base.radius;
                state->asteroids[i].base.dx *= -1;
            }
            else if (state->asteroids[i].base.x + state->asteroids[i].base.radius > MAP_WIDTH) {
                state->asteroids[i].base.x = MAP_WIDTH - state->asteroids[i].base.radius;
                state->asteroids[i].base.dx *= -1;
            }
            
            if (state->asteroids[i].base.y - state->asteroids[i].base.radius < 0) {
                state->asteroids[i].base.y = state->asteroids[i].base.radius;
                state->asteroids[i].base.dy *= -1;
            }
            else if (state->asteroids[i].base.y + state->asteroids[i].base.radius > MAP_HEIGHT) {
                state->asteroids[i].base.y = MAP_HEIGHT - state->asteroids[i].base.radius;
                state->asteroids[i].base.dy *= -1;
            }
            
            // Check for collision with ship
            if (checkCollision(&state->ship.base, &state->asteroids[i].base)) {
                // Apply damage based on asteroid size
                int damage = 0;
                switch (state->asteroids[i].size) {
                    case 3: damage = LARGE_ASTEROID_DAMAGE; break;
                    case 2: damage = MEDIUM_ASTEROID_DAMAGE; break;
                    case 1: damage = SMALL_ASTEROID_DAMAGE; break;
                }
                
                state->health -= damage;
                
                if (state->health <= 0) {
                    state->lives--;
                    if (state->lives <= 0) {
                        // Change to game over state instead of setting running to false
                        state->screenState = GAME_OVER_STATE;
                    } else {
                        resetShip(state);
                    }
                }
                
                // Destroy the asteroid that hit the ship
                splitAsteroid(state, i);
                break;
            }
        }
    }
    
    // Add collision detection between asteroids
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (state->asteroids[i].base.active) {
            for (int j = i + 1; j < MAX_ASTEROIDS; j++) {
                if (state->asteroids[j].base.active && 
                    checkCollision(&state->asteroids[i].base, &state->asteroids[j].base)) {
                    
                    // Calculate collision response
                    float dx = state->asteroids[j].base.x - state->asteroids[i].base.x;
                    float dy = state->asteroids[j].base.y - state->asteroids[i].base.y;
                    float distance = sqrt(dx * dx + dy * dy);
                    
                    // Avoid division by zero
                    if (distance == 0) distance = 0.01f;
                    
                    // Normalize direction
                    float nx = dx / distance;
                    float ny = dy / distance;
                    
                    // Calculate relative velocity
                    float dvx = state->asteroids[j].base.dx - state->asteroids[i].base.dx;
                    float dvy = state->asteroids[j].base.dy - state->asteroids[i].base.dy;
                    
                    // Calculate velocity along the normal direction
                    float velocityAlongNormal = dvx * nx + dvy * ny;
                    
                    // Don't resolve if velocities are separating
                    if (velocityAlongNormal > 0) continue;
                    
                    // Calculate restitution (bounciness)
                    float restitution = 0.8f;
                    
                    // Calculate impulse scalar
                    float impulse = -(1 + restitution) * velocityAlongNormal;
                    
                    // Calculate mass ratio based on size
                    float totalMass = state->asteroids[i].size + state->asteroids[j].size;
                    float massRatio1 = state->asteroids[j].size / totalMass;
                    float massRatio2 = state->asteroids[i].size / totalMass;
                    
                    // Apply impulse
                    float impulsex = impulse * nx;
                    float impulsey = impulse * ny;
                    
                    state->asteroids[i].base.dx -= impulsex * massRatio1;
                    state->asteroids[i].base.dy -= impulsey * massRatio1;
                    state->asteroids[j].base.dx += impulsex * massRatio2;
                    state->asteroids[j].base.dy += impulsey * massRatio2;
                    
                    // Prevent asteroids from getting stuck together by separating them
                    float overlap = state->asteroids[i].base.radius + state->asteroids[j].base.radius - distance;
                    if (overlap > 0) {
                        // Move asteroids apart based on their size/mass
                        state->asteroids[i].base.x -= nx * overlap * massRatio1 * 0.5f;
                        state->asteroids[i].base.y -= ny * overlap * massRatio1 * 0.5f;
                        state->asteroids[j].base.x += nx * overlap * massRatio2 * 0.5f;
                        state->asteroids[j].base.y += ny * overlap * massRatio2 * 0.5f;
                    }
                }
            }
        }
    }
    
    // Check if all asteroids are destroyed
    bool allAsteroidsDestroyed = true;
    state->asteroidsRemaining = 0;
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (state->asteroids[i].base.active) {
            allAsteroidsDestroyed = false;
            state->asteroidsRemaining++;
        }
    }
    
    // Check enemies too (all must be destroyed to complete wave)
    bool allEnemiesDestroyed = true;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (state->enemies[i].base.active) {
            allEnemiesDestroyed = false;
        }
    }
    
    // If all asteroids and enemies are destroyed, transition to next wave
    if (allAsteroidsDestroyed && allEnemiesDestroyed && !state->inWaveTransition) {
        state->inWaveTransition = true;
        state->waveDelayTimer = WAVE_DELAY;
        
        // Display wave complete message
        sprintf(state->waveMessage, "WAVE %d COMPLETE", state->currentWave);
        state->waveMessageTimer = WAVE_DELAY;
    }
    
    // Update particles
    updateParticles(state, deltaTime);
    
    // Update enemies
    updateEnemies(state, deltaTime);
    
    // Update powerups
    updatePowerups(state, deltaTime);
    
    // Check powerup collisions
    checkPowerupCollisions(state);
    
    // Update invulnerability timer and blinking effect
    if (state->isInvulnerable) {
        // Decrease invulnerability timer
        state->invulnerabilityTimer -= deltaTime;
        
        // Handle blinking effect
        state->blinkTimer -= deltaTime;
        if (state->blinkTimer <= 0.0f) {
            state->shipVisible = !state->shipVisible; // Toggle visibility
            state->blinkTimer = BLINK_FREQUENCY;
        }
        
        // End invulnerability when timer expires
        if (state->invulnerabilityTimer <= 0.0f) {
            state->isInvulnerable = false;
            state->shipVisible = true; // Make sure ship is visible when invulnerability ends
        }
    }
}

void preloadTextures(GameState* state) {
    // This function is now much simpler as I delegated the work to the resource manager
    loadAllTextures(state);
}