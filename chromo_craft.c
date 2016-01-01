#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <AL/al.h>
#include <AL/alut.h>

#include "types.h"
#include "util.h"
#include "random.h"
#include "vector.h"
#include "color.h"
#include "effect.h"
#include "gem.h"
#include "enemy.h"
#include "path.h"
#include "special.h"
#include "tower.h"
#include "player.h"
#define CHROMO_CRAFT_C
#include "chromo_craft.h"
#include "gui.h"
#include "game_event.h"
#include "gui_game_event.h"

static state_t *State;

#define TERRAIN_SIZE  64
#define TERRAIN_SCALE 4

static path_t *load_path_file(char *fn)
{
  io_bitmap_t  bm;
  path_t      *p;
  vector3_t    node[256];
  int          i, j;
  u8b_t        n;

  io_bitmap_load(fn, &bm);

  // Load image of node positions
  if( bm.w != bm.h || bm.w != TERRAIN_SIZE ) {
    Error("Path bitmap must be %dx%d in size.\n", TERRAIN_SIZE, TERRAIN_SIZE);
  }

  // Initialize positions to bogus values
  for( i = 0; i < 256; ++i ) {
    node[i].s.x = -1.0f;
    node[i].s.y = -1.0f;
    node[i].s.z = 0.0f;
  }

  // Scan image and store positions
  for( j = 0; j < bm.h; ++j ) {
    for( i = 0; i < bm.w; ++i ) {
      n = bm.d[(j*TERRAIN_SIZE + i) * 3];
      node[n].s.x = i * TERRAIN_SCALE;
      node[n].s.y = j * TERRAIN_SCALE;
    }
  }

  // Now build linked list out of valid positions (Skip v=0)
  p = NULL;
  for( i = 1; i < 256; ++i ) {
    // Add the node if the position is valid
    if (node[i].s.x > -1.0f && node[i].s.y > -1.0f) {
      // FIXME: It'd be nice to create a path with no nodes,
      // versus this condition
      if (p) {
	path_new_node(p, node+i);
      } else {
	p = path_new_path(node+i);
      }
    }
  }
  
  io_bitmap_free(&bm);

  return p;
}


static void load_towers_file(char *fn)
{
  vector3_t    position;
  io_bitmap_t  bm;
  int          i, j;
  u8b_t        n;

  io_bitmap_load(fn, &bm);

  // Load image of node positions
  if( bm.w != bm.h || bm.w != TERRAIN_SIZE ) {
    Error("Towers bitmap must be %dx%d in size.\n", TERRAIN_SIZE, TERRAIN_SIZE);
  }

  // Scan image and store positions
  for( j = 0; j < bm.h; ++j ) {
    for( i = 0; i < bm.w; ++i ) {
      n = bm.d[(j*TERRAIN_SIZE + i) * 3];
      if( n != 0 ) {
	// Add tower.
	position.s.x = i * TERRAIN_SCALE;
	position.s.y = j * TERRAIN_SCALE;
	position.s.z = 0.0;
	player_add_tower(&State->player, &position);
	tower_remove_gem(&State->player.towers[State->player.ntowers-1]);
      }
    }
  }
  
  io_bitmap_free(&bm);
}


////////////////////////////////////////////////////////////
// Sets up the global state to get ready for play
////////////////////////////////////////////////////////////
static void init_state()
{
  struct timeval tv;

  
  // Allocate the main state structure
  if( !(State=malloc(sizeof(state_t))) ) {
    Error("Could not allocate state structure (%u)\n",sizeof(state_t));
  }

  // Go ahead and clear the memory to start with.
  memset(State,0,sizeof(state_t));

  // Initialize the random number generator
  if( gettimeofday(&tv, NULL) == -1 ) {
    Warn("gettimeofday() failed; seeding random generator with 7 instead.\n");
    random_initrand(&State->random,7);
  } else {
    random_initrand(&State->random,tv.tv_usec);
  }

  // Set the current time
  State->time = 1;

  // Load the terrain heightmap
  io_bitmap_load("data/bmp/hm.bmp", &(State->terrain));
  if( (State->terrain.w != State->terrain.h) || (State->terrain.w != TERRAIN_SIZE) ) {
    Error("Terrain heightmap bitmap must be %dx%d in size.\n", TERRAIN_SIZE, TERRAIN_SIZE);
  }

  // Set the player up as level 1
  State->player.mana      =   50;
  State->player.base_mana =  100;
}

