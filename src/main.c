#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAP_WIDTH 2000
#define MAP_HEIGHT 1500
#define BOUNDARY_COLOR (Color){ 30, 30, 80, 255 }  // Dark blue boundary
#define MAX_BULLETS 30
#define MAX_ASTEROIDS 20
#define BULLET_SPEED 10.0f
#define SHIP_ACCELERATION 0.2f
#define SHIP_MAX_SPEED 5.0f
#define ROTATION_SPEED 5.0f
#define FRICTION 0.98f
#define MAX_HEALTH 100
#define LARGE_ASTEROID_DAMAGE 30
#define MEDIUM_ASTEROID_DAMAGE 20
#define SMALL_ASTEROID_DAMAGE 10
#define MAX_PARTICLES 150
#define PARTICLE_LIFETIME 1.0f
#define PARTICLE_SPEED 2.0f
#define MAX_AMMO 30
#define RELOAD_TIME 10.0f
#define FIRE_RATE 0.2f  // Time between shots in seconds
#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 50
#define TITLE_FONT_SIZE 60
#define BUTTON_FONT_SIZE 30
#define MAX_ENEMIES 10
#define TANK_ENEMY_RADIUS 25.0f
#define TANK_ENEMY_SPEED 0.7f
#define TANK_ENEMY_HEALTH 100
#define TANK_ENEMY_BULLET_DAMAGE 40
#define TANK_ENEMY_FIRE_RATE 2.0f
#define TANK_ENEMY_SCORE 500
#define SCOUT_ENEMY_RADIUS 12.0f
#define SCOUT_ENEMY_SPEED 3.5f
#define SCOUT_ENEMY_HEALTH 30
#define SCOUT_ENEMY_BULLET_DAMAGE 5
#define SCOUT_ENEMY_FIRE_RATE 0.1f
#define SCOUT_ENEMY_BURST_COUNT 5
#define SCOUT_ENEMY_BURST_DELAY 0.1f
#define SCOUT_ENEMY_SCORE 300
#define ENEMY_BULLET_SPEED 6.0f
#define MAX_ENEMY_BULLETS 50
#define ENEMY_DETECTION_RADIUS 500.0f
#define ENEMY_SPAWN_TIME 15.0f
#define SCOUT_ENEMY_ATTACK_DISTANCE 300.0f
#define TANK_ENEMY_ATTACK_DISTANCE 450.0f
#define MAX_SOUNDS 7  // Maximum number of sounds we'll load
#define SOUND_SHOOT 0
#define SOUND_RELOAD_START 1
#define SOUND_RELOAD_FINISH 2
#define SOUND_ASTEROID_HIT 3
#define SOUND_SHIP_HIT 4
#define SOUND_ENEMY_SHOOT 5
#define SOUND_ENEMY_EXPLODE 6

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
    PAUSE_STATE
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
    Rectangle playButton;
    Rectangle quitButton;
    Rectangle resumeButton;
    bool windowFocused;
    Sound sounds[MAX_SOUNDS];
    bool soundLoaded;
    Enemy enemies[MAX_ENEMIES];
    Bullet enemyBullets[MAX_ENEMY_BULLETS];
    float enemySpawnTimer;
} GameState;

// Function prototypes
void initGameState(GameState* state);
void handleInput(GameState* state);
void updateGame(GameState* state, float deltaTime);
void renderGame(const GameState* state);
void createAsteroids(GameState* state, int count);
void resetShip(GameState* state);
void fireWeapon(GameState* state);
bool checkCollision(GameObject* a, GameObject* b);
void splitAsteroid(GameState* state, int index);
void renderGameObject(const GameObject* obj, int sides, const GameState* state);
void updateParticles(GameState* state, float deltaTime);
void renderParticles(const GameState* state);
void emitParticles(GameState* state, int count);
void handleMenuInput(GameState* state);
void renderMenu(const GameState* state);
void handlePauseInput(GameState* state);
void renderPause(const GameState* state);
void loadSounds(GameState* state);
void spawnEnemy(GameState* state, EnemyType type);
void updateEnemies(GameState* state, float deltaTime);
void renderEnemies(const GameState* state);
void fireEnemyWeapon(GameState* state, Enemy* enemy);

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
    
    gameState.quitButton = (Rectangle){
        WINDOW_WIDTH/2 - BUTTON_WIDTH/2,
        WINDOW_HEIGHT/2 + BUTTON_HEIGHT + 20,
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    };
    
    gameState.resumeButton = (Rectangle){
        WINDOW_WIDTH/2 - BUTTON_WIDTH/2,
        WINDOW_HEIGHT/2 - BUTTON_HEIGHT - 20,
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

void initGameState(GameState* state) {
    // Initialize to default values
    state->score = 0;
    state->lives = 3;
    state->health = MAX_HEALTH;
    state->currentAmmo = MAX_AMMO;
    state->reloadTimer = 0.0f;
    state->isReloading = false;
    state->fireTimer = 0.0f;
    state->running = true;
    state->Debug = false;
    state->screenState = MENU_STATE;
    state->soundLoaded = false;
    
    // Load sounds
    loadSounds(state);
    
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
    
    state->enemySpawnTimer = ENEMY_SPAWN_TIME;
    
    // Create initial asteroids
    createAsteroids(state, 10);
}

void handleInput(GameState* state) {
    // Check for pause
    if (IsKeyPressed(KEY_P)) {
        state->screenState = PAUSE_STATE;
        return; // Skip other input handling when pausing
    }
    
    // Get mouse position in world space
    Vector2 mousePosition = GetScreenToWorld2D(GetMousePosition(), state->camera);
    
    // Calculate angle between ship and mouse cursor
    float dx = mousePosition.x - state->ship.base.x;
    float dy = mousePosition.y - state->ship.base.y;
    state->ship.base.angle = atan2(dx, -dy) * 180.0f / PI;
    
    // Handle mouse click for firing - changed to support holding the button
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && state->fireTimer <= 0) {
        fireWeapon(state);
        state->fireTimer = FIRE_RATE; // Set the cooldown timer
    }
    
    // Continuous key presses
    if (IsKeyDown(KEY_W)) {
        // Accelerate ship in the direction it's facing
        state->ship.base.dx += SHIP_ACCELERATION * sin(state->ship.base.angle * PI / 180.0f);
        state->ship.base.dy -= SHIP_ACCELERATION * cos(state->ship.base.angle * PI / 180.0f);
        
        // Emit particles when moving forward
        emitParticles(state, 2);
        
        // Limit speed
        float speed = sqrt(state->ship.base.dx * state->ship.base.dx + state->ship.base.dy * state->ship.base.dy);
        if (speed > SHIP_MAX_SPEED) {
            state->ship.base.dx = (state->ship.base.dx / speed) * SHIP_MAX_SPEED;
            state->ship.base.dy = (state->ship.base.dy / speed) * SHIP_MAX_SPEED;
        }
    }
    
    if (IsKeyDown(KEY_A)) {
        // Strafe left (perpendicular to the ship's facing direction)
        state->ship.base.dx -= SHIP_ACCELERATION * cos(state->ship.base.angle * PI / 180.0f);
        state->ship.base.dy -= SHIP_ACCELERATION * sin(state->ship.base.angle * PI / 180.0f);
    }
    
    if (IsKeyDown(KEY_D)) {
        // Strafe right (perpendicular to the ship's facing direction)
        state->ship.base.dx += SHIP_ACCELERATION * cos(state->ship.base.angle * PI / 180.0f);
        state->ship.base.dy += SHIP_ACCELERATION * sin(state->ship.base.angle * PI / 180.0f);
    }
    
    if (IsKeyDown(KEY_S)) {
        // Decelerate/reverse
        state->ship.base.dx -= SHIP_ACCELERATION * sin(state->ship.base.angle * PI / 180.0f);
        state->ship.base.dy += SHIP_ACCELERATION * cos(state->ship.base.angle * PI / 180.0f);
    }

    if (IsKeyDown(KEY_R)) {
        // Reload ammo
        if (!state->isReloading && state->currentAmmo < MAX_AMMO) {
            state->isReloading = true;
            state->reloadTimer = RELOAD_TIME;
            
            // Play reload start sound
            if (state->soundLoaded) {
                PlaySound(state->sounds[SOUND_RELOAD_START]);
            }
        }
    }

    if (IsKeyPressed(KEY_F3)) {
        // Toggle debug mode
        state->Debug = !state->Debug;
    }
}

