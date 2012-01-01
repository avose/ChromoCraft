#include "processedparams.h"
#if P_GUI

#ifndef GUI_DEME_C
#define GUI_DEME_C

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

#include "anolis.h"
#include "random.h"
#include "types.h"
#include "util.h"
#define GUI_WIDGET
#include "gui.h"
#undef GUI_WIDGET
#include "gui_deme.h"
#include "ps.h"
#include "stats.h"

////////////////////////////////////////////////////////////////////////////////

void Demes_PostScript(char *fn);

static float LightAmbient[]  = { 0.3f, 0.3f, 0.3f, 1.0f };
static float LightDiffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
static float LightPosition[] = { 1.0f, 1.0f, 0.5f, 1.0f };

////////////////////////////////////////////////////////////////////////////////

// Following code taken from OpenGL redbook
#define X .525731112119133606 
#define Z .850650808352039932
static GLfloat vdata[12][3] = {    
  {-X, 0.0, Z}, {X, 0.0, Z}, {-X, 0.0, -Z}, {X, 0.0, -Z},    
  {0.0, Z, X}, {0.0, Z, -X}, {0.0, -Z, X}, {0.0, -Z, -X},    
  {Z, X, 0.0}, {-Z, X, 0.0}, {Z, -X, 0.0}, {-Z, -X, 0.0} 
};
#undef X
#undef Y

static GLuint tindices[20][3] = { 
  {0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1},    
  {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},    
  {7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6}, 
  {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11} };

static void normalize(GLfloat *a) {
  GLfloat d=sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
  a[0]/=d; a[1]/=d; a[2]/=d;
}

static void drawtri(GLfloat *a, GLfloat *b, GLfloat *c, int div, float r) {
  GLfloat ab[3], ac[3], bc[3];
  int     i;

  if (div<=0) {
    glNormal3fv(a); glVertex3f(a[0]*r, a[1]*r, a[2]*r);
    glNormal3fv(b); glVertex3f(b[0]*r, b[1]*r, b[2]*r);
    glNormal3fv(c); glVertex3f(c[0]*r, c[1]*r, c[2]*r);
  } else {
    for (i=0;i<3;i++) {
      ab[i]=(a[i]+b[i])/2;
      ac[i]=(a[i]+c[i])/2;
      bc[i]=(b[i]+c[i])/2;
    }
    normalize(ab); normalize(ac); normalize(bc);
    drawtri(a, ab, ac, div-1, r);
    drawtri(b, bc, ab, div-1, r);
    drawtri(c, ac, bc, div-1, r);
    drawtri(ab, bc, ac, div-1, r);  //<--Comment this line and sphere looks really cool!
  }  
}

static void DrawSphere(double x, double y, double z, double radius) {
  int ndiv = 8;
  int i;

  glPushMatrix();
  glTranslatef(x,y,z);
  glBegin(GL_TRIANGLES);
  for(i=0;i<20;i++) {
    drawtri(vdata[tindices[i][0]], vdata[tindices[i][1]], vdata[tindices[i][2]], ndiv, radius);
  }
  glEnd();
  glPopMatrix();
}

////////////////////////////////////////////////////////////////////////////////

static void VecCross(vec3f_t *a, vec3f_t *b, vec3f_t *d)
{
  d->x = a->y * b->z - a->z * b->y;
  d->y = a->z * b->x - a->x * b->z;
  d->z = a->x * b->y - a->y * b->x;
}

static void VecSub3fv(float ax, float ay, float az, float bx, float by, float bz, vec3f_t *d)
{
  d->x = ax - bx;
  d->y = ay - by;
  d->z = az - bz;
}

static void VecAdd(vec3f_t *a, vec3f_t *b, vec3f_t *d)
{
  d->x = a->x + b->x;
  d->y = a->y + b->y;
  d->z = a->z + b->z;
}

static void VecDiv(vec3f_t *v, float s)
{
  v->x /= s;
  v->y /= s;
  v->z /= s;
}

////////////////////////////////////////////////////////////////////////////////
// Within-Deme graph
//
// This gives a surface graph of a property of a single deme, where the x-axis
// and the z-axis are environment height and width, respectively.
////////////////////////////////////////////////////////////////////////////////

void FillHeightMap(widget_t *w, float **hm, char *title, u32b_t *xr, u32b_t *yr)
{
  u32b_t x,y,xd,yd,i,n,hmdm;  static u32b_t **hmd;  gind_t *ind;  double *xp,*yp;  char *t1=NULL,*t2=NULL;

  deme_gui_t  *d  = (deme_gui_t*)w->wd;
  demes_gui_t *ds = (demes_gui_t*)d->dc;

  // Malloc space for the heightmap density
  if( !hmd ) {
    u32b_t *tp;

    x = maxi(P_LX*2+1, P_LY*2+1);  y = maxi(P_LX*2+1, P_LY*2+1);
    x = maxi(       x, P_LZ*2+1);  y = maxi(       y, P_LZ*2+1);
    x = maxi(       x, P_LC*2+1);  y = maxi(       y, P_LC*2+1);
    x = maxi(       x, P_LM*2+1);  y = maxi(       y, P_LM*2+1);
    x = maxi(       x, P_LF*2+1);  y = maxi(       y, P_LF*2+1);
    
    if( !(tp = malloc(x*y*sizeof(u32b_t))) ) Error("malloc(hmd) failed!");
    if( !(hmd = malloc(x*sizeof(u32b_t*))) ) Error("malloc(hmd) failed!");
    for(i=0; i<x; i++)
      hmd[i] = &tp[i*y];
  }

  // Create title
  switch( d->xmode ) {
  case DM_X0: { t1="x0";  *xr = P_LX*2+1;  break; }
  case DM_X1: { t1="x1";  *xr = P_LX*2+1;  break; }
  case DM_Y0: { t1="y0";  *xr = P_LY*2+1;  break; }
  case DM_Y1: { t1="y1";  *xr = P_LY*2+1;  break; }
  case DM_Z0: { t1="z0";  *xr = P_LZ*2+1;  break; }
  case DM_Z1: { t1="z1";  *xr = P_LZ*2+1;  break; }
  case DM_C:  { t1="c";   *xr = P_LC*2+1;  break; }
  case DM_M:  { t1="m";   *xr = P_LM*2+1;  break; }
  case DM_F:  { t1="f";   *xr = P_LF*2+1;  break; }
  // This will keep the compiler quiet and force a segfault
  default:    { *((u8b_t*)0) = 0; *xr = 0; }
  }
  switch( d->ymode ) {
  case DM_X0: { t2="x0";  *yr = P_LX*2+1;  break; }
  case DM_X1: { t2="x1";  *yr = P_LX*2+1;  break; }
  case DM_Y0: { t2="y0";  *yr = P_LY*2+1;  break; }
  case DM_Y1: { t2="y1";  *yr = P_LY*2+1;  break; }
  case DM_Z0: { t2="z0";  *yr = P_LZ*2+1;  break; }
  case DM_Z1: { t2="z1";  *yr = P_LZ*2+1;  break; }
  case DM_C:  { t2="c";   *yr = P_LC*2+1;  break; }
  case DM_M:  { t2="m";   *yr = P_LM*2+1;  break; }
  case DM_F:  { t2="f";   *yr = P_LF*2+1;  break; }
  // This will keep the compiler quiet and force a segfault
  default:    { *((u8b_t*)0) = 0; *yr = 0; }
  }
  // Complete the full title
  sprintf(title,"Density: %s, %s",t1,t2);

  // Allocate surface map
  for(x=0; x<*xr; x++) {
    memset( hm[x], 0, *yr*sizeof(float) );
    memset(hmd[x], 0, *yr*sizeof(u32b_t));
  }
  hmdm = 0;

  pthread_mutex_lock(&StateLock);
  if( (ds->x == -1) || (ds->y == -1) ) {
    // If the selected deme (ds->x,ds->y) is -1, combine all demes
    for(xd=0; xd < P_WIDTH; xd++) {
      for(yd=0; yd < P_HEIGHT; yd++) {
	// Fill in desnsity array from individual data with desired property
	n=Stateg->space[xd][yd].ninds;
	for(i=0; i<n; i++) {
	  ind = &Stateg->space[xd][yd].inds[i];
	  switch( d->xmode ) {
	  case DM_X0: { xp = &ind->x0;  break; }
	  case DM_X1: { xp = &ind->x1;  break; }
	  case DM_Y0: { xp = &ind->y0;  break; }
	  case DM_Y1: { xp = &ind->y1;  break; }
	  case DM_Z0: { xp = &ind->z0;  break; }
	  case DM_Z1: { xp = &ind->z1;  break; }
	  case DM_C:  { xp = &ind->c;   break; }
	  case DM_M:  { xp = &ind->m;   break; }
	  case DM_F:  { xp = &ind->f;   break; }
	    // This will keep the compiler quiet and force a segfault
	  default:    { *((u8b_t*)0) = 0; xp = NULL; }
	  }
	  switch( d->ymode ) {
	  case DM_X0: { yp = &ind->x0;  break; }
	  case DM_X1: { yp = &ind->x1;  break; }
	  case DM_Y0: { yp = &ind->y0;  break; }
	  case DM_Y1: { yp = &ind->y1;  break; }
	  case DM_Z0: { yp = &ind->z0;  break; }
	  case DM_Z1: { yp = &ind->z1;  break; }
	  case DM_C:  { yp = &ind->c;   break; }
	  case DM_M:  { yp = &ind->m;   break; }
	  case DM_F:  { yp = &ind->f;   break; }
	    // This will keep the compiler quiet and force a segfault
	  default:    { *((u8b_t*)0) = 0; yp = NULL; }
	  }
	  x = ((u32b_t)(*xp*(*yr-1)));
	  y = ((u32b_t)(*yp*(*xr-1)));
	  hmd[x][y]++;
	  if( hmd[x][y] > hmdm )
	    hmdm = hmd[x][y];
	}
      }
    }
  } else {
    // There was a valid deme selected, so use it:
    // Fill in desnsity array from individual data with desired property
    n=Stateg->space[ds->x][ds->y].ninds;
    for(i=0; i<n; i++) {
      ind = &Stateg->space[ds->x][ds->y].inds[i];
      switch( d->xmode ) {
      case DM_X0: { xp = &ind->x0;  break; }
      case DM_X1: { xp = &ind->x1;  break; }
      case DM_Y0: { xp = &ind->y0;  break; }
      case DM_Y1: { xp = &ind->y1;  break; }
      case DM_Z0: { xp = &ind->z0;  break; }
      case DM_Z1: { xp = &ind->z1;  break; }
      case DM_C:  { xp = &ind->c;   break; }
      case DM_M:  { xp = &ind->m;   break; }
      case DM_F:  { xp = &ind->f;   break; }
	// This will keep the compiler quiet and force a segfault
      default:    { *((u8b_t*)0) = 0; xp = NULL; }
      }
      switch( d->ymode ) {
      case DM_X0: { yp = &ind->x0;  break; }
      case DM_X1: { yp = &ind->x1;  break; }
      case DM_Y0: { yp = &ind->y0;  break; }
      case DM_Y1: { yp = &ind->y1;  break; }
      case DM_Z0: { yp = &ind->z0;  break; }
      case DM_Z1: { yp = &ind->z1;  break; }
      case DM_C:  { yp = &ind->c;   break; }
      case DM_M:  { yp = &ind->m;   break; }
      case DM_F:  { yp = &ind->f;   break; }
	// This will keep the compiler quiet and force a segfault
      default:    { *((u8b_t*)0) = 0; yp = NULL; }
      }
      x = ((u32b_t)(*xp*(*yr-1)));
      y = ((u32b_t)(*yp*(*xr-1)));
      hmd[x][y]++;
      if( hmd[x][y] > hmdm )
	hmdm = hmd[x][y];
    }
  }
  pthread_mutex_unlock(&StateLock);
    
  // Scale density to fill heightmap
  for(x=0; x<*xr; x++) {
    for(y=0; y<*yr; y++) {
      if( n ) hm[x][y] = hmd[x][y]/((float)hmdm);
      else    hm[x][y] = 0.0f;
    }
  }

}

