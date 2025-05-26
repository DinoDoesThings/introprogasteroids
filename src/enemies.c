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

// New function to emit enemy thrust particles
void emitEnemyThrustParticles(GameState* state, Enemy* enemy, int count) {
    // Calculate enemy rear position (opposite to facing direction)
    float radians = enemy->base.angle * PI / 180.0f;
    float rearX = enemy->base.x - sin(radians) * enemy->base.radius * 1.2f;
    float rearY = enemy->base.y + cos(radians) * enemy->base.radius * 1.2f;
    
    // Emit particles
    for (int i = 0; i < count; i++) {
        // Find an inactive particle
        for (int j = 0; j < MAX_PARTICLES; j++) {
            if (!state->particles[j].active) {
                state->particles[j].active = true;
                state->particles[j].life = PARTICLE_LIFETIME * 0.7f; // Slightly shorter life than player particles
                
                // Set particle position at the enemy's rear
                state->particles[j].position.x = rearX;
                state->particles[j].position.y = rearY;
                
                // Add slight randomness to position
                state->particles[j].position.x += GetRandomValue(-2, 2);
                state->particles[j].position.y += GetRandomValue(-2, 2);
                
                // Set particle velocity in the opposite direction of the enemy
                float particleAngle = radians + PI + GetRandomValue(-20, 20) * PI / 180.0f;
                float particleSpeed = PARTICLE_SPEED * 0.8f;
                state->particles[j].velocity.x = sin(particleAngle) * particleSpeed;
                state->particles[j].velocity.y = -cos(particleAngle) * particleSpeed;
                
                // Set particle appearance
                state->particles[j].radius = GetRandomValue(1, 3);
                
                // Different colors based on enemy type
                if (enemy->type == ENEMY_TANK) {
                    // Red/orange exhaust for tank enemies
                    int colorChoice = GetRandomValue(0, 2);
                    if (colorChoice == 0)
                        state->particles[j].color = (Color){ 255, 100, 50, 255 };  // Red-orange
                    else if (colorChoice == 1)
                        state->particles[j].color = (Color){ 255, 50, 20, 255 };   // Reddish
                    else
                        state->particles[j].color = (Color){ 255, 150, 50, 255 };  // Orange
                } else {
                    // Blue/cyan exhaust for scout enemies
                    int colorChoice = GetRandomValue(0, 2);
                    if (colorChoice == 0)
                        state->particles[j].color = (Color){ 50, 150, 255, 255 };  // Blue
                    else if (colorChoice == 1)
                        state->particles[j].color = (Color){ 0, 200, 255, 255 };   // Cyan
                    else
                        state->particles[j].color = (Color){ 100, 200, 255, 255 }; // Light blue
                }
                
                break;
            }
        }
    }
}

