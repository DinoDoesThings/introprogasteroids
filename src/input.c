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
#include "particles.h"
#include "playership.h"
#include "audio.h"
#include "initialize.h"

void handleInput(GameState* state) {
    // BUG FIX: Add pause functionality
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
        state->screenState = PAUSE_STATE;
        return; // Exit early to prevent other input processing
    }
    
    // Get mouse position in world space for ship aiming
    Vector2 mousePosition = GetScreenToWorld2D(GetMousePosition(), state->camera);
    
    // Calculate direction from ship to mouse cursor
    float dx = mousePosition.x - state->ship.base.x;
    float dy = mousePosition.y - state->ship.base.y;
    
    // Update ship angle to point toward cursor
    state->ship.base.angle = atan2(dx, -dy) * 180.0f / PI;
    
    // Handle mouse click for firing with weapon-specific timing
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        // Check weapon-specific fire rate
        if (state->currentWeapon == WEAPON_SHOTGUN) {
            // Shotgun has its own cooldown timer
            if (state->shotgunFireTimer <= 0) {
                fireWeapon(state);
            }
        } else if (state->currentWeapon == WEAPON_GRENADE) {
            // Grenade has its own cooldown timer
            if (state->grenadeFireTimer <= 0) {
                fireWeapon(state);
            }
        } else {
            // Normal weapon uses the general fire timer
            if (state->fireTimer <= 0) {
                fireWeapon(state);
                state->fireTimer = FIRE_RATE; // Set the cooldown timer for normal weapon
            }
        }
    }
    
    // Debug mode keybindings - only active when debug mode is on
    if (state->Debug) {
        // F4: Kill all asteroids
        if (IsKeyPressed(KEY_F4)) {
            int destroyedCount = 0;
            for (int i = 0; i < MAX_ASTEROIDS; i++) {
                if (state->asteroids[i].base.active) {
                    state->asteroids[i].base.active = false;
                    destroyedCount++;
                }
            }
            printf("Debug: Destroyed %d asteroids\n", destroyedCount);
        }
        
        // F5: Kill all enemies
        if (IsKeyPressed(KEY_F5)) {
            int destroyedCount = 0;
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (state->enemies[i].base.active) {
                    // Create explosion particles for visual feedback
                    for (int k = 0; k < 20; k++) {
                        for (int p = 0; p < MAX_PARTICLES; p++) {
                            if (!state->particles[p].active) {
                                state->particles[p].active = true;
                                state->particles[p].life = PARTICLE_LIFETIME;
                                state->particles[p].position.x = state->enemies[i].base.x;
                                state->particles[p].position.y = state->enemies[i].base.y;
                                
                                float particleAngle = GetRandomValue(0, 359) * PI / 180.0f;
                                float particleSpeed = PARTICLE_SPEED * GetRandomValue(50, 150) / 100.0f;
                                state->particles[p].velocity.x = cos(particleAngle) * particleSpeed;
                                state->particles[p].velocity.y = sin(particleAngle) * particleSpeed;
                                
                                state->particles[p].radius = GetRandomValue(2, 6);
                                state->particles[p].color = (Color){ 
                                    GetRandomValue(200, 255), 
                                    GetRandomValue(50, 100),
                                    GetRandomValue(0, 50),
                                    255
                                };
                                break;
                            }
                        }
                    }
                    
                    state->enemies[i].base.active = false;
                    destroyedCount++;
                }
            }
            printf("Debug: Destroyed %d enemies\n", destroyedCount);
        }
        
        // F6: Skip to next wave
        if (IsKeyPressed(KEY_F6)) {
            // Force transition to the next wave
            state->inWaveTransition = true;
            state->waveDelayTimer = WAVE_DELAY;
            
            // Display wave complete message
            sprintf(state->waveMessage, "WAVE %d COMPLETE", state->currentWave);
            state->waveMessageTimer = WAVE_DELAY;
            
            printf("Debug: Skipping to wave %d\n", state->currentWave + 1);
        }
    }
    
    // Continuous key presses
    if (IsKeyDown(KEY_W)) {
        // Accelerate ship in the direction it's facing
        state->ship.base.dx += SHIP_ACCELERATION * sin(state->ship.base.angle * PI / 180.0f);
        state->ship.base.dy -= SHIP_ACCELERATION * cos(state->ship.base.angle * PI / 180.0f);
        
        // Emit particles when moving forward
        emitParticles(state, 2);
        
        // Limit speed
        float speed = sqrt(state->ship.base.dx * state->ship.base.dx + state->ship.base.dy * state->ship.base.dy);
        if (speed > SHIP_MAX_SPEED) {
            state->ship.base.dx = (state->ship.base.dx / speed) * SHIP_MAX_SPEED;
            state->ship.base.dy = (state->ship.base.dy / speed) * SHIP_MAX_SPEED;
        }
    }
    
    if (IsKeyDown(KEY_A)) {
        // Strafe left (perpendicular to the ship's facing direction)
        state->ship.base.dx -= SHIP_ACCELERATION * 0.7f * cos(state->ship.base.angle * PI / 180.0f);
        state->ship.base.dy -= SHIP_ACCELERATION * 0.7f * sin(state->ship.base.angle * PI / 180.0f);
    }
    
    if (IsKeyDown(KEY_D)) {
        // Strafe right (perpendicular to the ship's facing direction)
        state->ship.base.dx += SHIP_ACCELERATION * 0.7f * cos(state->ship.base.angle * PI / 180.0f);
        state->ship.base.dy += SHIP_ACCELERATION * 0.7f * sin(state->ship.base.angle * PI / 180.0f);
    }
    
    if (IsKeyDown(KEY_S)) {
        // Decelerate/reverse
        state->ship.base.dx -= SHIP_ACCELERATION * 0.5f * sin(state->ship.base.angle * PI / 180.0f);
        state->ship.base.dy += SHIP_ACCELERATION * 0.5f * cos(state->ship.base.angle * PI / 180.0f);
    }

    if (IsKeyPressed(KEY_R)) {
        // Reload ammo (only for normal weapon)
        if (!state->isReloading && state->normalAmmo < MAX_AMMO && state->currentWeapon == WEAPON_NORMAL) {
            state->isReloading = true;
            state->reloadTimer = RELOAD_TIME;
            
            // Play reload start sound
            if (state->soundLoaded) {
                PlaySound(state->sounds[SOUND_RELOAD_START]);
            }
        }
    }

    if (IsKeyPressed(KEY_F3)) {
        // Toggle debug mode
        state->Debug = !state->Debug;
    }
}

