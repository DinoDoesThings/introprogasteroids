#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// GAME VERSION
// =============================================================================
#define VERSION_NUMBER "v2.2.0"

// =============================================================================
// WINDOW & MAP SETTINGS
// =============================================================================
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define MAP_WIDTH 2500
#define MAP_HEIGHT 1000
#define BOUNDARY_COLOR (Color){ 30, 30, 80, 255 }  // Dark blue boundary

// =============================================================================
// PLAYER SHIP SETTINGS
// =============================================================================
#define SHIP_ACCELERATION 0.2f
#define SHIP_MAX_SPEED 5.0f
#define ROTATION_SPEED 5.0f
#define FRICTION 0.98f
#define MAX_HEALTH 100
#define INVULNERABILITY_TIME 3.0f    // Invulnerability period in seconds after respawning or getting hit
#define BLINK_FREQUENCY 0.1f         // How often the ship blinks during invulnerability (in seconds)

// =============================================================================
// PLAYER WEAPONS & AMMUNITION
// =============================================================================
#define MAX_BULLETS 30
#define BULLET_SPEED 10.0f

// =============================================================================
// WEAPON SETTINGS
// =============================================================================
#define MAX_AMMO 30
#define SHOTGUN_MAX_AMMO 10
#define SHOTGUN_PELLETS 5              // Number of bullets per shotgun shot
#define SHOTGUN_SPREAD_ANGLE 30.0f     // Spread angle in degrees
#define SHOTGUN_FIRE_RATE 1.0f         // 1 second cooldown between shotgun shots
#define GRENADE_MAX_AMMO 5             // Number of grenades player can fire
#define GRENADE_FIRE_RATE 1.5f         // 1.5 second cooldown between grenade shots
#define RELOAD_TIME 5.0f
#define FIRE_RATE 0.35f  // Time between shots in seconds for normal weapon

// Player Grenade Settings
#define PLAYER_GRENADE_TIMER 2.5f      // Time before player grenade explodes
#define PLAYER_GRENADE_EXPLOSION_DAMAGE 35 // Damage of player grenade explosion bullets
#define PLAYER_GRENADE_EXPLOSION_SPEED 5.0f // Speed of explosion bullets
#define PLAYER_GRENADE_EXPLOSION_COUNT 8    // Number of bullets (8 directions)

// =============================================================================
// ASTEROIDS
// =============================================================================
#define MAX_ASTEROIDS 50
#define BASE_ASTEROID_COUNT 10
#define ASTEROID_INCREMENT 3
#define LARGE_ASTEROID_DAMAGE 40
#define MEDIUM_ASTEROID_DAMAGE 20
#define SMALL_ASTEROID_DAMAGE 10

// =============================================================================
// PARTICLE EFFECTS
// =============================================================================
#define MAX_PARTICLES 500
#define PARTICLE_LIFETIME 1.0f
#define PARTICLE_SPEED 2.0f

// =============================================================================
// ENEMY SETTINGS
// =============================================================================
#define MAX_ENEMIES 20
#define MAX_ENEMY_BULLETS 100
#define ENEMY_BULLET_SPEED 6.0f
#define ENEMY_DETECTION_RADIUS 500.0f
#define ENEMY_SPAWN_TIME 10.0f

// Tank Enemy
#define TANK_ENEMY_RADIUS 40.0f
#define TANK_ENEMY_SPEED 1.2f
#define TANK_ENEMY_HEALTH 100
#define TANK_ENEMY_BULLET_DAMAGE 40
#define TANK_ENEMY_FIRE_RATE 2.0f
#define TANK_ENEMY_ATTACK_DISTANCE 400.0f
#define TANK_ENEMY_SCORE 500

// Tank Grenade Settings
#define TANK_GRENADE_TIMER 2.0f          // Time before explosion (seconds)
#define TANK_GRENADE_EXPLOSION_DAMAGE 25 // Damage of explosion bullets
#define TANK_GRENADE_EXPLOSION_SPEED 4.0f // Speed of explosion bullets
#define TANK_GRENADE_EXPLOSION_COUNT 8    // Number of bullets (8 directions: cardinal + intercardinal)

// Scout Enemy
#define SCOUT_ENEMY_RADIUS 20.0f
#define SCOUT_ENEMY_SPEED 3.5f
#define SCOUT_ENEMY_HEALTH 30
#define SCOUT_ENEMY_BULLET_DAMAGE 5
#define SCOUT_ENEMY_FIRE_RATE 0.1f
#define SCOUT_ENEMY_BURST_COUNT 5
#define SCOUT_ENEMY_BURST_DELAY 0.1f
#define SCOUT_ENEMY_ATTACK_DISTANCE 250.0f
#define SCOUT_ENEMY_SCORE 300

