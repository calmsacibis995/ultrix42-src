/*	@(#)msdup_debug.h	4.1	(ULTRIX)	2/19/91	*/
/*
 * DUP Debugging macros.
 */

/* Ensure file is loaded only once. */
#ifndef DEBUG_h 
#define DEBUG_h

#ifndef MSDUP_DEBUG
#define MSDUP_DEBUG 0
#endif

#if MSDUP_DEBUG > 0
#  define cprint1(x) bprintf x
#  if MSDUP_DEBUG > 1
#    define cprint2(x) bprintf x
#    if MSDUP_DEBUG > 2
#      define cprint3(x) bprintf x
#    else /* MSDUP_DEBUG > 2 */
#      define cprint3(x) ;
#    endif /* MSDUP_DEBUG > 2 */
#  else /* MSDUP_DEBUG > 1 */
#    define cprint2(x) ;
#    define cprint3(x) ;
#  endif /* MSDUP_DEBUG > 1 */
#else /* MSDUP_DEBUG > 0 */
#  define cprint1(x) ;
#  define cprint2(x) ;
#  define cprint3(x) ;
#  define bcpy(x, y) ;
#endif /* MSDUP_DEBUG > 0 */

#endif	/* DEBUG_h */