void updateMenuAsteroids(GameState* state, float deltaTime) {
    // First update positions
    for (int i = 0; i < MAX_MENU_ASTEROIDS; i++) {
        if (state->menuAsteroids[i].active) {
            // Update position
            state->menuAsteroids[i].x += state->menuAsteroids[i].dx * 60 * deltaTime;
            state->menuAsteroids[i].y += state->menuAsteroids[i].dy * 60 * deltaTime;
            
            // Update rotation
            state->menuAsteroids[i].angle += state->menuAsteroids[i].rotationSpeed * 60 * deltaTime;
            
            // Check if asteroid is completely off screen
            float buffer = state->menuAsteroids[i].radius * 2; // Extra buffer
            if (state->menuAsteroids[i].x < -buffer || 
                state->menuAsteroids[i].x > WINDOW_WIDTH + buffer ||
                state->menuAsteroids[i].y < -buffer || 
                state->menuAsteroids[i].y > WINDOW_HEIGHT + buffer) {
                
                // Reset this asteroid to come in from a random edge
                int side = GetRandomValue(0, 3); // 0: top, 1: right, 2: bottom, 3: left
                
                switch (side) {
                    case 0: // Top
                        state->menuAsteroids[i].x = GetRandomValue(0, WINDOW_WIDTH);
                        state->menuAsteroids[i].y = -state->menuAsteroids[i].radius;
                        break;
                    case 1: // Right
                        state->menuAsteroids[i].x = WINDOW_WIDTH + state->menuAsteroids[i].radius;
                        state->menuAsteroids[i].y = GetRandomValue(0, WINDOW_HEIGHT);
                        break;
                    case 2: // Bottom
                        state->menuAsteroids[i].x = GetRandomValue(0, WINDOW_WIDTH);
                        state->menuAsteroids[i].y = WINDOW_HEIGHT + state->menuAsteroids[i].radius;
                        break;
                    case 3: // Left
                        state->menuAsteroids[i].x = -state->menuAsteroids[i].radius;
                        state->menuAsteroids[i].y = GetRandomValue(0, WINDOW_HEIGHT);
                        break;
                }
                
                // New random velocity toward approximate center of screen
                float targetX = WINDOW_WIDTH/2 + GetRandomValue(-200, 200);
                float targetY = WINDOW_HEIGHT/2 + GetRandomValue(-100, 100);
                float angle = atan2(targetY - state->menuAsteroids[i].y, 
                                  targetX - state->menuAsteroids[i].x);
                
                // Set random speed
                float speed = GetRandomValue(30, 100) / 100.0f;
                state->menuAsteroids[i].dx = cos(angle) * speed;
                state->menuAsteroids[i].dy = sin(angle) * speed;
                
                // Random rotation speed
                state->menuAsteroids[i].rotationSpeed = (GetRandomValue(0, 100) - 50) / 300.0f;
            }
        }
    }
    
    // Check for collisions between menu asteroids (similar to game asteroid collision logic)
    for (int i = 0; i < MAX_MENU_ASTEROIDS; i++) {
        if (state->menuAsteroids[i].active) {
            for (int j = i + 1; j < MAX_MENU_ASTEROIDS; j++) {
                if (state->menuAsteroids[j].active) {
                    // Check for collision
                    float dx = state->menuAsteroids[j].x - state->menuAsteroids[i].x;
                    float dy = state->menuAsteroids[j].y - state->menuAsteroids[i].y;
                    float distance = sqrtf(dx * dx + dy * dy);
                    
                    if (distance < state->menuAsteroids[i].radius + state->menuAsteroids[j].radius) {
                        // Collision detected - calculate collision response
                        
                        // Avoid division by zero
                        if (distance == 0) distance = 0.01f;
                        
                        // Normalize direction
                        float nx = dx / distance;
                        float ny = dy / distance;
                        
                        // Calculate relative velocity
                        float dvx = state->menuAsteroids[j].dx - state->menuAsteroids[i].dx;
                        float dvy = state->menuAsteroids[j].dy - state->menuAsteroids[i].dy;
                        
                        // Calculate velocity along the normal direction
                        float velocityAlongNormal = dvx * nx + dvy * ny;
                        
                        // Don't resolve if velocities are separating
                        if (velocityAlongNormal > 0) continue;
                        
                        // Calculate restitution (bounciness)
                        float restitution = 0.8f;
                        
                        // Calculate impulse scalar
                        float impulse = -(1 + restitution) * velocityAlongNormal;
                        
                        // Calculate mass ratio based on radius
                        float totalMass = state->menuAsteroids[i].radius + state->menuAsteroids[j].radius;
                        float massRatio1 = state->menuAsteroids[j].radius / totalMass;
                        float massRatio2 = state->menuAsteroids[i].radius / totalMass;
                        
                        // Apply impulse
                        float impulsex = impulse * nx;
                        float impulsey = impulse * ny;
                        
                        state->menuAsteroids[i].dx -= impulsex * massRatio1;
                        state->menuAsteroids[i].dy -= impulsey * massRatio1;
                        state->menuAsteroids[j].dx += impulsex * massRatio2;
                        state->menuAsteroids[j].dy += impulsey * massRatio2;
                        
                        // Prevent asteroids from getting stuck together by separating them
                        float overlap = state->menuAsteroids[i].radius + state->menuAsteroids[j].radius - distance;
                        if (overlap > 0) {
                            // Move asteroids apart based on their size/mass
                            state->menuAsteroids[i].x -= nx * overlap * massRatio1 * 0.5f;
                            state->menuAsteroids[i].y -= ny * overlap * massRatio1 * 0.5f;
                            state->menuAsteroids[j].x += nx * overlap * massRatio2 * 0.5f;
                            state->menuAsteroids[j].y += ny * overlap * massRatio2 * 0.5f;
                        }
                    }
                }
            }
        }
    }
}

