#ifndef GUI_H
#define GUI_H

#include <pthread.h>

#include <AL/al.h>
#include <AL/alut.h>

#include "types.h"
#include "path.h"
#include "random.h"
#include "tower.h"
#include "enemy.h"
#include "player.h"
#include "io_bitmap.h"
#include "game_event.h"

// GUI's version of game state
typedef struct str_gstate_t {
  u64b_t      time;         // Current time

  player_t    player;       // Current player

  enemy_t    *enemies;      // Array of enemies
  u32b_t      nenemies;     // Number of enemies
  u32b_t      senemies;     // Size of enemy array
  path_t     *path;         // Path the enemies follow

  rnd_t       random;       // Random number generator state

  io_bitmap_t terrain;      // Heightmap for the floor / terrain
} gstate_t;

// GUI's internal (non-"game") state
typedef struct str_guistate_t {
  u32b_t mouse_item_ndx;     // Current item "held" by the mouse
} guistate_t;

////////////////////////////////////////////////////////////
// For GUI and widget files
////////////////////////////////////////////////////////////
#ifdef GUI_WIDGET
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <GL/gl.h>

// Mouse button codes
#define MOUSE_LEFT   1
#define MOUSE_MIDDLE 2
#define MOUSE_RIGHT  3
#define MOUSE_UP     4
#define MOUSE_DOWN   5

// key and keycode
// These are probably machine + OS/xwindows specific
#define BACKSPACE 8   // Key
#define ESCAPE    27
#define ENTER     13
#define PAGEUP    99  // Keycode
#define PAGEDOWN  105
#define UP        98
#define DOWN      104
#define LEFT      100
#define RIGHT     102
#define F1        67
#define F2        68
#define F3        69
#define F4        70
#define F5        71
#define F6        72
#define F7        73
#define F8        74
#define F9        75
#define F10       76
#define F11       95
#define F12       96

// Holds what is needed to manage an opengl enabled window
typedef struct {
  Display             *dpy;
  int                  screen;
  Window               win;
  GLXContext           ctx;
  XSetWindowAttributes attr;
  int                  x, y;
  unsigned int         width,  height;
  unsigned int         pwidth, pheight;
  unsigned int         depth;   
  GLuint               font;
  int                  id;
} glwindow_t;

// Widget types and callbacks
typedef struct st_widget widget_t;

typedef void (*cbktick_t)      (widget_t *w);
typedef void (*cbkdraw_t)      (widget_t *w);
typedef void (*cbkkeypress_t)  (widget_t *w, char key, unsigned int keycode);
typedef void (*cbkmousedown_t) (widget_t *w, int x, int y, int b);
typedef void (*cbkmouseup_t)   (widget_t *w, int x, int y, int b);
typedef void (*cbkmousemove_t) (widget_t *w, int x, int y);

struct st_widget {
  glwindow_t     *glw;        // Window information
  float           x;          // x-pos
  float           y;          // y-pos
  float           w;          // width
  float           h;          // height
  cbkdraw_t       draw;       //
  cbkkeypress_t   keypress;   // 
  cbkmousedown_t  mousedown;  // Event callbacks
  cbkmouseup_t    mouseup;    //
  cbkmousemove_t  mousemove;  // 
  cbktick_t       tick;       // 
  int             update;     // Update request flag
  void           *wd;         // Custom widget data
};
#endif

////////////////////////////////////////////////////////////
// For files other than gui.c
////////////////////////////////////////////////////////////
#ifndef GUI_C

#ifdef CHROMO_CRAFT_C
// These are intended for the simulation to start/update the gui
extern int  StartGUI(char *version, gstate_t *s); 
extern void UpdateGuiState(gstate_t *s);
#endif // !GUI_WIDGET

// These are intended for use by widgets / gui subcomponents
#ifdef GUI_WIDGET
extern guistate_t        GuiState;
extern gstate_t         *Stateg,*Statec,*Statep;
extern pthread_mutex_t   StateLock;

extern void   printGLf(GLuint font, const char *fmt, ...);
extern u32b_t LoadTexture(char *fn);
extern void   ViewPort3D(int x, int y, int w, int h);
extern void   ViewPort2D(glwindow_t *glw);

extern void Cyan();
extern void Yellow();
extern void Red();
extern void Purple();
extern void Blue();
extern void White();
extern void Green();

extern float ScaleX(widget_t *w, const float x);
extern float ScaleY(widget_t *w, const float y);
extern void  GuiExit();

#define NUM_SOURCES 3
extern ALuint al_sources[NUM_SOURCES];
#define AL_MUSIC al_sources[0]
#define AL_FIRE  al_sources[1]
#define AL_KILL  al_sources[2]


#endif // GUI_WIDGET
#endif // !GUI_C

#endif // !GUI_H
