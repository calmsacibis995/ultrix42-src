#ifndef lint
static	char	*sccsid = "@(#)alias.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

# include <pwd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <signal.h>
# include <errno.h>
# include "sendmail.h"
# ifdef FLOCK
# include <sys/file.h>
# endif FLOCK
#include <sys/svcinfo.h>
#include <hesiod.h>

# ifndef DBM
# ifndef lint
static char	SccsId[] = "@(#)alias.c @(#)alias.c	1.2 (ULTRIX) 7/20/87 (no DBM)";
# endif not lint
# else DBM

# ifndef lint
static char	SccsId[] = "@(#)alias.c @(#)alias.c	1.2 (ULTRIX) 7/20/87 (with DBM)";
# endif not lint
# endif not DBM

/*
**  ALIAS -- Compute aliases.
**
**	Scans the alias file for an alias for the given address.
**	If found, it arranges to deliver to the alias list instead.
**	Uses libdbm database if -DDBM.
**
**	Parameters:
**		a -- address to alias.
**		sendq -- a pointer to the head of the send queue
**			to put the aliases in.
**
**	Returns:
**		none
**
**	Side Effects:
**		Aliases found are expanded.
**
**	Notes:
**		If NoAlias (the "-n" flag) is set, no aliasing is
**			done.
**
**	Deficiencies:
**		It should complain about names that are aliased to
**			nothing.
*/


#ifdef DBM
typedef struct
{
	char	*dptr;
	int	dsize;
} DATUM;
extern DATUM fetch();
#endif DBM

alias(a, sendq)
	register ADDRESS *a;
	ADDRESS **sendq;
{
	register char *p;
	extern char *aliaslookup();

# ifdef DEBUG
	if (tTd(27, 1))
		printf("alias(%s)\n", a->q_paddr);
# endif

	/* don't realias already aliased names */
	if (bitset(QDONTSEND, a->q_flags))
		return;

	CurEnv->e_to = a->q_paddr;

	/*
	**  Look up this name
	*/

	if (NoAlias)
		p = NULL;
	else
		p = aliaslookup(a->q_user);
	if (p == NULL)
		return;

	/*
	**  Match on Alias.
	**	Deliver to the target list.
	*/

# ifdef DEBUG
	if (tTd(27, 1))
		printf("%s (%s, %s) aliased to %s\n",
		    a->q_paddr, a->q_host, a->q_user, p);
# endif
	message(Arpa_Info, "aliased to %s", p);
	AliasLevel++;
	sendtolist(p, a, sendq);
	AliasLevel--;
}
/*
**  ALIASLOOKUP -- look up a name in the alias file.
**
**	Parameters:
**		name -- the name to look up.
**
**	Returns:
**		the value of name.
**		NULL if unknown.
**
**	Side Effects:
**		none.
**
**	Warnings:
**		The return value will be trashed across calls.
*/

char * aliaslookup_local();
char * aliaslookup_yp();
char * aliaslookup_bind();
char * (*aliaslookups []) ()={
		aliaslookup_local,
		aliaslookup_yp,
		aliaslookup_bind
};


/*
 * generic alias service routines
 */

char *
aliaslookup (name)
	register char *name;
{
	register char *p=NULL;
	register i;
	struct svcinfo *svcinfo;

	if ((svcinfo = getsvc()) != NULL)
		for (i=0; (svc_lastlookup = svcinfo->svcpath[SVC_ALIASES][i]) != SVC_LAST; i++)
			if (p = ((*(aliaslookups [svcinfo->svcpath[SVC_ALIASES][i]])) (name) ))
				break;
	return (p);
}

/*
 * specific alias service routines
 */

char *
aliaslookup_local(name)
	char *name;
{
# ifdef DBM
	DATUM rhs, lhs;

#ifdef DEBUG
	printf("aliaslookup_local(%s)\n", name);
#endif DEBUG
	/*
	 * Create a key for fetch
	 */
	lhs.dptr = name;
	lhs.dsize = strlen(name) + 1;
	rhs = fetch(lhs);

	if (rhs.dptr == NULL)
		return(NULL);
	return (rhs.dptr);
# else DBM
	register STAB *s;

#ifdef DEBUG
	printf("aliaslookup_local(%s)\n", name);
#endif DEBUG
	s = stab(name, ST_ALIAS, ST_FIND);

	if (s == NULL)
		return(NULL);
	return (s->s_alias);
# endif DBM
}

char *
aliaslookup_bind(name)
	char *name;
{
	char **pp;
	
#ifdef DEBUG
	printf("aliaslookup_bind(%s)\n", name);
#endif DEBUG
	setent_bind(0);
	pp = hes_resolve(name, "aliases");
	endent_bind();
	if (pp == NULL)
		return(NULL);
	return(*pp);
}

