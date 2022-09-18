/* msgchk.c - check for mail */
#include "../h/mh.h"
#include <stdio.h>
#include "../zotnet/mts.h"
#include "../zotnet/tws.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

/*  */

#if 	defined (X400) || defined (POP)
#define	POPminc(a)	0
#else	defined (X400) || defined (POP)
#define	POPminc(a)	(a)
#endif 	defined(X400)  || defined (POP)

#if 	defined (X400) || defined (POP)
#ifdef	RPOP
#define	RPOPminc(a)	0
#else	RPOP
#define	RPOPminc(a)	(a)
#endif	RPOP
#else	defined (X400) || defined (POP)
#define	RPOPminc(a)	(a)
#endif 	defined(X400)  || defined (POP)

static struct swit  switches[] = {
#define	DATESW	0
    "date", 0,
#define	NDATESW	1
    "nodate", 0,

#define	NOTESW	2
    "notify type", 0,
#define	NNOTESW	3
    "nonotify type", 0,

#define	HOSTSW	4
    "host host", POPminc (-4),
#define	USERSW	5
    "user user", POPminc (-4),

#define	RPOPSW	6
    "rpop", RPOPminc (-4),
#define	NRPOPSW	7
    "norpop", RPOPminc (-6),

#define SRCESW  8
    "source file/x400/pop", 0,

#define	HELPSW	9
    "help", 4,

    NULL, NULL
};

/*  */

#define	NT_NONE	0x0
#define	NT_MAIL	0x1
#define	NT_NMAI	0x2
#define	NT_ALL	(NT_MAIL | NT_NMAI)

#define	NONEOK	0x0
#define	UUCPOLD	0x1
#define	UUCPNEW	0x2
#define	UUCPOK	(UUCPOLD | UUCPNEW)
#define	MMDFOLD	0x4
#define	MMDFNEW	0x8
#define	MMDFOK	(MMDFOLD | MMDFNEW)

#ifdef	SYS5
struct passwd	*getpwuid(), *getpwnam();
#endif	SYS5

#ifdef POP
#define POPSRC   1
#endif POP

#define FILESRC  2

#ifdef X400
#define X400SRC  4
#endif x400

static struct srcents {
	char  *name;
	int    bitflag;
} srctbl[] = {
#ifdef POP
	{ "pop",   POPSRC  },
#endif POP
        { "file",  FILESRC },
#ifdef X400
	{ "x400",  X400SRC },
#endif X400
};

/* PJS: ANSI C 'implicit extern' -v- defined static... */
static int	donote(), checkmail();

#ifdef POP
static int  pop_remotemail();
#endif POP
#ifdef X400
static int x400_remotemail();
#endif X400

/*  */

/* ARGSUSED */

main (argc, argv)
int     argc;
char   *argv[];
{
    int     datesw = 1,
	    notifysw = NT_ALL,
	    rpop = 1,
	    status = 0,
#ifdef POP
	    pop_snoop = 0,
#endif POP
#ifdef X400
            x400_snoop = 0,
#endif X400
            i,
	    vecp = 0;
    char   *cp,
           *host,
            this_src = 0,
            all_src = 0,
            buf[80],
	  **ap,
          **argp,
	   *arguments[MAXARGS],
           *vec[50];
    struct passwd  *pw;

    invo_name = r1bindex (argv[0], '/');
    mts_init (invo_name);

#ifdef	POP
    if ((cp = getenv ("MHPOPDEBUG")) && *cp)
	pop_snoop++;
#endif	POP
#ifdef	X400
    if ((cp = getenv ("MHX400DEBUG")) && *cp)
	x400_snoop++;
#endif	X400

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
		    (void) sprintf (buf, "%s [switches] [users ...]",
			    invo_name);
		    help (buf, switches);
		    done (1);

		case DATESW:
		    datesw++;
		    continue;
		case NDATESW:
		    datesw = 0;
		    continue;

		case NOTESW:
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    notifysw |= donote (cp, 1);
		    continue;
		case NNOTESW:
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    notifysw &= ~donote (cp, 0);
		    continue;

		case HOSTSW: 
		    if (!(host = *argp++) || *host == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    continue;
		case USERSW: 
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    vec[vecp++] = cp;
		    continue;
	        case SRCESW:
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    this_src = get_src_flag(cp);
		    if (this_src == 0)
			adios (NULLCP, " unknown source %s", cp);
		    all_src |= this_src;
		    continue;		    
		case RPOPSW: 
		    rpop++;
		    continue;
		case NRPOPSW: 
		    rpop = 0;
		    continue;
	    }
	vec[vecp++] = cp;
    }

