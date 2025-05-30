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
    Texture2D texture;
} Ship;

typedef struct {
    GameObject base;
    int size; // 3 = large, 2 = medium, 1 = small
} Asteroid;

typedef struct {
    float x;
    float y;
    float dx;
    float dy;
    float radius;
    float angle;
    float rotationSpeed;
    bool active;
} MenuAsteroid;

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
    GAME_OVER_STATE,
    INFO_STATE
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
    Texture2D texture;
} Enemy;

typedef enum {
    BULLET_NORMAL,
    BULLET_GRENADE
} BulletType;

typedef struct {
    GameObject base;
    int damage;
    bool isPlayerBullet; // To distinguish player bullets from enemy bullets
    BulletType type;     // New field for bullet type
    float timer;         // Timer for grenade explosion
    bool hasExploded;    // Flag to prevent multiple explosions
} Bullet;

typedef enum {
    POWERUP_HEALTH,
    POWERUP_SHOTGUN,
    POWERUP_GRENADE,
    POWERUP_LIFE
} PowerupType;

typedef enum {
    WEAPON_NORMAL,
    WEAPON_SHOTGUN,
    WEAPON_GRENADE
} WeaponType;

typedef struct {
    GameObject base;
    PowerupType type;
    float lifetime;
    float pulseTimer;
    Texture2D texture;
} Powerup;

typedef struct {
    int score;
    int wave;
    char date[12];  // Format: MM/DD/YYYY
} HighScore;

typedef struct {
    Ship ship;
    GameObject bullets[MAX_BULLETS];
    Asteroid asteroids[MAX_ASTEROIDS];
    Enemy enemies[MAX_ENEMIES];
    Bullet enemyBullets[MAX_ENEMY_BULLETS];
    Particle particles[MAX_PARTICLES];
    Powerup powerups[MAX_POWERUPS];
    WeaponType currentWeapon;
    int normalAmmo;
    int shotgunAmmo;
    int grenadeAmmo;           // field for grenade ammo
    float fireTimer;           // General fire timer
    float shotgunFireTimer;    // Specific timer for shotgun cooldown
    float grenadeFireTimer;    // Specific timer for grenade cooldown
    int score;
    int lives;
    int health;
    Camera2D camera;
    float reloadTimer;
    bool isReloading;
    bool running;
    bool Debug;
    GameScreenState screenState;
    GameScreenState previousScreenState;
    MenuAsteroid menuAsteroids[MAX_MENU_ASTEROIDS];
    Rectangle playButton;
    Rectangle quitButton;
    Rectangle resumeButton;
    Rectangle optionsButton;
    Rectangle backButton;
    Rectangle volumeSlider;
    Rectangle musicVolumeSlider;
    Rectangle mainMenuButton;
    bool windowFocused;
    Sound sounds[MAX_SOUNDS];
    bool soundLoaded;
    float enemySpawnTimer;
    int currentWave;
    int asteroidsRemaining;
    float waveDelayTimer;
    bool inWaveTransition;
    char waveMessage[64];
    float waveMessageTimer;
    float soundVolume;
    bool isDraggingSlider;
    bool isDraggingMusicSlider;
    Music menuMusic;
    Music phase1Music;
    Music phase2Music;
    Music* currentMusic;  // Pointer to track which music is currently active
    bool musicLoaded;
    float musicVolume;
    float invulnerabilityTimer;
    bool isInvulnerable;
    float blinkTimer;
    bool shipVisible;
    int enemiesSpawnedThisWave;
    int maxEnemiesThisWave;
    int EnemySpawnComplete;
    HighScore highScores[MAX_HIGH_SCORES];
    int scoreCount;
} GameState;

typedef struct {
    int minRadius;
    int maxRadius;
    float randomRange;
    float rearOffset;
    float speedMultiplier;
    Color colors[3];  // Array of 3 possible colors for variety
} EnemyParticleConfig;

#endif // TYPEDEFS_H