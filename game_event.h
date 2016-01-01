#ifndef GAME_EVENT_H
#define GAME_EVENT_H

#include "vector.h"
#include "types.h"
#include "gem.h"
#include "enemy.h"
#include "tower.h"


// Game event types
#define GAME_EVENT_NONE              0

#define GAME_EVENT_TOWER_INSTALL_GEM 1
#define GAME_EVENT_TOWER_REMOVE_GEM  2
#define GAME_EVENT_TOWER_SWAP_GEM    3

#define GAME_EVENT_CREATE_GEM        4
#define GAME_EVENT_MIX_GEM           5

#define GAME_EVENT_NEXT_WAVE         6


// Event queue node/queue
typedef struct str_game_eventq_node_t {
  struct str_game_eventq_node_t *last;
  struct str_game_eventq_node_t *next;
  
  u32b_t type;    // Type of event (can use to pull out of union below)
  u32b_t flags;   // Event flags (unique use for each event type)
  u64b_t time;    // Time (in game ticks) this event occurred
  
  union {
    // Struct for tower install, remove, swap gem events.
    struct {
      vector3_t tpos;
      u32b_t    ndx;
    } tower_install_gem;
    // Struct for gem creation and mixing events.
    struct {
      gem_t     gem;
      u32b_t    ndx;
    } create_gem;
  };
} game_eventq_node_t, game_eventq_t;


////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////
#ifndef GAME_EVENT_C
// Consumers
extern game_eventq_t* game_event_get   (game_eventq_t *node);
extern void           game_event_remove(game_eventq_t *node);

// Producers
extern void game_event_tower_install_gem(tower_t *tower, u32b_t ndx);
extern void game_event_tower_remove_gem (tower_t *tower);
extern void game_event_tower_swap_gem   (tower_t *tower, u32b_t ndx);

extern void game_event_create_gem(gem_t *gem);
extern void game_event_mix_gem   (gem_t *gem, u32b_t ndx);

extern void game_event_next_wave();
#endif


#endif
