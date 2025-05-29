#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>

//custom headers
#include "typedefs.h"
#include "config.h"

void loadSounds(GameState* state) {
    if (state->soundLoaded) return;
    
    // Initialize audio device
    InitAudioDevice();
    
    // Load sound effects
    state->sounds[SOUND_SHOOT] = LoadSound("resources/sounds/shoot.ogg");
    state->sounds[SOUND_RELOAD_START] = LoadSound("resources/sounds/reload_start.ogg");
    state->sounds[SOUND_RELOAD_FINISH] = LoadSound("resources/sounds/reload_finish.ogg");
    state->sounds[SOUND_ASTEROID_HIT] = LoadSound("resources/sounds/asteroid_hit.ogg");
    state->sounds[SOUND_SCOUT_SHOOT] = LoadSound("resources/sounds/scout_shoot.ogg");
    state->sounds[SOUND_TANK_SHOOT] = LoadSound("resources/sounds/tank_shoot.ogg");
    state->sounds[SOUND_ENEMY_EXPLODE] = LoadSound("resources/sounds/enemy_explode.ogg");
    state->sounds[SOUND_POWERUP_PICKUP] = LoadSound("resources/sounds/powerup_pickup.ogg");
    
    // Set initial volume for all sounds
    for (int i = 0; i < MAX_SOUNDS; i++) {
        SetSoundVolume(state->sounds[i], state->soundVolume);
    }
    
    state->soundLoaded = true;
}

void loadMusic(GameState* state) {
    if (state->musicLoaded) return;
    
    // Load all music tracks
    state->menuMusic = LoadMusicStream("resources/soundtrack/menu.ogg");
    state->phase1Music = LoadMusicStream("resources/soundtrack/phase1.ogg");
    state->phase2Music = LoadMusicStream("resources/soundtrack/phase2.ogg");
    
    // Set initial music pointer to menu music
    state->currentMusic = &state->menuMusic;
    
    // Set all music to loop
    state->menuMusic.looping = true;
    state->phase1Music.looping = true;
    state->phase2Music.looping = true;
    
    // Set initial volume for all music
    SetMusicVolume(state->menuMusic, state->musicVolume);
    SetMusicVolume(state->phase1Music, state->musicVolume);
    SetMusicVolume(state->phase2Music, state->musicVolume);
    
    state->musicLoaded = true;
}

void updateSoundVolume(GameState* state, float volume) {
    state->soundVolume = volume;
    
    // Update all sound volumes
    if (state->soundLoaded) {
        for (int i = 0; i < MAX_SOUNDS; i++) {
            SetSoundVolume(state->sounds[i], state->soundVolume);
        }
    }
}

void updateMusicVolume(GameState* state, float volume) {
    state->musicVolume = volume;
    
    // Update all music volumes
    if (state->musicLoaded) {
        SetMusicVolume(state->menuMusic, state->musicVolume);
        SetMusicVolume(state->phase1Music, state->musicVolume);
        SetMusicVolume(state->phase2Music, state->musicVolume);
    }
}

void switchMusic(GameState* state, Music* newMusic) {
    if (!state->musicLoaded || state->currentMusic == newMusic) return;
    
    // Stop current music
    StopMusicStream(*state->currentMusic);
    
    // Update current music pointer
    state->currentMusic = newMusic;
    
    // Play new music
    PlayMusicStream(*state->currentMusic);
}

void unloadMusic(GameState* state) {
    if (state->musicLoaded) {
        StopMusicStream(state->menuMusic);
        StopMusicStream(state->phase1Music);
        StopMusicStream(state->phase2Music);
        
        UnloadMusicStream(state->menuMusic);
        UnloadMusicStream(state->phase1Music);
        UnloadMusicStream(state->phase2Music);
        
        state->musicLoaded = false;
    }
}