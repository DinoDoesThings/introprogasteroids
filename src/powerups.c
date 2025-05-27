#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "typedefs.h"
#include "config.h"
#include "audio.h"
#include "collisions.h"
#include "resources.h" 


void spawnHealthPowerup(GameState* state, float x, float y) {
    // Check drop chance
    if (GetRandomValue(1, 100) > HEALTH_POWERUP_DROP_CHANCE) {
        return; // No powerup dropped
    }
    
    // Find an inactive powerup slot
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!state->powerups[i].base.active) {
            state->powerups[i].base.active = true;
            state->powerups[i].base.x = x;
            state->powerups[i].base.y = y;
            state->powerups[i].base.radius = 15.0f;
            state->powerups[i].base.dx = 0;
            state->powerups[i].base.dy = 0;
            state->powerups[i].base.angle = 0;
            state->powerups[i].type = POWERUP_HEALTH;
            state->powerups[i].lifetime = POWERUP_LIFETIME;
            state->powerups[i].pulseTimer = 0.0f;
            
            // Try to load health powerup texture
            state->powerups[i].texture = loadTextureOnce(HEALTH_POWERUP_TEXTURE_PATH);
            
            break;
        }
    }
}

void spawnShotgunPowerup(GameState* state, float x, float y) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!state->powerups[i].base.active) {
            state->powerups[i].base.active = true;
            state->powerups[i].base.x = x;
            state->powerups[i].base.y = y;
            state->powerups[i].base.radius = 15.0f;
            state->powerups[i].base.angle = 0.0f;
            state->powerups[i].base.dx = 0.0f;
            state->powerups[i].base.dy = 0.0f;
            state->powerups[i].type = POWERUP_SHOTGUN;
            state->powerups[i].lifetime = 15.0f; // 15 second lifetime
            state->powerups[i].pulseTimer = 0.0f;
            
            // Try to load shotgun powerup texture
            state->powerups[i].texture = loadTextureOnce(SHOTGUN_POWERUP_TEXTURE_PATH);
            
            break;
        }
    }
}

void spawnGrenadePowerup(GameState* state, float x, float y) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!state->powerups[i].base.active) {
            state->powerups[i].base.active = true;
            state->powerups[i].base.x = x;
            state->powerups[i].base.y = y;
            state->powerups[i].base.radius = 15.0f;
            state->powerups[i].base.angle = 0.0f;
            state->powerups[i].base.dx = 0.0f;
            state->powerups[i].base.dy = 0.0f;
            state->powerups[i].type = POWERUP_GRENADE;
            state->powerups[i].lifetime = 15.0f; // 15 second lifetime
            state->powerups[i].pulseTimer = 0.0f;
            
            // Try to load grenade powerup texture
            state->powerups[i].texture = loadTextureOnce(GRENADE_POWERUP_TEXTURE_PATH);
            
            break;
        }
    }
}

void updatePowerups(GameState* state, float deltaTime) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (state->powerups[i].base.active) {
            Powerup* powerup = &state->powerups[i];
            
            // Update pulse timer for visual effect
            powerup->pulseTimer += deltaTime * 4.0f;
            
            // Update lifetime
            powerup->lifetime -= deltaTime;
            if (powerup->lifetime <= 0.0f) {
                powerup->base.active = false;
                if (powerup->texture.id > 0) {
                    UnloadTexture(powerup->texture);
                    powerup->texture.id = 0;
                }
                continue;
            }
            
            // Check for collision with player
            if (checkCollision(&state->ship.base, &powerup->base)) {
                if (powerup->type == POWERUP_HEALTH) {
                    // Heal player
                    state->health += 50;
                    if (state->health > MAX_HEALTH) {
                        state->health = MAX_HEALTH;
                    }
                } else if (powerup->type == POWERUP_SHOTGUN) {
                    // Give player shotgun weapon
                    state->currentWeapon = WEAPON_SHOTGUN;
                    state->shotgunAmmo = SHOTGUN_MAX_AMMO;
                    // Cancel any ongoing reload when switching weapons
                    if (state->isReloading) {
                        state->isReloading = false;
                        state->reloadTimer = 0.0f;
                    }
                } else if (powerup->type == POWERUP_GRENADE) {
                    // Give player grenade weapon
                    state->currentWeapon = WEAPON_GRENADE;
                    state->grenadeAmmo = GRENADE_MAX_AMMO;
                    // Cancel any ongoing reload when switching weapons
                    if (state->isReloading) {
                        state->isReloading = false;
                        state->reloadTimer = 0.0f;
                    }
                }
                
                // Deactivate powerup
                powerup->base.active = false;
                if (powerup->texture.id > 0) {
                    UnloadTexture(powerup->texture);
                    powerup->texture.id = 0;
                }
            }
        }
    }
}

// This function can now be simplified as the resource manager handles the real unloading
void unloadPowerupTextures(GameState* state) {
    // Just reset the texture references, actual unloading happens in the resource manager
    for (int i = 0; i < MAX_POWERUPS; i++) {
        state->powerups[i].texture = (Texture2D){0};
    }
}