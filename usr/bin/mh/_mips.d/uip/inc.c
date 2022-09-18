/* PDW: following 3 lines not from MH6.6 */
#ifndef lint
static char	*sccsid = "@(#)inc.c	4.1	(ULTRIX)	11/23/87";
#endif lint

/* inc.c - incorporate messages from a maildrop into a folder */

#include "../h/mh.h"
#ifdef	POP
#include "../h/dropsbr.h"
#endif	POP
#include "../h/formatsbr.h"
#include "../h/scansbr.h"
#include "../zotnet/tws.h"
#include <stdio.h>
#include "../zotnet/mts.h"
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

/* PDW: old version :	  */
/* #ifndef NFSFLOCK	  */
/* int onhup();		  */
/* int onintr();	  */
/* int onquit();	  */
/* int onterm();          */

/* char *newmail;         */
/* FILE *in, *aud = NULL; */

/* #endif NFSFLOCK        */

/*  */
#ifndef	MF
#define	MFminc(a)	(a)
#else	MF
#define	MFminc(a)	0
#endif	MF

#ifndef	POP
#define	POPminc(a)	(a)
#else	POP
#define	POPminc(a)	0
#endif	POP

#ifndef	RPOP
#define	RPOPminc(a)	(a)
#else	RPOP
#define	RPOPminc(a)	0
#endif	RPOP

#ifndef	TMA
#define	TMAminc(a)	(a)
#else	TMA
#define	TMAminc(a)	0
#endif	TMA

#ifndef X400
#define X400minc(a)	(a)
#else X400
#define X400minc(a)	(0)
#endif X400

static struct swit  switches[] = {
#define	AUDSW	0
    "audit audit-file", 0,
#define	NAUDSW	1
    "noaudit", 0,

#define	CHGSW	2
    "changecur", 0,
#define	NCHGSW	3
    "nochangecur", 0,

#define	DECRSW	4
    "decrypt", TMAminc (-7),
#define	NDECRSW	5
    "nodecrypt", TMAminc (-9),

#define	MSW	6
    "file name", 0,

#define	FORMSW	7
    "form formatfile", 0,
#define	FMTSW	8
    "format string", 5,

#define	HOSTSW	9
    "host host", POPminc (-4),
#define	USERSW	10
    "user user", POPminc (-4),
#define	PACKSW	11
    "pack file", POPminc (-4),
#define	NPACKSW	12
    "nopack", POPminc (-6),

#define	RPOPSW	13
    "rpop", RPOPminc (-4),
#define	NRPOPSW	14
    "norpop", RPOPminc (-6),

#define	SILSW	15
    "silent", 0,
#define	NSILSW	16
    "nosilent", 0,

#define	TRNCSW	17
    "truncate", 0,
#define	NTRNCSW	18
    "notruncate", 0,

#define	UUCPSW	19
    "uucp", MFminc (-4),
#define	NUUCPSW	20
    "nouucp", MFminc (-6),

#define	WIDSW	21
    "width columns", 0,

/* PDW: SOURCESW */
#define SOURCESW 22
#if defined (POP) && defined (X400)
    "source x400|file|pop", X400minc(-4),
#endif
#if defined (POP) && !(defined (X400))
    "source file|pop", RPOPminc (-4),
#endif
#if defined (X400) && !(defined (POP))
    "source x400|file", X400minc(-4),
#endif
#if !(defined (X400)) && !(defined (POP))
    "source file", 0,
#endif

#define	HELPSW	23
    "help", 4,

    NULL, NULL
};

/*  */

/* PDW: defines for 'source' flag : */
#define NONESRC 0
#define FILESRC 1	
#ifdef POP
#define POPSRC  2	
#endif POP
#ifdef X400
#define X400SRC 4	
#endif X400

static struct srcent {
    char *name;
    char bitflag;
} srctbl[] = {
#ifdef POP
    "pop", POPSRC,
#endif POP
#ifdef X400
    "x400", X400SRC,
#endif X400
    "file", FILESRC
};

static char all_src = 0;   	/* PDW: "all_src", "bitposn" are used    */
static int bitposn = 0;		/*      when the "-source" option is given */
static char this_src = 0;	/* holds the value of current source option. */
static int done_inc = 0;	/* flag for when mail has been inc'd.        */
static cur_updtd = 0;		/* flag for when current message has been    */
				/* updated.				     */
int real_uid;			/* holds the UID of the user */
extern int  errno;

#if defined (POP) || defined (X400)
static char *file = NULL;
#endif defined (POP) || defined (X400)

#ifdef	POP
int  snoop = 0;
extern char response[];

static int  size;
static long pos;
static long start;
static long stop;

static  int   pd = NOTOK;
static	FILE *pf = NULL;

static int	pop_action (), pop_pack ();
static int	map_count();
#endif	POP

#ifdef X400
int  x400_snoop = 0;
extern char x400_response[];

static int  x400_size;
static long x400_pos;
static long x400_start;
static long x400_stop;

static  int   x400_pd = NOTOK;
static  FILE *x400_pf = NULL;

static int      x400_action (), x400_pack ();
static int      x400_map_count();
#endif X400

/*  * /

/* ARGSUSED */

