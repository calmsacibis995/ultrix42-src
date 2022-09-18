

#ifndef lint
static	char	*sccsid = "@(#)chksid.c	4.1	(ULTRIX)	7/17/90";
#endif lint

# include	"../hdr/defines.h"


chksid(p,sp)
char *p;
register struct sid *sp;
{
	if (*p ||
		(sp->s_rel == 0 && sp->s_lev) ||
		(sp->s_lev == 0 && sp->s_br) ||
		(sp->s_br == 0 && sp->s_seq))
			fatal("invalid sid (co8)");
}
