#ifndef lint
static char *sccsid = "@(#)vipw.c	4.2 (Ultrix) 9/4/90";
/* Original Berkeley ID: "@(#)vipw.c	4.2 (Berkeley) 9/7/83"; */
#endif
/*
    vipw	edit the password file with root consistency
		checking.

    This program allows the user to edit /etc/passwd using
    the editor specified in the EDITOR environment variable.
    It sets a lock file up so that only one user can edit
    the file using vipw at one time.

    The program performs a number of consistency checks on
    the root password file entry, and will not allow a user
    to install a password file that does not have a valid
    root entry.

 ------------
 Modification History
 ~~~~~~~~~~~~~~~~~~~
05	26-Jun-90, Davind Long
	Use /etc/shells to check roots passwd entry shell field.

04	19-Jul-89, David Long
	Rebuild 4.3BSD style passwd data base with "mkpasswd" after
	editing.

03	10-Jan-88, David Long
	Modified to detect error writing temporary passwd file. Done
	to correct problem with truncated passwd files when / overflows.

02	7-Nov-85, Greg Tarsa
	Modified to make some format checks on all password entries.
	These checks include verifying that:
		entries are not missing fields
		entries have no extra fields
		name, uid, gid, home directory are not null
	These changes are under the condition CHECKALL.  If CHECKALL
	is not defined, then only root entry checking is performed.
	These changes have been tested and  are only awaiting documentation

	Added more useful messages to diagnostic outputs.

	Also added a check to be sure that a root directory entry
	exists in the file and that there is only one with the name
	"root".

	Also fixed return status for program.  It consistently returns
	0 if the password file was updated and 1 if it was not.

01	6-Nov-85, Greg Tarsa
	Added comments, fixed a bug where strncmp was being
	called without a count value.  Added diagnostics as
	to why the root entry is invalid, if it is found to
	be so.

*/
/************************************************************************
 *									*
 *			Copyright (c) 1985, 1988 by			*
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <strings.h>

/*
 The number of fields in a password entry
*/
#define F_NAME		1
#define F_PASSWD	2
#define F_UID		3
#define F_GID		4
#define F_GECOS		5
#define F_HOMEDIR	6
#define F_SHELL		7
#define MAXFIELDS	F_SHELL

/*
  Maximum number of erroneous fields processed
*/
#define MAXERRS		20

/*
 entry checking routine definitions
*/
#define GENERAL_ENT	1	/* perform general format checking */
#define ROOT_ENT	2	/* perform root format checking */

#define ENT_GOOD	1	/* good entry */
#define ENT_BAD		2	/* bad entry */
#define ENT_OTHER	3	/* not a root entry and not bad */

/*
    shorthand for entry errors
*/
#define ENTRY_ERR(msg) {\
	    fprintf (stderr,"vipw: %s entry %s\n",\
	    		typename,\
			msg);\
	    return ENT_BAD;\
	    }

/*
 * Password file editor with locking.
 */
char   *temp = "/etc/ptmp";
char   *passwd = "/etc/passwd";
char    buf[BUFSIZ];
char   *getenv ();
char   *index ();
extern int  errno;

