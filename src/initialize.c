#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <string.h>

// Include
#include "typedefs.h"
#include "config.h"
#include "audio.h"
#include "asteroids.h"

void startWave(GameState* state) {
    // Clear any remaining asteroids and enemies
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        state->asteroids[i].base.active = false;
    }
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        state->enemies[i].base.active = false;
    }
    
    // Calculate asteroid count based on wave
    int asteroidCount = BASE_ASTEROID_COUNT + (state->currentWave - 1) * ASTEROID_INCREMENT;
    asteroidCount = (asteroidCount <= MAX_ASTEROIDS) ? asteroidCount : MAX_ASTEROIDS;
    
    // Create asteroids
    createAsteroids(state, asteroidCount);
    
    // Count active asteroids
    state->asteroidsRemaining = 0;
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (state->asteroids[i].base.active) {
            state->asteroidsRemaining++;
        }
    }
    
    // Reset enemy spawn timer based on wave
    if (state->currentWave >= SCOUT_START_WAVE) {
        // Enemies should spawn faster in higher waves
        float spawnTime = ENEMY_SPAWN_TIME - (state->currentWave - SCOUT_START_WAVE) * 1.0f;
        // Ensure spawn time doesn't go below minimum
        state->enemySpawnTimer = (spawnTime > 3.0f) ? spawnTime : 3.0f;
    } else {
        // No enemies should spawn before their start wave
        state->enemySpawnTimer = FLT_MAX;
    }
    
    // Reset enemy spawn counter for new wave
    state->enemiesSpawnedThisWave = 0;
    state->EnemySpawnComplete = 0;  // Reset the enemy quota flag
    
    // Calculate max enemies for this wave (only applicable for waves with enemies)
    if (state->currentWave >= SCOUT_START_WAVE) {
        // BASE_ENEMY_MAX for wave 3, increment by ENEMY_INCREMENT each wave after
        state->maxEnemiesThisWave = BASE_ENEMY_MAX + 
            (state->currentWave - SCOUT_START_WAVE) * ENEMY_INCREMENT;
    } else {
        state->maxEnemiesThisWave = 0;  // No enemies allowed before SCOUT_START_WAVE
    }
    
    // Display wave message
    sprintf(state->waveMessage, "WAVE %d", state->currentWave);
    state->waveMessageTimer = 3.0f;  // Show message for 3 seconds
    
    // Debug output to verify wave transition
    if (state->Debug) {
        printf("Starting Wave %d with %d asteroids\n", state->currentWave, asteroidCount);
    }
    
    state->inWaveTransition = false;
}

void initMenuAsteroids(GameState* state) {
    for (int i = 0; i < MAX_MENU_ASTEROIDS; i++) {
        state->menuAsteroids[i].active = true;
        
        // Randomize asteroid size
        state->menuAsteroids[i].radius = GetRandomValue(15, 40);
        
        // Start positions - either off-screen from left, right, top or bottom
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
        
        // Random velocity toward center of screen, but not directly
        float angle = atan2(WINDOW_HEIGHT/2 - state->menuAsteroids[i].y, 
                           WINDOW_WIDTH/2 - state->menuAsteroids[i].x);
        
        // Add some randomness to direction
        angle += GetRandomValue(-30, 30) * PI / 180.0f;
        
        // Set random speed
        float speed = GetRandomValue(30, 100) / 100.0f;
        state->menuAsteroids[i].dx = cos(angle) * speed;
        state->menuAsteroids[i].dy = sin(angle) * speed;
        
        // Random initial angle and rotation speed
        state->menuAsteroids[i].angle = GetRandomValue(0, 359);
        state->menuAsteroids[i].rotationSpeed = (GetRandomValue(0, 100) - 50) / 300.0f;
    }
}

