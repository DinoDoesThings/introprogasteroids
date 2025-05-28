#ifndef AUDIO_H
#define AUDIO_H

// Custom headers
#include "typedefs.h"

void loadSounds(GameState* state);
void loadMusic(GameState* state);
void updateSoundVolume(GameState* state, float volume);
void updateMusicVolume(GameState* state, float volume);
void switchMusic(GameState* state, Music* newMusic);
void unloadMusic(GameState* state);

#endif // AUDIO_H