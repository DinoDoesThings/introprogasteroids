#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <string.h>

// custom headers
#include "typedefs.h"
#include "config.h"
#include "asteroids.h"
#include "collisions.h"
#include "initialize.h"
#include "particles.h"
#include "powerups.h"
#include "resources.h"

// Forward declarations for new helper functions
void updateEnemySpawner(GameState* state, float deltaTime);
float calculateDistanceSquared(float x1, float y1, float x2, float y2);
float calculateAngleToTarget(float srcX, float srcY, float targetX, float targetY);
Vector2 calculateAsteroidAvoidance(GameState* state, Enemy* enemy);
int collectScoutData(GameState* state, Vector2* scoutPositions, int* scoutIndices, int* groupDesires);
void updateTankBehavior(GameState* state, Enemy* enemy, float deltaTime, float distanceToPlayer, float angleToPlayer, Vector2 avoidVector);
void updateScoutBehavior(GameState* state, Enemy* enemy, int enemyIndex, float deltaTime, float distanceToPlayer, float angleToPlayer, Vector2 avoidVector, Vector2* scoutPositions, int* scoutIndices, int* groupDesires, int activeScouts);
void updateEnemyPosition(GameState* state, Enemy* enemy);
bool handleAsteroidCollisions(GameState* state, Enemy* enemy, int enemyIndex);
void handleBulletCollisions(GameState* state, Enemy* enemy, int enemyIndex);
void updateEnemyBullets(GameState* state, float deltaTime);
void createEnemyExplosion(GameState* state, float x, float y, int particleCount);

void spawnEnemy(GameState* state, EnemyType type) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!state->enemies[i].base.active) {
            // Initialize enemy properties
            state->enemies[i].base.active = true;
            state->enemies[i].type = type;
            state->enemies[i].base.angle = 0.0f;
            state->enemies[i].base.dx = 0.0f;
            state->enemies[i].base.dy = 0.0f;
            state->enemies[i].fireTimer = 0.0f;
            state->enemies[i].isBursting = false;
            state->enemies[i].burstCount = 0;
            state->enemies[i].burstTimer = 0.0f;
            state->enemies[i].moveTimer = 0.0f;
            state->enemies[i].moveAngle = GetRandomValue(0, 359) * PI / 180.0f;
            
            // Set health and radius based on type
            if (type == ENEMY_TANK) {
                state->enemies[i].base.radius = TANK_ENEMY_RADIUS;
                state->enemies[i].health = TANK_ENEMY_HEALTH;
                state->enemies[i].texture = loadTextureOnce(TANK_TEXTURE_PATH);
            } else {
                state->enemies[i].base.radius = SCOUT_ENEMY_RADIUS;
                state->enemies[i].health = SCOUT_ENEMY_HEALTH;
                state->enemies[i].texture = loadTextureOnce(SCOUT_TEXTURE_PATH);
            }
            
            // Try to find a safe spawn location
            bool validPosition = false;
            int attempts = 0;
            const int MAX_ATTEMPTS = 50;
            
            while (!validPosition && attempts < MAX_ATTEMPTS) {
                attempts++;
                validPosition = true;
                
                // Generate random position
                state->enemies[i].base.x = GetRandomValue(
                    state->enemies[i].base.radius, 
                    MAP_WIDTH - state->enemies[i].base.radius
                );
                
                state->enemies[i].base.y = GetRandomValue(
                    state->enemies[i].base.radius, 
                    MAP_HEIGHT - state->enemies[i].base.radius
                );
                
                // Check distance from player 
                float playerDistanceSquared = calculateDistanceSquared(
                    state->enemies[i].base.x, state->enemies[i].base.y,
                    state->ship.base.x, state->ship.base.y
                );
                
                // Too close to player?
                if (playerDistanceSquared < 160000) { // 400^2
                    validPosition = false;
                    continue;
                }
                
                // Check for collisions with asteroids
                for (int j = 0; j < MAX_ASTEROIDS; j++) {
                    if (!state->asteroids[j].base.active) continue;
                    
                    // Calculate safe distance from asteroid
                    float safeDistance = state->enemies[i].base.radius + 
                                         state->asteroids[j].base.radius + 20.0f; 
                    
                    float astDistSq = calculateDistanceSquared(
                        state->enemies[i].base.x, state->enemies[i].base.y,
                        state->asteroids[j].base.x, state->asteroids[j].base.y
                    );
                    
                    if (astDistSq < safeDistance * safeDistance) {
                        validPosition = false;
                        break;
                    }
                }
            }
            
            // If we couldn't find a safe position after max attempts,
            // just use the last position and hope for the best ¯\_(ツ)_/¯
            break;
        }
    }
}

