#include <string.h>
#include "types.h"
#include "color.h"

#define GEM_C
#include "gem.h"


////////////////////////////////////////////////////////////


void gem_mix_gems(gem_t *gem1, gem_t *gem2, gem_t *gem_out)
{
  vector3_t color1,color2,colorf;
  double    l1,l2;
  int       i;

  // Get current lengths, we will use these to scale output.
  l1 = vector3_length(&(gem1->color));
  l2 = vector3_length(&(gem2->color));

  // Normalize the inputs to get their direction.
  vector3_normalize(&(gem1->color),&color1);
  vector3_normalize(&(gem2->color),&color2);

  // Add an normalize to get output direction.
  vector3_add_vector(&color1, &color2, &colorf);
  vector3_normalize(&colorf,&colorf);

  // Scale output based on the length of the inputs.
  vector3_mult_scalar(&colorf, &colorf, (l1+l2));
  memcpy(&(gem_out->color),&colorf,sizeof(vector3_t));

  // Just in case things are too large.
  for(i=0; i<3; i++) {
    if( gem_out->color.a[i] > 255 ) {
      gem_out->color.a[1] = 255;
    }
  }
}


////////////////////////////////////////////////////////////
