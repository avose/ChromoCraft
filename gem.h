#ifndef GEM_H
#define GEM_H

#include "vector.h"

typedef struct str_gem_t {
  vector3_t color;  // gem color
  double    range;  // gem range
  double    rate;   // gem fire rate
} gem_t;


#ifndef GEM_C
extern void gem_mix_gems(gem_t *gem1, gem_t *gem2, gem_t *gem_out);
#endif

#endif