void updateGame(GameState* state, float deltaTime) {
    // Update fire timer
    if (state->fireTimer > 0) {
        state->fireTimer -= deltaTime;
    }
    
    // Update ship
    float newX = state->ship.base.x + state->ship.base.dx;
    float newY = state->ship.base.y + state->ship.base.dy;
    
    // Block ship at boundaries instead of teleporting
    if (newX - state->ship.base.radius >= 0 && newX + state->ship.base.radius <= MAP_WIDTH) {
        state->ship.base.x = newX;
    } else {
        state->ship.base.dx *= -0.5f; // Bounce with reduced speed
    }
    
    if (newY - state->ship.base.radius >= 0 && newY + state->ship.base.radius <= MAP_HEIGHT) {
        state->ship.base.y = newY;
    } else {
        state->ship.base.dy *= -0.5f; // Bounce with reduced speed
    }
    
    // Apply friction
    state->ship.base.dx *= FRICTION;
    state->ship.base.dy *= FRICTION;
    
    // Update camera to follow the ship
    state->camera.target = (Vector2){ state->ship.base.x, state->ship.base.y };
    
    // Handle reloading
    if (state->isReloading) {
        state->reloadTimer -= deltaTime;
        if (state->reloadTimer <= 0) {
            state->isReloading = false;
            state->currentAmmo = MAX_AMMO;
            
            // Play reload finish sound
            if (state->soundLoaded) {
                PlaySound(state->sounds[SOUND_RELOAD_FINISH]);
            }
        }
    }
    
    // Update bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (state->bullets[i].active) {
            state->bullets[i].x += state->bullets[i].dx;
            state->bullets[i].y += state->bullets[i].dy;
            
            // Check if bullet is out of bounds
            if (state->bullets[i].x < 0 || state->bullets[i].x > MAP_WIDTH || 
                state->bullets[i].y < 0 || state->bullets[i].y > MAP_HEIGHT) {
                state->bullets[i].active = false;
                continue;
            }
            
            // Check for collision with asteroids
            for (int j = 0; j < MAX_ASTEROIDS; j++) {
                if (state->asteroids[j].base.active && checkCollision(&state->bullets[i], &state->asteroids[j].base)) {
                    state->bullets[i].active = false;
                    splitAsteroid(state, j);
                    
                    // Update score based on asteroid size
                    state->score += (4 - state->asteroids[j].size) * 100;
                    
                    break;
                }
            }
        }
    }
    
    // Update asteroids
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (state->asteroids[i].base.active) {
            // Update position
            state->asteroids[i].base.x += state->asteroids[i].base.dx;
            state->asteroids[i].base.y += state->asteroids[i].base.dy;
            
            // Bounce asteroids off boundaries
            if (state->asteroids[i].base.x - state->asteroids[i].base.radius < 0) {
                state->asteroids[i].base.x = state->asteroids[i].base.radius;
                state->asteroids[i].base.dx *= -1;
            }
            else if (state->asteroids[i].base.x + state->asteroids[i].base.radius > MAP_WIDTH) {
                state->asteroids[i].base.x = MAP_WIDTH - state->asteroids[i].base.radius;
                state->asteroids[i].base.dx *= -1;
            }
            
            if (state->asteroids[i].base.y - state->asteroids[i].base.radius < 0) {
                state->asteroids[i].base.y = state->asteroids[i].base.radius;
                state->asteroids[i].base.dy *= -1;
            }
            else if (state->asteroids[i].base.y + state->asteroids[i].base.radius > MAP_HEIGHT) {
                state->asteroids[i].base.y = MAP_HEIGHT - state->asteroids[i].base.radius;
                state->asteroids[i].base.dy *= -1;
            }
            
            // Check for collision with ship
            if (checkCollision(&state->ship.base, &state->asteroids[i].base)) {
                // Apply damage based on asteroid size
                int damage = 0;
                switch (state->asteroids[i].size) {
                    case 3: damage = LARGE_ASTEROID_DAMAGE; break;
                    case 2: damage = MEDIUM_ASTEROID_DAMAGE; break;
                    case 1: damage = SMALL_ASTEROID_DAMAGE; break;
                }
                
                state->health -= damage;
                
                if (state->health <= 0) {
                    state->lives--;
                    if (state->lives <= 0) {
                        printf("Game Over! Final Score: %d\n", state->score);
                        state->running = false;
                    } else {
                        resetShip(state);
                    }
                }
                
                // Destroy the asteroid that hit the ship
                splitAsteroid(state, i);
                break;
            }
        }
    }
    
    // Add collision detection between asteroids
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (state->asteroids[i].base.active) {
            for (int j = i + 1; j < MAX_ASTEROIDS; j++) {
                if (state->asteroids[j].base.active && 
                    checkCollision(&state->asteroids[i].base, &state->asteroids[j].base)) {
                    
                    // Calculate collision response
                    float dx = state->asteroids[j].base.x - state->asteroids[i].base.x;
                    float dy = state->asteroids[j].base.y - state->asteroids[i].base.y;
                    float distance = sqrt(dx * dx + dy * dy);
                    
                    // Avoid division by zero
                    if (distance == 0) distance = 0.01f;
                    
                    // Normalize direction
                    float nx = dx / distance;
                    float ny = dy / distance;
                    
                    // Calculate relative velocity
                    float dvx = state->asteroids[j].base.dx - state->asteroids[i].base.dx;
                    float dvy = state->asteroids[j].base.dy - state->asteroids[i].base.dy;
                    
                    // Calculate velocity along the normal direction
                    float velocityAlongNormal = dvx * nx + dvy * ny;
                    
                    // Don't resolve if velocities are separating
                    if (velocityAlongNormal > 0) continue;
                    
                    // Calculate restitution (bounciness)
                    float restitution = 0.8f;
                    
                    // Calculate impulse scalar
                    float impulse = -(1 + restitution) * velocityAlongNormal;
                    
                    // Calculate mass ratio based on size
                    float totalMass = state->asteroids[i].size + state->asteroids[j].size;
                    float massRatio1 = state->asteroids[j].size / totalMass;
                    float massRatio2 = state->asteroids[i].size / totalMass;
                    
                    // Apply impulse
                    float impulsex = impulse * nx;
                    float impulsey = impulse * ny;
                    
                    state->asteroids[i].base.dx -= impulsex * massRatio1;
                    state->asteroids[i].base.dy -= impulsey * massRatio1;
                    state->asteroids[j].base.dx += impulsex * massRatio2;
                    state->asteroids[j].base.dy += impulsey * massRatio2;
                    
                    // Prevent asteroids from getting stuck together by separating them
                    float overlap = state->asteroids[i].base.radius + state->asteroids[j].base.radius - distance;
                    if (overlap > 0) {
                        // Move asteroids apart based on their size/mass
                        state->asteroids[i].base.x -= nx * overlap * massRatio1 * 0.5f;
                        state->asteroids[i].base.y -= ny * overlap * massRatio1 * 0.5f;
                        state->asteroids[j].base.x += nx * overlap * massRatio2 * 0.5f;
                        state->asteroids[j].base.y += ny * overlap * massRatio2 * 0.5f;
                    }
                }
            }
        }
    }
    
    // Check if all asteroids are destroyed
    bool allDestroyed = true;
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (state->asteroids[i].base.active) {
            allDestroyed = false;
            break;
        }
    }
    
    if (allDestroyed) {
        createAsteroids(state, 8 + state->score / 5000); // Increase difficulty with score
    }
    
    // Update particles
    updateParticles(state, deltaTime);
    
    // Update enemies
    updateEnemies(state, deltaTime);
}

