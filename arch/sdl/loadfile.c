#include <stdlib.h>
#include <stdio.h>
#include "loadfile.h"
#include "cfile.h"

const char *loadfile(const char *url, int *size) {
  CFILE *f;
  char *buf;
  long fsize;

  if (!(f = cfopen((char *)url, "rb")))
    return NULL;
  fsize = cfilelength(f);
  if (fsize != (int)fsize)
    return NULL;
  if (!(buf = (char *)malloc(fsize)))
    return NULL;
  cfread(buf, fsize, 1, f);
  cfclose(f);
  *size = fsize;
  return buf;
}
