#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <string.h>

#define UTIL_C
#include "util.h"


// fprintf(stderr); exit();
void Error(const char *fmt, ...)
{
  va_list ap;

  if(!fmt) exit(1);
  va_start(ap, fmt);  
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fflush(stderr);
  _Exit(1);
}

// fprintf(stderr);
void Warn(const char *fmt, ...)
{
  va_list ap;

  if(!fmt) return;
  va_start(ap, fmt);  
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fflush(stderr);
}
