#ifndef GUI_BAG_H
#define GUI_BAG_H

////////////////////////////////////////////////////////////
// For all files

#include "types.h"

typedef struct {
  int foo;  // For animation or whatever
} bag_gui_t;

////////////////////////////////////////////////////////////
// For files other than gui_bag.c
#ifndef GUI_BAG_C
extern void Bag_Down    (widget_t *w, const int x, const int y, const int b);
extern void Bag_Draw    (widget_t *w);
extern void Bag_KeyPress(widget_t *w, char key, unsigned int keycode);
#endif

#endif // !GUI_BAG_H
