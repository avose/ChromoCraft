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
#include "gui_game_event.h"
#define GUI_WIDGET
#include "gui.h"
#undef GUI_WIDGET
#include "gui_gameframe.h"

////////////////////////////////////////////////////////////////////////////////

// Button callbacks

void Gameframe_Down(widget_t *w, const int x, const int y, const int b)
{
  if( (x > ScaleX(w,w->x)) && (x < ScaleX(w,w->x+w->w)) && 
      (y > ScaleY(w,w->y)) && (y < ScaleY(w,w->y+w->h))     ) {
    //GuiExit();
  }
}

////////////////////////////////////////////////////////////////////////////////

static void DrawPath()
{
  path_t *p;

  // Draw enemy path
  glEnable(GL_LINE_SMOOTH);
  if( Statec->path ) {
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_LINES); 
    for(p=Statec->path; p != Statec->path->last; ) {
      glVertex2f( (p->position.s.x)/255.0, (p->position.s.y)/255.0 );
      p = p->next;
      glVertex2f( (p->position.s.x)/255.0, (p->position.s.y)/255.0 );
    }
    glEnd();
  }
  glDisable(GL_LINE_SMOOTH);
}

static void DrawTowers()
{
  double r;
  int    i,j;


  // Draw towers
  for(i=0; i<Statec->player.ntowers; i++) {
    // Tower border
    glBegin(GL_POLYGON);   
    glColor3f(1.0f, 1.0f, 1.0f); 
    r = 4 / 255.0;
    for(j=0; j<32; ) {
      glVertex3f((Statec->player.towers[i].position.s.x)/255.0+r*cos(2*3.14159265*(j/31.0)), 
		 (Statec->player.towers[i].position.s.y)/255.0+r*sin(2*3.14159265*(j/31.0)), 1.0f );
      j++;
      glVertex3f((Statec->player.towers[i].position.s.x)/255.0+r*cos(2*3.14159265*(j/31.0)), 
		 (Statec->player.towers[i].position.s.y)/255.0+r*sin(2*3.14159265*(j/31.0)), 1.0f );
    }
    glEnd();
    glBegin(GL_POLYGON);
    // Tower center
    glColor3f( (Statec->player.towers[i].gem.color.a[0])/255.0, 
               (Statec->player.towers[i].gem.color.a[1])/255.0,
               (Statec->player.towers[i].gem.color.a[2])/255.0  );
    r = 3 / 255.0;
    for(j=0; j<32; ) {
      glVertex2f((Statec->player.towers[i].position.s.x)/255.0+r*cos(2*3.14159265*(j/31.0)), 
		 (Statec->player.towers[i].position.s.y)/255.0+r*sin(2*3.14159265*(j/31.0))  );
      j++;
      glVertex2f((Statec->player.towers[i].position.s.x)/255.0+r*cos(2*3.14159265*(j/31.0)), 
		 (Statec->player.towers[i].position.s.y)/255.0+r*sin(2*3.14159265*(j/31.0))  );
    }
    glEnd();
  }

  // Draw tower ranges
  glEnable(GL_LINE_SMOOTH);
  glBegin(GL_LINES); 
  for(i=0; i<Statec->player.ntowers; i++) {
    glColor3f( (Statec->player.towers[i].gem.color.a[0])/512.0, 
               (Statec->player.towers[i].gem.color.a[1])/512.0,
               (Statec->player.towers[i].gem.color.a[2])/512.0  );
    r = (Statec->player.towers[i].gem.range) / 255.0;
    for(j=0; j<32; ) {
      glVertex2f((Statec->player.towers[i].position.s.x)/255.0+r*cos(2*3.14159265*(j/31.0)), 
		 (Statec->player.towers[i].position.s.y)/255.0+r*sin(2*3.14159265*(j/31.0))  );
      j++;
      glVertex2f((Statec->player.towers[i].position.s.x)/255.0+r*cos(2*3.14159265*(j/31.0)), 
		 (Statec->player.towers[i].position.s.y)/255.0+r*sin(2*3.14159265*(j/31.0))  );
    }
  }
  glEnd();
  glDisable(GL_LINE_SMOOTH);
}

