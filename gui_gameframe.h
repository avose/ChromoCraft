#ifndef GUI_GAMEFRAME_H
#define GUI_GAMEFRAME_H

////////////////////////////////////////////////////////////
// For all files

#include "types.h"

typedef struct {
  char   *text;     // Text drawn on gameframe
  u32b_t  val;      // Gameframe value
  u8b_t   sel;      // 1 == selected, 0 == not selected
  u32b_t *link;     // A pointer link to an int controlled by this gameframe
} gameframe_gui_t;

////////////////////////////////////////////////////////////
// For files other than gui_gameframe.c
#ifndef GUI_GAMEFRAME_C
extern void Gameframe_Down(widget_t *w, const int x, const int y, const int b);
extern void Gameframe_Draw(widget_t *w);
#endif

#endif // !GUI_GAMEFRAME_H