/*  */

    if (!host || !*host)
	host = NULL;

    if (all_src == 0 ) {
	all_src = ~0;

#ifdef POP
	if (!pophost || !(*pophost))
		all_src ^= POPSRC;
#endif POP
#ifdef X400
	if (!x400host || !(*x400host))
		all_src ^= X400SRC;
#endif X400
    }

    if (vecp == 0)
 	vec[vecp++] = NULL;

    if (host && *host){
#ifdef X400
	x400host = host;
#endif X400
#ifdef POP
	pophost  = host;
#endif POP
    }
/*  */

    for (i = 0; i < vecp; i++) {
        cp = vec[i];

        if (cp == NULL)
	    pw = getpwuid (getuid());
	else
	    pw = getpwnam (cp);
	if (pw)
	    printf("%s :\n",pw -> pw_name);
	else {
	    if (cp != NULL)
		printf("%s :\n",cp); 
	    else
		printf(" unknown user ");
        }
/* FILESRC: Check the spool maildrop file. */
	if ((all_src & FILESRC) == FILESRC) {
            if (pw == NULL) {
		fputs("   ",stdout);
	        advise (NULLCP, "no such user as %s",cp);
	    }
	    else
		status += checkmail(pw,datesw,notifysw,(cp == NULL)? 1 : 0);
        }

/* POPSRC: Connect to the POP server and enquire about mail. */
#ifdef POP
        if ((all_src & POPSRC) == POPSRC) {
	    if (pophost && *pophost)
		status += pop_remotemail(pophost,cp,rpop,notifysw,
					(cp == NULL)? 1: 0, pop_snoop);
	    else
		printf("   no pophost\n");
        }
#endif POP

#ifdef X400
/* X400SRC: Connect to the X400 server and enquire about mail. */
        if ((all_src & X400SRC) == X400SRC) {
	    if (x400host && *x400host)
		status += x400_remotemail(x400host,cp,rpop,notifysw,
					  (cp == NULL)? 1: 0,x400_snoop);
	    else
		printf("   no x400host\n");
        }
#endif X400
    }
    done (status);
}

/*  */

static int  get_src_flag (strng)
char    *strng;
{
	int i,
	    retrbit = 0,
	    num_ents;

	num_ents = sizeof(srctbl)/sizeof(srctbl[0]);
	for(i = 0; i < num_ents; i++) {
		if (uleq(strng, srctbl[i].name))
		    retrbit = srctbl[i].bitflag;
	}
	return (retrbit);
}

/*  */

static struct swit ntswitches[] = {
#define	NALLSW	0
    "all", 0,
#define	NMAISW	1
    "mail", 0,
#define	NNMAISW	2
    "nomail", 0,

    NULL, NULL
};


static int donote (cp, ntflag)
register char   *cp;
int	ntflag;
{
    switch (smatch (cp, ntswitches)) {
	case AMBIGSW: 
	    ambigsw (cp, ntswitches);
	    done (1);
	case UNKWNSW: 
	    adios (NULLCP, "-%snotify %s unknown", ntflag ? "" : "no", cp);

	case NALLSW: 
	    return NT_ALL;
	case NMAISW: 
	    return NT_MAIL;
	case NNMAISW: 
	    return NT_NMAI;
    }
}

/*  */

#ifdef	MF
/* ARGSUSED */
#endif	MF

