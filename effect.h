#ifndef EFFECT_H
#define EFFECT_H

#include "enemy_effect_types.h"


////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////
#ifndef EFFECT_C
extern void effect_poison_apply (enemy_t *enemy, effect_t *effect);
extern void effect_poison_remove(enemy_t *enemy, effect_t *effect);
extern void effect_poison_tick  (enemy_t *enemy, effect_t *effect);

extern void effect_chill_apply (enemy_t *enemy, effect_t *effect);
extern void effect_chill_remove(enemy_t *enemy, effect_t *effect);

extern void effect_burn_apply(enemy_t *enemy, effect_t *effect);
extern void effect_burn_remove(enemy_t *enemy, effect_t *effect);
extern void effect_burn_tick(enemy_t *enemy, effect_t *effect);

extern void effect_frozen_apply(enemy_t *enemy, effect_t *effect);
extern void effect_frozen_remove(enemy_t *enemy, effect_t *effect);
#endif

#endif