void initGameState(GameState* state) {
    // Initialize to default values
    state->score = 0;
    state->lives = 3;
    state->health = MAX_HEALTH;
    state->reloadTimer = 0.0f;
    state->isReloading = false;
    state->fireTimer = 0.0f;
    state->running = true;
    state->Debug = false;
    state->screenState = MENU_STATE;
    state->previousScreenState = MENU_STATE;  // Default previous state
    state->soundLoaded = false;
    state->musicLoaded = false;
    state->currentMusic = NULL;  // Initialize the pointer to NULL
    state->currentWave = 1;
    state->asteroidsRemaining = 0;
    state->waveDelayTimer = 0.0f;
    state->inWaveTransition = false;
    state->waveMessage[0] = '\0';
    state->waveMessageTimer = 0.0f;
    state->soundVolume = 0.5f;  // Default to half volume for sound
    state->musicVolume = 0.2f;  // Default to 20% volume for music
    state->isDraggingSlider = false;
    state->isDraggingMusicSlider = false;
    state->invulnerabilityTimer = 0.0f;
    state->isInvulnerable = false;
    state->blinkTimer = 0.0f;
    state->shipVisible = true;
    
    // Load sounds
    loadSounds(state);
    
    // Load ship texture
    state->ship.texture = LoadTexture(SHIP_TEXTURE_PATH);
    
    // Initialize camera
    state->camera.zoom = 1.0f;
    state->camera.rotation = 0.0f;
    state->camera.offset = (Vector2){ WINDOW_WIDTH/2, WINDOW_HEIGHT/2 };
    
    // Initialize ship
    state->ship.base.x = MAP_WIDTH / 2;
    state->ship.base.y = MAP_HEIGHT / 2;
    state->ship.base.dx = 0;
    state->ship.base.dy = 0;
    state->ship.base.angle = 0;
    state->ship.base.radius = 15.0f;
    state->ship.base.active = true;
    state->ship.rotationSpeed = ROTATION_SPEED;
    
    // Initialize bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        state->bullets[i].active = false;
    }
    
    // Initialize asteroids
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        state->asteroids[i].base.active = false;
    }
    
    // Initialize particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        state->particles[i].active = false;
    }
    
    // Initialize enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        state->enemies[i].base.active = false;
    }
    
    // Initialize enemy bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        state->enemyBullets[i].base.active = false;
    }
    
    // Initialize powerups
    for (int i = 0; i < MAX_POWERUPS; i++) {
        state->powerups[i].base.active = false;
        state->powerups[i].texture = (Texture2D){0};
    }
    
    // Initialize weapon system
    state->currentWeapon = WEAPON_NORMAL;
    state->normalAmmo = MAX_AMMO;
    state->shotgunAmmo = 0;
    state->grenadeAmmo = 0;
    state->fireTimer = 0.0f;
    state->shotgunFireTimer = 0.0f;
    state->grenadeFireTimer = 0.0f;
    
    state->enemySpawnTimer = ENEMY_SPAWN_TIME;
    
    // Initialize menu background asteroids
    initMenuAsteroids(state);
    
    // Start the first wave instead of directly creating asteroids
    startWave(state);
}

void resetGameData(GameState* state) {
    // Reset game-specific data without touching audio
    state->score = 0;
    state->lives = 3;
    state->health = MAX_HEALTH;
    state->reloadTimer = 0.0f;
    state->isReloading = false;
    state->fireTimer = 0.0f;
    state->Debug = false;
    state->currentWave = 1;
    state->asteroidsRemaining = 0;
    state->waveDelayTimer = 0.0f;
    state->inWaveTransition = false;
    state->waveMessage[0] = '\0';
    state->waveMessageTimer = 0.0f;
    state->isDraggingSlider = false;
    state->invulnerabilityTimer = 0.0f;
    state->isInvulnerable = false;
    state->blinkTimer = 0.0f;
    state->shipVisible = true;
    
    // Initialize camera
    state->camera.zoom = 1.0f;
    state->camera.rotation = 0.0f;
    state->camera.offset = (Vector2){ WINDOW_WIDTH/2, WINDOW_HEIGHT/2 };
    
    // Initialize ship
    state->ship.base.x = MAP_WIDTH / 2;
    state->ship.base.y = MAP_HEIGHT / 2;
    state->ship.base.dx = 0;
    state->ship.base.dy = 0;
    state->ship.base.angle = 0;
    state->ship.base.radius = 15.0f;
    state->ship.base.active = true;
    state->ship.rotationSpeed = ROTATION_SPEED;
    
    // Initialize bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        state->bullets[i].active = false;
    }
    
    // Initialize asteroids
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        state->asteroids[i].base.active = false;
    }
    
    // Initialize particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        state->particles[i].active = false;
    }
    
    // Initialize enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        state->enemies[i].base.active = false;
    }
    
    // Initialize enemy bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        state->enemyBullets[i].base.active = false;
    }
    
    // Clear powerups
    for (int i = 0; i < MAX_POWERUPS; i++) {
        state->powerups[i].base.active = false;
    }
    
    // Reset weapon system
    state->currentWeapon = WEAPON_NORMAL;
    state->normalAmmo = MAX_AMMO;
    state->shotgunAmmo = 0;
    state->grenadeAmmo = 0;
    state->isReloading = false;
    state->reloadTimer = 0.0f;
    state->fireTimer = 0.0f;
    state->shotgunFireTimer = 0.0f;
    state->grenadeFireTimer = 0.0f;
    
    state->enemySpawnTimer = ENEMY_SPAWN_TIME;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        state->enemies[i].base.active = false;
    }
    
    // Start the first wave
    startWave(state);
}

void resetShip(GameState* state) {
    state->ship.base.x = MAP_WIDTH / 2;
    state->ship.base.y = MAP_HEIGHT / 2;
    state->ship.base.dx = 0;
    state->ship.base.dy = 0;
    state->ship.base.angle = 0;
    state->ship.base.radius = 15.0f;
    state->ship.base.active = true;
    state->normalAmmo = MAX_AMMO; // Reset normal ammo
    state->health = MAX_HEALTH; // Reset health when respawning
    state->isReloading = false; // Stop reloading
    // Activate invulnerability when ship respawns
    state->isInvulnerable = true;
    state->invulnerabilityTimer = INVULNERABILITY_TIME;
    state->shipVisible = true;
    state->blinkTimer = 0.0f;
}