char *
aliaslookup_yp(name)
	char *name;
{
# ifdef DBM
	DATUM rhs;
	int err = 0;

	if (!YPdomain)
		return(NULL);

# ifdef DEBUG
	printf("aliaslookup_yp(%s)\n", name);

	/* Note: The only time you can see the following message
	 *	 is via a  "telnet host 25"   command   AND  the
	 *	 DEBUG code has been compiled in.
	 *	 mail -v user  would show on terminal IF we wanted
	 *	 to put in a  "message(Arpa_Info...)"  statement..
	 */
	if (tTd(27, 1))
		printf("cking YP mail.aliases map for  %s\n",name);
# endif DEBUG
	err = yp_match(YPdomain,"mail.aliases",name,strlen(name),&rhs.dptr,&rhs.dsize);
	if (err == YPERR_KEY) {
		err = yp_match(YPdomain,"mail.aliases",name,strlen(name) + 1 ,&rhs.dptr,&rhs.dsize);
		if (err) {
			/* Assume end of alias chain.
			 */
			return(NULL);
		}
	}
# ifdef DEBUG
	if (tTd(27, 1))
		printf("YP map aliases %s to %s\n",name,rhs.dptr);
# endif DEBUG
	/* The end of an alias from YP comes back with a
	 * newline. This disrupts the syslog entries and
	 * the qfAAnnnnnn entries for recipients. Therefore,
	 * squash them off.
	 */
	if (rhs.dptr[rhs.dsize] == '\n')
		rhs.dptr[rhs.dsize--] = NULL;	
	/*
	 * rhs.dptr will be null if the object is not found
	 * in the YP map. Therefore, no need to ck "err".
	 */
	return (rhs.dptr);

# else DBM
	register STAB *s;

	if (!YPdomain)
		return(NULL);
	else {
		err = yp_match(YPdomain,"mail.aliases",name,strlen(name),&s->s_alias,&size);
		if (err == YPERR_KEY) {
			err = yp_match(YPdomain,"mail.aliases",name,strlen(name) + 1 ,&s->s_alias,&size);
			if (err) {
				/* Assume end of alias chain.
				 */
				return(NULL);
			}
		}
	}
	return (s->s_alias);
# endif DBM
}

/*
**  INITALIASES -- initialize for aliasing
**
**	Very different depending on whether we are running DBM or not.
**
**	Parameters:
**		aliasfile -- location of aliases.
**		init -- if set and if DBM, initialize the DBM files.
**
**	Returns:
**		none.
**
**	Side Effects:
**		initializes aliases:
**		if DBM:  opens the database.
**		if ~DBM: reads the aliases into the symbol table.
*/

# define DBMMODE	0666

initaliases(aliasfile, init)
	char *aliasfile;
	bool init;
{
#ifdef DBM
	int atcnt;
	time_t modtime;
	bool automatic = FALSE;
	char buf[MAXNAME];
#endif DBM
	struct stat stb;
	static bool initialized = FALSE;

	if (initialized)
		return;
	initialized = TRUE;

	if (aliasfile == NULL || stat(aliasfile, &stb) < 0)
	{
		if (aliasfile != NULL && init)
			syserr("Cannot open %s", aliasfile);
		NoAlias = TRUE;
		errno = 0;
		return;
	}

# ifdef DBM
	/*
	**  Check to see that the alias file is complete.
	**	If not, we will assume that someone died, and it is up
	**	to us to rebuild it.
	*/

	if (!init)
		dbminit(aliasfile);
	atcnt = SafeAlias * 2;
	if (atcnt > 0)
	{
		while (!init && atcnt-- >= 0 && aliaslookup("@") == NULL)
		{
			/*
			**  Reinitialize alias file in case the new
			**  one is mv'ed in instead of cp'ed in.
			**
			**	Only works with new DBM -- old one will
			**	just consume file descriptors forever.
			**	If you have a dbmclose() it can be
			**	added before the sleep(30).
			*/

			sleep(30);
# ifdef NDBM
			dbminit(aliasfile);
# endif NDBM
		}
	}
	else
		atcnt = 1;

	/*
	**  See if the DBM version of the file is out of date with
	**  the text version.  If so, go into 'init' mode automatically.
	**	This only happens if our effective userid owns the DBM
	**	version or if the mode of the database is 666 -- this
	**	is an attempt to avoid protection problems.  Note the
	**	unpalatable hack to see if the stat succeeded.
	*/

	modtime = stb.st_mtime;
	(void) strcpy(buf, aliasfile);
	(void) strcat(buf, ".pag");
	stb.st_ino = 0;
	if (!init && (stat(buf, &stb) < 0 || stb.st_mtime < modtime || atcnt < 0))
	{
		errno = 0;
		if (AutoRebuild && stb.st_ino != 0 &&
		    ((stb.st_mode & 0777) == 0666 || stb.st_uid == geteuid()))
		{
			init = TRUE;
			automatic = TRUE;
			message(Arpa_Info, "rebuilding alias database");
#ifdef LOG
			if (LogLevel >= 7)
				syslog(LOG_INFO, "rebuilding alias database");
#endif LOG
		}
		else
		{
#ifdef LOG
			if (LogLevel >= 7)
				syslog(LOG_INFO, "alias database out of date");
#endif LOG
			message(Arpa_Info, "Warning: alias database out of date");
		}
	}


	/*
	**  If necessary, load the DBM file.
	**	If running without DBM, load the symbol table.
	*/

	if (init)
	{
#ifdef LOG
		if (LogLevel >= 6)
		{
			extern char *username();

			syslog(LOG_NOTICE, "alias database %srebuilt by %s",
				automatic ? "auto" : "", username());
		}
#endif LOG
		readaliases(aliasfile, TRUE);
	}
# else DBM
	readaliases(aliasfile, init);
# endif DBM
}
/*
**  READALIASES -- read and process the alias file.
**
**	This routine implements the part of initaliases that occurs
**	when we are not going to use the DBM stuff.
**
**	Parameters:
**		aliasfile -- the pathname of the alias file master.
**		init -- if set, initialize the DBM stuff.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Reads aliasfile into the symbol table.
**		Optionally, builds the .dir & .pag files.
*/

