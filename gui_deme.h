#ifndef GUI_DEME_H
#define GUI_DEME_H

////////////////////////////////////////////////////////////
// For all files

#include "types.h"
#include "gui.h"

// Heightmap modes
#define NDM 8
enum { DM_X0=0, DM_X1, DM_Y0, DM_Y1, DM_Z0, DM_Z1, DM_C, DM_M, DM_F };
enum { DM_2D=0, DM_3D=1, DM_3D_TRAIT=2 };

// Structure for the ->wd user data field of the overall demes graph
typedef struct {
  s32b_t  x;         // Currently selected deme
  s32b_t  y;

  u32b_t  md;        // Mouse down flag
} demes_gui_t;

// Structure for the ->wd user data field of the within-deme graph
typedef struct {
  s32b_t  ox;        // Mouse down position
  s32b_t  oy;
  s32b_t  nx;        // Current mouse position
  s32b_t  ny;

  u32b_t  mb;        // Mouse button involved
  u32b_t  d;         // Mouse down flag

  vec3f_t rot;       // Rotation vector
  vec3f_t trn;       // Translation vector
  s32b_t  xmode;     // Currently displayed x-axis
  s32b_t  ymode;     // Currently displayed y-axis
  s32b_t  mode;      // 2D/3D mode

  u32b_t  ticks;     // Timer ticks
  u32b_t  anim;      // Animate (auto-rotate) flag;

  u32b_t  clusters;  // Draw cluster markers flag (4D view)

  u32b_t  spctrshld; // Threshold value for 4D trait density plot.
                     // The name implies it can be helpful for visualizing
                     // species.

  ////////////////////////

  demes_gui_t *dc;   // The "demes" widget controls the within-deme graph

} deme_gui_t;

////////////////////////////////////////////////////////////
// For files other than gui_deme.c
#ifndef GUI_DEME_C
extern void FillHeightMap(widget_t *w, float **hm, char *title, u32b_t *xr, u32b_t *yr);

extern void Demes_PostScript(char *fn);

extern void Demes_Draw(widget_t *w);
extern void Demes_Down(widget_t *w, const int x, const int y, const int b);
extern void Demes_Up(widget_t *w, const int x, const int y, const int b);
extern void Demes_Move(widget_t *w, const int x, const int y);

extern void Deme_Draw (widget_t *w);
extern void Deme_Tick (widget_t *w);
extern void Deme_Down (widget_t *w, const int x, const int y, const int b);
extern void Deme_Up   (widget_t *w, const int x, const int y, const int b);
extern void Deme_Move (widget_t *w, const int x, const int y);
#endif // !GUI_DEME_C

#endif // !GUI_DEME_H
