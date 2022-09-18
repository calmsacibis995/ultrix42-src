/* dist.c - re-distribute a message */

#include "../h/mh.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

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

#define	FORMSW	7
    "form formfile", 0,

#define	INPLSW	8
    "inplace", 0,
#define	NINPLSW	9
    "noinplace", 0,

#define	WHATSW	10
    "whatnowproc program", 0,
#define	NWHATSW	11
    "nowhatnowproc", 0,

/* PJS: Option for X.400 template utility creating ORnames. */
#ifdef X400
#define TMPLTSW 12
    "template", 0,
#endif X400

#define	HELPSW	13
    "help", 4,

#define	FILESW	14
    "file file", -4,		/* interface from msh */

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

/* ARGSUSED */

main (argc, argv)
int     argc;
char   *argv[];
{
    int     anot = 0,
            inplace = 0,
	    nedit = 0,
	    nwhat = 0,
            i,
            in,
	    isdf = 0,
#ifdef X400
            tmplt = 0,
#endif X400
            out;
    char   *cp,
	   *cwd,
           *maildir,
	   *msgnam,
           *dfolder = NULL,
           *dmsg = NULL,
           *ed = NULL,
#ifdef X400
           *ted = NULL,
#endif X400
           *file = NULL,
           *folder = NULL,
           *form = NULL,
           *msg = NULL,
            buf[100],
	    drft[BUFSIZ],
          **ap,
          **argp,
           *arguments[MAXARGS];
    struct msgs *mp = NULL;
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
		    (void) sprintf (buf, "%s [+folder] [msg] [switches]",
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
		case FORMSW: 
		    if (!(form = *argp++) || *form == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    continue;

		case INPLSW: 
		    inplace++;
		    continue;
		case NINPLSW: 
		    inplace = 0;
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
	    if (msg)
		adios (NULLCP, "only one message at a time!");
	    else
		msg = cp;
    }

/*  */

    cwd = getcpy (pwd ());

    if (!m_find ("path"))
	free (path ("./", TFOLDER));
    if (file && (msg || folder))
	adios (NULLCP, "can't mix files and folders/msgs");

    if (form) {
	if ((in = open (libpath (form), 0)) == NOTOK)
	    adios (form, "unable to open form file");
    }
    else {
	if ((in = open (libpath (distcomps), 0)) == NOTOK)
	    adios (distcomps, "unable to open default components file");
	form = distcomps;
    }

try_it_again: ;
    (void) strcpy (drft, m_draft (dfolder, dmsg, NOUSE, &isdf));
    if (stat (drft, &st) != NOTOK) {
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
    if ((out = creat (drft, m_gmprot ())) == NOTOK)
	adios (drft, "unable to create");

    cpydata (in, out, form, drft);
    (void) close (in);
    (void) close (out);

/*  */

    if (file) {
	anot = 0;
	goto go_to_it;
    }

    if (!msg)
	msg = "cur";
    if (!folder)
	folder = m_getfolder ();
    maildir = m_maildir (folder);

    if (chdir (maildir) == NOTOK)
	adios (maildir, "unable to change directory to");
    if (!(mp = m_gmsg (folder)))
	adios (NULLCP, "unable to read folder %s", folder);
    if (mp -> hghmsg == 0)
	adios (NULLCP, "no messages in %s", folder);

    if (!m_convert (mp, msg))
	done (1);
    m_setseq (mp);

    if (mp -> numsel > 1)
	adios (NULLCP, "only one message at a time!");

go_to_it: ;
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

/* PJS: Let 'template' decide the default editor...
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
    if ((in = open (msgnam = file ? file : getcpy (m_name (mp -> lowsel)), 0))
	    == NOTOK)
	adios (msgnam, "unable to open message");

    if (!file) {
	m_replace (pfolder, folder);
	if (mp -> lowsel != mp -> curmsg)
	    m_setcur (mp, mp -> lowsel);
	m_sync (mp);
	m_update ();
    }

    if (nwhat)
	done (0);
    (void) m_whatnow (ed, nedit, NOUSE, drft, msgnam, 1, mp,
	anot ? "Resent" : NULLCP, inplace, cwd);
    done (1);
}