static
readaliases(aliasfile, init)
	char *aliasfile;
	bool init;
{
	register char *p;
	char *rhs;
	bool skipping;
	int naliases, bytes, longest;
	FILE *af;
	void (*oldsigint)();
	ADDRESS al, bl;
	register STAB *s;
	char line[BUFSIZ];

	if ((af = fopen(aliasfile, "r")) == NULL)
	{
# ifdef DEBUG
		if (tTd(27, 1))
			printf("Can't open %s\n", aliasfile);
# endif
		errno = 0;
		NoAlias++;
		return;
	}

# ifdef DBM
# ifdef FLOCK
	/* see if someone else is rebuilding the alias file already */
	if (flock(fileno(af), LOCK_EX | LOCK_NB) < 0 && errno == EWOULDBLOCK)
	{
		/* yes, they are -- wait until done and then return */
		message(Arpa_Info, "Alias file is already being rebuilt");
		if (OpMode != MD_INITALIAS)
		{
			/* wait for other rebuild to complete */
			(void) flock(fileno(af), LOCK_EX);
		}
		(void) fclose(af);
		errno = 0;
		return;
	}
# endif FLOCK
# endif DBM

	/*
	**  If initializing, create the new DBM files.
	*/

	if (init)
	{
		oldsigint = signal(SIGINT, SIG_IGN);
		(void) strcpy(line, aliasfile);
		(void) strcat(line, ".dir");
		if (close(creat(line, DBMMODE)) < 0)
		{
			syserr("cannot make %s", line);
			(void) signal(SIGINT, oldsigint);
			return;
		}
		(void) strcpy(line, aliasfile);
		(void) strcat(line, ".pag");
		if (close(creat(line, DBMMODE)) < 0)
		{
			syserr("cannot make %s", line);
			(void) signal(SIGINT, oldsigint);
			return;
		}
		dbminit(aliasfile);
	}

	/*
	**  Read and interpret lines
	*/

	FileName = aliasfile;
	LineNumber = 0;
	naliases = bytes = longest = 0;
	skipping = FALSE;
	while (fgets(line, sizeof (line), af) != NULL)
	{
		int lhssize, rhssize;

		LineNumber++;
		p = index(line, '\n');
		if (p != NULL)
			*p = '\0';
		switch (line[0])
		{
		  case '#':
		  case '\0':
			skipping = FALSE;
			continue;

		  case ' ':
		  case '\t':
			if (!skipping)
				syserr("Non-continuation line starts with space");
			skipping = TRUE;
			continue;
		}
		skipping = FALSE;

		/*
		**  Process the LHS
		**	Find the final colon, and parse the address.
		**	It should resolve to a local name -- this will
		**	be checked later (we want to optionally do
		**	parsing of the RHS first to maximize error
		**	detection).
		*/

		for (p = line; *p != '\0' && *p != ':' && *p != '\n'; p++)
			continue;
		if (*p++ != ':')
		{
			syserr("missing colon");
			continue;
		}
		if (parseaddr(line, &al, 1, ':') == NULL)
		{
			syserr("illegal alias name");
			continue;
		}
		loweraddr(&al);

		/*
		**  Process the RHS.
		**	'al' is the internal form of the LHS address.
		**	'p' points to the text of the RHS.
		*/

		rhs = p;
		for (;;)
		{
			register char c;

			if (init && CheckAliases)
			{
				/* do parsing & compression of addresses */
				while (*p != '\0')
				{
					extern char *DelimChar;

					while (isspace(*p) || *p == ',')
						p++;
					if (*p == '\0')
						break;
					if (parseaddr(p, &bl, -1, ',') == NULL)
						usrerr("%s... bad address", p);
					p = DelimChar;
				}
			}
			else
			{
				p = &p[strlen(p)];
				if (p[-1] == '\n')
					*--p = '\0';
			}

			/* see if there should be a continuation line */
			c = fgetc(af);
			if (!feof(af))
				(void) ungetc(c, af);
			if (c != ' ' && c != '\t')
				break;

			/* read continuation line */
			if (fgets(p, sizeof line - (p - line), af) == NULL)
				break;
			LineNumber++;
		}
		if (al.q_mailer != LocalMailer)
		{
			syserr("cannot alias non-local names");
			continue;
		}

		/*
		**  Insert alias into symbol table or DBM file
		*/

		lhssize = strlen(al.q_user) + 1;
		rhssize = strlen(rhs) + 1;

# ifdef DBM
		if (init)
		{
			DATUM key, content;

			key.dsize = lhssize;
			key.dptr = al.q_user;
			content.dsize = rhssize;
			content.dptr = rhs;
			store(key, content);
		}
		else
# endif DBM
		{
			s = stab(al.q_user, ST_ALIAS, ST_ENTER);
			s->s_alias = newstr(rhs);
		}

		/* statistics */
		naliases++;
		bytes += lhssize + rhssize;
		if (rhssize > longest)
			longest = rhssize;
	}

# ifdef DBM
	if (init)
	{
		/* add the distinquished alias "@" */
		DATUM key;

		key.dsize = 2;
		key.dptr = "@";
		store(key, key);

		/* restore the old signal */
		(void) signal(SIGINT, oldsigint);
	}
# endif DBM

	/* closing the alias file drops the lock */
	(void) fclose(af);
	CurEnv->e_to = NULL;
	FileName = NULL;
	message(Arpa_Info, "%d aliases, longest %d bytes, %d bytes total",
			naliases, longest, bytes);
# ifdef LOG
	if (LogLevel >= 8)
		syslog(LOG_INFO, "%d aliases, longest %d bytes, %d bytes total",
			naliases, longest, bytes);
# endif LOG
}
/*
**  FORWARD -- Try to forward mail
**
**	This is similar but not identical to aliasing.
**
**	Parameters:
**		user -- the name of the user who's mail we would like
**			to forward to.  It must have been verified --
**			i.e., the q_home field must have been filled
**			in.
**		sendq -- a pointer to the head of the send queue to
**			put this user's aliases in.
**
**	Returns:
**		none.
**
**	Side Effects:
**		New names are added to send queues.
*/

