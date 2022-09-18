/* x400mail.c - MH interface to X400S/SMTP */

/* LINTLIBRARY */

/* This module implements an interface to X400_submitd very similar to the
   MMDF mm_(3) routines.  The x400s() routines herein talk SMTP to the
   X400_submitd process, mapping SMTP reply codes into RP_-style codes.
 */

#ifdef	BSD42
/* Under 4.2BSD, the alarm handing stuff for time-outs will NOT work due to
   the way syscalls get restarted.  This really is not crucial, since we
   expect x400_submitd to be well-behaved and not hang on us.  The only time
   I've ever seen x400_submitd hang was never...
 */
#endif	BSD42

#ifndef	BSD42
#undef	SMTP
#endif	not BSD42
#ifdef	SMTP
#undef	SENDMAIL
#endif	SMTP

#include "../h/strings.h"
#include <stdio.h>
#include "x4mail.h"
#include "../zotnet/mts.h"
#include <ctype.h>
#include <signal.h>

#define	NOTOK	(-1)
#define	OK	0
#define	DONE	1

#define	TRUE	1
#define	FALSE	0

#define	NBITS	((sizeof (int)) * 8)

#define	min(a,b)	((a) < (b) ? (a) : (b))


#define	SM_OPEN	 30
#define	SM_HELO	 20
#define	SM_RSET	 15
#define	SM_MAIL	 40
#define	SM_RCPT	120
#define	SM_DATA	 20
#define	SM_TEXT	120
#define	SM_DOT	120
#define	SM_QUIT	 20
#define	SM_CLOS	 10
#define SM_NOTE	 20   /* PDW: for -note */

/*  */

/* PJS: added 'static' for ANSI C... */
static int	alrmser ();

static int  x400s_addrs = 0;
static int  x400s_alarmed = 0;
#ifndef	SMTP
static int  x400s_child = NOTOK;
#endif	not SMTP
static int  x400s_debug = 0;
static int  x400s_nl = TRUE;
static int  x400s_verbose = 0;

static  FILE *x400s_rfp = NULL;
static  FILE *x400s_wfp = NULL;

static char *x400s_noreply = "No reply text given";
static char *x400s_moreply = "; ";

struct x400 x400s_reply;		/* global... */


void	discard ();
char   *r1bindex ();

/* PJS: ANSI C 'implicit extern' -v- defined static... */
static int	rclient(), x400s_ierror(), x400s_talk(), x400s_wrecord();
static int	x400s_wstream();
static int	x400s_werror(), x400s_hear(), x400s_rrecord(), x400s_rerror();

/*  */

int     x400s_init (client, server, watch, verbose, debug)
register char   *client,
	        *server;
register int     watch,
  	         verbose,
		 debug;
{
    register int    result,
                    sd1,
                    sd2;

    if (watch)
	verbose = TRUE;
    x400s_verbose = verbose;
    x400s_debug = debug;
    if (x400s_rfp != NULL && x400s_wfp != NULL)
	return RP_OK;
#ifndef	SENDMTS
#if	!defined(SENDMTS) || defined(MMDFII)
    if (client == NULL || *client == NULL)
	client = LocalName ();
#endif	not SENDMTS
#endif

    if ((sd1 = rclient (server, "tcp", "x400_submitd")) == NOTOK)
	return RP_BHST;
    if ((sd2 = dup (sd1)) == NOTOK) {
	(void) close (sd1);
	return x400s_ierror ("unable to dup");
    }

    (void) signal (SIGALRM, alrmser);
    (void) signal (SIGPIPE, SIG_IGN);

    if ((x400s_rfp = fdopen (sd1, "r")) == NULL
	    || (x400s_wfp = fdopen (sd2, "w")) == NULL) {
	(void) close (sd1);
	(void) close (sd2);
	x400s_rfp = x400s_wfp = NULL;
	return x400s_ierror ("unable to fdopen");
    }
    x400s_alarmed = 0;
    (void) alarm (SM_OPEN);
    result = x400s_hear ();
    (void) alarm (0);
    switch (result) {
	case 220: 
	    break;

	default: 
	    (void) x400s_end (NOTOK);
	    return RP_RPLY;
    }
    if (client && *client)
	switch (x400s_talk (SM_HELO, "HELO %s", client)) {
	    case 250: 
		break;

	    default: 
		(void) x400s_end (NOTOK);
		return RP_RPLY;
	}

    return RP_OK;
}


