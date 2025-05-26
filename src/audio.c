#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>

//custom headers
#include "typedef.h"
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
    state->sounds[SOUND_SHIP_HIT] = LoadSound("resources/sounds/ship_hit.ogg");
    state->sounds[SOUND_ENEMY_SHOOT] = LoadSound("resources/sounds/enemy_shoot.ogg");
    state->sounds[SOUND_ENEMY_EXPLODE] = LoadSound("resources/sounds/enemy_explode.ogg");
    
    // Set initial volume for all sounds
    for (int i = 0; i < MAX_SOUNDS; i++) {
        SetSoundVolume(state->sounds[i], state->soundVolume);
    }
    
    state->soundLoaded = true;
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