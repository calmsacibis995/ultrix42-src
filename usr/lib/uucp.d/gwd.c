#ifndef lint
static char sccsid[] = "@(#)gwd.c	4.1	(ULTRIX)	7/2/90";
#endif

#include "uucp.h"

/*******
 *	gwd(wkdir)	get working directory
 *
 *	return codes  0 | FAIL
 */

gwd(wkdir)
char *wkdir;
{
	FILE *fp;
	extern char *getwd();
	char *c;

#ifdef ULTRIX
	if (getwd(wkdir)==(char *)0) {
		wkdir[0]='\0';
		return(FAIL);
	}
	return(0);
#else
	*wkdir = '\0';
	if ((fp = popen("pwd 2>&-", "r")) == NULL)
		return(FAIL);
	if (fgets(wkdir, 100, fp) == NULL) {
		pclose(fp);
		return(FAIL);
	}
	if (*(c = wkdir + strlen(wkdir) - 1) == '\n')
		*c = '\0';
	pclose(fp);
	return(0);
#endif
}
