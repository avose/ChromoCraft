#include <string.h>

#include "types.h"
#include "gem.h"
#include "special.h"
#include "enemy.h"
#include "gui_game_event.h"
#define BAG_C
#include "bag.h"

static s32b_t bag_find_free_slot(bag_t *bag)
{
  u32b_t i;

  for(i=0; i < (sizeof(bag->items)/sizeof(item_t)); i++) {
    if( bag->items[i].type == BAG_ITEM_TYPE_NONE ) {
      // Empty spot found
      return i;
    }
  }

  // Not found
  return -1;
}

////////////////////////////////////////////////////////////////////////////////

void bag_remove_item(bag_t *bag, u32b_t item)
{
  bag->items[item].type = BAG_ITEM_TYPE_NONE;
}

u32b_t bag_add_gem(bag_t *bag, gem_t *gem)
{
  s32b_t slot;

  // Make sure there is room
  if( (slot=bag_find_free_slot(bag)) >= 0 ) {
    // Put gem in bag
    bag->items[slot].type = BAG_ITEM_TYPE_GEM;
    memcpy(&bag->items[slot].gem, gem, sizeof(gem_t));
  }

  // !!av: ??
  return 0;
}
