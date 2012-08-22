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
#include <AL/al.h>
#include <AL/alut.h>
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

static int *gl_lights;
static int  gl_nlights = 1;
static int  gl_maxlights;

////////////////////////////////////////////////////////////////////////////////

// Button callbacks

void Gameframe_Down(widget_t *w, const int x, const int y, const int b)
{
  if( (x > ScaleX(w,w->x)) && (x < ScaleX(w,w->x+w->w)) && 
      (y > ScaleY(w,w->y)) && (y < ScaleY(w,w->y+w->h))     ) {
    // !!av:
    // Project all towers from 3D coords to 2D
    // Select closest tower within a radius of the mouse position
    // If the tower has a gem, start dragging the gem
  }
}

////////////////////////////////////////////////////////////////////////////////

static int BuildSkyBox()
{
  float color[4]={0.25f,0.25f,0.25f,1.0f};
  int   stup,stdown,stleft,stright,stback,stfront;
  int   cl;
  float a,b;

  // I use these values to slightly tweak the skybox textures
  a = 8/4500.0f;
  b = 1.0f-8/4500.0f;

  // I don't really see anything wrong with loading textures
  // here.  There are not many of them, and this whole section
  // is done only once anyways.
  stup    = LoadTexture("data/bmp/SkyUp.bmp"   ); 
  stdown  = LoadTexture("data/bmp/SkyDown.bmp" ); 
  stleft  = LoadTexture("data/bmp/SkyLeft.bmp" ); 
  stright = LoadTexture("data/bmp/SkyRight.bmp"); 
  stback  = LoadTexture("data/bmp/SkyBack.bmp" ); 
  stfront = LoadTexture("data/bmp/SkyFront.bmp");

  glNewList(cl=glGenLists(1),GL_COMPILE);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  color);
  glColor3f(1.0f,1.0f,1.0f);
  glBindTexture(GL_TEXTURE_2D,stup);
  glBegin(GL_QUADS);
  glTexCoord2f(b,b); glVertex3i(-10, 10,  10);
  glTexCoord2f(a,b); glVertex3i(-10, 10, -10);
  glTexCoord2f(a,a); glVertex3i( 10, 10, -10);
  glTexCoord2f(b,a); glVertex3i( 10, 10,  10);
  glEnd();
  glBindTexture(GL_TEXTURE_2D,stdown);
  glBegin(GL_QUADS);
  glTexCoord2f(b,a); glVertex3i(-10,-10,  10);
  glTexCoord2f(b,b); glVertex3i( 10,-10,  10);
  glTexCoord2f(a,b); glVertex3i( 10,-10, -10);
  glTexCoord2f(a,a); glVertex3i(-10,-10, -10);
  glEnd();
  glBindTexture(GL_TEXTURE_2D,stleft);
  glBegin(GL_QUADS);
  glTexCoord2f(a,a); glVertex3i(-10,-10, 10);
  glTexCoord2f(b,a); glVertex3i(-10,-10,-10);
  glTexCoord2f(b,b); glVertex3i(-10, 10,-10);
  glTexCoord2f(a,b); glVertex3i(-10, 10, 10);
  glEnd();
  glBindTexture(GL_TEXTURE_2D,stright);
  glBegin(GL_QUADS);
  glTexCoord2f(b,a); glVertex3i( 10,-10, 10);
  glTexCoord2f(b,b); glVertex3i( 10, 10, 10);
  glTexCoord2f(a,b); glVertex3i( 10, 10,-10);
  glTexCoord2f(a,a); glVertex3i( 10,-10,-10);
  glEnd();
  glBindTexture(GL_TEXTURE_2D,stback);
  glBegin(GL_QUADS);
  glTexCoord2f(a,a); glVertex3i(-10,-10,-10);
  glTexCoord2f(b,a); glVertex3i( 10,-10,-10);
  glTexCoord2f(b,b); glVertex3i( 10, 10,-10);
  glTexCoord2f(a,b); glVertex3i(-10, 10,-10);
  glEnd();
  glBindTexture(GL_TEXTURE_2D,stfront);
  glBegin(GL_QUADS);
  glTexCoord2f(b,a); glVertex3i(-10,-10, 10);
  glTexCoord2f(b,b); glVertex3i(-10, 10, 10);
  glTexCoord2f(a,b); glVertex3i( 10, 10, 10);
  glTexCoord2f(a,a); glVertex3i( 10,-10, 10);
  glEnd();
  glEndList();

  // Return the new call list
  return cl;
}

