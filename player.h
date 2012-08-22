#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"
#include "tower.h"
#include "bag.h"

typedef struct {
  char     name[128];    // Player name

  double   base_mana;    // Player's base (max) mana pool
  double   mana;         // Player's mana pool

  tower_t *towers;       // Array of player's towers
  u32b_t   ntowers;      // Number of towers
  u32b_t   stowers;      // Size of tower array

  bag_t    bag;          // Inventory

  double   score;        // Player score (points)

  gem_t    mouse_gem;    // Current gem "held" by the mouse
} player_t;

////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////
#ifndef PLAYER_C
extern void player_add_tower(player_t *player, vector3_t *position);
#endif

#endif
