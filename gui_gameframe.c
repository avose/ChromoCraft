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

#include "io_bitmap.h"

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
  vector3_t  p0,p1,p2,p3,a,b,n0,n1;
  float      color[4],shine[1];
  int        i,j;

  static int         init=1;
  static u32b_t      floor;

  if( init ) {
    // Initialize if needed
    init = 0;
    floor = LoadTexture("data/bmp/floor.bmp");
  }

  // Setup material properties and bind the floor texture
  color[0] = 1.0f;
  color[1] = 1.0f;
  color[2] = 1.0f;
  color[3] = 1.0f;
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  color);
  shine[0] = 25.0f;
  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shine);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, floor);

  // Draw the floor
  glBegin(GL_TRIANGLES);
  for(i=0; i<63; i++) {
    for(j=0; j<63; j++) {
      // Get points from the loop and heightmap
      p0.s.x =  i/63.0f;
      p0.s.y =  Statec->terrain.d[(    i*64+    j)*3] / 255.0f / 4.0f;
      p0.s.z =  j/63.0f;
      p1.s.x =  (i+1)/63.0f;
      p1.s.y =  Statec->terrain.d[((i+1)*64+    j)*3] / 255.0f / 4.0f;
      p1.s.z =  j/63.0f;
      p2.s.x =  (i+1)/63.0f;
      p2.s.y =  Statec->terrain.d[((i+1)*64+(j+1))*3] / 255.0f / 4.0f;
      p2.s.z =  (j+1)/63.0f;
      p3.s.x =  i/63.0f;
      p3.s.y =  Statec->terrain.d[(    i*64+(j+1))*3] / 255.0f / 4.0f;
      p3.s.z =  (j+1)/63.0f;
      // Compute normals
      vector3_sub_vector(&p0, &p1, &a);
      vector3_sub_vector(&p0, &p2, &b);
      vector3_crossprod(&b, &a, &n0);
      vector3_normalize(&n0, &n0);
      vector3_sub_vector(&p0, &p2, &a);
      vector3_sub_vector(&p0, &p3, &b);
      vector3_crossprod(&b, &a, &n1);
      vector3_normalize(&n1, &n1);
      // Set normals, texture coords, and verticies
      glNormal3f(n0.s.x, n0.s.y, n0.s.z);
      glTexCoord2f(     i/63.0f,     j/63.0f);  glVertex3f(p0.s.x, p0.s.y, p0.s.z);
      glTexCoord2f( (i+1)/63.0f,     j/63.0f);  glVertex3f(p1.s.x, p1.s.y, p1.s.z);
      glTexCoord2f( (i+1)/63.0f, (j+1)/63.0f);  glVertex3f(p2.s.x, p2.s.y, p2.s.z);
      glNormal3f(n1.s.x, n1.s.y, n1.s.z);
      glTexCoord2f( (i+1)/63.0f, (j+1)/63.0f);  glVertex3f(p2.s.x, p2.s.y, p2.s.z);
      glTexCoord2f(     i/63.0f, (j+1)/63.0f);  glVertex3f(p3.s.x, p3.s.y, p3.s.z);
      glTexCoord2f(     i/63.0f,     j/63.0f);  glVertex3f(p0.s.x, p0.s.y, p0.s.z);
    }
  }
  glEnd();

  // Disable textures as the default?
  glDisable(GL_TEXTURE_2D);
}

static void DrawPath()
{
  path_t *p;
  int     x,y;

  // Draw enemy path
  glEnable(GL_LINE_SMOOTH);
  if( Statec->path ) {
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_LINES); 
    for(p=Statec->path; p != Statec->path->last; ) {
      x = (p->position.s.x)/255.0f * 63.0f;
      y = (p->position.s.y)/255.0f * 63.0f;
      glVertex3f( (p->position.s.x)/255.0f, 
		  Statec->terrain.d[(x*64+y)*3] / 255.0f / 4.0f + 0.005,
		  (p->position.s.y)/255.0f );
      p = p->next;
      x = (p->position.s.x)/255.0f * 63.0f;
      y = (p->position.s.y)/255.0f * 63.0f;
      glVertex3f( (p->position.s.x)/255.0f, 
		  Statec->terrain.d[(x*64+y)*3] / 255.0f / 4.0f + 0.005,
		  (p->position.s.y)/255.0f );
    }
    glEnd();
  }
  glDisable(GL_LINE_SMOOTH);
}

