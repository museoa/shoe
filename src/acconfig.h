/* $Id: acconfig.h,v 1.3 1999/10/04 23:02:11 noring Exp $ */

#ifndef __UNIVERSE_H__
#define __UNIVERSE_H__

/* This must apparently always be defined. */
#ifndef POSIX_SOURCE
#define POSIX_SOURCE
#endif

/*
 * The following four are used by smartlink.
 */

/* Define this if your ld sets the run path with -rpath. */
#undef USE_RPATH

/* Define this if your ld sets the run path with -R. */
#undef USE_R

/* Define this if your ld uses -rpath, but your cc wants -Wl, -rpath. */
#undef USE_Wl

/* Define this if your ld uses -R, but your cc wants -Wl, -R. */
#undef USE_Wl_R

/* Define this if your ld doesn't have an option to set the run path. */
#undef USE_LD_LIBRARY_PATH

@TOP@
/* Byteorder of your computer, most machines use 4321, x86 use 1234. */
#define SHOE_BYTEORDER 0

@BOTTOM@

#endif /* __UNIVERSE_H__ */
