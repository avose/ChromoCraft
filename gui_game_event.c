#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "types.h"
#include "gem.h"
#include "enemy.h"
#include "util.h"
#define GUI_GAME_EVENT
#include "gui_game_event.h"


////////////////////////////////////////////////////////////////////////////////


static void          gui_game_event_new_node  (gui_eventq_t *eventq);
static gui_eventq_t* gui_game_event_new_eventq();


////////////////////////////////////////////////////////////////////////////////


// Main event q
static gui_eventq_t    *Events;
static pthread_mutex_t  EventLock=PTHREAD_MUTEX_INITIALIZER;


////////////////////////////////////////////////////////////////////////////////
// Init / internally used
////////////////////////////////////////////////////////////////////////////////


// Allocates initial event q
static void gui_game_event_init()
{
  pthread_mutex_lock(&EventLock);

  // One time allocation
  if( !Events ) {
    Events = gui_game_event_new_eventq();
  }

  pthread_mutex_unlock(&EventLock);
}

// Appends a node to the end of the gui_game_event linked list
static void gui_game_event_new_node(gui_eventq_t *eventq)
{
  gui_eventq_node_t *node;

  // Allocate a new eventq node
  if( !(node=malloc(sizeof(gui_eventq_node_t))) ) {
    Error("Failed to allocate space (%u) for new eventq node.\n",sizeof(gui_eventq_node_t));
  }

  // Setup the new node for insertion
  memset(node,0,sizeof(gui_eventq_node_t));
  node->next = eventq;
  node->last = eventq->last;

  // Insert the node
  eventq->last->next = node;
  eventq->last       = node;
}

static gui_eventq_t* gui_game_event_new_eventq()
{
  gui_eventq_t *eventq;

  // Allocate eventq
  if( !(eventq=malloc(sizeof(gui_eventq_t))) ) {
    Error("Failed to allocate space (%u) for new eventq.\n",sizeof(gui_eventq_t));
  }
  memset(eventq,0,sizeof(gui_eventq_t));
  eventq->next = eventq;
  eventq->last = eventq;

  return eventq;
}


////////////////////////////////////////////////////////////////////////////////
// Used by event producers
////////////////////////////////////////////////////////////////////////////////


gui_eventq_t* gui_game_event_get(gui_eventq_t *node)
{
  gui_game_event_init();

  // Set a current if none
  if( !node ) {
    node = Events;
  }

  // Return next node
  pthread_mutex_lock(&EventLock);

  if( node->next == Events ) {
    pthread_mutex_unlock(&EventLock);
    return NULL;
  }

  // Grab the next node
  node = node->next;

  pthread_mutex_unlock(&EventLock);

  // Return the next node
  return node;
}

void gui_game_event_remove(gui_eventq_t *node)
{
  gui_game_event_init();

  pthread_mutex_lock(&EventLock);

  // Do some sanity checks
  if( (!node) || (node == Events) ) {
    pthread_mutex_unlock(&EventLock);
    return;
  }

  // Remove it from the queue
  node->next->last = node->last;
  node->last->next = node->next;

  // Free node
  free(node);

  pthread_mutex_unlock(&EventLock);
}


////////////////////////////////////////////////////////////////////////////////
// Used by event consumers
////////////////////////////////////////////////////////////////////////////////


void gui_game_event_kill(u64b_t time, enemy_t *enemy)
{
  gui_game_event_init();

  // Fill in new node
  pthread_mutex_lock(&EventLock);

  gui_game_event_new_node(Events);
  Events->last->type = GUI_GAME_EVENT_KILL;
  vector3_copy(&enemy->position, &Events->last->kill.enemy);
  Events->last->time = time;
  
  pthread_mutex_unlock(&EventLock);
}

void gui_game_event_fire(u64b_t time, tower_t *tower, enemy_t *enemy, double dmg)
{
  gui_game_event_init();

  // Fill in new node
  pthread_mutex_lock(&EventLock);

  gui_game_event_new_node(Events);
  Events->last->type = GUI_GAME_EVENT_FIRE;
  vector3_copy(&tower->position,  &Events->last->fire.tower);
  vector3_copy(&enemy->position,  &Events->last->fire.enemy);
  vector3_copy(&tower->gem.color, &Events->last->fire.color);
  Events->last->fire.health = dmg;
  Events->last->time        = time;
  
  pthread_mutex_unlock(&EventLock);
}

void gui_game_event_hit(u64b_t time, enemy_t *enemy)
{
  gui_game_event_init();

  // Fill in new node
  pthread_mutex_lock(&EventLock);

  gui_game_event_new_node(Events);
  Events->last->type = GUI_GAME_EVENT_HIT;
  vector3_copy(&(enemy->position), &(Events->last->hit.enemy));
  vector3_copy(&(enemy->color), &(Events->last->hit.color));
  Events->last->time = time;
  
  pthread_mutex_unlock(&EventLock);
}
