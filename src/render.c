#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <string.h>

//custom headers
#include "typedefs.h"
#include "config.h"
#include "enemies.h"
#include "powerups.h"
#include "scoreboard.h"

void renderPowerups(const GameState* state) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (state->powerups[i].base.active) {
            const Powerup* powerup = &state->powerups[i];
            
            // Calculate pulsing effect
            float pulseAlpha = 0.7f + 0.3f * sinf(powerup->pulseTimer);
            
            // Flash faster when about to expire
            if (powerup->lifetime < 3.0f) {
                pulseAlpha = 0.5f + 0.5f * sinf(powerup->pulseTimer * 3.0f);
            }
            
            if (powerup->texture.id > 0) {
                // Draw texture
                Vector2 origin = { powerup->texture.width / 2.0f, powerup->texture.height / 2.0f };
                Rectangle source = { 0, 0, powerup->texture.width, powerup->texture.height };
                Rectangle dest = { 
                    powerup->base.x, 
                    powerup->base.y, 
                    powerup->texture.width * POWERUP_TEXTURE_SCALE, 
                    powerup->texture.height * POWERUP_TEXTURE_SCALE 
                };
                
                Color tintColor = WHITE;
                tintColor.a = (unsigned char)(255 * pulseAlpha);
                
                DrawTexturePro(powerup->texture, source, dest, origin, powerup->base.angle, tintColor);
            } else {
                // Fallback: draw as colored circle
                Color powerupColor;
                switch (powerup->type) {
                    case POWERUP_HEALTH:
                        powerupColor = (Color){255, 100, 100, (unsigned char)(255 * pulseAlpha)};
                        break;
                    case POWERUP_LIFE:
                        powerupColor = (Color){255, 255, 0, (unsigned char)(255 * pulseAlpha)};
                        break;
                    case POWERUP_SHOTGUN:
                        powerupColor = (Color){100, 255, 100, (unsigned char)(255 * pulseAlpha)};
                        break;
                    case POWERUP_GRENADE:
                        powerupColor = (Color){255, 165, 0, (unsigned char)(255 * pulseAlpha)};
                        break;
                    default:
                        powerupColor = (Color){255, 255, 255, (unsigned char)(255 * pulseAlpha)};
                        break;
                }
                
                DrawCircleV((Vector2){powerup->base.x, powerup->base.y}, powerup->base.radius, powerupColor);
                
                // Draw appropriate symbol
                if (powerup->type == POWERUP_HEALTH) {
                    // Draw a cross for health powerup
                    Color crossColor = {255, 255, 255, (unsigned char)(255 * pulseAlpha)};
                    float crossSize = powerup->base.radius * 0.6f;
                    DrawRectangle(powerup->base.x - 2, powerup->base.y - crossSize, 4, crossSize * 2, crossColor);
                    DrawRectangle(powerup->base.x - crossSize, powerup->base.y - 2, crossSize * 2, 4, crossColor);
                } else if (powerup->type == POWERUP_SHOTGUN) {
                    // Draw "S" for shotgun powerup
                    Color textColor = {255, 255, 255, (unsigned char)(255 * pulseAlpha)};
                    DrawText("S", powerup->base.x - 6, powerup->base.y - 8, 16, textColor);
                } else if (powerup->type == POWERUP_GRENADE) {
                    // Draw "G" for grenade powerup
                    Color textColor = {255, 255, 255, (unsigned char)(255 * pulseAlpha)};
                    DrawText("G", powerup->base.x - 6, powerup->base.y - 8, 16, textColor);
                } else if (powerup->type == POWERUP_LIFE) {
                    // Draw "1UP" for life powerup
                    Color textColor = {255, 255, 0, (unsigned char)(255 * pulseAlpha)};
                    DrawText("1UP", powerup->base.x - 12, powerup->base.y - 8, 14, textColor);
                }
            }
        }
    }
}

void renderGameObject(const GameObject* obj, int sides, const GameState* state) {
    if (sides <= 0 || !obj->active) return;
    
    // Skip rendering the ship if it's in the invisible part of the blinking cycle
    if (sides == 3 && obj == &state->ship.base && state->isInvulnerable && !state->shipVisible) {
        return;
    }
    
    // Special rendering for the ship (triangle with texture)
    if (sides == 3 && obj == &state->ship.base) {
        // Draw ship texture first
        if (state->ship.texture.id > 0) {
            // Calculate texture positioning
            Vector2 origin = { state->ship.texture.width / 2.0f, state->ship.texture.height / 2.0f };
            Rectangle source = { 0, 0, state->ship.texture.width, state->ship.texture.height };
            Rectangle dest = { 
                obj->x, 
                obj->y, 
                state->ship.texture.width * SHIP_TEXTURE_SCALE, 
                state->ship.texture.height * SHIP_TEXTURE_SCALE 
            };
            
            // Draw the ship texture rotated (image is facing north)
            DrawTexturePro(state->ship.texture, source, dest, origin, obj->angle, WHITE);
        }
        
        // Draw hitbox lines in debug mode or as fallback
        if (state->Debug || state->ship.texture.id == 0) {
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
            
            // Draw the hitbox lines (semi-transparent when debug mode is on)
            Color hitboxColor = state->Debug ? (Color){0, 255, 0, 100} : GREEN;
            DrawLineV(points[0], points[1], hitboxColor);
            DrawLineV(points[1], points[2], hitboxColor);
            DrawLineV(points[2], points[0], hitboxColor);
        }
    } else {
        // Regular rendering for other objects (asteroids, enemies)
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

void renderParticles(const GameState* state) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (state->particles[i].active) {
            DrawCircleV(state->particles[i].position, state->particles[i].radius, state->particles[i].color);
        }
    }
}

