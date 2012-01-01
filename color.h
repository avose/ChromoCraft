#ifndef COLOR_H
#define COLOR_H


#define COLOR_MAX_COLOR        256.0
#define COLOR_MAX_AREA         16777216.0
#define COLOR_MAX_SPECIAL_AREA 262144.0

#include "types.h"
#include "vector.h"


////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////
#ifndef COLOR_C
// Returns the overall area of the color
extern double color_area(vector3_t *color);   

// Returns the colors area within the specified special 
// special defined by: ( r in [1,4], g in [1,4], b in [1,4] )
extern double color_special_area(vector3_t *color, int r, int g, int b);

extern u32b_t color_is_black(vector3_t *color);
#endif





#endif