void updateEnemies(GameState* state, float deltaTime) {
    // Find active scout enemies for grouping behavior
    int activeScouts = 0;
    Vector2 scoutPositions[MAX_ENEMIES];
    int scoutIndices[MAX_ENEMIES];
    int groupDesires[MAX_ENEMIES];  
    
    // First collect all active scout positions and determine group desire
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (state->enemies[i].base.active && state->enemies[i].type == ENEMY_SCOUT) {
            scoutPositions[activeScouts] = (Vector2){state->enemies[i].base.x, state->enemies[i].base.y};
            scoutIndices[activeScouts] = i;
            
            // Determine if this scout wants to group based on its index (consistent behavior)
            // Using index ensures the decision is persistent and doesn't change randomly
            groupDesires[activeScouts] = (i % 100 < SCOUT_GROUP_CHANCE) ? 1 : 0;
            
            activeScouts++;
        }
    }

    // Update enemy spawn timer 
    if (state->currentWave >= SCOUT_START_WAVE && !state->inWaveTransition) {
        state->enemySpawnTimer -= deltaTime;
        if (state->enemySpawnTimer <= 0) {
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
            
            // Reset timer with some randomness
            float baseTime = ENEMY_SPAWN_TIME - (state->currentWave - SCOUT_START_WAVE) * 1.0f;
            baseTime = (baseTime < 3.0f) ? 3.0f : baseTime;
            state->enemySpawnTimer = baseTime + GetRandomValue(-100, 100) / 100.0f;
        }
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
            
            // Calculate avoidance vector for asteroids
            Vector2 avoidVector = {0, 0};
            float avoidanceWeight = 2.0f;    // Strength of avoidance
            float detectionDistance = enemy->base.radius * 5.0f;  // Distance to detect asteroids
            
            // Check nearby asteroids to generate avoidance vector
            for (int j = 0; j < MAX_ASTEROIDS; j++) {
                if (state->asteroids[j].base.active) {
                    // Calculate distance and direction to asteroid
                    float ax = state->asteroids[j].base.x - enemy->base.x;
                    float ay = state->asteroids[j].base.y - enemy->base.y;
                    float asteroidDist = sqrt(ax * ax + ay * ay);
                    float asteroidRadius = state->asteroids[j].base.radius;
                    
                    // If asteroid is close enough to avoid
                    if (asteroidDist < detectionDistance + asteroidRadius) {
                        // Calculate normalized direction vector away from asteroid
                        float weight = 1.0f - asteroidDist / (detectionDistance + asteroidRadius);
                        weight = weight * weight * avoidanceWeight; // Make avoidance stronger when closer
                        
                        // Avoid division by zero
                        if (asteroidDist < 0.1f) asteroidDist = 0.1f;
                        
                        // Add weighted avoidance vector (away from asteroid)
                        avoidVector.x -= (ax / asteroidDist) * weight;
                        avoidVector.y -= (ay / asteroidDist) * weight;
                    }
                }
            }
            
            // Normalize avoidance vector if it's not zero
            float avoidanceMagnitude = sqrt(avoidVector.x * avoidVector.x + avoidVector.y * avoidVector.y);
            if (avoidanceMagnitude > 0) {
                avoidVector.x /= avoidanceMagnitude;
                avoidVector.y /= avoidanceMagnitude;
            }
            
            // Update movement based on enemy type
            if (enemy->type == ENEMY_TANK) {
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
            } else {  // ENEMY_SCOUT
                // Scout has more complex movement with group behavior
                if (distanceToPlayer < ENEMY_DETECTION_RADIUS) {
                    // Check if scout should use group behavior
                    Vector2 groupInfluence = {0, 0};
                    Vector2 separationForce = {0, 0}; // Force to prevent crowding
                    int groupSize = 0;
                    int scoutIdx = -1;
                    int wantsToGroup = 0;

                    // Find this scout's index and group desire
                    for (int j = 0; j < activeScouts; j++) {
                        if (scoutIndices[j] == i) {
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
                            if (otherScoutIdx != i) { // Don't include self
                                // Calculate distance to other scout
                                float sx = scoutPositions[j].x - enemy->base.x;
                                float sy = scoutPositions[j].y - enemy->base.y;
                                float scoutDist = sqrt(sx * sx + sy * sy);
                                
                                // Include in group if other scout is close enough AND wants to group too
                                if (scoutDist < SCOUT_GROUP_RADIUS && 
                                    groupDesires[j]) {
                                    groupInfluence.x += sx;
                                    groupInfluence.y += sy;
                                    groupSize++;
                                    
                                    // Add separation force if too close to avoid clipping
                                    if (scoutDist < SCOUT_SEPARATION_RADIUS) {
                                        float separationStrength = 1.0f - (scoutDist / SCOUT_SEPARATION_RADIUS);
                                        if (scoutDist < 0.1f) scoutDist = 0.1f; // Avoid division by zero
                                        
                                        separationForce.x -= (sx / scoutDist) * separationStrength;
                                        separationForce.y -= (sy / scoutDist) * separationStrength;
                                    }
                                }
                            }
                        }
                    }
                    
                    // Apply group cohesion if we found other scouts nearby
                    float targetDx = 0;
                    float targetDy = 0;
                    
                    if (groupSize > 0 && wantsToGroup) {
                        // Normalize group influence
                        float groupMagnitude = sqrt(groupInfluence.x * groupInfluence.x + 
                                                   groupInfluence.y * groupInfluence.y);
                        if (groupMagnitude > 0) {
                            groupInfluence.x /= groupMagnitude;
                            groupInfluence.y /= groupMagnitude;
                        }
                        
                        // Normalize separation force
                        float separationMagnitude = sqrt(separationForce.x * separationForce.x + 
                                                        separationForce.y * separationForce.y);
                        if (separationMagnitude > 0) {
                            separationForce.x /= separationMagnitude;
                            separationForce.y /= separationMagnitude;
                        }
                        
                        // Approach distance - use wider margins to prevent rapid state transitions
                        if (distanceToPlayer > SCOUT_ENEMY_ATTACK_DISTANCE * 1.3f) {
                            // Move as group toward player, in formation
                            targetDx = SCOUT_ENEMY_SPEED * sin(angleToPlayer * PI / 180.0f);
                            targetDy = -SCOUT_ENEMY_SPEED * cos(angleToPlayer * PI / 180.0f);
                            
                            // Calculate formation position based on index within group
                            float formationOffset = ((i % 3) - 1) * 40.0f; // -40, 0, or +40 units offset
                            float perpAngle = (angleToPlayer + 90.0f) * PI / 180.0f;
                            
                            // Add perpendicular offset for side-by-side formation
                            targetDx += formationOffset * cos(perpAngle) * 0.02f;
                            targetDy += formationOffset * sin(perpAngle) * 0.02f;
                            
                            // Add group cohesion
                            targetDx += groupInfluence.x * SCOUT_ENEMY_SPEED * SCOUT_GROUP_COHESION;
                            targetDy += groupInfluence.y * SCOUT_ENEMY_SPEED * SCOUT_GROUP_COHESION;
                            
                            // Add separation force to prevent clipping
                            targetDx += separationForce.x * SCOUT_ENEMY_SPEED * SCOUT_SEPARATION_FORCE;
                            targetDy += separationForce.y * SCOUT_ENEMY_SPEED * SCOUT_SEPARATION_FORCE;
                            
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
                            // Calculate position in formation (equally spaced around circle)
                            float groupIndex = 0;
                            for (int j = 0; j < activeScouts; j++) {
                                if (scoutIndices[j] == i) {
                                    groupIndex = j;
                                    break;
                                }
                            }
                            
                            // Calculate uniform spacing around the player (avoid clumping)
                            float formationAngle = (360.0f / (groupSize + 1)) * ((int)groupIndex % (groupSize + 1));
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
                            targetDx += groupInfluence.x * SCOUT_ENEMY_SPEED * SCOUT_GROUP_COHESION * 0.5f;
                            targetDy += groupInfluence.y * SCOUT_ENEMY_SPEED * SCOUT_GROUP_COHESION * 0.5f;
                            
                            // Add separation force to prevent clipping
                            targetDx += separationForce.x * SCOUT_ENEMY_SPEED * SCOUT_SEPARATION_FORCE * 1.5f;
                            targetDy += separationForce.y * SCOUT_ENEMY_SPEED * SCOUT_SEPARATION_FORCE * 1.5f;
                            
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
                                    enemy->fireTimer = SCOUT_ENEMY_FIRE_RATE * 30.0f + 
                                                     (groupSize % 3) * SCOUT_GROUP_ATTACK_DELAY * 5.0f;
                                }
                            }
                        }
                    } else {
                        // Use original scout behavior when not in a group
                        // Approach distance - use wider margins to prevent rapid state transitions
                        if (distanceToPlayer > SCOUT_ENEMY_ATTACK_DISTANCE * 1.3f) {
                            // Move directly toward player to get in range, but with smoothed velocity
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
                            float circlingDirection = (i % 2 == 0) ? 90.0f : -90.0f;
                            
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
            
            // Check for collisions with asteroids
            for (int j = 0; j < MAX_ASTEROIDS; j++) {
                if (state->asteroids[j].base.active && checkCollision(&enemy->base, &state->asteroids[j].base)) {
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
                        
                        // Play explosion sound
                        if (state->soundLoaded) {
                            PlaySound(state->sounds[SOUND_ENEMY_EXPLODE]);
                        }
                        
                        // Give player half the score value when asteroid destroys an enemy
                        state->score += (enemy->type == ENEMY_TANK ? TANK_ENEMY_SCORE : SCOUT_ENEMY_SCORE) / 2;
                        
                        break;  // Exit asteroid loop since enemy is destroyed
                    }
                }
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
            
            // Check for bullet collision with asteroids
            for (int j = 0; j < MAX_ASTEROIDS; j++) {
                if (state->asteroids[j].base.active && checkCollision(&state->enemyBullets[i].base, &state->asteroids[j].base)) {
                    state->enemyBullets[i].base.active = false;
                    splitAsteroid(state, j);
                    
                    // Play asteroid hit sound
                    if (state->soundLoaded) {
                        PlaySound(state->sounds[SOUND_ASTEROID_HIT]);
                    }
                    
                    break;
                }
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
                        // Use consistent game over handling
                        state->screenState = GAME_OVER_STATE;
                    } else {
                        resetShip(state);
                    }
                }
            }
        }
    }
}