static int  checkmail (pw, datesw, notifysw, personal)
register struct passwd  *pw;
int	datesw,
	notifysw,
	personal;
{
    int     mf,
            status;
    char    buffer[BUFSIZ];
    struct stat st;

    (void) sprintf (buffer, "%s/%s",
	    mmdfldir[0] ? mmdfldir : pw -> pw_dir,
	    mmdflfil[0] ? mmdflfil : pw -> pw_name);
#ifndef	MF
    if (datesw) {
	st.st_size = 0;
	st.st_atime = st.st_mtime = 0;
    }
#endif	MF
    mf = (stat (buffer, &st) == NOTOK || st.st_size == 0) ? NONEOK
	: st.st_atime <= st.st_mtime ? MMDFNEW : MMDFOLD;

#ifdef	MF
    if (umincproc != NULL && *umincproc != NULL) {
	(void) sprintf (buffer, "%s/%s",
		uucpldir[0] ? uucpldir : pw -> pw_dir,
		uucplfil[0] ? uucplfil : pw -> pw_name);
	mf |= (stat (buffer, &st) == NOTOK || st.st_size == 0) ? NONEOK
	    : st.st_atime <= st.st_mtime ? UUCPNEW : UUCPOLD;
    }
#endif	MF

    if ((mf & UUCPOK) || (mf & MMDFOK)) {
	if (notifysw & NT_MAIL) {

	    if (mf & UUCPOK)
		printf ("%s old-style bell",mf & UUCPOLD ? "   Old": "   New");
	    if ((mf & UUCPOK) && (mf & MMDFOK))
		printf (" and ");
	    if (mf & MMDFOK)
		printf ("%s%s", mf & MMDFOLD ? "   Old" : "   New",
			mf & UUCPOK ? " Internet" : "");
	    printf (" file source mail waiting ");
	}
	else
	    notifysw = 0;

	status = 0;
    }
    else {
	if (notifysw & NT_NMAI)
	    printf ("   No file-source mail waiting");
      	else
	    notifysw = 0;

	status = 1;
    }

#ifndef	MF
    if (notifysw)
	if (datesw && st.st_atime)
	    printf ("; last read on %s",
		    dasctime (dlocaltime ((long *) & st.st_atime), TW_NULL));
#endif	MF
    if (notifysw)
	printf ("\n");

    return status;
}

/*  */

#ifdef	POP
extern	char response[];
/* pop_remotemail : Connect to pop server and enquire about mail */

static int  pop_remotemail (host, user, rpop, notifysw, personal, snoop)
register char   *host;
char   *user;
int	rpop,
	notifysw,
	personal,
	snoop;
{
    int     nmsgs,
            nbytes,
            status;
    char   *pass = NULL;

    if (rpop) {
	if (user == NULL)
	    user = getusr ();
	pass = getusr ();
    }
    else {
	fputs("   pop: ",stdout);
	ruserpass (host, &user, &pass);
    }

    if (pop_init (host, user, pass, snoop, rpop) == NOTOK
	    || pop_stat (&nmsgs, &nbytes) == NOTOK
	    || pop_quit () == NOTOK) {
	fputs("   ",stdout);
	advise (NULLCP, "%s", response);
	return 1;
    }

    if (nmsgs) {
	if (notifysw & NT_MAIL) {
	    printf ("   New pop source mail waiting");
	    printf ("%d message%s (%d bytes)",
		    nmsgs, nmsgs != 1 ? "s" : "", nbytes);
	}
	else
	    notifysw = 0;

	status = 0;
    }
    else {
	if (notifysw & NT_NMAI)
	    printf ("   No pop source mail waiting");
	else
	    notifysw = 0;
	status = 1;
    }
    if (notifysw)
	printf (" on %s\n", host);

    return status;
}
#endif	POP

/*  */

#ifdef	X400
extern	char x400_response[];
/* x400_remotemail : Connect to pop server and enquire about mail */

static int  x400_remotemail (host, user, rpop, notifysw, personal, x400snoop)
register char   *host;
char   *user;
int	rpop,
	notifysw,
	personal,
	x400snoop;
{
    int     nmsgs,
            nbytes,
            status;
    char   *pass = NULL;

    if (rpop) {
	if (user == NULL)
	    user = getusr ();
	pass = getusr ();
    }
    else {
	fputs("   x400: ",stdout);
	ruserpass (host, &user, &pass);
    }

    if (x400d_init (host, user, pass, x400snoop, rpop) == NOTOK
	    || x400d_stat (&nmsgs, &nbytes) == NOTOK
	    || x400d_quit () == NOTOK) {
	fputs("   ",stdout);
	advise (NULLCP, "%s", x400_response);
	return 1;
    }

    if (nmsgs) {
	if (notifysw & NT_MAIL) {
	    printf("   New x400 source mail waiting");
	    printf ("%d message%s (%d bytes)",
		    nmsgs, nmsgs != 1 ? "s" : "", nbytes);
	}
	else
	    notifysw = 0;

	status = 0;
    }
    else {
	if (notifysw & NT_NMAI)
	    printf("   No x400 source mail waiting");
	else
	    notifysw = 0;
	status = 1;
    }
    if (notifysw)
	printf (" on %s\n", host);

    return status;
}
#endif	X400
