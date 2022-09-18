/* x400dsbr.c - X400 client subroutines */

/* LINTLIBRARY */

#include "../h/strings.h"
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

static int  x400print = 0;
static int  x400hack = 0;

char    x400_response[BUFSIZ];

static FILE *input;
static FILE *output;

static int	traverse(), command(), multiline(), getline();
static putline();

/*  */

int     x400d_init (host, user, pass, snoop, rpop)
int     rpop;
char   *host,
       *user,
       *pass;
int	snoop;
{
    int     fd1,
            fd2;
    char    buffer[BUFSIZ];

    if ((fd1 = client (host, "tcp", "x400_deliverd", rpop, x400_response)) == NOTOK)
	return NOTOK;

    if ((fd2 = dup (fd1)) == NOTOK) {
	(void) sprintf(x400_response, "unable to dup connection descriptor: %s",
		errno > 0 && errno < sys_nerr ? sys_errlist[errno]
		: "unknown error");
	(void) close (fd1);
	return NOTOK;
    }
    if (x400d_set (fd1, fd2, snoop) == NOTOK)
	return NOTOK;

    (void) signal (SIGPIPE, SIG_IGN);

    switch (getline (x400_response, sizeof x400_response, input)) {
	case OK: 
	    if (x400print)
		fprintf (stderr, "<--- %s\n", x400_response);
	    if (*x400_response == '+'
		    && command ("USER %s", user) != NOTOK
		    && command ("%s %s", rpop ? "RPOP" : (x400hack++, "PASS"),
					pass) != NOTOK)
		return OK;
	    if (*x400_response != '+') {
		(void) strcpy (buffer, x400_response);
		(void) command ("QUIT");
		(void) strcpy (x400_response, buffer);
	    }			/* fall */

	case NOTOK: 
	case DONE: 
	    if (x400print)	    
		fprintf (stderr, "%s\n", x400_response);
	    (void) fclose (input);
	    (void) fclose (output);
	    return NOTOK;
    }
/* NOTREACHED */
}

/*  */

int	x400d_set (in, out, snoop)
int	in,
	out,
	snoop;
{
    if ((input = fdopen (in, "r")) == NULL
	    || (output = fdopen (out, "w")) == NULL) {
	(void) strcpy (x400_response, "fdopen failed on connection descriptor");
	if (input)
	    (void) fclose (input);
	else
	    (void) close (in);
	(void) close (out);
	return NOTOK;
    }

    x400print = snoop;

    return OK;
}


int	x400d_fd (in, out)
char   *in,
       *out;
{
    (void) sprintf (in, "%d", fileno (input));
    (void) sprintf (out, "%d", fileno (output));
    return OK;
}

/*  */

int     x400d_stat (nmsgs, nbytes)
int    *nmsgs,
       *nbytes;
{
    if (command ("STAT") == NOTOK)
	return NOTOK;

    *nmsgs = *nbytes = 0;
    (void) sscanf (x400_response, "+OK %d %d", nmsgs, nbytes);
    return OK;
}

#ifdef NOT_NEEDED

#ifndef	BPOP
int     x400d_list (msgno, nmsgs, msgs, bytes)
#else	BPOP
int     x400d_list (msgno, nmsgs, msgs, bytes, ids)
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
	if (command ("LIST %d", msgno) == NOTOK)
	    return NOTOK;

	*msgs = *bytes = 0;
	if (ids) {
	    *ids = 0;
	    (void) sscanf (x400_response, "+OK %d %d %d", msgs, bytes, ids);
	}
	else
	    (void) sscanf (x400_response, "+OK %d %d", msgs, bytes);
	return OK;
    }

    if (command ("LIST") == NOTOK)
	return NOTOK;

    for (i = 0; i < *nmsgs; i++)
	switch (multiline ()) {
	    case NOTOK: 
		return NOTOK;
	    case DONE: 
		*nmsgs = ++i;
		return OK;
	    case OK: 
		*msgs = *bytes = 0;
		if (ids) {
		    *ids = 0;
		    (void) sscanf (x400_response, "%d %d %d",
			    msgs++, bytes++, ids++);
		}
		else
		    (void) sscanf (x400_response, "%d %d", msgs++, bytes++);
		break;
	}
    for (;;)
	switch (multiline ()) {
	    case NOTOK: 
		return NOTOK;
	    case DONE: 
		return OK;
	    case OK: 
		break;
	}
}
#endif NOT_NEEDED
/*  */

