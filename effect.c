
#include "enemy.h"
#include "color.h"
#define EFFECT_C
#include "effect.h"

////////////////////////////////////////////////////////////
// B1
////////////////////////////////////////////////////////////

////////////////////////////////////////
// R1
////////////////////////////////////////

// G1
// G2

void effect_poison_apply(enemy_t *enemy, effect_t *effect)
{
  // Apply the green glow visual effect
  enemy->fxflags |= ENEMY_EFFECT_POISON;
}

void effect_poison_remove(enemy_t *enemy, effect_t *effect)
{
  // Remove the green glow
  enemy->fxflags &= ~ENEMY_EFFECT_POISON;
}

void effect_poison_tick(enemy_t *enemy, effect_t *effect)
{
  double damage;

  // Poison damage, takes into consideration the green component of
  // the enemy.  Damage scales based on the area of the special: (0,64]
  damage  = color_special_area(&effect->color,1,2,1) / COLOR_MAX_SPECIAL_AREA;
  damage *= (1.0 - enemy->color.s.y/COLOR_MAX_COLOR);
  damage *= 64.0;

  // Apply the damage
  enemy->health -= damage;
}

// G3

////////////////////////////////////////
// R2
////////////////////////////////////////

// G1
// G2
// G3

////////////////////////////////////////
// R3
////////////////////////////////////////

// G1

void effect_burn_apply(enemy_t *enemy, effect_t *effect)
{
  // Apply the green glow visual effect
  enemy->fxflags |= ENEMY_EFFECT_BURN;
}

void effect_burn_remove(enemy_t *enemy, effect_t *effect)
{
  // Remove the green glow
  enemy->fxflags &= ~ENEMY_EFFECT_BURN;
}

void effect_burn_tick(enemy_t *enemy, effect_t *effect)
{
  double damage;

  // Burn damage, takes into consideration the red component of
  // the enemy.  Damage scales based on the area of the special: (0,96]
  damage  = color_special_area(&effect->color,2,1,1) / COLOR_MAX_SPECIAL_AREA;
  damage *= (1.0 - enemy->color.s.x/COLOR_MAX_COLOR);
  damage *= 96.0;

  // Apply the damage
  enemy->health -= damage;
}

// G2
// G3

////////////////////////////////////////////////////////////
// B2
////////////////////////////////////////////////////////////

////////////////////////////////////////
// R1
////////////////////////////////////////

// G1
// G2
// G3

////////////////////////////////////////
// R2
////////////////////////////////////////

// G1
// G2
// G3

////////////////////////////////////////
// R3
////////////////////////////////////////

// G1
// G2
// G3

////////////////////////////////////////////////////////////
// B3
////////////////////////////////////////////////////////////

////////////////////////////////////////
// R1
////////////////////////////////////////

// G1

void effect_chill_apply(enemy_t *enemy, effect_t *effect)
{
  double slow_amount;
  
  // Calculate slow amount (maximum slow is 90% of regular speed)
  slow_amount = color_special_area(&effect->color,1,1,3) / COLOR_MAX_SPECIAL_AREA;
  slow_amount *= .90;
  slow_amount = 1.0 - slow_amount;

  // Slow enemy
  enemy->speed *= slow_amount;

  // Apply the blue glow visual effect
  enemy->fxflags |= ENEMY_EFFECT_CHILLED;
}

void effect_chill_remove(enemy_t *enemy, effect_t *effect)
{
   double slow_amount;
  
  // Calculate slow amount (maximum slow is 90% of regular speed)
  slow_amount = color_special_area(&effect->color,1,1,3) / COLOR_MAX_SPECIAL_AREA;
  slow_amount *= .90;
  slow_amount = 1.0 - slow_amount;

  // Remove the slow effect
  enemy->speed /= slow_amount;

  // Remove the blue glow
  enemy->fxflags &= ~ENEMY_EFFECT_CHILLED;
}

// G2
// G3

////////////////////////////////////////
// R2
////////////////////////////////////////

// G1
// G2
// G3

////////////////////////////////////////
// R3
////////////////////////////////////////

// G1
// G2
// G3

////////////////////////////////////////////////////////////
// B4
////////////////////////////////////////////////////////////

////////////////////////////////////////
// R1
////////////////////////////////////////

// G1

void effect_frozen_apply(enemy_t *enemy, effect_t *effect)
{
  // Apply the frozen effect
  enemy->fxflags |= ENEMY_EFFECT_FROZEN;
}

void effect_frozen_remove(enemy_t *enemy, effect_t *effect)
{
  // Remove the frozen effect
  enemy->fxflags &= ~ENEMY_EFFECT_FROZEN;
}

// G2
// G3

////////////////////////////////////////
// R2
////////////////////////////////////////

// G1
// G2
// G3

////////////////////////////////////////
// R3
////////////////////////////////////////

// G1
// G2
// G3
