#ifndef TOWER_H
#define TOWER_H

#include "vector.h"
#include "gem.h"
#include "enemy.h"

#define TOWER_FLAG_NONE     0
#define TOWER_FLAG_SELECTED 1

typedef struct str_tower_t {
  gem_t     gem;
  vector3_t position;
  vector3_t scr_pos;
  u64b_t    ftime;      // Time last fired
  u64b_t    flags;
} tower_t;

////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////
#ifndef TOWER_C
extern void tower_fire_towers(tower_t *towers, const u32b_t ntowers, enemy_t *enemies, const u32b_t nenemies, const u64b_t time);
extern void tower_install_gem(tower_t *tower, gem_t *gem);
extern void tower_remove_gem (tower_t *tower);
#endif


#endif
