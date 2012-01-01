#ifndef ENEMY_H
#define ENEMY_H

#include "random.h"
#include "vector.h"
#include "path.h"
#include "enemy_effect_types.h"

// These are the visual effects that should be applied to enemies
#define ENEMY_EFFECT_CHILLED (1ull<<0)
#define ENEMY_EFFECT_POISON  (1ull<<1)
#define ENEMY_EFFECT_BURN    (1ull<<2)
#define ENEMY_EFFECT_FROZEN  (1ull<<3)

////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////
#ifndef ENEMY_C
extern void     enemy_apply_effect(enemy_t *enemy, effect_t *effect);
extern enemy_t* enemy_get_nearest_enemy(vector3_t *position, enemy_t *enemies, const u32b_t nenemies);
extern double   enemy_get_enemy_xp(enemy_t *enemy);
extern void     enemy_new_enemy(enemy_t *enemy, enemy_t **enemies, u32b_t *nenemies, u32b_t *senemies);
extern void     enemy_new_wave(vector3_t *color, const double time, rnd_t *random, enemy_t **enemies, u32b_t *nenemies, u32b_t *senemies);
extern void     enemy_kill_enemy(u32b_t index, enemy_t *enemies, u32b_t *nenemies, u32b_t senemies);
#endif



#endif