void renderGame(const GameState* state) {
    BeginDrawing();
    
    // Clear screen with a very dark background for space
    ClearBackground((Color){5, 5, 15, 255});
    
    // Begin camera rendering
    BeginMode2D(state->camera);
    
    // Draw map boundaries
    DrawRectangleLines(0, 0, MAP_WIDTH, MAP_HEIGHT, BOUNDARY_COLOR);
    
    // Draw grid lines to help visualize the larger map (optional)
    for (int i = 0; i < MAP_WIDTH; i += 200) {
        DrawLine(i, 0, i, MAP_HEIGHT, (Color){20, 20, 40, 100});
    }
    for (int i = 0; i < MAP_HEIGHT; i += 200) {
        DrawLine(0, i, MAP_WIDTH, i, (Color){20, 20, 40, 100});
    }
    
    // Draw particles
    renderParticles(state);
    
    // Draw ship
    renderGameObject(&state->ship.base, 3, state);

    
    // Draw bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (state->bullets[i].active) {
            DrawRectangle(
                state->bullets[i].x - state->bullets[i].radius, 
                state->bullets[i].y - state->bullets[i].radius, 
                state->bullets[i].radius * 2, 
                state->bullets[i].radius * 2, 
                YELLOW
            );
        }
    }
    
    // Draw asteroids
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (state->asteroids[i].base.active) {
            renderGameObject(&state->asteroids[i].base, 8, state); // Draw as octagon
        }
    }
    
    // Draw enemies
    renderEnemies(state);
    
    // Draw aiming guide
    Vector2 mousePosition = GetScreenToWorld2D(GetMousePosition(), state->camera);
    DrawLine(state->ship.base.x, state->ship.base.y, mousePosition.x, mousePosition.y, GRAY);
    
    EndMode2D();
    
    // UI elements should be drawn after EndMode2D so they stay fixed on screen
    
    // Draw score and lives
    DrawText(TextFormat("Score: %d", state->score), 10, 10, 20, WHITE);

    if (state->lives > 1) {
        DrawText(TextFormat("Lives: %d", state->lives), 10, 40, 20, WHITE);
    } else {
        DrawText(TextFormat("Lives: %d", state->lives), 10, 40, 20, RED);
    }
    
    // Draw health bar
    int barWidth = 200;
    int barHeight = 20;
    int barX = WINDOW_WIDTH - barWidth - 20;
    int barY = 8;
    
    // Background of health bar
    DrawRectangle(barX, barY, barWidth, barHeight, GRAY);
    
    // Calculate health percentage
    float healthPercent = (float)state->health / MAX_HEALTH;
    int currentHealthWidth = (int)(barWidth * healthPercent);
    
    // Pick color based on health level
    Color healthColor;
    if (healthPercent > 0.7f) healthColor = GREEN;
    else if (healthPercent > 0.3f) healthColor = YELLOW;
    else healthColor = RED;
    
    // Draw the filled part of the health bar
    DrawRectangle(barX, barY, currentHealthWidth, barHeight, healthColor);
    
    // Draw border around health bar
    DrawRectangleLines(barX, barY, barWidth, barHeight, WHITE);
    
    // Draw health text
    DrawText(TextFormat("Health: %d/%d", state->health, MAX_HEALTH), barX, barY + barHeight + 5, 15, WHITE);
    
    // Draw ammo counter
    int ammoY = 70; // Position below lives
    
    if (state->isReloading) {
        // Draw reload timer text
        DrawText(TextFormat("RELOADING: %.1f", state->reloadTimer), 10, ammoY, 20, RED);
        
        // Draw reload progress bar - positioned directly below the text
        int reloadBarWidth = 150;
        int reloadBarHeight = 15;
        int reloadBarX = 10; // Align with text
        int reloadBarY = ammoY + 25; // Position below text with some spacing
        
        // Background of reload bar
        DrawRectangle(reloadBarX, reloadBarY, reloadBarWidth, reloadBarHeight, GRAY);
        
        // Calculate reload progress percentage
        float reloadPercent = 1.0f - (state->reloadTimer / RELOAD_TIME);
        int currentReloadWidth = (int)(reloadBarWidth * reloadPercent);
        
        // Draw the filled part of the reload bar
        DrawRectangle(reloadBarX, reloadBarY, currentReloadWidth, reloadBarHeight, ORANGE);
        
        // Draw border around reload bar
        DrawRectangleLines(reloadBarX, reloadBarY, reloadBarWidth, reloadBarHeight, WHITE);
    } else {
        // Draw ammo count
        Color ammoColor = state->currentAmmo > 5 ? WHITE : RED;
        DrawText(TextFormat("AMMO: %d/%d", state->currentAmmo, MAX_AMMO), 10, ammoY, 20, ammoColor);
        
        // Draw ammo indicators as small rectangles - positioned below the text
        int ammoRectSize = 5;
        int ammoRectSpacing = 3;
        int ammoStartX = 10; // Align with text
        int ammoIndicatorY = ammoY + 30; // Position below text with some spacing
        
        // Draw ammo indicators
        for (int i = 0; i < MAX_AMMO; i++) {
            Color rectColor = i < state->currentAmmo ? YELLOW : DARKGRAY;
            DrawRectangle(
                ammoStartX + (i * (ammoRectSize + ammoRectSpacing)), 
                ammoIndicatorY, 
                ammoRectSize, 
                ammoRectSize, 
                rectColor
            );
        }
    }

    if (state->Debug) {
        // Draw debug information
        DrawText(TextFormat("FPS: %d", GetFPS()), 10, 120, 20, WHITE);
        DrawText(TextFormat("Ship Position: (%.1f, %.1f)", state->ship.base.x, state->ship.base.y), 10, 150, 20, WHITE);
        DrawText(TextFormat("Ship Velocity: (%.1f, %.1f)", state->ship.base.dx, state->ship.base.dy), 10, 180, 20, WHITE);

        // Count active asteroids
        int activeAsteroids = 0;
        for (int i = 0; i < MAX_ASTEROIDS; i++) {
            if (state->asteroids[i].base.active) {
                activeAsteroids++;
            }
        }
        DrawText(TextFormat("Active Asteroids: %d", activeAsteroids), 10, 210, 20, WHITE);
    }
    
    EndDrawing();
}

