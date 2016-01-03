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
#include "color.h"
#define GUI_WIDGET
#include "gui.h"
#undef GUI_WIDGET
#include "gui_bag.h"

////////////////////////////////////////////////////////////////////////////////

static gem_t* bag_get_nearest_gem(widget_t *w, int x, int y, int *ndx)
{
  gem_t *gem = NULL;
  float xf,yf,xs,ys,d,md = 1000000.0f;
  int   i;

  // Find the closest gem to the mouse..
  *ndx = -1;
  for(i=0; i<(sizeof(Statec->player.bag.items)/sizeof(item_t)); i++) {
    // Skip the current selection.
    if( i != GuiState.mouse_item_ndx ) {
      switch(Statec->player.bag.items[i].type) {
      case BAG_ITEM_TYPE_GEM:
        // Find position of the gem
        xf = i%3;
        xf = xf/3.0;
        xf = xf + (1.0f/6.0f); 
        yf = i/3;
        yf = yf/15.0;
        yf = yf + (1.0f/30.0f); 
        xs  = ScaleX(w,xf*w->w+w->x);
        ys  = ScaleY(w,yf*w->h+w->y);
        // Get distance to mouse
        d = sqrt((xs-x)*(xs-x) + (ys-y)*(ys-y));
        if( d < md && ISCLOSE(d) ) {
          *ndx = i;
          md   = d;
          gem  = &(Statec->player.bag.items[i].gem);
        }
        break;
      }
    }
  }

  // Return the gem if we got one.
  return gem;
}

////////////////////////////////////////////////////////////////////////////////
// Event Callbacks
////////////////////////////////////////////////////////////////////////////////

//
// Mouse callbacks
//

void Bag_Down(widget_t *w, const int x, const int y, const int b)
{
  gem_t     *gem = NULL;
  int        ndx;

  if( Statec->player.mana < 0 ) {
    return;
  }

  if( (x > ScaleX(w,w->x)) && (x < ScaleX(w,w->x+w->w)) && 
      (y > ScaleY(w,w->y)) && (y < ScaleY(w,w->y+w->h))     ) {
    switch(b) {
    case MOUSE_LEFT:
      // If we have a gem selected, and we are close to another gem, mix them.
      // Otherwise, add the selected gem back into the bag.
      if ( GuiState.mouse_item_ndx != -1 ) {
        gem = bag_get_nearest_gem(w,x,y,&ndx);
        if ( gem ) {
          // Add a game event to mix the gems.
          game_event_mix_gems(ndx, GuiState.mouse_item_ndx);
        }
        // Always deselect the gem, even if we didn't mix
        GuiState.mouse_item_ndx = -1;
      } else {
        // No gem selected, do a regular select: find the closest gem to the mouse..
        gem = bag_get_nearest_gem(w,x,y,&ndx);
        if( gem ) {
          // Pick up gem into mouse (escape key / other event will drop)
          GuiState.mouse_item_ndx = ndx;
        }
      }
      break;
    case MOUSE_RIGHT:
      // Clear the bag mouse selection.
      GuiState.mouse_item_ndx = -1;
      break;
    }
  }
}

void Bag_MouseMove(widget_t *w, int x, int y)
{
  //gem_t     *gem = NULL;
  //int        ndx;

  if( Statec->player.mana < 0 ) {
    return;
  }
}

//
// Key callbacks
//

