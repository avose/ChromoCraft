#ifndef GEM_H
#define GEM_H

#include "vector.h"

typedef struct str_gem_t {
  vector3_t color;  // gem color
  double    range;  // gem range
  double    rate;   // gem fire rate
} gem_t;

#endif
