

#ifndef lint
static	char	*sccsid = "@(#)fmterr.c	4.1	(ULTRIX)	7/17/90";
#endif lint

# include	"../hdr/defines.h"


fmterr(pkt)
register struct packet *pkt;
{
	fclose(pkt->p_iop);
	sprintf(Error,"format error at line %u (co4)",pkt->p_slnno);
	fatal(Error);
}
