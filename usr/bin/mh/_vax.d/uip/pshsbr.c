/* pshsbr.c - NNTP client subroutines */
#ifndef	lint
static char ident[] = "@(#)$Id: pshsbr.c,v 1.2 90/11/25 19:05:29 sharpe Exp $";
#endif	lint

/* LINTLIBRARY */

#include "../h/strings.h"
#include "../h/nntp.h"
#include <stdio.h>
#include <signal.h>


#define	NOTOK	(-1)
#define	OK	0
#define	DONE	1

#define	TRM	"."
#define	TRMLEN	(sizeof TRM - 1)

extern int  errno;
extern int  sys_nerr;
extern char *sys_errlist[];

static int  poprint = 0;
static int  pophack = 0;

char    response[BUFSIZ];

static FILE *input;
static FILE *output;

#ifdef	BPOP	/* stupid */
static	int	xtnd_last = -1,
		xtnd_first = 0;
static char	xtnd_name[512];	/* INCREDIBLE HACK!! */
#endif

static int    traverse(), command(), multiline(), getline();
static                putline();
/*  */

#ifndef	RPOP
int     pop_init (host, user, pass, snoop)
#else	RPOP
int     pop_init (host, user, pass, snoop, rpop)
int     rpop;
#endif	RPOP
char   *host,
       *user,
       *pass;
int	snoop;
{
    int     fd1,
            fd2;
#ifndef	RPOP
    int	    rpop = 0;
#endif	RPOP
    char    buffer[BUFSIZ];

    if ((fd1 = client (host, "tcp", "nntp", rpop, response)) == NOTOK)
	return NOTOK;

    if ((fd2 = dup (fd1)) == NOTOK) {
	(void) sprintf (response, "unable to dup connection descriptor: %s",
		errno > 0 && errno < sys_nerr ? sys_errlist[errno]
		: "unknown error");
	(void) close (fd1);
	return NOTOK;
    }
    if (pop_set (fd1, fd2, snoop, (char *)0) == NOTOK)
	return NOTOK;

    (void) signal (SIGPIPE, SIG_IGN);

    switch (getline (response, sizeof response, input)) {
	case OK: 
	    if (poprint)
		fprintf (stderr, "<--- %s\n", response);
	    if (*response < CHAR_ERR)
		return OK;
	    else {
		(void) strcpy (buffer, response);
		(void) command ("QUIT");
		(void) strcpy (response, buffer);
	    }			/* fall */

	case NOTOK: 
	case DONE: 
	    if (poprint)	    
		fprintf (stderr, "%s\n", response);
	    (void) fclose (input);
	    (void) fclose (output);
	    return NOTOK;
    }
/* NOTREACHED */
}

/*  */

int	pop_set (in, out, snoop, myname)
int	in,
	out,
	snoop;
char   *myname;
{
    if (myname && *myname)
	strcpy (xtnd_name, myname);	/* interface from bbc to msh */

    if ((input = fdopen (in, "r")) == NULL
	    || (output = fdopen (out, "w")) == NULL) {
	(void) strcpy (response, "fdopen failed on connection descriptor");
	if (input)
	    (void) fclose (input);
	else
	    (void) close (in);
	(void) close (out);
	return NOTOK;
    }

    poprint = snoop;

    return OK;
}


int	pop_fd (in, out)
char   *in,
       *out;
{
    (void) sprintf (in, "%d", fileno (input));
    (void) sprintf (out, "%d", fileno (output));
    return OK;
}

/*  */

int     pop_stat (nmsgs, nbytes)
int    *nmsgs,
       *nbytes;
{
    char **ap;
    extern char **brkstring();

    if (xtnd_last < 0) { 	/* in msh, xtnd_name is set from myname */
	if (command("GROUP %s", xtnd_name) == NOTOK)
	    return NOTOK;

	ap = brkstring (response, " ", "\n"); /* "211 nart first last ggg" */
	xtnd_first = atoi (ap[2]);
	xtnd_last  = atoi (ap[3]);
    }

    /* nmsgs is not the real nart, but an incredible simuation */
    if (xtnd_last > 0)
	*nmsgs = xtnd_last - xtnd_first + 1;	/* because of holes... */
    else
	*nmsgs = 0;
    *nbytes = xtnd_first;	/* for subtracting offset in msh() */

    return OK;
}

int	pop_exists (action)
int	(*action) ();
{
    if (traverse (action, "XMSGS %d-%d", xtnd_first, xtnd_last) == OK)
	return OK;

    return traverse (action, "XHDR NONAME %d-%d", xtnd_first, xtnd_last);
}


#ifndef	BPOP
int     pop_list (msgno, nmsgs, msgs, bytes)
#else	BPOP
int     pop_list (msgno, nmsgs, msgs, bytes, ids)
int    *ids;
#endif	BPOP
int     msgno,
       *nmsgs,
       *msgs,
       *bytes;
{
    int     i;
#ifndef	BPOP
    int    *ids = NULL;
#endif	not BPOP

    if (msgno) {
	*msgs = *bytes = 0;
	if (command ("STAT %d", msgno) == NOTOK) 
	    return NOTOK;

	if (ids) {
	    *ids = msgno;
	}
	return OK;
    }
    return NOTOK;
}

/*  */

/* VARARGS2 */