void renderEnemies(const GameState* state) {
    // Find active scout groups for visualization
    if (state->Debug) {
        // Identify scouts that want to group
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (state->enemies[i].base.active && state->enemies[i].type == ENEMY_SCOUT) {
                for (int j = i + 1; j < MAX_ENEMIES; j++) {
                    if (state->enemies[j].base.active && state->enemies[j].type == ENEMY_SCOUT) {
                        float dx = state->enemies[i].base.x - state->enemies[j].base.x;
                        float dy = state->enemies[i].base.y - state->enemies[j].base.y;
                        float distance = sqrt(dx * dx + dy * dy);
                        
                        // Draw lines between scouts in same group
                        if (distance < SCOUT_GROUP_RADIUS && 
                            (i % 100 < SCOUT_GROUP_CHANCE) && 
                            (j % 100 < SCOUT_GROUP_CHANCE)) {
                            DrawLineEx(
                                (Vector2){state->enemies[i].base.x, state->enemies[i].base.y},
                                (Vector2){state->enemies[j].base.x, state->enemies[j].base.y},
                                1.0f, 
                                (Color){0, 200, 255, 100}
                            );
                        }
                    }
                }
            }
        }
    }
    
    // Regular enemy rendering 
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (state->enemies[i].base.active) {
            // Draw attack range visualization when in debug mode
            if (state->Debug) {
                // Draw detection radius (outer circle)
                DrawCircleLines(
                    state->enemies[i].base.x,
                    state->enemies[i].base.y,
                    ENEMY_DETECTION_RADIUS,
                    (Color){150, 150, 255, 100}
                );
                
                // Draw attack radius (inner circle)
                float attackDistance = (state->enemies[i].type == ENEMY_TANK) ? 
                                       TANK_ENEMY_ATTACK_DISTANCE : 
                                       SCOUT_ENEMY_ATTACK_DISTANCE;
                                       
                DrawCircleLines(
                    state->enemies[i].base.x,
                    state->enemies[i].base.y,
                    attackDistance,
                    (Color){255, 150, 150, 100}
                );
            }
            
            // Draw enemy texture
            if (state->enemies[i].texture.id > 0) {
                // Get appropriate scale based on enemy type
                float textureScale = getEnemyTextureScale(state->enemies[i].type);
                
                // Calculate scaled dimensions
                float scaledWidth = state->enemies[i].texture.width * textureScale;
                float scaledHeight = state->enemies[i].texture.height * textureScale;
                
                // Calculate texture positioning with proper centering
                Vector2 origin = { scaledWidth / 2.0f, scaledHeight / 2.0f };
                Rectangle source = { 0, 0, state->enemies[i].texture.width, state->enemies[i].texture.height };
                Rectangle dest = { 
                    state->enemies[i].base.x, 
                    state->enemies[i].base.y, 
                    scaledWidth, 
                    scaledHeight 
                };
                
                // Draw the enemy texture rotated (images are facing north)
                DrawTexturePro(state->enemies[i].texture, source, dest, origin, state->enemies[i].base.angle, WHITE);
            }
            
            // Draw hitbox lines in debug mode or as fallback
            if (state->Debug || state->enemies[i].texture.id == 0) {
                // Draw enemy based on type
                if (state->enemies[i].type == ENEMY_TANK) {
                    // Draw tank enemy as pentagon with red color
                    Vector2 points[5];
                    for (int j = 0; j < 5; j++) {
                        float angle = state->enemies[i].base.angle + j * 72.0f;  //  360 / 5 = 72 degrees
                        float radians = angle * PI / 180.0f;
                        points[j].x = state->enemies[i].base.x + sin(radians) * state->enemies[i].base.radius;
                        points[j].y = state->enemies[i].base.y - cos(radians) * state->enemies[i].base.radius;
                    }
                    
                    Color hitboxColor = state->Debug ? (Color){255, 0, 0, 100} : RED;
                    for (int j = 0; j < 5; j++) {
                        int next = (j + 1) % 5;
                        DrawLineV(points[j], points[next], hitboxColor);
                    }
                    
                    // Draw a gun barrel pointing toward player (debug only)
                    if (state->Debug) {
                        float radians = state->enemies[i].base.angle * PI / 180.0f;
                        Vector2 barrelStart = {
                            state->enemies[i].base.x, 
                            state->enemies[i].base.y
                        };
                        Vector2 barrelEnd = {
                            state->enemies[i].base.x + sin(radians) * state->enemies[i].base.radius * 1.5f,
                            state->enemies[i].base.y - cos(radians) * state->enemies[i].base.radius * 1.5f
                        };
                        DrawLineV(barrelStart, barrelEnd, (Color){255, 0, 0, 100});
                    }
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
                    
                    // Draw the hitbox lines
                    Color hitboxColor = state->Debug ? (Color){0, 0, 255, 100} : SKYBLUE;
                    DrawLineV(points[0], points[1], hitboxColor);
                    DrawLineV(points[1], points[2], hitboxColor);
                    DrawLineV(points[2], points[0], hitboxColor);
                    
                    // If in debug mode, add an indicator for scouts that want to group
                    if (state->Debug && (i % 100 < SCOUT_GROUP_CHANCE)) {
                        // Draw small dot on scouts that want to group
                        DrawCircleV(
                            (Vector2){state->enemies[i].base.x, state->enemies[i].base.y},
                            3.0f,
                            (Color){0, 255, 255, 200}
                        );
                    }
                }
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
            Color bulletColor;
            
            if (state->enemyBullets[i].type == BULLET_GRENADE) {
                // Grenade bullets are larger and orange/red
                float pulseIntensity = 0.8f + 0.2f * sinf(GetTime() * 8.0f); // Pulsing effect
                bulletColor = (Color){
                    (unsigned char)(255 * pulseIntensity), 
                    (unsigned char)(100 * pulseIntensity), 
                    0, 
                    255
                };
                
                // Draw grenade with a slightly larger radius
                DrawCircleV((Vector2){state->enemyBullets[i].base.x, state->enemyBullets[i].base.y}, 
                           state->enemyBullets[i].base.radius, bulletColor);

                DrawLineEx(
                    (Vector2){state->enemyBullets[i].base.x, state->enemyBullets[i].base.y - 3},
                    (Vector2){state->enemyBullets[i].base.x, state->enemyBullets[i].base.y + 3},
                    2.0f, WHITE
                );
            } else {
                // Check if this is an explosion bullet (smaller damage from grenade explosion)
                if (state->enemyBullets[i].damage == TANK_GRENADE_EXPLOSION_DAMAGE) {
                    // Explosion bullets are red
                    bulletColor = RED;
                } else {
                    // Normal bullets - color based on damage
                    bulletColor = (state->enemyBullets[i].damage >= TANK_ENEMY_BULLET_DAMAGE) ? RED : YELLOW;
                }
                
                DrawCircleV((Vector2){state->enemyBullets[i].base.x, state->enemyBullets[i].base.y}, 
                           state->enemyBullets[i].base.radius, bulletColor);
            }
        }
    }
}

