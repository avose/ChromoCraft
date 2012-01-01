#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "vector.h"
#define PATH_C
#include "path.h"

// Appends a node to the end of the path linked list
void path_new_node(path_t *path, vector3_t *position)
{
  path_node_t *node;

  // Allocate a new path node
  if( !(node=malloc(sizeof(path_node_t))) ) {
    Error("Failed to allocate space (%u) for new path node.\n",sizeof(path_node_t));
  }

  // Setup the new node for insertion
  memcpy(&node->position,position,sizeof(vector3_t));
  node->next = path;
  node->last = path->last;

  // Insert the node
  path->last->next = node;
  path->last       = node;
}

path_t* path_new_path(vector3_t *position)
{
  path_t *path;

  // Allocate path
  if( !(path=malloc(sizeof(path_t))) ) {
    Error("Failed to allocate space (%u) for new path.\n",sizeof(path_t));
  }
  path->next = path;
  path->last = path;

  // Fill in position
  memcpy(&path->position,position,sizeof(vector3_t));

  return path;
}