static void CalcNormals(float **hm, vec3f_t **nrml, u32b_t xr, u32b_t yr)
{
  vec3f_t v1,v2,v3,v4,v5,v6;
  vec3f_t n,n1,n2,n3,n4,n5,n6;
  int     x,y,Y;

  //!!av: Clean this mess up

  for (y=0; y<yr; y++) {
    Y = y;
    for (x=0; x<xr; x++) {
      if ( y == 0 && x == 0 ) {
        // Back left corner - 1 tri 2 vertices
        VecSub3fv( Y+1,  x, yr*hm[y+1][x],Y, x,yr*hm[y][x], &v1);
        VecSub3fv(   Y,x+1, yr*hm[y][x+1],Y, x,yr*hm[y][x], &v2);
	VecCross(&v1,&v2,&n);
      } else if ( (y > 0 && y < (yr-1)) && x == 0 ) {
        // Left edge - 3 tri 4 vertices
        VecSub3fv(   Y,x+1, yr*hm[y][x+1],Y,   x,yr*hm[y][x], &v1);
        VecSub3fv( Y-1,x+1, yr*hm[y-1][x+1],Y, x,yr*hm[y][x], &v2);
        VecSub3fv( Y-1,  x, yr*hm[y-1][x],Y,   x,yr*hm[y][x], &v3);
        VecSub3fv(   Y,x+1, yr*hm[y][x+1],Y,   x,yr*hm[y][x], &v4);
	VecCross(&v1,&v2,&n1);  VecCross(&v2,&v3,&n2);  VecCross(&v3,&v4,&n3);
	VecAdd(&n1,&n2,&n);     VecAdd(&n,&n3,&n);      VecDiv(&n,3.0f);
      } else if ( y == (yr-1) && x == 0  ) {
        // Front left corner - 2 tri 3 vertices
        VecSub3fv(   Y,x+1, yr*hm[y][x+1],Y,   x,yr*hm[y][x], &v1);
        VecSub3fv( Y-1,x+1, yr*hm[y-1][x+1],Y, x,yr*hm[y][x], &v2);
        VecSub3fv( Y-1,  x, yr*hm[y-1][x],Y,   x,yr*hm[y][x], &v3);
        VecCross(&v1,&v2,&n1); VecCross(&v2,&v3,&n2);
	VecAdd(&n1,&n2,&n);    VecDiv(&n,2.0f);
      } else if ( y == (yr-1) && (x > 0 && x < (xr-1)) ) {
        // Front edge - 3 tri 4 vertices
        VecSub3fv(   Y,x+1, yr*hm[y][x+1],Y,   x,yr*hm[y][x], &v1);
        VecSub3fv( Y-1,x+1, yr*hm[y-1][x+1],Y, x,yr*hm[y][x], &v2);
        VecSub3fv( Y-1,  x, yr*hm[y-1][x],Y,   x,yr*hm[y][x], &v3);
        VecSub3fv(   Y,x-1, yr*hm[y][x-1],Y,   x,yr*hm[y][x], &v4);
        VecCross(&v1,&v2,&n1);  VecCross(&v2,&v3,&n2);  VecCross(&v3,&v4,&n3);
	VecAdd(&n1,&n2,&n);     VecAdd(&n,&n3,&n);      VecDiv(&n,3.0f);
      } else if ( y == (yr-1) && x == (xr-1) ) {
        // Front right corner - 1 tri 2 vertices
        VecSub3fv( Y-1,  x, yr*hm[y-1][x],Y, x,yr*hm[y][x], &v1);
        VecSub3fv(   Y,x-1, yr*hm[y][x-1],Y, x,yr*hm[Y][x], &v2);
        VecCross(&v1,&v2,&n);
      } else if ( ( y > 0 && y < (yr-1)) && x == (xr-1)  ) {
        // Right edge - 3 tri 4 vertices
        VecSub3fv( Y-1,  x, yr*hm[y-1][x],Y,   x,yr*hm[y][x], &v1);
        VecSub3fv(   Y,x-1, yr*hm[y][x-1],Y,   x,yr*hm[y][x], &v2);
        VecSub3fv( Y+1,x-1, yr*hm[y+1][x-1],Y, x,yr*hm[y][x], &v3);
        VecSub3fv( Y+1,  x, yr*hm[y+1][x],Y,   x,yr*hm[y][x], &v4);
        VecCross(&v1,&v2,&n1);  VecCross(&v2,&v3,&n2);  VecCross(&v3,&v4,&n3);
	VecAdd(&n1,&n2,&n);     VecAdd(&n,&n3,&n);      VecDiv(&n,3.0f);
      } else if ( y == 0 && x == (xr-1) ) {
        // Back right corner - 2 tri 3 vertices
        VecSub3fv(   Y,x-1, yr*hm[y][x-1], Y,     x, yr*hm[y][x],   &v1);
        VecSub3fv( Y+1,x-1, yr*hm[y+1][x-1], Y, x-1, yr*hm[y][x-1], &v2);
        VecSub3fv( Y+1,x-1, yr*hm[y+1][x-1], Y,   x, yr*hm[y][x],   &v3);
        VecSub3fv( Y+1,  x, yr*hm[y+1][x], Y,     x, yr*hm[y][x],   &v4);
        VecCross(&v1,&v2,&n1);  VecCross(&v3,&v4,&n2);
	VecAdd(&n1,&n2,&n);     VecDiv(&n,2.0f);
      } else if ( y == 0 && ( x > 0 && x < (xr-1)) ) {
        // Back edge - 3 tri 4 vertices
        VecSub3fv(   Y,x-1, yr*hm[y][x-1],Y,   x,yr*hm[y][x], &v1);
        VecSub3fv( Y+1,x-1, yr*hm[y+1][x-1],Y, x,yr*hm[y][x], &v2);
        VecSub3fv( Y+1,  x, yr*hm[y+1][x],Y,   x,yr*hm[y][x], &v3);
        VecSub3fv(   Y,x+1, yr*hm[y][x+1],Y,   x,yr*hm[y][x], &v4);
        VecCross(&v1,&v2,&n1);  VecCross(&v2,&v3,&n2);  VecCross(&v3,&v4,&n3);
	VecAdd(&n1,&n2,&n);     VecAdd(&n,&n3,&n);      VecDiv(&n,3.0f);
      } else {
        // Internal - 6 tri 6 vertices
        VecSub3fv(   Y,x+1, yr*hm[y][x+1],Y,   x,yr*hm[y][x], &v1);
        VecSub3fv( Y-1,x+1, yr*hm[y-1][x+1],Y, x,yr*hm[y][x], &v2);
        VecSub3fv( Y-1,  x, yr*hm[y-1][x],Y,   x,yr*hm[y][x], &v3);
        VecSub3fv(   Y,x-1, yr*hm[y][x-1],Y,   x,yr*hm[y][x], &v4);
        VecSub3fv( Y+1,x-1, yr*hm[y+1][x-1],Y, x,yr*hm[y][x], &v5);
        VecSub3fv( Y+1,  x, yr*hm[y+1][x],Y,   x,yr*hm[y][x], &v6);
        VecCross(&v1,&v2,&n1);  VecCross(&v2,&v3,&n2);  VecCross(&v3,&v4,&n3);
        VecCross(&v4,&v5,&n4);  VecCross(&v5,&v6,&n5);  VecCross(&v6,&v1,&n6);
	VecAdd(&n1,&n2,&n);     VecAdd(&n3,&n4,&n1);    VecAdd(&n5,&n6,&n2);
	VecAdd(&n,&n1,&n);     	VecAdd(&n,&n2,&n);      VecDiv(&n,6.0f);
      }
      // Normalizing should be done automatically by Opengl as long as GL_NORMALIZE is enabled.
      // VecNormalize(n);
      memcpy(&nrml[y][x], &n, sizeof(vec3f_t));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

double *cluster_z;

int compare(const void *a, const void *b)
{
  int c = *((int*)a);
  int d = *((int*)b);
  
  if(cluster_z[c] > cluster_z[d]) return -1;
  if(cluster_z[c] < cluster_z[d]) return  1;
  return 0;
}

void Deme_Draw(widget_t *w)
{
  static float         **hm;  
  static vec3f_t       **nrml; 
  static u32b_t       ***tsg;
  static clust_info_t   *ci;
  static int            *translate;
  gind_t                *i;
  char                   title[64];  
  u32b_t                 x,y,xr,yr,j,k,l,m;
  double                 ax0,ax1,am,size;

  deme_gui_t  *d  = (deme_gui_t*)w->wd;
  demes_gui_t *ds = (demes_gui_t*)d->dc;

  static GLUquadricObj *quadratic;

  if( !quadratic ) {
    quadratic=gluNewQuadric();
    gluQuadricNormals(quadratic, GLU_SMOOTH);
  }

  // Malloc space for the heightmap
  if( !hm ) {
    float *p;  vec3f_t *vp;

    // Find max size needed for all modes
    x = maxi(P_LX*2+1, P_LY*2+1);  y = maxi(P_LX*2+1, P_LY*2+1);
    x = maxi(       x, P_LZ*2+1);  y = maxi(       y, P_LZ*2+1);
    x = maxi(       x, P_LC*2+1);  y = maxi(       y, P_LC*2+1);
    x = maxi(       x, P_LM*2+1);  y = maxi(       y, P_LM*2+1);
    x = maxi(       x, P_LF*2+1);  y = maxi(       y, P_LF*2+1);

    // Allocate space for heightmap
    if( !(p = malloc(x*y*sizeof(float))) ) Error("malloc(heightmap) failed!");
    if( !(hm = malloc(x*sizeof(float*))) ) Error("malloc(heightmap) failed!");
    for(j=0; j<x; j++)
      hm[j] = &p[j*y];

    // Allocate space for normals
    if( !(vp   = malloc(x*y*sizeof(vec3f_t))) ) Error("malloc(heightmap) failed!");
    if( !(nrml = malloc(x*sizeof(vec3f_t*)))  ) Error("malloc(heightmap) failed!");
    for(j=0; j<x; j++)
      nrml[j] = &vp[j*y];
  }

  if( !ci ) {
    //ci = CountClustersInDeme(&Stateg->space[P_WIDTH/2][P_HEIGHT/2]);
    ci = (void*)1;
    //if( !(translate = malloc(ci->nspc[ci->gnx]*sizeof(int))) )    Error("malloc(translate) failed!");
    //if( !(cluster_z = malloc(ci->nspc[ci->gnx]*sizeof(double))) ) Error("malloc(cluster_z) failed!");
  }

  // Go through demes and compute the property to be graphed
  FillHeightMap(w, hm, title, &xr, &yr);
  CalcNormals(hm, nrml, xr, yr);

  switch( d->mode ) {
  case DM_3D_TRAIT:
    ////////////////////////////////////////////////////////////
    // 3D trait cluster view
    ////////////////////////////////////////////////////////////

    // Outline widget border
    Yellow();
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.0f,0.0f);
    glVertex2f(0.0f,1.0f);
    glVertex2f(1.0f,1.0f);
    glVertex2f(1.0f,0.0f);
    glEnd();

    // Draw title
    glRasterPos2f(0.5f-((strlen("Density: x0,x1,m")*6.0f/2.0f)/ScaleX(w,w->w)),0.85f); 
    printGLf(w->glw->font,"Density: x0,x1,m");
    sprintf(title,"Min: %d",d->spctrshld);
    glRasterPos2f(0.5f-((strlen(title)*6.0f/2.0f)/ScaleX(w,w->w)),0.90f); 
    printGLf(w->glw->font,title);
    sprintf(title,"Clusters: %d",ci->gcount);
    glRasterPos2f(0.5f-((strlen(title)*6.0f/2.0f)/ScaleX(w,w->w)),0.95f); 
    printGLf(w->glw->font,title);
    
    // Switch over to a custom viewport "mode" which will let
    // us draw with our own perspective within our widget boundries.
    ViewPort3D(ScaleX(w,w->x), w->glw->height-ScaleY(w,w->y)-ScaleY(w,w->h), ScaleX(w,w->w), ScaleY(w,w->h));
    
    glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
    glLightfv(GL_LIGHT1, GL_POSITION,LightPosition);    

    // Rotate and translate to position 0.5 at the origin.
    // This allows all further drawing to be done [0,1], which,
    // IMHO, is very nice to work with.
    //glTranslatef(x,yf,z);                                      // Move graph "object" up/down on screen
    glRotatef(-55.0f+d->rot.x+(d->ny-d->oy),1.0f,0.0f,0.0f);     // Tilt back (rotate on graph's x axis)
    glRotatef(d->rot.z+(d->nx-d->ox)+d->ticks,0.0f,0.0f,1.0f);   // Rotate on graph's y-axis
    glTranslatef(-0.5f,-0.5f,-0.0f);                             // Position .5 at origin
    glScalef(1.0f,1.0f,0.75f);
    
    // Outline bottom and top of heightmap
    glColor3f(0.3f, 0.3f, 0.3f);
    glEnable(GL_LINE_SMOOTH);
    glBegin(GL_LINE_LOOP); 
    glVertex2f(0.0f,0.0f);
    glVertex2f(0.0f,1.0f);
    glVertex2f(1.0f,1.0f);
    glVertex2f(1.0f,0.0f);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glVertex3f(0.0f,0.0f,1.0f);
    glVertex3f(0.0f,1.0f,1.0f);
    glVertex3f(1.0f,1.0f,1.0f);
    glVertex3f(1.0f,0.0f,1.0f);
    glEnd();
    glDisable(GL_LINE_SMOOTH);

    // Draw the clusters
    if( d->clusters ) { 

      // Project to screen space
      {
	double model_view_matrix[16];
	double projection_matrix[16];
	int    viewport[16];
	double wx,wy,wz;
	
	for(k=0; k<ci->nspc[ci->gnx]; k++) {
	  translate[k] = k;
	}
	
	glGetDoublev(GL_MODELVIEW_MATRIX, model_view_matrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projection_matrix);
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	for(j=0; j<ci->nspc[ci->gnx]; j++) {
	  ax0 = 0.0;
	  ax1 = 0.0;
	  am  = 0.0;
	  for(k=0; k<ci->spc[ci->gnx][j].n; k++) {
	    ax0 += ci->spc[ci->gnx][j].i[k]->x0;
	    ax1 += ci->spc[ci->gnx][j].i[k]->x1;
	    am  += ci->spc[ci->gnx][j].i[k]->m;
	  }
	  ax0 /= ci->spc[ci->gnx][j].n;
	  ax1 /= ci->spc[ci->gnx][j].n;
	  am  /= ci->spc[ci->gnx][j].n;
	  
	  gluProject( ax0, ax1, am, model_view_matrix, projection_matrix, viewport, &wx, &wy, &wz);
	  cluster_z[j] = wz;
	}
	
	qsort(translate,ci->nspc[ci->gnx],sizeof(int),compare);
      }
      
      // Find maximum cluster size so we can scale
      m = 0;
      for(j=0; j<ci->nspc[ci->gnx]; j++) {
	if( ci->spc[ci->gnx][j].n > m ) { 
	  m = ci->spc[ci->gnx][j].n; 
	}
      }

      // Process one cluster at a time
      for(j=0; j<ci->nspc[ci->gnx]; j++) {
	// Skip clusters that are too small
	if( ci->spc[ci->gnx][translate[j]].n >= P_K0*0.05 ) {
	  // Find the average (x0,x1,m) position of this cluster
	  ax0 = 0.0;
	  ax1 = 0.0;
	  am  = 0.0;
	  for(k=0; k<ci->spc[ci->gnx][translate[j]].n; k++) {
	    ax0 += ci->spc[ci->gnx][translate[j]].i[k]->x0;
	    ax1 += ci->spc[ci->gnx][translate[j]].i[k]->x1;
	    am  += ci->spc[ci->gnx][translate[j]].i[k]->m;
	  }
	  ax0 /= ci->spc[ci->gnx][translate[j]].n;
	  ax1 /= ci->spc[ci->gnx][translate[j]].n;
	  am  /= ci->spc[ci->gnx][translate[j]].n;
	  
	  // Draw a tick mark where the cluster is
	  size = ci->spc[ci->gnx][translate[j]].n / ((double)m);
	  /*
	  if( size < 6.0/64.0 ) {
	    size = 6.0/64.0;
	  }
	  glPointSize(size*64.0f);
	  glEnable(GL_POINT_SMOOTH);
	  glBegin(GL_POINTS); 
	  glColor3f(size,0.0f,1.0-size);
	  glVertex3f(ax0,ax1,am);
	  glEnd(); 
	  glPointSize(1.0f);
	  glDisable(GL_POINT_SMOOTH);
	  */
	  //DrawSphere(ax0,ax1,m,size);

	  // Sphere stuff (cluster markers)
	  {
	    if( size < .1 ) {
	      size = .1;
	    }

	    //glColor3f(size,0.0f,1.0-size);
	    float mcolor[] = { size, 0.0f, 1.0f-size, 0.6f };
	    float shine=50;

	    glPushMatrix();
	    glCullFace(GL_BACK);
	    glEnable(GL_CULL_FACE);
	    glTranslatef(ax0,ax1,am);
	    glEnable(GL_LIGHTING);
	    glEnable(GL_LIGHT1);
	    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mcolor);
	    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,            mcolor);
	    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS,           &shine);
	    gluSphere(quadratic,size/16.0,16,16);
	    glDisable(GL_LIGHT1);
	    glDisable(GL_LIGHTING);
	    glDisable(GL_CULL_FACE);
	    glPopMatrix();
	  }

	}
      }
    }

    //
    // 3-d density plot
    //
    if( !tsg ) {
      // Memory not yet allocated, do so.
      if( !(tsg = (u32b_t***)malloc((P_LY*2+1)*sizeof(u32b_t**))) )
	Error("Malloc tsg_y0 failed.");
      for(j=0; j<(P_LY*2+1); j++) {
	if( !(tsg[j] = (u32b_t**)malloc((P_LY*2+1)*sizeof(u32b_t*))) )
	  Error("Malloc tsg_y1 failed.");
	for(k=0; k<(P_LM*2+1); k++) {
	  if( !(tsg[j][k] = (u32b_t*)malloc((P_LM*2+1)*sizeof(u32b_t))) )
	    Error("Malloc tsg_m failed.");
	}
      }
    }

    // Fill in density plot
    for(j=0; j<(P_LY*2+1); j++)
      for(k=0; k<(P_LY*2+1); k++)
	memset(tsg[j][k],0,(P_LM*2+1)*sizeof(u32b_t));
    m = 0;
    pthread_mutex_lock(&StateLock);
    //for(x=0; x<P_WIDTH; x++) {
    //for(y=0; y<P_HEIGHT; y++) {
    x = P_WIDTH/2;
    y = P_HEIGHT/2;
    for(j=0; j<Stateg->space[x][y].ninds; j++) {
      i = &Stateg->space[x][y].inds[j];
      if( ++tsg[(int)(i->x0*P_LX*2)][(int)(i->x1*P_LX*2)][(int)(i->m*P_LM*2)] > m )
	m = tsg[(int)(i->x0*P_LX*2)][(int)(i->x1*P_LX*2)][(int)(i->m*P_LM*2)];
    }
    //}
    //}
    pthread_mutex_unlock(&StateLock);    

    // We want all this density plot stuff to be drawn _over_ (screen z-order)
    // the earlier cluster plot.  To do this, I'll clear the Z-buffer here.
    glClear(GL_DEPTH_BUFFER_BIT);

    // Draw the plot   
    glColor3f(1.0f, 0.0f, 1.0f);
    glPointSize(6.0f);
    glEnable(GL_POINT_SMOOTH);
    glBegin(GL_POINTS); 
    for(j=0; j<(P_LX*2+1); j++) {
      for(k=0; k<(P_LX*2+1); k++) {
	for(l=0; l<(P_LM*2+1); l++) {
	  if( tsg[j][k][l] >= d->spctrshld ) {
	    glColor3f(((float)tsg[j][k][l])/m,((float)tsg[j][k][l])/m,(((float)tsg[j][k][l])/m));
	    glVertex3f(((float)j)/(P_LX*2),((float)k)/(P_LX*2),((float)l)/(P_LM*2));
	  }
	}
      }
    }
    glEnd(); 
    glPointSize(1.0f);
    glDisable(GL_POINT_SMOOTH);

    // Label the four corners
    White(); 
    glRasterPos3f(0.0f,0.0f,0.1f); printGLf(w->glw->font,"(0,0)");
    glRasterPos3f(0.0f,1.0f,0.1f); printGLf(w->glw->font,"(0,1)");
    glRasterPos3f(1.0f,1.0f,0.1f); printGLf(w->glw->font,"(1,1)");
    glRasterPos3f(1.0f,0.0f,0.1f); printGLf(w->glw->font,"(1,0)");
    
    // Restore 2d viewport for other widgets
    ViewPort2D(w->glw);
    break;

  case DM_3D:
    ////////////////////////////////////////////////////////////
    // 3D view
    ////////////////////////////////////////////////////////////

    // Outline widget border
    Yellow();
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.0f,0.0f);
    glVertex2f(0.0f,1.0f);
    glVertex2f(1.0f,1.0f);
    glVertex2f(1.0f,0.0f);
    glEnd();

    // Draw title
    glRasterPos2f(0.5f-((strlen(title)*6.0f/2.0f)/ScaleX(w,w->w)),0.9f); 
    printGLf(w->glw->font,"%s",title);
    
    // Switch over to a custom viewport "mode" which will let
    // us draw with our own perspective within our widget boundries.
    ViewPort3D(ScaleX(w,w->x), w->glw->height-ScaleY(w,w->y)-ScaleY(w,w->h), ScaleX(w,w->w), ScaleY(w,w->h));
    
    // Position our light for shading
    //glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
    //glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
    //glLightfv(GL_LIGHT1, GL_POSITION,LightPosition);

    // Rotate and translate to position 0.5 at the origin.
    // This allows all further drawing to be done [0,1], which,
    // IMHO, is very nice to work with.
    //glTranslatef(x,yf,z);                                      // Move graph "object" up/down on screen
    glRotatef(-55.0f+d->rot.x+(d->ny-d->oy),1.0f,0.0f,0.0f);     // Tilt back (rotate on graph's x axis)
    glRotatef(d->rot.z+(d->nx-d->ox)+d->ticks,0.0f,0.0f,1.0f);   // Rotate on graph's y-axis
    glTranslatef(-0.5f,-0.5f,-0.0f);                             // Position .5 at origin
    glScalef(1.0f,1.0f,0.75f);
    
    // Outline bottom and top of heightmap
    glColor3f(0.3f, 0.3f, 0.3f);
    glEnable(GL_LINE_SMOOTH);
    glBegin(GL_LINE_LOOP); 
    glVertex2f(0.0f,0.0f);
    glVertex2f(0.0f,1.0f);
    glVertex2f(1.0f,1.0f);
    glVertex2f(1.0f,0.0f);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glVertex3f(0.0f,0.0f,1.0f);
    glVertex3f(0.0f,1.0f,1.0f);
    glVertex3f(1.0f,1.0f,1.0f);
    glVertex3f(1.0f,0.0f,1.0f);
    glEnd();
    glDisable(GL_LINE_SMOOTH);
    
    //
    // Draw triangle strip for the surface, based on computed values
    //
    glPolygonOffset( 1.5f, 2.0f );
    glEnable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT1);
    for(x=0; x<xr-1; x++) {
      glBegin(GL_TRIANGLE_STRIP);
      for(y=0; y<yr; y++) {
	// Draw points for heightmap
	for(j=0; j<2; j++) {
	  // Set vertex color
	  float mcolor[] = { hm[x+j][y], 0.0f, 1.0f-hm[x+j][y], 1.0f };
	  float shine=50;
	  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mcolor);
	  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,            mcolor);
	  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS,           &shine);
	  // Set normal for lighting/shading
	  glNormal3f( nrml[x+j][y].x, nrml[x+j][y].y, nrml[x+j][y].z );
	  // Draw vertex
	  glVertex3f((x+j)/((float)xr-1), y/((float)yr-1), hm[x+j][y]);
	}
      }
      glEnd();
    }
    glDisable(GL_LIGHT1);
    glDisable(GL_LIGHTING);
    glDisable(GL_POLYGON_OFFSET_FILL);

    //
    // Draw square grid over the heightmap
    //
    glLineWidth(2.0);
    glEnable(GL_LINE_SMOOTH);
    glColor3f(0.0f, 0.0f, 0.0f);
    for(x=0; x<xr; x++) {
      glBegin(GL_LINE_STRIP);
      for(y=0; y<yr; y++) 
	glVertex3f(x/((float)xr-1), y/((float)yr-1), hm[x][y]);
      glEnd();
      glBegin(GL_LINE_STRIP);
      for(y=0; y<yr; y++)
	glVertex3f(y/((float)yr-1), x/((float)xr-1), hm[y][x]);
      glEnd();
    }
    glLineWidth(1.0);
    glDisable(GL_LINE_SMOOTH);

    // Label the four corners
    White(); 
    glRasterPos3f(0.0f,0.0f,0.1f); printGLf(w->glw->font,"(0,0)");
    glRasterPos3f(0.0f,1.0f,0.1f); printGLf(w->glw->font,"(0,1)");
    glRasterPos3f(1.0f,1.0f,0.1f); printGLf(w->glw->font,"(1,1)");
    glRasterPos3f(1.0f,0.0f,0.1f); printGLf(w->glw->font,"(1,0)");
    
    // Restore 2d viewport for other widgets
    ViewPort2D(w->glw);
    break;

  case DM_2D:
    ////////////////////////////////////////////////////////////
    // 2D view
    ////////////////////////////////////////////////////////////

    // Fades from white <-> black to represent density
    glBegin(GL_QUADS);
    for(x=0; x<xr; x++) {
      for(y=0; y<yr; y++) {
	// Draw colored quad
	if( hm[x][y] == -1 ) {
	  // Bright red should never really happen apparently
	  glColor3f(1.0f, 0.0f, 0.0f);
	} else if( hm[x][y] == 0 ) {
	  // light red represents empty buckets
	  glColor3f(0.3f, 0.0f, 0.0f);
	} else {
	  glColor3f(hm[x][y], hm[x][y], hm[x][y]);
	}
	glVertex2f((1.0f/xr)*x,     (1.0f/yr)*y);
	glVertex2f((1.0f/xr)*x,     (1.0f/yr)*(y+1));
	glVertex2f((1.0f/xr)*(x+1), (1.0f/yr)*(y+1));
	glVertex2f((1.0f/xr)*(x+1), (1.0f/yr)*y);
      }
    }
    glEnd();
    
    // Fades from white <-> black to represent density
    for(x=0; x<xr; x++) {
      for(y=0; y<yr; y++) {
	// Draw colored quad
	if( hm[x][y] == -1 ) {
	  // Bright red should never really happen apparently
	  glColor3f(1.0f, 0.0f, 0.0f);
	} else if( hm[x][y] == 0 ) {
	  // light red represents empty buckets
	  glColor3f(0.3f, 0.0f, 0.0f);
	} else {
	  glColor3f(hm[x][y], hm[x][y], hm[x][y]);
	}
	glVertex2f((1.0f/xr)*x,     (1.0f/yr)*y);
	glVertex2f((1.0f/xr)*x,     (1.0f/yr)*(y+1));
	glVertex2f((1.0f/xr)*(x+1), (1.0f/yr)*(y+1));
	glVertex2f((1.0f/xr)*(x+1), (1.0f/yr)*y);
      }
    }

    glRasterPos2f(0.5f-((strlen("Density: x0,x1,m")*6.0f/2.0f)/ScaleX(w,w->w)),0.85f); 
    printGLf(w->glw->font,"Density: x0,x1,m");

    // Outline widget border
    Yellow();
    glBegin(GL_LINE_LOOP);
    glVertex2f(0.0f,0.0f);
    glVertex2f(0.0f,1.0f);
    glVertex2f(1.0f,1.0f);
    glVertex2f(1.0f,0.0f);
    glEnd();

    // Draw title
    Yellow();
    glRasterPos2f(0.5f-((strlen(title)*6.0f/2.0f)/ScaleX(w,w->w)),0.1f); 
    printGLf(w->glw->font,"%s",title);
    
    break;
  }
}