// Update this function to call our new menu asteroids update function
void handleMenuInput(GameState* state) {
    // Update menu background asteroids
    updateMenuAsteroids(state, GetFrameTime());
    
    Vector2 mousePoint = GetMousePosition();
    
    // Check if mouse is over the Play button
    bool isMouseOverPlayButton = CheckCollisionPointRec(mousePoint, state->playButton);
    
    // Check if mouse is over the Options button
    bool isMouseOverOptionsButton = CheckCollisionPointRec(mousePoint, state->optionsButton);
    
    // Check if mouse is over the Quit button
    bool isMouseOverQuitButton = CheckCollisionPointRec(mousePoint, state->quitButton);
    
    // Change to info state if play button is clicked (instead of directly to game)
    if (isMouseOverPlayButton && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state->screenState = INFO_STATE;
    }
    
    // Change to options state if options button is clicked
    if (isMouseOverOptionsButton && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state->previousScreenState = MENU_STATE;  // Remember we came from menu
        state->screenState = OPTIONS_STATE;
    }
    
    // Exit the game if quit button is clicked
    if (isMouseOverQuitButton && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state->running = false;
    }
}

void handlePauseInput(GameState* state) {
    Vector2 mousePoint = GetMousePosition();
    
    // Check if mouse is over the Resume button
    bool isMouseOverResumeButton = CheckCollisionPointRec(mousePoint, state->resumeButton);
    
    // Check if mouse is over the Options button
    bool isMouseOverOptionsButton = CheckCollisionPointRec(mousePoint, state->optionsButton);
    
    // Check if mouse is over the Quit button
    bool isMouseOverQuitButton = CheckCollisionPointRec(mousePoint, state->quitButton);
    
    // Resume the game if resume button is clicked
    if (isMouseOverResumeButton && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state->screenState = GAME_STATE;
    }
    
    // Go to options if options button is clicked
    if (isMouseOverOptionsButton && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state->previousScreenState = PAUSE_STATE;  // Remember we came from pause
        state->screenState = OPTIONS_STATE;
    }
    
    // Exit the game if quit button is clicked
    if (isMouseOverQuitButton && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state->running = false;
    }
    
    // Also check for P key to resume
    if (IsKeyPressed(KEY_P)) {
        state->screenState = GAME_STATE;
    }
}