int     x400d_retr (msgno, action)
int     msgno,
        (*action) ();
{
    return traverse (action, "RETR %d", msgno);
}


/* VARARGS2 */

static int  traverse (action, fmt, a, b, c, d)
int     (*action) ();
char   *fmt,
       *a,
       *b,
       *c,
       *d;
{
    char    buffer[sizeof x400_response];

    if (command (fmt, a, b, c, d) == NOTOK)
	return NOTOK;
    (void) strcpy (buffer, x400_response);

    for (;;)
	switch (multiline ()) {
	    case NOTOK: 
		return NOTOK;

	    case DONE: 
		(void) strcpy (x400_response, buffer);
		return OK;

	    case OK: 
		(*action) (x400_response);
		break;
	}
}

/*  */

int     x400d_dele (msgno)
int     msgno;
{
    return command ("DELE %d", msgno);
}


int     x400d_noop () {
    return command ("NOOP");
}


int     x400d_rset () {
    return command ("RSET");
}

/*  */

int     x400d_top (msgno, lines, action)
int     msgno,
	lines,
        (*action) ();
{
    return traverse (action, "TOP %d %d", msgno, lines);
}

#ifdef NOT_NEEDED
#ifdef	BPOP
int	x400_xtnd (action, fmt, a, b, c, d)
int     (*action) ();
char   *fmt,
       *a,
       *b,
       *c,
       *d;
{
    char buffer[BUFSIZ];

    (void) sprintf (buffer, "XTND %s", fmt);
    return traverse (action, buffer, a, b, c, d);
}
#endif	BPOP
#endif NOT_NEEDED

/*  */

int     x400d_quit () {
    int     i;

    i = command ("QUIT");
    (void) x400d_done ();

    return i;
}


int     x400d_done () {
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
    if (x400print)
	if (x400hack) {
	    if (cp = index (buffer, ' '))
		*cp = NULL;
	    fprintf (stderr, "---> %s ********\n", buffer);
	    if (cp)
		*cp = ' ';
	    x400hack = 0;
	}
	else
	    fprintf (stderr, "---> %s\n", buffer);

    if (putline (buffer, output) == NOTOK)
	return NOTOK;

    switch (getline (x400_response, sizeof x400_response, input)) {
	case OK: 
	    if (x400print)
		fprintf (stderr, "<--- %s\n", x400_response);
	    return (*x400_response == '+' ? OK : NOTOK);

	case NOTOK: 
	case DONE: 
	    if (x400print)	    
		fprintf (stderr, "%s\n", x400_response);
	    return NOTOK;
    }
/* NOTREACHED */
}

static int  multiline () {
    char    buffer[BUFSIZ + TRMLEN];

    if (getline (buffer, sizeof buffer, input) != OK)
	return NOTOK;
    if (strncmp (buffer, TRM, TRMLEN) == 0) {
	if (buffer[TRMLEN] == NULL)
	    return DONE;
	else
	    (void) strcpy (x400_response, buffer + TRMLEN);
    }
    else
	(void) strcpy (x400_response, buffer);

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

/* PDW: Old version :   */
/*  if (ferror (iop)) { */

    if (ferror (iop) && c != EOF) {
	(void) strcpy (x400_response, "error on connection");
	return NOTOK;
    }
    if (c == EOF && p == s) {
	(void) strcpy (x400_response, "connection closed by foreign host");
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
	(void) strcpy (x400_response, "lost connection");
	return NOTOK;
    }

    return OK;
}