////////////////////////////////////////////////////////////
// Debug prints
////////////////////////////////////////////////////////////

// Some debugging helpers:
#if 0
static void print_towers()
{
  u32b_t i;

  // print them out
  printf("Towers: %u\n",State->player.ntowers);
  for(i=0; i<State->player.ntowers; i++) {
    printf( "\tgem: ");
    if( color_is_black(&State->player.towers[i].gem.color) ) {
      printf("none");
    } else {
      printf( "<%.0lf,%.0lf,%.0lf>\trange: %.1lf\tfire rate: %.1lf\t",
	      State->player.towers[i].gem.color.s.x,
	      State->player.towers[i].gem.color.s.y,
	      State->player.towers[i].gem.color.s.z,
	      State->player.towers[i].gem.range,
	      State->player.towers[i].gem.rate
	      );
    }
    printf("\tposition: <%.0lf,%.0lf,%.0lf>\n",
	   State->player.towers[i].position.s.x,
	   State->player.towers[i].position.s.y,
	   State->player.towers[i].position.s.z
	   );
  }
  printf("\n");

}

static void print_enemies() 
{
  u32b_t i;
  
  printf("Enemies: %u\n",State->nenemies);
  for(i=0; i<State->nenemies; i++) {
    printf( "\tHealth: %.1lf \tSpeed: %.1lf\tColor: <%.0lf,%.0lf,%.0lf>  \tStart Time: %.1lf",
	    State->enemies[i].health,
	    State->enemies[i].speed,
	    State->enemies[i].color.s.x,
	    State->enemies[i].color.s.y,
	    State->enemies[i].color.s.z,
	    State->enemies[i].start_time
	    );
    printf( "\tFlags: %llu\n",
	    State->enemies[i].fxflags
	    );
  }
  printf("\n");
}
  
static void print_player()
{
  printf( "Player name: \"%s\"\tmana: %.1lf\tbase mana: %.1lf\n\n",
	  State->player.name,
	  State->player.mana,	
	  State->player.base_mana
	  );
}
#endif

////////////////////////////////////////////////////////////
// Random initial setup
////////////////////////////////////////////////////////////

static void add_some_enemies(u32b_t level)
{
  vector3_t  color;
  u32b_t     i,n;

  // Get a random color for this wave based on the level
  color.s.x = random_rnd(&State->random,level-1)+2;
  color.s.y = random_rnd(&State->random,level-1)+2;
  color.s.z = random_rnd(&State->random,level-1)+2;
  
  // Add a wave
  n = State->nenemies;
  enemy_new_wave(&color, State->time, &State->random, &State->enemies, &State->nenemies, &State->senemies);

  // Update position of each enemy
  for(i=n; i<State->nenemies; i++) {
    vector3_copy(&State->path->position, &State->enemies[i].position);
    State->enemies[i].path = State->path->next;
  }
}

static void add_some_gems()
{
  gem_t g;

  g.color.a[0] = 128;
  g.color.a[1] = 0;
  g.color.a[2] = 0;

  bag_add_gem(&State->player.bag,&g);
  bag_add_gem(&State->player.bag,&g);
  bag_add_gem(&State->player.bag,&g);

  g.color.a[0] = 0;
  g.color.a[1] = 128;
  g.color.a[2] = 0;

  bag_add_gem(&State->player.bag,&g);
  bag_add_gem(&State->player.bag,&g);
  bag_add_gem(&State->player.bag,&g);
}

////////////////////////////////////////////////////////////
// Main game loop and helpers
////////////////////////////////////////////////////////////

static void update_enemy_effects()
{

}

#define BASE_SPEED 0.015

