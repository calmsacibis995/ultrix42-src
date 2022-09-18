#ifdef lint
static char *sccsid = "@(#)two.twostr.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/* Make a 2 letter code into an integer we can switch on easily */
int fix2; /* hack */
#define	two( s1, s2 )	(s1 + 256 * s2 )
#define	twostr( str )	two( *str, str[ 1 ] )