void renderGame(const GameState* state) {
    BeginDrawing();
    
    // Clear screen with a very dark background for space
    ClearBackground((Color){5, 5, 15, 255});
    
    // Begin camera rendering
    BeginMode2D(state->camera);
    
    // Draw map boundaries
    DrawRectangleLines(0, 0, MAP_WIDTH, MAP_HEIGHT, BOUNDARY_COLOR);
    
    // Draw grid lines to help visualize the larger map
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
    
    // Draw powerups
    renderPowerups(state);
    
    // Draw aiming guide only in debug mode
    if (state->Debug) {
        Vector2 mousePosition = GetScreenToWorld2D(GetMousePosition(), state->camera);
        DrawLine(state->ship.base.x, state->ship.base.y, mousePosition.x, mousePosition.y, GRAY);
    }
    
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
    
    // Get current weapon ammo info
    int currentAmmo, maxAmmo;
    const char* weaponName;
    
    if (state->currentWeapon == WEAPON_SHOTGUN) {
        currentAmmo = state->shotgunAmmo;
        maxAmmo = SHOTGUN_MAX_AMMO;
        weaponName = "SHOTGUN";
    } else if (state->currentWeapon == WEAPON_GRENADE) {
        currentAmmo = state->grenadeAmmo;
        maxAmmo = GRENADE_MAX_AMMO;
        weaponName = "GRENADE";
    } else {
        currentAmmo = state->normalAmmo;
        maxAmmo = MAX_AMMO;
        weaponName = "RIFLE";
    }
    
    if (state->isReloading && state->currentWeapon == WEAPON_NORMAL) {
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
        // Draw ammo count with weapon type
        Color ammoColor = currentAmmo > (maxAmmo / 4) ? WHITE : RED;
        DrawText(TextFormat("%s: %d/%d", weaponName, currentAmmo, maxAmmo), 10, ammoY, 20, ammoColor);
        
        // Draw ammo indicators as small rectangles - positioned below the text
        int ammoRectSize = 5;
        int ammoRectSpacing = 3;
        int ammoStartX = 10; // Align with text
        int ammoIndicatorY = ammoY + 30; // Position below text with some spacing
        
        // Draw ammo indicators
        for (int i = 0; i < maxAmmo; i++) {
            Color rectColor;
            if (i < currentAmmo) {
                if (state->currentWeapon == WEAPON_SHOTGUN) {
                    rectColor = GREEN;
                } else if (state->currentWeapon == WEAPON_GRENADE) {
                    rectColor = ORANGE;
                } else {
                    rectColor = YELLOW;
                }
            } else {
                rectColor = DARKGRAY;
            }
            
            DrawRectangle(
                ammoStartX + (i * (ammoRectSize + ammoRectSpacing)), 
                ammoIndicatorY, 
                ammoRectSize, 
                ammoRectSize, 
                rectColor
            );
        }
    }
    
    // Draw weapon indicator
    int weaponIndicatorY = ammoY + 50;
    const char* weaponText;
    Color weaponColor;
    
    if (state->currentWeapon == WEAPON_SHOTGUN) {
        weaponText = "WEAPON: SHOTGUN";
        weaponColor = GREEN;
    } else if (state->currentWeapon == WEAPON_GRENADE) {
        weaponText = "WEAPON: GRENADE";
        weaponColor = ORANGE;
    } else {
        weaponText = "WEAPON: RIFLE";
        weaponColor = WHITE;
    }
    
    DrawText(weaponText, 10, weaponIndicatorY, 18, weaponColor);

    if (state->Debug) {
        // Draw debug information at bottom left
        int debugStartY = WINDOW_HEIGHT - 210; // Start 200 pixels from bottom
        
        DrawText(TextFormat("FPS: %d", GetFPS()), 10, debugStartY - 30, 20, WHITE);
        DrawText(TextFormat("Ship Position: (%.1f, %.1f)", state->ship.base.x, state->ship.base.y), 10, debugStartY, 20, WHITE);
        DrawText(TextFormat("Ship Velocity: (%.1f, %.1f)", state->ship.base.dx, state->ship.base.dy), 10, debugStartY + 30, 20, WHITE);

        // Count active asteroids and enemies
        int activeAsteroids = 0;
        for (int i = 0; i < MAX_ASTEROIDS; i++) {
            if (state->asteroids[i].base.active) {
                activeAsteroids++;
            }
        }
        
        int activeEnemies = 0;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (state->enemies[i].base.active) {
                activeEnemies++;
            }
        }
        
        DrawText(TextFormat("Active Asteroids: %d", activeAsteroids), 10, debugStartY + 60, 20, WHITE);
        DrawText(TextFormat("Active Enemies: %d", activeEnemies), 10, debugStartY + 90, 20, WHITE);
        DrawText("F4: Kill all asteroids", 10, debugStartY + 120, 20, YELLOW);
        DrawText("F5: Kill all enemies", 10, debugStartY + 150, 20, YELLOW);
        DrawText("F6: Skip to Next Wave", 10, debugStartY + 180, 20, YELLOW);
    }
    
    // Draw wave counter in bottom right
    DrawText(TextFormat("Wave: %d", state->currentWave), 
             WINDOW_WIDTH - MeasureText(TextFormat("Wave: %d", state->currentWave), 20) - 10,
             WINDOW_HEIGHT - 30,
             20, WHITE);
             

    DrawText(TextFormat("Asteroids: %d", state->asteroidsRemaining),
            WINDOW_WIDTH - MeasureText(TextFormat("Asteroids: %d", state->asteroidsRemaining), 20) - 10,
            WINDOW_HEIGHT - 60,
            20, WHITE);
    
            // Draw enemies counter (only if we're in a wave that has enemies)
    if (state->currentWave >= SCOUT_START_WAVE) {
        int enemiesRemaining = 0;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (state->enemies[i].base.active) {
                enemiesRemaining++;
            }
        }
        
        // Calculate enemies that haven't been spawned yet
        int enemiesNotYetSpawned = 0;
        if (state->maxEnemiesThisWave > state->enemiesSpawnedThisWave) {
            enemiesNotYetSpawned = state->maxEnemiesThisWave - state->enemiesSpawnedThisWave;
        }
        
        // Total remaining = currently active + not yet spawned
        int totalEnemiesRemaining = enemiesRemaining + enemiesNotYetSpawned;
        
        DrawText(TextFormat("Enemies: %d/%d", totalEnemiesRemaining, state->maxEnemiesThisWave),
                WINDOW_WIDTH - MeasureText(TextFormat("Enemies: %d/%d", totalEnemiesRemaining, state->maxEnemiesThisWave), 20) - 10,
                WINDOW_HEIGHT - 90,
                20, WHITE);
    }
    
    // Draw wave message if timer is active
    if (state->waveMessageTimer > 0) {
        int fontSize = 40;
        int textWidth = MeasureText(state->waveMessage, fontSize);
        
        // Calculate alpha for fade-out effect
        unsigned char alpha = (unsigned char)(255.0f * (state->waveMessageTimer > 1.0f ? 
                                                       1.0f : state->waveMessageTimer));
        
        // Draw with semi-transparent background for better visibility
        DrawRectangle(WINDOW_WIDTH/2 - textWidth/2 - 10,
                      WINDOW_HEIGHT/2 - fontSize/2 - 10,
                      textWidth + 20, fontSize + 20,
                      (Color){0, 0, 0, alpha/2});
                    
        DrawText(state->waveMessage,
                WINDOW_WIDTH/2 - textWidth/2,
                WINDOW_HEIGHT/2 - fontSize/2,
                fontSize,
                (Color){255, 255, 255, alpha});
    }
    
    EndDrawing();
}