void handleOptionsInput(GameState* state) {
    Vector2 mousePoint = GetMousePosition();
    
    // Check if mouse is over the Back button
    bool isMouseOverBackButton = CheckCollisionPointRec(mousePoint, state->backButton);
    
    // Return to previous screen (menu or pause) if back button is clicked
    if (isMouseOverBackButton && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state->screenState = state->previousScreenState;  // Return to wherever we came from
    }
    
    Rectangle adjustedMusicSliderRect = {
        state->musicVolumeSlider.x,
        state->musicVolumeSlider.y + 40, 
        state->musicVolumeSlider.width,
        state->musicVolumeSlider.height
    };
    
    // Handle volume slider interaction
    bool isMouseOverSlider = CheckCollisionPointRec(mousePoint, state->volumeSlider);
    bool isMouseOverMusicSlider = CheckCollisionPointRec(mousePoint, adjustedMusicSliderRect);
    
    // Start dragging
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (isMouseOverSlider) {
            state->isDraggingSlider = true;
            state->isDraggingMusicSlider = false;
        } else if (isMouseOverMusicSlider) {
            state->isDraggingSlider = false;
            state->isDraggingMusicSlider = true;
        }
    }
    
    // Stop dragging
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state->isDraggingSlider = false;
        state->isDraggingMusicSlider = false;
    }
    
    // Update slider if dragging
    if (state->isDraggingSlider) {
        // Calculate volume based on mouse position
        float relativeX = mousePoint.x - state->volumeSlider.x;
        float newVolume = relativeX / state->volumeSlider.width;
        
        // Clamp volume between 0 and 1
        newVolume = newVolume < 0.0f ? 0.0f : newVolume;
        newVolume = newVolume > 1.0f ? 1.0f : newVolume;
        
        updateSoundVolume(state, newVolume);
    }

     // Update music slider if dragging
    if (state->isDraggingMusicSlider) {
        // Calculate volume based on mouse position
        float relativeX = mousePoint.x - state->musicVolumeSlider.x;
        float newVolume = relativeX / state->musicVolumeSlider.width;
        
        // Clamp volume between 0 and 1
        newVolume = newVolume < 0.0f ? 0.0f : newVolume;
        newVolume = newVolume > 1.0f ? 1.0f : newVolume;
        
        updateMusicVolume(state, newVolume);
    }
}

void handleGameOverInput(GameState* state) {
    Vector2 mousePoint = GetMousePosition();
    
    // Check if mouse is over the main menu button
    bool isMouseOverMainMenuButton = CheckCollisionPointRec(mousePoint, state->mainMenuButton);
    
    // Return to menu if button is clicked
    if (isMouseOverMainMenuButton && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        // Reset game data but preserve sound settings
        resetGameData(state);
        state->screenState = MENU_STATE;
    }
}

void handleInfoInput(GameState* state) {
    // Press Z to close info screen and start the game
    if (IsKeyPressed(KEY_Z)) {
        state->screenState = GAME_STATE;
    }
    
    // Also allow ESC to go back to menu
    if (IsKeyPressed(KEY_ESCAPE)) {
        state->screenState = MENU_STATE;
    }
}