static void update_enemies_path()
{
  vector3_t v;
  u32b_t    i;

  // Update position of each enemy that has "started"
  for(i=0; i<State->nenemies; ) {
    // Move enemy towards next path point
    if( (State->enemies[i].start_time < State->time) && (State->enemies[i].path != State->path) ) {
      // Build vector poiting toward waypoint
      vector3_sub_vector(&State->enemies[i].path->position, &State->enemies[i].position, &v);
      // If waypoint is in reach, simply jump there and assign next node
      if( vector3_length(&v) < State->enemies[i].speed*BASE_SPEED ) {
	vector3_copy(&State->enemies[i].path->position, &State->enemies[i].position);
	State->enemies[i].path = State->enemies[i].path->next;
	// Move on to next entry
	i++;
      } else {
	// Just move towards it
	vector3_normalize(&v, &v);
	vector3_mult_scalar(&v,&v,State->enemies[i].speed*BASE_SPEED);
	vector3_add_vector(&State->enemies[i].position, &v, &State->enemies[i].position);
	// Move on to next entry
	i++;  
      }
    } else if( State->enemies[i].path == State->path ) {
      // Enemy reached last path node!
      // Remove points from mana.
      State->player.mana -= State->enemies[i].health*0.1;
      // Notify GUI of the death.
      gui_game_event_kill(State->time,&State->enemies[i]);
      // Kill it (will move last entry into current); retest current.
      enemy_kill_enemy(i, State->enemies, &State->nenemies, State->senemies);
    } else {
      // Move on to next entry.
      i++;
    }
  }

}

#undef BASE_SPEED

static void start_wave()
{
  // Start a new wave.
  add_some_enemies(25+(State->wave*10));
  State->wave++;
}

static void update_enemies()
{
  static u32b_t ticks=0;

  // Start a new wave if needed.
  ticks++;
  if( ticks >= 10000 ) {
    ticks = 0;
    start_wave();
  }

  // Update the enemies.
  update_enemy_effects();
  update_enemies_path();
}

static void update_towers()
{
  u32b_t i;
  
  // Attack
  tower_fire_towers(State->player.towers, State->player.ntowers, State->enemies, State->nenemies, State->time);

  // Officially kill all those with negative health
  for(i=0; i<State->nenemies; ) {
    if( State->enemies[i].health < 0 ) {
      // Add points to xp
      State->player.xp   += State->enemies[i].base_health         + State->wave;
      State->player.mana += ((State->enemies[i].base_health)*0.1) + State->wave;
      // Notify GUI of the death
      gui_game_event_kill(State->time,&State->enemies[i]);
      // Kill it (will move last entry into current); retest current
      enemy_kill_enemy(i, State->enemies, &State->nenemies, State->senemies);
    } else {
      // Move on to next entry
      i++;
    }
  }
}

static void update_player()
{
  State->player.base_mana = 100 + ((((int)State->player.xp)/1000)*100);
  State->player.mana += 0.001;
  if( State->player.mana > State->player.base_mana ) {
    State->player.mana = State->player.base_mana;
  }
}

