#include <string.h>
#include <math.h>

#include "vector.h"
#include "color.h"
#include "effect.h"
#include "enemy.h"
#include "gem.h"
#define SPECIAL_C
#include "special.h"

////////////////////////////////////////////////////////////////////////////////
// The functions which define the specials are below.  Because the 
// Color_Special_Map will be filled in with pointers, the definitions below 
// never need to be exported from this file.  For this reason they will be
// static.
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
// B1
////////////////////////////////////////////////////////////

////////////////////////////////////////
// R1
////////////////////////////////////////

// G1

static void special_damage(enemy_t *enemy, gem_t *gem)
{
  double damage;

  // The damage is r+g+b (in [0,64] per axis), scaled by enemy color (+armor or -weakness)
  //
  // The reason you see this instead of something based on color_special_area(), is
  // because this first lowly part of the color spectrum doesn't deserve such power.
  damage =  fmin(gem->color.s.x,64.0) * (1 - enemy->color.s.x/COLOR_MAX_COLOR);
  damage += fmin(gem->color.s.y,64.0) * (1 - enemy->color.s.y/COLOR_MAX_COLOR);
  damage += fmin(gem->color.s.z,64.0) * (1 - enemy->color.s.z/COLOR_MAX_COLOR);

  // Apply the damage
  enemy->health -= damage;
}

// G2

static void special_poison(enemy_t *enemy, gem_t *gem)
{
  effect_t effect;

  // Build the poison effect
  effect.end    = 0;                    // !!av: Need to add time so this junk can work
  effect.apply  = (effect_func_t)effect_poison_apply;
  effect.remove = (effect_func_t)effect_poison_remove;
  effect.tick   = (effect_func_t)effect_poison_tick;
  memcpy(&effect.color,&gem->color,sizeof(vector3_t)); 

  // Apply the effect to the enemy
  enemy_apply_effect(enemy,&effect);
}

// G3

////////////////////////////////////////
// R2
////////////////////////////////////////

// G1

static void special_fire(enemy_t *enemy, gem_t *gem)
{
  double damage;

  // Fire damage, takes into consideration the red component of
  // the enemy.  Damage scales based on the area of the special: (0,128]
  damage  = color_special_area(&gem->color,2,1,1) / COLOR_MAX_SPECIAL_AREA;
  damage *= (1.0 - enemy->color.s.x/COLOR_MAX_COLOR);
  damage *= 128.0;

  // Apply the damage
  enemy->health -= damage;
}

// G2
// G3

////////////////////////////////////////
// R3
////////////////////////////////////////

// G1

static void special_burn(enemy_t *enemy, gem_t *gem)
{
  effect_t effect;

  // Build the chill effect
  effect.end    = 0;                    // !!av: Need to add time so this junk can work
  effect.apply  = (effect_func_t)effect_burn_apply;
  effect.remove = (effect_func_t)effect_burn_remove;
  effect.tick   = (effect_func_t)effect_burn_tick;
  memcpy(&effect.color,&gem->color,sizeof(vector3_t)); 

  // Apply the effect to the enemy
  enemy_apply_effect(enemy,&effect);
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

static void special_chill(enemy_t *enemy, gem_t *gem)
{
  effect_t effect;

  // Build the chill effect
  effect.end    = 0;                    // !!av: Need to add time so this junk can work
  effect.apply  = (effect_func_t)effect_chill_apply;
  effect.remove = (effect_func_t)effect_chill_remove;
  effect.tick   = ((void*)0);
  memcpy(&effect.color,&gem->color,sizeof(vector3_t)); 

  // Apply the effect to the enemy
  enemy_apply_effect(enemy,&effect);
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

static void special_frozen(enemy_t *enemy, gem_t *gem)
{
  effect_t effect;

  // Build the freeze effect
  effect.end    = 0;                    // !!av: Need to add time so this junk can work
  effect.apply  = (effect_func_t)effect_frozen_apply;
  effect.remove = (effect_func_t)effect_frozen_remove;
  effect.tick   = ((void*)0);
  memcpy(&effect.color,&gem->color,sizeof(vector3_t)); 

  // Apply the effect to the enemy
  enemy_apply_effect(enemy,&effect);
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

////////////////////////////////////////////////////////////////////////////////
// Color_Special_Map: I^3 in [1,4] -> function pointer applying appricate effect
////////////////////////////////////////////////////////////////////////////////

special_t Color_Special_Map[4][4][4] = 
  {            // g1                   // g2                   // g3                   // g4
    { { S111,S112,S113,S114 },{ S121,S122,S123,S124 },{ S131,S132,S133,S134 },{ S141,S142,S143,S144 } }, // r1
    { { S211,S212,S213,S214 },{ S221,S222,S223,S224 },{ S231,S232,S233,S234 },{ S241,S242,S243,S244 } }, // r2
    { { S311,S312,S313,S314 },{ S321,S322,S323,S324 },{ S331,S332,S333,S334 },{ S341,S342,S343,S344 } }, // r3
    { { S411,S412,S413,S414 },{ S421,S422,S423,S424 },{ S431,S432,S433,S434 },{ S441,S442,S443,S444 } }  // r4
  };    //b1 //b2 //b3 //b4     //b1 //b2 //b3 //b4     //b1 //b2 //b3 //b4     //b1 //b2 //b3 //b4