void renderGameObject(const GameObject* obj, int sides, const GameState* state) {
    if (sides <= 0 || !obj->active) return;
    
    // Special rendering for the ship (triangle)
    if (sides == 3 && obj == &state->ship.base) {
        Vector2 points[3];
        float radians = obj->angle * PI / 180.0f;
        
        // Front point (nose of the ship)
        points[0].x = obj->x + sin(radians) * obj->radius * 1.5f;
        points[0].y = obj->y - cos(radians) * obj->radius * 1.5f;
        
        // Left wing
        float leftAngle = radians + PI * 0.8f;
        points[1].x = obj->x + sin(leftAngle) * obj->radius;
        points[1].y = obj->y - cos(leftAngle) * obj->radius;
        
        // Right wing
        float rightAngle = radians - PI * 0.8f;
        points[2].x = obj->x + sin(rightAngle) * obj->radius;
        points[2].y = obj->y - cos(rightAngle) * obj->radius;
        
        // Draw the lines
        DrawLineV(points[0], points[1], GREEN);
        DrawLineV(points[1], points[2], GREEN);
        DrawLineV(points[2], points[0], GREEN);
    } else {
        // Regular rendering for other objects
        Vector2 points[sides];
        for (int i = 0; i < sides; i++) {
            float angle = obj->angle + i * (360.0f / sides);
            float radians = angle * PI / 180.0f;
            points[i].x = obj->x + sin(radians) * obj->radius;
            points[i].y = obj->y - cos(radians) * obj->radius;
        }
        
        // Draw the lines
        for (int i = 0; i < sides; i++) {
            int next = (i + 1) % sides;
            DrawLineV(points[i], points[next], LIGHTGRAY);
        }
    }
}

void createAsteroids(GameState* state, int count) {
    int created = 0;
    
    for (int i = 0; i < MAX_ASTEROIDS && created < count; i++) {
        if (!state->asteroids[i].base.active) {
            state->asteroids[i].base.active = true;
            state->asteroids[i].size = 3; // Start with large asteroids
            state->asteroids[i].base.radius = 20.0f * state->asteroids[i].size;
            
            // Place asteroid away from the ship but within map bounds
            do {
                state->asteroids[i].base.x = GetRandomValue(
                    state->asteroids[i].base.radius, 
                    MAP_WIDTH - state->asteroids[i].base.radius
                );
                
                state->asteroids[i].base.y = GetRandomValue(
                    state->asteroids[i].base.radius, 
                    MAP_HEIGHT - state->asteroids[i].base.radius
                );
                
            } while (sqrt(pow(state->asteroids[i].base.x - state->ship.base.x, 2) + 
                         pow(state->asteroids[i].base.y - state->ship.base.y, 2)) < 200);
            
            // Random velocity
            float angle = GetRandomValue(0, 359) * PI / 180.0f;
            float speed = 1.0f + GetRandomValue(0, 100) / 100.0f;
            state->asteroids[i].base.dx = sin(angle) * speed;
            state->asteroids[i].base.dy = -cos(angle) * speed;
            state->asteroids[i].base.angle = GetRandomValue(0, 359);
            
            created++;
        }
    }
}

void resetShip(GameState* state) {
    state->ship.base.x = MAP_WIDTH / 2;
    state->ship.base.y = MAP_HEIGHT / 2;
    state->ship.base.dx = 0;
    state->ship.base.dy = 0;
    state->ship.base.angle = 0;
    state->ship.base.radius = 15.0f;
    state->ship.base.active = true;
    state->health = MAX_HEALTH; // Reset health when respawning
    state->currentAmmo = MAX_AMMO; // Reset ammo
    state->isReloading = false; // Stop reloading
}

void fireWeapon(GameState* state) {
    // Don't fire if reloading or out of ammo
    if (state->isReloading || state->currentAmmo <= 0) {
        return;
    }
    
    // Get mouse position in world space
    Vector2 mousePosition = GetScreenToWorld2D(GetMousePosition(), state->camera);
    
    // Calculate direction from ship to mouse cursor
    float dx = mousePosition.x - state->ship.base.x;
    float dy = mousePosition.y - state->ship.base.y;
    float length = sqrt(dx * dx + dy * dy);
    
    // Normalize the direction
    if (length > 0) {
        dx /= length;
        dy /= length;
    }
    
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!state->bullets[i].active) {
            state->bullets[i].active = true;
            
            // Start the bullet at the ship's position
            state->bullets[i].x = state->ship.base.x;
            state->bullets[i].y = state->ship.base.y;
            state->bullets[i].radius = 2.0f;
            
            // Set bullet velocity toward the mouse cursor
            state->bullets[i].dx = dx * BULLET_SPEED;
            state->bullets[i].dy = dy * BULLET_SPEED;
            
            // Play shooting sound effect
            if (state->soundLoaded) {
                PlaySound(state->sounds[SOUND_SHOOT]);
            }
            
            // Decrease ammo
            state->currentAmmo--;
            
            // Start reloading if out of ammo
            if (state->currentAmmo <= 0) {
                state->isReloading = true;
                state->reloadTimer = RELOAD_TIME;
                
                // Play reload start sound
                if (state->soundLoaded) {
                    PlaySound(state->sounds[SOUND_RELOAD_START]);
                }
            }
            
            // Only fire one bullet at a time
            break;
        }
    }
}

bool checkCollision(GameObject* a, GameObject* b) {
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float distance = sqrt(dx * dx + dy * dy);
    
    return distance < (a->radius + b->radius);
}

void splitAsteroid(GameState* state, int index) {
    // Get the asteroid properties before deactivating it
    float x = state->asteroids[index].base.x;
    float y = state->asteroids[index].base.y;
    int size = state->asteroids[index].size;
    
    // Deactivate the hit asteroid
    state->asteroids[index].base.active = false;
    
    // If it's not the smallest size, split into two smaller asteroids
    if (size > 1) {
        int newSize = size - 1;
        int created = 0;
        
        for (int i = 0; i < MAX_ASTEROIDS && created < 2; i++) {
            if (!state->asteroids[i].base.active) {
                state->asteroids[i].base.active = true;
                state->asteroids[i].size = newSize;
                state->asteroids[i].base.radius = 20.0f * newSize;
                state->asteroids[i].base.x = x;
                state->asteroids[i].base.y = y;
                
                // Random velocity
                float angle = GetRandomValue(0, 359) * PI / 180.0f;
                float speed = 1.5f + GetRandomValue(0, 100) / 100.0f;
                state->asteroids[i].base.dx = sin(angle) * speed;
                state->asteroids[i].base.dy = -cos(angle) * speed;
                state->asteroids[i].base.angle = GetRandomValue(0, 359);
                
                created++;
            }
        }
    }
}

