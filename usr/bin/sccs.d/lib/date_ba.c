

#ifndef lint
static	char	*sccsid = "@(#)date_ba.c	4.1	(ULTRIX)	7/17/90";
#endif lint

# include	"../hdr/defines.h"
# define DO2(p,n,c)	*p++ = n/10 + '0'; *p++ = n%10 + '0'; *p++ = c;


char *
date_ba(bdt,adt)
long *bdt;
char *adt;
{
	register struct tm *lcltm;
	register char *p;
	struct tm *localtime();

	lcltm = localtime(bdt);
	p = adt;
	DO2(p,lcltm->tm_year,'/');
	DO2(p,(lcltm->tm_mon + 1),'/');
	DO2(p,lcltm->tm_mday,' ');
	DO2(p,lcltm->tm_hour,':');
	DO2(p,lcltm->tm_min,':');
	DO2(p,lcltm->tm_sec,0);
	return(adt);
}