static void DrawGround()
{
  vector3_t  p0,p1,p2,p3,a,b,n;
  float      color[4],shine[1];
  int        i,j;

  static int         init=1,sb;
  static u32b_t      floor;
  static vector3_t   normals[63][63][2];
  static vector3_t   hnormals[63][63][2];
  static io_bitmap_t horizon;

  if( init ) {
    // Initialize if needed
    init = 0;
    // Load flor texture
    floor = LoadTexture("data/bmp/floor.bmp");
    // Compute normals for the heightmapped floor
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
	vector3_crossprod(&b, &a, &n);
	vector3_normalize(&n, &(normals[i][j][0]));
	vector3_sub_vector(&p0, &p2, &a);
	vector3_sub_vector(&p0, &p3, &b);
	vector3_crossprod(&b, &a, &n);
	vector3_normalize(&n, &(normals[i][j][1]));
      }
    }
    // Compute normals and load horizon heightmap
    io_bitmap_load("data/bmp/hhm.bmp", &horizon);
    if( (horizon.w != horizon.h) || (horizon.w != 64) ) {
      Error("Horizon heightmap bitmap must be 64x64 in size.\n");
    }
    for(i=0; i<63; i++) {
      for(j=0; j<63; j++) {
	// Get points from the loop and heightmap
	p0.s.x =  i/63.0f;
	p0.s.y =  horizon.d[(    i*64+    j)*3] / 255.0f / 4.0f;
	p0.s.z =  j/63.0f;
	p1.s.x =  (i+1)/63.0f;
	p1.s.y =  horizon.d[((i+1)*64+    j)*3] / 255.0f / 4.0f;
	p1.s.z =  j/63.0f;
	p2.s.x =  (i+1)/63.0f;
	p2.s.y =  horizon.d[((i+1)*64+(j+1))*3] / 255.0f / 4.0f;
	p2.s.z =  (j+1)/63.0f;
	p3.s.x =  i/63.0f;
	p3.s.y =  horizon.d[(    i*64+(j+1))*3] / 255.0f / 4.0f;
	p3.s.z =  (j+1)/63.0f;
	// Compute normals
	vector3_sub_vector(&p0, &p1, &a);
	vector3_sub_vector(&p0, &p2, &b);
	vector3_crossprod(&b, &a, &n);
	vector3_normalize(&n, &(hnormals[i][j][0]));
	vector3_sub_vector(&p0, &p2, &a);
	vector3_sub_vector(&p0, &p3, &b);
	vector3_crossprod(&b, &a, &n);
	vector3_normalize(&n, &(hnormals[i][j][1]));
      }
    }
    // Build the skybox call list
    glEnable(GL_TEXTURE_2D);
    sb = BuildSkyBox();
    glDisable(GL_TEXTURE_2D);
  }

  // Draw the skybox
  glEnable(GL_TEXTURE_2D);
  color[0] = 0.25f;
  color[1] = 0.25f;
  color[2] = 0.25f;
  color[3] = 1.0f;
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  color);
  glCallList(sb);
  glDisable(GL_TEXTURE_2D);

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
      // Set normals, texture coords, and verticies
      glNormal3f(normals[i][j][0].s.x, normals[i][j][0].s.y, normals[i][j][0].s.z);
      glTexCoord2f(     i/63.0f*4.0f,     j/63.0f*4.0f);  glVertex3f(p0.s.x, p0.s.y, p0.s.z);
      glTexCoord2f( (i+1)/63.0f*4.0f,     j/63.0f*4.0f);  glVertex3f(p1.s.x, p1.s.y, p1.s.z);
      glTexCoord2f( (i+1)/63.0f*4.0f, (j+1)/63.0f*4.0f);  glVertex3f(p2.s.x, p2.s.y, p2.s.z);
      glNormal3f(normals[i][j][1].s.x, normals[i][j][1].s.y, normals[i][j][1].s.z);
      glTexCoord2f( (i+1)/63.0f*4.0f, (j+1)/63.0f*4.0f);  glVertex3f(p2.s.x, p2.s.y, p2.s.z);
      glTexCoord2f(     i/63.0f*4.0f, (j+1)/63.0f*4.0f);  glVertex3f(p3.s.x, p3.s.y, p3.s.z);
      glTexCoord2f(     i/63.0f*4.0f,     j/63.0f*4.0f);  glVertex3f(p0.s.x, p0.s.y, p0.s.z);
    }
  }
  glEnd();

  // Draw the horizon
  color[0] = 0.5f;
  color[1] = 0.5f;
  color[2] = 0.5f;
  color[3] = 1.0f;
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  color);
  glBegin(GL_TRIANGLES);
  for(i=0; i<63; i++) {
    for(j=0; j<63; j++) {
      // Get points from the loop and heightmap
      p0.s.x =  i/63.0f * 25.0f - 12.5f;
      p0.s.y =  horizon.d[(    i*64+    j)*3] / 255.0f * 6.0f - .001f;
      p0.s.z =  j/63.0f * 25.0f - 12.5f;
      p1.s.x =  (i+1)/63.0f * 25.0f - 12.5f;
      p1.s.y =  horizon.d[((i+1)*64+    j)*3] / 255.0f * 6.0f - .001f;
      p1.s.z =  j/63.0f * 25.0f - 12.5f;
      p2.s.x =  (i+1)/63.0f * 25.0f - 12.5f;
      p2.s.y =  horizon.d[((i+1)*64+(j+1))*3] / 255.0f * 6.0f - .001f;
      p2.s.z =  (j+1)/63.0f * 25.0f - 12.5f;
      p3.s.x =  i/63.0f * 25.0f - 12.5f;
      p3.s.y =  horizon.d[(    i*64+(j+1))*3] / 255.0f * 6.0f - .001f;
      p3.s.z =  (j+1)/63.0f * 25.0f - 12.5f;
      // Set normals, texture coords, and verticies
      glNormal3f(hnormals[i][j][0].s.x, hnormals[i][j][0].s.y, hnormals[i][j][0].s.z);
      glTexCoord2f(     i/63.0f*96.0f,     j/63.0f*96.0f);  glVertex3f(p0.s.x, p0.s.y, p0.s.z);
      glTexCoord2f( (i+1)/63.0f*96.0f,     j/63.0f*96.0f);  glVertex3f(p1.s.x, p1.s.y, p1.s.z);
      glTexCoord2f( (i+1)/63.0f*96.0f, (j+1)/63.0f*96.0f);  glVertex3f(p2.s.x, p2.s.y, p2.s.z);
      glNormal3f(hnormals[i][j][1].s.x, hnormals[i][j][1].s.y, hnormals[i][j][1].s.z);
      glTexCoord2f( (i+1)/63.0f*96.0f, (j+1)/63.0f*96.0f);  glVertex3f(p2.s.x, p2.s.y, p2.s.z);
      glTexCoord2f(     i/63.0f*96.0f, (j+1)/63.0f*96.0f);  glVertex3f(p3.s.x, p3.s.y, p3.s.z);
      glTexCoord2f(     i/63.0f*96.0f,     j/63.0f*96.0f);  glVertex3f(p0.s.x, p0.s.y, p0.s.z);
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
  float r,color[4],white[4]={1.0f,1.0f,1.0f,1.0f},black[4]={0.0f,0.0f,0.0f,0.0f};
  int   i,x,y,slices=GGF_GEM_SLICES,stacks=GGF_GEM_STACKS;

  static int         init=1;
  static GLUquadric *qdrc;

  if(init) {
    // Init if needed
    init = 0;
    qdrc=gluNewQuadric();
  }

  for(i=0; i<Statec->player.ntowers; i++) {
    // Draw a small sphere at the center of the tower
    r = 2 / 255.0f;
    color[0] = (Statec->player.towers[i].gem.color.a[0])/255.0f;
    color[1] = (Statec->player.towers[i].gem.color.a[1])/255.0f;
    color[2] = (Statec->player.towers[i].gem.color.a[2])/255.0f;
    color[3] = 0.5f;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  color);
    color[0] = (Statec->player.towers[i].gem.color.a[0])/2/255.0f;
    color[1] = (Statec->player.towers[i].gem.color.a[1])/2/255.0f;
    color[2] = (Statec->player.towers[i].gem.color.a[2])/2/255.0f;
    color[3] = 0.001f;
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  color);
    glPushMatrix();
    x = (Statec->player.towers[i].position.s.x)/255.0f * 63.0f;
    y = (Statec->player.towers[i].position.s.y)/255.0f * 63.0f;
    glTranslatef((Statec->player.towers[i].position.s.x)/255.0f,
		 Statec->terrain.d[(x*64+y)*3] / 255.0f / 4.0f + 0.025,
		 (Statec->player.towers[i].position.s.y)/255.0f);
    gluSphere(qdrc, r, slices , stacks);
    glPopMatrix();
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);
    // Draw the stone tower stand
    glPushMatrix();
    glTranslatef((Statec->player.towers[i].position.s.x)/255.0f,
		 Statec->terrain.d[(x*64+y)*3] / 255.0f / 4.0f + 0.025,
		 (Statec->player.towers[i].position.s.y)/255.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    color[0] = 0.5f;
    color[1] = 0.5f;
    color[2] = 0.5f;
    color[3] = 1.0f;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  color);
    gluCylinder(qdrc, r/3.0f*1.0f, r/3.0f*2.0f, 0.035, slices, stacks);
    glPopMatrix();
  }
}