main (argv, argc)
char *argv[];
int argc;
{
    int     fd;
    FILE * tmp_fd, *pwd_fd;
    char   *editor;

    /* Ignore all the proper signals */
    signal (SIGINT, SIG_IGN);
    signal (SIGQUIT, SIG_IGN);
    signal (SIGHUP, SIG_IGN);

    setbuf (stderr, NULL);

    umask (0);

    /* 
     Open the file for exclusive access.  If it exists
     then we can't edit right now since someone else is.
     */
    if ((fd = open (temp, O_WRONLY | O_CREAT | O_EXCL, 0644)) < 0)
	{
	if (errno == EEXIST)
	    {
	    fprintf (stderr, "vipw: password file busy\n");
	    exit (1);
	    }
	fprintf (stderr, "vipw: ");
	perror (temp);
	exit (1);
	}

    /* 
     Now open the file as a stdio stream.
     */
    tmp_fd = fdopen (fd, "w");
    if (tmp_fd == NULL)
	{
	fprintf (stderr, "vipw: ");
	perror (temp);
	goto bad;
	}

    /* 
     Open the actual password file for reading
     */
    pwd_fd = fopen (passwd, "r");
    if (pwd_fd == NULL)
	{
	fprintf (stderr, "vipw: ");
	perror (passwd);
	goto bad;
	}

    /* copy the passwd file to a temporary file */
    { int c;
    while ((c=fgetc(pwd_fd)) != EOF)
	if (fputc (c, tmp_fd) == EOF)
	    break;
    }

    fflush (tmp_fd);
    if (ferror (pwd_fd))
	{
	puts("vipw: error reading passwd file.");
	goto bad;
	}
    if (ferror (tmp_fd))
	{
	puts("vipw: error creating temporary file.");
	goto bad;
	}

    fclose (tmp_fd);
    fclose (pwd_fd);

    /* 
     Get set to edit.  Editor is in the environment variable,
     or use vi if there is no default editor.
     */
    if ((editor = getenv ("EDITOR")) == 0)
	editor = "vi";

    sprintf (buf, "%s %s", editor, temp);/* build the command */

    /* 
     Execute the command
     If successful, then run the consistency checks.
     */
    if (system (buf) == 0)
	{
	struct stat sbuf;
	int errcnt;		/* # of bad entries */
	int root_found = 0;	/* # of root entries found */

	/* sanity check: temp file nonexistent? */
	if (stat (temp, &sbuf) < 0)
	    {
	    fprintf (stderr,
		    "vipw: can't stat temp file, %s unchanged\n",
		    passwd);
	    goto bad;
	    }

	/* sanity check: temp file empty? */
	if (sbuf.st_size == 0)
	    {
	    fprintf (stderr, "vipw: bad temp file, %s unchanged\n",
		    passwd);
	    goto bad;
	    }

	/* Try to reopen a stream on the file */
	tmp_fd = fopen (temp, "r");
	if (tmp_fd == NULL)
	    {
	    fprintf (stderr,
		    "vipw: can't reopen temp file, %s unchanged\n",
		    passwd);
	    goto bad;
	    }


	/*
	 Perform the password and root entry consistency checking.
	*/
	errcnt = 0;
	while (fgets (buf, sizeof (buf) - 1, tmp_fd) != NULL)
	    {
#ifdef CHECKALL
	    switch (goodentry(GENERAL_ENT, buf))
		{
		/*
		This entry would not be accepted in any
		context
		*/
		case ENT_BAD:
		    errcnt++;
		    fprintf(stderr, "vipw: Invalid entry: %s\n",buf);
		    break;

		case ENT_OTHER:
		    fprintf(stderr,
		    		"vipw: Warning--extraneous line: %s\n",
				buf);
		    break;

		/*
		 This entry is OK, but must be checked for
		 "rootness", that is, if it is a root entry
		 then it must be a proper root entry.
		*/
		case ENT_GOOD:
		    /*
		     Entries of type ENT_OTHER merely fall
		     through this loop.
		    */
#endif CHECKALL
		    switch (goodentry(ROOT_ENT,buf))
			{
			case ENT_BAD:
			    errcnt++;
			    fprintf(stderr,
			    		"vipw: Invalid root entry: %s\n",
			    		buf);
			    /* Fall through */

			case ENT_GOOD:
			    if (++root_found > 1)
				{
				errcnt++;
				root_found--;
				fprintf(stderr,
					"vipw: Another root entry found: %s\n",
					buf);
				}
			    break;
			}/* end switch (goodroot()) */
#ifdef CHECKALL
		    break;
		}/* end switch(goodentry) */
#endif
	    if (errcnt > MAXERRS)
		{
		fprintf(stderr,
			"vipw: More than %d errors found.  Ignoring others\n",
			MAXERRS);
		break;
		}

	    }/* end while ("entries left") */

	/* close the temporary file */
	fclose (tmp_fd);

	if (root_found == 0)
	    {
	    errcnt++;
	    fprintf(stderr, "vipw: No root entry found\n");
	    }

	/*
	 If we have errors, then alert the user to the fact
	 that we are ignoring his work.
	*/
	if (errcnt)
	    {
	    fprintf (stderr,"vipw: %d problem%s found.\n",
	    		errcnt,
			(errcnt > 1) ? "s" :"");

	    fprintf (stderr,
#ifdef CHECKALL
		    "vipw: your file has invalid entries. %s unchanged.\n",
#else
		    "vipw: your file has an invalid root entry. %s unchanged.\n",
#endif CHECKALL
		    passwd);

	    goto bad;
	    }

	/*
	 All is well.  Rename the temporary file to
	 be the real thing.  We have the ultimate
	 error if this cannot be done.
	*/
	if (rename (temp, passwd) < 0)
	    perror ("vipw: rename");
	unlink(temp);

/*
  Rebuild the passwd data base from the ASCII file.
*/

	execl("/etc/mkpasswd", "mkpasswd", "-u", passwd, (char *) 0);
	fputs(argv[0], stderr);
	fputs(": Unable to exec /etc/mkpasswd\n", stderr);
	exit(1);

    }/* end if ("edit successful") */

bad: 
    unlink (temp);	/* delete the temporary file */
    exit(1);
}

