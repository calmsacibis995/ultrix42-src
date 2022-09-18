

#ifndef lint
static	char	*sccsid = "@(#)auxf.c	4.1	(ULTRIX)	7/17/90";
#endif lint


/*
	Figures out names for g-file, l-file, x-file, etc.

	File	Module	g-file	l-file	x-file & rest

	a/s.m	m	m	l.m	a/x.m

	Second argument is letter; 0 means module name is wanted.
*/

#include "defines.h"

char *
auxf(sfile,ch)
register char *sfile;
register char ch;
{
	static char auxfile[FILESIZE];
	register char *snp;

	snp = sname(sfile);

	switch(ch) {

	case 0:
	case 'g':	copy(&snp[2],auxfile);
			break;

	case 'l':	copy(snp,auxfile);
			auxfile[0] = 'l';
			break;

	default:
			copy(sfile,auxfile);
			auxfile[snp-sfile] = ch;
	}
	return(auxfile);
}
