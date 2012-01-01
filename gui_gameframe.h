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
  u32b_t  md;       // Mouse down flags
  u32b_t  mdx;      // Mouse x-axis coordinate when mouse went down
  u32b_t  mdy;      // Mouse y-axis coordinate when mouse went down
  float   rotx;     // Rotation around x-axis
  float   roty;     // Rotation around y-axis
  float   zoom;     // Zoom into the origin
} gameframe_gui_t;


////////////////////////////////////////////////////////////
// For files other than gui_gameframe.c
#ifndef GUI_GAMEFRAME_C
extern void Gameframe_Draw(widget_t *w);
extern void Gameframe_MouseDown(widget_t *w, int x, int y, int b);
extern void Gameframe_MouseUp(widget_t *w, int x, int y, int b);
extern void Gameframe_MouseMove(widget_t *w, int x, int y);
#endif


#endif // !GUI_GAMEFRAME_H
