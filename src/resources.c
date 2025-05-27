#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Custom headers
#include "typedefs.h"
#include "config.h"
#include "resources.h"

// Tracks all loaded textures for proper cleanup
typedef struct {
    Texture2D* textures;
    int count;
    int capacity;
} TextureTracker;

static TextureTracker textureTracker = {0};

void initResources(GameState* state) {
    // Initialize texture tracker
    textureTracker.capacity = 20; // Start with space for 20 textures
    textureTracker.textures = (Texture2D*)malloc(sizeof(Texture2D) * textureTracker.capacity);
    textureTracker.count = 0;
    
    if (textureTracker.textures == NULL) {
        printf("Error: Failed to allocate memory for texture tracker\n");
        exit(1);
    }
}

// Helper function to track a texture
static void trackTexture(Texture2D texture) {
    if (texture.id == 0) return; // Don't track invalid textures
    
    // Check if we need to expand capacity
    if (textureTracker.count >= textureTracker.capacity) {
        int newCapacity = textureTracker.capacity * 2;
        Texture2D* newTextures = (Texture2D*)realloc(textureTracker.textures, 
                                                    sizeof(Texture2D) * newCapacity);
        
        if (newTextures == NULL) {
            printf("Error: Failed to expand texture tracker capacity\n");
            return;
        }
        
        textureTracker.textures = newTextures;
        textureTracker.capacity = newCapacity;
    }
    
    // Add the texture to our tracker
    textureTracker.textures[textureTracker.count++] = texture;
}

bool isTextureLoaded(Texture2D texture) {
    return texture.id != 0;
}

Texture2D loadTextureOnce(const char* path) {
    Texture2D texture = {0};
    
    // Try to load the texture
    texture = LoadTexture(path);
    
    // Track the texture if successfully loaded
    if (texture.id != 0) {
        trackTexture(texture);
    } else {
        printf("Warning: Failed to load texture: %s\n", path);
    }
    
    return texture;
}

void loadAllTextures(GameState* state) {
    // Ship texture
    if (!isTextureLoaded(state->ship.texture)) {
        state->ship.texture = loadTextureOnce(SHIP_TEXTURE_PATH);
    }
    
    // Load enemy textures
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!state->enemies[i].base.active && state->enemies[i].texture.id == 0) {
            // Load scout texture
            state->enemies[i].type = ENEMY_SCOUT;
            state->enemies[i].texture = loadTextureOnce(SCOUT_TEXTURE_PATH);
            state->enemies[i].base.active = false;
            break;
        }
    }
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!state->enemies[i].base.active && state->enemies[i].texture.id == 0) {
            // Load tank texture
            state->enemies[i].type = ENEMY_TANK;
            state->enemies[i].texture = loadTextureOnce(TANK_TEXTURE_PATH);
            state->enemies[i].base.active = false;
            break;
        }
    }
    
    // Load powerup textures
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!state->powerups[i].base.active && state->powerups[i].texture.id == 0) {
            state->powerups[i].type = POWERUP_HEALTH;
            state->powerups[i].texture = loadTextureOnce(HEALTH_POWERUP_TEXTURE_PATH);
            state->powerups[i].base.active = false;
            break;
        }
    }
    
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!state->powerups[i].base.active && state->powerups[i].texture.id == 0) {
            state->powerups[i].type = POWERUP_SHOTGUN;
            state->powerups[i].texture = loadTextureOnce(SHOTGUN_POWERUP_TEXTURE_PATH);
            state->powerups[i].base.active = false;
            break;
        }
    }
    
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!state->powerups[i].base.active && state->powerups[i].texture.id == 0) {
            state->powerups[i].type = POWERUP_GRENADE;
            state->powerups[i].texture = loadTextureOnce(GRENADE_POWERUP_TEXTURE_PATH);
            state->powerups[i].base.active = false;
            break;
        }
    }
    
    // Asteroid textures would be loaded here if you decide to use textures for them
}

void unloadAllTextures(GameState* state) {
    // Unload all tracked textures
    for (int i = 0; i < textureTracker.count; i++) {
        if (textureTracker.textures[i].id != 0) {
            UnloadTexture(textureTracker.textures[i]);
        }
    }
    
    // Reset tracker
    textureTracker.count = 0;
    
    // Free the tracker memory
    free(textureTracker.textures);
    textureTracker.textures = NULL;
    textureTracker.capacity = 0;
    
    // Reset texture references in game state
    
    // Ship texture
    state->ship.texture = (Texture2D){0};
    
    // Enemy textures
    for (int i = 0; i < MAX_ENEMIES; i++) {
        state->enemies[i].texture = (Texture2D){0};
    }
    
    // Powerup textures
    for (int i = 0; i < MAX_POWERUPS; i++) {
        state->powerups[i].texture = (Texture2D){0};
    }
}