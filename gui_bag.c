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
#include "gui_bag.h"

////////////////////////////////////////////////////////////////////////////////

// Button callbacks

void Bag_Down(widget_t *w, const int x, const int y, const int b)
{
  if( (x > ScaleX(w,w->x)) && (x < ScaleX(w,w->x+w->w)) && 
      (y > ScaleY(w,w->y)) && (y < ScaleY(w,w->y+w->h))     ) {
    //GuiExit();
  }
}

////////////////////////////////////////////////////////////////////////////////

void Bag_Draw(widget_t *w)
{
  //bag_gui_t *gf = (bag_gui_t*)w->wd;

  u32b_t i,j,x,y;
  double r,ratio=w->w/((double)w->h);

  // Draw the items
  for(i=0; i<(sizeof(Statec->player.bag.items)/sizeof(item_t)); i++) {
    // Find position of items
    x = i%4;
    y = i/4;
    // Figure out item type
    switch(Statec->player.bag.items[i].type) {
    case BAG_ITEM_TYPE_GEM:
      // Gem border
      glBegin(GL_POLYGON);   
      glColor3f(1.0f, 1.0f, 1.0f); 
      r = 20 / 255.0;
      for(j=0; j<6; ) {
	glVertex3f((x)/3.0+r*cos(2*3.14159265*(j/5.0)), 
		   (y)/15.0+r*ratio*sin(2*3.14159265*(j/5.0)), 1.0f );
	j++;
	glVertex3f((x)/3.0+r*cos(2*3.14159265*(j/5.0)), 
		   (y)/15.0+r*ratio*sin(2*3.14159265*(j/5.0)), 1.0f );
      }
      glEnd();
      glBegin(GL_POLYGON);
      // Tower center
      glColor3f( (Statec->player.bag.items[i].gem.color.a[0])/255.0, 
		 (Statec->player.bag.items[i].gem.color.a[1])/255.0,
		 (Statec->player.bag.items[i].gem.color.a[2])/255.0  );
      r = 16 / 255.0;
      for(j=0; j<6; ) {
	glVertex2f((x)/3.0+r*cos(2*3.14159265*(j/5.0)), 
		   (y)/15.0+r*ratio*sin(2*3.14159265*(j/5.0))  );
	j++;
	glVertex2f((x)/3.0+r*cos(2*3.14159265*(j/5.0)), 
		   (y)/15.0+r*ratio*sin(2*3.14159265*(j/5.0))  );
      }
      glEnd();
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

#endif // !GUI_BAG_C
