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

static void DrawGround()
{
  float color[4];
  int   i,j;

  // !!av:
  return;

  color[0] = 0.75f;
  color[1] = 0.75f;
  color[2] = 0.75f;
  color[3] = 0.01f;
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
  glBegin(GL_QUADS);
  for(i=0; i<32; i++) {
    for(j=0; j<32; j++) {
      glVertex3f(     i/32.0f, -0.1f,     j/32.0f);
      glVertex3f( (i+1)/32.0f, -0.1f,     j/32.0f);
      glVertex3f( (i+1)/32.0f, -0.1f, (j+1)/32.0f);
      glVertex3f(     i/32.0f, -0.1f, (j+1)/32.0f);
    }
  }
  glEnd();
}

static void DrawPath()
{
  path_t *p;

  // Draw enemy path
  glEnable(GL_LINE_SMOOTH);
  if( Statec->path ) {
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_LINES); 
    for(p=Statec->path; p != Statec->path->last; ) {
      glVertex3f( (p->position.s.x)/255.0f, 0.0f, (p->position.s.y)/255.0f );
      p = p->next;
      glVertex3f( (p->position.s.x)/255.0f, 0.0f, (p->position.s.y)/255.0f );
    }
    glEnd();
  }
  glDisable(GL_LINE_SMOOTH);
}

static void DrawTowers()
{
  GLUquadric *sphere;
  float       r,color[4];
  int         i,j,slices,stacks;

  for(i=0; i<Statec->player.ntowers; i++) {
    // Draw a small sphere at the center of the tower
    sphere=gluNewQuadric();
    slices = stacks = 32;
    r = 4 / 255.0f;
    color[0] = (Statec->player.towers[i].gem.color.a[0])/255.0f;
    color[1] = (Statec->player.towers[i].gem.color.a[1])/255.0f;
    color[2] = (Statec->player.towers[i].gem.color.a[2])/255.0f;
    color[3] = 0.75f;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
    glPushMatrix();
    glTranslatef((Statec->player.towers[i].position.s.x)/255.0f,
		 0.0f,
		 (Statec->player.towers[i].position.s.y)/255.0f);
    gluSphere( sphere, r, slices , stacks);
    glPopMatrix();
  }

  // Draw tower ranges
  glEnable(GL_LINE_SMOOTH);
  glBegin(GL_LINES);
  for(i=0; i<Statec->player.ntowers; i++) {
    color[0] = (Statec->player.towers[i].gem.color.a[0])/255.0f;
    color[1] = (Statec->player.towers[i].gem.color.a[1])/255.0f;
    color[2] = (Statec->player.towers[i].gem.color.a[2])/255.0f;
    color[3] = 0.75f;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
    r = (Statec->player.towers[i].gem.range) / 255.0f;
    for(j=0; j<32; ) {
      glVertex3f((Statec->player.towers[i].position.s.x)/255.0f+r*cos(2*3.14159265*(j/31.0)), 0.0f,
		 (Statec->player.towers[i].position.s.y)/255.0f+r*sin(2*3.14159265*(j/31.0)) );
      j++;
      glVertex3f((Statec->player.towers[i].position.s.x)/255.0f+r*cos(2*3.14159265*(j/31.0)), 0.0f, 
		 (Statec->player.towers[i].position.s.y)/255.0f+r*sin(2*3.14159265*(j/31.0)) );
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
    glColor3f( (Statec->enemies[i].color.a[0])/255.0f, 
               (Statec->enemies[i].color.a[1])/255.0f,
               (Statec->enemies[i].color.a[2])/255.0f  );
    
    glVertex3f((Statec->enemies[i].position.s.x+0)/255.0f, 0.0f, (Statec->enemies[i].position.s.y-2)/255.0f);
    glVertex3f((Statec->enemies[i].position.s.x-2)/255.0f, 0.0f, (Statec->enemies[i].position.s.y+2)/255.0f);
    glVertex3f((Statec->enemies[i].position.s.x+2)/255.0f, 0.0f, (Statec->enemies[i].position.s.y+2)/255.0f);
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
      glVertex3f((q->fire.tower.s.x)/255.0f, 0.0f, (q->fire.tower.s.y)/255.0f);
      glVertex3f((q->fire.enemy.s.x)/255.0f, 0.0f, (q->fire.enemy.s.y)/255.0f);
      glEnd();
      glLineWidth(1.0f);
      glDisable(GL_LINE_SMOOTH);
      glColor3f( 1.0f, 0.0f, 0.0f);
      glRasterPos3f( ((q->fire.tower.s.x+q->fire.enemy.s.x)/2.0)/255.0f, 0.0f,
		     ((q->fire.tower.s.y+q->fire.enemy.s.y)/2.0)/255.0f );
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
      glVertex3f((q->kill.enemy.s.x-2)/255.0f, 0.0f, (q->kill.enemy.s.y-2)/255.0f);
      glVertex3f((q->kill.enemy.s.x+2)/255.0f, 0.0f, (q->kill.enemy.s.y+2)/255.0f);
      glVertex3f((q->kill.enemy.s.x+2)/255.0f, 0.0f, (q->kill.enemy.s.y-2)/255.0f);
      glVertex3f((q->kill.enemy.s.x-2)/255.0f, 0.0f, (q->kill.enemy.s.y+2)/255.0f);
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
  GLfloat light_position[] = { 10.0, 10.0, -10.0, 1.0 };
  GLfloat light_color[]    = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat ambient_color[]  = { 0.2, 0.2, 0.2, 1.0 };

  // Save 2D state so we can restore it later
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glMatrixMode(GL_MODELVIEW);

  // Switch to 3D perspective
  ViewPort3D(w->x, w->y, w->w, w->h);
  glMatrixMode(GL_MODELVIEW);

  // Setup lighting
  glShadeModel(GL_SMOOTH);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position );
  glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient_color );
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_color );
  glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_color );
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);

  // Set "board" positioning / rotation
  glLoadIdentity();
  glTranslatef(0.0f, 0.0f, gf->zoom);
  glRotatef(gf->rotx, 1.0f, 0.0f, 0.0f);
  glRotatef(gf->roty, 0.0f, 1.0f, 0.0f);
  glTranslatef(-0.5f, 0.0f, -0.5f);

  // Draw ground
  DrawGround();

  // Draw towers
  DrawTowers();
  
  // !!av:
  // Disable lighting
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  glEnable(GL_LINE_SMOOTH);
  glBegin(GL_LINES); 

  // Draw enemy path
  DrawPath();

  // Draw enemies
  DrawEnemies();

  // Draw / handle game events
  DrawEvents(w);

  // Restore 2D mode to draw 2D stuffs
  ViewPort2D(w->glw);
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  // Outline
  Yellow();
  glBegin(GL_LINE_LOOP);
  glVertex2f(0.0f,0.0f);
  glVertex2f(0.0f,1.0f);
  glVertex2f(1.0f,1.0f);
  glVertex2f(1.0f,0.0f);
  glEnd();
}

