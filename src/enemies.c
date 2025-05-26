#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <string.h>

// custom headers
#include "typedef.h"
#include "config.h"

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