static int  traverse (action, fmt, a, b, c, d)
int     (*action) ();
char   *fmt,
       *a,
       *b,
       *c,
       *d;
{
    char    buffer[sizeof response];

    if (command (fmt, a, b, c, d) == NOTOK)
	return NOTOK;
    (void) strcpy (buffer, response);

    for (;;)
	switch (multiline ()) {
	    case NOTOK: 
		return NOTOK;

	    case DONE: 
		(void) strcpy (response, buffer);
		return OK;

	    case OK: 
		(*action) (response);
		break;
	}
}

/*  */

int     pop_dele (msgno)
int     msgno;
{
    return command ("DELE %d", msgno);
}


int     pop_noop () {
    return command ("NOOP");
}


int     pop_rset () {
    return command ("RSET");
}

/*  */

int     pop_top (msgno, lines, action)
int     msgno,
	lines,		/* sadly, ignored */
        (*action) ();
{
    return traverse (action, "HEAD %d", msgno);
}


int     pop_retr (msgno, action)
int     msgno,
        (*action) ();
{
    return traverse (action, "ARTICLE %d", msgno);
}


#ifdef	BPOP

int	pop_xtnd (action, fmt, a, b, c, d)
int     (*action) ();
char   *fmt, *a, *b, *c, *d;
{
    extern char **brkstring();
    char  buffer[BUFSIZ], **ap;

    sprintf (buffer, fmt, a, b, c, d);
    ap = brkstring (buffer, " ", "\n");	/* a hack, i know... */

    if (uleq(ap[0], "x-bboards")) {	/* XTND "X-BBOARDS group */
	/* most of these parameters are meaningless under NNTP. 
	 * bbc.c was modified to set AKA and LEADERS as appropriate,
	 * the rest are left blank.
	 */
	return OK;
    }
    if (uleq (ap[0], "archive") && ap[1]) {
	sprintf (xtnd_name, "%s", ap[1]);		/* save the name */
	xtnd_last = 0;
	xtnd_first = 1;		/* setup to fail in pop_stat */
	return OK;
    }
    if (uleq (ap[0], "bboards")) {

	if (ap[1]) {			/* XTND "BBOARDS group" */
	    sprintf (xtnd_name, "%s", ap[1]);		/* save the name */
	    if (command("GROUP %s", xtnd_name) == NOTOK)
		return NOTOK;

	    strcpy (buffer, response);	/* action must ignore extra args */
	    ap = brkstring (response, " ", "\n");/* "211 nart first last g" */
	    xtnd_first = atoi (ap[2]);
	    xtnd_last  = atoi (ap[3]);

	    (*action) (buffer);		
	    return OK;

	} else {		/* XTND "BBOARDS" */
	    return traverse (action, "LIST", a, b, c, d);
	}
    }
    return NOTOK;	/* unknown XTND command */
}
#endif	BPOP

/*  */

int     pop_quit () {
    int     i;

    i = command ("QUIT");
    (void) pop_done ();

    return i;
}


int     pop_done () {
    (void) fclose (input);
    (void) fclose (output);

    return OK;
}

/*  */

/* VARARGS1 */

static int  command (fmt, a, b, c, d)
char   *fmt,
       *a,
       *b,
       *c,
       *d;
{
    char   *cp,
	    buffer[BUFSIZ];

    (void) sprintf (buffer, fmt, a, b, c, d);
    if (poprint)
	if (pophack) {
	    if (cp = index (buffer, ' '))
		*cp = NULL;
	    fprintf (stderr, "---> %s ********\n", buffer);
	    if (cp)
		*cp = ' ';
	    pophack = 0;
	}
	else
	    fprintf (stderr, "---> %s\n", buffer);

    if (putline (buffer, output) == NOTOK)
	return NOTOK;

    switch (getline (response, sizeof response, input)) {
	case OK: 
	    if (poprint)
		fprintf (stderr, "<--- %s\n", response);
	    return (*response < CHAR_ERR ? OK : NOTOK);

	case NOTOK: 
	case DONE: 
	    if (poprint)	    
		fprintf (stderr, "%s\n", response);
	    return NOTOK;
    }
/* NOTREACHED */
}

static int  multiline () {
    char    buffer[BUFSIZ + TRMLEN];

    if (getline (buffer, sizeof buffer, input) != OK)
	return NOTOK;
#ifdef	DEBUG
    if (poprint)
	fprintf (stderr, "<--- %s\n", response);
#endif	DEBUG
    if (strncmp (buffer, TRM, TRMLEN) == 0) {
	if (buffer[TRMLEN] == NULL)
	    return DONE;
	else
	    (void) strcpy (response, buffer + TRMLEN);
    }
    else
	(void) strcpy (response, buffer);

    return OK;
}

/*  */

static int  getline (s, n, iop)
char   *s;
int     n;
FILE * iop;
{
    int     c;
    char   *p;

    p = s;
    while (--n > 0 && (c = fgetc (iop)) != EOF)
	if ((*p++ = c) == '\n')
	    break;
    if (ferror (iop) && c != EOF) {
	(void) strcpy (response, "error on connection");
	return NOTOK;
    }
    if (c == EOF && p == s) {
	(void) strcpy (response, "connection closed by foreign host");
	return DONE;
    }
    *p = NULL;
    if (*--p == '\n')
	*p = NULL;
    if (*--p == '\r')
	*p = NULL;

    return OK;
}


static  putline (s, iop)
char   *s;
FILE * iop;
{
    (void) fprintf (iop, "%s\r\n", s);
    (void) fflush (iop);
    if (ferror (iop)) {
	(void) strcpy (response, "lost connection");
	return NOTOK;
    }

    return OK;
}
