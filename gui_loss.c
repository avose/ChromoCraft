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
#include <pwd.h>

#include "types.h"
#include "util.h"
#define GUI_WIDGET
#include "gui.h"
#undef GUI_WIDGET
#include "gui_loss.h"


////////////////////////////////////////////////////////////////////////////////


#define HS_FILEN ".chromocraft"


////////////////////////////////////////////////////////////////////////////////


typedef struct st_high_score_t {
  char   tag[1024];
  double time;
  double xp;
} high_score_t;


static int cmp_high_scores(const void *a, const void *b)
{
  high_score_t *hsa = (high_score_t*)a;
  high_score_t *hsb = (high_score_t*)b;

  if( hsa->xp < hsb->xp ) {
    return 1;
  }
  if( hsa->xp > hsb->xp ) {
    return -1;
  }
  return 0;
}


static int high_score(double xp, high_score_t hxp[10])
{
  high_score_t  *hs=NULL;
  struct passwd *pw;
  FILE          *f;
  char           line[1024],tag[1024],path[1024];
  double         tm,txp;
  int            i,j,nhs,hss=0;

  // Insane that fopen() doesn't support paths with "~"
  // in them, so we have to do it manually..
  if( !(pw = getpwuid(getuid())) ) {
    Error("high_score(): Error finding users's home directory.\n");
  }
  sprintf(path,"%s/%s",pw->pw_dir,HS_FILEN);

  // Open high-score file.
  if( !(f=fopen(path, "r+")) ) {
    perror("high_score()");
    Warn("high_score(): Error opening high-score file!\n");
    if( !(f=fopen(path, "w+")) ) {
      perror("high_score()");
      Error("high_score(): Error creating high-score file!\n");
    }
  }

  // Clear the HS list we will return to caler.
  memset(hxp,0,10*sizeof(high_score_t));

  // Parse the file.
  nhs = 0;
  while( fgets(line,sizeof(line),f) != NULL ) {
    // Parse the line
    if( sscanf(line,"%lf %lf %s\n",&tm,&txp,tag) != 3 ) {
      Error("high_score(): Error parsing line in high-score file!\n");
    }
    // Add to list of high scores
    if( !(hs=(high_score_t*)realloc(hs,(nhs+1)*sizeof(high_score_t))) ) {
      Error("high_score(): Failed to grow list of high scores by one.\n");
    }
    strcpy(hs[nhs].tag,tag);
    hs[nhs].time = tm;
    hs[nhs].xp   = txp;
    nhs++;
  }

  // Sort the array of scores we read in
  if( nhs ) {
    qsort(hs, nhs, sizeof(high_score_t), cmp_high_scores);
  }
  
  // Check for new high score and write to file.
  if( !nhs || (xp > hs[0].xp) ) {
    hss = 1;
    strcpy(tag,getenv("USER"));
    for(i=0; i<strlen(tag); i++){
      if( tag[i] == ' ' ) {
	tag[i] = '_';
      }
    }
    tm = time(NULL);
    fprintf(f,"%lf %lf %s\n",tm,xp,tag);
  }

  // Copy top ten (less if aren't that many) to output array.
  if( hss ) {
    strcpy(hxp[0].tag,tag);
    hxp[0].time = tm;
    hxp[0].xp   = xp;
    i = 1;
  } else {
    i = 0;
  }
  for(j=0; (j<nhs) && (i<10); i++,j++) {
    memcpy(&(hxp[i]),&(hs[j]),sizeof(high_score_t));
  }

  // Close and cleanup
  fclose(f);
  free(hs);

  // Return if a new high score was set.
  return hss;
}


////////////////////////////////////////////////////////////////////////////////


void Loss_Draw(widget_t *w)
{
  static high_score_t hxp[10];
  static vector3_t    color;
  static double       xp;
  static int          first_neg=1,hss=0;
  char                buf[1024];
  int                 i;

  
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
      hss = high_score(xp,hxp);
      if( hss ) {
	// We made a new high-score! Play the triumph song!
	alSourcePlay(AL_TRIUMPH);
	if( alGetError() != AL_NO_ERROR ) {
	  Warn("Error starting triumph music.\n");
	}
      } else {
	// We did no get a new high-score! Play the failure song!
	alSourcePlay(AL_FAILURE);
	if( alGetError() != AL_NO_ERROR ) {
	  Warn("Error starting failure music.\n");
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
  if( hss ) {
    sprintf(buf,"You Lose!! Lolol!! - High Score!!");
  } else {
    sprintf(buf,"You Lose!! Lolol!! - Low Score.. (%lf < %lf)",xp,hxp[0].xp);
  }
  glRasterPos2f(0.5f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)), 0.3f+4.0f/ScaleY(w,w->h));
  printGLf(w->glw->font,"%s",buf);
  sprintf(buf,"XP: %.1lf",xp);
  glRasterPos2f(0.5f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)), 0.35f+4.0f/ScaleY(w,w->h));
  printGLf(w->glw->font,"%s",buf);
  sprintf(buf,"Killed By: (%.0lf,%.0lf,%.0lf)",color.s.x,color.s.y,color.s.z);
  glRasterPos2f(0.5f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)), 0.4f+4.0f/ScaleY(w,w->h));
  printGLf(w->glw->font,"%s",buf);

  // Draw high-score list
  sprintf(buf,"High Scores:");
  glRasterPos2f(0.5f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)), 0.5f+4.0f/ScaleY(w,w->h));
  printGLf(w->glw->font,"%s",buf);
  for(i=0; i<10; i++) {
    sprintf(buf,"XP:    %.1lf",hxp[i].xp);
    glRasterPos2f(0.5f-((strlen("XP:    ")*6.0f/2.0f)/ScaleX(w,w->w)), (0.025*(i+1))+0.5f+4.0f/ScaleY(w,w->h));
    printGLf(w->glw->font,"%s",buf);
  }

  glEnable(GL_DEPTH_TEST);
}

#endif // !GUI_LOSS_C
