#ifndef GUI_BAG_H
#define GUI_BAG_H

////////////////////////////////////////////////////////////
// For all files

#include "types.h"

typedef struct {
  u32b_t *mix_btn_link;  // Pointer to mix button's sel flag, so we know that button state.
} bag_gui_t;

////////////////////////////////////////////////////////////
// For files other than gui_bag.c
#ifndef GUI_BAG_C
extern void Bag_Down     (widget_t *w, const int x, const int y, const int b);
extern void Bag_Draw     (widget_t *w);
extern void Bag_KeyPress (widget_t *w, char key, unsigned int keycode);
extern void Bag_MouseMove(widget_t *w, int x, int y);
#endif

#endif // !GUI_BAG_H
