#ifndef TOWER_H
#define TOWER_H

#include "vector.h"
#include "gem.h"
#include "enemy.h"

typedef struct str_tower_t {
  gem_t     gem;
  vector3_t position;
  u64b_t    ftime;      // Time last fired
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
