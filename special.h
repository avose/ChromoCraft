#ifndef SPECIAL_H
#define SPECIAL_H

#include "enemy.h"
#include "gem.h"


typedef void (*special_t)(enemy_t*,gem_t*);


////////////////////////////////////////////////////////////
// For Special.c only
////////////////////////////////////////////////////////////
#ifdef SPECIAL_C
#define S111 special_damage
#define S112 ((void*)(0))
#define S113 special_chill
#define S114 ((void*)(0))
#define S121 special_poison
#define S122 ((void*)(0))
#define S123 ((void*)(0))
#define S124 ((void*)(0))
#define S131 ((void*)(0))
#define S132 ((void*)(0))
#define S133 ((void*)(0))
#define S134 ((void*)(0))
#define S141 ((void*)(0))
#define S142 ((void*)(0))
#define S143 ((void*)(0))
#define S144 ((void*)(0))
#define S211 special_fire
#define S212 ((void*)(0))
#define S213 ((void*)(0))
#define S214 ((void*)(0))
#define S221 ((void*)(0))
#define S222 ((void*)(0))
#define S223 ((void*)(0))
#define S224 ((void*)(0))
#define S231 ((void*)(0))
#define S232 ((void*)(0))
#define S233 ((void*)(0))
#define S234 ((void*)(0))
#define S241 ((void*)(0))
#define S242 ((void*)(0))
#define S243 ((void*)(0))
#define S244 ((void*)(0))
#define S311 special_burn
#define S312 ((void*)(0))
#define S313 ((void*)(0))
#define S314 ((void*)(0))
#define S321 ((void*)(0))
#define S322 ((void*)(0))
#define S323 ((void*)(0))
#define S324 ((void*)(0))
#define S331 ((void*)(0))
#define S332 ((void*)(0))
#define S333 ((void*)(0))
#define S334 ((void*)(0))
#define S341 ((void*)(0))
#define S342 ((void*)(0))
#define S343 ((void*)(0))
#define S344 ((void*)(0))
#define S411 special_frozen
#define S412 ((void*)(0))
#define S413 ((void*)(0))
#define S414 ((void*)(0))
#define S421 ((void*)(0))
#define S422 ((void*)(0))
#define S423 ((void*)(0))
#define S424 ((void*)(0))
#define S431 ((void*)(0))
#define S432 ((void*)(0))
#define S433 ((void*)(0))
#define S434 ((void*)(0))
#define S441 ((void*)(0))
#define S442 ((void*)(0))
#define S443 ((void*)(0))
#define S444 ((void*)(0))
#endif

////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////
#ifndef SPECIAL_C
extern special_t Color_Special_Map[4][4][4];
#endif


#endif