static void DrawTowers()
{
  float       r,color[4],white[4]={1.0f,1.0f,1.0f,1.0f};
  int         i,x,y,slices,stacks;

  static int         init=1;
  static GLUquadric *qdrc;

  if(init) {
    // Init if needed
    init = 0;
    qdrc=gluNewQuadric();
  }

  for(i=0; i<Statec->player.ntowers; i++) {
    // Draw a small sphere at the center of the tower
    slices = stacks = 32;
    r = 2 / 255.0f;
    color[0] = (Statec->player.towers[i].gem.color.a[0])/255.0f;
    color[1] = (Statec->player.towers[i].gem.color.a[1])/255.0f;
    color[2] = (Statec->player.towers[i].gem.color.a[2])/255.0f;
    color[3] = 0.75f;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  color);
    glPushMatrix();
    x = (Statec->player.towers[i].position.s.x)/255.0f * 63.0f;
    y = (Statec->player.towers[i].position.s.y)/255.0f * 63.0f;
    glTranslatef((Statec->player.towers[i].position.s.x)/255.0f,
		 Statec->terrain.d[(x*64+y)*3] / 255.0f / 4.0f + 0.025,
		 (Statec->player.towers[i].position.s.y)/255.0f);
    gluSphere(qdrc, r, slices , stacks);
    glPopMatrix();
    glPushMatrix();
    glTranslatef((Statec->player.towers[i].position.s.x)/255.0f,
		 Statec->terrain.d[(x*64+y)*3] / 255.0f / 4.0f + 0.025,
		 (Statec->player.towers[i].position.s.y)/255.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(qdrc, r/3.0f*2.0f, r/3.0f*2.0f, 0.03, slices, stacks);
    glPopMatrix();
  }
}

static void DrawEnemies()
{
  float r,color[4],white[4]={1.0f,1.0f,1.0f,1.0f};
  int   i,x,y,slices,stacks;

  static int         init=1;
  static GLUquadric *qdrc;

  if(init) {
    // Init if needed
    init = 0;
    qdrc=gluNewQuadric();
  }

  // Draw enemies
  for(i=0; i<Statec->nenemies; i++) {
    // Draw a small sphere at the center of the enemy
    slices = stacks = 32;
    r = 2 / 255.0f;
    color[0] = (Statec->enemies[i].color.a[0])/255.0f;
    color[1] = (Statec->enemies[i].color.a[1])/255.0f;
    color[2] = (Statec->enemies[i].color.a[2])/255.0f;
    color[3] = 0.75f;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  color);
    glPushMatrix();
    x = (Statec->enemies[i].position.s.x)/255.0f * 63.0f;
    y = (Statec->enemies[i].position.s.y)/255.0f * 63.0f;
    glTranslatef((Statec->enemies[i].position.s.x)/255.0f,
		 Statec->terrain.d[(x*64+y)*3] / 255.0f / 4.0f + 0.005,
		 (Statec->enemies[i].position.s.y)/255.0f);
    gluSphere(qdrc, r, slices , stacks);
    glPopMatrix();
  }
}