void renderMenu(const GameState* state) {
    BeginDrawing();
    
    // Clear screen with a very dark background for space
    ClearBackground((Color){5, 5, 15, 255});
    
    // Draw animated background asteroids
    for (int i = 0; i < MAX_MENU_ASTEROIDS; i++) {
        if (state->menuAsteroids[i].active) {
            // Draw each menu asteroid as an octagon, similar to game asteroids
            Vector2 points[8];
            for (int j = 0; j < 8; j++) {
                float angle = state->menuAsteroids[i].angle + j * 45.0f;  // 360 / 8 = 45 degrees
                float radians = angle * PI / 180.0f;
                points[j].x = state->menuAsteroids[i].x + sin(radians) * state->menuAsteroids[i].radius;
                points[j].y = state->menuAsteroids[i].y - cos(radians) * state->menuAsteroids[i].radius;
            }
            
            // Draw the asteroid outlines
            for (int j = 0; j < 8; j++) {
                int next = (j + 1) % 8;
                DrawLineV(points[j], points[next], (Color){150, 150, 150, 200});
            }
        }
    }
    
    // Calculate estimated height of scoreboard
    int scoreboardHeight = 50 + (MAX_HIGH_SCORES * 30);  // Header + (rows * row height)
    int scoreboardY = (WINDOW_HEIGHT - scoreboardHeight) / 2;  // Center vertically
    
    // Draw scoreboard on left side
    renderScoreboard(state, 20, scoreboardY, 350);
    
    // Calculate right side area (for title and buttons)
    int rightSideX = 400;  // Starting X position of right side content
    int rightSideWidth = WINDOW_WIDTH - rightSideX - 20;  // Width of right side area
    int centerRightX = rightSideX + rightSideWidth/2;  // Center X of right side
    
    // Draw title centered in the right area
    const char* title = "ASTEROIDS";
    int titleWidth = MeasureText(title, TITLE_FONT_SIZE);
    DrawText(title, centerRightX - titleWidth/2, WINDOW_HEIGHT/4, TITLE_FONT_SIZE, WHITE);
    
    // Adjust button positions to be centered in the right area
    Rectangle playButtonPos = {
        centerRightX - BUTTON_WIDTH/2,
        WINDOW_HEIGHT/2,
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    };
    
    Rectangle optionsButtonPos = {
        centerRightX - BUTTON_WIDTH/2,
        WINDOW_HEIGHT/2 + (BUTTON_HEIGHT + 20),
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    };
    
    Rectangle quitButtonPos = {
        centerRightX - BUTTON_WIDTH/2,
        WINDOW_HEIGHT/2 + 2 * (BUTTON_HEIGHT + 20),
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    };
    
    // Draw play button
    Vector2 mousePoint = GetMousePosition();
    bool isMouseOverPlayButton = CheckCollisionPointRec(mousePoint, playButtonPos);
    
    Color playButtonColor = isMouseOverPlayButton ? GREEN : DARKGREEN;
    DrawRectangleRec(playButtonPos, playButtonColor);
    DrawRectangleLinesEx(playButtonPos, 2, WHITE);
    
    // Draw play button text
    const char* playButtonText = "PLAY";
    int playButtonTextWidth = MeasureText(playButtonText, BUTTON_FONT_SIZE);
    DrawText(
        playButtonText,
        playButtonPos.x + playButtonPos.width/2 - playButtonTextWidth/2,
        playButtonPos.y + playButtonPos.height/2 - BUTTON_FONT_SIZE/2,
        BUTTON_FONT_SIZE,
        WHITE
    );
    
    // Draw options button
    bool isMouseOverOptionsButton = CheckCollisionPointRec(mousePoint, optionsButtonPos);
    Color optionsButtonColor = isMouseOverOptionsButton ? BLUE : DARKBLUE;
    DrawRectangleRec(optionsButtonPos, optionsButtonColor);
    DrawRectangleLinesEx(optionsButtonPos, 2, WHITE);
    
    // Draw options button text
    const char* optionsButtonText = "OPTIONS";
    int optionsButtonTextWidth = MeasureText(optionsButtonText, BUTTON_FONT_SIZE);
    DrawText(
        optionsButtonText,
        optionsButtonPos.x + optionsButtonPos.width/2 - optionsButtonTextWidth/2,
        optionsButtonPos.y + optionsButtonPos.height/2 - BUTTON_FONT_SIZE/2,
        BUTTON_FONT_SIZE,
        WHITE
    );
    
    // Draw quit button
    bool isMouseOverQuitButton = CheckCollisionPointRec(mousePoint, quitButtonPos);
    
    Color quitButtonColor = isMouseOverQuitButton ? RED : MAROON;
    DrawRectangleRec(quitButtonPos, quitButtonColor);
    DrawRectangleLinesEx(quitButtonPos, 2, WHITE);
    
    // Draw quit button text
    const char* quitButtonText = "QUIT";
    int quitButtonTextWidth = MeasureText(quitButtonText, BUTTON_FONT_SIZE);
    DrawText(
        quitButtonText,
        quitButtonPos.x + quitButtonPos.width/2 - quitButtonTextWidth/2,
        quitButtonPos.y + quitButtonPos.height/2 - BUTTON_FONT_SIZE/2,
        BUTTON_FONT_SIZE,
        WHITE
    );
             
    // Draw version number in the bottom right corner
    DrawText(VERSION_NUMBER, 
             WINDOW_WIDTH - MeasureText(VERSION_NUMBER, 16) - 10,
             WINDOW_HEIGHT - 25,
             16, GRAY);
    
    EndDrawing();
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
    
    // Calculate button positions centered on screen
    Rectangle resumeButtonPos = {
        WINDOW_WIDTH/2 - BUTTON_WIDTH/2,
        WINDOW_HEIGHT/2 - BUTTON_HEIGHT - 20,
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    };
    
    Rectangle optionsButtonPos = {
        WINDOW_WIDTH/2 - BUTTON_WIDTH/2,
        WINDOW_HEIGHT/2,
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    };
    
    Rectangle quitButtonPos = {
        WINDOW_WIDTH/2 - BUTTON_WIDTH/2,
        WINDOW_HEIGHT/2 + BUTTON_HEIGHT + 20,
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    };
    
    // Draw resume button
    Vector2 mousePoint = GetMousePosition();
    bool isMouseOverResumeButton = CheckCollisionPointRec(mousePoint, resumeButtonPos);
    
    Color resumeButtonColor = isMouseOverResumeButton ? GREEN : DARKGREEN;
    DrawRectangleRec(resumeButtonPos, resumeButtonColor);
    DrawRectangleLinesEx(resumeButtonPos, 2, WHITE);
    
    // Draw resume button text
    const char* resumeButtonText = "RESUME";
    int resumeButtonTextWidth = MeasureText(resumeButtonText, BUTTON_FONT_SIZE);
    DrawText(
        resumeButtonText,
        resumeButtonPos.x + resumeButtonPos.width/2 - resumeButtonTextWidth/2,
        resumeButtonPos.y + resumeButtonPos.height/2 - BUTTON_FONT_SIZE/2,
        BUTTON_FONT_SIZE,
        WHITE
    );
    
    // Draw options button
    bool isMouseOverOptionsButton = CheckCollisionPointRec(mousePoint, optionsButtonPos);
    Color optionsButtonColor = isMouseOverOptionsButton ? BLUE : DARKBLUE;
    DrawRectangleRec(optionsButtonPos, optionsButtonColor);
    DrawRectangleLinesEx(optionsButtonPos, 2, WHITE);
    
    // Draw options button text
    const char* optionsButtonText = "OPTIONS";
    int optionsButtonTextWidth = MeasureText(optionsButtonText, BUTTON_FONT_SIZE);
    DrawText(
        optionsButtonText,
        optionsButtonPos.x + optionsButtonPos.width/2 - optionsButtonTextWidth/2,
        optionsButtonPos.y + optionsButtonPos.height/2 - BUTTON_FONT_SIZE/2,
        BUTTON_FONT_SIZE,
        WHITE
    );
    
    // Draw quit button
    bool isMouseOverQuitButton = CheckCollisionPointRec(mousePoint, quitButtonPos);
    
    Color quitButtonColor = isMouseOverQuitButton ? RED : MAROON;
    DrawRectangleRec(quitButtonPos, quitButtonColor);
    DrawRectangleLinesEx(quitButtonPos, 2, WHITE);
    
    // Draw quit button text
    const char* quitButtonText = "QUIT";
    int quitButtonTextWidth = MeasureText(quitButtonText, BUTTON_FONT_SIZE);
    DrawText(
        quitButtonText,
        quitButtonPos.x + quitButtonPos.width/2 - quitButtonTextWidth/2,
        quitButtonPos.y + quitButtonPos.height/2 - BUTTON_FONT_SIZE/2,
        BUTTON_FONT_SIZE,
        WHITE
    );
    
    EndDrawing();
}

