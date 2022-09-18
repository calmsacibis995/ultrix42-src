/*	<@(#)tmpfile.c	4.1>	*/
/*LINTLIBRARY*/
/*
 *	tmpfile - return a pointer to an update file that can be
 *		used for scratch. The file will automatically
 *		go away if the program using it terminates.
 *
 *****************************************************************************
 *
 *	Modification History
 *
 * 1	Peter Hack, 1989 October 13
 *	Eliminated references to tmpnam()
 */
#include <stdio.h>

extern FILE *fopen();
extern int unlink();
extern void perror();
extern char *mktemp(), *strcpy(), *strcat();
static char str[L_tmpnam], seed[] = { 'a', 'a', 'a', 'a', '\0' };
				/* add one more char than tmpnam() uses */

FILE *
tmpfile()
{
	char	tfname[L_tmpnam];
	register FILE	*p;
	void mktmpnam();

	mktmpnam(tfname);
	if((p = fopen(tfname, "w+")) == NULL)
		return NULL;
	else
		(void) unlink(tfname);
	return(p);
}


static void
mktmpnam(s)
char	*s;
{
	register char *p, *q;

	p = s;
	(void) strcpy(p, P_tmpdir);
	(void) strcat(p, seed);
	(void) strcat(p, "XXXXXX");

	q = seed;
	while(*q == 'z')
		*q++ = 'a';
	++*q;

	(void) mktemp(p);
}