static void DrawEvents(widget_t *w)
{
  eventq_t *q,*t;
  float     h,color[4];
  float     white[4]={1.0f,1.0f,1.0f,1.0f},black[4]={0.0f,0.0f,0.0f,0.0f};
  int       x,y; 

  // Draw everything on the event list
  for(q=gui_game_event_get(NULL); q; q=gui_game_event_get(q)) {
    // Find event type
    switch(q->type) {
    case GUI_GAME_EVENT_FIRE:
      // Tower fire event
      glEnable(GL_LINE_SMOOTH);
      color[0] = (q->fire.color.a[0])/256.0f;
      color[1] = (q->fire.color.a[1])/256.0f;
      color[2] = (q->fire.color.a[2])/256.0f;
      color[3] = 1.0f;
      glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  color);
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color);
      glLineWidth(2.0f);
      glBegin(GL_LINES); 
      x = (q->fire.tower.s.x)/255.0f * 63.0f;
      y = (q->fire.tower.s.y)/255.0f * 63.0f;
      h = Statec->terrain.d[(x*64+y)*3] / 255.0f / 4.0f + 0.025;
      glVertex3f((q->fire.tower.s.x)/255.0f, h, (q->fire.tower.s.y)/255.0f);
      x = (q->fire.enemy.s.x)/255.0f * 63.0f;
      y = (q->fire.enemy.s.y)/255.0f * 63.0f;
      h = Statec->terrain.d[(x*64+y)*3] / 255.0f / 4.0f + 0.005;
      glVertex3f((q->fire.enemy.s.x)/255.0f, h, (q->fire.enemy.s.y)/255.0f);
      glEnd();
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);
      glLineWidth(1.0f);
      glDisable(GL_LINE_SMOOTH);
      color[0] = 1.0f;
      color[1] = 0.0f;
      color[2] = 0.0f;
      color[3] = 1.0f;
      glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  color);
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color);
      glColor3f( 1.0f, 0.0f, 0.0f);
      x = ((q->fire.tower.s.x+q->fire.enemy.s.x)/2.0)/255.0f * 63.0f;
      y = ((q->fire.tower.s.y+q->fire.enemy.s.y)/2.0)/255.0f * 63.0f;
      h = Statec->terrain.d[(x*64+y)*3] / 255.0f / 4.0f + 0.01;
      glRasterPos3f( ((q->fire.tower.s.x+q->fire.enemy.s.x)/2.0)/255.0f, 
		     h,
		     ((q->fire.tower.s.y+q->fire.enemy.s.y)/2.0)/255.0f );
      printGLf(w->glw->font,"%.0lf",q->fire.health);
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);
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
      color[0] = 1.0f;
      color[1] = 0.0f;
      color[2] = 0.0f;
      color[3] = 1.0f;
      glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  color);
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color);
      x = (q->kill.enemy.s.x)/255.0f * 63.0f;
      y = (q->kill.enemy.s.y)/255.0f * 63.0f;
      h = Statec->terrain.d[(x*64+y)*3] / 255.0f / 4.0f + 0.005;
      glBegin(GL_LINES); 
      glVertex3f((q->kill.enemy.s.x-4)/255.0f, h, (q->kill.enemy.s.y-4)/255.0f);
      glVertex3f((q->kill.enemy.s.x+4)/255.0f, h, (q->kill.enemy.s.y+4)/255.0f);
      glVertex3f((q->kill.enemy.s.x+4)/255.0f, h, (q->kill.enemy.s.y-4)/255.0f);
      glVertex3f((q->kill.enemy.s.x-4)/255.0f, h, (q->kill.enemy.s.y+4)/255.0f);
      glVertex3f((q->kill.enemy.s.x)/255.0f, h-4.0f/255.0f, (q->kill.enemy.s.y)/255.0f);
      glVertex3f((q->kill.enemy.s.x)/255.0f, h+4.0f/255.0f, (q->kill.enemy.s.y)/255.0f);
      glEnd();
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);
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
  GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  GLfloat specular_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  GLfloat diffuse_color[]  = { 0.7f, 0.7f, 0.7f, 1.0f };
  GLfloat ambient_color[]  = { 0.1f, 0.1f, 0.1f, 1.0f };

  // Save 2D state so we can restore it later
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glMatrixMode(GL_MODELVIEW);

  // Switch to 3D perspective
  ViewPort3D(w->x, w->y, ScaleX(w, w->w), ScaleY(w, w->h));
  glMatrixMode(GL_MODELVIEW);

  // Setup lighting
  glShadeModel(GL_SMOOTH);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position );
  glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient_color );
  glLightfv(GL_LIGHT0, GL_SPECULAR, specular_color );
  glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse_color );
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

  // Draw enemy path
  DrawPath();

  // Draw enemies
  DrawEnemies();

  // Draw / handle game events
  DrawEvents(w);

  // Disable lighting
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);

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