void renderOptions(const GameState* state) {
    BeginDrawing();
    
    // Clear screen with a very dark background for space
    ClearBackground((Color){5, 5, 15, 255});
    
    // Draw title
    const char* title = "OPTIONS";
    int titleWidth = MeasureText(title, TITLE_FONT_SIZE);
    DrawText(title, WINDOW_WIDTH/2 - titleWidth/2, WINDOW_HEIGHT/4, TITLE_FONT_SIZE, WHITE);
    
    // Draw volume label
    const char* volumeText = "SOUND VOLUME";
    int volumeTextWidth = MeasureText(volumeText, OPTIONS_FONT_SIZE);
    DrawText(
        volumeText,
        WINDOW_WIDTH/2 - volumeTextWidth/2,
        state->volumeSlider.y - 40,
        OPTIONS_FONT_SIZE,
        WHITE
    );
    
    // Draw slider background
    DrawRectangleRec(state->volumeSlider, DARKGRAY);
    
    // Draw filled slider part based on current volume
    Rectangle filledSlider = {
        state->volumeSlider.x,
        state->volumeSlider.y,
        state->volumeSlider.width * state->soundVolume,
        state->volumeSlider.height
    };
    DrawRectangleRec(filledSlider, BLUE);
    
    // Draw slider border
    DrawRectangleLinesEx(state->volumeSlider, 2, WHITE);
    
    // Draw slider handle
    float handleX = state->volumeSlider.x + (state->volumeSlider.width * state->soundVolume);
    DrawRectangle(
        handleX - 5, 
        state->volumeSlider.y - 5, 
        10, 
        state->volumeSlider.height + 10,
        WHITE
    );
    
    // Draw volume percentage
    char volumePercentText[10];
    sprintf(volumePercentText, "%d%%", (int)(state->soundVolume * 100));
    int volumePercentWidth = MeasureText(volumePercentText, OPTIONS_FONT_SIZE);
    DrawText(
        volumePercentText,
        WINDOW_WIDTH/2 - volumePercentWidth/2,
        state->volumeSlider.y + state->volumeSlider.height + 20,
        OPTIONS_FONT_SIZE,
        WHITE
    );

    // Draw music volume label
    const char* musicVolumeText = "MUSIC VOLUME";
    int musicVolumeTextWidth = MeasureText(musicVolumeText, OPTIONS_FONT_SIZE);
    DrawText(
        musicVolumeText,
        WINDOW_WIDTH/2 - musicVolumeTextWidth/2,
        state->musicVolumeSlider.y,
        OPTIONS_FONT_SIZE,
        WHITE
    );
    
    // Draw music slider background
    Rectangle musicSliderBackground = {
        state->musicVolumeSlider.x,
        state->musicVolumeSlider.y + 40,
        state->musicVolumeSlider.width,
        state->musicVolumeSlider.height
    };
    DrawRectangleRec(musicSliderBackground, DARKGRAY);
    
    // Draw filled music slider part based on current volume
    Rectangle filledMusicSlider = {
        state->musicVolumeSlider.x,
        state->musicVolumeSlider.y + 40,
        state->musicVolumeSlider.width * state->musicVolume,
        state->musicVolumeSlider.height
    };
    DrawRectangleRec(filledMusicSlider, (Color){100, 100, 255, 255});  // Different color for music
    
    // Draw music slider border
    Rectangle musicSliderBorder = {
        state->musicVolumeSlider.x,
        state->musicVolumeSlider.y + 40,
        state->musicVolumeSlider.width,
        state->musicVolumeSlider.height
    };
    DrawRectangleLinesEx(musicSliderBorder, 2, WHITE);
    
    // Draw music slider handle
    float musicHandleX = state->musicVolumeSlider.x + (state->musicVolumeSlider.width * state->musicVolume);
    DrawRectangle(
        musicHandleX - 5, 
        state->musicVolumeSlider.y + 35, 
        10, 
        state->musicVolumeSlider.height + 10,
        WHITE
    );
    
    // Draw music volume percentage
    char musicVolumePercentText[10];
    sprintf(musicVolumePercentText, "%d%%", (int)(state->musicVolume * 100));
    int musicVolumePercentWidth = MeasureText(musicVolumePercentText, OPTIONS_FONT_SIZE);
    DrawText(
        musicVolumePercentText,
        WINDOW_WIDTH/2 - musicVolumePercentWidth/2,
        state->musicVolumeSlider.y + 35 + state->musicVolumeSlider.height + 20,
        OPTIONS_FONT_SIZE,
        WHITE
    );
    
    // Draw back button
    Vector2 mousePoint = GetMousePosition();
    bool isMouseOverBackButton = CheckCollisionPointRec(mousePoint, state->backButton);
    
    Color backButtonColor = isMouseOverBackButton ? GREEN : DARKGREEN;
    DrawRectangleRec(state->backButton, backButtonColor);
    DrawRectangleLinesEx(state->backButton, 2, WHITE);
    
    // Draw back button text
    const char* backButtonText = "BACK";
    int backButtonTextWidth = MeasureText(backButtonText, BUTTON_FONT_SIZE);
    DrawText(
        backButtonText,
        state->backButton.x + state->backButton.width/2 - backButtonTextWidth/2,
        state->backButton.y + state->backButton.height/2 - BUTTON_FONT_SIZE/2,
        BUTTON_FONT_SIZE,
        WHITE
    );
    
    // Draw version number in the bottom right corner
    DrawText(VERSION_NUMBER, 
             WINDOW_WIDTH - MeasureText(VERSION_NUMBER, 16) - 10,
             WINDOW_HEIGHT - 25,
             16, GRAY);
    
    EndDrawing();
}

