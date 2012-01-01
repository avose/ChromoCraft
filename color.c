
#include "types.h"
#define COLOR_C
#include "color.h"

// Returns true if the color is black
u32b_t color_is_black(vector3_t *color)
{
  return ( (color->s.x+color->s.y+color->s.z) == 0 );
}

// Returns the overall area of the color
double color_area(vector3_t *color)
{
  double area;

  area = color->s.x * color->s.y * color->s.z;

  return ((area>COLOR_MAX_AREA)?(COLOR_MAX_AREA):(area));
}

// Returns the color's area within the specified special.
// Special defined by: ( r in [1,4], g in [1,4], b in [1,4] )
double color_special_area(vector3_t *color, u32b_t r, u32b_t g, u32b_t b)
{
  vector3_t c;
  double    area;

  // Find out how far into the special we are along each axis
  c.s.x = color->s.x - 64*(r-1);
  c.s.y = color->s.y - 64*(g-1);
  c.s.z = color->s.z - 64*(b-1);

  // If any axis is not in the special return 0
  if( (c.s.x < 0) || (c.s.y < 0) || (c.s.z < 0) ) {
    return 0;
  }

  area = c.s.x * c.s.y * c.s.z;

  return ((area>COLOR_MAX_SPECIAL_AREA)?(COLOR_MAX_SPECIAL_AREA):(area));
}
