

#ifndef lint
static	char	*sccsid = "@(#)sid_ab.c	4.1	(ULTRIX)	7/17/90";
#endif lint

# include	"../hdr/defines.h"


char *
sid_ab(p,sp)
register char *p;
register struct sid *sp;
{
	extern	char	*satoi();
	if (*(p = satoi(p,&sp->s_rel)) == '.')
		p++;
	if (*(p = satoi(p,&sp->s_lev)) == '.')
		p++;
	if (*(p = satoi(p,&sp->s_br)) == '.')
		p++;
	p = satoi(p,&sp->s_seq);
	return(p);
}
