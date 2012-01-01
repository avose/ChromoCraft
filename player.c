#include <stdlib.h>
#include <string.h>

#include "types.h" 
#include "util.h"
#include "vector.h"
#include "tower.h"
#define PLAYER_C
#include "player.h"

void player_add_tower(player_t *player, vector3_t *position)
{
  // Make sure there is room
  if( player->ntowers+1 > player->stowers ) {
    if( !player->stowers ) {
      // Initial allocation with size 8
      player->stowers = 8;
      if( !(player->towers=malloc(player->stowers*sizeof(tower_t))) ) {
	Error("Could not allocate space for towers (%u) for player \"%s\"\n",
	      player->stowers*sizeof(tower_t),player->name);
      }
    } else {
      // Double the size of the tower array
      player->stowers *= 2;
      if( !(player->towers=realloc(player->towers,player->stowers*sizeof(tower_t))) ) {
	Error("Could not allocate space for more towers (%u) for player \"%s\"\n",
	      player->stowers*sizeof(tower_t),player->name);
      }
    }
  }

  // Setup the new tower
  memset(&player->towers[player->ntowers],0,sizeof(tower_t));
  memcpy(&player->towers[player->ntowers].position,position,sizeof(vector3_t));
  player->ntowers++;
}


