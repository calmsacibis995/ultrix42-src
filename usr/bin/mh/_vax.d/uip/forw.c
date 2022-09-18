/* forw.c - forward messages */
#ifndef	lint
static char ident[] = "@(#)$Id: forw.c,v 1.3 90/11/25 19:04:41 sharpe Exp $";
#endif	lint

#include "../h/mh.h"
#include "../h/formatsbr.h"
#include "../zotnet/tws.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>


#define	IFMT	"digest-issue-%s"
#define	VFMT	"digest-volume-%s"

static	mhl_draft(), copy_draft(), build_form();
/*  */

static struct swit switches[] = {
#define	ANNOSW	0
    "annotate", 0,
#define	NANNOSW	1
    "noannotate", 0,

#define	DFOLDSW	2
    "draftfolder +folder", 0,
#define	DMSGSW	3
    "draftmessage msg", 0,
#define	NDFLDSW	4
    "nodraftfolder", 0,

#define	EDITRSW	5
    "editor editor", 0,
#define	NEDITSW	6
    "noedit", 0,

#define	FILTSW	7
    "filter filterfile", 0,
#define	FORMSW	8
    "form formfile", 0,

#define	FRMTSW	9
    "format", 5,
#define	NFRMTSW	10
    "noformat", 7,

#define	INPLSW	11
    "inplace", 0,
#define	NINPLSW	12
    "noinplace", 0,

#define	DGSTSW	13
    "digest list", 0,
#define	ISSUESW	14
    "issue number", 0,
#define	VOLUMSW	15
    "volume number", 0,

#define	WHATSW	16
    "whatnowproc program", 0,
#define	NWHATSW	17
    "nowhatnowproc", 0,

#define	HELPSW	18
    "help", 4,

#define	FILESW	19
    "file file", -4,		/* interface from msh */

#define	DASHSW	20
    "dashmunging", -4,		/* interface to mhl */
#define	NDASHSW	21
    "nodashmunging", -6,

#ifdef	MHE
#define	BILDSW	22
    "build", -5,		/* interface from mhe */
#endif	MHE

#ifdef X400
#define TMPLTSW 23
    "template", 0,		/* interface to X.400: NB Number 23! */
#endif X400

    NULL, NULL
};

/*  */

static struct swit aqrnl[] = {
#define	NOSW	0
    "quit", 0,
#define	YESW	1
    "replace", 0,
#define	LISTDSW	2
    "list", 0,
#define	REFILSW	3
    "refile +folder", 0,
#define NEWSW	4
    "new", 0,

    NULL, NULL
};


static struct swit aqrl[] = {
    "quit", 0,
    "replace", 0,
    "list", 0,
    "refile +folder", 0,

    NULL, NULL
};

/*  */

static char drft[BUFSIZ];

static char delim3[] =
    "\n------------------------------------------------------------\n\n";
static char delim4[] = "\n------------------------------\n\n";


static struct msgs *mp = NULL;		/* used a lot */


long	lseek (), time ();

/*  */

/* ARGSUSED */

