/* scan.c - display a one-line "scan" listing */
#ifndef	lint
static char ident[] = "@(#)$Id: scan.c,v 1.2 90/11/25 19:05:45 sharpe Exp $";
#endif	lint

#include "../h/mh.h"
#include "../h/formatsbr.h"
#include "../h/scansbr.h"
#include "../zotnet/tws.h"
#include <errno.h>
#include <stdio.h>

/*  */

static struct swit switches[] = {
#define	CLRSW	0
    "clear", 0,
#define	NCLRSW	1
    "noclear", 0,

#define	FORMSW	2
    "form formatfile", 0,
#define	FMTSW	3
    "format string", 5,

#define	HEADSW	4
    "header", 0,
#define	NHEADSW	5
    "noheader", 0,

#define	WIDSW	6
    "width columns", 0,

#define	REVSW	7
    "reverse", 0,
#define	NREVSW	8
    "noreverse", 0,

#define	FILESW	9
    "file file", 4,

#define	HELPSW	10
    "help", 4,

    NULL, NULL
};

/*  */

extern int errno;
#ifdef	VAN	/* global for sbr/formatsbr.c - yech! */
extern struct msgs *fmt_current_folder;	
#endif


void	clear_screen ();

/*  */

/* ARGSUSED */

main (argc, argv)
int     argc;
char   *argv[];
{
     int    clearflag = 0,
	    hdrflag = 0,
	    revflag = 0, 	/* used to be #ifdef BERK */
	    width = 0,
            msgp = 0,
	    ontty,
	    state,
            msgnum;
    long    clock;
    char   *cp,
           *maildir,
	   *file = NULL,
           *folder = NULL,
	   *form = NULL,
	   *format = NULL,
            buf[100],
          **ap,
          **argp,
           *nfs,
           *arguments[MAXARGS],
           *msgs[MAXARGS];
    struct msgs *mp;
    FILE * in;

    invo_name = r1bindex (argv[0], '/');
    mts_init (invo_name);
    if ((cp = m_find (invo_name)) != NULL) {
	ap = brkstring (cp = getcpy (cp), " ", "\n");
	ap = copyip (ap, arguments);
    }
    else
	ap = arguments;
    (void) copyip (argv + 1, ap);
    argp = arguments;

/*  */

    while (cp = *argp++) {
	if (*cp == '-')
	    switch (smatch (++cp, switches)) {
		case AMBIGSW: 
		    ambigsw (cp, switches);
		    done (1);
		case UNKWNSW: 
		    adios (NULLCP, "-%s unknown", cp);
		case HELPSW: 
		    (void) sprintf (buf, "%s [+folder] [msgs] [switches]",
			    invo_name);
		    help (buf, switches);
		    done (1);

		case CLRSW: 
		    clearflag++;
		    continue;
		case NCLRSW: 
		    clearflag = 0;
		    continue;

		case FORMSW: 
		    if (!(form = *argp++) || *form == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    format = NULL;
		    continue;
		case FMTSW: 
		    if (!(format = *argp++) || *format == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    form = NULL;
		    continue;

		case HEADSW: 
		    hdrflag++;
		    continue;
		case NHEADSW: 
		    hdrflag = 0;
		    continue;

		case WIDSW: 
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    width = atoi (cp);
		    continue;
		case REVSW:
		    revflag++;
		    continue;
		case NREVSW:
		    revflag = 0;
		    continue;

		case FILESW:
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    file = path (cp, TFILE);
		    continue;
	    }
	if (*cp == '+' || *cp == '@') {
	    if (folder)
		adios (NULLCP, "only one folder at a time!");
	    else
		folder = path (cp + 1, *cp == '+' ? TFOLDER : TSUBCWF);
	}
	else
	    msgs[msgp++] = cp;
    }


    if (!m_find ("path"))
	free (path ("./", TFOLDER));
    if (!msgp)
	msgs[msgp++] = "all";
    if (!folder)
	folder = m_getfolder ();
    maildir = m_maildir (folder);

    nfs = new_fs (form, format, FORMAT);	/* must be before chdir() */

    if (chdir (maildir) == NOTOK)
	adios (maildir, "unable to change directory to");
    if (!(mp = m_gmsg (folder)))
	adios (NULLCP, "unable to read folder %s", folder);
    if (mp -> hghmsg == 0)
	adios (NULLCP, "no messages in %s", folder);

    for (msgnum = 0; msgnum < msgp; msgnum++)
	if (!m_convert (mp, msgs[msgnum]))
	    done(1);
    m_setseq (mp);

    m_replace (pfolder, folder);
    m_sync (mp);
    m_update ();

    ontty = isatty (fileno (stdout));

    if (file) {
	/* we've been asked to scan a maildrop file */
	in = fopen (file, "r");
	if (in == NULL)
		adios (NULLCP, "can't open %s (%d)", file, errno);

	m_unknown (in);
	for (msgnum = 1; ; ++msgnum) {
		state = scan (in, msgnum, -1, nfs, width, 0, hdrflag, 0L, 1);
		if (state != SCNMSG && state != SCNENC)
			break;
	}
	fclose (in);
	done (0);
    }
#ifdef	VAN
    else
	fmt_current_folder = mp;
#endif

/*  */

    for (msgnum = revflag ? mp -> hghsel : mp -> lowsel;
	    (revflag ? msgnum >= mp -> lowsel : msgnum <= mp -> hghsel);
	    msgnum += revflag ? (-1) : 1)
	if (mp -> msgstats[msgnum] & SELECTED) {
	    if ((in = fopen (cp = m_name (msgnum), "r")) == NULL) {
#ifdef	notdef
		if (errno != EACCES)
#endif
		    admonish (cp, "unable to open message");
#ifdef	notdef
		else
		    printf ("%*d  unreadable\n", DMAXFOLDER, msgnum);
#endif
		continue;
	    }

	    if (hdrflag) {
		(void) time (&clock);
		printf ("Folder %-32s%s\n\n", folder,
			dasctime (dlocaltime (&clock), TW_NULL));
	    }
	    switch (state = scan (in, msgnum, 0, nfs, width,
			msgnum == mp -> curmsg,
			hdrflag, 0L, 1)) {
		case SCNMSG: 
		case SCNENC: 
		case SCNERR: 
		    break;

		default: 
		    adios (NULLCP, "scan() botch (%d)", state);

		case SCNEOF: 
#ifdef	notdef
		    printf ("%*d  empty\n", DMAXFOLDER, msgnum);
#else
		    advise (NULLCP, "message %d: empty", msgnum);
#endif
		    break;
	    }
	    hdrflag = 0;
	    (void) fclose (in);
	    if (ontty)
		(void) fflush (stdout);
	}
#ifdef	VAN
    m_sync (mp);	/* because formatsbr might have made changes */
#endif

/*  */

    if (clearflag)
	clear_screen ();

    done (0);
}