static void DrawEnemies()
{
  int i;

  // Draw enemies
  glBegin(GL_TRIANGLES);
  for(i=0; i<Statec->nenemies; i++) {
    glColor3f( (Statec->enemies[i].color.a[0])/255.0, 
               (Statec->enemies[i].color.a[1])/255.0,
               (Statec->enemies[i].color.a[2])/255.0  );
    
    glVertex2f((Statec->enemies[i].position.s.x+0)/255.0, (Statec->enemies[i].position.s.y-2)/255.0);
    glVertex2f((Statec->enemies[i].position.s.x-2)/255.0, (Statec->enemies[i].position.s.y+2)/255.0);
    glVertex2f((Statec->enemies[i].position.s.x+2)/255.0, (Statec->enemies[i].position.s.y+2)/255.0);
  }
  glEnd();
}

static void DrawEvents(widget_t *w)
{
  eventq_t *q,*t;

  // Draw everything on the event list
  for(q=gui_game_event_get(NULL); q; q=gui_game_event_get(q)) {
    // Find event type
    switch(q->type) {
    case GUI_GAME_EVENT_FIRE:
      // Tower fire event
      glEnable(GL_LINE_SMOOTH);
      glColor3f( (q->fire.color.a[0])/256.0, 
		 (q->fire.color.a[1])/256.0,
		 (q->fire.color.a[2])/256.0  );
      glLineWidth(2.0f);
      glBegin(GL_LINES); 
      glVertex2f((q->fire.tower.s.x)/255.0, (q->fire.tower.s.y)/255.0);
      glVertex2f((q->fire.enemy.s.x)/255.0, (q->fire.enemy.s.y)/255.0);
      glEnd();
      glLineWidth(1.0f);
      glDisable(GL_LINE_SMOOTH);
      glColor3f( 1.0f, 0.0f, 0.0f);
      glRasterPos2f( ((q->fire.tower.s.x+q->fire.enemy.s.x)/2.0)/255.0,
		     ((q->fire.tower.s.y+q->fire.enemy.s.y)/2.0)/255.0   );
      printGLf(w->glw->font,"%.0lf",q->fire.health);
      // Remove if needed
      if( (Statec->time - q->time) > 20 ) {
	t = q->last;
	gui_game_event_remove(q);
	q = t;
      }
      break;
    case GUI_GAME_EVENT_KILL:
      // Enemy death event
      glEnable(GL_LINE_SMOOTH);
      glColor3f(1.0f, 0.0f, 0.0f); 
      glBegin(GL_LINES); 
      glVertex2f((q->kill.enemy.s.x-2)/255.0, (q->kill.enemy.s.y-2)/255.0);
      glVertex2f((q->kill.enemy.s.x+2)/255.0, (q->kill.enemy.s.y+2)/255.0);
      glVertex2f((q->kill.enemy.s.x+2)/255.0, (q->kill.enemy.s.y-2)/255.0);
      glVertex2f((q->kill.enemy.s.x-2)/255.0, (q->kill.enemy.s.y+2)/255.0);
      glEnd();
      glDisable(GL_LINE_SMOOTH);
      // Remove if needed
      if( (Statec->time - q->time) > 250 ) {
	t = q->last;
	gui_game_event_remove(q);
	q = t;
      }
      break;
    default:
      // Unknown event type
      break;
    }
  }
}

////////////////////////////////////////////////////////////

void Gameframe_Draw(widget_t *w)
{
  gameframe_gui_t *gf = (gameframe_gui_t*)w->wd;

  // Draw enemy path
  DrawPath();

  // Draw towers
  DrawTowers();

  // Draw enemies
  DrawEnemies();

  // Draw / handle game events
  DrawEvents(w);

  // Outline
  Yellow();
  glBegin(GL_LINE_LOOP);
  glVertex2f(0.0f,0.0f);
  glVertex2f(0.0f,1.0f);
  glVertex2f(1.0f,1.0f);
  glVertex2f(1.0f,0.0f);
  glEnd();

}

#endif // !GUI_GAMEFRAME_C