static int rclient (server, protocol, service)
char   *server,
       *protocol,
       *service;
{
    int     sd;
    char    response[BUFSIZ];
    extern char	*x400host;

/* PJS: If servers is NULL, then get the mtstailor:x400host entry. */
    if (server == (char *)NULL || *server == '\0')
	server = x400host;
    if ((sd = client (server, protocol, service, FALSE, response)) != NOTOK)
	return sd;

    (void) x400s_ierror ("%s", response);
    return NOTOK;
}

/*  */

int     x400s_winit (mode, from)
register int	mode;
register char   *from;
{
    switch (x400s_talk (SM_MAIL, "%s FROM:<%s>",
		mode == S_SEND ? "SEND" : mode == S_SOML ? "SOML"
		: mode == S_SAML ? "SAML" : "MAIL", from)) {
	case 250: 
	    x400s_addrs = 0;
	    return RP_OK;

	case 500: 
	case 501: 
	case 552: 
	    return RP_PARM;

	default: 
	    return RP_RPLY;
    }
}

/*  */

#ifdef	BERK
/* ARGUSED */
#endif	BERK

int     x400s_wadr (mbox, host, path)
register char   *mbox;
#ifndef	BERK
register
#endif	not BERK
	 char   *host,
		*path;
{
#ifndef	BERK
    switch (x400s_talk (SM_RCPT, host && *host ? "RCPT TO:<%s%s@%s>"
					   : "RCPT TO:<%s%s>",
			     path ? path : "", mbox, host)) {
#else	BERK
    switch (x400s_talk (SM_RCPT, "RCPT TO:%s", mbox)) {
#endif	BERK
	case 250: 
	case 251: 
	    x400s_addrs++;
	    return RP_OK;

	case 421: 
	case 450: 
	case 451: 
	case 452: 
	    return RP_NO;

	case 500: 
	case 501: 
	    return RP_PARM;

	case 550: 
	case 551: 
	case 552: 
	case 553: 
	    return RP_USER;

	default: 
	    return RP_RPLY;
    }
}

/*  */

int     x400s_waend (note) 
int note;
{
    int talk;

    if (note)
	talk = x400s_talk (SM_NOTE, "NOTE");
    else
   	talk = x400s_talk (SM_DATA, "DATA"); 

    switch (talk) {
        case 354: 
            x400s_nl = TRUE;
            return RP_OK;

        case 421: 
	case 451: 
	    return RP_NO;

	case 500: 
	case 501: 
	case 503: 
	case 554: 
	    return RP_NDEL;
    
	default: 
	    return RP_RPLY;
    }
}

/*  */

int     x400s_wtxt (buffer, len)
register char   *buffer;
register int     len;
{
    register int    result;

    x400s_alarmed = 0;
    (void) alarm (SM_TEXT);
    result = x400s_wstream (buffer, len);
    (void) alarm (0);

    return (result == NOTOK ? RP_BHST : RP_OK);
}

/*  */

int     x400s_wtend () {
    if (x400s_wstream ((char *) NULL, 0) == NOTOK)
	return RP_BHST;

    switch (x400s_talk (SM_DOT + 3 * x400s_addrs, ".")) {
	case 250: 
	case 251: 
	    return RP_OK;

	case 451: 
	case 452: 
	default: 
	    return RP_NO;

	case 552: 
	case 554: 
	    return RP_NDEL;
    }
}

/*  */

int     x400s_end (type)
register int     type;
{
    register int    status;
    struct x400 x400s_note;

#ifndef	SMTP
    switch (x400s_child) {
	case NOTOK: 
	case OK: 
	    return RP_OK;

	default: 
	    break;
    }
#endif	not SMTP
    if (x400s_rfp == NULL && x400s_wfp == NULL)
	return RP_OK;

    switch (type) {
	case OK: 
	    (void) x400s_talk (SM_QUIT, "QUIT");
	    break;

	case NOTOK: 
	    x400s_note.code = x400s_reply.code;
	    (void) strncpy (x400s_note.text, x400s_reply.text,
		    x400s_note.length = x400s_reply.length);/* fall */
	case DONE: 
	    if (x400s_talk (SM_RSET, "RSET") == 250 && type == DONE)
		return RP_OK;
#ifndef	SMTP
	    (void) kill (x400s_child, SIGKILL);
	    discard (x400s_rfp);
	    discard (x400s_wfp);
#else	SMTP
	    (void) x400s_talk (SM_QUIT, "QUIT");
#endif	not SMTP
	    if (type == NOTOK) {
		x400s_reply.code = x400s_note.code;
		(void) strncpy (x400s_reply.text, x400s_note.text,
			x400s_reply.length = x400s_note.length);
	    }
	    break;
    }
    if (x400s_rfp != NULL) {
	(void) alarm (SM_CLOS);
	(void) fclose (x400s_rfp);
	(void) alarm (0);
    }
    if (x400s_wfp != NULL) {
	(void) alarm (SM_CLOS);
	(void) fclose (x400s_wfp);
	(void) alarm (0);
    }

#ifndef	SMTP
    status = pidwait (x400s_child);

    x400s_child = NOTOK;
#else	SMTP
    status = 0;
#endif	SMTP
    x400s_rfp = x400s_wfp = NULL;

    return (status ? RP_BHST : RP_OK);
}

/*  */

/* VARARGS */

static int  x400s_ierror (fmt, a, b, c, d)
char   *fmt,
       *a,
       *b,
       *c,
       *d;
{
    (void) sprintf (x400s_reply.text, fmt, a, b, c, d);
    x400s_reply.length = strlen (x400s_reply.text);
    x400s_reply.code = NOTOK;

    return RP_BHST;
}

/*  */

/* VARARGS2 */

static int  x400s_talk (time, fmt, a, b, c, d)
register int     time;
register char   *fmt;
{
    register int    result;
    char    buffer[BUFSIZ];

    (void) sprintf (buffer, fmt, a, b, c, d);
    if (x400s_debug) {
	printf ("=> %s\n", buffer);
	(void) fflush (stdout);
    }

    x400s_alarmed = 0;
    (void) alarm ((unsigned) time);
    if ((result = x400s_wrecord (buffer, strlen (buffer))) != NOTOK)
	result = x400s_hear ();
    (void) alarm (0);

    return result;
}

/*  */

static int  x400s_wrecord (buffer, len)
register char   *buffer;
register int     len;
{
    if (x400s_wfp == NULL)
	return x400s_werror ();

    (void) fwrite (buffer, sizeof *buffer, len, x400s_wfp);
    fputs ("\r\n", x400s_wfp);
    (void) fflush (x400s_wfp);

    return (ferror (x400s_wfp) ? x400s_werror () : OK);
}

/*  */

static int  x400s_wstream (buffer, len)
register char   *buffer;
register int     len;
{
    register char  *bp;
    static char lc = NULL;

    if (x400s_wfp == NULL)
	return x400s_werror ();

    if (buffer == NULL && len == 0) {
	if (lc != '\n')
	    fputs ("\r\n", x400s_wfp);
	lc = NULL;
	return (ferror (x400s_wfp) ? x400s_werror () : OK);
    }

    for (bp = buffer; len > 0; bp++, len--) {
	switch (*bp) {
	    case '\n': 
		x400s_nl = TRUE;
		(void) fputc ('\r', x400s_wfp);
		break;

	    case '.': 
		if (x400s_nl)
		    (void) fputc ('.', x400s_wfp);/* FALL THROUGH */
	    default: 
		x400s_nl = FALSE;
	}
	(void) fputc (*bp, x400s_wfp);
	if (ferror (x400s_wfp))
	    return x400s_werror ();
    }

    if (bp > buffer)
	lc = *--bp;
    return (ferror (x400s_wfp) ? x400s_werror () : OK);
}

/*  */

static int  x400s_werror () {
    x400s_reply.length =
#ifdef	SMTP
	strlen (strcpy (x400s_reply.text, x400s_wfp == NULL ? "no socket opened"
	    : x400s_alarmed ? "write to socket timed out"
	    : "error writing to socket"));
#else	not SMTP
	strlen (strcpy (x400s_reply.text, x400s_wfp == NULL ? "no pipe opened"
	    : x400s_alarmed ? "write to pipe timed out"
	    : "error writing to pipe"));
#endif	not SMTP

    return (x400s_reply.code = NOTOK);
}

/*  */

static int  x400s_hear () {
    register int    i,
                    code,
                    cont,
		    rc,
		    more;
    int     bc;
    register char  *bp,
                   *rp;
    char    buffer[BUFSIZ];

again: ;

    x400s_reply.text[x400s_reply.length = 0] = NULL;

    rp = x400s_reply.text, rc = sizeof x400s_reply.text - 1;
    for (more = FALSE; x400s_rrecord (bp = buffer, &bc) != NOTOK;) {
	if (x400s_debug) {
	    printf ("<= %s\n", buffer);
	    (void) fflush (stdout);
	}

	for (; bc > 0 && (!isascii (*bp) || !isdigit (*bp)); bp++, bc--)
	    continue;

	cont = FALSE;
	code = atoi (bp);
	bp += 3, bc -= 3;
	for (; bc > 0 && isspace (*bp); bp++, bc--)
	    continue;
	if (bc > 0 && *bp == '-') {
	    cont = TRUE;
	    bp++, bc--;
	    for (; bc > 0 && isspace (*bp); bp++, bc--)
		continue;
	}

	if (more) {
	    if (code != x400s_reply.code || cont)
		continue;
	    more = FALSE;
	}
	else {
	    x400s_reply.code = code;
	    more = cont;
	    if (bc <= 0) {
		(void) strcpy (bp = buffer, x400s_noreply);
		bc = strlen (x400s_noreply);
	    }
	}
	if ((i = min (bc, rc)) > 0) {
	    (void) strncpy (rp, bp, i);
	    rp += i, rc -= i;
	    if (more && rc > strlen (x400s_moreply) + 1) {
		(void) strcpy (x400s_reply.text + rc, x400s_moreply);
		rc += strlen (x400s_moreply);
	    }
	}
	if (more)
	    continue;
	if (x400s_reply.code < 100) {
	    if (x400s_verbose) {
		printf ("%s\n", x400s_reply.text);
		(void) fflush (stdout);
	    }
	    goto again;
	}

	x400s_reply.length = rp - x400s_reply.text;
	return x400s_reply.code;
    }

    return NOTOK;
}

/*  */

static int  x400s_rrecord (buffer, len)
register char   *buffer;
register int    *len;
{
    if (x400s_rfp == NULL)
	return x400s_rerror ();

    buffer[*len = 0] = NULL;

    (void) fgets (buffer, BUFSIZ, x400s_rfp);
    *len = strlen (buffer);
    if (ferror (x400s_rfp) || feof (x400s_rfp))
	return x400s_rerror ();
    if (buffer[*len - 1] != '\n')
	while (getc (x400s_rfp) != '\n' && !ferror (x400s_rfp) && !feof (x400s_rfp))
	    continue;
    else
	if (buffer[*len - 2] == '\r')
	    *len -= 1;
    buffer[*len - 1] = NULL;

    return OK;
}

/*  */

static int  x400s_rerror () {
    x400s_reply.length =
#ifdef	SMTP
	strlen (strcpy (x400s_reply.text, x400s_rfp == NULL ? "no socket opened"
	    : x400s_alarmed ? "read from socket timed out"
	    : feof (x400s_rfp) ? "premature end-of-file on socket"
	    : "error reading from socket"));
#else	not SMTP
	strlen (strcpy (x400s_reply.text, x400s_rfp == NULL ? "no pipe opened"
	    : x400s_alarmed ? "read from pipe timed out"
	    : feof (x400s_rfp) ? "premature end-of-file on pipe"
	    : "error reading from pipe"));
#endif	not SMTP

    return (x400s_reply.code = NOTOK);
}

/*  */

/* ARGSUSED */

static	int alrmser (i)
int     i;
{
#ifndef	BSD42
    signal (SIGALRM, alrmser);
#endif	BSD42
    x400s_alarmed++;

    if (x400s_debug) {
	printf ("timed out...\n");
	(void) fflush (stdout);
    }
}

/*  */

#ifdef NOTUSED
char   *rp_string (code)
register int     code;
{
    register char  *text;
    static char buffer[BUFSIZ];

    switch (x400s_reply.code != NOTOK ? code : NOTOK) {
	case RP_AOK:
	    text = "AOK";
	    break;

	case RP_MOK:
	    text = "MOK";
	    break;

	case RP_OK: 
	    text = "OK";
	    break;

	case RP_RPLY: 
	    text = "RPLY";
	    break;

	case RP_BHST: 
	default: 
	    text = "BHST";
	    (void) sprintf (buffer, "[%s] %s", text, x400s_reply.text);
	    return buffer;

	case RP_PARM: 
	    text = "PARM";
	    break;

	case RP_NO: 
	    text = "NO";
	    break;

	case RP_USER: 
	    text = "USER";
	    break;

	case RP_NDEL: 
	    text = "NDEL";
	    break;
    }

    (void) sprintf (buffer, "[%s] %3d %s", text, x400s_reply.code, x400s_reply.text);
    return buffer;
}
#endif NOTUSED
