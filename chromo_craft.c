#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "types.h"
#include "util.h"
#include "random.h"
#include "vector.h"
#include "color.h"
#include "effect.h"
#include "gem.h"
#include "enemy.h"
#include "special.h"
#include "tower.h"
#include "player.h"
#define CHROMO_CRAFT_C
#include "chromo_craft.h"
#include "gui.h"

static state_t *State;

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
    Warn("gettiemofday() failed; seeding random generator with 7 instead.\n");
    random_initrand(&State->random,7);
  } else {
    random_initrand(&State->random,tv.tv_usec);
  }

  // Set the current time
  State->time = 1;

  // Set the player up as level 1
  State->player.mana      =   50;
  State->player.base_mana =  100;
}

////////////////////////////////////////////////////////////
// Debug prints
////////////////////////////////////////////////////////////

// Some debugging helpers:
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

////////////////////////////////////////////////////////////
// Randon initial setup
////////////////////////////////////////////////////////////

static void add_some_enemies(u32b_t level)
{
  vector3_t  color;
  u32b_t     i;

  // Get a random color for this wave based on the level
  color.s.x = random_rnd(&State->random,level-1)+2;
  color.s.y = random_rnd(&State->random,level-1)+2;
  color.s.z = random_rnd(&State->random,level-1)+2;
  
  // Add a wave
  enemy_new_wave(&color, State->time, &State->random, &State->enemies, &State->nenemies, &State->senemies);

  // Update position of each enemy
  for(i=0; i<State->nenemies; i++) {
    vector3_copy(&State->path->position, &State->enemies[i].position);
    State->enemies[i].path = State->path->next;
  }
}

static void add_some_towers(u32b_t ntowers, u32b_t level)
{
  vector3_t position;
  gem_t     gem;
  u32b_t    i;

  for(i=0; i<ntowers; i++) {
    // Setup a random position for the towers
    position.s.x = random_rnd(&State->random,256);
    position.s.y = random_rnd(&State->random,256);
    position.s.z = 0.0; //random_rnd(&State->random,256);

    // Create the tower
    player_add_tower(&State->player, &position);
    tower_remove_gem(&State->player.towers[State->player.ntowers-1]);

    // (level == 0) => empty tower
    if( level ) {
      // Build gem to install into tower
      gem.color.s.x = random_rnd(&State->random,level)+1;
      gem.color.s.y = random_rnd(&State->random,level)+1;
      gem.color.s.z = random_rnd(&State->random,level)+1;
      gem.range     = (color_area(&gem.color)/COLOR_MAX_AREA) * 80.0 + 20.0;
      gem.rate      = (color_area(&gem.color)/COLOR_MAX_AREA) * 80.0 + 20.0;
      // Install the gem
      tower_install_gem(&State->player.towers[State->player.ntowers-1],&gem);
    }
  }
}

static void add_some_path()
{
  vector3_t v;
  u32b_t    n;

  // Create a new path head node at the start position
  memset(&v,0,sizeof(v));
  State->path = path_new_path(&v);

  // Add some random positions onto the path linked list
  n = 4 + random_rnd(&State->random,10);
  while(n--) {
    // Find a random position for this node
    v.s.x = random_rnd(&State->random,256);
    v.s.y = random_rnd(&State->random,256);
    v.s.z = 0.0; //random_rnd(&State->random,256);
    // Add the random node
    path_new_node(State->path,&v);
  }

  // Add one last node at the opposite end.
  v.s.x = 255.0;
  v.s.y = 255.0;
  v.s.z = 0.0; //255.0;
  path_new_node(State->path,&v);
}

static void add_some_gems()
{
  gem_t g;

  g.color.a[0] = 200;
  g.color.a[1] = 40;
  g.color.a[2] = 43;

  bag_add_gem(&State->player.bag,&g);
  bag_add_gem(&State->player.bag,&g);
  bag_add_gem(&State->player.bag,&g);

  g.color.a[0] = 32;
  g.color.a[1] = 22;
  g.color.a[2] = 43;

  bag_add_gem(&State->player.bag,&g);

  bag_add_gem(&State->player.bag,&g);
  bag_add_gem(&State->player.bag,&g);

  g.color.a[0] = 32;
  g.color.a[1] = 128;
  g.color.a[2] = 43;

  bag_add_gem(&State->player.bag,&g);
  bag_add_gem(&State->player.bag,&g);
}

////////////////////////////////////////////////////////////
// Main game loop and helpers
////////////////////////////////////////////////////////////

static void update_enemy_effects()
{

}

#define BASE_SPEED 0.025

static void update_enemies_path()
{
  vector3_t v;
  u32b_t    i;

  // Update position of each enemy that has "started"
  for(i=0; i<State->nenemies; i++) {
    // Move enemy towards next path point
    if( (State->enemies[i].start_time < State->time) && (State->enemies[i].path != State->path) ) {
      // Build vector poiting toward waypoint
      vector3_sub_vector(&State->enemies[i].path->position, &State->enemies[i].position, &v);
      // If waypoint is in reach, simply jump there and assign next node
      if( vector3_length(&v) < State->enemies[i].speed*BASE_SPEED ) {
	vector3_copy(&State->enemies[i].path->position, &State->enemies[i].position);
	State->enemies[i].path = State->enemies[i].path->next;
      } else {
	// Just move towards it
	vector3_normalize(&v, &v);
	vector3_mult_scalar(&v,&v,State->enemies[i].speed*BASE_SPEED);
	vector3_add_vector(&State->enemies[i].position, &v, &State->enemies[i].position);
      }
    }
  }

}

#undef BASE_SPEED

static void update_enemies()
{
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
      // Notify GUI of the death
      gui_game_event_kill(State->time,&State->enemies[i]);
      // Kill it (will move last entry into current; retest current
      enemy_kill_enemy(i, State->enemies, &State->nenemies, State->senemies);
    } else {
      // Move on to next entry
      i++;
    }
  }
}

void tick()
{
  update_enemies();
  update_towers();
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
  u64b_t         t1,t2,sleep,i;

  // Set a path for enemies ...
  add_some_path();

  // Add some stuff ...
  add_some_towers(20,255);
  add_some_enemies(100);
  add_some_gems();

  // Start the GUI
  StartGUI("pre-Alpha",State); 
  UpdateGuiState(State);

  while( (State->time = ++ticks) ) {

    if( !(ticks%3000) ) {
      add_some_enemies(100);
    }

    // Progress the game one time step
    t1 = get_time();
    tick();
    t2 = get_time();

    // Update the gui
    UpdateGuiState(State);

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
  // Get ready to do stuffs...
  init_state();

  // Set the player name
  sprintf(State->player.name,"visaris");

  // Game loop
  game_loop();

  // Done, exit game.
  return 0;
}