static void DrawEnemies()
{
  float r,color[4],white[4]={1.0f,1.0f,1.0f,1.0f},black[4]={0.0f,0.0f,0.0f,0.0f};
  int   i,x,y,slices=16,stacks=16;

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
    r = log( enemy_get_enemy_xp(&(Statec->enemies[i])) ) / 4.0f / 255.0f;
    color[0] = (Statec->enemies[i].color.a[0])/255.0f;
    color[1] = (Statec->enemies[i].color.a[1])/255.0f;
    color[2] = (Statec->enemies[i].color.a[2])/255.0f;
    color[3] = 0.9f;
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  color);
    color[0] = (Statec->enemies[i].color.a[0])/255.0f;
    color[1] = (Statec->enemies[i].color.a[1])/255.0f;
    color[2] = (Statec->enemies[i].color.a[2])/255.0f;
    color[3] = 0.1f;
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color);
    glPushMatrix();
    x = (Statec->enemies[i].position.s.x)/255.0f * 63.0f;
    y = (Statec->enemies[i].position.s.y)/255.0f * 63.0f;
    glTranslatef((Statec->enemies[i].position.s.x)/255.0f,
		 Statec->terrain.d[(x*64+y)*3] / 255.0f / 4.0f + 0.005,
		 (Statec->enemies[i].position.s.y)/255.0f);
    gluSphere(qdrc, r, slices, stacks);
    glPopMatrix();
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);
  }
}