void updateParticles(GameState* state, float deltaTime) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (state->particles[i].active) {
            // Update particle position
            state->particles[i].position.x += state->particles[i].velocity.x;
            state->particles[i].position.y += state->particles[i].velocity.y;
            
            // Update particle life
            state->particles[i].life -= deltaTime;
            
            // Make particles shrink over time
            state->particles[i].radius = 3.0f * (state->particles[i].life / PARTICLE_LIFETIME);
            
            // Fade out particles over time
            state->particles[i].color.a = (unsigned char)(255.0f * (state->particles[i].life / PARTICLE_LIFETIME));
            
            // Deactivate expired particles
            if (state->particles[i].life <= 0) {
                state->particles[i].active = false;
            }
        }
    }
}

void renderParticles(const GameState* state) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (state->particles[i].active) {
            DrawCircleV(state->particles[i].position, state->particles[i].radius, state->particles[i].color);
        }
    }
}

void emitParticles(GameState* state, int count) {
    // Calculate ship rear position (opposite to the front)
    float radians = state->ship.base.angle * PI / 180.0f;
    float rearX = state->ship.base.x - sin(radians) * state->ship.base.radius * 1.2f;
    float rearY = state->ship.base.y + cos(radians) * state->ship.base.radius * 1.2f;
    
    // Emit particles
    for (int i = 0; i < count; i++) {
        // Find an inactive particle
        for (int j = 0; j < MAX_PARTICLES; j++) {
            if (!state->particles[j].active) {
                state->particles[j].active = true;
                state->particles[j].life = PARTICLE_LIFETIME;
                
                // Set particle position at the ship's rear
                state->particles[j].position.x = rearX;
                state->particles[j].position.y = rearY;
                
                // Add slight randomness to position
                state->particles[j].position.x += GetRandomValue(-3, 3);
                state->particles[j].position.y += GetRandomValue(-3, 3);
                
                // Set particle velocity in the opposite direction of the ship
                float particleAngle = radians + PI + GetRandomValue(-30, 30) * PI / 180.0f;
                state->particles[j].velocity.x = sin(particleAngle) * PARTICLE_SPEED;
                state->particles[j].velocity.y = -cos(particleAngle) * PARTICLE_SPEED;
                
                // Set particle appearance
                state->particles[j].radius = GetRandomValue(2, 5);
                
                // Different colors for visual interest - orange/red/yellow for engine exhaust
                int colorChoice = GetRandomValue(0, 2);
                if (colorChoice == 0)
                    state->particles[j].color = (Color){ 255, 120, 0, 255 };  // Orange
                else if (colorChoice == 1)
                    state->particles[j].color = (Color){ 255, 50, 0, 255 };   // Red-orange
                else
                    state->particles[j].color = (Color){ 255, 215, 0, 255 };  // Yellow
                
                break;
            }
        }
    }
}

void handleMenuInput(GameState* state) {
    Vector2 mousePoint = GetMousePosition();
    
    // Check if mouse is over the Play button
    bool isMouseOverPlayButton = CheckCollisionPointRec(mousePoint, state->playButton);
    
    // Check if mouse is over the Quit button
    bool isMouseOverQuitButton = CheckCollisionPointRec(mousePoint, state->quitButton);
    
    // Change to game state if play button is clicked
    if (isMouseOverPlayButton && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state->screenState = GAME_STATE;
    }
    
    // Exit the game if quit button is clicked
    if (isMouseOverQuitButton && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state->running = false;
    }
}

void renderMenu(const GameState* state) {
    BeginDrawing();
    
    // Clear screen with a very dark background for space
    ClearBackground((Color){5, 5, 15, 255});
    
    // Draw title
    const char* title = "ASTEROIDS";
    int titleWidth = MeasureText(title, TITLE_FONT_SIZE);
    DrawText(title, WINDOW_WIDTH/2 - titleWidth/2, WINDOW_HEIGHT/4, TITLE_FONT_SIZE, WHITE);
    
    // Draw play button
    Vector2 mousePoint = GetMousePosition();
    bool isMouseOverPlayButton = CheckCollisionPointRec(mousePoint, state->playButton);
    
    Color playButtonColor = isMouseOverPlayButton ? GREEN : DARKGREEN;
    DrawRectangleRec(state->playButton, playButtonColor);
    DrawRectangleLinesEx(state->playButton, 2, WHITE);
    
    // Draw play button text
    const char* playButtonText = "PLAY";
    int playButtonTextWidth = MeasureText(playButtonText, BUTTON_FONT_SIZE);
    DrawText(
        playButtonText,
        state->playButton.x + state->playButton.width/2 - playButtonTextWidth/2,
        state->playButton.y + state->playButton.height/2 - BUTTON_FONT_SIZE/2,
        BUTTON_FONT_SIZE,
        WHITE
    );
    
    // Draw quit button
    bool isMouseOverQuitButton = CheckCollisionPointRec(mousePoint, state->quitButton);
    
    Color quitButtonColor = isMouseOverQuitButton ? RED : MAROON;
    DrawRectangleRec(state->quitButton, quitButtonColor);
    DrawRectangleLinesEx(state->quitButton, 2, WHITE);
    
    // Draw quit button text
    const char* quitButtonText = "QUIT";
    int quitButtonTextWidth = MeasureText(quitButtonText, BUTTON_FONT_SIZE);
    DrawText(
        quitButtonText,
        state->quitButton.x + state->quitButton.width/2 - quitButtonTextWidth/2,
        state->quitButton.y + state->quitButton.height/2 - BUTTON_FONT_SIZE/2,
        BUTTON_FONT_SIZE,
        WHITE
    );
    
    // Draw instructions
    DrawText("Use WASD to move, Mouse to aim and shoot", 
             WINDOW_WIDTH/2 - MeasureText("Use WASD to move, Mouse to aim and shoot", 20)/2,
             WINDOW_HEIGHT - 150, 20, LIGHTGRAY);
    DrawText("Press R to reload", 
             WINDOW_WIDTH/2 - MeasureText("Press R to reload", 20)/2,
             WINDOW_HEIGHT - 120, 20, LIGHTGRAY);
    DrawText("Press P to pause", 
             WINDOW_WIDTH/2 - MeasureText("Press P to pause", 20)/2,
             WINDOW_HEIGHT - 90, 20, LIGHTGRAY);
    
    EndDrawing();
}

void handlePauseInput(GameState* state) {
    Vector2 mousePoint = GetMousePosition();
    
    // Check if mouse is over the Resume button
    bool isMouseOverResumeButton = CheckCollisionPointRec(mousePoint, state->resumeButton);
    
    // Check if mouse is over the Quit button
    bool isMouseOverQuitButton = CheckCollisionPointRec(mousePoint, state->quitButton);
    
    // Resume the game if resume button is clicked
    if (isMouseOverResumeButton && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state->screenState = GAME_STATE;
    }
    
    // Exit the game if quit button is clicked
    if (isMouseOverQuitButton && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state->running = false;
    }
    
    // Also check for P key to resume
    if (IsKeyPressed(KEY_P)) {
        state->screenState = GAME_STATE;
    }
}