void fireEnemyWeapon(GameState* state, Enemy* enemy) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!state->enemyBullets[i].base.active) {
            state->enemyBullets[i].base.active = true;
            
            // Mark as enemy bullet
            state->enemyBullets[i].isPlayerBullet = false;

            // Start the bullet at the enemy's position
            state->enemyBullets[i].base.x = enemy->base.x;
            state->enemyBullets[i].base.y = enemy->base.y;
            state->enemyBullets[i].base.radius = (enemy->type == ENEMY_TANK) ? 6.0f : 2.0f;
            
            // Set bullet properties based on enemy type
            if (enemy->type == ENEMY_TANK) {
                state->enemyBullets[i].damage = TANK_ENEMY_BULLET_DAMAGE;
                state->enemyBullets[i].type = BULLET_GRENADE;
                state->enemyBullets[i].timer = TANK_GRENADE_TIMER;
                state->enemyBullets[i].hasExploded = false;
            } else {
                state->enemyBullets[i].damage = SCOUT_ENEMY_BULLET_DAMAGE;
                state->enemyBullets[i].type = BULLET_NORMAL;
                state->enemyBullets[i].timer = 0.0f;
                state->enemyBullets[i].hasExploded = false;
            }
            
            // Calculate direction to player
            float dx = state->ship.base.x - enemy->base.x;
            float dy = state->ship.base.y - enemy->base.y;
            
            // Add a bit of inaccuracy for scout enemies
            float inaccuracy = (enemy->type == ENEMY_SCOUT) ? GetRandomValue(-10, 10) * PI / 180.0f : 0;
            float angle = atan2(dx, -dy) + inaccuracy;
            
            // Set bullet velocity (slower for grenades)
            float bulletSpeed = (enemy->type == ENEMY_TANK) ? ENEMY_BULLET_SPEED * 0.7f : ENEMY_BULLET_SPEED;
            state->enemyBullets[i].base.dx = sin(angle) * bulletSpeed;
            state->enemyBullets[i].base.dy = -cos(angle) * bulletSpeed;
            
            // Play shooting sound effect
            if (state->soundLoaded) {
                if (enemy->type == ENEMY_TANK) {
                    PlaySound(state->sounds[SOUND_TANK_SHOOT]);
                } else {
                    PlaySound(state->sounds[SOUND_SCOUT_SHOOT]);
                }
            }
            
            break;
        }
    }
}

void explodeGrenade(GameState* state, int grenadeIndex) {
    Bullet* grenade = &state->enemyBullets[grenadeIndex];
    bool isPlayerGrenade = grenade->isPlayerBullet;
    
    // Create explosion particles
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < MAX_PARTICLES; j++) {
            if (!state->particles[j].active) {
                state->particles[j].active = true;
                state->particles[j].life = PARTICLE_LIFETIME * 0.8f;
                state->particles[j].position.x = grenade->base.x;
                state->particles[j].position.y = grenade->base.y;
                
                // Add randomness to particle position
                state->particles[j].position.x += GetRandomValue(-5, 5);
                state->particles[j].position.y += GetRandomValue(-5, 5);
                
                // Set particle velocity outward from explosion center
                float particleAngle = GetRandomValue(0, 359) * PI / 180.0f;
                float particleSpeed = PARTICLE_SPEED * GetRandomValue(80, 150) / 100.0f;
                state->particles[j].velocity.x = cos(particleAngle) * particleSpeed;
                state->particles[j].velocity.y = sin(particleAngle) * particleSpeed;
                
                state->particles[j].radius = GetRandomValue(2, 5);
                
                // Orange explosion color for player grenades, red for enemy
                if (isPlayerGrenade) {
                    state->particles[j].color = (Color){ 255, 165, 0, 255 };  // Orange
                } else {
                    state->particles[j].color = (Color){ 255, 100, 0, 255 };  // Red-orange
                }
                
                break;
            }
        }
    }
    
    // Create explosion bullets in cardinal and intercardinal directions
    float directions[8][2] = {
        {0, -1},      // North
        {0.707f, -0.707f},  // Northeast
        {1, 0},       // East
        {0.707f, 0.707f},   // Southeast
        {0, 1},       // South
        {-0.707f, 0.707f},  // Southwest
        {-1, 0},      // West
        {-0.707f, -0.707f}  // Northwest
    };
    
    int explosionCount = isPlayerGrenade ? PLAYER_GRENADE_EXPLOSION_COUNT : TANK_GRENADE_EXPLOSION_COUNT;
    float explosionSpeed = isPlayerGrenade ? PLAYER_GRENADE_EXPLOSION_SPEED : TANK_GRENADE_EXPLOSION_SPEED;
    int explosionDamage = isPlayerGrenade ? PLAYER_GRENADE_EXPLOSION_DAMAGE : TANK_GRENADE_EXPLOSION_DAMAGE;
    
    for (int dir = 0; dir < explosionCount; dir++) {
        // Find an available bullet slot
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            if (!state->enemyBullets[i].base.active) {
                state->enemyBullets[i].base.active = true;
                state->enemyBullets[i].base.x = grenade->base.x;
                state->enemyBullets[i].base.y = grenade->base.y;
                state->enemyBullets[i].base.radius = 3.0f;
                state->enemyBullets[i].damage = explosionDamage;
                state->enemyBullets[i].type = BULLET_NORMAL;
                state->enemyBullets[i].timer = 0.0f;
                state->enemyBullets[i].hasExploded = false;
                state->enemyBullets[i].isPlayerBullet = isPlayerGrenade; // Maintain player ownership
                
                // Set velocity in the specified direction
                state->enemyBullets[i].base.dx = directions[dir][0] * explosionSpeed;
                state->enemyBullets[i].base.dy = directions[dir][1] * explosionSpeed;
                
                break;
            }
        }
    }
    
    // Play explosion sound
    if (state->soundLoaded) {
        PlaySound(state->sounds[SOUND_ENEMY_EXPLODE]);
    }
    
    // Mark grenade as exploded and deactivate it
    grenade->hasExploded = true;
    grenade->base.active = false;
}

