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

void fireWeapon(GameState* state) {
    // Don't fire if reloading or out of ammo
    if (state->isReloading || state->currentAmmo <= 0) {
        return;
    }
    
    // Get mouse position in world space
    Vector2 mousePosition = GetScreenToWorld2D(GetMousePosition(), state->camera);
    
    // Calculate direction from ship to mouse cursor
    float dx = mousePosition.x - state->ship.base.x;
    float dy = mousePosition.y - state->ship.base.y;
    float length = sqrt(dx * dx + dy * dy);
    
    // Normalize the direction
    if (length > 0) {
        dx /= length;
        dy /= length;
    }
    
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!state->bullets[i].active) {
            state->bullets[i].active = true;
            
            // Start the bullet at the ship's position
            state->bullets[i].x = state->ship.base.x;
            state->bullets[i].y = state->ship.base.y;
            state->bullets[i].radius = 2.0f;
            
            // Set bullet velocity toward the mouse cursor
            state->bullets[i].dx = dx * BULLET_SPEED;
            state->bullets[i].dy = dy * BULLET_SPEED;
            
            // Play shooting sound effect at reduced volume when rapidly firing
            if (state->soundLoaded) {
                // If we're firing rapidly, reduce volume slightly to prevent audio overload
                float volumeMultiplier = state->fireTimer < 0.05f ? 0.6f : 1.0f;
                
                // Set the specific sound volume for this shot
                SetSoundVolume(state->sounds[SOUND_SHOOT], state->soundVolume * volumeMultiplier);
                
                // Play the sound
                PlaySound(state->sounds[SOUND_SHOOT]);
                
                // Reset to normal volume for next time
                SetSoundVolume(state->sounds[SOUND_SHOOT], state->soundVolume);
            }
            
            // Decrease ammo
            state->currentAmmo--;
            
            // Start reloading if out of ammo
            if (state->currentAmmo <= 0) {
                state->isReloading = true;
                state->reloadTimer = RELOAD_TIME;
                
                // Play reload start sound
                if (state->soundLoaded) {
                    PlaySound(state->sounds[SOUND_RELOAD_START]);
                }
            }
            
            // Only fire one bullet at a time
            break;
        }
    }
}