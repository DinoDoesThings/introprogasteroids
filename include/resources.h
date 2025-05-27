#ifndef RESOURCES_H
#define RESOURCES_H

#include "raylib.h"
#include "typedefs.h"

// Initialize the resource manager
void initResources(GameState* state);

// Load all game textures
void loadAllTextures(GameState* state);

// Unload all game textures
void unloadAllTextures(GameState* state);

// Load a specific texture only if not already loaded
Texture2D loadTextureOnce(const char* path);

// Utility to check if a texture is loaded
bool isTextureLoaded(Texture2D texture);

#endif // RESOURCES_H