////////////////////////////////////////////////////////////

void Deme_Tick(widget_t *w)
{
  deme_gui_t *d = (deme_gui_t*)w->wd;

  // Only update ticks if mouse is not down
  if( (!d->d) && (d->anim) && ( (d->mode == DM_3D) || (d->mode == DM_3D_TRAIT) ) )
    d->ticks++;
}

void Deme_Down(widget_t *w, const int x, const int y, const int b)
{
  deme_gui_t *d = (deme_gui_t*)w->wd;

  // We only care about mouse down events within the widget area
  if( (x > ScaleX(w,w->x)) && (x < ScaleX(w,w->x+w->w)) && 
      (y > ScaleY(w,w->y)) && (y < ScaleY(w,w->y+w->h))     ) {
    // Switch on mouse button pressed
    switch( b ) {
    case MOUSE_UP:
      if( d->mode != DM_3D_TRAIT ) {
	// Cycle x-mode
	d->xmode++; 
	if( d->xmode > NDM ) d->xmode = 0;
	w->update = 1;
      } else {
	// Raise 4D threshold
	d->spctrshld++;
      }
      break;
    case MOUSE_DOWN:
      if( d->mode != DM_3D_TRAIT ) {
	// Cycle y-mode
	d->ymode++; 
	if( d->ymode > NDM ) d->ymode = 0;
	w->update = 1;
      } else {
	// Lower 4D threshold
	if( d->spctrshld != 0 )
	  d->spctrshld--;
      }
      break;
    case MOUSE_RIGHT:
      // Toggle animation (auto-rotate) flag.
      if( d->mode != DM_2D )
	d->anim ^= 1;
      break;
    case MOUSE_MIDDLE:
      // Toggle 2D/3D
      if( ++d->mode > DM_3D_TRAIT )
	d->mode = DM_2D;
      break;
    case MOUSE_LEFT:
      // Mouse rotation controll
      if( (!d->d) && (d->mode != DM_2D) ) {
	d->d  = 1;
	d->mb = b;
	d->ox = d->nx = x;
	d->oy = d->ny = y;
      }
    }
  }

}