// Main enemy update function refactored into smaller parts
void updateEnemies(GameState* state, float deltaTime) {
    // Handle enemy spawning
    updateEnemySpawner(state, deltaTime);
    
    // Collect scout data for group behavior - moved from inside the loop
    Vector2 scoutPositions[MAX_ENEMIES];
    int scoutIndices[MAX_ENEMIES];
    int groupDesires[MAX_ENEMIES];
    int activeScouts = collectScoutData(state, scoutPositions, scoutIndices, groupDesires);
    
    // Process each enemy
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!state->enemies[i].base.active) continue;
        
        Enemy* enemy = &state->enemies[i];
        
        // Calculate common values once (optimization)
        float distanceSquared = calculateDistanceSquared(
            enemy->base.x, enemy->base.y, 
            state->ship.base.x, state->ship.base.y
        );
        float distanceToPlayer = sqrt(distanceSquared); // Only calculate sqrt when needed
        
        float angleToPlayer = calculateAngleToTarget(
            enemy->base.x, enemy->base.y,
            state->ship.base.x, state->ship.base.y
        );
        enemy->base.angle = angleToPlayer; // Set enemy facing direction
        
        // Calculate asteroid avoidance once
        Vector2 avoidVector = calculateAsteroidAvoidance(state, enemy);
        
        // Update enemy based on type
        if (enemy->type == ENEMY_TANK) {
            updateTankBehavior(state, enemy, deltaTime, distanceToPlayer, angleToPlayer, avoidVector);
        } else {
            updateScoutBehavior(state, enemy, i, deltaTime, distanceToPlayer, angleToPlayer, 
                                avoidVector, scoutPositions, scoutIndices, groupDesires, activeScouts);
        }
        
        // Update position with boundary checking
        updateEnemyPosition(state, enemy);
        
        // Handle collisions
        if (handleAsteroidCollisions(state, enemy, i)) {
            continue; // Enemy destroyed, skip rest of processing
        }
        
        handleBulletCollisions(state, enemy, i);
    }
    
    // Update enemy bullets
    updateEnemyBullets(state, deltaTime);
}

// Enemy spawning logic
void updateEnemySpawner(GameState* state, float deltaTime) {
    if (state->currentWave < SCOUT_START_WAVE || state->inWaveTransition) {
        return;
    }
    
    state->enemySpawnTimer -= deltaTime;
    
    if (state->enemySpawnTimer <= 0 && state->enemiesSpawnedThisWave < state->maxEnemiesThisWave) {
        // Determine which enemy type to spawn
        EnemyType type = ENEMY_SCOUT; // Default to scout
        
        if (state->currentWave >= TANK_START_WAVE) {
            // Higher chance of tank at higher waves
            int tankChance = 20 + (state->currentWave - TANK_START_WAVE) * 5;
            tankChance = (tankChance > 50) ? 50 : tankChance; // Cap at 50%
            
            // Roll for tank/scout
            if (GetRandomValue(1, 100) <= tankChance) {
                type = ENEMY_TANK;
            }
        }
        
        spawnEnemy(state, type);
        
        // Increment counter for enemies spawned this wave
        state->enemiesSpawnedThisWave++;
        
        // Check if we've spawned all enemies for this wave
        if (state->enemiesSpawnedThisWave >= state->maxEnemiesThisWave) {
            state->EnemySpawnComplete = 1;
        }
        
        // Reset timer with some randomness
        float baseTime = ENEMY_SPAWN_TIME - (state->currentWave - SCOUT_START_WAVE) * 1.0f;
        baseTime = (baseTime < 3.0f) ? 3.0f : baseTime;
        state->enemySpawnTimer = baseTime + GetRandomValue(-100, 100) / 100.0f;
    }
}

// Calculate squared distance (optimization to avoid sqrt calls)
float calculateDistanceSquared(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy;
}

// Calculate angle to target
float calculateAngleToTarget(float srcX, float srcY, float targetX, float targetY) {
    float dx = targetX - srcX;
    float dy = targetY - srcY;
    return atan2(dx, -dy) * 180.0f / PI;
}

// Calculate asteroid avoidance vector
Vector2 calculateAsteroidAvoidance(GameState* state, Enemy* enemy) {
    Vector2 avoidVector = {0, 0};
    float avoidanceWeight = 2.0f;
    float detectionDistance = enemy->base.radius * 5.0f;
    
    // Check nearby asteroids to generate avoidance vector
    for (int j = 0; j < MAX_ASTEROIDS; j++) {
        if (!state->asteroids[j].base.active) continue;
        
        // Calculate distance and direction to asteroid (using squared distance optimization)
        float ax = state->asteroids[j].base.x - enemy->base.x;
        float ay = state->asteroids[j].base.y - enemy->base.y;
        float asteroidDistSq = ax * ax + ay * ay;
        float asteroidRadius = state->asteroids[j].base.radius;
        float detectionThreshold = detectionDistance + asteroidRadius;
        float detectionThresholdSq = detectionThreshold * detectionThreshold;
        
        // If asteroid is close enough to avoid - use squared distance for optimization
        if (asteroidDistSq < detectionThresholdSq) {
            float asteroidDist = sqrt(asteroidDistSq); // Only calculate sqrt when needed
            
            // Calculate normalized direction vector away from asteroid
            float weight = 1.0f - asteroidDist / detectionThreshold;
            weight = weight * weight * avoidanceWeight; // Make avoidance stronger when closer
            
            // Avoid division by zero
            if (asteroidDist < 0.1f) asteroidDist = 0.1f;
            
            // Add weighted avoidance vector (away from asteroid)
            avoidVector.x -= (ax / asteroidDist) * weight;
            avoidVector.y -= (ay / asteroidDist) * weight;
        }
    }
    
    // Normalize avoidance vector if it's not zero
    float avoidanceMagnitude = sqrt(avoidVector.x * avoidVector.x + avoidVector.y * avoidVector.y);
    if (avoidanceMagnitude > 0) {
        avoidVector.x /= avoidanceMagnitude;
        avoidVector.y /= avoidanceMagnitude;
    }
    
    return avoidVector;
}

// Collect scout positions and group desire data
int collectScoutData(GameState* state, Vector2* scoutPositions, int* scoutIndices, int* groupDesires) {
    int activeScouts = 0;
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (state->enemies[i].base.active && state->enemies[i].type == ENEMY_SCOUT) {
            scoutPositions[activeScouts] = (Vector2){state->enemies[i].base.x, state->enemies[i].base.y};
            scoutIndices[activeScouts] = i;
            
            // Determine if this scout wants to group based on its index (consistent behavior)
            groupDesires[activeScouts] = (i % 100 < SCOUT_GROUP_CHANCE) ? 1 : 0;
            
            activeScouts++;
        }
    }
    
    return activeScouts;
}