forward(user, sendq)
	ADDRESS *user;
	ADDRESS **sendq;
{
	char buf[60];
	extern bool safefile();
	int realuser;

# ifdef DEBUG
	if (tTd(27, 1))
		printf("forward(%s)\n", user->q_paddr);
# endif DEBUG

	if (user->q_mailer != LocalMailer || bitset(QBADADDR, user->q_flags))
		return;
# ifdef DEBUG
	if (user->q_home == NULL)
		syserr("forward: no home");
# endif DEBUG

	/* good address -- look for .forward file in home
	 */
	define('z', user->q_home, CurEnv);
	expand("\001z/.forward", buf, &buf[sizeof buf - 1], CurEnv);
	if (!safefile(buf, user->q_uid, S_IREAD)) {

                /* Look for an alternate forward file.
                 * Of the form: /usr/spool/mail/username.forward
                 */
                char aforward[MAXNAME];

                strcpy(aforward,"/usr/spool/mail/");
                strcat(aforward,user->q_user);
                strcat(aforward,".forward");
                expand(aforward, buf, &buf[sizeof buf - 1], CurEnv);
# ifdef DEBUG
                if (tTd(27, 1))
                        printf("\naforward = %s\n",aforward);
# endif DEBUG
                if (!safefile(buf, user->q_uid, S_IREAD))
                        return;

	}
	/* we do have an address to forward to -- do it
	 * (include routine is in recipient.c)
	 */

	/*
	 * Fix for NFS.
	 * -----------
	 * Become the desired user instead of root in order to
	 * have the ability to chown across NFS.
	 */
	realuser = getuid();
	setreuid(0,user->q_uid);

# ifdef DEBUG
	if (tTd(27, 1))
		syslog(LOG_INFO,"becoming uid %d\n", user->q_uid);
# endif DEBUG

	include(buf, "forwarding", user, sendq);
	setuid(0);
	setreuid(realuser,0);
}