void Deme_Up(widget_t *w, const int x, const int y, const int b)
{
  deme_gui_t *d = (deme_gui_t*)w->wd;

  // Mouse rotation controll
  if( (!d->d) || (b != d->mb) )
    return;

  switch( d->mb ) {
  case MOUSE_LEFT:
    d->d  = 0;
    d->nx = x;
    d->ny = y;
    d->rot.z += d->nx - d->ox;
    d->rot.x += d->ny - d->oy;
    d->ox = d->nx;
    d->oy = d->ny;
    break;
  }

}

void Deme_Move(widget_t *w, const int x, const int y)
{
  deme_gui_t *d = (deme_gui_t*)w->wd;

  // Mouse rotation controll
  if( !d->d ) 
    return;

  switch( d->mb ) {
  case MOUSE_LEFT:
    d->nx = x;
    d->ny = y;
    break;
  }

}

////////////////////////////////////////////////////////////////////////////////
// Multiple demes graph
//
// This displays a graph of all demes.  It's data point(s) for each deme 
// represent averages over the entire deme's population for a given property.
////////////////////////////////////////////////////////////////////////////////

void Demes_Draw(widget_t *w)
{
  static u32b_t *x0d,*x1d,*y0d,*y1d,*z0d,*z1d,*cd,*md,*fd;  
  double         mx,mX,my,mY,mz,mZ,mc,mm,mf,vx,vX,vy,vY,vz,vZ,vc,vm,vf;
  demes_gui_t   *d = (demes_gui_t*)w->wd;  u32b_t x,y,i,n,M;  gind_t *ind;  char buf[128];
  
  // Malloc space for the heightmap
  if( !x0d ) {
    // Allocate space for heightmap
    if( !(x0d = malloc((P_LX*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist x0) failed!");
    if( !(x1d = malloc((P_LX*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist x1) failed!");
    if( !(y0d = malloc((P_LY*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist y0) failed!");
    if( !(y1d = malloc((P_LY*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist y1) failed!");
    if( !(z0d = malloc((P_LZ*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist z0) failed!");
    if( !(z1d = malloc((P_LZ*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist z1) failed!");
    if( !(cd  = malloc((P_LC*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist c) failed!");
    if( !(md  = malloc((P_LM*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist m) failed!");
    if( !(fd  = malloc((P_LF*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist f) failed!");
  }

  // Outline deme graph
  Yellow();
  glBegin(GL_LINE_LOOP);
  glVertex2f(0.0f,0.0f);  glVertex2f(0.0f,1.0f);
  glVertex2f(1.0f,1.0f);  glVertex2f(1.0f,0.0f);
  glEnd();

  pthread_mutex_lock(&StateLock);

  for(x=0; x<P_WIDTH; x++) {
    for(y=0; y<P_HEIGHT; y++) {
      // Setup for drawing a single deme
      glPushMatrix();
      glTranslatef(x*1.0f/P_WIDTH, y*1.0f/P_HEIGHT, 0.0f);
      glScalef(1.0f/P_WIDTH, 1.0f/P_HEIGHT, 1.0f);

      if( Stateg->space[x][y].ninds ) {

	// Draw a mess of quads ;)
	glBegin(GL_QUADS);

	// Clear distributions and averages
	memset(x0d, 0, (P_LX*2+1)*sizeof(u32b_t));  memset(x1d, 0, (P_LX*2+1)*sizeof(u32b_t));
	memset(y0d, 0, (P_LY*2+1)*sizeof(u32b_t));  memset(y1d, 0, (P_LY*2+1)*sizeof(u32b_t));
	memset(z0d, 0, (P_LZ*2+1)*sizeof(u32b_t));  memset(z1d, 0, (P_LZ*2+1)*sizeof(u32b_t));
	memset(cd,  0, (P_LC*2+1)*sizeof(u32b_t));  
	memset(md,  0, (P_LM*2+1)*sizeof(u32b_t));
	memset(fd,  0, (P_LF*2+1)*sizeof(u32b_t));
	mx = mX = my = mY = mz = mZ = mc = mm = mf = 0.0;
	n = Stateg->space[x][y].ninds;
	for(i=0; i<n; i++) {
	  ind = &Stateg->space[x][y].inds[i];
	  // Sum for average trait value
	  mx += ind->x0;  mX += ind->x1;
	  my += ind->y0;  mY += ind->y1;
	  mz += ind->z0;  mZ += ind->z1;
	  mc += ind->c;   mm += ind->m;   mf += ind->f;
	  // Compute distributions
	  x0d[((int)(ind->x0*(P_LX*2)))]++;  x1d[((int)(ind->x1*(P_LX*2)))]++;
	  y0d[((int)(ind->y0*(P_LY*2)))]++;  y1d[((int)(ind->y1*(P_LY*2)))]++;
	  z0d[((int)(ind->z0*(P_LZ*2)))]++;  z1d[((int)(ind->z1*(P_LZ*2)))]++;
	  cd[((int)(ind->c*(P_LC*2)))]++;    
	  md[((int)(ind->m*(P_LM*2)))]++;
	  fd[((int)(ind->f*(P_LF*2)))]++;
	}
	// Divide for average
	mx /= n;  mX /= n;
	my /= n;  mY /= n;
	mz /= n;  mZ /= n;
	mc /= n;  mm /= n;  mf /= n;
	// Now use the average to compute varience
	vx = vX = vy = vY = vz = vZ = vc = vm = vf = 0.0;
	for(i=0; i<n; i++) {
	  ind = &Stateg->space[x][y].inds[i];
	  vx += (ind->x0-mx)*(ind->x0-mx);  vX += (ind->x1-mX)*(ind->x1-mX);
	  vy += (ind->y0-my)*(ind->y0-my);  vY += (ind->y1-mY)*(ind->y1-mY);
	  vz += (ind->z0-mz)*(ind->z0-mz);  vZ += (ind->z1-mZ)*(ind->z1-mZ);
	  vc += (ind->c-mc)*(ind->c-mc);    vm += (ind->m-mm)*(ind->m-mm);    vf += (ind->f-mf)*(ind->f-mf);
	}
	// Divide and sqrt for varience -> std.dev
	vx = sqrt(vx/n);  vX = sqrt(vX/n); 
	vy = sqrt(vy/n);  vY = sqrt(vY/n);
	vz = sqrt(vz/n);  vZ = sqrt(vZ/n);
	vc = sqrt(vc/n);  vm = sqrt(vm/n);
	vf = sqrt(vf/n);

	// Perch height:
	for(M=i=0; i<(P_LX*2+1); i++)
	  if( x0d[i] > M ) 
	    M = x0d[i];
	for(i=0; i<(P_LX*2+1); i++) {
	  glColor3f(((float)x0d[i])/M, 0.0f, 0.0f);
	  glVertex2f(0/9.0f, 1.0f-((i+1)/(P_LX*2+1.0f))); glVertex2f(0/9.0f, 1.0f-( i   /(P_LX*2+1.0f)));
	  glVertex2f(1/9.0f, 1.0f-( i   /(P_LX*2+1.0f))); glVertex2f(1/9.0f, 1.0f-((i+1)/(P_LX*2+1.0f))); 
	}
	for(M=i=0; i<(P_LY*2+1); i++)
	  if( y0d[i] > M ) 
	    M = y0d[i];
	for(i=0; i<(P_LY*2+1); i++) {
	  glColor3f(((float)y0d[i])/M, 0.0f, 0.0f);
	  glVertex2f(1/9.0f, 1.0f-((i+1)/(P_LY*2+1.0f))); glVertex2f(1/9.0f, 1.0f-( i   /(P_LY*2+1.0f)));
	  glVertex2f(2/9.0f, 1.0f-( i   /(P_LY*2+1.0f))); glVertex2f(2/9.0f, 1.0f-((i+1)/(P_LY*2+1.0f))); 
	}
	for(M=i=0; i<(P_LZ*2+1); i++)
	  if( z0d[i] > M ) 
	    M = z0d[i];
	for(i=0; i<(P_LZ*2+1); i++) {
	  glColor3f(((float)z0d[i])/M, 0.0f, 0.0f);
	  glVertex2f(2/9.0f, 1.0f-((i+1)/(P_LZ*2+1.0f))); glVertex2f(2/9.0f, 1.0f-( i   /(P_LZ*2+1.0f)));
	  glVertex2f(3/9.0f, 1.0f-( i   /(P_LZ*2+1.0f))); glVertex2f(3/9.0f, 1.0f-((i+1)/(P_LZ*2+1.0f))); 
	}
	// Perch diameter:
	for(M=i=0; i<(P_LX*2+1); i++)
	  if( x1d[i] > M ) 
	    M = x1d[i];
	for(i=0; i<(P_LX*2+1); i++) {
	  glColor3f(0.0f, 0.0f, ((float)x1d[i])/M);
	  glVertex2f(3/9.0f, 1.0f-((i+1)/(P_LX*2+1.0f))); glVertex2f(3/9.0f, 1.0f-( i   /(P_LX*2+1.0f)));
	  glVertex2f(4/9.0f, 1.0f-( i   /(P_LX*2+1.0f))); glVertex2f(4/9.0f, 1.0f-((i+1)/(P_LX*2+1.0f))); 
	}
	for(M=i=0; i<(P_LY*2+1); i++)
	  if( y1d[i] > M ) 
	    M = y1d[i];
	for(i=0; i<(P_LY*2+1); i++) {
	  glColor3f(0.0f, 0.0f, ((float)y1d[i])/M);
	  glVertex2f(4/9.0f, 1.0f-((i+1)/(P_LY*2+1.0f))); glVertex2f(4/9.0f, 1.0f-( i   /(P_LY*2+1.0f)));
	  glVertex2f(5/9.0f, 1.0f-( i   /(P_LY*2+1.0f))); glVertex2f(5/9.0f, 1.0f-((i+1)/(P_LY*2+1.0f))); 
	}
	for(M=i=0; i<(P_LZ*2+1); i++)
	  if( z1d[i] > M ) 
	    M = z1d[i];
	for(i=0; i<(P_LZ*2+1); i++) {
	  glColor3f(0.0f, 0.0f, ((float)z1d[i])/M);
	  glVertex2f(5/9.0f, 1.0f-((i+1)/(P_LZ*2+1.0f))); glVertex2f(5/9.0f, 1.0f-( i   /(P_LZ*2+1.0f)));
	  glVertex2f(6/9.0f, 1.0f-( i   /(P_LZ*2+1.0f))); glVertex2f(6/9.0f, 1.0f-((i+1)/(P_LZ*2+1.0f))); 
	}
	// Mating:
	for(M=i=0; i<(P_LC*2+1); i++)
	  if( cd[i] > M ) 
	    M = cd[i];
	for(i=0; i<(P_LC*2+1); i++) {
	  glColor3f(0.0f, ((float)cd[i])/M, 0.0f);
	  glVertex2f(6/9.0f, 1.0f-((i+1)/(P_LC*2+1.0f))); glVertex2f(6/9.0f, 1.0f-( i   /(P_LC*2+1.0f)));
	  glVertex2f(7/9.0f, 1.0f-( i   /(P_LC*2+1.0f))); glVertex2f(7/9.0f, 1.0f-((i+1)/(P_LC*2+1.0f))); 
	}
	for(M=i=0; i<(P_LM*2+1); i++)
	  if( md[i] > M ) 
	    M = md[i];
	for(i=0; i<(P_LM*2+1); i++) {
	  glColor3f(0.0f, ((float)md[i])/M, ((float)md[i])/M);
	  glVertex2f(7/9.0f, 1.0f-((i+1)/(P_LM*2+1.0f))); glVertex2f(7/9.0f, 1.0f-( i   /(P_LM*2+1.0f)));
	  glVertex2f(8/9.0f, 1.0f-( i   /(P_LM*2+1.0f))); glVertex2f(8/9.0f, 1.0f-((i+1)/(P_LM*2+1.0f))); 
	}
	for(M=i=0; i<(P_LF*2+1); i++)
	  if( fd[i] > M ) 
	    M = fd[i];
	for(i=0; i<(P_LF*2+1); i++) {
	  glColor3f(((float)fd[i])/M, 0.0f, ((float)fd[i])/M);
	  glVertex2f(8/9.0f, 1.0f-((i+1)/(P_LF*2+1.0f))); glVertex2f(8/9.0f, 1.0f-( i   /(P_LF*2+1.0f)));
	  glVertex2f(9/9.0f, 1.0f-( i   /(P_LF*2+1.0f))); glVertex2f(9/9.0f, 1.0f-((i+1)/(P_LF*2+1.0f))); 
	}

	// End the mess of quads we just drew ;)
	glEnd();
	
	if( (P_WIDTH <= 4) && (P_HEIGHT <= 4) ) {
	  //
	  // If there are too many demes, the numerical data doesn't fit on the display.
	  //
	  
	  // Add labels for mean
	  White();
	  sprintf(buf,"%.0lf",mx*100.0);
	  glRasterPos2f(0.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.1f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",my*100.0);
	  glRasterPos2f(1.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.1f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",mz*100.0);
	  glRasterPos2f(2.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.1f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",mX*100.0);
	  glRasterPos2f(3.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.1f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",mY*100.0);
	  glRasterPos2f(4.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.1f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",mZ*100.0);
	  glRasterPos2f(5.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.1f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",mc*100.0);
	  glRasterPos2f(6.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.1f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",mm*100.0);
	  glRasterPos2f(7.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.1f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",mf*100.0);
	  glRasterPos2f(8.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.1f); 
	  printGLf(w->glw->font,"%s",buf);
	  
	  // Add labels for std.dev
	  White();
	  sprintf(buf,"%.0lf",vx*100.0);
	  glRasterPos2f(0.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.9f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",vy*100.0);
	  glRasterPos2f(1.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.9f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",vz*100.0);
	  glRasterPos2f(2.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.9f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",vX*100.0);
	  glRasterPos2f(3.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.9f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",vY*100.0);
	  glRasterPos2f(4.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.9f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",vZ*100.0);
	  glRasterPos2f(5.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.9f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",vc*100.0);
	  glRasterPos2f(6.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.9f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",vm*100.0);
	  glRasterPos2f(7.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.9f); 
	  printGLf(w->glw->font,"%s",buf);
	  sprintf(buf,"%.0lf",vf*100.0);
	  glRasterPos2f(8.0f/9.0f+1.0f/18.0f-((strlen(buf)*6.0f/2.0f)/ScaleX(w,w->w)),0.9f); 
	  printGLf(w->glw->font,"%s",buf);
	  
	  // Add labels for columns
	  White();
	  glRasterPos2f(0.0f/9.0f+1.0f/18.0f-((strlen("x0")*6.0f/2.0f)/ScaleX(w,w->w)),0.05f); 
	  printGLf(w->glw->font,"x0");
	  glRasterPos2f(1.0f/9.0f+1.0f/18.0f-((strlen("y0")*6.0f/2.0f)/ScaleX(w,w->w)),0.05f); 
	  printGLf(w->glw->font,"y0");
	  glRasterPos2f(2.0f/9.0f+1.0f/18.0f-((strlen("z0")*6.0f/2.0f)/ScaleX(w,w->w)),0.05f); 
	  printGLf(w->glw->font,"z0");
	  glRasterPos2f(3.0f/9.0f+1.0f/18.0f-((strlen("x1")*6.0f/2.0f)/ScaleX(w,w->w)),0.05f); 
	  printGLf(w->glw->font,"x1");
	  glRasterPos2f(4.0f/9.0f+1.0f/18.0f-((strlen("y1")*6.0f/2.0f)/ScaleX(w,w->w)),0.05f); 
	  printGLf(w->glw->font,"y1");
	  glRasterPos2f(5.0f/9.0f+1.0f/18.0f-((strlen("z1")*6.0f/2.0f)/ScaleX(w,w->w)),0.05f); 
	  printGLf(w->glw->font,"z1");
	  glRasterPos2f(6.0f/9.0f+1.0f/18.0f-((strlen("c")*6.0f/2.0f)/ScaleX(w,w->w)),0.05f); 
	  printGLf(w->glw->font,"c");
	  glRasterPos2f(7.0f/9.0f+1.0f/18.0f-((strlen("m")*6.0f/2.0f)/ScaleX(w,w->w)),0.05f); 
	  printGLf(w->glw->font,"m",buf);
	  glRasterPos2f(8.0f/9.0f+1.0f/18.0f-((strlen("f")*6.0f/2.0f)/ScaleX(w,w->w)),0.05f); 
	  printGLf(w->glw->font,"f",buf);
	}
      }
	
      // Outline deme
      Yellow();
      glBegin(GL_LINE_LOOP);
      glVertex2f(0.0f,0.0f); glVertex2f(0.0f,1.0f);
      glVertex2f(1.0f,1.0f); glVertex2f(1.0f,0.0f);
      glEnd();

      // Add faint grey column separators
      glColor3f(0.2f,0.2f,0.2f);
      glBegin(GL_LINES);
      for(i=0; i<9; i++) {
	glVertex2f((i+1)/9.0f,0.0f); 
	glVertex2f((i+1)/9.0f,1.0f); 
      }
      glEnd();

      glPopMatrix();
    }
  }

  pthread_mutex_unlock(&StateLock);

  // Highlight selected deme
  glColor3ub(255, 255, 255);
  glLineWidth(2.0f);
  glBegin(GL_LINE_LOOP);
  glVertex2f(((float)d->x)/P_WIDTH,   ((float)d->y)/P_HEIGHT);   
  glVertex2f(((float)d->x)/P_WIDTH,   ((float)d->y+1)/P_HEIGHT);
  glVertex2f(((float)d->x+1)/P_WIDTH, ((float)d->y+1)/P_HEIGHT);
  glVertex2f(((float)d->x+1)/P_WIDTH, ((float)d->y)/P_HEIGHT);
  glEnd();
  glLineWidth(1.0f);
}

////////////////////////////////////////////////////////////

void Demes_Down(widget_t *w, const int x, const int y, const int b)
{
  demes_gui_t *d = (demes_gui_t*)w->wd;  u32b_t nx,ny;

  // We only care about mouse down events within the widget area
  if( (x > ScaleX(w,w->x)) && (x < ScaleX(w,w->x+w->w)) && 
      (y > ScaleY(w,w->y)) && (y < ScaleY(w,w->y+w->h))     ) {
    // Switch on mouse button
    switch( b ) {
    case MOUSE_LEFT:
      // Select new deme
      d->md = 1;
      nx = x/(ScaleX(w,w->w)/P_WIDTH);
      ny = y/(ScaleY(w,w->h)/P_HEIGHT);
      if( (nx != d->x) || (ny != d->y) ) {
	d->x = nx;
	d->y = ny;
	w->update = 1;
      }
      break;
    case MOUSE_RIGHT:
      printf("Writing to PostScript file.\n");
      Demes_PostScript("demes.eps");
      break;
    }
  }

}

void Demes_Up(widget_t *w, const int x, const int y, const int b)
{
  demes_gui_t *d = (demes_gui_t*)w->wd;

  d->md = 0;
}

void Demes_Move(widget_t *w, const int x, const int y)
{
  demes_gui_t *d = (demes_gui_t*)w->wd;  u32b_t nx,ny;

  // Mouse point selection control: only if in bounds
  if( (x > ScaleX(w,w->x)) && (x < ScaleX(w,w->x+w->w)) && 
      (y > ScaleY(w,w->y)) && (y < ScaleY(w,w->y+w->h))     ) {
    if( d->md ) {
      nx = x/(ScaleX(w,w->w)/P_WIDTH);
      ny = y/(ScaleY(w,w->h)/P_HEIGHT);
      if( (nx != d->x) || (ny != d->y) ) {
	d->x = nx;
	d->y = ny;
	w->update = 1;
      }
    }
  }
}

////////////////////////////////////////////////////////////
// Post Script code
////////////////////////////////////////////////////////////

void Demes_PostScript(char *fn)
{
  u32b_t x,y,i,n,M;  gind_t *ind;  static u32b_t *x0d,*x1d,*y0d,*y1d,*z0d,*z1d,*cd,*md,*fd;  FILE *f;
  float sx,sy,tx,ty;  double mx,mX,my,mY,mz,mZ,mc,mm,mf,vx,vX,vy,vY,vz,vZ,vc,vm,vf;  char buf[128];
  
  // Malloc space for the heightmap
  if( !x0d ) {
    // Allocate space for heightmap
    if( !(x0d = malloc((P_LX*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist x0) failed!");
    if( !(x1d = malloc((P_LX*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist x1) failed!");
    if( !(y0d = malloc((P_LY*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist y0) failed!");
    if( !(y1d = malloc((P_LY*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist y1) failed!");
    if( !(z0d = malloc((P_LZ*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist z0) failed!");
    if( !(z1d = malloc((P_LZ*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist z1) failed!");
    if( !(cd  = malloc((P_LC*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist c) failed!");
    if( !(md  = malloc((P_LM*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist m) failed!");
    if( !(fd  = malloc((P_LF*2+1)*sizeof(u32b_t))) ) Error("gui: malloc(dist f) failed!");
  }

  // Open PostScript file
  f = PS_Start(fn, 6.0f, 6.0f);
      
  // Outline deme graph and drawn black background
  PS_Color(f, 1.0f, 1.0f, 1.0f);
  PS_Box(f, 0.0f, 0.0f, 6.0f, 6.0f, 0);
  PS_Color(f, 0.0f, 0.0f, 0.0f);
  PS_Box(f, 0.0f, 0.0f, 6.0f, 6.0f, 1);

  pthread_mutex_lock(&StateLock);

  for(x=0; x<P_WIDTH; x++) {
    for(y=0; y<P_HEIGHT; y++) {
      // Setup for drawing a single deme
      sx = 6.0f/P_WIDTH;
      sy = 6.0f/P_HEIGHT;
      tx = x*sx;
      ty = y*sy;

      // These should be initialized even if there are no inds to process
      mx = mX = my = mY = mz = mZ = mc = mm = mf = 0.0;
      vx = vX = vy = vY = vz = vZ = vc = vm = vf = 0.0;

      if( Stateg->space[x][y].ninds ) {
	// Clear distributions and averages
	memset(x0d, 0, (P_LX*2+1)*sizeof(u32b_t));  memset(x1d, 0, (P_LX*2+1)*sizeof(u32b_t));
	memset(y0d, 0, (P_LY*2+1)*sizeof(u32b_t));  memset(y1d, 0, (P_LY*2+1)*sizeof(u32b_t));
	memset(z0d, 0, (P_LZ*2+1)*sizeof(u32b_t));  memset(z1d, 0, (P_LZ*2+1)*sizeof(u32b_t));
	memset(cd,  0, (P_LC*2+1)*sizeof(u32b_t));  
	memset(md,  0, (P_LM*2+1)*sizeof(u32b_t));
	memset(fd,  0, (P_LF*2+1)*sizeof(u32b_t));
	n = Stateg->space[x][y].ninds;
	for(i=0; i<n; i++) {
	  ind = &Stateg->space[x][y].inds[i];
	  // Sum for average trait value
	  mx += ind->x0;  mX += ind->x1;
	  my += ind->y0;  mY += ind->y1;
	  mz += ind->z0;  mZ += ind->z1;
	  mc += ind->c;   mm += ind->m;   mf += ind->f;
	  // Compute distributions
	  x0d[((int)(ind->x0*(P_LX*2)))]++;  x1d[((int)(ind->x1*(P_LX*2)))]++;
	  y0d[((int)(ind->y0*(P_LY*2)))]++;  y1d[((int)(ind->y1*(P_LY*2)))]++;
	  z0d[((int)(ind->z0*(P_LZ*2)))]++;  z1d[((int)(ind->z1*(P_LZ*2)))]++;
	  cd[((int)(ind->c*(P_LC*2)))]++;    
	  md[((int)(ind->m*(P_LM*2)))]++;
	  fd[((int)(ind->f*(P_LF*2)))]++;
	}
	// Divide for average
	mx /= n;  mX /= n;
	my /= n;  mY /= n;
	mz /= n;  mZ /= n;
	mc /= n;  mm /= n;  mf /= n;
	// Now use the average to compute varience
	for(i=0; i<n; i++) {
	  ind = &Stateg->space[x][y].inds[i];
	  vx += (ind->x0-mx)*(ind->x0-mx);  vX += (ind->x1-mX)*(ind->x1-mX);
	  vy += (ind->y0-my)*(ind->y0-my);  vY += (ind->y1-mY)*(ind->y1-mY);
	  vz += (ind->z0-mz)*(ind->z0-mz);  vZ += (ind->z1-mZ)*(ind->z1-mZ);
	  vc += (ind->c-mc)*(ind->c-mc);    vm += (ind->m-mm)*(ind->m-mm);    vf += (ind->f-mf)*(ind->f-mf);
	  }
	// Divide and sqrt for varience -> std.dev
	vx = sqrt(vx/n);  vX = sqrt(vX/n); 
	vy = sqrt(vy/n);  vY = sqrt(vY/n);
	vz = sqrt(vz/n);  vZ = sqrt(vZ/n);
	vc = sqrt(vc/n);  vm = sqrt(vm/n);
	vf = sqrt(vf/n);

	// Perch height:
	for(M=i=0; i<(P_LX*2+1); i++)
	  if( x0d[i] > M ) 
	    M = x0d[i];
	for(i=0; i<(P_LX*2+1); i++) {
	  PS_Color(f, ((float)x0d[i])/M, 0.0f, 0.0f);
	  PS_Quad(f,
		  tx + sx*(0/9.0f), ty + sy*(1.0f-((i+1)/(P_LX*2+1.0f))),
		  tx + sx*(0/9.0f), ty + sy*(1.0f-( i   /(P_LX*2+1.0f))),
		  tx + sx*(1/9.0f), ty + sy*(1.0f-( i   /(P_LX*2+1.0f))),
		  tx + sx*(1/9.0f), ty + sy*(1.0f-((i+1)/(P_LX*2+1.0f))),
		  1);
	}
	for(M=i=0; i<(P_LY*2+1); i++)
	  if( y0d[i] > M ) 
	    M = y0d[i];
	for(i=0; i<(P_LY*2+1); i++) {
	  PS_Color(f, ((float)y0d[i])/M, 0.0f, 0.0f);
	  PS_Quad(f,
		  tx + sx*(1/9.0f), ty + sy*(1.0f-((i+1)/(P_LY*2+1.0f))),
		  tx + sx*(1/9.0f), ty + sy*(1.0f-( i   /(P_LY*2+1.0f))),
		  tx + sx*(2/9.0f), ty + sy*(1.0f-( i   /(P_LY*2+1.0f))),
		  tx + sx*(2/9.0f), ty + sy*(1.0f-((i+1)/(P_LY*2+1.0f))),
		  1);
	}
	for(M=i=0; i<(P_LZ*2+1); i++)
	  if( z0d[i] > M ) 
	    M = z0d[i];
	for(i=0; i<(P_LZ*2+1); i++) {
	  PS_Color(f, ((float)z0d[i])/M, 0.0f, 0.0f);
	  PS_Quad(f,
		  tx + sx*(2/9.0f), ty + sy*(1.0f-((i+1)/(P_LZ*2+1.0f))),
		  tx + sx*(2/9.0f), ty + sy*(1.0f-( i   /(P_LZ*2+1.0f))),
		  tx + sx*(3/9.0f), ty + sy*(1.0f-( i   /(P_LZ*2+1.0f))),
		  tx + sx*(3/9.0f), ty + sy*(1.0f-((i+1)/(P_LZ*2+1.0f))),
		  1);
	}
	// Perch diameter:
	for(M=i=0; i<(P_LX*2+1); i++)
	  if( x1d[i] > M ) 
	    M = x1d[i];
	for(i=0; i<(P_LX*2+1); i++) {
	  PS_Color(f, 0.0f, 0.0f, ((float)x1d[i])/M);
	  PS_Quad(f,
		  tx + sx*(3/9.0f), ty + sy*(1.0f-((i+1)/(P_LX*2+1.0f))),
		  tx + sx*(3/9.0f), ty + sy*(1.0f-( i   /(P_LX*2+1.0f))),
		  tx + sx*(4/9.0f), ty + sy*(1.0f-( i   /(P_LX*2+1.0f))),
		  tx + sx*(4/9.0f), ty + sy*(1.0f-((i+1)/(P_LX*2+1.0f))),
		  1);
	}
	for(M=i=0; i<(P_LY*2+1); i++)
	  if( y1d[i] > M ) 
	    M = y1d[i];
	for(i=0; i<(P_LY*2+1); i++) {
	  PS_Color(f, 0.0f, 0.0f, ((float)y1d[i])/M);
	  PS_Quad(f,
		  tx + sx*(4/9.0f), ty + sy*(1.0f-((i+1)/(P_LY*2+1.0f))),
		  tx + sx*(4/9.0f), ty + sy*(1.0f-( i   /(P_LY*2+1.0f))),
		  tx + sx*(5/9.0f), ty + sy*(1.0f-( i   /(P_LY*2+1.0f))),
		  tx + sx*(5/9.0f), ty + sy*(1.0f-((i+1)/(P_LY*2+1.0f))),
		  1);
	}
	for(M=i=0; i<(P_LZ*2+1); i++)
	  if( z1d[i] > M ) 
	    M = z1d[i];
	for(i=0; i<(P_LZ*2+1); i++) {
	  PS_Color(f, 0.0f, 0.0f, ((float)z1d[i])/M);
	  PS_Quad(f,
		  tx + sx*(5/9.0f), ty + sy*(1.0f-((i+1)/(P_LZ*2+1.0f))),
		  tx + sx*(5/9.0f), ty + sy*(1.0f-( i   /(P_LZ*2+1.0f))),
		  tx + sx*(6/9.0f), ty + sy*(1.0f-( i   /(P_LZ*2+1.0f))),
		  tx + sx*(6/9.0f), ty + sy*(1.0f-((i+1)/(P_LZ*2+1.0f))),
		  1);
	}
	// Mating:
	for(M=i=0; i<(P_LC*2+1); i++)
	  if( cd[i] > M ) 
	    M = cd[i];
	for(i=0; i<(P_LC*2+1); i++) {
	  PS_Color(f, 0.0f, ((float)cd[i])/M, 0.0f);
	  PS_Quad(f,
		  tx + sx*(6/9.0f), ty + sy*(1.0f-((i+1)/(P_LC*2+1.0f))),
		  tx + sx*(6/9.0f), ty + sy*(1.0f-( i   /(P_LC*2+1.0f))),
		  tx + sx*(7/9.0f), ty + sy*(1.0f-( i   /(P_LC*2+1.0f))),
		  tx + sx*(7/9.0f), ty + sy*(1.0f-((i+1)/(P_LC*2+1.0f))),
		  1);
	}
	for(M=i=0; i<(P_LM*2+1); i++)
	  if( md[i] > M ) 
	    M = md[i];
	for(i=0; i<(P_LM*2+1); i++) {
	  PS_Color(f, 0.0f, ((float)md[i])/M, ((float)md[i])/M);
	  PS_Quad(f,
		  tx + sx*(7/9.0f), ty + sy*(1.0f-((i+1)/(P_LM*2+1.0f))),
		  tx + sx*(7/9.0f), ty + sy*(1.0f-( i   /(P_LM*2+1.0f))),
		  tx + sx*(8/9.0f), ty + sy*(1.0f-( i   /(P_LM*2+1.0f))),
		  tx + sx*(8/9.0f), ty + sy*(1.0f-((i+1)/(P_LM*2+1.0f))),
		  1);
	}
	for(M=i=0; i<(P_LF*2+1); i++)
	  if( fd[i] > M ) 
	    M = fd[i];
	for(i=0; i<(P_LF*2+1); i++) {
	  PS_Color(f, ((float)fd[i])/M, 0.0f, ((float)fd[i])/M);
	  PS_Quad(f,
		  tx + sx*(8/9.0f), ty + sy*(1.0f-((i+1)/(P_LF*2+1.0f))),
		  tx + sx*(8/9.0f), ty + sy*(1.0f-( i   /(P_LF*2+1.0f))),
		  tx + sx*(9/9.0f), ty + sy*(1.0f-( i   /(P_LF*2+1.0f))),
		  tx + sx*(9/9.0f), ty + sy*(1.0f-((i+1)/(P_LF*2+1.0f))),
		  1);
	}
      } else {
	// Empty deme
	PS_Color(f, 0.0f, 0.0f, 0.0f);
	PS_Box(f, tx, ty, sy*1.0f, sy*1.0f, 1);
      }

      if( (P_WIDTH <= 2) && (P_HEIGHT <= 2) ) {
	//
	// If there are too many demes, the numerical data doesn't fit on the display.
	//

	// Add labels for mean
	PS_Color(f, 1.0f, 1.0f, 1.0f);
	sprintf(buf,"%.0lf",mx*100.0);
	PS_Text(f,tx+sx*(0.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.1f),10,buf); 
	sprintf(buf,"%.0lf",my*100.0);
	PS_Text(f,tx+sx*(1.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.1f),10,buf); 
	sprintf(buf,"%.0lf",mz*100.0);
	PS_Text(f,tx+sx*(2.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.1f),10,buf); 
	sprintf(buf,"%.0lf",mX*100.0);
	PS_Text(f,tx+sx*(3.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.1f),10,buf); 
	sprintf(buf,"%.0lf",mY*100.0);
	PS_Text(f,tx+sx*(4.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.1f),10,buf); 
	sprintf(buf,"%.0lf",mZ*100.0);
	PS_Text(f,tx+sx*(5.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.1f),10,buf); 
	sprintf(buf,"%.0lf",mc*100.0);
	PS_Text(f,tx+sx*(6.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.1f),10,buf); 
	sprintf(buf,"%.0lf",mm*100.0);
	PS_Text(f,tx+sx*(7.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.1f),10,buf); 
	sprintf(buf,"%.0lf",mf*100.0);
	PS_Text(f,tx+sx*(8.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.1f),10,buf); 
	
	// Add labels for std.dev
	sprintf(buf,"%.0lf",vx*100.0);
	PS_Text(f,tx+sx*(0.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.9f),10,buf); 
	sprintf(buf,"%.0lf",vy*100.0);
	PS_Text(f,tx+sx*(1.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.9f),10,buf); 
	sprintf(buf,"%.0lf",vz*100.0);
	PS_Text(f,tx+sx*(2.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.9f),10,buf); 
	sprintf(buf,"%.0lf",vX*100.0);
	PS_Text(f,tx+sx*(3.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.9f),10,buf); 
	sprintf(buf,"%.0lf",vY*100.0);
	PS_Text(f,tx+sx*(4.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.9f),10,buf); 
	sprintf(buf,"%.0lf",vZ*100.0);
	PS_Text(f,tx+sx*(5.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.9f),10,buf); 
	sprintf(buf,"%.0lf",vc*100.0);
	PS_Text(f,tx+sx*(6.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.9f),10,buf); 
	sprintf(buf,"%.0lf",vm*100.0);
	PS_Text(f,tx+sx*(7.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.9f),10,buf); 
	sprintf(buf,"%.0lf",vf*100.0);
	PS_Text(f,tx+sx*(8.0f/9.0f+1.0f/18.0f-(strlen(buf)*1.0f/192.0f)),ty+sy*(0.9f),10,buf); 
	
	// Add labels for columns
	PS_Text(f,tx+sx*(0.0f/9.0f+1.0f/18.0f-(strlen("x0")*1.0f/192.0f)),ty+sy*(0.05f),10,"x0"); 
	PS_Text(f,tx+sx*(1.0f/9.0f+1.0f/18.0f-(strlen("y0")*1.0f/192.0f)),ty+sy*(0.05f),10,"y0"); 
	PS_Text(f,tx+sx*(2.0f/9.0f+1.0f/18.0f-(strlen("z0")*1.0f/192.0f)),ty+sy*(0.05f),10,"z0"); 
	PS_Text(f,tx+sx*(3.0f/9.0f+1.0f/18.0f-(strlen("x1")*1.0f/192.0f)),ty+sy*(0.05f),10,"x1"); 
	PS_Text(f,tx+sx*(4.0f/9.0f+1.0f/18.0f-(strlen("y1")*1.0f/192.0f)),ty+sy*(0.05f),10,"y1"); 
	PS_Text(f,tx+sx*(5.0f/9.0f+1.0f/18.0f-(strlen("z1")*1.0f/192.0f)),ty+sy*(0.05f),10,"z1"); 
	PS_Text(f,tx+sx*(6.0f/9.0f+1.0f/18.0f-(strlen("c")*1.0f/192.0f)), ty+sy*(0.05f),10,"c"); 
	PS_Text(f,tx+sx*(7.0f/9.0f+1.0f/18.0f-(strlen("m")*1.0f/192.0f)), ty+sy*(0.05f),10,"m"); 
	PS_Text(f,tx+sx*(8.0f/9.0f+1.0f/18.0f-(strlen("f")*1.0f/192.0f)), ty+sy*(0.05f),10,"f"); 
      }

      // Add faint vertical column separators
      PS_Color(f, 0.2f, 0.2f, 0.2f);
      for(i=0; i<9; i++)
	PS_Line(f, tx+sx*((i+1)/9.0f), ty+0.0f, tx+sx*((i+1)/9.0f), ty+sy*(1.0f)); 

      // Outline deme
      PS_Color(f, 1.0f, 1.0f, 1.0f);
      PS_Box(f, tx, ty, sy*1.0f, sy*1.0f, 0);
    } 
  }

  pthread_mutex_unlock(&StateLock);

  PS_End(f);
}

////////////////////////////////////////

void Deme_PostScripts()
{
  static float  **hm,**ahm,**vhm;
  static u32b_t   mx,my;  
  FILE   *f;
  float   xp,yp,xs,ys;
  u32b_t  x,y,xr,yr,j,dx,dy,m;
  char    title[64];
  u32b_t  modes[4]  = {DM_X0, DM_X1, DM_Y0, DM_Y1};
  char*   files[4]  = {"demes-x0m.eps","demes-x1m.eps","demes-y0m.eps","demes-y1m.eps"};
  char*   afiles[4] = {"x0m.eps","x1m.eps","y0m.eps","y1m.eps"};
  char*   vfiles[4] = {"x0mv.eps","x1mv.eps","y0mv.eps","y1mv.eps"};
  char    buf[128];

  // In order to use the existing FillHeightMap() function, I need a 
  // deme_gui_t and a demes_gui_t object around.  These will be only 
  // have the needed fields filled in and will not be the "normal"
  // versions of the objects.
  widget_t     w;
  deme_gui_t  *d;

  w.wd  = malloc(sizeof(deme_gui_t));
  d = (deme_gui_t*) w.wd;
  d->dc = malloc(sizeof(demes_gui_t));

  // Malloc space for the heightmap
  if( !hm ) {
    float *p;

    // Find max size needed for all modes
    x = maxi(P_LX*2+1, P_LY*2+1);  y = maxi(P_LX*2+1, P_LY*2+1);
    x = maxi(       x, P_LZ*2+1);  y = maxi(       y, P_LZ*2+1);
    x = maxi(       x, P_LC*2+1);  y = maxi(       y, P_LC*2+1);
    x = maxi(       x, P_LM*2+1);  y = maxi(       y, P_LM*2+1);
    mx = x = maxi(x, P_LF*2+1);  
    my = y = maxi(y, P_LF*2+1);

    // Allocate space for heightmap
    if( !(p = malloc(x*y*sizeof(float))) ) Error("malloc(heightmap) failed!");
    if( !(hm = malloc(x*sizeof(float*))) ) Error("malloc(heightmap) failed!");
    for(j=0; j<x; j++)
      hm[j] = &p[j*y];

    // Space for "average" heightmap
    if( !(p   = malloc(x*y*sizeof(float))) ) Error("malloc(heightmap) failed!");
    if( !(ahm = malloc(x*sizeof(float*)))  ) Error("malloc(heightmap) failed!");
    for(j=0; j<x; j++)
      ahm[j] = &p[j*y];

    // Space for "variance" heightmap
    if( !(p   = malloc(x*y*sizeof(float))) ) Error("malloc(heightmap) failed!");
    if( !(vhm = malloc(x*sizeof(float*)))  ) Error("malloc(heightmap) failed!");
    for(j=0; j<x; j++)
      vhm[j] = &p[j*y];
  }

  // Produce graphs for each "mode" (DM_X0, DM_X1, DM_Y0, DM_Y1, etc)
  for(m=0; m<4; m++) {
    
    // Clear the average heightmap
    for(x=0; x<mx; x++) {
      for(y=0; y<my; y++) {
	ahm[x][y] = 0.0f;
      }
    }

    // Draw the desired graph for each deme, collected into one large EPS image
    {
      // Open PostScript file
      f = PS_Start(files[m], 6.0f, 6.0f);
      
      // Outline deme graph and drawn black background
      PS_Color(f, 1.0f, 1.0f, 1.0f);
      PS_Box(f, 0.0f, 0.0f, 6.0f, 6.0f, 0);
      PS_Color(f, 0.0f, 0.0f, 0.0f);
      PS_Box(f, 0.0f, 0.0f, 6.0f, 6.0f, 1);
      
      // Go through all the demes and make a little mini-2d plot for each showing
      // the trait density matrix that is desired.
      for(dx=0; dx<P_WIDTH; dx++) {
	for(dy=0; dy<P_HEIGHT; dy++) {
	  
	  // Setup the w object and members to select the desired
	  // heightmap parameters / source.
	  // DM_X0, DM_X1, DM_Y0, DM_Y1, DM_Z0, DM_Z1, DM_C, DM_M, DM_F
	  d->xmode = modes[m];
	  d->ymode = DM_M;
	  d->dc->x = dx;
	  d->dc->y = dy;
	  
	  // Fill the heightmap based on the above chosen criteria
	  FillHeightMap(&w, hm, title, &xr, &yr);
	  
	  // Add this deme's heightmap into the average
	  for(x=0; x<xr; x++) {
	    for(y=0; y<yr; y++) {
	      ahm[x][y] += hm[x][y];
	    }
	  }
	  
	  // Fades from white <-> black to represent density
	  for(x=0; x<xr; x++) {
	    for(y=0; y<yr; y++) {
	      // Draw colored quad
	      if( hm[x][y] == -1 ) PS_Color(f, 0.2f,     0.0f,     0.0f);
	      else                 PS_Color(f, hm[x][y], hm[x][y], hm[x][y]);
	      xp = ((6.0f/P_WIDTH) /xr)*x + (6.0f/P_WIDTH) *dx;
	      yp = ((6.0f/P_HEIGHT)/yr)*y + (6.0f/P_HEIGHT)*dy;
	      xs = 6.0f/P_WIDTH;
	      ys = 6.0f/P_HEIGHT;
	      PS_Box(f, xp, yp, xs, ys, 1);
	    }
	  }
	  // Outline this deme a little to tell it apart from others.
	  PS_Color(f, 0.2f, 0.2f, 0.2f);
	  PS_Box(f, (6.0f/P_WIDTH)*dx, (6.0f/P_HEIGHT)*dy, (6.0f/P_WIDTH), (6.0f/P_HEIGHT), 0);
	}
      }
      
      // Draw title
      PS_Color(f, 1.0f, 1.0f, 0.0f);
      PS_Text(f, 3.0f-(strlen(title)*1.0f/192.0f), 3.0f, 10, title); 
      
      // Close and finish the EPS image file
      PS_End(f);
    }

    // Now that the graph with all the demes has been drawn, draw the version averages over all demes.
    {
      // Divide for average
      for(x=0; x<xr; x++) {
	for(y=0; y<yr; y++) {
	  ahm[x][y] /= P_WIDTH*P_HEIGHT;
	}
      }

      // Open PostScript file
      f = PS_Start(afiles[m], 3.0f, 3.0f);
      
      // Outline deme graph and drawn black background
      PS_Color(f, 1.0f, 1.0f, 1.0f);
      PS_Box(f, 0.0f, 0.0f, 3.0f, 3.0f, 0);
      PS_Color(f, 0.0f, 0.0f, 0.0f);
      PS_Box(f, 0.0f, 0.0f, 3.0f, 3.0f, 1);

      // So, instead of drawing the average of the demes' individually scaled heightmaps,
      // we'll draw the average of a heightmap that is scaled over all demes:
      d->xmode = modes[m];
      d->ymode = DM_M;
      d->dc->x = -1;
      d->dc->y = -1;
      FillHeightMap(&w, hm, title, &xr, &yr);

      // Fades from white <-> black to represent density
      for(x=0; x<xr; x++) {
	for(y=0; y<yr; y++) {
	  // Draw colored quad
	  if( hm[x][y] == -1 ) PS_Color(f, 0.2f,      0.0f,      0.0f);
	  else                 PS_Color(f, hm[x][y], hm[x][y], hm[x][y]);
	  xp = (3.0f/xr)*x;
	  yp = (3.0f/yr)*y;
	  xs = 3.0f/xr;
	  ys = 3.0f/yr;
	  PS_Box(f, xp, yp, xs, ys, 1);
	}
      }
      
      // Draw title
      PS_Color(f, 1.0f, 1.0f, 0.0f);
      PS_Text(f, 1.5f-(strlen(title)*1.0f/192.0f), 1.5f, 10, title); 
      
      // Close and finish the EPS image file
      PS_End(f);
    }

    // Now that I have the average over all demes, I can produce a graph of the varience.
    {
      for(dx=0; dx<P_WIDTH; dx++) {
	for(dy=0; dy<P_HEIGHT; dy++) {
	  // Set widget properties as needed
	  d->xmode = modes[m];
	  d->ymode = DM_M;
	  d->dc->x = dx;
	  d->dc->y = dy;

	  // Fill the heightmap based on the above chosen criteria
	  FillHeightMap(&w, hm, title, &xr, &yr);

	  // Sum square of difference between demes for later use in
	  // computing the variance.
	  for(x=0; x<xr; x++) {
	    for(y=0; y<yr; y++) {
	      vhm[x][y] += (hm[x][y]-ahm[x][y])*(hm[x][y]-ahm[x][y]);
	    }
	  }
	}
      }

      // Divide for variance
      for(x=0; x<xr; x++) {
	for(y=0; y<yr; y++) {
	  vhm[x][y] /= P_WIDTH*P_HEIGHT;
	}
      }

      // Open PostScript file
      f = PS_Start(vfiles[m], 3.0f, 3.0f);
      
      // Outline deme graph and drawn black background
      PS_Color(f, 1.0f, 1.0f, 1.0f);
      PS_Box(f, 0.0f, 0.0f, 3.0f, 3.0f, 0);
      PS_Color(f, 0.0f, 0.0f, 0.0f);
      PS_Box(f, 0.0f, 0.0f, 3.0f, 3.0f, 1);

      // Fades from white <-> black to represent density
      for(x=0; x<xr; x++) {
	for(y=0; y<yr; y++) {
	  // Draw colored quad
	  if( vhm[x][y] == -1 ) PS_Color(f, 0.2f,      0.0f,      0.0f);
	  else                  PS_Color(f, vhm[x][y], vhm[x][y], vhm[x][y]);
	  xp = (3.0f/xr)*x;
	  yp = (3.0f/yr)*y;
	  xs = 3.0f/xr;
	  ys = 3.0f/yr;
	  PS_Box(f, xp, yp, xs, ys, 1);
	}
      }
      
      // Draw title
      sprintf(buf,"Variance: %s",title+9);
      PS_Color(f, 1.0f, 1.0f, 0.0f);
      PS_Text(f, 1.5f-(strlen(buf)*1.0f/192.0f), 1.5f, 10, buf); 
      
      // Close and finish the EPS image file
      PS_End(f);
    }

  }
}

////////////////////////////////////////////////////////////////////////////////

#endif // !GUI_DEME_C

#endif // P_GUI