void renderGameOver(const GameState* state) {
    BeginDrawing();
    
    // Clear screen with a very dark background for space
    ClearBackground((Color){5, 5, 15, 255});
    
    // Draw game over title
    const char* gameOverTitle = "GAME OVER";
    int titleWidth = MeasureText(gameOverTitle, TITLE_FONT_SIZE);
    DrawText(gameOverTitle, WINDOW_WIDTH/2 - titleWidth/2, WINDOW_HEIGHT/4, TITLE_FONT_SIZE, RED);
    
    // Draw final score
    char scoreText[64];
    sprintf(scoreText, "Final Score: %d", state->score);
    int scoreWidth = MeasureText(scoreText, BUTTON_FONT_SIZE);
    DrawText(
        scoreText,
        WINDOW_WIDTH/2 - scoreWidth/2,
        WINDOW_HEIGHT/3 + 30,
        BUTTON_FONT_SIZE,
        WHITE
    );

    // Show the wave the player reached
    char waveText[64];
    sprintf(waveText, "Reached Wave: %d", state->currentWave);
    int waveWidth = MeasureText(waveText, BUTTON_FONT_SIZE);
    DrawText(
        waveText,
        WINDOW_WIDTH/2 - waveWidth/2,
        WINDOW_HEIGHT/3 + 70,
        BUTTON_FONT_SIZE,
        WHITE
    );
    
    // Draw main menu button
    Vector2 mousePoint = GetMousePosition();
    bool isMouseOverMainMenuButton = CheckCollisionPointRec(mousePoint, state->mainMenuButton);
    
    Color mainMenuButtonColor = isMouseOverMainMenuButton ? GREEN : DARKGREEN;
    DrawRectangleRec(state->mainMenuButton, mainMenuButtonColor);
    DrawRectangleLinesEx(state->mainMenuButton, 2, WHITE);
    
    // Draw button text
    const char* mainMenuButtonText = "MAIN MENU";
    int mainMenuButtonTextWidth = MeasureText(mainMenuButtonText, BUTTON_FONT_SIZE);
    DrawText(
        mainMenuButtonText,
        state->mainMenuButton.x + state->mainMenuButton.width/2 - mainMenuButtonTextWidth/2,
        state->mainMenuButton.y + state->mainMenuButton.height/2 - BUTTON_FONT_SIZE/2,
        BUTTON_FONT_SIZE,
        WHITE
    );
    
    // Draw version number in the bottom right corner
    DrawText(VERSION_NUMBER, 
             WINDOW_WIDTH - MeasureText(VERSION_NUMBER, 16) - 10,
             WINDOW_HEIGHT - 25,
             16, GRAY);
    
    EndDrawing();
}