void renderPause(const GameState* state) {
    // First, render the game underneath to show what's paused
    BeginDrawing();
    
    // Begin camera rendering
    BeginMode2D(state->camera);
    
    // Draw map boundaries
    DrawRectangleLines(0, 0, MAP_WIDTH, MAP_HEIGHT, BOUNDARY_COLOR);
    
    // Draw grid lines
    for (int i = 0; i < MAP_WIDTH; i += 200) {
        DrawLine(i, 0, i, MAP_HEIGHT, (Color){20, 20, 40, 100});
    }
    for (int i = 0; i < MAP_HEIGHT; i += 200) {
        DrawLine(0, i, MAP_WIDTH, i, (Color){20, 20, 40, 100});
    }
    
    // Draw particles
    renderParticles(state);
    
    // Draw ship
    renderGameObject(&state->ship.base, 3, state);
    
    // Draw bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (state->bullets[i].active) {
            DrawRectangle(
                state->bullets[i].x - state->bullets[i].radius, 
                state->bullets[i].y - state->bullets[i].radius, 
                state->bullets[i].radius * 2, 
                state->bullets[i].radius * 2, 
                YELLOW
            );
        }
    }
    
    // Draw asteroids
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (state->asteroids[i].base.active) {
            renderGameObject(&state->asteroids[i].base, 8, state); // Draw as octagon
        }
    }
    
    // Draw enemies
    renderEnemies(state);
    
    EndMode2D();
    
    // Draw semi-transparent overlay to darken the paused game
    DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, (Color){0, 0, 0, 150});
    
    // Draw pause title
    const char* pauseTitle = "PAUSED";
    int pauseTitleWidth = MeasureText(pauseTitle, TITLE_FONT_SIZE);
    DrawText(pauseTitle, WINDOW_WIDTH/2 - pauseTitleWidth/2, WINDOW_HEIGHT/4, TITLE_FONT_SIZE, WHITE);
    
    // Draw resume button
    Vector2 mousePoint = GetMousePosition();
    bool isMouseOverResumeButton = CheckCollisionPointRec(mousePoint, state->resumeButton);
    
    Color resumeButtonColor = isMouseOverResumeButton ? GREEN : DARKGREEN;
    DrawRectangleRec(state->resumeButton, resumeButtonColor);
    DrawRectangleLinesEx(state->resumeButton, 2, WHITE);
    
    // Draw resume button text
    const char* resumeButtonText = "RESUME";
    int resumeButtonTextWidth = MeasureText(resumeButtonText, BUTTON_FONT_SIZE);
    DrawText(
        resumeButtonText,
        state->resumeButton.x + state->resumeButton.width/2 - resumeButtonTextWidth/2,
        state->resumeButton.y + state->resumeButton.height/2 - BUTTON_FONT_SIZE/2,
        BUTTON_FONT_SIZE,
        WHITE
    );
    
    // Draw quit button
    bool isMouseOverQuitButton = CheckCollisionPointRec(mousePoint, state->quitButton);
    
    Color quitButtonColor = isMouseOverQuitButton ? RED : MAROON;
    DrawRectangleRec(state->quitButton, quitButtonColor);
    DrawRectangleLinesEx(state->quitButton, 2, WHITE);
    
    // Draw quit button text
    const char* quitButtonText = "QUIT";
    int quitButtonTextWidth = MeasureText(quitButtonText, BUTTON_FONT_SIZE);
    DrawText(
        quitButtonText,
        state->quitButton.x + state->quitButton.width/2 - quitButtonTextWidth/2,
        state->quitButton.y + state->quitButton.height/2 - BUTTON_FONT_SIZE/2,
        BUTTON_FONT_SIZE,
        WHITE
    );
    
    EndDrawing();
}

void loadSounds(GameState* state) {
    if (state->soundLoaded) return;
    
    // Initialize audio device
    InitAudioDevice();
    
    // Load sound effects
    state->sounds[SOUND_SHOOT] = LoadSound("resources/shoot.wav");
    state->sounds[SOUND_RELOAD_START] = LoadSound("resources/reload_start.wav");
    state->sounds[SOUND_RELOAD_FINISH] = LoadSound("resources/reload_finish.wav");
    state->sounds[SOUND_ASTEROID_HIT] = LoadSound("resources/asteroid_hit.wav");
    state->sounds[SOUND_SHIP_HIT] = LoadSound("resources/ship_hit.wav");
    state->sounds[SOUND_ENEMY_SHOOT] = LoadSound("resources/enemy_shoot.wav");
    state->sounds[SOUND_ENEMY_EXPLODE] = LoadSound("resources/enemy_explode.wav");
    
    state->soundLoaded = true;
}

void spawnEnemy(GameState* state, EnemyType type) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!state->enemies[i].base.active) {
            state->enemies[i].base.active = true;
            state->enemies[i].type = type;
            state->enemies[i].fireTimer = 0.0f;
            state->enemies[i].burstCount = 0;
            state->enemies[i].burstTimer = 0.0f;
            state->enemies[i].isBursting = false;
            state->enemies[i].moveTimer = GetRandomValue(3, 6);
            state->enemies[i].moveAngle = GetRandomValue(0, 359) * PI / 180.0f;
            
            // Set enemy properties based on type
            if (type == ENEMY_TANK) {
                state->enemies[i].base.radius = TANK_ENEMY_RADIUS;
                state->enemies[i].health = TANK_ENEMY_HEALTH;
            } else {
                state->enemies[i].base.radius = SCOUT_ENEMY_RADIUS;
                state->enemies[i].health = SCOUT_ENEMY_HEALTH;
            }
            
            // Spawn enemies away from the player
            do {
                state->enemies[i].base.x = GetRandomValue(
                    state->enemies[i].base.radius, 
                    MAP_WIDTH - state->enemies[i].base.radius
                );
                
                state->enemies[i].base.y = GetRandomValue(
                    state->enemies[i].base.radius, 
                    MAP_HEIGHT - state->enemies[i].base.radius
                );
                
            } while (sqrt(pow(state->enemies[i].base.x - state->ship.base.x, 2) + 
                         pow(state->enemies[i].base.y - state->ship.base.y, 2)) < 400);
            
            break;
        }
    }
}

