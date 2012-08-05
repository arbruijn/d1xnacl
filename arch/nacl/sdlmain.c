#include <stdio.h>
#include "args.h"
#include "console.h"

#include "loaddata.h"

extern int game_main(int argc, char *argv[]);

#include "crash_dump/untrusted_crash_dump.h"
#include "sdlmain.h"

const char *hogdata;
const char *pigdata;
int hogdataSize;
int pigdataSize;
static void loaddata(void *runner) {
  hogdata = loadurl(runner, "descent.hog", &hogdataSize);
  fprintf(stderr, "loaded hog %d\n", hogdataSize);
  pigdata = loadurl(runner, "descent.pig", &pigdataSize);
  fprintf(stderr, "loaded pig %d\n", pigdataSize);
}


int sdl_main(int argc, char **argv, void *runner) {
  char *game_argv[] = {"descent", "-debug"};
  int game_argc = 2;
  NaClCrashDumpInit();
  loaddata(runner);
  GameArg.DbgVerbose = CON_DEBUG;
  con_printf(CON_DEBUG, "starting\n");
  game_main(game_argc, game_argv);
  return 0;
}
