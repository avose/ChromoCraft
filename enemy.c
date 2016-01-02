#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "random.h"
#include "vector.h"
#include "color.h"
#include "effect.h"
#define ENEMY_C
#include "enemy.h"

////////////////////////////////////////////////////////////
// Applies and effect to an enemy
////////////////////////////////////////////////////////////
void enemy_apply_effect(enemy_t *enemy, effect_t *effect)
{
  // Make sure there is room
  if( enemy->neffects+1 > enemy->seffects ) {
    if( !enemy->seffects ) {
      // Initial allocation with size 8
      enemy->seffects = 8;
      if( !(enemy->effects=malloc(enemy->seffects*sizeof(effect_t))) ) {
	Error("Could not allocate space for effects (%u) for enemy\n",
	      enemy->seffects*sizeof(effect_t));
      }
    } else {
      // Double the size of the effect array
      enemy->seffects *= 2;
      if( !(enemy->effects=realloc(enemy->effects,enemy->seffects*sizeof(effect_t))) ) {
	Error("Could not allocate space for more effects (%u) for enemy\n",
	      enemy->seffects*sizeof(effect_t));
      }
    }
  }

  // Setup the new effect
  memcpy(&enemy->effects[enemy->neffects],effect,sizeof(effect_t));
  enemy->neffects++;
}

////////////////////////////////////////////////////////////
// Returns the enemy nearest position
////////////////////////////////////////////////////////////
enemy_t* enemy_get_nearest_enemy(vector3_t *position, enemy_t *enemies, const u32b_t nenemies)
{
  vector3_t  b;
  double     d, mind=((double)(1<<20));
  enemy_t   *e,*mine=NULL;

  // Just look at all enemies and find the closest...
  for(e=enemies; e<(enemies+nenemies); e++) {
    vector3_sub_vector(position, &(e->position), &b);
    d = vector3_length(&b);
    if( d < mind ) {
      mind = d;
      mine = e;
    }
  }

  return mine;
}

////////////////////////////////////////////////////////////
// Returns the XP value of the enemy
////////////////////////////////////////////////////////////
double enemy_get_enemy_xp(enemy_t *enemy)
{
  return enemy->base_health * (color_area(&enemy->color)/COLOR_MAX_AREA) * 100;
}

////////////////////////////////////////////////////////////
// Creates a new enemy modeled after the passed enemy in the passed nemey arrays
////////////////////////////////////////////////////////////
void enemy_new_enemy(enemy_t *enemy, enemy_t **enemies, u32b_t *nenemies, u32b_t *senemies)
{
  // Make sure there is room
  if( *nenemies+1 > *senemies ) {
    if( !*senemies ) {
      // Initial allocation with size 8
      *senemies = 8;
      if( !(*enemies=malloc(*senemies*sizeof(enemy_t))) ) {
	Error("Could not allocate space for enemies (%u) for enemy\n",
	      *senemies*sizeof(enemy_t));
      }
    } else {
      // Double the size of the enemy array
      *senemies *= 2;
      if( !(*enemies=realloc(*enemies,*senemies*sizeof(enemy_t))) ) {
	Error("Could not allocate space for more enemies (%u) for enemy\n",
	      *senemies*sizeof(enemy_t));
      }
    }
  }

  // Setup the new enemy
  memcpy(&((*enemies)[*nenemies]),enemy,sizeof(enemy_t));
  (*nenemies)++;

  // !!av: call paul so he can know we created an enemy
}

////////////////////////////////////////////////////////////
// Creates a new enemy modeled after the passed enemy in the passed nemey arrays
////////////////////////////////////////////////////////////
void enemy_kill_enemy(u32b_t index, enemy_t *enemies, u32b_t *nenemies, u32b_t senemies)
{
  // Just in case ...
  if( (!*nenemies) || (index > *nenemies) || (*nenemies > senemies) ) {
    Warn("This should never happen.  In enemy_kill_enemy().\n");
    return;
  }

  // Copy last over current and decrement count to remove
  (*nenemies)--;
  memcpy(&(enemies[index]),&(enemies[*nenemies]),sizeof(enemy_t));

  // !!av: call paul so he can know we killed an enemy
}

////////////////////////////////////////////////////////////
// Spawns an entire new wave of enemies
//
// color must conform to: <r != {0,1}, g != {0,1}, b != {0,1}>
////////////////////////////////////////////////////////////
void enemy_new_wave(vector3_t *color, const double time, rnd_t *random, enemy_t **enemies, u32b_t *nenemies, u32b_t *senemies)
{
  static dist_t *distribution = ((void*)0);
  enemy_t        enemy; 
  double         sum;
  double         wave_start_time;
  u32b_t         number_to_spawn;
  u32b_t         enemy_spacing;
  u32b_t         choice,oot;
  u32b_t         i;

  // Trim the color.
  if( color->s.x > 255 ) {
    color->s.x = 255;
  }
  if( color->s.y > 255 ) {
    color->s.y = 255;
  }
  if( color->s.z > 255 ) {
    color->s.z = 255;
  }

  // Get a new random distribution if needed
  if( !distribution ) {
    distribution = random_allocdist(3);
  }

  // Fill in the probabilities
  sum =  distribution->p[0] = color->a[0]; // Red
  sum += distribution->p[1] = color->a[1]; // Green
  sum += distribution->p[2] = color->a[2]; // Blue

  // Handle the "black" case: (0,0,0)
  if( !sum ) {
    distribution->p[0] = 1.0; // Red
    distribution->p[1] = 1.0; // Green
    distribution->p[2] = 1.0; // Blue
    sum = 3;
  }

  // Initialize the random color choice distribution
  random_initdist(distribution,sum);

  // Let's spawn a number of enemies from 10-256 ...
  number_to_spawn = ((u32b_t)(color_area(color)/COLOR_MAX_AREA*255)) + 10;   
  oot = 1;
  if( random_U01(random) < .5 ) {
    oot = 2;
  }
  number_to_spawn /= oot;
  if( number_to_spawn < 1 ) {
    number_to_spawn = 1;
  }

  // Let's space them apart a random amount from [10,100] ticks...
  enemy_spacing = random_rnd(random,100-10) + 15;

  wave_start_time = time + enemy_spacing;

  // Add the appropriate amount of enemies
  for(i=0; i<number_to_spawn; i++) {
    // Clear the structure (I count on color being 0 ...)
    memset(&enemy,0,sizeof(enemy_t));

    // Either one or two colors.
    enemy.color.s.x = enemy.color.s.y = enemy.color.s.z = 1;
    if( oot == 2 ) {
      if( random_U01(random) < .5 ) {
	// Two colors chosed based on color distribution
	do {
	  choice = random_drand(random,distribution);
	} while( enemy.color.a[choice] == color->a[choice] );
	// The next dimension is "weaker".
	enemy.color.a[choice] = color->a[choice]*0.25;
      }
    }

    // One color based on color distribution
    do {
      choice = random_drand(random,distribution);
    } while( enemy.color.a[choice] == color->a[choice] );
    enemy.color.a[choice] = color->a[choice];
    
    // The health of the enemy will be the area of the color
    enemy.health = enemy.base_health = color_area(&enemy.color);

    // The speed of the enemy will be [10,100] based on color area
    enemy.speed = (color_area(&enemy.color)/COLOR_MAX_AREA*50.0) + 5.0;
    if( random_U01(random) < .5 ) {
      enemy.speed += 5.0;
    }

    // Set the start time for correct wave spacing
    enemy.start_time = wave_start_time + enemy_spacing*i;

    // Spawn the enemy
    enemy_new_enemy(&enemy, enemies, nenemies, senemies);
  }

}