static void process_events()
{
  game_eventq_t *q;
  gem_t          gem;
  int            i;

  for(q=game_event_get(NULL); q; q=game_event_get(q)) {
    // Find event type
    switch(q->type) {
    case GAME_EVENT_TOWER_INSTALL_GEM:
      for(i=0; i<State->player.ntowers; i++) {
	// Apply event to the tower at the specified location.
	if( vector3_compare(&(State->player.towers[i].position), &(q->tower_install_gem.tpos)) ) {
	  tower_install_gem(&(State->player.towers[i]), &(State->player.bag.items[q->tower_install_gem.ndx].gem));
	  bag_remove_item(&(State->player.bag),q->tower_install_gem.ndx);
	  break;
	}
      }
      // Remove the event from the queue.
      game_event_remove(q);
      break;
    case GAME_EVENT_TOWER_REMOVE_GEM:
      for(i=0; i<State->player.ntowers; i++) {
	// Apply event to the tower at the specified location.
	if( vector3_compare(&(State->player.towers[i].position), &(q->tower_install_gem.tpos)) ) {
	  bag_add_gem(&(State->player.bag), &(State->player.towers[i].gem));
	  tower_remove_gem(&(State->player.towers[i]));
	  break;
	}
      }
      // Remove the event from the queue.
      game_event_remove(q);
      break;
    case GAME_EVENT_TOWER_SWAP_GEM:
      for(i=0; i<State->player.ntowers; i++) {
	// Apply event to the tower at the specified location.
	if( vector3_compare(&(State->player.towers[i].position), &(q->tower_install_gem.tpos)) ) {
	  bag_add_gem(&(State->player.bag), &(State->player.towers[i].gem));
	  tower_remove_gem(&(State->player.towers[i]));
	  tower_install_gem(&(State->player.towers[i]), &(State->player.bag.items[q->tower_install_gem.ndx].gem));
	  bag_remove_item(&(State->player.bag),q->tower_install_gem.ndx);
	  break;
	}
      }
      // Remove the event from the queue.
      game_event_remove(q);
      break;
    case GAME_EVENT_CREATE_GEM:
      // Create new gem in player's bag.
      if( State->player.mana >= GEM_CREATE_MANA_COST ) {
	bag_add_gem(&(State->player.bag), &(q->create_gem.gem));
	State->player.mana -= GEM_CREATE_MANA_COST;
      }
      // Remove the event from the queue.
      game_event_remove(q);
      break;
    case GAME_EVENT_MIX_GEMS:
      // Mix gems in player's bag.
      if( State->player.mana >= GEM_MIX_MANA_COST ) {
	// Make new "mixed" gem.
	gem_mix_gems(&(State->player.bag.items[q->mix_gems.ndx1].gem),
		     &(State->player.bag.items[q->mix_gems.ndx2].gem),
		     &gem);
	// Add new "mixed" gem.
	bag_add_gem(&(State->player.bag), &gem);
	// Remove the "parent" gems.
	bag_remove_item(&(State->player.bag),q->mix_gems.ndx1);
	bag_remove_item(&(State->player.bag),q->mix_gems.ndx2);
	// Apply mana cost.
	State->player.mana -= GEM_MIX_MANA_COST;
      }
      // Remove the event from the queue.
      game_event_remove(q);
      break;
    case GAME_EVENT_NEXT_WAVE:
      // Bring next wave early.
      start_wave();
      // Give player a bonus.
      State->player.mana += 10;
      State->player.xp   += 100;
      // Remove the event from the queue.
      game_event_remove(q);
      break;
    }
  }
}

void tick()
{
  process_events();

  update_enemies();
  update_towers();
  update_player();
}

u64b_t get_time()
{
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return tv.tv_sec * 1000000ull + tv.tv_usec;
}

static void game_loop()
{
  u64b_t         ticks = 0;
  u64b_t         t1,t2,sleep;

  // Set a path for enemies ...
  State->path = load_path_file("data/bmp/path.bmp");

  // Load tower positions from file..
  load_towers_file("data/bmp/towers.bmp");

  // Add some stuff ...
  start_wave();
  add_some_gems();

  // Start the GUI
  StartGUI("pre-Alpha",((gstate_t*)State)); 
  UpdateGuiState(((gstate_t*)State));

  while( (State->time = ++ticks) ) {
    // Progress the game one time step
    t1 = get_time();
    tick();
    t2 = get_time();

    // Update the gui
    UpdateGuiState(((gstate_t*)State));

    // Sleep for the remainder of the tick cycle
    if( (t1 < t2) && ((t2-t1) < 5000) ) {
      sleep = 5000 - (t2-t1);
      if( sleep > 0 ) {
	usleep( sleep );
      }
    }
  }

}

////////////////////////////////////////////////////////////
// Application entry point
////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  // Init sound support
  alutInit(&argc, argv);

  // Get ready to do stuffs...
  init_state();

  // Set the player name
  sprintf(State->player.name,"visaris");

  // Game loop
  game_loop();

  // Done, exit game.
  return 0;
}
