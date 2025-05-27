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
    
    // Game loop
    while (!WindowShouldClose() && gameState.running) {
        float deltaTime = GetFrameTime();
        
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
                // Use default cursor for menus, pause, options, and game over
                ShowCursor();
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }
            lastScreenState = gameState.screenState;
        }
        
        switch (gameState.screenState) {
            case MENU_STATE:
                handleMenuInput(&gameState);
                renderMenu(&gameState);
                break;
                
            case GAME_STATE:
                gameState.fireTimer -= deltaTime; // Update fire cooldown timer
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
                handlePauseInput(&gameState);
                renderPause(&gameState);
                break;
                
            case OPTIONS_STATE:
                handleOptionsInput(&gameState);
                renderOptions(&gameState);
                break;
                
            case GAME_OVER_STATE:
                handleGameOverInput(&gameState);
                renderGameOver(&gameState);
                break;
        }
    }
    
    // Clean up resources
    if (hasCustomCursor) {
        UnloadTexture(crosshairTexture);
    }
    
    if (gameState.soundLoaded) {
        for (int i = 0; i < MAX_SOUNDS; i++) {
            UnloadSound(gameState.sounds[i]);
        }
        CloseAudioDevice();
    }
    
    // Unload ship texture
    if (gameState.ship.texture.id > 0) {
        UnloadTexture(gameState.ship.texture);
    }
    
    // Unload enemy textures
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (gameState.enemies[i].texture.id > 0) {
            UnloadTexture(gameState.enemies[i].texture);
        }
    }
    
    // Close Raylib
    CloseWindow();
    return 0;
}