void Gameframe_MouseDown(widget_t *w, int x, int y, int b)
{
  gameframe_gui_t *gf = (gameframe_gui_t*)w->wd;

  // Check bounds
  if( (x >= ScaleX(w, w->x)) && (x <= ScaleX(w, w->x+w->w)) && 
      (y >= ScaleY(w, w->y)) && (y <= ScaleY(w, w->y+w->h))    ) {
    switch(b) {
    case MOUSE_UP:
      // Zoom out
      gf->zoom -= .1;
      if( gf->zoom < 0 ) {
	gf->zoom = 0;
      }
      break;
    case MOUSE_DOWN:
      // Zoom in
      gf->zoom += .1;
      if( gf->zoom > 1 ) {
	gf->zoom = 1;
      }
      break;
    case MOUSE_LEFT:
      // Record that the left mouse is down
      gf->md = b;
      gf->mdx = x;
      gf->mdy = y;
      break;
    }
  }
}

void Gameframe_MouseUp(widget_t *w, int x, int y, int b)
{
  gameframe_gui_t *gf = (gameframe_gui_t*)w->wd;

  if( gf->md == MOUSE_LEFT ) {
    // Apply distance moved to the rotation
    gf->rotx += (((float)y)-gf->mdy) / 3.1415f / 2.0f;
    gf->roty += (((float)x)-gf->mdx) / 3.1415f / 6.0f;
    gf->mdx = x;
    gf->mdy = y;
  }

  // Record that the mouse is now up
  if( gf->md == b ) {
    gf->md = 0;
  }
}

void Gameframe_MouseMove(widget_t *w, int x, int y)
{
  gameframe_gui_t *gf = (gameframe_gui_t*)w->wd;

  if( gf->md == MOUSE_LEFT ) {
    // Apply distance moved to the rotation
    gf->rotx += (((float)y)-gf->mdy) / 3.1415f;
    gf->roty += (((float)x)-gf->mdx) / 3.1415f;
    gf->mdx = x;
    gf->mdy = y;
  }
}


#endif // !GUI_GAMEFRAME_C