// Tank enemy behavior update
void updateTankBehavior(GameState* state, Enemy* enemy, float deltaTime, float distanceToPlayer, float angleToPlayer, Vector2 avoidVector) {
    float avoidanceMagnitude = sqrt(avoidVector.x * avoidVector.x + avoidVector.y * avoidVector.y);
    
    // Tank moves directly toward player when in detection range
    if (distanceToPlayer < ENEMY_DETECTION_RADIUS) {
        // Move slowly toward player
        if (distanceToPlayer > TANK_ENEMY_ATTACK_DISTANCE) {
            // Base movement direction toward player
            float targetDx = TANK_ENEMY_SPEED * sin(angleToPlayer * PI / 180.0f);
            float targetDy = -TANK_ENEMY_SPEED * cos(angleToPlayer * PI / 180.0f);
            
            // Apply asteroid avoidance
            if (avoidanceMagnitude > 0) {
                targetDx += avoidVector.x * TANK_ENEMY_SPEED * 1.5f;
                targetDy += avoidVector.y * TANK_ENEMY_SPEED * 1.5f;
            }
            
            // Apply smoothed movement
            enemy->base.dx = enemy->base.dx * 0.8f + targetDx * 0.2f;
            enemy->base.dy = enemy->base.dy * 0.8f + targetDy * 0.2f;
            
            // Emit thrust particles when moving toward player
            emitEnemyThrustParticles(state, enemy, 1);
        } else {
            // Stop when at attack distance, but still avoid asteroids
            if (avoidanceMagnitude > 0) {
                enemy->base.dx = avoidVector.x * TANK_ENEMY_SPEED * 1.5f;
                enemy->base.dy = avoidVector.y * TANK_ENEMY_SPEED * 1.5f;
            } else {
                enemy->base.dx *= 0.9f;
                enemy->base.dy *= 0.9f;
            }
        }
        
        // Fire at player when close enough
        enemy->fireTimer -= deltaTime;
        if (enemy->fireTimer <= 0 && distanceToPlayer < ENEMY_DETECTION_RADIUS) {
            fireEnemyWeapon(state, enemy);
            enemy->fireTimer = TANK_ENEMY_FIRE_RATE;
        }
    } else {
        // Random movement when player not detected, still with asteroid avoidance
        enemy->moveTimer -= deltaTime;
        if (enemy->moveTimer <= 0) {
            enemy->moveAngle = GetRandomValue(0, 359) * PI / 180.0f;
            enemy->moveTimer = GetRandomValue(3, 6);
        }
        
        // Base random movement
        float targetDx = TANK_ENEMY_SPEED * 0.5f * cos(enemy->moveAngle);
        float targetDy = TANK_ENEMY_SPEED * 0.5f * sin(enemy->moveAngle);
        
        // Apply asteroid avoidance
        if (avoidanceMagnitude > 0) {
            targetDx += avoidVector.x * TANK_ENEMY_SPEED;
            targetDy += avoidVector.y * TANK_ENEMY_SPEED;
        }
        
        // Apply smoothed movement
        enemy->base.dx = enemy->base.dx * 0.8f + targetDx * 0.2f;
        enemy->base.dy = enemy->base.dy * 0.8f + targetDy * 0.2f;
        
        // Emit thrust particles occasionally during random movement
        if (GetRandomValue(0, 10) == 0) {
            emitEnemyThrustParticles(state, enemy, 1);
        }
    }
}

