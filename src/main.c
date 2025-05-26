#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <string.h>

// Include
#include "typedef.h"
#include "config.h"
#include "game.h"
#include "initialize.h"

int main(int argc, char* argv[]) {
    // Initialize random seed
    srand(time(NULL));
    
    // Create game state
    GameState gameState = {0}; // Initialize to zero
    
    // Initialize Raylib
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Asteroids");
    SetTargetFPS(60);
    
    // Initialize game state
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
        
        switch (gameState.screenState) {
            case MENU_STATE:
                handleMenuInput(&gameState);
                renderMenu(&gameState);
                break;
                
            case GAME_STATE:
                gameState.fireTimer -= deltaTime; // Update fire cooldown timer
                handleInput(&gameState);
                updateGame(&gameState, deltaTime);
                renderGame(&gameState);
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
    
    // Clean up sound resources
    if (gameState.soundLoaded) {
        for (int i = 0; i < MAX_SOUNDS; i++) {
            UnloadSound(gameState.sounds[i]);
        }
        CloseAudioDevice();
    }
    
    // Close Raylib
    CloseWindow();
    return 0;
}