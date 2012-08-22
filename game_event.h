#ifndef GAME_EVENT_H
#define GAME_EVENT_H

#include "vector.h"
#include "types.h"
#include "gem.h"
#include "enemy.h"
#include "tower.h"

// Game event types
#define GAME_EVENT_TOWER_INSTALL_GEM 1

// Event queue node/queue
typedef struct str_game_eventq_node_t {
  struct str_game_eventq_node_t *last;
  struct str_game_eventq_node_t *next;
  
  u32b_t type;    // Type of event (can use to pull out of union below)
  u32b_t flags;   // Event flags (unique use for each event type)
  u64b_t time;    // Time (in game ticks) this event occurred
  
  union {
    // Struct for a tower install gem event
    struct {
      vector3_t tpos;
      u32b_t    ndx;
    } tower_install_gem;
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
#endif


#endif