// Scout Group Behavior
#define SCOUT_GROUP_RADIUS 250.0f       // Distance to consider scouts as part of same group
#define SCOUT_GROUP_COHESION 0.3f       // Strength of group cohesion force
#define SCOUT_GROUP_ATTACK_DELAY 0.5f   // Delay between group members attacking
#define SCOUT_GROUP_CHANCE 75           // Percent chance a scout will try to join/form a group
#define SCOUT_SEPARATION_RADIUS 40.0f   // Minimum distance between scouts in a group
#define SCOUT_SEPARATION_FORCE 0.8f     // Strength of separation force to prevent clipping

// =============================================================================
// WAVE SYSTEM
// =============================================================================
#define SCOUT_START_WAVE 3
#define TANK_START_WAVE 5
#define WAVE_DELAY 3.0f  // Delay between waves in seconds

// =============================================================================
// AUDIO SETTINGS
// =============================================================================
#define MAX_SOUNDS 9  // Maximum number of sounds the program will load
#define SOUND_SHOOT 0
#define SOUND_RELOAD_START 1
#define SOUND_RELOAD_FINISH 2
#define SOUND_ASTEROID_HIT 3
#define SOUND_SHIP_HIT 4
#define SOUND_SCOUT_SHOOT 5
#define SOUND_TANK_SHOOT 6
#define SOUND_ENEMY_EXPLODE 7
#define SOUND_POWERUP_PICKUP 8

// =============================================================================
// TEXTURE SETTINGS
// =============================================================================
// Player Ship
#define SHIP_TEXTURE_PATH "resources/ships/playership.png"
#define SHIP_TEXTURE_SCALE 1.0f

// Enemy Ships
#define SCOUT_TEXTURE_PATH "resources/ships/scout.png"
#define SCOUT_TEXTURE_SCALE 1.0f

#define TANK_TEXTURE_PATH "resources/ships/tank.png"
#define TANK_TEXTURE_SCALE 1.3f   // 30% bigger than scout

// Misc
#define CROSSHAIR_TEXTURE_PATH "resources/misc/crosshair.png"

// Powerup Textures
#define HEALTH_POWERUP_TEXTURE_PATH "resources/powerups/health.png"
#define SHOTGUN_POWERUP_TEXTURE_PATH "resources/powerups/shotgun.png"
#define GRENADE_POWERUP_TEXTURE_PATH "resources/powerups/grenade.png"
#define POWERUP_TEXTURE_SCALE 0.8f

// =============================================================================
// UI SETTINGS
// =============================================================================
#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 50
#define SLIDER_WIDTH 300
#define SLIDER_HEIGHT 20
#define TITLE_FONT_SIZE 60
#define BUTTON_FONT_SIZE 30
#define OPTIONS_FONT_SIZE 24

// =============================================================================
// ENEMY PARTICLE SETTINGS
// =============================================================================
// Tank Enemy Particles
#define TANK_PARTICLE_MIN_RADIUS 2
#define TANK_PARTICLE_MAX_RADIUS 4
#define TANK_PARTICLE_RANDOM_RANGE 3.0f
#define TANK_PARTICLE_REAR_OFFSET 0.9f
#define TANK_PARTICLE_SPEED_MULTIPLIER 0.8f

// Scout Enemy Particles
#define SCOUT_PARTICLE_MIN_RADIUS 1
#define SCOUT_PARTICLE_MAX_RADIUS 3
#define SCOUT_PARTICLE_RANDOM_RANGE 2.0f
#define SCOUT_PARTICLE_REAR_OFFSET 0.9f
#define SCOUT_PARTICLE_SPEED_MULTIPLIER 0.8f

// =============================================================================
// POWERUP SETTINGS
// =============================================================================
#define MAX_POWERUPS 20
#define HEALTH_POWERUP_HEAL_AMOUNT 20
#define HEALTH_POWERUP_DROP_CHANCE 10  // 10% chance
#define SHOTGUN_DROP_CHANCE 20         // 20% chance for shotgun drop from scouts
#define GRENADE_DROP_CHANCE 20         // 20% chance for grenade drop from tanks
#define POWERUP_LIFETIME 15.0f  // Powerups last 15 seconds
#define POWERUP_PULSE_SPEED 3.0f  // Speed of pulsing animation

#endif // CONFIG_H