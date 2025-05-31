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
#include "game.h"
#include "initialize.h"
#include "input.h"
#include "render.h"
#include "audio.h"
#include "powerups.h"
#include "resources.h" 
#include "scoreboard.h"

int main(int argc, char* argv[]) {
    // Initialize random seed
    srand(time(NULL));

    // Initialize Raylib
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Asteroids");
    SetTargetFPS(60);
    
    // Load and set the window icon
    Image icon = LoadImage(SHIP_TEXTURE_PATH);
    SetWindowIcon(icon);
    UnloadImage(icon); // Free the image data after setting icon
    
    // Load custom crosshair cursor
    Texture2D crosshairTexture = {0};
    bool hasCustomCursor = false;
    
    Image crosshairImage = LoadImage(CROSSHAIR_TEXTURE_PATH);
    if (crosshairImage.data != NULL) {
        crosshairTexture = LoadTextureFromImage(crosshairImage);
        UnloadImage(crosshairImage);
        hasCustomCursor = true;
    }
    
    // Start with default cursor for menu
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    
    // Create game state
    GameState gameState = {0}; // Initialize to zero
    initGameState(&gameState);
    
    // Initialize the resource manager
    initResources(&gameState);
    
    // Preload all textures for info screen and performance
    loadAllTextures(&gameState);
    
    // Setup buttons
    gameState.playButton = (Rectangle){
        WINDOW_WIDTH/2 - BUTTON_WIDTH/2,
        WINDOW_HEIGHT/2,
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    };
    
    gameState.resumeButton = (Rectangle){
        WINDOW_WIDTH/2 - BUTTON_WIDTH/2,
        WINDOW_HEIGHT/2 - (BUTTON_HEIGHT + 20),
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    };
    
    gameState.optionsButton = (Rectangle){
        WINDOW_WIDTH/2 - BUTTON_WIDTH/2,
        WINDOW_HEIGHT/2 + (BUTTON_HEIGHT + 20),
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    };
    
    gameState.quitButton = (Rectangle){
        WINDOW_WIDTH/2 - BUTTON_WIDTH/2,
        WINDOW_HEIGHT/2 + (BUTTON_HEIGHT + 90),
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    };
    
    gameState.backButton = (Rectangle){
        WINDOW_WIDTH/2 - BUTTON_WIDTH/2,
        WINDOW_HEIGHT - BUTTON_HEIGHT - 40,
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    };
    
    gameState.volumeSlider = (Rectangle){
        WINDOW_WIDTH/2 - SLIDER_WIDTH/2,
        WINDOW_HEIGHT/2,
        SLIDER_WIDTH,
        SLIDER_HEIGHT
    };

    gameState.musicVolumeSlider = (Rectangle){
        WINDOW_WIDTH/2 - SLIDER_WIDTH/2,
        WINDOW_HEIGHT/2 + 80,  
        SLIDER_WIDTH,
        SLIDER_HEIGHT
    };

    gameState.mainMenuButton = (Rectangle){
    WINDOW_WIDTH/2 - BUTTON_WIDTH/2,
    WINDOW_HEIGHT/2 + (BUTTON_HEIGHT + 20),
    BUTTON_WIDTH,
    BUTTON_HEIGHT
    };
    
    // Start with menu state
    gameState.screenState = MENU_STATE;
    gameState.windowFocused = true;
    
    // Load sounds
    loadSounds(&gameState);
    loadMusic(&gameState);

    // Start playing menu music
    PlayMusicStream(gameState.menuMusic);
    
    // Load high scores
    loadHighScores(&gameState);
    
    // Game loop
    while (!WindowShouldClose() && gameState.running) {
        float deltaTime = GetFrameTime();
        

        // Update music streaming for any active music
        if (gameState.musicLoaded && gameState.currentMusic != NULL) {
            UpdateMusicStream(*gameState.currentMusic);
        }

        // Check window focus status
        bool currentlyFocused = IsWindowFocused();
        if (gameState.windowFocused && !currentlyFocused && gameState.screenState == GAME_STATE) {
            gameState.screenState = PAUSE_STATE;
        }
        gameState.windowFocused = currentlyFocused;
        
        // Handle cursor switching based on game state
        static GameScreenState lastScreenState = MENU_STATE;
        if (gameState.screenState != lastScreenState) {
            if (gameState.screenState == GAME_STATE) {
                // Hide system cursor during gameplay to show custom crosshair
                if (hasCustomCursor) {
                    HideCursor();
                } else {
                    SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
                }
            } else {
                // Use default cursor for menus, pause, options, info, and game over
                ShowCursor();
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }
            lastScreenState = gameState.screenState;
        }
        
        switch (gameState.screenState) {
            case MENU_STATE:
                // Switch to menu music when returning to menu
                if (gameState.musicLoaded && gameState.currentMusic != &gameState.menuMusic) {
                    switchMusic(&gameState, &gameState.menuMusic);
                }
                handleMenuInput(&gameState);
                renderMenu(&gameState);
                break;
                
            case INFO_STATE:
                handleInfoInput(&gameState);
                renderInfo(&gameState);
                break;
                
            case GAME_STATE:
                // Start game with phase1 music unless we're already playing phase2
                if (gameState.musicLoaded) {
                    if (gameState.currentMusic == &gameState.menuMusic) {
                        // Coming from menu - switch to phase1
                        switchMusic(&gameState, &gameState.phase1Music);
                    } else if (gameState.currentWave >= TANK_START_WAVE && 
                               gameState.currentMusic == &gameState.phase1Music) {
                        // We've reached wave 5 - switch to phase2
                        switchMusic(&gameState, &gameState.phase2Music);
                    }
                    // Otherwise keep playing current phase music
                }
                
                gameState.fireTimer -= deltaTime; // Update fire cooldown timer
                gameState.shotgunFireTimer -= deltaTime; // Update shotgun cooldown timer
                gameState.grenadeFireTimer -= deltaTime; // Update grenade cooldown timer
                handleInput(&gameState);
                updateGame(&gameState, deltaTime);
                
                // Render game
                renderGame(&gameState);
                
                // Draw custom crosshair at mouse position during gameplay
                if (hasCustomCursor) {
                    Vector2 mousePos = GetMousePosition();
                    float crosshairSize = 32.0f; // Adjust size as needed
                    DrawTextureEx(crosshairTexture, 
                                (Vector2){mousePos.x - crosshairSize/2, mousePos.y - crosshairSize/2}, 
                                0.0f, crosshairSize/crosshairTexture.width, WHITE);
                }
                break;
                
            case PAUSE_STATE:
                // Keep playing current game music in pause
                handlePauseInput(&gameState);
                renderPause(&gameState);
                break;
                
            case OPTIONS_STATE:
                // Keep playing current music in options
                handleOptionsInput(&gameState);
                renderOptions(&gameState);
                break;
                
            case GAME_OVER_STATE:
                // Keep playing current phase music for game over
                handleGameOverInput(&gameState);
                renderGameOver(&gameState);
                break;
        }
    }
    
    // Clean up resources
    if (hasCustomCursor) {
        UnloadTexture(crosshairTexture);
    }
    
    // Unload all textures with the resource manager
    unloadAllTextures(&gameState);
    
    // Unload sound effects
    if (gameState.soundLoaded) {
        for (int i = 0; i < MAX_SOUNDS; i++) {
            UnloadSound(gameState.sounds[i]);
        }
        CloseAudioDevice();
    }

    // Unload music
    unloadMusic(&gameState);
    
    // Close Raylib
    CloseWindow();
    return 0;
}