#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include "config.h"
#include "raylib.h"
#include <stdbool.h>

typedef struct {
    float x, y;
    float dx, dy;
    float angle;
    float radius;
    bool active;
} GameObject;

typedef struct {
    GameObject base;
    float rotationSpeed;
} Ship;

typedef struct {
    GameObject base;
    int size; // 3 = large, 2 = medium, 1 = small
} Asteroid;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float radius;
    float life;
    Color color;
    bool active;
} Particle;

typedef enum {
    MENU_STATE,
    GAME_STATE,
    PAUSE_STATE,
    OPTIONS_STATE,
    GAME_OVER_STATE
} GameScreenState;

typedef enum {
    ENEMY_TANK,
    ENEMY_SCOUT
} EnemyType;

typedef struct {
    GameObject base;
    EnemyType type;
    int health;
    float fireTimer;
    int burstCount;    // For scout enemy burst fire
    float burstTimer;
    bool isBursting;
    float moveAngle;   // Angle for movement direction
    float moveTimer;   // Timer for changing movement direction
} Enemy;

typedef struct {
    GameObject base;
    int damage;
    bool isPlayerBullet; // To distinguish player bullets from enemy bullets
} Bullet;

typedef struct {
    Ship ship;
    GameObject bullets[MAX_BULLETS];
    Asteroid asteroids[MAX_ASTEROIDS];
    Particle particles[MAX_PARTICLES];
    int score;
    int lives;
    int health;
    Camera2D camera;
    int currentAmmo;
    float reloadTimer;
    bool isReloading;
    float fireTimer;
    bool running;
    bool Debug;
    GameScreenState screenState;
    GameScreenState previousScreenState;
    Rectangle playButton;
    Rectangle quitButton;
    Rectangle resumeButton;
    Rectangle optionsButton;
    Rectangle backButton;
    Rectangle volumeSlider;
    Rectangle mainMenuButton;
    bool windowFocused;
    Sound sounds[MAX_SOUNDS];
    bool soundLoaded;
    Enemy enemies[MAX_ENEMIES];
    Bullet enemyBullets[MAX_ENEMY_BULLETS];
    float enemySpawnTimer;
    int currentWave;
    int asteroidsRemaining;
    float waveDelayTimer;
    bool inWaveTransition;
    char waveMessage[64];
    float waveMessageTimer;
    float soundVolume;
    bool isDraggingSlider;
    // Add these fields for invulnerability
    float invulnerabilityTimer;
    bool isInvulnerable;
    float blinkTimer;
    bool shipVisible;
} GameState;

#endif // TYPEDEFS_H