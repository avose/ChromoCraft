#ifndef ENEMY_EFFECT_TYPES_H
#define ENEMY_EFFECT_TYPES_H

#include "types.h" 
#include "vector.h"
#include "path.h" 

// I need these types defined up high so they will be
// available to everything lower in the file.
typedef struct str_effect_t effect_t;
typedef struct str_enemy_t  enemy_t;


////////////////////////////////////////////////////////////
// Effect
////////////////////////////////////////////////////////////

// All three effect member functions have the same format and one type
typedef void (*effect_func_t)(enemy_t*,effect_t*);

// This is the effect structure
struct str_effect_t {
  double        start;  // Time when the effect started
  double        end;    // Time when the effect should be removed

  effect_func_t apply;  // Function called to apply the effect to an enemy
  effect_func_t remove; // Function called to remove the effect from an enemy
  effect_func_t tick;   // Called each 'tick' to do something to the enemy

  vector3_t     color;  // Color that caused this effect
};


////////////////////////////////////////////////////////////
// Enemy
////////////////////////////////////////////////////////////

struct str_enemy_t {

  // Basics
  double               health;       // HP of the enemy
  double               base_health;  // Base HP of the enemy
  double               speed;        // Movement rate
  vector3_t            position;     // Position of the enemy
  vector3_t            scr_pos;      // Position (2D on screen)
  vector3_t            color;        // Color of the enemy
  path_t               *path;        // Current path node

  // Effects
  effect_t            *effects;      // Array of effects currently on this enemy
  u32b_t               neffects;     // Number of effects on this enemy
  u32b_t               seffects;     // Size of the effects array

  // Effect flags
  u64b_t               fxflags;      // Bitflag of effects on this enemy

  double               start_time;   // Time the enemy should be created
};

#endif