static void DrawEvents(widget_t *w)
{
  eventq_t *q,*t;
  float     h,r,color[4];
  float     white[4]={1.0f,1.0f,1.0f,1.0f},black[4]={0.0f,0.0f,0.0f,0.0f};
  int       i,x,y,slices=GGF_GEM_SLICES,stacks=GGF_GEM_STACKS; 
  GLfloat   light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  GLfloat   light_color[]    = { 1.0f, 1.0f, 1.0f, 1.0f };

  static int         init=1;
  static GLUquadric *qdrc;

  if( init ) {
    // Init if needed
    init = 0;
    // For drawing 3D "primitives"
    qdrc=gluNewQuadric();
  }

  // Draw everything on the event list
  for(q=gui_game_event_get(NULL); q; q=gui_game_event_get(q)) {
    // Find event type
    switch(q->type) {
    case GUI_GAME_EVENT_FIRE:
      // Tower fire event
      if( !q->flags ) {
	alSourcePlay(AL_FIRE);
	q->flags = 1;
      }
      // Draw laser attack beam
      glEnable(GL_LINE_SMOOTH);
      color[0] = (q->fire.color.a[0])/256.0f;
      color[1] = (q->fire.color.a[1])/256.0f;
      color[2] = (q->fire.color.a[2])/256.0f;
      color[3] = (0.75f * GGF_FIRE_TIME / (Statec->time - q->time)) +
                 (0.10f * (1.0f - GGF_FIRE_TIME / (Statec->time - q->time)));
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
      // Light-up the "gem" in the tower
      glPushMatrix();
      r = 2 / 255.0f;
      x = (q->fire.tower.s.x)/255.0f * 63.0f;
      y = (q->fire.tower.s.y)/255.0f * 63.0f;
      h = Statec->terrain.d[(x*64+y)*3] / 255.0f / 4.0f + 0.025;
      glTranslatef((q->fire.tower.s.x)/255.0f,
		   h,
		   (q->fire.tower.s.y)/255.0f );
      gluSphere(qdrc, r, slices , stacks);
      glPopMatrix();
      // Keep a light on if new enough
      if( (Statec->time - q->time) < GGF_FIRE_LIGHT_TIME ) {
	if( gl_nlights < gl_maxlights ) {
	  light_position[0] = (q->fire.tower.s.x)/255.0f;
	  light_position[1] = h + 0.01;
	  light_position[2] = (q->fire.tower.s.y)/255.0f;
	  light_position[3] = 1.0f;
	  light_color[0] = (q->fire.color.a[0])/256.0f;
	  light_color[1] = (q->fire.color.a[1])/256.0f;
	  light_color[2] = (q->fire.color.a[2])/256.0f;
	  light_color[3] = 0.5f * (1.0f-(Statec->time-q->time)/GGF_FIRE_LIGHT_TIME);
	  glLightfv(gl_lights[gl_nlights], GL_POSITION, light_position );
	  glLightfv(gl_lights[gl_nlights], GL_AMBIENT,  black );
	  glLightfv(gl_lights[gl_nlights], GL_SPECULAR, black );
	  glLightfv(gl_lights[gl_nlights], GL_DIFFUSE,  light_color );
	  glLightf(gl_lights[gl_nlights], GL_CONSTANT_ATTENUATION,  8.0f);
	  glLightf(gl_lights[gl_nlights], GL_LINEAR_ATTENUATION,    8.0f);
	  glLightf(gl_lights[gl_nlights], GL_QUADRATIC_ATTENUATION, 1.5f);
	  glEnable(gl_lights[gl_nlights]);
	  gl_nlights++;
	}
      }
      // Draw the damage text hover label
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
      if( (Statec->time - q->time) > GGF_FIRE_TIME ) {
	t = q->last;
	gui_game_event_remove(q);
	q = t;
      }
      break;
    case GUI_GAME_EVENT_KILL:
      // Enemy death event
      if( !q->flags ) {
	alSourcePlay(AL_KILL);
	q->flags = 1;
      }
      // Draw "cross"
      glEnable(GL_LINE_SMOOTH);
      color[0] = (1.0f * GGF_KILL_TIME / (Statec->time - q->time)) +
                 (0.5f * (1.0f - GGF_KILL_TIME / (Statec->time - q->time)));
      color[1] = 0.0f;
      color[2] = 0.0f;
      color[3] = (0.500f * GGF_KILL_TIME / (Statec->time - q->time)) +
                 (0.001f * (1.0f - GGF_KILL_TIME / (Statec->time - q->time)));
      glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  color);
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color);
      x = (q->kill.enemy.s.x)/255.0f * 63.0f;
      y = (q->kill.enemy.s.y)/255.0f * 63.0f;
      h = Statec->terrain.d[(x*64+y)*3] / 255.0f / 4.0f + 0.005;
      r = 4.0f * (1.0f-(Statec->time-q->time)/GGF_KILL_TIME);
      for(i=0; i<3; i++) {
	glPushMatrix();
	glTranslatef(q->kill.enemy.s.x/255.0f, h, q->kill.enemy.s.y/255.0f);
	if( i == 0 ) {
	  glRotatef(i*20.0f*(1.0f * GGF_KILL_TIME / (Statec->time - q->time)), 1.0f, 0.0f, 0.0f);
	} else if( i == 1 ) {
	  glRotatef(i*30.0f*(1.0f * GGF_KILL_TIME / (Statec->time - q->time)), 0.0f, 1.0f, 0.0f);
	} else {
	  glRotatef(i*20.0f*(1.0f * GGF_KILL_TIME / (Statec->time - q->time)), 0.0f, 0.0f, 1.0f);
	}
	glTranslatef(-q->kill.enemy.s.x/255.0f, -h, -q->kill.enemy.s.y/255.0f);
	glBegin(GL_LINES); 
	glVertex3f((q->kill.enemy.s.x-r)/255.0f, h, (q->kill.enemy.s.y-r)/255.0f);
	glVertex3f((q->kill.enemy.s.x+r)/255.0f, h, (q->kill.enemy.s.y+r)/255.0f);
	glVertex3f((q->kill.enemy.s.x+r)/255.0f, h, (q->kill.enemy.s.y-r)/255.0f);
	glVertex3f((q->kill.enemy.s.x-r)/255.0f, h, (q->kill.enemy.s.y+r)/255.0f);
	glVertex3f((q->kill.enemy.s.x)/255.0f, h-r/255.0f, (q->kill.enemy.s.y)/255.0f);
	glVertex3f((q->kill.enemy.s.x)/255.0f, h+r/255.0f, (q->kill.enemy.s.y)/255.0f);
	glEnd();
	glPopMatrix();
      }
      glDisable(GL_LINE_SMOOTH);
      // Draw a small sphere at center
      glPushMatrix();
      r = 1.25f / 255.0f * (1.0f-(Statec->time-q->time)/GGF_KILL_TIME);
      x = (q->kill.enemy.s.x)/255.0f * 63.0f;
      y = (q->kill.enemy.s.y)/255.0f * 63.0f;
      h = Statec->terrain.d[(x*64+y)*3] / 255.0f / 4.0f + 0.005;
      glTranslatef((q->kill.enemy.s.x)/255.0f,
		   h,
		   (q->kill.enemy.s.y)/255.0f );
      gluSphere(qdrc, r, slices , stacks);
      glPopMatrix();
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);
      // Keep a light on if new enough
      if( (Statec->time - q->time) < GGF_KILL_LIGHT_TIME ) {
	if( gl_nlights < gl_maxlights ) {
	  light_position[0] = (q->kill.enemy.s.x)/255.0f;
	  light_position[1] = h + 0.01;
	  light_position[2] = (q->kill.enemy.s.y)/255.0f;
	  light_position[3] = 1.0f;
	  light_color[0] = 1.0f * (1.0f-(Statec->time-q->time)/GGF_KILL_LIGHT_TIME);
	  light_color[1] = 0.0f;
	  light_color[2] = 0.0f;
	  light_color[3] = 1.0f;
	  glLightfv(gl_lights[gl_nlights], GL_POSITION, light_position );
	  glLightfv(gl_lights[gl_nlights], GL_AMBIENT,  black );
	  glLightfv(gl_lights[gl_nlights], GL_SPECULAR, black );
	  glLightfv(gl_lights[gl_nlights], GL_DIFFUSE,  light_color );
	  glLightf(gl_lights[gl_nlights], GL_CONSTANT_ATTENUATION,  1.0f);
	  glLightf(gl_lights[gl_nlights], GL_LINEAR_ATTENUATION,    0.2f);
	  glLightf(gl_lights[gl_nlights], GL_QUADRATIC_ATTENUATION, 0.4f);
	  glEnable(gl_lights[gl_nlights]);
	  gl_nlights++;
	}
      }
      // Remove if needed
      if( (Statec->time - q->time) > GGF_KILL_TIME ) {
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
  int i;

  GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  GLfloat specular_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  GLfloat diffuse_color[]  = { 0.7f, 0.7f, 0.7f, 1.0f };
  GLfloat ambient_color[]  = { 0.1f, 0.1f, 0.1f, 1.0f };

  static int init=1;

  // Init if needed
  if( init == 1 ){
    init = 0;
    // Setup max light sources
    glGetIntegerv(GL_MAX_LIGHTS, &gl_maxlights);
    if( gl_maxlights > 128 ) {
      gl_maxlights = 128;
    }
    gl_lights = malloc(gl_maxlights*sizeof(int));
    for(i=0; i<gl_maxlights; i++) {
      gl_lights[i] = GL_LIGHT0+i;
    }
  }

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
  if( gf->rotx < 1.0f ) {
    gf->rotx = 1.0f;
  }
  if( gf->rotx > 90.0f ) {
    gf->rotx = 90.0f;
  }
  glRotatef(gf->rotx + 5.0f, 1.0f, 0.0f, 0.0f);
  glRotatef(gf->roty, 0.0f, 1.0f, 0.0f);
  glTranslatef(-0.5f, 0.0f, -0.5f);

  // Draw / handle game events
  DrawEvents(w);

  // Draw ground
  DrawGround();

  // Draw towers
  DrawTowers();

  // Draw enemy path
  DrawPath();

  // Draw enemies
  DrawEnemies();

  // Disable lighting
  for(i=1; i<gl_nlights; i++) {
    glDisable(gl_lights[i]);
  }
  gl_nlights=1;
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
      if( gf->zoom > 1.25 ) {
	gf->zoom = 1.25;
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
