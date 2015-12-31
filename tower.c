#include <string.h>

#include "types.h"
#include "gem.h"
#include "color.h"
#include "special.h"
#include "enemy.h"
#include "gui_game_event.h"
#define TOWER_C
#include "tower.h"


void tower_fire_towers(tower_t *towers, const u32b_t ntowers, enemy_t *enemies, const u32b_t nenemies, const u64b_t time)
{
  u32b_t     i,r,g,b;
  enemy_t   *enemy;
  special_t  special;
  vector3_t  v;
  double     h;

  // Fire all towers in towers list
  for(i=0; i<ntowers; i++) {

    // Only fire if our rate says we can
    if( (time-towers[i].ftime) > (100 - towers[i].gem.rate) ) {

      // At nearest enemy
      enemy = enemy_get_nearest_enemy(&towers[i].position, enemies, nenemies);
      if( enemy ) {

	// Only shoot enemies in range
	if( vector3_length(vector3_sub_vector(&towers[i].position, &enemy->position, &v)) < towers[i].gem.range ) {
	  // Update fire time
	  towers[i].ftime = time;	
	  // Record start health
	  h = enemy->health;
	  // Hit with all specials the tower's gem is capable of
	  for(r=0; r<=towers[i].gem.color.s.x; r+=64) {
	    for(g=0; g<=towers[i].gem.color.s.y; g+=64) {
	      for(b=0; b<=towers[i].gem.color.s.z; b+=64) {
		// Hit the enemy with the special
		special = Color_Special_Map[r/64][g/64][b/64];
		if( special ) {
		  special(enemy,&towers[i].gem);
		}
	      }
	    }
	  }
	  // Register this event with the GUI
	  gui_game_event_fire(time, &towers[i], enemy, h-enemy->health);
	}
      }

    }
  }

}

void tower_install_gem(tower_t *tower, gem_t *gem)
{
  memcpy(&tower->gem,gem,sizeof(gem_t));
  tower->gem.range = (color_area(&(gem->color))/COLOR_MAX_AREA) * 80.0 + 20.0;
  tower->gem.rate  = (color_area(&(gem->color))/COLOR_MAX_AREA) * 80.0 + 20.0;
}

void tower_remove_gem(tower_t *tower)
{
  memset(&tower->gem,0,sizeof(gem_t));
}