void Bag_KeyPress(widget_t *w, char key, unsigned int keycode)
{
  if( Statec->player.mana < 0 ) {
    return;
  }

  if( GuiState.mouse_item_ndx != -1 ) {
    GuiState.mouse_item_ndx = -1;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Draw routines.
////////////////////////////////////////////////////////////////////////////////

static int BuildBagBG()
{
  int   bgtex,cl,i;
  float a,b;

  // I use these values to slightly tweak the textures positions.
  a = 0.0f;
  b = 1.0f;

  // I guess we can load this here..
  bgtex = LoadTexture("data/bmp/BagBG.bmp"); 

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

  // Draw an item grid.
  glDisable(GL_TEXTURE_2D);
  glColor4ub(0,0,0,96);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  for(i=0; i<2; i++) {
    glBegin(GL_QUADS);
    glVertex2f((1.0/3.0)*(i+1)+0.015, 0.0);
    glVertex2f((1.0/3.0)*(i+1)-0.015, 0.0);
    glVertex2f((1.0/3.0)*(i+1)-0.015, 1.0);
    glVertex2f((1.0/3.0)*(i+1)+0.015, 1.0);
    glEnd();   
  }
  for(i=0; i<14; i++) {
    glBegin(GL_QUADS);
    glVertex2f(0.0, (1.0/15.0)*(i+1)+0.005);
    glVertex2f(0.0, (1.0/15.0)*(i+1)-0.005);
    glVertex2f(1.0, (1.0/15.0)*(i+1)-0.005);
    glVertex2f(1.0, (1.0/15.0)*(i+1)+0.005);
    glEnd();   
  }
  glDisable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);

  // End the list.
  glEndList();

  // Return the new call list
  return cl;
}


void Gem_Draw(gem_t *gem, double xf, double yf, double ratio)
{
  double r;
  int    j;

  // Gem border
  glBegin(GL_POLYGON);   
  glColor3f(1.0f, 1.0f, 1.0f); 
  r = 20 / 255.0;
  for(j=0; j<6; ) {
    glVertex3f(xf+r*cos(2*3.14159265*(j/5.0)), 
	       yf+r*ratio*sin(2*3.14159265*(j/5.0)), 1.0f );
    j++;
    glVertex3f(xf+r*cos(2*3.14159265*(j/5.0)), 
	       yf+r*ratio*sin(2*3.14159265*(j/5.0)), 1.0f );
  }
  glEnd();
  glBegin(GL_POLYGON);
  // Gem center
  glColor3f( (gem->color.a[0])/255.0, 
	     (gem->color.a[1])/255.0,
	     (gem->color.a[2])/255.0  );
  r = 16 / 255.0;
  for(j=0; j<6; ) {
    glVertex2f(xf+r*cos(2*3.14159265*(j/5.0)), 
	       yf+r*ratio*sin(2*3.14159265*(j/5.0))  );
    j++;
    glVertex2f(xf+r*cos(2*3.14159265*(j/5.0)), 
	       yf+r*ratio*sin(2*3.14159265*(j/5.0))  );
  }
  glEnd();
}

void Bag_Draw(widget_t *w)
{
  static int bg  = 0;
  u32b_t     i,x,y;
  double     xf,yf,ratio=w->w/((double)w->h);
  gem_t     *gem = NULL;
  int        ndx;

  // Init if needed.. I guess this can be here.
  if( bg == 0 ) {
    glEnable(GL_TEXTURE_2D);
    bg = BuildBagBG();
    glDisable(GL_TEXTURE_2D);
  }

  // Draw background.
  glEnable(GL_TEXTURE_2D);
  White();
  glCallList(bg);
  glDisable(GL_TEXTURE_2D);

  // Draw the items
  for(i=0; i<(sizeof(Statec->player.bag.items)/sizeof(item_t)); i++) {
    // Find position of items
    x  = i%3;
    xf = x/3.0;
    xf = xf + (1.0f/6.0f); 
    y  = i/3;
    yf = y/15.0;
    yf = yf + (1.0f/30.0f); 
    // Figure out item type
    switch(Statec->player.bag.items[i].type) {
    case BAG_ITEM_TYPE_GEM:
      if( i != GuiState.mouse_item_ndx ) {
	Gem_Draw(&(Statec->player.bag.items[i].gem),xf,yf,ratio);
      } else {
      }
      break;
    }
  }

  // Outline
  Yellow();
  glBegin(GL_LINE_LOOP);
  glVertex2f(0.0f,0.0f);
  glVertex2f(0.0f,1.0f);
  glVertex2f(1.0f,1.0f);
  glVertex2f(1.0f,0.0f);
  glEnd();

  // Only if no gems selected.
  if( GuiState.mouse_item_ndx == -1 ) {
    if( (GuiState.hand_pos.s.x > ScaleX(w,w->x)) && (GuiState.hand_pos.s.x < ScaleX(w,w->x+w->w)) && 
	(GuiState.hand_pos.s.y > ScaleY(w,w->y)) && (GuiState.hand_pos.s.y < ScaleY(w,w->y+w->h))     ) {
      // Find nearest gem.
      gem = bag_get_nearest_gem(w,GuiState.hand_pos.s.x,GuiState.hand_pos.s.y,&ndx);
      if( gem ) {
	// Highlight the gem.
	xf = ndx%3;
	yf = ndx/3;
	glColor3ub(128,128,0);
	glBegin(GL_LINE_LOOP);
	glVertex2f(xf/3.0f,     yf/15.0f);
	glVertex2f(xf/3.0f,     (yf+1)/15.0f);
	glVertex2f((xf+1)/3.0f, (yf+1)/15.0f);
	glVertex2f((xf+1)/3.0f, yf/15.0f);
	glEnd();
	// Draw the hover box for the current hover gem.
	glPushMatrix();
	glLoadIdentity();
	xf = GuiState.hand_pos.s.x;
	yf = GuiState.hand_pos.s.y;
	if( xf+64 > w->glw->width ) {
	  xf -= (xf+64) - w->glw->width;
	}
	if( yf+64 > w->glw->height ) {
	  yf -= (yf+64) - w->glw->height;
	}
	glTranslatef(xf, yf, 0.0f);
	// Background
	glColor4ub(0,0,0,220);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_POLYGON);
	glVertex2f(0.0f,0.0f);
	glVertex2f(0.0f,64.0f);
	glVertex2f(64.0f,64.0f);
	glVertex2f(64.0f,0.0f);
	glEnd();
	glDisable(GL_BLEND);
	// Text
	Red();
	glRasterPos2f(strlen("r: ")*6.0f/2.0f, 16.0f);
	printGLf(w->glw->font,"r: %.1lf",gem->color.s.x);
	Green();
	glRasterPos2f(strlen("g: ")*6.0f/2.0f, 16.0f+16.0f*1);
	printGLf(w->glw->font,"g: %.1lf",gem->color.s.y);
	Blue();
	glRasterPos2f(strlen("b: ")*6.0f/2.0f, 16.0f+16.0f*2);
	printGLf(w->glw->font,"b: %.1lf",gem->color.s.z);
	// Outline
	Yellow();
	glBegin(GL_LINE_LOOP);
	glVertex2f(0.0f,0.0f);
	glVertex2f(0.0f,64.0f);
	glVertex2f(64.0f,64.0f);
	glVertex2f(64.0f,0.0f);
	glEnd();
	glPopMatrix();
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

#endif // !GUI_BAG_C
