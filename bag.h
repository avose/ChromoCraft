#ifndef BAG_H
#define BAG_H

#include "vector.h"
#include "gem.h"

#define BAG_ITEM_TYPE_NONE 0
#define BAG_ITEM_TYPE_GEM  1

typedef struct st_item {

  u32b_t type; // Item type
  
  union {
    gem_t gem;
  };

} item_t;


typedef struct str_bag_t {
  item_t items[16*4];
} bag_t;


////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////
#ifndef BAG_C
extern void   bag_remove_item(bag_t *bag, u32b_t item);
extern u32b_t bag_add_gem    (bag_t *bag, gem_t *gem);
#endif


#endif
