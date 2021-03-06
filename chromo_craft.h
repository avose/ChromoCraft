#ifndef CHROMO_CRAFT_H
#define CHROMO_CRAFT_H

#include "types.h"
#include "path.h"
#include "random.h"
#include "enemy.h"
#include "player.h"
#include "io_bitmap.h"

#define GEM_CREATE_MANA_COST 100
#define GEM_MIX_MANA_COST    50

typedef struct str_state_t {
  u64b_t      time;         // Current time

  player_t    player;       // Current player

  enemy_t    *enemies;      // Array of enemies
  u32b_t      nenemies;     // Number of enemies
  u32b_t      senemies;     // Size of enemy array
  path_t     *path;         // Path the enemies follow
  u64b_t      wave;         // Enemy wave number

  rnd_t       random;       // Random number generator state

  io_bitmap_t terrain;      // Heightmap for the floor / terrain
} state_t;


#endif
