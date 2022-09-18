/*	@(#)unctrl.h	4.1	(ULTRIX)	7/2/90	*/

/*
 * unctrl.h
 *
 */

#ifndef unctrl
extern char	*_unctrl[];

# define	unctrl(ch)	(_unctrl[(unsigned) ch])
#endif