void updateEnemies(GameState* state, float deltaTime) {
    // Update enemy spawn timer
    state->enemySpawnTimer -= deltaTime;
    if (state->enemySpawnTimer <= 0) {
        // Spawn either a tank or scout enemy
        EnemyType type = (GetRandomValue(0, 1) == 0) ? ENEMY_TANK : ENEMY_SCOUT;
        spawnEnemy(state, type);
        
        // Reset timer with some randomness
        state->enemySpawnTimer = ENEMY_SPAWN_TIME + GetRandomValue(-3, 3);
    }
    
    // Update each enemy
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (state->enemies[i].base.active) {
            Enemy* enemy = &state->enemies[i];
            
            // Calculate distance to player
            float dx = state->ship.base.x - enemy->base.x;
            float dy = state->ship.base.y - enemy->base.y;
            float distanceToPlayer = sqrt(dx * dx + dy * dy);
            
            // Calculate angle to player
            float angleToPlayer = atan2(dx, -dy) * 180.0f / PI;
            enemy->base.angle = angleToPlayer;
            
            // Update movement based on enemy type
            if (enemy->type == ENEMY_TANK) {
                // Tank moves directly toward player when in detection range
                if (distanceToPlayer < ENEMY_DETECTION_RADIUS) {
                    // Move slowly toward player
                    if (distanceToPlayer > TANK_ENEMY_ATTACK_DISTANCE) {
                        enemy->base.dx = TANK_ENEMY_SPEED * sin(angleToPlayer * PI / 180.0f);
                        enemy->base.dy = -TANK_ENEMY_SPEED * cos(angleToPlayer * PI / 180.0f);
                    } else {
                        // Stop when at attack distance
                        enemy->base.dx *= 0.9f;
                        enemy->base.dy *= 0.9f;
                    }
                    
                    // Fire at player when close enough
                    enemy->fireTimer -= deltaTime;
                    if (enemy->fireTimer <= 0 && distanceToPlayer < ENEMY_DETECTION_RADIUS) {
                        fireEnemyWeapon(state, enemy);
                        enemy->fireTimer = TANK_ENEMY_FIRE_RATE;
                    }
                } else {
                    // Random movement when player not detected
                    enemy->moveTimer -= deltaTime;
                    if (enemy->moveTimer <= 0) {
                        enemy->moveAngle = GetRandomValue(0, 359) * PI / 180.0f;
                        enemy->moveTimer = GetRandomValue(3, 6);
                    }
                    
                    enemy->base.dx = TANK_ENEMY_SPEED * 0.5f * cos(enemy->moveAngle);
                    enemy->base.dy = TANK_ENEMY_SPEED * 0.5f * sin(enemy->moveAngle);
                }
            } else {  // ENEMY_SCOUT
                // Scout has more complex movement
                if (distanceToPlayer < ENEMY_DETECTION_RADIUS) {
                    // Approach to attack distance
                    if (distanceToPlayer > SCOUT_ENEMY_ATTACK_DISTANCE * 1.5f) {
                        enemy->base.dx = SCOUT_ENEMY_SPEED * sin(angleToPlayer * PI / 180.0f);
                        enemy->base.dy = -SCOUT_ENEMY_SPEED * cos(angleToPlayer * PI / 180.0f);
                    } 
                    // Circle the player at attack distance
                    else if (distanceToPlayer > SCOUT_ENEMY_ATTACK_DISTANCE * 0.8f) {
                        // Calculate perpendicular angle for circling
                        float circlingAngle = angleToPlayer + 90.0f; 
                        if (GetRandomValue(0, 1) == 0) {
                            circlingAngle = angleToPlayer - 90.0f;  // Randomize circle direction
                        }
                        circlingAngle *= PI / 180.0f;
                        
                        enemy->base.dx = SCOUT_ENEMY_SPEED * cos(circlingAngle);
                        enemy->base.dy = SCOUT_ENEMY_SPEED * sin(circlingAngle);
                    }
                    // Back away if too close
                    else {
                        enemy->base.dx = -SCOUT_ENEMY_SPEED * sin(angleToPlayer * PI / 180.0f);
                        enemy->base.dy = SCOUT_ENEMY_SPEED * cos(angleToPlayer * PI / 180.0f);
                    }
                    
                    // Burst fire logic
                    if (!enemy->isBursting) {
                        enemy->fireTimer -= deltaTime;
                        if (enemy->fireTimer <= 0 && distanceToPlayer < ENEMY_DETECTION_RADIUS) {
                            enemy->isBursting = true;
                            enemy->burstCount = 0;
                            enemy->burstTimer = 0;
                        }
                    } else {
                        enemy->burstTimer -= deltaTime;
                        if (enemy->burstTimer <= 0) {
                            fireEnemyWeapon(state, enemy);
                            enemy->burstCount++;
                            enemy->burstTimer = SCOUT_ENEMY_BURST_DELAY;
                            
                            // End burst after firing enough shots
                            if (enemy->burstCount >= SCOUT_ENEMY_BURST_COUNT) {
                                enemy->isBursting = false;
                                enemy->fireTimer = SCOUT_ENEMY_FIRE_RATE;
                            }
                        }
                    }
                } else {
                    // Random fast movement when player not detected
                    enemy->moveTimer -= deltaTime;
                    if (enemy->moveTimer <= 0) {
                        enemy->moveAngle = GetRandomValue(0, 359) * PI / 180.0f;
                        enemy->moveTimer = GetRandomValue(1, 3);
                    }
                    
                    enemy->base.dx = SCOUT_ENEMY_SPEED * 0.7f * cos(enemy->moveAngle);
                    enemy->base.dy = SCOUT_ENEMY_SPEED * 0.7f * sin(enemy->moveAngle);
                }
            }
            
            // Update enemy position with boundary checking
            float newX = enemy->base.x + enemy->base.dx;
            float newY = enemy->base.y + enemy->base.dy;
            
            if (newX - enemy->base.radius >= 0 && newX + enemy->base.radius <= MAP_WIDTH) {
                enemy->base.x = newX;
            } else {
                enemy->base.dx *= -1;
                enemy->moveAngle = PI - enemy->moveAngle;
            }
            
            if (newY - enemy->base.radius >= 0 && newY + enemy->base.radius <= MAP_HEIGHT) {
                enemy->base.y = newY;
            } else {
                enemy->base.dy *= -1;
                enemy->moveAngle = -enemy->moveAngle;
            }
            
            // Check for collisions with player bullets
            for (int j = 0; j < MAX_BULLETS; j++) {
                if (state->bullets[j].active && checkCollision(&enemy->base, &state->bullets[j])) {
                    state->bullets[j].active = false;
                    enemy->health -= 10;  // Each player bullet deals 10 damage
                    
                    if (enemy->health <= 0) {
                        // Enemy destroyed
                        enemy->base.active = false;
                        
                        // Add score based on enemy type
                        state->score += (enemy->type == ENEMY_TANK) ? TANK_ENEMY_SCORE : SCOUT_ENEMY_SCORE;
                        
                        // Play explosion sound
                        if (state->soundLoaded) {
                            PlaySound(state->sounds[SOUND_ENEMY_EXPLODE]);
                        }
                        
                        // Generate explosion particles
                        for (int k = 0; k < 20; k++) {
                            for (int p = 0; p < MAX_PARTICLES; p++) {
                                if (!state->particles[p].active) {
                                    state->particles[p].active = true;
                                    state->particles[p].life = PARTICLE_LIFETIME;
                                    state->particles[p].position.x = enemy->base.x;
                                    state->particles[p].position.y = enemy->base.y;
                                    
                                    float particleAngle = GetRandomValue(0, 359) * PI / 180.0f;
                                    float particleSpeed = PARTICLE_SPEED * GetRandomValue(50, 150) / 100.0f;
                                    state->particles[p].velocity.x = cos(particleAngle) * particleSpeed;
                                    state->particles[p].velocity.y = sin(particleAngle) * particleSpeed;
                                    
                                    state->particles[p].radius = GetRandomValue(2, 6);
                                    
                                    // Enemy explosion colors - reddish
                                    state->particles[p].color = (Color){ 
                                        GetRandomValue(200, 255), 
                                        GetRandomValue(50, 100),
                                        GetRandomValue(0, 50),
                                        255
                                    };
                                    
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Update enemy bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (state->enemyBullets[i].base.active) {
            state->enemyBullets[i].base.x += state->enemyBullets[i].base.dx;
            state->enemyBullets[i].base.y += state->enemyBullets[i].base.dy;
            
            // Check if bullet is out of bounds
            if (state->enemyBullets[i].base.x < 0 || state->enemyBullets[i].base.x > MAP_WIDTH || 
                state->enemyBullets[i].base.y < 0 || state->enemyBullets[i].base.y > MAP_HEIGHT) {
                state->enemyBullets[i].base.active = false;
                continue;
            }
            
            // Check for collision with player
            if (checkCollision(&state->ship.base, &state->enemyBullets[i].base)) {
                state->enemyBullets[i].base.active = false;
                state->health -= state->enemyBullets[i].damage;
                
                // Play hit sound
                if (state->soundLoaded) {
                    PlaySound(state->sounds[SOUND_SHIP_HIT]);
                }
                
                if (state->health <= 0) {
                    state->lives--;
                    if (state->lives <= 0) {
                        printf("Game Over! Final Score: %d\n", state->score);
                        state->running = false;
                    } else {
                        resetShip(state);
                    }
                }
            }
        }
    }
}

void fireEnemyWeapon(GameState* state, Enemy* enemy) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!state->enemyBullets[i].base.active) {
            state->enemyBullets[i].base.active = true;
            
            // Start the bullet at the enemy's position
            state->enemyBullets[i].base.x = enemy->base.x;
            state->enemyBullets[i].base.y = enemy->base.y;
            state->enemyBullets[i].base.radius = (enemy->type == ENEMY_TANK) ? 4.0f : 2.0f;
            
            // Set bullet properties based on enemy type
            if (enemy->type == ENEMY_TANK) {
                state->enemyBullets[i].damage = TANK_ENEMY_BULLET_DAMAGE;
            } else {
                state->enemyBullets[i].damage = SCOUT_ENEMY_BULLET_DAMAGE;
            }
            
            // Calculate direction to player
            float dx = state->ship.base.x - enemy->base.x;
            float dy = state->ship.base.y - enemy->base.y;
            float length = sqrt(dx * dx + dy * dy);
            
            // Add a bit of inaccuracy for scout enemies
            float inaccuracy = (enemy->type == ENEMY_SCOUT) ? GetRandomValue(-10, 10) * PI / 180.0f : 0;
            float angle = atan2(dx, -dy) + inaccuracy;
            
            // Normalize the direction
            if (length > 0) {
                dx = sin(angle);
                dy = -cos(angle);
            }
            
            // Set bullet velocity
            float bulletSpeed = ENEMY_BULLET_SPEED;
            state->enemyBullets[i].base.dx = dx * bulletSpeed;
            state->enemyBullets[i].base.dy = dy * bulletSpeed;
            
            // Play shooting sound effect
            if (state->soundLoaded) {
                PlaySound(state->sounds[SOUND_ENEMY_SHOOT]);
            }
            
            break;
        }
    }
}

void renderEnemies(const GameState* state) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (state->enemies[i].base.active) {
            // Draw enemy based on type
            if (state->enemies[i].type == ENEMY_TANK) {
                // Draw tank enemy as pentagon with red color
                Vector2 points[5];
                for (int j = 0; j < 5; j++) {
                    float angle = state->enemies[i].base.angle + j * 72.0f;  // 360 / 5 = 72 degrees
                    float radians = angle * PI / 180.0f;
                    points[j].x = state->enemies[i].base.x + sin(radians) * state->enemies[i].base.radius;
                    points[j].y = state->enemies[i].base.y - cos(radians) * state->enemies[i].base.radius;
                }
                
                for (int j = 0; j < 5; j++) {
                    int next = (j + 1) % 5;
                    DrawLineV(points[j], points[next], RED);
                }
                
                // Draw a gun barrel pointing toward player
                float radians = state->enemies[i].base.angle * PI / 180.0f;
                Vector2 barrelStart = {
                    state->enemies[i].base.x, 
                    state->enemies[i].base.y
                };
                Vector2 barrelEnd = {
                    state->enemies[i].base.x + sin(radians) * state->enemies[i].base.radius * 1.5f,
                    state->enemies[i].base.y - cos(radians) * state->enemies[i].base.radius * 1.5f
                };
                DrawLineV(barrelStart, barrelEnd, RED);
            } else {
                // Draw scout enemy as triangle with blue color
                Vector2 points[3];
                
                // Front point
                float radians = state->enemies[i].base.angle * PI / 180.0f;
                points[0].x = state->enemies[i].base.x + sin(radians) * state->enemies[i].base.radius * 1.5f;
                points[0].y = state->enemies[i].base.y - cos(radians) * state->enemies[i].base.radius * 1.5f;
                
                // Left wing
                float leftAngle = radians + PI * 0.8f;
                points[1].x = state->enemies[i].base.x + sin(leftAngle) * state->enemies[i].base.radius;
                points[1].y = state->enemies[i].base.y - cos(leftAngle) * state->enemies[i].base.radius;
                
                // Right wing
                float rightAngle = radians - PI * 0.8f;
                points[2].x = state->enemies[i].base.x + sin(rightAngle) * state->enemies[i].base.radius;
                points[2].y = state->enemies[i].base.y - cos(rightAngle) * state->enemies[i].base.radius;
                
                // Draw the lines
                DrawLineV(points[0], points[1], SKYBLUE);
                DrawLineV(points[1], points[2], SKYBLUE);
                DrawLineV(points[2], points[0], SKYBLUE);
            }
            
            // Draw health bar above enemy
            if (state->Debug) {
                int barWidth = state->enemies[i].base.radius * 2;
                int barHeight = 5;
                int barX = state->enemies[i].base.x - barWidth / 2;
                int barY = state->enemies[i].base.y - state->enemies[i].base.radius - 12;
                
                float maxHealth = (state->enemies[i].type == ENEMY_TANK) ? TANK_ENEMY_HEALTH : SCOUT_ENEMY_HEALTH;
                float healthPercent = (float)state->enemies[i].health / maxHealth;
                int currentHealthWidth = (int)(barWidth * healthPercent);
                
                // Background of health bar
                DrawRectangle(barX, barY, barWidth, barHeight, DARKGRAY);
                
                // Filled part of health bar
                Color healthColor = (healthPercent > 0.5f) ? GREEN : RED;
                DrawRectangle(barX, barY, currentHealthWidth, barHeight, healthColor);
            }
        }
    }
    
    // Draw enemy bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (state->enemyBullets[i].base.active) {
            Color bulletColor = (state->enemyBullets[i].damage >= TANK_ENEMY_BULLET_DAMAGE) ? RED : BLUE;
            DrawCircleV((Vector2){state->enemyBullets[i].base.x, state->enemyBullets[i].base.y}, 
                       state->enemyBullets[i].base.radius, bulletColor);
        }
    }
}