main (argc, argv)
int	argc;
char   *argv[];
{
    int     chgflag = 1,
	    trnflag = 1,
	    decflag = 1,
            noisy = 1,
	    width = 0,

/* PDW: old version - rtrnflag : */
	    rtrnflag = 0,

#ifdef	MF
	    uucp = 1,
#endif	MF
	    locked = 0,
#ifdef	POP
	    nmsgs,
	    nbytes,
	    p,
#endif	POP
#ifdef  X400
	    x400_nmsgs,
	    x400_nbytes,
	    x400_p,
#endif  X400
	    rpop = 1,
            i,
	    hghnum,
            msgnum,
	    source_arg = 0;   /* PDW: source_arg for use with "-source" flag */

    char   *cp,
           *maildir,
           *folder = NULL,
	   *form = NULL,
	   *format = NULL,
           *audfile = NULL,
           *from = NULL,
/* PDW: host 
	   *host = NULL,  */
	   *user = NULL,
#ifdef X400
	   *x400_user = NULL,
#endif X400
#ifdef	POP
	   *pass = NULL,
#endif	POP
#ifdef  X400
	   *x400_pass = NULL,
#endif  X400
/* PDW: old version : */
/* #ifndef NFSFLOCK   */
           *newmail,
/* PDW: old version : */
/* #endif	      */
            buf[100],
          **ap,
          **argp,
           *nfs,
           *arguments[MAXARGS];
    struct msgs *mp;
    struct stat st,
                s1;

/* PDW: old version : */
/* #ifndef NFSFLOCK   */

    FILE *in, *aud = NULL;
/* PDW: old version : */
/* #endif NFSFLOCK    */

#ifdef	MHE
    FILE *mhe = NULL;
#endif	MHE

    invo_name = r1bindex (argv[0], '/');
    mts_init (invo_name);
#ifdef	POP
    if ((cp = getenv ("MHPOPDEBUG")) && *cp)
	snoop++;
#endif	POP
#ifdef  X400
    if ((cp = getenv ("MHX400DEBUG")) && *cp)
	x400_snoop++;
#endif  X400
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
		    (void) sprintf (buf, "%s [+folder] [switches]", invo_name);
		    help (buf, switches);
		    done (1);

		case AUDSW: 
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    audfile = getcpy (m_maildir (cp));
		    continue;
		case NAUDSW: 
		    audfile = NULL;
		    continue;

		case CHGSW: 
		    chgflag++;
		    continue;
		case NCHGSW: 
		    chgflag = 0;
		    continue;

		case TRNCSW: 

/* PDW: old version - rtrnflag : */
		    rtrnflag++;

		    trnflag++;
		    continue;
		case MSW: 
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    from = path (cp, TFILE);/* fall */
/* PJS: Don't set the source flag, as otherwise just specifying '-file filename'
 * switches off the default behaviour of all sources...
 */
		    all_src |= FILESRC;

		/* Falling through is wrong. If the user gives the command:
		 *
		 * inc -truncate -file /filename
		 *
		 * we will cancel the requested truncate flag.. 
		 *
		 * On the other hand, if the user reverses the order to:
		 *
		 * inc -file /filename -truncate
		 * 
		 * We will do the requested function..
		 *
		 * The default is NOT to truncate the file unless specifically
		 * requested to do so by the user.
		 */
/* PDW: next if stmt from old version : */
		    if (rtrnflag)
			continue;
		
		case NTRNCSW: 
		    trnflag = 0;
		    continue;

		case SILSW: 
		    noisy = 0;
		    continue;
		case NSILSW: 
		    noisy++;
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

		case WIDSW: 
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    width = atoi (cp);
		    continue;

		case DECRSW:
		    decflag++;
		    continue;
		case NDECRSW:
		    decflag = 0;
		    continue;

		case UUCPSW: 
#ifdef	MF
		    uucp++;
#endif	MF
		    continue;
		case NUUCPSW: 
#ifdef	MF
		    uucp = 0;
#endif	MF
		    continue;
/* PDW: host changed to pophost */
		case HOSTSW:
		    if (!(pophost = *argp++) || *pophost == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    x400host = pophost;
		    continue;
		case USERSW:
		    if (!(user = *argp++) || *user == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    continue;
		case PACKSW:
/*
#ifndef	POP
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
#else	POP
		    if (!(file = *argp++) || *file == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
#endif	POP
*/
#if !(defined (POP)) && !(defined (X400))
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
#endif !(undefined (POP)) && !(undefined (X400))

#if defined (POP) || defined (X400) 
		    if (!(file = *argp++) || *file == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
#endif defined (POP) || defined (X400)

		    continue;
		case NPACKSW:
#if defined (POP) || defined (X400)
		    file = NULLCP;
#endif defined (POP) || defined (X400)
	
		    continue;
		case RPOPSW:
		    rpop++;
		    continue;
		case NRPOPSW:
		    rpop = 0;
		    continue;

/* PDW: -source {x400|file|pop} */
		case SOURCESW:
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    
		    source_arg = get_source_arg (cp);
		    if (source_arg == NONESRC)
			    adios (NULLCP, "missing argument to %s", argp[-2]);
		    all_src |= source_arg;
		    continue;
	    }
	if (*cp == '+' || *cp == '@') {
	    if (folder)
		adios (NULLCP, "only one folder at a time!");
	    else
		folder = path (cp + 1, *cp == '+' ? TFOLDER : TSUBCWF);
	}
	else
	    adios (NULLCP, "usage: %s [+folder] [switches]", invo_name);
    }

/*  */

    if (all_src == 0) {        /* if no sources specified, then check all */
	all_src = ~0; 	       /* sources.				*/

/* MMS : if pophost exists then check pop */
#ifdef POP
        if (!pophost || !(*pophost))
                all_src ^= POPSRC;
#endif POP

/* MMS : if x400host exists then check x400 */
#ifdef X400
        if (!x400host || !(*x400host))
               	all_src ^= X400SRC;
#endif X400

    }

    real_uid = getuid ();    /* Store real user id in real_uid */
    getusr ();

    while (all_src != 0) {

        (void) seteuid (0);        /* set effective uid to root and real uid */
	(void) setruid (real_uid); /* to real users id, so that user can only */
			           /* read mail that belongs to him.          */

	/* Set this_src to appropriate source - POP, FILE or X400 */
        while ((bitposn < sizeof(all_src) * 8) && 
	        (this_src = all_src & (1 << bitposn)) == 0)
            ++ bitposn;

        if (bitposn == sizeof(all_src) * 8)
            break;
        all_src ^= this_src;	
#ifdef	POP
        if (pophost && !*pophost)
            pophost = NULL;
#endif	POP
#ifdef  X400
        if (x400host && !*x400host)
            x400host = NULL;
#endif  X400
        switch ( this_src ) {        /* PDW: -file or -source file */
            case FILESRC:
                if ( from ) 	 /* -file */
                    newmail = from;
                else {
                    if (((newmail = getenv ("MAILDROP")) && *newmail)
        	        || ((newmail = m_find ("maildrop")) && *newmail))
	                newmail = m_mailpath (newmail);
	            else {
#ifdef	MF
	                if (uucp && umincproc && *umincproc)
	                    get_uucp_mail ();
#endif	MF
	                newmail = concat (MAILDIR, "/", MAILFIL, NULLCP);
 	            }
	        }
	        (void) setruid (0);
	        (void) seteuid (real_uid);
	        if (stat (newmail, &s1) == NOTOK || s1.st_size == 0) {
	            if (all_src == 0 && done_inc == 0)	
	                adios (NULLCP, "no mail to incorporate");
	            continue;
	        }
	        break;

#ifdef	POP
	    case POPSRC:			/* -[no]rpop or -source pop */
	        if ( pophost == NULL) {
		    fprintf (stderr, "%s: no pophost\n", invo_name);
		    if (all_src == 0)
			done (1);
		    continue;
		}
	        if (rpop) {
	            if (user == NULL)
	                user = getusr ();
	            pass = getusr ();
	        }
	        else {
	            fprintf(stderr, "POPhost: ");
	            ruserpass (pophost, &user, &pass);  /* -norpop */
	        }

	        if (pop_init (pophost, user, pass, snoop, rpop) == NOTOK
	            || pop_stat (&nmsgs, &nbytes) == NOTOK) {
		    fprintf (stderr, "%s: %s\n", invo_name, response);
		    if (all_src == 0)
			done (1);
		    continue;
		}
	        (void) setruid (0);
	        (void) seteuid (real_uid);
	        if (nmsgs == 0) {
	            (void) pop_quit ();
	            if (all_src == 0 && done_inc == 0)
	                adios (NULLCP, "no mail to incorporate");
	            continue;
	        }
	        break;
#endif	POP

#ifdef X400
	    case X400SRC:				/* -source x400 */
	        if (x400host == NULL) {
		    fprintf (stderr, "%s: no x400 host\n", invo_name);
		    if (all_src == 0)
			done (1);
		    continue;
		}
	        if (rpop) {
	            if (x400_user == NULL)
		        x400_user = getusr ();
	            x400_pass = getusr ();
	        }
	        else {
	            fprintf(stderr, "X400host: ");
	            ruserpass (x400host, &x400_user, &x400_pass);  /* -norpop */
	        }

	        if (x400d_init (x400host, x400_user, x400_pass, x400_snoop,
	            rpop) == NOTOK
		    || x400d_stat (&x400_nmsgs, &x400_nbytes) == NOTOK) {
		    fprintf (stderr, "%s: %s\n", invo_name, x400_response);
		    if (all_src == 0)
			done (1);
		    continue;
		}
	        (void) setruid(0);
	        (void) seteuid (real_uid);
	        if (x400_nmsgs == 0) {
	            (void) x400d_quit ();
	            if (all_src == 0 && done_inc == 0)
	                adios (NULLCP, "no mail to incorporate");
	            continue;
	        }
	        break;
#endif X400

	    default: 
	        if (all_src == 0 && done_inc == 0)
	            adios (NULLCP, "no mail to incorporate");
		continue;
		/* done (0); */
	
        }           

#ifdef	POP
        if (pophost && file)
	    goto go_to_it;
#endif	POP

#ifdef  X400
	if (x400host && file)
	    goto go_to_it;
#endif  X400

        if (!m_find ("path"))
            free (path ("./", TFOLDER));
        if (!folder)
            folder = defalt;
        maildir = m_maildir (folder);

        if (stat (maildir, &st) == NOTOK) {
            if (errno != ENOENT) {
	 	if (all_src == 0)
                    adios (maildir, "error on folder");
		continue;
	    }
            cp = concat ("Create folder \"", maildir, "\"? ", NULLCP);
            if (noisy && !getanswer (cp))
                done (1);
            free (cp);

	    (void) seteuid (0);
	    (void) setruid (real_uid);
            if (!makedir (maildir)) {
		if (all_src == 0)
                    adios (NULLCP, "unable to create folder %s", maildir);
		continue;
	    }
	    (void) setruid (0);
	    (void) seteuid (real_uid);
        }

        if (chdir (maildir) == NOTOK) {
	    if (all_src == 0)
                adios (maildir, "unable to change directory to");
	    continue;
	}
        if (!(mp = m_gmsg (folder))) {
	    if (all_src == 0)
                adios (NULLCP, "unable to read folder %s", folder);
	    continue;
	}
	(void) setruid (0);
	(void) seteuid (real_uid);

/*  */

#if defined (POP) || defined (X400)
go_to_it: ;
#endif defined (POP) || defined (X400)
	(void) setruid (0);
	(void) seteuid (real_uid);
        if ( this_src == FILESRC ) {
            if (access (newmail, 02) == NOTOK) {
                trnflag = 0;
                if ((in = fopen (newmail, "r")) == NULL) {
		    fprintf (stderr, "unable to read %s\n", newmail);
		    if (all_src == 0)
			done (1);
		    continue;
		}
            }
            else {
                locked++;
                if (trnflag) {
                    (void) signal (SIGHUP, SIG_IGN);
                    (void) signal (SIGINT, SIG_IGN);
                    (void) signal (SIGQUIT, SIG_IGN);
                    (void) signal (SIGTERM, SIG_IGN);
                }
/* PDW: old version :				 */
/* #ifndef NFSFLOCK				 */
/*   	else {					 */
		/* Make sure no stale lock files are
		 * left lying about..
		 */
/*		(void) signal(SIGHUP, onhup);	 */
/*		(void) signal(SIGINT, onintr);	 */
/*		(void) signal(SIGQUIT, onquit);  */
/*		(void) signal(SIGTERM, onterm);	 */
/*	}					 */
/* #endif NFSFLOCK				 */

                if ((in = lkfopen (newmail, "r")) == NULL) {
		    if (all_src == 0)
                        adios (NULLCP, "unable to lock and fopen %s", newmail);
		    continue;
		}
                (void) fstat (fileno(in), &s1);
            }
        }

        if (audfile) {
            if ((i = stat (audfile, &st)) == NOTOK)
                advise (NULLCP, "Creating Receive-Audit: %s", audfile);
            if ((aud = fopen (audfile, "a")) == NULL) {
		if (all_src == 0)
                    adios (audfile, "unable to append to");
		continue;
	    }
            else
                if (i == NOTOK)
    	            (void) chmod (audfile, m_gmprot ());
#if !(defined (POP)) && !(defined (X400))
	    fprintf (aud, from ? "<<inc>> %s  -ms %s\n" : "<<inc>> %s\n",
	            dtimenow (), from);
#endif !(defined (POP)) && !(defined (X400))
#ifdef X400
	    if (this_src == X400SRC) {
	        fprintf (aud, from ? "<<inc>> %s -ms %s\n"
		            : x400host ? "<<inc>> %s -host %s -user %s%s\n"
		            : "<<inc>> %s\n",
	                dtimenow (), from ? from : x400host, x400_user,
                        rpop ? " -rpop" : "");
            }
#endif X400
#ifdef POP
	    if (this_src == POPSRC) {
	       fprintf (aud, from ? "<<inc>> %s -ms %s\n"
		           : pophost ? "<<inc>> %s -host %s -user %s%s\n"
		           : "<<inc>> %s\n",
	               dtimenow (), from ? from : pophost, user,
                       rpop ? " -rpop" : "");
  	    }
#endif POP
        }

#ifdef	MHE
        if (m_find ("mhe")) {
            cp = concat (maildir, "/++", NULLCP);
            i = stat (cp, &st);
            if ((mhe = fopen (cp, "a")) == NULL)
                admonish (cp, "unable to append to");
            else
                if (i == NOTOK)
    	            (void) chmod (cp, m_gmprot ());
	    free (cp);
        }
#endif	MHE

        nfs = new_fs (form, format, FORMAT);

        if (noisy && done_inc == 0) {
      	    done_inc++ ;
	    printf ("Incorporating new mail into %s...\n\n", folder);
	    (void) fflush (stdout);
        }

/*  */

#ifdef	POP
        if (this_src == POPSRC) {
            if (file) {	/* -packf option. */
                file = path (file, TFILE);
                if (stat (file, &st) == NOTOK) {
   	            if (errno != ENOENT) {
			if (all_src == 0)
	                    adios (file, "error on file");
		 	continue;
		    }
	            cp = concat ("Create file \"", file, "\"? ", NULLCP);
	            if (noisy && !getanswer (cp))
	                done (1);
	            free (cp);
	        }
	        msgnum = map_count ();
	        if ((pd = mbx_open (file, getuid (), getgid (), m_gmprot ()))
	                == NOTOK) {
		    if (all_src == 0)
	                adios (file, "unable to open");
		    continue;
		}
	        if ((pf = fdopen (pd, "w+")) == NULL) {
		    if (all_src == 0)
	                adios (NULLCP, "unable to fdopen %s", file);
		    continue;
		}
	    }
	    else {
	        hghnum = msgnum = mp -> hghmsg;
	        if ((mp = m_remsg (mp, 0, mp -> hghmsg + nmsgs)) == NULL) {
		    if (all_src == 0)
	                adios (NULLCP, "unable to allocate folder storage");
		    continue;
		}
	    }

	    for (i = 1; i <= nmsgs; i++) {
	        msgnum++;
	        if (file) {	/* -packf option. */

	            (void) fseek (pf, 0L, 1);
	            pos = ftell (pf);
	            size = 0;
	            (void) fwrite (mmdlm1, 1, strlen (mmdlm1), pf);
	            start = ftell (pf);

	            if (pop_retr (i, pop_pack) == NOTOK) {
			fprintf (stderr, "%s: %s\n", invo_name, response);
			if (all_src == 0)
			    done (1);
			continue;
		    }

	            (void) fseek (pf, 0L, 1);
	            stop = ftell (pf);
	            if (fflush (pf)) {
			fprintf (stderr, "write error on %s\n", file);
			if (all_src == 0)
			    done (1);
			continue;
		    }
	            (void) fseek (pf, start, 0);
	        }
	        else {
	            cp = getcpy (m_name (msgnum));
	            if ((pf = fopen (cp, "w+")) == NULL) {
			fprintf (stderr, "unable to write %s\n", cp);
			if (all_src == 0)
			    done (1);
			continue;
		    }
	            (void) chmod (cp, m_gmprot ());
	            start = stop = 0L;

	            if (pop_retr (i, pop_action) == NOTOK) {
			fprintf (stderr, "%s: %s\n", invo_name, response);
			if (all_src == 0)
			    done (1);
			continue;
		    }
        
	            if (fflush (pf)) {
			fprintf (stderr, "write error on %s\n", cp);
			if (all_src == 0)
			    done (1);
			continue;
		    }
	            (void) fseek (pf, 0L, 0);
	        }
	        switch (p = scan (pf, msgnum, 0, nfs, width,
	  	           file ? 0 : msgnum == mp -> hghmsg + 1 && chgflag &&
			   !cur_updtd,
		           0, stop - start, noisy)) {
		    case SCNEOF: 
		        fprintf (stderr, "%*d  empty\n", DMAXFOLDER, msgnum);
		        break;

		    case SCNERR: 
		    case SCNNUM: 
		        break;

		    case SCNMSG: 
		    case SCNENC:
		    default: 
		        if (aud)
		            fputs (scanl, aud);
#ifdef	MHE
		        if (mhe)
		            fputs (scanl, mhe);
#endif	MHE
		        if (noisy)
		            (void) fflush (stdout);
		        if (!file) {
		            mp -> msgstats[msgnum] = EXISTS;
#ifdef	TMA
		            if (p == SCNENC) {
		                if (mp -> lowsel == 0 || 
                                        msgnum < mp -> lowsel)
			            mp -> lowsel = msgnum;
		                if (mp -> hghsel == 0 ||
                                        msgnum > mp -> hghsel)
			            mp -> hghsel = msgnum;
		                mp -> numsel++;
		                mp -> msgstats[msgnum] |= SELECTED;
		            }
#endif	TMA
		            mp -> msgstats[msgnum] |= UNSEEN;
			    mp -> msgflags |= SEQMOD;
		        }
		        break;
		    }
	        if (file) {	/* -packf option. */
	            (void) fseek (pf, stop, 0);
	            (void) fwrite (mmdlm2, 1, strlen (mmdlm2), pf);
	            if (fflush (pf)) {
			fprintf (stderr, "write error on %s\n",file);
			if (all_src == 0)
			    done (1);
			continue;
		    }
/* PDW: old version :							*/
/*	(void) map_write (file, pd, 0, start, stop, pos, size, noisy);  */

	            (void) map_write (file, pd, 0, 0L, start, stop, pos,
	    		 	      size, noisy);

	        }
	        else {
	            (void) fclose (pf);
	            free (cp);
	        }

	        if (trnflag && pop_dele (i) == NOTOK) {
		    fprintf (stderr, "%s: %s\n", invo_name, response);
		    if (all_src == 0)
			done (1);
		    continue;
		}
	    }
	    if (pop_quit () == NOTOK) {
		fprintf (stderr, "%s: %s\n", invo_name, response);
		if (all_src == 0)
		    done (1);
		continue;
	    }
	    if (file) {
	        (void) mbx_close (file, pd);
	        pd = NOTOK;
	    }
        }
#endif	POP

/*  */

#ifdef X400
        if (this_src == X400SRC) {
            if (file) {  /* -packf option. */
                file = path (file, TFILE);
                if (stat (file, &st) == NOTOK) {
  	            if (errno != ENOENT) {
			fprintf (stderr, "error on file %s\n", file);
			if (all_src == 0)
			    done(1);
			continue;
		    }
	            cp = concat ("Create file \"", file, "\"? ", NULLCP);
	            if (noisy && !getanswer (cp))
	                done (1);
	            free (cp);
	        }
	        msgnum = x400_map_count ();
	        if ((x400_pd = mbx_open (file, getuid (), getgid (), 
		        m_gmprot ())) == NOTOK) {
		    fprintf (stderr, "unable to open %s\n", file);
		    if (all_src == 0)
			done (1);
		    continue;
		}
	        if ((x400_pf = fdopen (x400_pd, "w+")) == NULL) {
		    if (all_src == 0)
		        adios (NULLCP, "unable to fdopen %s", file);
		    continue;
		}
	    }
	    else {
	        hghnum = msgnum = mp -> hghmsg;
	        if ((mp = m_remsg (mp, 0, mp -> hghmsg + x400_nmsgs)) == NULL) {
		    if (all_src == 0)
	                adios (NULLCP, "unable to allocate folder storage");
		    continue;
		}
	    }

	    for (i = 1; i <= x400_nmsgs; i++) {
	        msgnum++;
	        if (file) { /* -packf option. */
	            (void) fseek (x400_pf, 0L, 1);
	            x400_pos = ftell (x400_pf);
	            x400_size = 0;
	            (void) fwrite (mmdlm1, 1, strlen (mmdlm1), x400_pf);
	            x400_start = ftell (x400_pf);

	            if (x400d_retr (i, x400_pack) == NOTOK) {
			fprintf (stderr, "%s: %s\n", invo_name, x400_response);
			if (all_src == 0)
			    done (1);
			continue;
		    }

	            (void) fseek (x400_pf, 0L, 1);
	            x400_stop = ftell (x400_pf);
	            if (fflush (x400_pf)) {
			fprintf (stderr, "write error on %s\n", file);
			if (all_src == 0)
			    done (1);
			continue;
		    }
	            (void) fseek (x400_pf, x400_start, 0);
	        }
	        else {
	            cp = getcpy (m_name (msgnum));
	            if ((x400_pf = fopen (cp, "w+")) == NULL) {
			fprintf (stderr, "unable to write %s\n", cp);
			if (all_src == 0)
			    done (1);
			continue;
		    }
	            (void) chmod (cp, m_gmprot ());
	            x400_start = x400_stop = 0L;

	            if (x400d_retr (i, x400_action) == NOTOK) {
			fprintf (stderr, "%s: %s\n", invo_name, x400_response);
			if (all_src == 0)
			    done (1);
			continue;
		    }
        
	            if (fflush (x400_pf)) {
			fprintf (stderr, "write error on %s\n", cp);
			if (all_src == 0)
			    done (1);
			continue;
		    }
	            (void) fseek (x400_pf, 0L, 0);
	        }
	        switch (x400_p = scan (x400_pf, msgnum, 0, nfs, width,
	   	           file ? 0 : msgnum == mp -> hghmsg + 1 && chgflag &&
			   !cur_updtd,
		           0, x400_stop - x400_start, noisy)) {
		    case SCNEOF: 
		        fprintf (stderr, "%*d  empty\n", DMAXFOLDER, msgnum);
		        break;

		    case SCNERR: 
		    case SCNNUM: 
		        break;

		    case SCNMSG: 
		    case SCNENC:
		    default: 
		        if (aud)
		            fputs (scanl, aud);
#ifdef	MHE
		        if (mhe)
		            fputs (scanl, mhe);
#endif	MHE
		        if (noisy)
		            (void) fflush (stdout);
		        if (!file) {
		            mp -> msgstats[msgnum] = EXISTS;
#ifdef	TMA
		            if (x400_p == SCNENC) {
		                if (mp -> lowsel == 0 || 
                                    msgnum < mp -> lowsel)
		   	            mp -> lowsel = msgnum;
			        if (mp -> hghsel == 0 ||
                                    msgnum > mp -> hghsel)
			            mp -> hghsel = msgnum;
			        mp -> numsel++;
			        mp -> msgstats[msgnum] |= SELECTED;
			    }
#endif	TMA
			    mp -> msgstats[msgnum] |= UNSEEN;
			    mp -> msgflags |= SEQMOD;
		        }
		        break;
		}
	        if (file) { /* -packf option. */
		    (void) fseek (x400_pf, x400_stop, 0);
		    (void) fwrite (mmdlm2, 1, strlen (mmdlm2), x400_pf);
		    if (fflush (x400_pf)) {
			fprintf (stderr, "write error on %s\n", file);
			if (all_src == 0)
			    done (1);
			continue;
		    }
		    (void) map_write (file, x400_pd, 0, 0L, 
		         	      x400_start, x400_stop, x400_pos,
				      x400_size, noisy);

	        }
	        else {
		    (void) fclose (x400_pf);
		    free (cp);
	        }

	        if (trnflag && x400d_dele (i) == NOTOK) {
		    fprintf (stderr, "%s: %s\n", invo_name, x400_response);
		    if (all_src == 0)
			done (1);
		    continue;
		}
	    }
	    if (x400d_quit () == NOTOK) {
	        fprintf (stderr, "%s: %s\n", invo_name, x400_response);
		if (all_src == 0)
		    done (1);
		continue;
	    }
	    if (file) { /* -packf option. */
	        (void) mbx_close (file, x400_pd);
	        x400_pd = NOTOK;
	    }
        }
#endif X400

        if  (this_src == FILESRC) {
            m_unknown (in);		/* the MAGIC invocation... */
            hghnum = msgnum = mp -> hghmsg;
            for (;;) {
	        if (msgnum >= mp -> hghoff)
	            if ((mp = m_remsg (mp, 0, mp -> hghoff + MAXFOLDER)) == NULL) {
			if (all_src == 0)
	                    adios (NULLCP, "unable to allocate folder storage");
			continue;
		    }

	        switch (i = scan (in, msgnum + 1, msgnum + 1, nfs, width,
	                    msgnum == hghnum && chgflag && !cur_updtd,
	                    0,
	                    0L,
	                    noisy)) {
	            case SCNEOF: 
	                break;

	            case SCNERR: 
	                if (aud)
	                    fputs ("inc aborted!\n", aud);
	                adios (NULLCP, "aborted!");

	            case SCNNUM: 
	                adios (NULLCP, "more than %d messages in folder %s, %s not zero'd", MAXFOLDER, folder, newmail);

	            default: 
		        adios (NULLCP, "scan() botch (%d)", i);

	            case SCNMSG:
	            case SCNENC:
		        if (aud)
		            fputs (scanl, aud);
#ifdef	MHE
		        if (mhe)
		            fputs (scanl, mhe);
#endif	MHE
		        if (noisy)
		            (void) fflush (stdout);

		        msgnum++, mp -> hghmsg++;
		        mp -> msgstats[msgnum] = EXISTS;
#ifdef	TMA
		        if (i == SCNENC) {
		            if (mp -> lowsel == 0 || mp -> lowsel > msgnum)
		                mp -> lowsel = msgnum;
		            if (mp -> hghsel == 0 || mp -> hghsel < msgnum)
		                mp -> hghsel = msgnum;
		            mp -> numsel++;
		            mp -> msgstats[msgnum] |= SELECTED;
		        }
#endif	TMA
		        mp -> msgstats[msgnum] |= UNSEEN;
		        mp -> msgflags |= SEQMOD;
		        continue;
	        } 
	        break;
            }
        }

        if (aud)
            (void) fclose (aud);
#ifdef	MHE
        if (mhe)
            (void) fclose (mhe);
#endif	MHE
        if (noisy)
            (void) fflush (stdout);
#ifdef	POP
        if (pophost && file)
            done (0);
#endif	POP
#ifdef  X400
        if (x400host && file)
            done (0);
#endif  X400

/*  */

        if (this_src == FILESRC) {
            if (trnflag) {
                if (stat (newmail, &st) != NOTOK &&
   	            s1.st_mtime != st.st_mtime)
	            advise (NULLCP, "new messages have arrived!\007");
	        else {
	            if ((i = creat (newmail, 0600)) != NOTOK)
	                (void) close (i);
	            else
	                admonish (newmail, "error zero'ing");
	            (void) unlink (map_name (newmail));
	        }
            }
            else
	        if (noisy)
	            fprintf (stderr, "%s not zero'd\n", newmail);
        }

        if (msgnum == hghnum && done_inc != 0)
            admonish (NULLCP, "no messages incorporated");
        else {
            if (cur_updtd == 0 && msgnum != hghnum) {
	        cur_updtd++;
	        m_replace (pfolder, folder);
	        if (chgflag)
	            mp -> curmsg = hghnum + 1;
	        mp -> hghmsg = msgnum;
	        if (mp -> lowmsg == 0)
	            mp -> lowmsg = 1;
	        if (chgflag)		/* sigh... */
	            m_setcur (mp, mp -> curmsg);
            }
        }

	(void) setruid (0);
	(void) seteuid (real_uid);
        if ( this_src == FILESRC ) {
            if (locked)
	        (void) lkfclose (in, newmail);
            else
	        (void) fclose (in);
        }
        m_setvis (mp, 0);
        m_sync (mp);
	(void) seteuid (0);
	(void) setruid (real_uid);
        m_update ();

#ifdef	TMA
        if (decflag && mp -> numsel > 0) {
	    if (noisy) {
	        printf ("\nIncorporating encrypted mail into %s...\n\n",
	            folder);
	        (void) fflush (stdout);
	    }

	    tmastart (0);
	    for (msgnum = mp -> lowsel; msgnum <= mp -> hghsel; msgnum++)
	        if (mp -> msgstats[msgnum] & SELECTED &&
                    decipher (msgnum) == OK) {
	            if ((in = fopen (cp = m_name (msgnum), "r")) == NULL) {
	                admonish (cp, "unable to open message");
	                free (cp);
	                continue;
	            }
	            switch (scan (in, msgnum, 0, nfs, width,
	  	            msgnum == mp -> curmsg,
		            0,
		            fstat (fileno (in), &st) != NOTOK ? (long) st.st_size
		            : 0L,
		            noisy)) {
		        case SCNEOF: 
		            printf ("%*d  empty\n", DMAXFOLDER, msgnum);
		            break;

		        default: 
		            break;
		    }
		    (void) fclose (in);
		    free (cp);
	        }
	    tmastop ();
        
	    if (noisy)
	        (void) fflush (stdout);
        }
#endif	TMA

    } 
    done (0); 
}

/*  */

#if defined (POP) || defined (X400)
void	done (status)
int	status;
{
    if (file && pd != NOTOK)
	(void) mbx_close (file, pd);

    exit (status);
}
#endif defined (POP) || defined (X400)

/*  */

#ifdef MF
get_uucp_mail () {
    int     child_id;
    char    buffer[BUFSIZ];
    struct stat st;

    (void) sprintf (buffer, "%s/%s", UUCPDIR, UUCPFIL);
    if (stat (buffer, &st) == NOTOK || st.st_size == 0)
	return;

    switch (child_id = vfork ()) {
	case NOTOK: 
	    admonish ("fork", "unable to");
	    break;

	case OK: 
	    execlp (umincproc, r1bindex (umincproc, '/'), NULLCP);
	    fprintf (stderr, "unable to exec ");
	    perror (umincproc);
	    _exit (-1);

	default: 
	    (void) pidXwait (child_id, umincproc);
	    break;
    }
}
#endif	MF

/*  */

#ifdef	POP
static int  pop_action (s)
register char *s;
{
    fprintf (pf, "%s\n", s);
    stop += strlen (s) + 1;
}


static int  pop_pack (s)
register char *s;
{
    register int    j;
    char    buffer[BUFSIZ];

    (void) sprintf (buffer, "%s\n", s);
    for (j = 0; (j = stringdex (mmdlm1, buffer)) >= 0; buffer[j]++)
	continue;
    for (j = 0; (j = stringdex (mmdlm2, buffer)) >= 0; buffer[j]++)
	continue;
    fputs (buffer, pf);
    size += strlen (buffer) + 1;
}

static int  map_count () {
    int     md;
    char   *cp;
    struct drop d;
    struct stat st;

    if (stat (file, &st) == NOTOK)
	return 0;
    if ((md = open (cp = map_name (file), 0)) == NOTOK
	    || map_chk (cp, md, &d, (long) st.st_size, 1)) {
	if (md != NOTOK)
	    (void) close (md);
	return 0;
    }
    (void) close (md);
    return (d.d_id);
}
#endif	POP

#ifdef  X400
static int  x400_action (s)
register char *s;
{
    fprintf (x400_pf, "%s\n", s);
    x400_stop += strlen (s) + 1;
}


static int  x400_pack (s)
register char *s;
{
    register int    j;
    char    buffer[BUFSIZ];

    (void) sprintf (buffer, "%s\n", s);
    for (j = 0; (j = stringdex (mmdlm1, buffer)) >= 0; buffer[j]++)
	continue;
    for (j = 0; (j = stringdex (mmdlm2, buffer)) >= 0; buffer[j]++)
	continue;
    fputs (buffer, x400_pf);
    x400_size += strlen (buffer) + 1;
}

static int  x400_map_count () {
    int     md;
    char   *cp;
    struct drop d;
    struct stat st;

    if (stat (file, &st) == NOTOK)
	return 0;
    if ((md = open (cp = map_name (file), 0)) == NOTOK
	    || map_chk (cp, md, &d, (long) st.st_size, 1)) {
	if (md != NOTOK)
	    (void) close (md);
	return 0;
    }
    (void) close (md);
    return (d.d_id);
}
#endif  X400

/* PDW: get_source_arg - gets the argument following "-source" flag */

get_source_arg (cp) 
char *cp;
{
    int numents, loop = 0;

    numents = sizeof (srctbl) / sizeof (srctbl[1]);

    for (loop = 0; loop < numents; loop++)
	if (strcmp (srctbl[loop].name, cp) == 0)
	    break;

    return ((loop == numents) ? NONESRC : srctbl[loop].bitflag);
}

/* PDW: old version : */
/*.sbttl onhup(), onintr(), onquit(), onterm() */
/* Function:
 *
 *	onxxxx	
 *
 * Function Description:
 *
 *	Routines to process termination signals.
 *	ie. Interrupt control flow for termination processing.
 *	(control-c, etc..)
 *
 * Arguments:
 *
 *	none
 *
 * Return values:
 *
 *	none
 *
 * Side Effects:
 *
 *	The existing "lock" file is removed..
 */
/*
onhup()
{
	(void) signal(SIGHUP, SIG_IGN);
	done (lkfclose (in, newmail));
}

onintr()
{
	(void) signal(SIGINT, SIG_IGN);
	done (lkfclose (in, newmail));
}

onquit()
{
	(void) signal(SIGQUIT, SIG_IGN);
	done (lkfclose (in, newmail));
}

onterm()
{
	(void) signal(SIGTERM, SIG_IGN);
	done (lkfclose (in, newmail));
}
*/
/*E onxxxx() */
