#ifndef VECTOR_C
#define VECTOR_C

#include <math.h>

#include "vector.h"


////////////////////////////////////////////////////////////

//
// l = sqrt(pow(v[i],2));
//
double vector3_length(vector3_t *v)
{
  return sqrt( v->a[0]*v->a[0] + v->a[1]*v->a[1] + v->a[2]*v->a[2] );
}

//
// v2[i] = v1[i]*s
//
vector3_t* vector3_mult_scalar(vector3_t *v1, vector3_t *v2, const double s)
{
  v2->s.x = v1->s.x * s;
  v2->s.y = v1->s.y * s;
  v2->s.z = v1->s.z * s;

  return v2;
}

//
// v2[i] = v1[i] + s
//
vector3_t* vector3_add_scalar(vector3_t *v1, vector3_t *v2, const double s)
{
  v2->s.x = v1->s.x + s;
  v2->s.y = v1->s.y + s;
  v2->s.z = v1->s.z + s;
  
  return v2;
}

//
// v3[i] = v1[i] + v2[i]
//
vector3_t* vector3_add_vector(vector3_t *v1, vector3_t *v2, vector3_t *v3)
{
  v3->s.x = v1->s.x + v2->s.x;
  v3->s.y = v1->s.y + v2->s.y;
  v3->s.z = v1->s.z + v2->s.z;
  
  return v3;
}

//
// v3[i] = v1[i] - v2[i]
//
vector3_t* vector3_sub_vector(vector3_t *v1, vector3_t *v2, vector3_t *v3)
{
  v3->s.x = v1->s.x - v2->s.x;
  v3->s.y = v1->s.y - v2->s.y;
  v3->s.z = v1->s.z - v2->s.z;
  
  return v3;
}

//
// v3 = cross_product(v1,v2)
//
vector3_t* vector3_crossprod(vector3_t *v1, vector3_t *v2, vector3_t *v3)
{
  v3->s.x = v1->s.y * v2->s.z - v1->s.z * v2->s.y;
  v3->s.y = v1->s.z * v2->s.x - v1->s.x * v2->s.z;
  v3->s.z = v1->s.x * v2->s.y - v1->s.y * v2->s.x;

  return v3;
}

//
// v2 = normalize(v1)
//
vector3_t* vector3_normalize(vector3_t *v1, vector3_t *v2)
{
  const double l = sqrt( v1->s.x*v1->s.x + v1->s.y*v1->s.y + v1->s.z*v1->s.z);

  v2->s.x = v1->s.x / l;
  v2->s.y = v1->s.y / l;
  v2->s.z = v1->s.z / l;

  return v2;
}

//
// v2[i] = v1[i]
//
vector3_t* vector3_copy(vector3_t *v1, vector3_t *v2)
{
  v2->s.x = v1->s.x;
  v2->s.y = v1->s.y;
  v2->s.z = v1->s.z;
  
  return v2;
}

////////////////////////////////////////////////////////////

//
//
//
vector4_t* vector4_mult_scalar(vector4_t *v1, vector4_t *v2, const double s)
{
  return v2;
}

//
//
//
vector4_t* vector4_add_scalar(vector4_t *v1, vector4_t *v2, const double s)
{
  return v2;
}

//
//
//
vector4_t* vector4_add_vector(vector4_t *v1, vector4_t *v2, vector4_t *v3)
{
  return v3;
}

//
//
//
vector4_t* vector4_sub_vector(vector4_t *v1, vector4_t *v2, vector4_t *v3)
{
  return v3;
}

//
//
//
vector4_t* vector4_copy(vector4_t *v1, vector4_t *v2)
{
  return v2;
}


#endif
