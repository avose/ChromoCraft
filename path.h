#ifndef PATH_H
#define PATH_H

#include "vector.h"

typedef struct str_path_node_t {
  struct str_path_node_t *last;
  struct str_path_node_t *next;
  vector3_t               position;
} path_node_t, path_t;


////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////
#ifndef PATH_C
extern path_node_t* path_new_node(path_t *path, vector3_t *position);
extern path_t* path_new_path(vector3_t *position);
#endif


#endif