// Scout enemy behavior update
void updateScoutBehavior(GameState* state, Enemy* enemy, int enemyIndex, float deltaTime, float distanceToPlayer, 
                        float angleToPlayer, Vector2 avoidVector, Vector2* scoutPositions, int* scoutIndices, 
                        int* groupDesires, int activeScouts) {
    float avoidanceMagnitude = sqrt(avoidVector.x * avoidVector.x + avoidVector.y * avoidVector.y);
    
    if (distanceToPlayer < ENEMY_DETECTION_RADIUS) {
        // Check if scout should use group behavior
        Vector2 groupInfluence = {0, 0};
        Vector2 separationForce = {0, 0};
        int groupSize = 0;
        int scoutIdx = -1;
        int wantsToGroup = 0;
        
        // Find this scout's index and group desire
        for (int j = 0; j < activeScouts; j++) {
            if (scoutIndices[j] == enemyIndex) {
                scoutIdx = j;
                wantsToGroup = groupDesires[j];
                break;
            }
        }
        
        // Only consider grouping if this scout wants to group
        if (wantsToGroup && scoutIdx >= 0) {
            // Find nearby scouts for group behavior
            for (int j = 0; j < activeScouts; j++) {
                int otherScoutIdx = scoutIndices[j];
                if (otherScoutIdx == enemyIndex) continue; // Don't include self
                
                // Calculate squared distance to other scout for optimization
                float sx = scoutPositions[j].x - enemy->base.x;
                float sy = scoutPositions[j].y - enemy->base.y;
                float scoutDistSq = sx * sx + sy * sy;
                float scoutGroupRadiusSq = SCOUT_GROUP_RADIUS * SCOUT_GROUP_RADIUS;
                float scoutSeparationRadiusSq = SCOUT_SEPARATION_RADIUS * SCOUT_SEPARATION_RADIUS;
                
                // Include in group if other scout is close enough AND wants to group too
                if (scoutDistSq < scoutGroupRadiusSq && groupDesires[j]) {
                    float scoutDist = sqrt(scoutDistSq); // Only calculate sqrt when needed
                    groupInfluence.x += sx;
                    groupInfluence.y += sy;
                    groupSize++;
                    
                    // Add separation force if too close to avoid clipping
                    if (scoutDistSq < scoutSeparationRadiusSq) {
                        float separationStrength = 1.0f - (scoutDist / SCOUT_SEPARATION_RADIUS);
                        separationStrength = separationStrength * separationStrength; // Square for stronger effect
                        if (scoutDist < 0.1f) scoutDist = 0.1f; // Avoid division by zero
                        
                        separationForce.x -= (sx / scoutDist) * separationStrength * 2.0f;
                        separationForce.y -= (sy / scoutDist) * separationStrength * 2.0f;
                    }
                }
            }
        }
        
        // Calculate target movement
        float targetDx = 0;
        float targetDy = 0;
        
        // Apply group behavior if we found other scouts nearby
        if (groupSize > 0 && wantsToGroup) {
            // Normalize group influence
            float groupMagnitude = sqrt(groupInfluence.x * groupInfluence.x + groupInfluence.y * groupInfluence.y);
            if (groupMagnitude > 0) {
                groupInfluence.x /= groupMagnitude;
                groupInfluence.y /= groupMagnitude;
            }
            
            // Normalize separation force
            float separationMagnitude = sqrt(separationForce.x * separationForce.x + separationForce.y * separationForce.y);
            if (separationMagnitude > 0) {
                separationForce.x /= separationMagnitude;
                separationForce.y /= separationMagnitude;
            }
            
            // Approach distance - use wider margins
            if (distanceToPlayer > SCOUT_ENEMY_ATTACK_DISTANCE * 1.3f) {
                // Move as group toward player, in formation
                targetDx = SCOUT_ENEMY_SPEED * sin(angleToPlayer * PI / 180.0f);
                targetDy = -SCOUT_ENEMY_SPEED * cos(angleToPlayer * PI / 180.0f);
                
                // Calculate formation position based on index
                float formationOffset = ((enemyIndex % 3) - 1) * 60.0f;
                float perpAngle = (angleToPlayer + 90.0f) * PI / 180.0f;
                
                // Add perpendicular offset for side-by-side formation
                targetDx += formationOffset * cos(perpAngle) * 0.03f;
                targetDy += formationOffset * sin(perpAngle) * 0.03f;
                
                // Add group cohesion
                targetDx += groupInfluence.x * SCOUT_ENEMY_SPEED * SCOUT_GROUP_COHESION * 0.8f;
                targetDy += groupInfluence.y * SCOUT_ENEMY_SPEED * SCOUT_GROUP_COHESION * 0.8f;
                
                // Add separation force
                targetDx += separationForce.x * SCOUT_ENEMY_SPEED * SCOUT_SEPARATION_FORCE * 2.0f;
                targetDy += separationForce.y * SCOUT_ENEMY_SPEED * SCOUT_SEPARATION_FORCE * 2.0f;
                
                // Add asteroid avoidance
                if (avoidanceMagnitude > 0) {
                    targetDx += avoidVector.x * SCOUT_ENEMY_SPEED * 2.0f;
                    targetDy += avoidVector.y * SCOUT_ENEMY_SPEED * 2.0f;
                }
                
                // Smooth acceleration
                enemy->base.dx = enemy->base.dx * 0.9f + targetDx * 0.1f;
                enemy->base.dy = enemy->base.dy * 0.9f + targetDy * 0.1f;
                
                // Emit thrust particles when approaching player
                emitEnemyThrustParticles(state, enemy, 2);
            } 
            // Circle the player at attack distance, as a group in formation
            else if (distanceToPlayer > SCOUT_ENEMY_ATTACK_DISTANCE * 0.6f) {
                // Calculate position in formation
                float groupIndex = 0;
                for (int j = 0; j < activeScouts; j++) {
                    if (scoutIndices[j] == enemyIndex) {
                        groupIndex = j;
                        break;
                    }
                }
                
                // Calculate uniform spacing around the player
                float divisor = fmaxf(groupSize + 1, 3); 
                float formationAngle = (360.0f / divisor) * ((int)groupIndex % (int)divisor); 
                float circlingDirection = formationAngle;
                
                // Calculate the perpendicular angle for smooth circling
                float circlingAngle = (angleToPlayer + circlingDirection) * PI / 180.0f;
                
                // Calculate target velocity for circling
                targetDx = SCOUT_ENEMY_SPEED * 0.8f * cos(circlingAngle);
                targetDy = SCOUT_ENEMY_SPEED * 0.8f * sin(circlingAngle);
                
                // Apply small correction to maintain distance
                float distError = distanceToPlayer - SCOUT_ENEMY_ATTACK_DISTANCE;
                float correction = 0.03f * distError / 50.0f;
                
                targetDx += correction * sin(angleToPlayer * PI / 180.0f);
                targetDy -= correction * cos(angleToPlayer * PI / 180.0f);
                
                // Add group cohesion, but weaker during attack
                targetDx += groupInfluence.x * SCOUT_ENEMY_SPEED * SCOUT_GROUP_COHESION * 0.3f;
                targetDy += groupInfluence.y * SCOUT_ENEMY_SPEED * SCOUT_GROUP_COHESION * 0.3f;
                
                // Add separation force to prevent clipping (stronger during circling)
                targetDx += separationForce.x * SCOUT_ENEMY_SPEED * SCOUT_SEPARATION_FORCE * 2.5f;
                targetDy += separationForce.y * SCOUT_ENEMY_SPEED * SCOUT_SEPARATION_FORCE * 2.5f;
                
                // Add asteroid avoidance
                if (avoidanceMagnitude > 0) {
                    targetDx += avoidVector.x * SCOUT_ENEMY_SPEED * 2.0f;
                    targetDy += avoidVector.y * SCOUT_ENEMY_SPEED * 2.0f;
                }
                
                // Smooth acceleration
                enemy->base.dx = enemy->base.dx * 0.85f + targetDx * 0.15f;
                enemy->base.dy = enemy->base.dy * 0.85f + targetDy * 0.15f;
            }
            // Retreat as a group if too close
            else {
                // Calculate retreat direction
                targetDx = -SCOUT_ENEMY_SPEED * 0.7f * sin(angleToPlayer * PI / 180.0f);
                targetDy = SCOUT_ENEMY_SPEED * 0.7f * cos(angleToPlayer * PI / 180.0f);
                
                // Add slight group cohesion, even when retreating
                targetDx += groupInfluence.x * SCOUT_ENEMY_SPEED * SCOUT_GROUP_COHESION * 0.3f;
                targetDy += groupInfluence.y * SCOUT_ENEMY_SPEED * SCOUT_GROUP_COHESION * 0.3f;
                
                // Add separation force to prevent clipping (stronger when retreating)
                targetDx += separationForce.x * SCOUT_ENEMY_SPEED * SCOUT_SEPARATION_FORCE * 2.0f;
                targetDy += separationForce.y * SCOUT_ENEMY_SPEED * SCOUT_SEPARATION_FORCE * 2.0f;
                
                // Add asteroid avoidance
                if (avoidanceMagnitude > 0) {
                    targetDx += avoidVector.x * SCOUT_ENEMY_SPEED * 2.0f;
                    targetDy += avoidVector.y * SCOUT_ENEMY_SPEED * 2.0f;
                }
                
                // Smoother acceleration
                enemy->base.dx = enemy->base.dx * 0.8f + targetDx * 0.2f;
                enemy->base.dy = enemy->base.dy * 0.8f + targetDy * 0.2f;
            }
            
            // Coordinate firing pattern within group
            if (!enemy->isBursting) {
                enemy->fireTimer -= deltaTime;
                if (enemy->fireTimer <= 0 && distanceToPlayer < ENEMY_DETECTION_RADIUS) {
                    // Start burst with slight timing offsets for group members
                    enemy->isBursting = true;
                    enemy->burstCount = 0;
                    enemy->burstTimer = SCOUT_ENEMY_BURST_DELAY * (groupSize % 3) * SCOUT_GROUP_ATTACK_DELAY;
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
                        // Set a cooldown between bursts with variance
                        enemy->fireTimer = SCOUT_ENEMY_FIRE_RATE * 30.0f + (groupSize % 3) * SCOUT_GROUP_ATTACK_DELAY * 5.0f;
                    }
                }
            }
        } else {
            // Use original scout behavior when not in a group
            targetDx = 0;
            targetDy = 0;
            
            // Approach distance - use wider margins to prevent rapid state transitions
            if (distanceToPlayer > SCOUT_ENEMY_ATTACK_DISTANCE * 1.3f) {
                // Move directly toward player
                targetDx = SCOUT_ENEMY_SPEED * sin(angleToPlayer * PI / 180.0f);
                targetDy = -SCOUT_ENEMY_SPEED * cos(angleToPlayer * PI / 180.0f);
                
                // Add asteroid avoidance
                if (avoidanceMagnitude > 0) {
                    targetDx += avoidVector.x * SCOUT_ENEMY_SPEED * 2.0f;
                    targetDy += avoidVector.y * SCOUT_ENEMY_SPEED * 2.0f;
                }
                
                // Smooth acceleration instead of instant speed change
                enemy->base.dx = enemy->base.dx * 0.9f + targetDx * 0.1f;
                enemy->base.dy = enemy->base.dy * 0.9f + targetDy * 0.1f;
                
                // Emit thrust particles when approaching player
                emitEnemyThrustParticles(state, enemy, 2);
            } 
            // Circle the player at attack distance
            else if (distanceToPlayer > SCOUT_ENEMY_ATTACK_DISTANCE * 0.6f) {
                // Use consistent circling direction for each enemy
                float circlingDirection = (enemyIndex % 2 == 0) ? 90.0f : -90.0f;
                
                // Calculate the perpendicular angle for smooth circling
                float circlingAngle = (angleToPlayer + circlingDirection) * PI / 180.0f;
                
                // Calculate target velocity for circling
                targetDx = SCOUT_ENEMY_SPEED * 0.8f * cos(circlingAngle);
                targetDy = SCOUT_ENEMY_SPEED * 0.8f * sin(circlingAngle);
                
                // Apply small correction to maintain distance, with reduced sensitivity
                float distError = distanceToPlayer - SCOUT_ENEMY_ATTACK_DISTANCE;
                float correction = 0.03f * distError / 50.0f;  
                
                targetDx += correction * sin(angleToPlayer * PI / 180.0f);
                targetDy -= correction * cos(angleToPlayer * PI / 180.0f);
                
                // Add asteroid avoidance
                if (avoidanceMagnitude > 0) {
                    targetDx += avoidVector.x * SCOUT_ENEMY_SPEED * 1.5f;
                    targetDy += avoidVector.y * SCOUT_ENEMY_SPEED * 1.5f;
                }
                
                // Smooth acceleration
                enemy->base.dx = enemy->base.dx * 0.85f + targetDx * 0.15f;
                enemy->base.dy = enemy->base.dy * 0.85f + targetDy * 0.15f;
            }
            // Back away if too close - more gradual transition
            else {
                // Calculate retreat direction
                targetDx = -SCOUT_ENEMY_SPEED * 0.7f * sin(angleToPlayer * PI / 180.0f);
                targetDy = SCOUT_ENEMY_SPEED * 0.7f * cos(angleToPlayer * PI / 180.0f);
                
                // Add asteroid avoidance
                if (avoidanceMagnitude > 0) {
                    targetDx += avoidVector.x * SCOUT_ENEMY_SPEED * 1.5f;
                    targetDy += avoidVector.y * SCOUT_ENEMY_SPEED * 1.5f;
                }
                
                // Even smoother acceleration when retreating to avoid jerky movements
                enemy->base.dx = enemy->base.dx * 0.8f + targetDx * 0.2f;
                enemy->base.dy = enemy->base.dy * 0.8f + targetDy * 0.2f;
            }
            
            // Standard burst fire logic for solo scouts
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
                        // Set a longer cooldown between bursts (3-5 seconds)
                        enemy->fireTimer = SCOUT_ENEMY_FIRE_RATE * 30.0f + GetRandomValue(0, 20) / 10.0f;
                    }
                }
            }
        }
    } else {
        // Random movement when player not detected - make it more interesting
        enemy->moveTimer -= deltaTime;
        if (enemy->moveTimer <= 0) {
            // Change direction more often for more erratic movement
            enemy->moveAngle = GetRandomValue(0, 359) * PI / 180.0f;
            // Shorter time between movement changes
            enemy->moveTimer = GetRandomValue(1, 2);
        }
        
        // Base random movement
        float targetDx = SCOUT_ENEMY_SPEED * 0.7f * cos(enemy->moveAngle);
        float targetDy = SCOUT_ENEMY_SPEED * 0.7f * sin(enemy->moveAngle);
        
        // Add asteroid avoidance
        if (avoidanceMagnitude > 0) {
            targetDx += avoidVector.x * SCOUT_ENEMY_SPEED * 1.5f;
            targetDy += avoidVector.y * SCOUT_ENEMY_SPEED * 1.5f;
        }
        
        // Apply smoothed movement
        enemy->base.dx = enemy->base.dx * 0.8f + targetDx * 0.2f;
        enemy->base.dy = enemy->base.dy * 0.8f + targetDy * 0.2f;
    }
}

