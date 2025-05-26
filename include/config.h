#ifndef CONFIG_H
#define CONFIG_H

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
#define RELOAD_TIME 5.0f
#define FIRE_RATE 0.2f  // Time between shots in seconds
#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 50
#define TITLE_FONT_SIZE 60
#define BUTTON_FONT_SIZE 30
#define MAX_ENEMIES 10
#define TANK_ENEMY_RADIUS 40.0f
#define TANK_ENEMY_SPEED 1.2f
#define TANK_ENEMY_HEALTH 100
#define TANK_ENEMY_BULLET_DAMAGE 40
#define TANK_ENEMY_FIRE_RATE 2.0f
#define TANK_ENEMY_SCORE 500
#define SCOUT_ENEMY_RADIUS 20.0f
#define SCOUT_ENEMY_SPEED 3.5f
#define SCOUT_ENEMY_HEALTH 30
#define SCOUT_ENEMY_BULLET_DAMAGE 5
#define SCOUT_ENEMY_FIRE_RATE 0.1f
#define SCOUT_ENEMY_BURST_COUNT 5
#define SCOUT_ENEMY_BURST_DELAY 0.1f
#define SCOUT_ENEMY_SCORE 300
#define SCOUT_GROUP_RADIUS 250.0f       // Distance to consider scouts as part of same group
#define SCOUT_GROUP_COHESION 0.3f       // Strength of group cohesion force
#define SCOUT_GROUP_ATTACK_DELAY 0.5f   // Delay between group members attacking
#define SCOUT_GROUP_CHANCE 75           // Percent chance a scout will try to join/form a group
#define SCOUT_SEPARATION_RADIUS 40.0f   // Minimum distance between scouts in a group
#define SCOUT_SEPARATION_FORCE 0.5f     // Strength of separation force to prevent clipping
#define ENEMY_BULLET_SPEED 6.0f
#define MAX_ENEMY_BULLETS 50
#define ENEMY_DETECTION_RADIUS 500.0f
#define ENEMY_SPAWN_TIME 15.0f
#define SCOUT_ENEMY_ATTACK_DISTANCE 250.0f
#define TANK_ENEMY_ATTACK_DISTANCE 400.0f
#define MAX_SOUNDS 7  // Maximum number of sounds we'll load
#define SOUND_SHOOT 0
#define SOUND_RELOAD_START 1
#define SOUND_RELOAD_FINISH 2
#define SOUND_ASTEROID_HIT 3
#define SOUND_SHIP_HIT 4
#define SOUND_ENEMY_SHOOT 5
#define SOUND_ENEMY_EXPLODE 6
#define BASE_ASTEROID_COUNT 10
#define ASTEROID_INCREMENT 3
#define SCOUT_START_WAVE 3
#define TANK_START_WAVE 5
#define WAVE_DELAY 3.0f  // Delay between waves in seconds
#define SLIDER_WIDTH 300
#define SLIDER_HEIGHT 20
#define OPTIONS_FONT_SIZE 24
#define VERSION_NUMBER "v1.1.0"
#define INVULNERABILITY_TIME 3.0f    // Invulnerability period in seconds after respawning or getting hit
#define BLINK_FREQUENCY 0.1f         // How often the ship blinks during invulnerability (in seconds)

#endif // CONFIG_H