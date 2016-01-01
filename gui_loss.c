#ifndef GUI_LOSS_C
#define GUI_LOSS_C

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
#include "gui_loss.h"

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

void Loss_Draw(widget_t *w)
{
  if( Statec->player.mana >= 0 ) {
    return;
  }

  glDisable(GL_DEPTH_TEST);

  // Outline
  glColor4ub(0,0,0,220);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_POLYGON);
  glVertex2f(0.0f,0.0f);
  glVertex2f(0.0f,1.0f);
  glVertex2f(1.0f,1.0f);
  glVertex2f(1.0f,0.0f);
  glEnd();
  glDisable(GL_BLEND);
  Yellow();
  glBegin(GL_LINE_LOOP);
  glVertex2f(0.0f,0.0f);
  glVertex2f(0.0f,1.0f);
  glVertex2f(1.0f,1.0f);
  glVertex2f(1.0f,0.0f);
  glEnd();

  // Draw message text
  White();
  glRasterPos2f(0.5f-((strlen("You Lose!! Lolol!!")*6.0f/2.0f)/ScaleX(w,w->w)), 0.5f+4.0f/ScaleY(w,w->h));
  printGLf(w->glw->font,"%s","You Lose!! Lolol!!");

  glEnable(GL_DEPTH_TEST);
}

#endif // !GUI_LOSS_C