// Update enemy position with boundary checking
void updateEnemyPosition(GameState* state, Enemy* enemy) {
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
}

// Handle collisions with asteroids
bool handleAsteroidCollisions(GameState* state, Enemy* enemy, int enemyIndex) {
    for (int j = 0; j < MAX_ASTEROIDS; j++) {
        if (!state->asteroids[j].base.active) continue;
        
        if (checkCollision(&enemy->base, &state->asteroids[j].base)) {
            // Calculate collision response
            float dx = state->asteroids[j].base.x - enemy->base.x;
            float dy = state->asteroids[j].base.y - enemy->base.y;
            float distance = sqrt(dx * dx + dy * dy);
            
            // Avoid division by zero
            if (distance == 0) distance = 0.01f;
            
            // Normalize direction
            float nx = dx / distance;
            float ny = dy / distance;
            
            // Apply damage to enemy based on asteroid size
            int damage = 0;
            switch (state->asteroids[j].size) {
                case 3: damage = LARGE_ASTEROID_DAMAGE / 2; break;   // Reduced damage for enemies
                case 2: damage = MEDIUM_ASTEROID_DAMAGE / 2; break;
                case 1: damage = SMALL_ASTEROID_DAMAGE / 2; break;
            }
            
            enemy->health -= damage;
            
            // Bounce enemy away from asteroid
            enemy->base.dx = -nx * (enemy->type == ENEMY_TANK ? TANK_ENEMY_SPEED : SCOUT_ENEMY_SPEED);
            enemy->base.dy = -ny * (enemy->type == ENEMY_TANK ? TANK_ENEMY_SPEED : SCOUT_ENEMY_SPEED);
            
            // Change movement direction after collision
            enemy->moveAngle = atan2(-ny, -nx);
            enemy->moveTimer = GetRandomValue(1, 3);  // Reset movement timer
            
            // Split asteroid on collision
            splitAsteroid(state, j);
            
            // Check if enemy is destroyed
            if (enemy->health <= 0) {
                // Enemy destroyed by asteroid
                enemy->base.active = false;
                
                // Generate explosion particles
                createEnemyExplosion(state, enemy->base.x, enemy->base.y, 20);
                
                // Play explosion sound
                if (state->soundLoaded) {
                    PlaySound(state->sounds[SOUND_ENEMY_EXPLODE]);
                }
                
                // Give player half the score value when asteroid destroys an enemy
                state->score += (enemy->type == ENEMY_TANK ? TANK_ENEMY_SCORE : SCOUT_ENEMY_SCORE) / 2;
                
                return true;  // Enemy destroyed
            }
            
            break;  // Only handle one collision per frame
        }
    }
    
    return false; // Enemy not destroyed
}

