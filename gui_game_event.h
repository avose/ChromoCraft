#ifndef GUI_GAME_EVENT_H
#define GUI_GAME_EVENT_H

#include "vector.h"
#include "types.h"
#include "gem.h"
#include "enemy.h"
#include "tower.h"

// Game event types
#define GUI_GAME_EVENT_FIRE 1
#define GUI_GAME_EVENT_KILL 2

// Event queue node/queue
typedef struct str_eventq_node_t {
  struct str_eventq_node_t *last;
  struct str_eventq_node_t *next;
  
  u32b_t type;    // Type of event (can use to pull out of union below)
  u64b_t time;    // Time (in game ticks) this event occurred
  
  union {
    // Struct for a fire event
    struct {
      vector3_t tower;
      vector3_t color;
      vector3_t enemy;
      double    health;
    } fire;
    // Struct for a enemy kill event
    struct {
      vector3_t enemy;
    } kill;
  };
} eventq_node_t, eventq_t;


////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////
#ifndef GUI_GAME_EVENT_C
// Consumers
extern eventq_t* gui_game_event_get   (eventq_t *node);
extern void      gui_game_event_remove(eventq_t *node);

// Producers
extern void      gui_game_event_kill(u64b_t time, enemy_t *enemy);
extern void      gui_game_event_fire(u64b_t time, tower_t *tower, enemy_t *enemy, double dmg);
#endif


#endif
