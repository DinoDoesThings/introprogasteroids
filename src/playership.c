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
    // Check ammo based on current weapon
    int currentAmmo;
    if (state->currentWeapon == WEAPON_SHOTGUN) {
        currentAmmo = state->shotgunAmmo;
    } else if (state->currentWeapon == WEAPON_GRENADE) {
        currentAmmo = state->grenadeAmmo;
    } else {
        currentAmmo = state->normalAmmo;
    }
    
    // Don't fire if reloading or out of ammo
    if (state->isReloading || currentAmmo <= 0) {
        return;
    }
    
    // Check weapon-specific fire rate limitations
    if (state->currentWeapon == WEAPON_SHOTGUN && state->shotgunFireTimer > 0) {
        return; // Shotgun still on cooldown
    }
    
    if (state->currentWeapon == WEAPON_GRENADE && state->grenadeFireTimer > 0) {
        return; // Grenade still on cooldown
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
    
    if (state->currentWeapon == WEAPON_SHOTGUN) {
        // Fire shotgun pellets
        int pelletsToFire = SHOTGUN_PELLETS;
        int pelletsSpawned = 0;
        
        for (int i = 0; i < MAX_BULLETS && pelletsSpawned < pelletsToFire; i++) {
            if (!state->bullets[i].active) {
                state->bullets[i].active = true;
                
                // Start the bullet at the ship's position
                state->bullets[i].x = state->ship.base.x;
                state->bullets[i].y = state->ship.base.y;
                state->bullets[i].radius = 2.0f;
                
                // Calculate spread angle for this pellet
                float spreadRange = SHOTGUN_SPREAD_ANGLE * PI / 180.0f;
                float pelletSpread = ((float)pelletsSpawned / (pelletsToFire - 1) - 0.5f) * spreadRange;
                
                // Apply spread to direction
                float spreadDx = dx * cos(pelletSpread) - dy * sin(pelletSpread);
                float spreadDy = dx * sin(pelletSpread) + dy * cos(pelletSpread);
                
                // Set bullet velocity with spread
                state->bullets[i].dx = spreadDx * BULLET_SPEED;
                state->bullets[i].dy = spreadDy * BULLET_SPEED;
                
                pelletsSpawned++;
            }
        }
        
        // Decrease shotgun ammo
        state->shotgunAmmo--;
        
        // Set shotgun cooldown timer
        state->shotgunFireTimer = SHOTGUN_FIRE_RATE;
        
        // Switch back to normal weapon when out of shotgun ammo
        if (state->shotgunAmmo <= 0) {
            state->currentWeapon = WEAPON_NORMAL;
            // Give full ammo when switching back to normal weapon
            state->normalAmmo = MAX_AMMO;
            state->currentAmmo = MAX_AMMO; // Keep for compatibility
            // Cancel any ongoing reload since we now have full ammo
            state->isReloading = false;
            state->reloadTimer = 0.0f;
        }
    } else if (state->currentWeapon == WEAPON_GRENADE) {
        // Fire grenade (use enemy bullet system but mark as player bullet)
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            if (!state->enemyBullets[i].base.active) {
                state->enemyBullets[i].base.active = true;
                
                // Start the grenade at the ship's position
                state->enemyBullets[i].base.x = state->ship.base.x;
                state->enemyBullets[i].base.y = state->ship.base.y;
                state->enemyBullets[i].base.radius = 6.0f; // Larger grenade
                
                // Set grenade properties
                state->enemyBullets[i].damage = PLAYER_GRENADE_EXPLOSION_DAMAGE;
                state->enemyBullets[i].type = BULLET_GRENADE;
                state->enemyBullets[i].timer = PLAYER_GRENADE_TIMER;
                state->enemyBullets[i].hasExploded = false;
                state->enemyBullets[i].isPlayerBullet = true; // Mark as player bullet
                
                // Set grenade velocity toward the mouse cursor (slower than bullets)
                state->enemyBullets[i].base.dx = dx * BULLET_SPEED * 0.7f;
                state->enemyBullets[i].base.dy = dy * BULLET_SPEED * 0.7f;
                
                break;
            }
        }
        
        // Decrease grenade ammo
        state->grenadeAmmo--;
        
        // Set grenade cooldown timer
        state->grenadeFireTimer = GRENADE_FIRE_RATE;
        
        // Switch back to normal weapon when out of grenade ammo
        if (state->grenadeAmmo <= 0) {
            state->currentWeapon = WEAPON_NORMAL;
            // Give full ammo when switching back to normal weapon
            state->normalAmmo = MAX_AMMO;
            state->currentAmmo = MAX_AMMO; // Keep for compatibility
            // Cancel any ongoing reload since we now have full ammo
            state->isReloading = false;
            state->reloadTimer = 0.0f;
        }
    } else {
        // Fire normal weapon
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
                
                // Only fire one bullet at a time for normal weapon
                break;
            }
        }
        
        // Decrease normal ammo
        state->normalAmmo--;
        
        // Start reloading if out of ammo
        if (state->normalAmmo <= 0) {
            state->isReloading = true;
            state->reloadTimer = RELOAD_TIME;
            
            // Play reload start sound
            if (state->soundLoaded) {
                PlaySound(state->sounds[SOUND_RELOAD_START]);
            }
        }
    }
    
    // Play shooting sound effect
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
}