// Handle collisions with player bullets
void handleBulletCollisions(GameState* state, Enemy* enemy, int enemyIndex) {
    for (int j = 0; j < MAX_BULLETS; j++) {
        if (!state->bullets[j].active) continue;
        
        if (checkCollision(&enemy->base, &state->bullets[j])) {
            state->bullets[j].active = false;
            enemy->health -= 10;  // Each player bullet deals 10 damage
            
            if (enemy->health <= 0) {
                // Enemy destroyed
                enemy->base.active = false;
                
                // Add score based on enemy type
                state->score += (enemy->type == ENEMY_TANK) ? TANK_ENEMY_SCORE : SCOUT_ENEMY_SCORE;
                
                // Check for powerup drops
                if (enemy->type == ENEMY_SCOUT) {
                    // Chance to drop shotgun powerup
                    if (GetRandomValue(1, 100) <= SHOTGUN_DROP_CHANCE) {
                        spawnShotgunPowerup(state, enemy->base.x, enemy->base.y);
                    }
                } else if (enemy->type == ENEMY_TANK) {
                    // Chance to drop grenade powerup
                    if (GetRandomValue(1, 100) <= GRENADE_DROP_CHANCE) {
                        spawnGrenadePowerup(state, enemy->base.x, enemy->base.y);
                    }
                }
                
                // Play explosion sound
                if (state->soundLoaded) {
                    PlaySound(state->sounds[SOUND_ENEMY_EXPLODE]);
                }
                
                // Generate explosion particles
                createEnemyExplosion(state, enemy->base.x, enemy->base.y, 20);
            }
            
            break; // Only handle one collision per frame
        }
    }
}