/*
    goodentry

    This routine performs two kinds of checking.  In
    GENERAL_ENT mode, it checks the format of the passed
    entry to verify that it is a valid password entry.
    It returns ENT_GOOD if it is.  It returns ENT_OTHER if
    it is blank.  If it is in error, then it prints a
    diagnostic message and returns ENT_BAD.

    In ROOT_ENT mode, it checks to see that the passed
    record has a valid name.  If it does, but it is not
    "root", then ENT_OTHER is returned.  Otherwise, it is
    checked in a manner similar to GENERAL_ENT, except that
    certain root-specific checks are made.  Specifically:
    uid of 0 home directory of / shell field of "" "/bin/sh"
    or "/bin/csh"

    If all is well then ENT_GOOD is returned, otherwise
    a diagnostic message is printed as to the reason for the
    entry's rejection and ENT_BAD is returned.

    Note that this function munges the buffer, changing the first
    colon to EOS and the tailing newline to null.

*/
goodentry(type, entry)
int type;
char *entry;
{
    register char  *cp = entry;
    register int fn;
    char *typename;
    char *ocp = entry;

    switch (type)
	{
	case GENERAL_ENT:	typename = "password"; break;
	case ROOT_ENT:		typename = "superuser"; break;
	default:		typename = "internal error"; break;
	}
	
    /* 
     Position ptr to end of line and change
     it to EOS (End of String).  If none
     then loop for more.
     */
    if ((cp = index (entry, '\n')) == 0)
        return ENT_OTHER;

    /*
     Scan for each field, reporting errors if it is not
     found.
    */
    for (fn = 1, cp = entry; fn < MAXFIELDS; fn++, ocp = cp)
	{
	/* Does this field and the ones following exist? */
	if ((cp = index (++cp, ':')) == 0)
	     /*
	      We ignore bogus name fields if in root checking mode, 
	      since we are only interested in the root entry.
	      In CHECKALL, the previous entry validation call
	      will cover this case for non-root entries.
	     */
	     if (type == ROOT_ENT && fn == 1)
	         return ENT_OTHER;
	     else
	         {
		 fprintf (stderr,
		 	  "vipw: %s entry is missing %d field%s\n",
			  typename,
			  MAXFIELDS - fn,
			  (MAXFIELDS - fn > 1) ? "s" : "");
		return ENT_BAD;
		}

	switch (fn)
	    {
	    case F_NAME:
	        switch (type)
		    {
		    case GENERAL_ENT:
			/*
			 Check for null name
			*/
			if (cp - ocp == 1)
			    ENTRY_ERR("has a null name field");
			break;

		    case ROOT_ENT:
			/* 
			 Get the first field descriptor and change it to
			 EOS for name comparison. If none, then
			 we don't have a valid entry.
			*/
			if (type == ROOT_ENT)
			    {
			    /* 
			     The name is not root.  Skip it.
			     We only do root entry checking.
			     */
			    if (strncmp (ocp, "root", cp - ocp))
				return ENT_OTHER;
			    }
		    }/* end switch (type) */
		break;

	    case F_PASSWD:
		/* password existent? */
		if (cp - ocp == 1)
		    {
		    *ocp = '\0';
		    fprintf(stderr,
			    "vipw: warning--entry for user %s has no password!\n",
			    	entry);
		    *ocp = ':';
		    }
		break;

	    case F_UID:
	        /*
		 Check for null uid
		*/
		if (cp - ocp == 1)
		    ENTRY_ERR("has a null uid field");

		if (type == ROOT_ENT)
		    {
		    /* uid valid? */
		    if (atoi (++ocp) != 0)
			ENTRY_ERR("has a non-zero uid field");
		    }
		break;

	    case F_GID:
	        /*
		 Check for null gid
		*/
		if (cp - ocp == 1)
		    ENTRY_ERR("has a null gid field");

		break;

	    case F_HOMEDIR:
	        /*
		 Check for null homedirectory
		*/
		if (cp - ocp == 1)
		    ENTRY_ERR("has a null home directory field");

		/*
		 If the directory field is existent, then for
		 GENERAL_ENT we assume that the shell field is existent
		 and valid and check for "extra" fields.
		*/
		if (type != ROOT_ENT)
		    if (index(cp+1,':'))
			ENTRY_ERR("has extra fields")
		    else
			break;

		/* 
		 Check directory field to be sure it is
		 set to /.
		 */
		if (strncmp (++ocp, "/:", 2))
		    ENTRY_ERR ("has a root directory that is not \"/\"");

		/*
		 The user's shell should be the remainder of the record.
		 Since we have no separator to check for, we just skip
		 the two characters that comprise the home directory and
		 the field sep.
		*/
		cp++;		/* skip what we checked */

		/* Now check the shell */
		if (*cp && *cp != '\n') {
			FILE *shells;
			char shell[MAXPATHLEN+1], *s;

			if(shells=fopen("/etc/shells", "r")) {
				while(s=fgets(shell, sizeof shell, shells))
					if(strcmp(shell, cp)) {
						char *s;

						if(s=rindex(shell, '/'))
							if(!strcmp(++s, cp))
								break;
					} else
						break;
				if(!s) {
					rewind(shells);
					fprintf(stderr,
					    "vipw: %s entry has an invalid shell: %s\nMust be one of:\n",
					    typename,
					    cp);
					while(fgets(shell, sizeof shell, shells))
						fputs(shell, stderr);
					fclose(shells);
					return ENT_BAD;
				} else
					fclose(shells);
			} else {
				fputs("vipw: Warning, unable to open /etc/shells\n", stderr);
				if(strcmp(cp, "/bin/sh\n") && strcmp(cp, "/bin/csh\n"))
					return ENT_BAD;
			}
		}
		break;
	    }/* end switch (field) */

    }/* end for "each field" */

    return ENT_GOOD;		/* root entry is OK */
}/* end goodentry() */