void renderInfo(const GameState* state) {
    BeginDrawing();
    
    // Clear screen with a very dark background for space
    ClearBackground((Color){5, 5, 15, 255});
    
    // Draw title 
    const char* title = "GAME INFORMATION";
    int titleWidth = MeasureText(title, TITLE_FONT_SIZE);
    DrawText(title, WINDOW_WIDTH/2 - titleWidth/2, WINDOW_HEIGHT/4 - 30, TITLE_FONT_SIZE, WHITE);
    
    // Center the two columns on screen
    int totalContentWidth = 800; 
    int startX = (WINDOW_WIDTH - totalContentWidth) / 2;
    int leftColumnX = startX;
    int rightColumnX = startX + totalContentWidth / 2 + 80; 
    
    // Left Column - Controls and Weapons
    int currentY = WINDOW_HEIGHT/4 + 45; 
    
    // Controls section
    DrawText("CONTROLS:", leftColumnX, currentY, 30, (Color){255, 200, 100, 255});
    currentY += 45;
    
    DrawText("WASD - Move ship", leftColumnX, currentY, 22, WHITE);
    currentY += 28;
    DrawText("Mouse - Aim and shoot", leftColumnX, currentY, 22, WHITE);
    currentY += 28;
    DrawText("R - Reload", leftColumnX, currentY, 22, WHITE);
    currentY += 28;
    DrawText("P - Pause", leftColumnX, currentY, 22, WHITE);
    currentY += 50;
    
    // Weapons section
    DrawText("WEAPONS & POWERUPS:", leftColumnX, currentY, 30, (Color){255, 150, 100, 255});
    currentY += 45;
    
    // Health powerup
    bool drewHealthTexture = false;
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (state->powerups[i].type == POWERUP_HEALTH && state->powerups[i].texture.id > 0) {
            float scale = 32.0f / state->powerups[i].texture.width;
            Rectangle source = { 0, 0, state->powerups[i].texture.width, state->powerups[i].texture.height };
            Rectangle dest = { leftColumnX + 5, currentY + 2, 
                             state->powerups[i].texture.width * scale, 
                             state->powerups[i].texture.height * scale };
            Vector2 origin = { 0, 0 };
            DrawTexturePro(state->powerups[i].texture, source, dest, origin, 0.0f, WHITE);
            drewHealthTexture = true;
            break;
        }
    }
    if (!drewHealthTexture) {
        DrawCircleV((Vector2){leftColumnX + 20, currentY + 16}, 14, (Color){255, 100, 100, 255});
        DrawText("+", leftColumnX + 16, currentY + 8, 16, WHITE);
    }
    DrawText("Health - Restores 20 health", leftColumnX + 45, currentY + 8, 18, WHITE);
    currentY += 40; 

    // Life powerup
    bool drewLifeTexture = false;
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (state->powerups[i].type == POWERUP_LIFE && state->powerups[i].texture.id > 0) {
            float scale = 32.0f / state->powerups[i].texture.width;
            Rectangle source = { 0, 0, state->powerups[i].texture.width, state->powerups[i].texture.height };
            Rectangle dest = { leftColumnX + 5, currentY + 2, 
                            state->powerups[i].texture.width * scale, 
                            state->powerups[i].texture.height * scale };
            Vector2 origin = { 0, 0 };
            DrawTexturePro(state->powerups[i].texture, source, dest, origin, 0.0f, WHITE);
            drewLifeTexture = true;
            break;
        }
    }
    if (!drewLifeTexture) {
        DrawCircleV((Vector2){leftColumnX + 20, currentY + 16}, 14, (Color){255, 100, 255, 255});
        DrawText("1UP", leftColumnX + 8, currentY + 8, 14, WHITE);
    }
    DrawText("Extra Life - Gain one additional life", leftColumnX + 45, currentY + 8, 18, WHITE);
    currentY += 40; 
    
    // Shotgun powerup
    bool drewShotgunTexture = false;
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (state->powerups[i].type == POWERUP_SHOTGUN && state->powerups[i].texture.id > 0) {
            float scale = 32.0f / state->powerups[i].texture.width;
            Rectangle source = { 0, 0, state->powerups[i].texture.width, state->powerups[i].texture.height };
            Rectangle dest = { leftColumnX + 5, currentY + 2, 
                             state->powerups[i].texture.width * scale, 
                             state->powerups[i].texture.height * scale };
            Vector2 origin = { 0, 0 };
            DrawTexturePro(state->powerups[i].texture, source, dest, origin, 0.0f, WHITE);
            drewShotgunTexture = true;
            break;
        }
    }
    if (!drewShotgunTexture) {
        DrawCircleV((Vector2){leftColumnX + 20, currentY + 16}, 14, (Color){100, 255, 100, 255});
        DrawText("S", leftColumnX + 16, currentY + 8, 16, WHITE);
    }
    DrawText("Shotgun - 10 shots", leftColumnX + 45, currentY + 8, 18, WHITE);
    currentY += 40; 
    
    // Grenade powerup
    bool drewGrenadeTexture = false;
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (state->powerups[i].type == POWERUP_GRENADE && state->powerups[i].texture.id > 0) {
            float scale = 32.0f / state->powerups[i].texture.width;
            Rectangle source = { 0, 0, state->powerups[i].texture.width, state->powerups[i].texture.height };
            Rectangle dest = { leftColumnX + 5, currentY + 2, 
                             state->powerups[i].texture.width * scale, 
                             state->powerups[i].texture.height * scale };
            Vector2 origin = { 0, 0 };
            DrawTexturePro(state->powerups[i].texture, source, dest, origin, 0.0f, WHITE);
            drewGrenadeTexture = true;
            break;
        }
    }
    if (!drewGrenadeTexture) {
        DrawCircleV((Vector2){leftColumnX + 20, currentY + 16}, 14, (Color){255, 165, 0, 255});
        DrawText("G", leftColumnX + 16, currentY + 8, 16, WHITE);
    }
    DrawText("Grenade - 5 explosive shots", leftColumnX + 45, currentY + 8, 18, WHITE);
    
    // Right Column - Enemies and Objectives
    currentY = WINDOW_HEIGHT/4 + 45; 
    
    // Enemies section
    DrawText("ENEMIES:", rightColumnX, currentY, 30, (Color){255, 100, 100, 255});
    currentY += 45;
    
    // Scout enemy
    bool drewScoutTexture = false;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (state->enemies[i].type == ENEMY_SCOUT && state->enemies[i].texture.id > 0) {
            float scale = 32.0f / state->enemies[i].texture.width;
            Rectangle source = { 0, 0, state->enemies[i].texture.width, state->enemies[i].texture.height };
            Rectangle dest = { rightColumnX + 5, currentY + 4, 
                             state->enemies[i].texture.width * scale, 
                             state->enemies[i].texture.height * scale };
            Vector2 origin = { 0, 0 };
            DrawTexturePro(state->enemies[i].texture, source, dest, origin, 0.0f, WHITE);
            drewScoutTexture = true;
            break;
        }
    }
    if (!drewScoutTexture) {
        Vector2 scoutPoints[3] = {
            {rightColumnX + 20, currentY + 8},
            {rightColumnX + 10, currentY + 23},
            {rightColumnX + 30, currentY + 23}
        };
        DrawTriangleLines(scoutPoints[0], scoutPoints[1], scoutPoints[2], SKYBLUE);
    }
    DrawText("Scout - Fast, drops shotgun", rightColumnX + 45, currentY + 8, 18, WHITE);
    currentY += 35;
    
    // Tank enemy
    bool drewTankTexture = false;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (state->enemies[i].type == ENEMY_TANK && state->enemies[i].texture.id > 0) {
            float scale = 32.0f / state->enemies[i].texture.width;
            Rectangle source = { 0, 0, state->enemies[i].texture.width, state->enemies[i].texture.height };
            Rectangle dest = { rightColumnX + 5, currentY + 4, 
                             state->enemies[i].texture.width * scale, 
                             state->enemies[i].texture.height * scale };
            Vector2 origin = { 0, 0 };
            DrawTexturePro(state->enemies[i].texture, source, dest, origin, 0.0f, WHITE);
            drewTankTexture = true;
            break;
        }
    }
    if (!drewTankTexture) {
        DrawRectangleLines(rightColumnX + 10, currentY + 8, 24, 16, RED);
    }
    DrawText("Tank - Strong, drops grenade", rightColumnX + 45, currentY + 8, 18, WHITE);
    currentY += 50;
    
    // Game objective
    DrawText("OBJECTIVES:", rightColumnX, currentY, 30, (Color){255, 255, 100, 255});
    currentY += 45;
    DrawText("-> Destroy all asteroids", rightColumnX, currentY, 20, WHITE);
    currentY += 28;
    DrawText("-> Eliminate enemies for points", rightColumnX, currentY, 20, WHITE);
    currentY += 28;
    DrawText("-> Survive as long as possible", rightColumnX, currentY, 20, WHITE);
    
    // Instructions to continue - centered at bottom 
    const char* continueText = "Press Z to start playing!";
    int continueWidth = MeasureText(continueText, OPTIONS_FONT_SIZE);
    DrawText(continueText, 
             WINDOW_WIDTH/2 - continueWidth/2, 
             WINDOW_HEIGHT - 100, 
             OPTIONS_FONT_SIZE, 
             WHITE);
    
    // Draw version number in the bottom right corner 
    DrawText(VERSION_NUMBER, 
             WINDOW_WIDTH - MeasureText(VERSION_NUMBER, 16) - 10,
             WINDOW_HEIGHT - 25,
             16, GRAY);
    
    EndDrawing();
}