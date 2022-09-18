/*	@(#)execargs.h	1.2	*/
#include <ansi_compat.h>
#if __vax
char **execargs = (char**)(0x7ffffffc);
#endif

#if __pdp11
char **execargs = (char**)(-2);
#endif

#if __u3b || __u3b5 || __gould
/* empty till we can figure out what to do for the shell */
#endif
