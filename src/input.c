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
    // Check for pause
    if (IsKeyPressed(KEY_P)) {
        state->screenState = PAUSE_STATE;
        return; // Skip other input handling when pausing
    }
    
    // Get mouse position in world space
    Vector2 mousePosition = GetScreenToWorld2D(GetMousePosition(), state->camera);
    
    // Calculate angle between ship and mouse cursor
    float dx = mousePosition.x - state->ship.base.x;
    float dy = mousePosition.y - state->ship.base.y;
    state->ship.base.angle = atan2(dx, -dy) * 180.0f / PI;
    
    // Handle mouse click for firing - changed to support holding the button
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && state->fireTimer <= 0) {
        fireWeapon(state);
        state->fireTimer = FIRE_RATE; // Set the cooldown timer
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
        state->ship.base.dx -= SHIP_ACCELERATION * cos(state->ship.base.angle * PI / 180.0f);
        state->ship.base.dy -= SHIP_ACCELERATION * sin(state->ship.base.angle * PI / 180.0f);
    }
    
    if (IsKeyDown(KEY_D)) {
        // Strafe right (perpendicular to the ship's facing direction)
        state->ship.base.dx += SHIP_ACCELERATION * cos(state->ship.base.angle * PI / 180.0f);
        state->ship.base.dy += SHIP_ACCELERATION * sin(state->ship.base.angle * PI / 180.0f);
    }
    
    if (IsKeyDown(KEY_S)) {
        // Decelerate/reverse
        state->ship.base.dx -= SHIP_ACCELERATION * sin(state->ship.base.angle * PI / 180.0f);
        state->ship.base.dy += SHIP_ACCELERATION * cos(state->ship.base.angle * PI / 180.0f);
    }

    if (IsKeyDown(KEY_R)) {
        // Reload ammo
        if (!state->isReloading && state->currentAmmo < MAX_AMMO) {
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

void handleMenuInput(GameState* state) {
    Vector2 mousePoint = GetMousePosition();
    
    // Check if mouse is over the Play button
    bool isMouseOverPlayButton = CheckCollisionPointRec(mousePoint, state->playButton);
    
    // Check if mouse is over the Options button
    bool isMouseOverOptionsButton = CheckCollisionPointRec(mousePoint, state->optionsButton);
    
    // Check if mouse is over the Quit button
    bool isMouseOverQuitButton = CheckCollisionPointRec(mousePoint, state->quitButton);
    
    // Change to game state if play button is clicked
    if (isMouseOverPlayButton && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state->screenState = GAME_STATE;
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
    
    // Handle volume slider interaction
    bool isMouseOverSlider = CheckCollisionPointRec(mousePoint, state->volumeSlider);
    
    // Start dragging
    if (isMouseOverSlider && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        state->isDraggingSlider = true;
    }
    
    // Stop dragging
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state->isDraggingSlider = false;
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