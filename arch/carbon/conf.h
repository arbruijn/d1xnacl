/* conf.h.  Generated by configure.  */
/* conf.h.in.  Generated from configure.ac by autoheader.  */
// Modified by Chris to work for an Apple computer with OS 9 or above

/* Define to enable console */
/* #undef CONSOLE */

/* d2x major version */
#define D1XMAJOR "0"

/* d2x minor version */
#define D1XMINOR "5"

/* d2x micro version */
#define D1XMICRO "5"

#define PROGRAM_NAME "D1X-Rebirth"

/* Define if you want to build the editor */
/* #undef EDITOR */

/* Define to 1 if you have the <netipx/ipx.h> header file. */
/* #undef HAVE_NETIPX_IPX_H */

/* Define if you want to build for mac datafiles */
//#define MACDATA 

/* Define to disable asserts, int3, etc. */
/* #undef NDEBUG */

/* Define if you want an assembler free build */
#define NO_ASM 

/* Define if you want an OpenGL build */
//#define OGL

/* Define for a "release" build */
/* #undef RELEASE */

/* Define this to be the shared game directory root */
#define SHAREPATH "."
#define DESCENT_DATA_PATH SHAREPATH

/* Define if your processor needs data to be word-aligned */
/* #undef WORDS_NEED_ALIGNMENT */


        /* General defines */
//#define VERSION_NAME PACKAGE_STRING
#define NMONO 1

#if defined(__APPLE__) && defined(__MACH__)
# define __unix__
/* Define if you want a network build */
# define NETWORK
#else // Mac OS 9
# ifndef __MWERKS__
#  define inline
# endif
#endif	// OS 9/X

#define SDL_INPUT 1
