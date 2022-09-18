#ifndef lint
static char *sccsid="@(#)subr.c	4.1	(ULTRIX)	7/17/90";
#endif
#include <stdio.h>
#include <pwd.h>
#include "dgate.h"

char dgated[64];
/*
 * Find out what the gateway node is, the gateway account,
 * and set our uid up.
 */
getgateway(mach, user)
char *mach, *user;
{
	register char *bufp;
	register int c;
	register FILE *fp = NULL;
	FILE *fopen();
	char	buf[512];
	int myuid = getuid();
	int myeuid = geteuid();
	char * getenv();

	bufp = getenv("HOME");
	if (bufp != NULL) {
		setreuid(myeuid, myuid);
		sprintf(buf, "%s/.dgateway", bufp);
		fp = fopen(buf, "r");
		setreuid(myuid, myeuid);
	}
	if (fp != NULL)
		setreuid(myuid, myuid);
	else {
		setmyuid();
		fp = fopen(GATEWAY, "r");
	}
	if (fp == NULL) {
		perror(GATEWAY);
		exit(1);
	}
	c = getc(fp);
	c = fillup(fp, c, mach);
	c = fillup(fp, c, user);
	c = fillup(fp, c, dgated);

	if (*user == '\0')
		sprintf(user, "%s", RNAME);
	if (dgated[0] == '\0')
		sprintf(dgated, "%s", RDLOGIND);
	fclose(fp);
}

fillup(fp, c, bufp)
register FILE *fp;
register int c;
register char *bufp;
{
	while (c != EOF && c != ' ' && c != '\t' & c != '\n') {
		*bufp++ = c;
		c = getc(fp);
	}
	*bufp = '\0';
	while (c == ' ' || c == '\t')
		c = getc(fp);
	return(c);
}

/*
 * Look up the gateway account and do a setuid. The main reason that
 * we do this is to take care of root running this program, since
 * root ignores the setuid bit.
 */
setmyuid()
{
	register struct passwd *pw;
	struct passwd *getpwnam();

	pw = getpwnam(RNAME);
	endpwent();
	if (pw == NULL) {
		fprintf(stderr, "cannot find %s in the password file\n", RNAME);
		exit(1);
	}
	setreuid(pw->pw_uid, pw->pw_uid);
}

/*
 * Take a string, put it in single quotes, and the new string.
 * We have to be careful about single quotes in the string.
 *	e.g.
 *		string'quote
 *	get returned as
 *		'string'\''quote'
 * We need to quote things so that the gateway won't expand
 * what we give it.
 */
char *quote(s)
char *s;
{
	register char *t, *p;
	register i;
	char *malloc();

	for (i = 3, t = s; *t; i++, t++) {
		if (*t == '\'')
			i += 3;
	}
	p = malloc(i);

	if (p == NULL) {
		fprintf(stderr, "too many arguments\n");
		exit(1);
	}
	t = p;
	*t++ = '\'';
	while (*s) {
		if ((*t++ = *s++) == '\'') {
			*t++ = '\\';
			*t++ = '\'';
			*t++ = '\'';
		}
	}
	*t++ = '\'';
	*t++ = '\0';
	return(p);
}
