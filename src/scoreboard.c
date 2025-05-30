#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Custom headers
#include "typedefs.h"
#include "config.h"

// Load high scores from file
void loadHighScores(GameState* state) {
    FILE* file = fopen(SCORE_FILE_PATH, "r");
    
    // Initialize scores
    state->scoreCount = 0;
    for (int i = 0; i < MAX_HIGH_SCORES; i++) {
        state->highScores[i].score = 0;
        state->highScores[i].wave = 0;
        strcpy(state->highScores[i].date, "");
    }
    
    if (file == NULL) {
        // File doesn't exist yet, which is fine for first launch
        return;
    }
    
    // Read scores from file
    while (!feof(file) && state->scoreCount < MAX_HIGH_SCORES) {
        int result = fscanf(file, "%d,%d,%11s\n", 
                           &state->highScores[state->scoreCount].score,
                           &state->highScores[state->scoreCount].wave,
                           state->highScores[state->scoreCount].date);
        
        // Check if we read all 3 values successfully
        if (result == 3) {
            state->scoreCount++;
        }
    }
    
    fclose(file);
}

// Save high scores to file
void saveHighScores(GameState* state) {
    FILE* file = fopen(SCORE_FILE_PATH, "w");
    
    if (file == NULL) {
        // Failed to open file for writing
        return;
    }
    
    // Write each score to file
    for (int i = 0; i < state->scoreCount; i++) {
        fprintf(file, "%d,%d,%s\n", 
                state->highScores[i].score,
                state->highScores[i].wave,
                state->highScores[i].date);
    }
    
    fclose(file);
}

// Add a new high score if it qualifies
void addHighScore(GameState* state, int score, int wave) {
    if (score <= 0) return;  // Don't add zero scores
    
    // Get current date
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    char date[12];
    strftime(date, 12, "%m/%d/%Y", tm_info);
    
    // Find where this score should be inserted
    int insertPos = state->scoreCount;
    for (int i = 0; i < state->scoreCount; i++) {
        if (score > state->highScores[i].score) {
            insertPos = i;
            break;
        }
    }
    
    // If we're at max capacity and the new score doesn't qualify, return
    if (insertPos >= MAX_HIGH_SCORES) {
        return;
    }
    
    // Shift lower scores down
    int lastPos = (state->scoreCount < MAX_HIGH_SCORES - 1) ? state->scoreCount : MAX_HIGH_SCORES - 1;
for (int i = lastPos; i > insertPos; i--) {
    state->highScores[i] = state->highScores[i-1];
}
    
    // Insert new score
    state->highScores[insertPos].score = score;
    state->highScores[insertPos].wave = wave;
    strcpy(state->highScores[insertPos].date, date);
    
    // Increase score count if not at max
    if (state->scoreCount < MAX_HIGH_SCORES) {
        state->scoreCount++;
    }
    
    // Save the updated scores
    saveHighScores(state);
}

// Render scoreboard on the menu
void renderScoreboard(const GameState* state, int x, int y, int width) {
    // Draw scoreboard background
    DrawRectangle(x, y, width, 360, (Color){0, 0, 0, 150});
    DrawRectangleLines(x, y, width, 360, WHITE);
    
    // Draw scoreboard title
    const char* title = "HIGH SCORES";
    int titleWidth = MeasureText(title, 24);
    DrawText(title, x + width/2 - titleWidth/2, y + 10, 24, WHITE);
    
    // Draw header line
    DrawLine(x + 10, y + 40, x + width - 10, y + 40, GRAY);
    
    // Draw table header
    DrawText("RANK", x + 20, y + 50, 16, GRAY);
    DrawText("SCORE", x + 80, y + 50, 16, GRAY);
    DrawText("WAVE", x + 180, y + 50, 16, GRAY);
    DrawText("DATE", x + 230, y + 50, 16, GRAY);
    
    // Draw dividing line
    DrawLine(x + 10, y + 72, x + width - 10, y + 72, GRAY);
    
    // Draw scores
    for (int i = 0; i < state->scoreCount; i++) {
        int rowY = y + 80 + i * 26;
        
        // Highlight the top 3 scores with different colors
        Color rowColor = WHITE;
        if (i == 0) rowColor = GOLD;
        else if (i == 1) rowColor = LIGHTGRAY;
        else if (i == 2) rowColor = BROWN;
        
        // Draw rank
        char rankText[10];
        sprintf(rankText, "%d", i + 1);
        DrawText(rankText, x + 20, rowY, 18, rowColor);
        
        // Draw score
        char scoreText[20];
        sprintf(scoreText, "%d", state->highScores[i].score);
        DrawText(scoreText, x + 80, rowY, 18, rowColor);
        
        // Draw wave
        char waveText[10];
        sprintf(waveText, "%d", state->highScores[i].wave);
        DrawText(waveText, x + 180, rowY, 18, rowColor);
        
        // Draw date
        DrawText(state->highScores[i].date, x + 230, rowY, 18, rowColor);
    }
    
    // If no scores yet, show message
    if (state->scoreCount == 0) {
        const char* noScores = "No scores yet!";
        int noScoresWidth = MeasureText(noScores, 20);
        DrawText(noScores, x + width/2 - noScoresWidth/2, y + 160, 20, GRAY);
        
        const char* playToAdd = "Play to add your score";
        int playWidth = MeasureText(playToAdd, 18);
        DrawText(playToAdd, x + width/2 - playWidth/2, y + 190, 18, LIGHTGRAY);
    }
}