main (argc, argv)
int     argc;
char   *argv[];
{
    int     msgp = 0,
            anot = 0,
            inplace = 0,
	    issue = 0,
	    volume = 0,
	    dashflg = 1,
#ifdef	MHE
	    buildsw = 0,
#endif	MHE
	    nedit = 0,
	    nwhat = 0,
	    i,
            in,
            out,
	    isdf = 0,
#ifdef X400
            tmplt = 0,
#endif X400
            msgnum;
    char   *cp,
	   *cwd,
           *maildir,
	   *dfolder = NULL,
	   *dmsg = NULL,
	   *digest = NULL,
           *ed = NULL,
#ifdef X400
           *ted = NULL,
#endif X400
           *file = NULL,
	   *filter = NULL,
           *folder = NULL,
           *form = NULL,
            buf[100],
	    value[10],
          **ap,
          **argp,
           *arguments[MAXARGS],
           *msgs[MAXARGS];
    struct stat st;

    invo_name = r1bindex (argv[0], '/');
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

		case ANNOSW: 
		    anot++;
		    continue;
		case NANNOSW: 
		    anot = 0;
		    continue;

		case EDITRSW: 
		    if (!(ed = *argp++) || *ed == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    nedit = 0;
		    continue;
		case NEDITSW:
		    nedit++;
		    continue;

		case WHATSW: 
		    if (!(whatnowproc = *argp++) || *whatnowproc == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    nwhat = 0;
		    continue;
#ifdef	MHE
		case BILDSW:
		    buildsw++;	/* fall... */
#endif	MHE
		case NWHATSW: 
		    nwhat++;
		    continue;

		case FILESW: 
		    if (file)
			adios (NULLCP, "only one file at a time!");
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    file = path (cp, TFILE);
		    continue;
		case FILTSW:
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    filter = getcpy (libpath (cp));
		    continue;
		case FORMSW: 
		    if (!(form = *argp++) || *form == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    continue;

		case FRMTSW:
		    filter = getcpy (libpath (mhlforward));
		    continue;
		case NFRMTSW:
		    filter = NULL;
		    continue;

		case INPLSW: 
		    inplace++;
		    continue;
		case NINPLSW: 
		    inplace = 0;
		    continue;

		case DGSTSW: 
		    if (!(digest = *argp++) || *digest == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    continue;
		case ISSUESW:
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    if ((issue = atoi (cp)) < 1)
			adios (NULLCP, "bad argument %s %s", argp[-2], cp);
		    continue;
		case VOLUMSW:
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    if ((volume = atoi (cp)) < 1)
			adios (NULLCP, "bad argument %s %s", argp[-2], cp);
		    continue;

		case DFOLDSW: 
		    if (dfolder)
			adios (NULLCP, "only one draft folder at a time!");
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    dfolder = path (*cp == '+' || *cp == '@' ? cp + 1 : cp,
				    *cp != '@' ? TFOLDER : TSUBCWF);
		    continue;
		case DMSGSW:
		    if (dmsg)
			adios (NULLCP, "only one draft message at a time!");
		    if (!(dmsg = *argp++) || *dmsg == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    continue;
		case NDFLDSW: 
		    dfolder = NULL;
		    isdf = NOTOK;
		    continue;

		case DASHSW: 
		    dashflg++;
		    continue;
		case NDASHSW: 
		    dashflg = 0;
		    continue;

#ifdef X400
                case TMPLTSW:
		    cp = *(argp+1);
                    if (cp && *(cp+1) != '-')
                        ted = *argp++;
                    ++tmplt;
                    continue;
#endif X400
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

/*  */

    cwd = getcpy (pwd ());

    if (!m_find ("path"))
	free (path ("./", TFOLDER));
    if (file && (msgp || folder))
	adios (NULLCP, "can't mix files and folders/msgs");

try_it_again: ;
#ifndef MHE
    (void) strcpy (drft, m_draft (dfolder, dmsg, NOUSE, &isdf));
    if (stat (drft, &st) != NOTOK) {
#else	MHE
    (void) strcpy (drft, buildsw ? m_maildir ("draft")
			  : m_draft (dfolder, NULLCP, NOUSE, &isdf));
    if (!buildsw && stat (drft, &st) != NOTOK) {
#endif	MHE
	printf ("Draft \"%s\" exists (%ld bytes).", drft, st.st_size);
	for (i = LISTDSW; i != YESW;) {
	    if (!(argp = getans ("\nDisposition? ", isdf ? aqrnl : aqrl)))
		done (1);
	    switch (i = smatch (*argp, isdf ? aqrnl : aqrl)) {
		case NOSW: 
		    done (0);
		case NEWSW: 
		    dmsg = NULL;
		    goto try_it_again;
		case YESW: 
		    break;
		case LISTDSW: 
		    (void) showfile (++argp, drft);
		    break;
		case REFILSW: 
		    if (refile (++argp, drft) == 0)
			i = YESW;
		    break;
		default: 
		    advise (NULLCP, "say what?");
		    break;
	    }
	}
    }

/*  */

    if (file) {
	anot = 0;
	goto go_to_it;
    }

    if (!msgp)
	msgs[msgp++] = "cur";
    if (!folder)
	folder = m_getfolder ();
    maildir = m_maildir (folder);

    if (chdir (maildir) == NOTOK)
	adios (maildir, "unable to change directory to");
    if (!(mp = m_gmsg (folder)))
	adios (NULLCP, "unable to read folder %s", folder);
    if (mp -> hghmsg == 0)
	adios (NULLCP, "no messages in %s", folder);

    for (msgnum = 0; msgnum < msgp; msgnum++)
	if (!m_convert (mp, msgs[msgnum]))
	    done (1);
    m_setseq (mp);

/*  */

go_to_it: ;
    if (filter && access (filter, 04) == NOTOK)
	adios (filter, "unable to read");

    if (digest) {
	if (issue == 0) {
	    (void) sprintf (buf, IFMT, digest);
	    if (volume == 0
		    && (cp = m_find (buf))
		    && ((issue = atoi (cp)) < 0))
		issue = 0;
	    issue++;
	}
	if (volume == 0)
	    (void) sprintf (buf, VFMT, digest);
	    if ((cp = m_find (buf)) == NULL || (volume = atoi (cp)) <= 0)
		volume = 1;
	if (!form)
	    form = digestcomps;
	in = build_form (form, digest, volume, issue);
    }
    else
	if (form) {
	    if ((in = open (libpath (form), 0)) == NOTOK)
		adios (form, "unable to open form file");
	}
	else {
	    if ((in = open (libpath (forwcomps), 0)) == NOTOK)
		adios (forwcomps, "unable to open default components file");
	    form = forwcomps;
	}

    if ((out = creat (drft, m_gmprot ())) == NOTOK)
	adios (drft, "unable to create");

    cpydata (in, out, form, drft);
    (void) close (in);

/*  */

    if (file) {
	if ((in = open (file, 0)) == NOTOK)
	    adios (file, "unable to open");
	cpydata (in, out, file, drft);
	(void) close (in);
	(void) close (out);
	goto edit_it;
    }

    if (filter)
	mhl_draft (out, digest, volume, issue, drft, filter, dashflg);
    else
	copy_draft (out, digest, drft, volume, issue);
    (void) close (out);

    if (digest) {
	(void) sprintf (buf, IFMT, digest);
	(void) sprintf (value, "%d", issue);
	m_replace (buf, getcpy (value));
	(void) sprintf (buf, VFMT, digest);
	(void) sprintf (value, "%d", volume);
	m_replace (buf, getcpy (value));
    }

    m_replace (pfolder, folder);
    if (mp -> lowsel != mp -> curmsg)
	m_setcur (mp, mp -> lowsel);
    m_sync (mp);
    m_update ();

edit_it: ;

/* PJS: Invoking the template X400 ORname creation utility. */
#ifdef X400
    if (tmplt > 0) {
    int	pid;
    static char	*vecp[] = {
	"template", "-editor","", "-draft","", (char *)NULL,
    };

/* If we have no template editor given on the command-line, try to find a
 * profile entry: if this fails, leave it to 'template' to decide.
 */
	if (ted == (char *)NULL)
	    ted = m_find("Template_Editor");

/* PJS: Let 'template' default the editor...
	if (ted == (char *)NULL)
	    ted = getcpy("prompter");
 */

	if (ted != (char *)NULL)
	    vecp[2] = ted;
	if (drft != (char *)NULL)
	    vecp[4] = drft;

	pid = fork();
	if (pid == 0) {	/* PJS: Child process: exec the template utility. */
	    execv (vecp[0], vecp);
	    execvp(vecp[0], vecp);
	    fprintf(stderr,"Can't find template utility.\n");
	    exit(1);
	}
	if (pid > 0)	/* PJS: Parent process: wait for template to finish. */
	    wait((union wait *)0);
    }

#endif X400

    if (nwhat)
	done (0);
    (void) m_whatnow (ed, nedit, NOUSE, drft, NULLCP, 0, mp,
	anot ? "Forwarded" : NULLCP, inplace, cwd);
    done (1);
}

/*  */

static	mhl_draft  (out, digest, volume, issue, file, filter, dashflg)
int     out,
        volume,
        issue,
	dashflg;
register char   *digest,
		*file,
		*filter;
{
    int     i,
            child_id,
	    msgnum,
            pd[2];
    char   *vec[MAXARGS];
    char    buf1[BUFSIZ];
    char    buf2[BUFSIZ];
    
    if (pipe (pd) == NOTOK)
	adios ("pipe", "unable to create");

    vec[0] = r1bindex (mhlproc, '/');

    for (i = 0; (child_id = fork ()) == NOTOK && i < 5; i++)
	sleep (5);
    switch (child_id) {
	case NOTOK: 
	    adios ("fork", "unable to");

	case OK: 
	    (void) close (pd[0]);
	    (void) dup2 (pd[1], 1);
	    (void) close (pd[1]);

	    i = 1;
	    vec[i++] = "-forwall";
	    vec[i++] = "-form";
	    vec[i++] = filter;
	    if (digest) {
		vec[i++] = "-digest";
		vec[i++] = digest;
		vec[i++] = "-issue";
		sprintf(buf1, "%d", issue); vec[i++] = buf1;
		vec[i++] = "-volume";
		sprintf(buf2, "%d", volume); vec[i++] = buf2;
	    }
	    vec[i++] = dashflg ? "-dashmunging" : "-nodashmunging";
	    if (mp -> numsel >= MAXARGS - i)
		adios (NULLCP, "more than %d messages for %s exec",
			vec[0], MAXARGS - i);
	    for (msgnum = mp -> lowsel; msgnum <= mp -> hghsel; msgnum++)
		if (mp -> msgstats[msgnum] & SELECTED)
		    vec[i++] = getcpy (m_name (msgnum));
	    vec[i] = NULL;

	    execvp (mhlproc, vec);
	    fprintf (stderr, "unable to exec ");
	    perror (mhlproc);
	    _exit (-1);

	default: 
	    (void) close (pd[1]);
	    cpydata (pd[0], out, vec[0], file);
	    (void) close (pd[0]);
	    (void) pidXwait (child_id, mhlproc);
	    break;
    }
}

/*  */

static	copy_draft (out, digest, file, volume, issue)
int     out,
        volume,
        issue;
register char   *digest,
		*file;
{
    int     fd,i,
            msgcnt,
            msgnum;
    register char  *bp,
                   *msgnam;
    char    buffer[BUFSIZ];

    msgcnt = 1;
    for (msgnum = mp -> lowsel; msgnum <= mp -> hghsel; msgnum++)
	if (mp -> msgstats[msgnum] & SELECTED) {
	    if (digest)
		(void) strcpy (buffer,
			msgnum == mp -> lowsel ? delim3 : delim4);
	    else {
		(void) strcpy (bp = buffer, "\n-------"), bp += strlen (bp);
		if (msgnum == mp -> lowsel)
		    (void) sprintf (bp, " Forwarded Message%s",
			    mp -> numsel > 1 ? "s" : "");
		else
		    (void) sprintf (bp, " Message %d", msgcnt);
		bp += strlen (bp);
		(void) strcpy (bp, "\n\n");
	    }
	    (void) write (out, buffer, strlen (buffer));

	    if ((fd = open (msgnam = m_name (msgnum), 0)) == NOTOK) {
		admonish (msgnam, "unable to read message");
		continue;
	    }
	    cpydgst (fd, out, msgnam, file);
	    (void) close (fd);

	    msgcnt++;
	}

    if (digest)
	(void) strcpy (buffer, delim4);
    else
	(void) sprintf (buffer, "\n------- End of Forwarded Message%s\n\n",
		mp -> numsel > 1 ? "s" : "");
    (void) write (out, buffer, strlen (buffer));

    if (digest) {
	(void) sprintf (buffer, "End of %s Digest [Volume %d Issue %d]\n", digest, volume, issue);
	i = strlen (buffer);
	for (bp = buffer + i; i > 1; i--)
	    *bp++ = '*';
	*bp++ = '\n';
	*bp = NULL;
	(void) write (out, buffer, strlen (buffer));
    }
}

/*  */

static int  build_form (form, digest, volume, issue)
register char  *form,
               *digest;
int     volume,
        issue;
{
    int	    in;
    int     fmtsize;
    register char *nfs;
    char   *line,
            tmpfil[BUFSIZ];
    register    FILE *tmp;
    register struct comp *cptr;
    struct format *fmt;
    int     dat[4];

    nfs = new_fs (form, NULLCP, NULLCP);
    fmtsize = strlen (nfs) + 256;
    (void) fmt_compile (nfs, &fmt);

    FINDCOMP (cptr, "digest");
    if (cptr)
	cptr->c_text = digest;
    FINDCOMP (cptr, "date");
    if (cptr)
	cptr->c_text = getcpy(dtimenow ());

    dat[0] = issue;
    dat[1] = volume;
    dat[2] = 0;
    dat[3] = fmtsize;

    (void) strcpy (tmpfil, m_tmpfil (invo_name));
    if ((tmp = fopen (tmpfil, "w+")) == NULL)
	adios (tmpfil, "unable to create");
    (void) unlink (tmpfil);
    if ((in = dup (fileno (tmp))) == NOTOK)
	adios ("dup", "unable to");

    if ((line = malloc ((unsigned) fmtsize)) == NULLCP)
	adios (NULLCP, "unable to allocate format line storage");
    (void) fmtscan (fmt, line, fmtsize, dat);
    (void) fputs (line, tmp);
    (void) free (line);
    if (fclose (tmp))
	adios (tmpfil, "error writing");

    (void) lseek (in, 0L, 0);
    return in;
}
