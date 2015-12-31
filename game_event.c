#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "types.h"
#include "gem.h"
#include "enemy.h"
#include "util.h"
#define GAME_EVENT
#include "game_event.h"


////////////////////////////////////////////////////////////////////////////////


static void           game_event_new_node  (game_eventq_t *eventq);
static game_eventq_t* game_event_new_eventq();


////////////////////////////////////////////////////////////////////////////////


// Main event q
static game_eventq_t   *Events;
static pthread_mutex_t  EventLock=PTHREAD_MUTEX_INITIALIZER;


////////////////////////////////////////////////////////////////////////////////
// Init / internally used
////////////////////////////////////////////////////////////////////////////////


// Allocates initial event q
static void game_event_init()
{
  pthread_mutex_lock(&EventLock);

  // One time allocation
  if( !Events ) {
    Events = game_event_new_eventq();
  }

  pthread_mutex_unlock(&EventLock);
}

// Appends a node to the end of the game_event linked list
static void game_event_new_node(game_eventq_t *eventq)
{
  game_eventq_node_t *node;

  // Allocate a new eventq node
  if( !(node=malloc(sizeof(game_eventq_node_t))) ) {
    Error("Failed to allocate space (%u) for new eventq node.\n",sizeof(game_eventq_node_t));
  }

  // Setup the new node for insertion
  memset(node,0,sizeof(game_eventq_node_t));
  node->next = eventq;
  node->last = eventq->last;

  // Insert the node
  eventq->last->next = node;
  eventq->last       = node;
}

static game_eventq_t* game_event_new_eventq()
{
  game_eventq_t *eventq;

  // Allocate eventq
  if( !(eventq=malloc(sizeof(game_eventq_t))) ) {
    Error("Failed to allocate space (%u) for new eventq.\n",sizeof(game_eventq_t));
  }
  memset(eventq,0,sizeof(game_eventq_t));
  eventq->next = eventq;
  eventq->last = eventq;

  return eventq;
}


////////////////////////////////////////////////////////////////////////////////
// Used by event consumers
////////////////////////////////////////////////////////////////////////////////


game_eventq_t* game_event_get(game_eventq_t *node)
{
  game_event_init();

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

void game_event_remove(game_eventq_t *node)
{
  game_event_init();

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
// Used by event producers
////////////////////////////////////////////////////////////////////////////////


void game_event_tower_install_gem(tower_t *tower, u32b_t ndx)
{
  game_event_init();

  // Fill in new node
  pthread_mutex_lock(&EventLock);

  game_event_new_node(Events);
  Events->last->type = GAME_EVENT_TOWER_INSTALL_GEM;
  Events->last->tower_install_gem.ndx = ndx;
  vector3_copy(&(tower->position), &(Events->last->tower_install_gem.tpos));
  
  pthread_mutex_unlock(&EventLock);
}


void game_event_tower_remove_gem(tower_t *tower)
{
  game_event_init();

  // Fill in new node
  pthread_mutex_lock(&EventLock);

  game_event_new_node(Events);
  Events->last->type = GAME_EVENT_TOWER_REMOVE_GEM;
  vector3_copy(&(tower->position), &(Events->last->tower_install_gem.tpos));
  
  pthread_mutex_unlock(&EventLock);
}


void game_event_tower_swap_gem(tower_t *tower, u32b_t ndx)
{
  game_event_init();

  // Fill in new node
  pthread_mutex_lock(&EventLock);

  game_event_new_node(Events);
  Events->last->type = GAME_EVENT_TOWER_SWAP_GEM;
  Events->last->tower_install_gem.ndx = ndx;
  vector3_copy(&(tower->position), &(Events->last->tower_install_gem.tpos));
  
  pthread_mutex_unlock(&EventLock);
}


void game_event_create_gem(gem_t *gem)
{
  game_event_init();

  // Fill in new node
  pthread_mutex_lock(&EventLock);

  game_event_new_node(Events);
  Events->last->type = GAME_EVENT_CREATE_GEM;
  memcpy(&(Events->last->create_gem.gem), gem, sizeof(gem_t));
  Events->last->create_gem.ndx = -1;  

  pthread_mutex_unlock(&EventLock);
}

void game_event_mix_gem(gem_t *gem, u32b_t ndx)
{
  game_event_init();

  // Fill in new node
  pthread_mutex_lock(&EventLock);

  game_event_new_node(Events);
  Events->last->type = GAME_EVENT_MIX_GEM;
  memcpy(&(Events->last->create_gem.gem), gem, sizeof(gem_t));
  Events->last->create_gem.ndx = ndx;
  
  pthread_mutex_unlock(&EventLock);
}
