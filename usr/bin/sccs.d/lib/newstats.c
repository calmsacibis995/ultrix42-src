

#ifndef lint
static	char	*sccsid = "@(#)newstats.c	4.1	(ULTRIX)	7/17/90";
#endif lint

# include	"../hdr/defines.h"


newstats(pkt,strp,ch)
register struct packet *pkt;
register char *strp;
register char *ch;
{
	char fivech[6];
	repeat(fivech,ch,5);
	sprintf(strp,"%c%c %s/%s/%s\n",CTLCHAR,STATS,fivech,fivech,fivech);
	putline(pkt,strp);
}
