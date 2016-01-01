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

// Button callbacks

void Bag_Down(widget_t *w, const int x, const int y, const int b)
{
  bag_gui_t *bag = (bag_gui_t*)(w->wd);
  gem_t     *gem = NULL;
  int        i,ndx;
  float      xf,yf,xs,ys,d,md = 1000000.0f;

  if( Statec->player.mana < 0 ) {
    return;
  }

  if( (x > ScaleX(w,w->x)) && (x < ScaleX(w,w->x+w->w)) && 
      (y > ScaleY(w,w->y)) && (y < ScaleY(w,w->y+w->h))     ) {
    switch(b) {
    case MOUSE_LEFT:
      if( *(bag->mix_btn_link) == 1 ) {
	// Mix button is selected..
	if( GuiState.mouse_item_ndx != -1 ) {
	  // Find the closest gem to the mouse..
	  for(i=0; i<(sizeof(Statec->player.bag.items)/sizeof(item_t)); i++) {
	    if( i != GuiState.mouse_item_ndx ) {
	      switch(Statec->player.bag.items[i].type) {
	      case BAG_ITEM_TYPE_GEM:
		// Find position of the gem
		xf  = i%3 + (2.0f/(3.0f*2.0f)); 
		xf /= 3.0f;
		yf  = i/3 + (2.0f/(3.0f*2.0f)); 
		yf /= 15.0f;
		xs  = ScaleX(w,xf*w->w+w->x);
		ys  = ScaleY(w,yf*w->h+w->y);
		// Get distance to mouse
		d = sqrt((xs-x)*(xs-x) + (ys-y)*(ys-y));
		if( d < md ) {
		  ndx = i;
		  md  = d;
		  gem = &(Statec->player.bag.items[i].gem);
		}
		break;
	      }
	    }
	  }
	  if( gem ) {
	    // Add a game event to mix the gems.
	    game_event_mix_gems(ndx, GuiState.mouse_item_ndx);
	    // Disable mix button selection.
	    *(bag->mix_btn_link) = 0;
	    // Disable mouse/bag selection.
	    GuiState.mouse_item_ndx = -1;
	  }
	}
      } else {
	// No modifiers, do a regular select: find the closest gem to the mouse..
	for(i=0; i<(sizeof(Statec->player.bag.items)/sizeof(item_t)); i++) {
	  if( i != GuiState.mouse_item_ndx ) {
	    switch(Statec->player.bag.items[i].type) {
	    case BAG_ITEM_TYPE_GEM:
	      // Find position of the gem
	      xf  = i%3 + (2.0f/(3.0f*2.0f)); 
	      xf /= 3.0f;
	      yf  = i/3 + (2.0f/(3.0f*2.0f)); 
	      yf /= 15.0f;
	      xs  = ScaleX(w,xf*w->w+w->x);
	      ys  = ScaleY(w,yf*w->h+w->y);
	      // Get distance to mouse
	      d = sqrt((xs-x)*(xs-x) + (ys-y)*(ys-y));
	      if( d < md ) {
		ndx = i;
		md  = d;
		gem = &(Statec->player.bag.items[i].gem);
	      }
	      break;
	    }
	  }
	}
	// Pick up the gem.
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

// Key callbacks

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
  u32b_t i,x,y;
  double xf,yf,ratio=w->w/((double)w->h);

  // Draw the items
  for(i=0; i<(sizeof(Statec->player.bag.items)/sizeof(item_t)); i++) {
    // Find position of items
    x  = i%3;
    xf = x + (2.0f/(3.0f*2.0f)); 
    xf /= 3.0;
    y  = i/3;
    yf = y + (2.0f/(3.0f*2.0f)); 
    yf /= 15.0;
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
}

////////////////////////////////////////////////////////////////////////////////

void Bag_MouseMove(widget_t *w, int x, int y)
{
  if( Statec->player.mana < 0 ) {
    return;
  }
}

#endif // !GUI_BAG_C
