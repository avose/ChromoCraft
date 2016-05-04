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


#define HS_FILEN "./.chromocraft"


////////////////////////////////////////////////////////////////////////////////


static double high_score(double xp)
{
  FILE  *f;
  char   line[1024],tag[1024],mtag[1024];
  double tm,txp,mxp,mtm;
  int    i;

  // Open high-score file.
  if( !(f=fopen(HS_FILEN, "r+")) ) {
    Warn("high_score(): Error opening high-score file!\n");
    if( !(f=fopen(HS_FILEN, "w+")) ) {
      Error("high_score(): Error creating high-score file!\n");
    }
  }

  // Parse the file.
  mtm = 0;
  mxp = 0;
  while( fgets(line,sizeof(line),f) != NULL ) {
    sscanf(line,"%lf %lf %s\n",&tm,&txp,tag);
    if( txp > mxp )   {
      mxp = txp;
      mtm = tm;
      strcpy(mtag,tag);
    }
  }
  
  // Check for new high score and write to file.
  if( xp > mxp ) {
    mxp = xp;
    mtm = time(NULL);
    strcpy(mtag,getenv("USER"));
    for(i=0; i<strlen(mtag); i++){
      if( mtag[i] == ' ' ) {
	mtag[i] = '_';
      }
    }
    fprintf(f,"%lf %lf %s\n",mtm,mxp,mtag);
  }

  // Close and cleanup
  fclose(f);

  // Return the max high score so far.
  return mxp;
}


////////////////////////////////////////////////////////////////////////////////


void Loss_Draw(widget_t *w)
{
  static vector3_t color;
  static double    xp,hxp;
  static int       first_neg=1;
  char             buf[1024];

  
  // Detect first "death" event.
  if( (Statec->player.mana >= 0) && (first_neg == 1) ) {
    // Still alive, so this widget remains hidden.
    return;
  } else {
    // First time we found a loss event.
    if( first_neg == 1 ) {
      vector3_copy(&(GuiState.enemy_hit_color), &color);
      xp = Statec->player.xp;
      // Stop default music.
      alSourceStop(AL_MUSIC);
      if( alGetError() != AL_NO_ERROR ) {
	Warn("Error stopping background music.\n");
      }
      // Read high-score file.
      hxp = high_score(xp);
      if( hxp == xp ) {
	alSourcePlay(AL_TRIUMPH);
	if( alGetError() != AL_NO_ERROR ) {
	  Warn("Error starting triumph music.\n");
	}
      }
      first_neg = 0;
    }
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
  if( hxp == xp ) {
    sprintf(buf,"You Lose!! Lolol!! - High Score!!");
  } else {
    sprintf(buf,"You Lose!! Lolol!! - Low Score.. (%lf < %lf)",xp,hxp);
  }
  glRasterPos2f(0.5f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)), 0.5f+4.0f/ScaleY(w,w->h));
  printGLf(w->glw->font,"%s",buf);
  sprintf(buf,"XP: %.1lf",xp);
  glRasterPos2f(0.5f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)), 0.55f+4.0f/ScaleY(w,w->h));
  printGLf(w->glw->font,"%s",buf);
  sprintf(buf,"Killed By: (%.0lf,%.0lf,%.0lf)",color.s.x,color.s.y,color.s.z);
  glRasterPos2f(0.5f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)), 0.6f+4.0f/ScaleY(w,w->h));
  printGLf(w->glw->font,"%s",buf);


  glEnable(GL_DEPTH_TEST);
}

#endif // !GUI_LOSS_C
