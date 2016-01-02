#ifndef GUI_GAMEFRAME_C
#define GUI_GAMEFRAME_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <pthread.h>
#include <math.h> 
#include <signal.h>

#include "types.h"
#include "util.h"
#define GUI_WIDGET
#include "gui.h"
#undef GUI_WIDGET
#include "gui_stats.h"

////////////////////////////////////////////////////////////////////////////////

static int BuildStatsBG()
{
  int   bgtex,cl;
  float a,b;

  // I use these values to slightly tweak the textures positions.
  a = 0.0f;
  b = 1.0f;

  // I guess we can load this here..
  bgtex = LoadTexture("data/bmp/StatsBG.bmp"); 

  // Start a list
  glNewList(cl=glGenLists(1),GL_COMPILE);

  // Draw cloth BG.
  glColor3f(0.5f,0.5f,0.5f);
  glBindTexture(GL_TEXTURE_2D,bgtex);
  glBegin(GL_QUADS);
  glTexCoord2f(a,b); glVertex3i(0.0, 1.0, 1.0);
  glTexCoord2f(a,a); glVertex3i(0.0, 0.0, 1.0);
  glTexCoord2f(b,a); glVertex3i(1.0, 0.0, 1.0);
  glTexCoord2f(b,b); glVertex3i(1.0, 1.0, 1.0);
  glEnd();

  // End the list.
  glEndList();

  // Return the new call list
  return cl;
}

void Stats_Draw(widget_t *w)
{
  //stats_gui_t *gf = (stats_gui_t*)w->wd;
  static int bg  = 0;
  char       buf[1024];

  // Init if needed.. I guess this can be here.
  if( bg == 0 ) {
    glEnable(GL_TEXTURE_2D);
    bg = BuildStatsBG();
    glDisable(GL_TEXTURE_2D);
  }

  // Draw background.
  glEnable(GL_TEXTURE_2D);
  White();
  glCallList(bg);
  glDisable(GL_TEXTURE_2D);

  // Draw the xp info:
  Yellow();
  sprintf(buf,"XP:");
  glRasterPos2f(0.1f, 0.1f);
  printGLf(w->glw->font,"%s",buf);
  sprintf(buf,"%.1lf",Statec->player.xp);
  glRasterPos2f(0.4f, 0.1f);
  printGLf(w->glw->font,"%s",buf);

  // Draw the mana info:
  Purple();
  sprintf(buf,"Mana:");
  glRasterPos2f(0.1f, 0.2f);
  printGLf(w->glw->font,"%s",buf);
  sprintf(buf,"%.1lf /",Statec->player.mana);
  glRasterPos2f(0.4f, 0.2f);
  printGLf(w->glw->font,"%s",buf);
  sprintf(buf,"%.0lf",Statec->player.base_mana);
  glRasterPos2f(0.4f, 0.3f);
  printGLf(w->glw->font,"%s",buf);

  // Draw the wave info:
  Green();
  sprintf(buf,"Wave:");
  glRasterPos2f(0.1f, 0.4f);
  printGLf(w->glw->font,"%s",buf);
  sprintf(buf,"%lu",Statec->wave);
  glRasterPos2f(0.4f, 0.4f);
  printGLf(w->glw->font,"%s",buf);

  // Outline
  Yellow();
  glBegin(GL_LINE_LOOP);
  glVertex2f(0.0f,0.0f);
  glVertex2f(0.0f,1.0f);
  glVertex2f(1.0f,1.0f);
  glVertex2f(1.0f,0.0f);
  glEnd();
}

#endif // !GUI_STATS_C