// Create enemy explosion particles
void createEnemyExplosion(GameState* state, float x, float y, int particleCount) {
    for (int k = 0; k < particleCount; k++) {
        for (int p = 0; p < MAX_PARTICLES; p++) {
            if (!state->particles[p].active) {
                state->particles[p].active = true;
                state->particles[p].life = PARTICLE_LIFETIME;
                state->particles[p].position.x = x;
                state->particles[p].position.y = y;
                
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

// Update enemy bullets with optimized collision detection
void updateEnemyBullets(GameState* state, float deltaTime) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!state->enemyBullets[i].base.active) continue;
        
        Bullet* bullet = &state->enemyBullets[i];
        
        // Update grenade timer
        if (bullet->type == BULLET_GRENADE && !bullet->hasExploded) {
            bullet->timer -= deltaTime;
            if (bullet->timer <= 0) {
                explodeGrenade(state, i);
                continue; // Skip normal bullet update
            }
        }
        
        // Update position
        bullet->base.x += bullet->base.dx;
        bullet->base.y += bullet->base.dy;
        
        // Check if bullet is out of bounds
        if (bullet->base.x < 0 || bullet->base.x > MAP_WIDTH || 
            bullet->base.y < 0 || bullet->base.y > MAP_HEIGHT) {
            bullet->base.active = false;
            continue;
        }
        
        // Check for bullet collision with asteroids
        bool asteroidHit = false;
        for (int j = 0; j < MAX_ASTEROIDS; j++) {
            if (!state->asteroids[j].base.active) continue;
            
            if (checkCollision(&bullet->base, &state->asteroids[j].base)) {
                // If it's a grenade, explode it on impact
                if (bullet->type == BULLET_GRENADE && !bullet->hasExploded) {
                    explodeGrenade(state, i);
                } else {
                    bullet->base.active = false;
                }
                splitAsteroid(state, j);
                
                // Play asteroid hit sound
                if (state->soundLoaded) {
                    PlaySound(state->sounds[SOUND_ASTEROID_HIT]);
                }
                
                asteroidHit = true;
                break;
            }
        }
        
        if (asteroidHit) continue;
        
        // Handle player-controlled bullets hitting enemies
        if (bullet->isPlayerBullet) {
            bool enemyHit = false;
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (!state->enemies[j].base.active) continue;
                
                if (checkCollision(&bullet->base, &state->enemies[j].base)) {
                    // Deactivate bullet unless it's a grenade that needs to explode
                    if (bullet->type != BULLET_GRENADE || bullet->hasExploded) {
                        bullet->base.active = false;
                    } else if (bullet->type == BULLET_GRENADE && !bullet->hasExploded) {
                        explodeGrenade(state, i);
                    }
                    
                    // Apply damage to enemy
                    state->enemies[j].health -= bullet->damage;
                    
                    // Check if enemy is destroyed
                    if (state->enemies[j].health <= 0) {
                        // Enemy destroyed
                        state->enemies[j].base.active = false;
                        
                        // Add score based on enemy type
                        state->score += (state->enemies[j].type == ENEMY_TANK) ? 
                                        TANK_ENEMY_SCORE : SCOUT_ENEMY_SCORE;
                        
                        // Check for powerup drops
                        if (state->enemies[j].type == ENEMY_SCOUT) {
                            if (GetRandomValue(1, 100) <= SHOTGUN_DROP_CHANCE) {
                                spawnShotgunPowerup(state, state->enemies[j].base.x, state->enemies[j].base.y);
                            }
                        } else if (state->enemies[j].type == ENEMY_TANK) {
                            if (GetRandomValue(1, 100) <= GRENADE_DROP_CHANCE) {
                                spawnGrenadePowerup(state, state->enemies[j].base.x, state->enemies[j].base.y);
                            }
                        }
                        
                        // Play explosion sound
                        if (state->soundLoaded) {
                            PlaySound(state->sounds[SOUND_ENEMY_EXPLODE]);
                        }
                        
                        // Generate explosion particles
                        createEnemyExplosion(state, state->enemies[j].base.x, state->enemies[j].base.y, 20);
                    }
                    
                    enemyHit = true;
                    break;
                }
            }
            
            if (enemyHit) continue;
        } 
        // Enemy bullets hitting player
        else if (checkCollision(&state->ship.base, &bullet->base)) {
            // If it's a grenade, explode it on impact with player
            if (bullet->type == BULLET_GRENADE && !bullet->hasExploded) {
                explodeGrenade(state, i);
            } else {
                bullet->base.active = false;
            }
            
            // Only apply damage if player is not invulnerable
            if (!state->isInvulnerable) {
                state->health -= bullet->damage;
                
                if (state->health <= 0) {
                    state->lives--;
                    if (state->lives <= 0) {
                        // Game over
                        state->screenState = GAME_OVER_STATE;
                    } else {
                        resetShip(state);
                    }
                }
            }
        }
    }
}

float getEnemyTextureScale(EnemyType type) {
    switch (type) {
        case ENEMY_TANK:
            return TANK_TEXTURE_SCALE;
        case ENEMY_SCOUT:
            return SCOUT_TEXTURE_SCALE;
        default:
            return 1.0f; // Default scale
    }
}