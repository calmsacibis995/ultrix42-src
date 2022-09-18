#ifdef TENEX
#ifndef lint
static char *sccsid = "@(#)tenex.c	4.3  (ULTRIX)        12/20/90";
#endif
/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/
/*
 * Tenex style file name recognition, .. and more.
 *
 * Author: Ken Greer, Sept. 1975, Carnegie-Mellon University.
 *
 * Modification History
 *
 * 014  Bob Fontaine - Mon Oct 22 10:51:40 EDT 1990
 *      Change octal 210s to a quoted backspaces so that tabs settings will
 *      not be changed when terminal is in vt300 and 8 bit MultiNational
 *      Characters modes set.  Thanks to AKira Tanaka (JRD).  Fixes QARS
 *      #5853 and #6041.
 *
 * 013  Bob Fontaine - Mon Oct 22 10:51:40 EDT 1990
 *      Allow users to set tty characteristics to raw or cbreak.
 *      Thanks to Arika Tanaka (JRD).  Fixes QAR 5290.
 *
 * 012	Bob Fontaine - Fri Jun 22 09:53:01 EDT 1990
 *	Changed call to internal printf function to csh_printf to avoid
 *	confusion with stdio library routine.
 *	
 * 011  Bob Fontaine - Thu May 24 10:48:21 EDT 1990
 *	Don't force ECHO on in tty setup.  If user has turned it off
 *	leave it that way.
 *
 * 010	Gary A. Gaudet - Wed Dec 20 16:51:55 EST 1989
 * 	Increased bp[] in termchars to BUFSIZ
 *	Added return value test from ioctl()
 *	removed unused variable ep
 *	added some (castings)
 *
 *
 * 09	Al Delorey (afd), 11-Nov-88.
 *	Fix null ptr references and uninitialized variables.
 *	Also skip trying to recognize aliases and shell variables;
 *	these are now stored as a binary tree in the "varent struct".
 *	Expanding them is not very usefull anyway.
 *
 * 08	Al Delorey (afd), 20-Sep-88.
 *	Ported to PMAX:
 *	sigignore(SIGxxx) => omask = sigblock(sigmask(SIGxxx)
 *	sigirelse(SIGxxx) => sigsetmask(omask)
 *
 * 07	Al Delorey, 08-Feb-88.
 *	Made ESC invoke command "!!:v" (command line edit)
 *	when the ESC is the first character of the line.
 *
 * 06	Greg Tarsa, 10-Feb-84.
 *	Changed so that information for builtins comes from csh's
 *	internal tables, not dummy directory.  Also added aliases to
 *	the list of command names.
 *
 * 05	Greg Tarsa, 4-Feb-84.
 *	Changed package to ignore duplicate entries and to print the
 *	contents of the history entry when listing history entries.
 *
 * 04	Greg Tarsa, 19-Jan-84.
 *	Removed Gotos and refs to curses package.  Added recognition of
 *	variables and history expansion strings.
 *
 * 03	Greg Tarsa, 13-Jan-1984.
 *	Fixed to translate ~ based on the USER environment variable and
 *	not the uid.
 *
 * 02	Mike Ellis, Fairchild A.I. Labs, September 1983.
 *	Search and recognition of command names (in addition to file
 *	names) added.
 *
 * 01	Ken Greer, Carnegie-Mellon University, December 1981.
 *	Finally got around to adding to the Cshell.
 */


#include "sh.h"
#include <sgtty.h>
#include <sys/dir.h>
#include <pwd.h>
/* Don't include stdio.h  Csh doesn't like it!! */

#define TRUE		1
#define FALSE		0
#define ON		1
#define OFF		0
#define SCREEN_WIDTH	80		/* reasonable screen width */
#define FILSIZ		512		/* Max reasonable file name length */
#define ESC		'\033'
#define is_set(var)	adrof(var)

extern short SHIN, SHOUT;
extern char *getenv();
extern putchar();

typedef enum {LIST, RECOGNIZE} COMMAND;

static char
	*BELL = "\07";

/*
	Command source types (for dirctr)/

	Values -1, -2 are special values,
	values 0-n reflect the directory in
	that position in the path variable.
*/
#define CMD_BUILTIN	-1	/* get from builtin command table */
#define CMD_ALIAS	-2	/* get from alias list (this is NOT used) */

#define MAXITEMS 2048	/* max items allowed in lists */

/*
	Definition for general context pointer
	in getentry.
*/
union entry_context
	{
	DIR *dir_fd;		/* for directory lookups */
	struct Hist *curhist;	/* for history lookups */
	struct varent *curvar;	/* for variable  and alias lookups */
	struct biltins *curbfunc;	/* for BUILTIN lookups */
	};

int bfuncnum;			/* count bfunc entries so we don't
				   fall off end of list*/

/*
	Search word types.
*/
#define DIR_WD		0	/* default case */
#define CMD_WD		1	/* command word */
#define LNAM_WD		2	/* login name */
#define HIST_WD		3	/* history word */
#define VAR_WD		4	/* variable name */


/*
	setup_tty

	Set the terminal up to enable recognition features
	or restore original terminal characteristics.

	The shell makes sure that the tty is not in some
	weird state and fixes it if it is.  This is a useful
	feature in it's own right. . .But it should be noted
	that the tenex routine will not work correctly in
	CBREAK or RAW mode so this code below is, therefore,
	mandatory.
*/
static
setup_tty (on)
{
	static struct sgttyb sgtty;	/* Save sg_flags in basic mode */
	static short save_sg_flags;

	static struct tchars  tchars;	/* INT, QUIT, XON, XOFF, EOF, BRK */
	static char save_t_brkc = -1;	/* Save user's break character */
	int omask;				/* old signal mask */

	omask = sigblock(sigmask(SIGINT));

	if (on)
	{

	/* Get and save current break character. */
	if (ioctl (SHIN, TIOCGETC, &tchars) == -1) {	/* 010 - GAG */
	/*	Perror ("ioctl")*/;
	}
	save_t_brkc = tchars.t_brkc;

	/* Set to ESC, if necessary */
	if (save_t_brkc != ESC)
		{
		tchars.t_brkc = ESC;
		if (ioctl (SHIN, TIOCSETC, &tchars) == -1) {	/* 010 - GAG */
			/*Perror ("ioctl")*/;
			}
		}

	if (ioctl (SHIN, TIOCGETP, &sgtty) == -1) {	/* 010 - GAG */
		/*Perror ("ioctl")*/;
		}
	
	/* ECHO is not mandatory, but is nice. */
	/* Don't force ECHO on if it is already off */   
	
		if ((sgtty.sg_flags & (RAW | CBREAK)))  /* 011 - rnf */
		{
		sgtty.sg_flags &= ~(RAW | CBREAK);
		if (ioctl (SHIN, TIOCSETP, &sgtty) == -1) {	/* 010 - GAG */
			/*Perror ("ioctl")*/;
			}
		}
	}
	else		/* turn special features off */
	{
	/* Reset break char, if necessary. */
	if (save_t_brkc != tchars.t_brkc)
		{
		tchars.t_brkc = save_t_brkc;
		if (ioctl (SHIN, TIOCSETC, &tchars) == -1) {	/* 010 - GAG */
			/*Perror ("ioctl")*/;
			}
		}
				/* 013 RNF */
		if(save_sg_flags && save_sg_flags != sgtty.sg_flags)
		{
			sgtty.sg_flags = save_sg_flags;
			if(ioctl(SHIN,TIOCSETP,&sgtty) == -1) {
				Perror("ioctl");
			}
		}
	}/* end if (on) */

	(void) sigsetmask(omask);
}/* end setup_tty() */

#ifdef CURSES
/*
	termchars

	Initialize any special terminal characters

*/
static
termchars ()
{
	extern char *tgetstr ();
	char bp[BUFSIZ];	/* 010 - GAG */
	static char area[256];
	static int been_here = 0;
	char *ap = area;
	register char *s;

	/* Execute this code only once */
	if (been_here)
	return;

	been_here = TRUE;

	/*
	 Get the terminal entry and determine whether
	 there is a special bell character.
	*/
	if (tgetent (bp, getenv ("TERM")) != 1)
		return;

	if (s = tgetstr ("vb", &ap))		/* Visible Bell */
	BELL = s;

	return;
}/* end termchars() */
#endif CURSES



/*
	back_to_col_1

	Move back to beginning of current line
*/
static
back_to_col_1 ()
{
	struct sgttyb tty, tty_normal;
	int omask;		/* old signal mask */

	omask = sigblock(sigmask(SIGINT));

	/*
	  Get current terminal characteristics.
	  Save them for later restoration, setup
	  to allow CR output and output a CR.

	  Restore original modes.
	*/
	if (ioctl (SHIN, TIOCGETP, &tty) == -1) {	/* 010 - GAG */
		Perror ("ioctl");
		}

	tty_normal = tty;
	tty.sg_flags &= ~CRMOD;
	if (ioctl (SHIN, TIOCSETN, &tty) == -1) {	/* 010 - GAG */
		Perror ("ioctl");
		}

	(void) write (SHOUT, "\r", 1);

	if (ioctl (SHIN, TIOCSETN, &tty_normal) == -1) {	/* 010 - GAG */
		Perror ("ioctl");
		}

	(void) sigsetmask(omask);
}/* end back_to_col_1() */



/*
	pushback

	Push string contents back into tty queue
*/
static
pushback (string)
char  *string;
{
	extern char *malloc();
	register char  *p;
	struct sgttyb   tty, tty_normal;
	int omask;		/* old signal mask */

	omask = sigblock(sigmask(SIGINT));

	/*
	 Get current terminal parameters and save
	 for later restoration.  Turn off terminal
	 echo immediatly, without flushing input
	 buffer.  
	*/
	if (ioctl (SHOUT, TIOCGETP, &tty) == -1) {	/* 010 - GAG */
		Perror ("ioctl");
		}

	tty_normal = tty;
	tty.sg_flags &= ~ECHO;
	if (ioctl (SHOUT, TIOCSETN, &tty) == -1) {	/* 010 - GAG */
		Perror ("ioctl");
		}

	/* Put the chars into the input buffer */
	for (p = string; *p; p++)
	if (ioctl (SHOUT, TIOCSTI, p) == -1) {	/* 010 - GAG */
		Perror ("ioctl");
		}

	/* Restore tty params */
	if (ioctl (SHOUT, TIOCSETN, &tty_normal) == -1) {	/* 010 - GAG */
		Perror ("ioctl");
		}

	(void) sigsetmask(omask);
}/* end pushback() */



/*
	catn

	Concatenate src onto tail of des.

	Des is a string whose maximum length is count.
	Always null terminate.
*/ 
catn (des, src, count)
register char *des, *src;
register count;
{
	while (--count >= 0 && *des)
	des++;

	while (--count >= 0)
	if ((*des++ = *src++) == 0)
		return;

	*des = '\0';
}/* end catn() */



/*
	max

	return the maximum of two numbers.
*/
static
max (a, b)
{
	if (a > b)
	return (a);
	return (b);
}/* end max() */



/*
	copyn

	Like strncpy but always leave room for trailing \0
	and always null terminate.
*/
copyn (des, src, count)
register char *des, *src;
register count;
{
	while (--count >= 0)
	if ((*des++ = *src++) == 0)
		return;

	*des = '\0';
}/* end copyn() */



/*
	fcompare

	For qsort()
*/
static
fcompare (file1, file2)
char  **file1, **file2;
{
	return (strcmp (*file1, *file2));
}/* end fcompare() */


static char
filetype (dir, file)
char *dir, *file;
{
	if (dir)
	{
	char path[512];
	struct stat statb;
	strcpy (path, dir);
	catn (path, file, sizeof path);
	if (stat (path, &statb) >= 0)
		{
		if (statb.st_mode & S_IFDIR)
		return ('/');

		if (statb.st_mode & 0111)
		return ('*');
		}
	}

	return (' ');

}/* end filetype() */


/*
	tilde

	Expand "old" file name with possible tilde usage.

	old = 	~person/mumble
	expands to
	new = home_directory_of_person/mumble

	Return NULL if "person" is invalid, otherwise
	return new.
*/
char *
tilde (new, old)
char *new, *old;
{
	extern struct passwd *getpwuid (), *getpwnam ();

	register char *o, *p;
	register struct passwd *pw;
	static char person[40] = {0};

	/* No leading tilde? No translation. Return orig string */
	if (old[0] != '~')
	{
	strcpy (new, old);
	return (new);
	}

	/*
	 Copy everything after the tilde up to EOS or
	 start of subdir name.
	*/
	for (p = person, o = &old[1]; *o && *o != '/'; *p++ = *o++);
	*p = '\0';

	if (person[0] == '\0')		/* Use current uid if no name */
	strcpy (new, value("home"));	/* translate from environment */
	else
	{
	pw = getpwnam (person);		/* Else lookup on name */
	
	if (pw == NULL)			/* Not found? Return Error. */
		return (NULL);

	strcpy (new, pw -> pw_dir);	/* Found: copy. */
	}

	/* place the translation into the old name */
	(void) strcat (new, o);

	return (new);
}/* end tilde() */


/*
	retype

	Cause pending line to be printed
*/
static
retype ()
{
	int     pending_input = LPENDIN;
	if (ioctl (SHOUT, TIOCLBIS, &pending_input) == -1) {	/* 010 - GAG */
		Perror ("ioctl");
		}
}


/*
	beep

	Ring the terminal bell
*/
static
beep ()
{
	(void) write (SHOUT, BELL, strlen(BELL));
}



/*
	extract_dir_and_name

	Parse full path in file into 2 parts: directory and file name

	Leaves slash (/) at end of dir.
*/
static
extract_dir_and_name (path, dir, name)
char   *path, *dir, *name;
{
	extern char *rindex ();
	register char  *p;

	if ((p = rindex(path, '/')))	/* Is there a dir? */
	{
	p++;				/* Yes: extract */
	copyn (name, p, MAXNAMLEN);	/* copy name w/o '/' */
	copyn (dir, path, p - path);	/* copy dir w/ '/' */
	}
	else
	{		
	copyn (name, path, MAXNAMLEN);	/* No: copy name only */
	dir[0] = '\0';			/* make dir null */
	}

}/* end extract_dir_and_name() */



/*
	getentry

	Get a specified entry from the list of available ones.

	Necessary initializations of files or structure pointers
	has been done prior to getentry call.

	Dirctr is examined when type is CMD_WD,
	to determine the source from which to get the entry.
	CMD_BUILTIN and CMD_ALIAS are currently special cases with
	other cases defaulting to assume context to be a directory fd.
*/

char *
getentry (type, context, dirctr)
register union entry_context *context;
int dirctr;
{
	switch (type)
	{
	case LNAM_WD:		/* get a login name */
		{
		struct passwd *getpwent ();
		register struct passwd *pw;

		/* Read a pw entry.  None: exit. */
		if((pw = getpwent ()) == NULL)
			return (NULL);
		else
			return (pw -> pw_name);
		}

	case CMD_WD:		/* get a command entry */
		/*
		 Process special sources.
		 First, builtins from internal tables,
		 then aliases from internal alias list.
		*/
		if (dirctr == CMD_BUILTIN)
		{
		context->curbfunc++;
		/*
		 afd fix 09:
		 fix null ptr reference, bfunc array is NOT NULL terminated
		*/
		bfuncnum++;
		if (bfuncnum > nbfunc)
			return (NULL);
		return(context->curbfunc->bname);
		}
		else
		if (dirctr == CMD_ALIAS)
			return (getentry(VAR_WD,context,0));
		/* Fall through */

	case DIR_WD:		/* get a directory entry */
		{
		register struct direct *dirp;

		/* read a directory entry. None: exit. */
		if (dirp = readdir (context->dir_fd))
		return (dirp -> d_name);

		return (NULL);
		}

	case VAR_WD:		/* get a variable entry */
		/* Get next variable in list. None: exit. */
		return(NULL);
	case HIST_WD:
		/*
		 Get the next non-negative entry in the history
		 list.  Negative entries are "hidden".  None: exit.
		*/
		do
		context->curhist = context->curhist->Hnext;
		while (context->curhist && context->curhist->Hnum < 0);

		return (context->curhist
			? context->curhist->Hlex.next->word
			: NULL);

	}/* end switch(type) */

}/* end getentry() */

/*
	remove_duplicates

	Routine to remove adjacent duplicate entries from
	a sorted list and return a count of unique items and
	the width of the widest item.

	Duplicate entries are moved to the bottom of the list.
*/
static
remove_duplicates(items, count, widest)
char *items[];
register int *widest;
{
	register char **ip, **op;	/* input and output pointers */
	register int i = 0;
	register char *old_item = NULL;

	*widest = 0;		/* clear any old max value */

	if (count == 0)		/* no entries: exit. */
	return(0);

	/*
	 afd fix 09:
	 old_item starts out as NULL, so we can't do a strcmp on it.
	 So, prime the pump with the first item.
	*/
	ip = op = items;
	*op++ = old_item = *ip;	/* copy ip to op and old_item */
	*widest = strlen (*ip);

	for (ip++; *ip; ip++)
	{
	/*
	 If this is a duplicate, then release it and count
	 it, otherwise move it to the proper place in the
	 array and bump the output pointer to the next free
	 slot; check for wider entry.
	*/
	if (strcmp(old_item,*ip) == 0)	/* ip is a duplicate */
		{
		free(*ip);			/* free the duplicate */
		i++;			/* and keep a count of it */
		}
	else
		{				/* ip is not a duplicate */
		*op++ = old_item = *ip;	/* copy ip to op and old_item */

		/* get the current max width */
		*widest = max (*widest, strlen (*ip));
		}
	}/* end for "each item" */

	/* copy the array terminator also */
	*op++ = *ip;

	return (count - i);			/* return number of unique entries */

}/* end remove_duplicates */


/*
	print_by_column

	Print columns that are sorted downwards.

*/
static
print_by_column (dir, items, count, maxwidth, type)
char *dir;
register char *items[];
{
	register int w, i, r, c;	/* various indices */
	register int rows;
	int columns;

	/*
	 Add two spaces, one to separate columns and
	 one to show the type of the file.  (file type
	 only if this is a directory list.
	*/
	maxwidth += (type == DIR_WD) ? 2 : 1;

	/*
	 Determine the number of columns and rows.
	 If this is a history listing or max item
	 width is greater than the screen width,
	 then assume one column.
	*/
	if (type == HIST_WD || maxwidth > SCREEN_WIDTH)
	columns = 1;
	else
	columns = SCREEN_WIDTH / maxwidth;

	rows = (count + (columns - 1)) / columns;

	for (r = 0; r < rows; r++)
	{
	for (c = 0; c < columns; c++)
		{
		i = c * rows + r;

		/* if it is valid, then print it */
		if (i < count)
		{
		/* special output for history */
		if (type == HIST_WD)
			print_history(items[i], maxwidth);
		else
			{
			csh_printf("%s", items[i]);		/* 012 RNF */
			w = strlen (items[i]);

			/*
			 If this is a directory lookup, print
			 filename followed by '/' or '*' or ' '
			*/
			if (type == DIR_WD)
			{
			putchar (filetype (dir, items[i]));
			w++;
			}

			/* space between columns, if not last */
			if (c < (columns - 1))
			for (; w < maxwidth; w++)
				putchar (' ');
			}
		}
		}/* end for "each column" */

	putchar('\n');
	}/* end for "each row" */
}/* end print_by_column() */

/*
	print_words

	This routine prints the contents of a wordlist up to,
	the specified maximum length.  If the wordlist exceeds this
	max length, it is truncated and a FALSE value is returned
	otherwise the routine returns TRUE.
*/
static
print_words(start, end, maxlen)
struct wordent *start, *end;
register int maxlen;
{
	register char *wp;

	/*
	 print each character in the word separated by
	 a space, until we exceed the maximum length or
	 we run out of words.  Exceeding maximum length
	 causes a FALSE return.
	*/

	for (;;maxlen--)
	{
	for (wp = start->word; *wp; wp++)
		if (--maxlen <= 1)
		return (FALSE);
		else
		putchar(*wp);

	start = start->next;

	if (start == end)
		break;

	putchar(' ');
	}

	return (TRUE);
}/* end print_words() */


/*
	print_history

	This routine will print the first history entry found in
	the history list, with the rest of the history event text
	appearing parentheses after the entry. 

	item	points to a history keyword.
	maxwidth 	is the width of the widest history keyword.
*/
static
print_history(item, maxwidth)
register char *item;
{
#define COLWID	2
	register int colw;			/* used for column positioning */
	register struct wordent *wp;	/* local word pointer */
	register char *ep;			/* local entry pointer */
	struct Hist *hp;			/* local history pointer */

	/* add spaces to separate history word from contents */
	maxwidth += COLWID;

	/*
	 Search the history list for each item and print 
	 the first matching history entry in its entirety.
	*/
	hp = &Histlist;
	do
	ep = getentry(HIST_WD, &hp, 0);
	while (ep && strcmp(ep,item));

	/* If none exit, otherwise print it. */
	if (ep == 0)
	return;

	csh_printf(" %s",ep);	/* print match word */		/* 012 RNF */

	/*
	 Print the (contents) column, if there is one.
	*/
	if ((wp = hp->Hlex.next->next) != hp->Hlex.prev)
	{
	for (colw = strlen(ep); colw < maxwidth; colw++)
		putchar(' ');

	/* Print as much of the contents as possible. */
	putchar('(');
	if (!print_words(	wp,
				hp->Hlex.prev,
				SCREEN_WIDTH - maxwidth - (COLWID + 2)))
		csh_printf("...");	/* truncated */		/* 012 RNF */
	putchar(')');
	}

}/* end print_history() */


/*
	free_items

	Deallocate space for the specified item list.
	Item list ends with a 0 entry.
*/
static
free_items (items)
register char **items;
{
	register int i;

	/* free individual items */
	for (i = 0; items[i]; i++)
	free (items[i]);

	/* free the vector itself */
	free ((char*)items);
}/* end free_items() */

/*
	FREE_ITEMS

	Macro to properly call above.
*/
#define FREE_ITEMS(items)\
{\
	int omask;\
	omask = sigblock(sigmask(SIGINT));\
	free_items (items);\
	items = NULL;\
	(void) sigsetmask(omask);\
}

/*
	FREE_DIR

	Macro to release a directory
*/
#define FREE_DIR(fd)\
{\
	int omask;\
	omask = sigblock(sigmask(SIGINT));\
	closedir (fd);\
	fd = NULL;\
	(void) sigsetmask(omask);\
}


/*
	get_dir_from_path

	Only called when type == CMD_WD.

	Get the next directory from the path list;
	return ptr to next unstripped directory.

	A path list is a list of directory paths separated
	by colons or spaces.

	Increments dirctr.  If it is non-negative,
	then it extracts a directory from the path, otherwise it
	just returns the path without change.
*/
char *
get_dir_from_path (path, dir, dirctr, dirflag)
char *path, dir[], *dirflag;
int *dirctr;
{
	register char *d = dir;

	/* 
	 Increment the directory counter, if it
	 is negative, then return path w/o change.
	*/
	if (++(*dirctr) < 0)
	return (path);

	/* skip past separators */
	while (*path && (*path == ' ' || *path == ':'))
	path++;

	/* Copy the dir name */
	while (*path && (*path != ' ' && *path != ':'))
	*(d++) = *(path++);

	*(d++) = '/';	/* put a trailing slash on the path */
	*d = 0;

	/* Skip past the next separator */
	while (*path && (*path == ' ' || *path == ':'))
	path++;

	if (*dir == '.')
		strcpy (dirflag, " .");
	else
	{
		*dirflag++ = ' ';

	if (*dirctr <= 9)
		*dirflag++ = '0' + *dirctr;
	else
		{
		*dirflag++ = '0' + *dirctr / 10;
		*dirflag++ = '0' + *dirctr % 10;
		}

	*dirflag++ = '\0';
	}

	return (path);
}/* end get_dir_from_path() */


/*
	item_search

	The heart of the tenex module.
	Perform a RECOGNIZE or LIST command on string "word".
*/
static
item_search (word, wp, command, routine, max_word_length, looking_for_command)
char   *word, *wp;	/* original start and end-of-word */
COMMAND command;	/* action: list or recognize */
int (*routine) ();
{

	register numitems,
		name_length,	/* Length of prefix (file name) */
		findtype;	/* The kind of item to search for */

	int 	showpathn;	/* True if we want path number */

	struct stat
		dot_statb,	/* Stat buffer for "." */
		curdir_statb;	/* Stat buffer for current directory */

	int	dot_scan,	/* True if scanning "." */
		dot_got;	/* True if have scanned dot already */

	int  dirctr = 0;	/* -2,-1 (for BUILTINS), 0,1,2 ...(for dirlist pos)*/
	char dirflag[5];	/*  ' nn\0' - dir #s -  . 1 2 ... */
	char *getentry();

	char    tilded_dir[FILSIZ + 1],	 /* dir after ~ expansion */
		dir[FILSIZ + 1],	 /* /x/y/z/ part in /x/y/z/f */
		name[MAXNAMLEN + 1],	 /* f part in /d/d/d/f */
	      extended_name[MAXNAMLEN+1],/* the recognized (extended) name */
	        *entry,		/* single directory entry or logname */
		*path;		/* hacked PATH environment variable */

	union entry_context context;	/* entry context, used all over */

	static char **items;		/* list of found entries */

	context.dir_fd = NULL;		/* initialize the context */

	/* decide what we're looking for */
	switch (*word)
	{
	case '$':			/* variable name */
		findtype = VAR_WD;
		return(0);
		/* NOTREACHED */
		break;

	case '~':			/* login names */
		if (index(word,'/') == NULL)
		{
		findtype = LNAM_WD;
		break;
		}
		/* Fall through if not LNAM */
	
	default:			/* hist expansion */
		/*
		 HIST_WD check is processed here because
		 HIST is not a constant expression and
		 so cannot be part of a case.
		*/
		if (*word == HIST)		/* hist expansion */
		findtype = HIST_WD;
		else
		if (looking_for_command) /* command position? */
			findtype = CMD_WD;
		else
			findtype = DIR_WD; /* Default: dir search */
		break;
	}/* end switch (type) */

	/*
	 If the user has specified a directory then
	 don't do command path searching, but do
	 normal directory searching.
	*/
	if (findtype == CMD_WD && index(word,'/'))
	findtype = DIR_WD;
		
	/*
	 Special setup for command search.
	 Setup for BUILTINs and aliases.
	*/
	if (findtype == CMD_WD)
	{
	dirctr = CMD_BUILTIN; 	/* set BUILTINS code */
	dirflag[0] = NULL;

	/* get the current path for command searching */
	if ((path = getenv("PATH")) == NULL)
		path = "";
	}


	numitems = 0;

	dot_got = FALSE;
	/* afd fix 09: This must be explicitly initialized (it wasn't on VAX) */
	dot_scan = FALSE;
	stat (".", &dot_statb);



	do		/* One loop per directory in PATH, if findtype == CMD_WD */
	{
	/*
	 Perform the necessary item source
	 initializations.
	*/
	switch (findtype)
		{
		case LNAM_WD:		 /* login names */
		/*
		 Open pw file,
		 copy word w/o '~'
		*/
		setpwent ();
		copyn (name, &word[1], MAXNAMLEN);
		break;


		case DIR_WD:		/* Normal dir search */
		/*
		 Expand the given directory
		 and open it, if valid.
		*/
		extract_dir_and_name (word, dir, name);

		if (!tilde(tilded_dir, dir)
			   || !(context.dir_fd = opendir (*tilded_dir ? tilded_dir : ".")))
			return (0);

		dot_scan = FALSE;
		break;

		case CMD_WD:		/* Command keyword */
		switch (dirctr)
			{
			case CMD_BUILTIN:
			/*
			 Set flag, name and context
			 for builtin function names.
			 Context is set one entry back
			 because getentry() always increments
			 the context ptr prior to access.
			*/
			strcpy(dirflag," -");
			copyn(name,word,MAXNAMLEN);
			context.curbfunc = &bfunc[-1];
			bfuncnum = 0;
			break;

			case CMD_ALIAS:
			/*
			 Set flag, name and context for
			 aliases.  Alise structure is
			 identical to variable structure,
			 but with a different head.
			*/
			strcpy(dirflag," +");
			copyn(name,word,MAXNAMLEN);
			context.curvar = &aliases;
			break;

			default:
			/*
			 Expand the passed name,
			 open dir, if valid
			 get next path dir if not valid.
			*/
			if (!tilde(tilded_dir, dir)
				   || !(context.dir_fd = opendir (*tilded_dir
				  				 ? tilded_dir
							 : ".")))
				continue;

			dot_scan = FALSE;

			/* Are we searching "."? */
			fstat (context.dir_fd->dd_fd, &curdir_statb);
			if (curdir_statb.st_dev == dot_statb.st_dev
				&& curdir_statb.st_ino == dot_statb.st_ino)
				{
				if (dot_got)	/* Second time in PATH? */
				continue;

				dot_scan = TRUE;
				dot_got = TRUE;
				}

			break;
			}/* end switch (dirctr) */
		break;

		case VAR_WD:		/* Variable names */
		/*
		 Copy name w/o '$'.
		 Init the current variable ptr
		 to the head of the sorted list.
		*/
		copyn (name, &word[1], MAXNAMLEN);
		context.curvar = &shvhed;
		break;

		case HIST_WD:		/* History keywords */
		/*
		 Copy the name w/o HIST char.
		 Init the current variable ptr
		 to the head of the history list.
		 Entries are in "most recent" order.
		*/
		copyn (name, &word[1], MAXNAMLEN);
		context.curhist = &Histlist;
		break;

		}/* end switch (findtype) */


	name_length = strlen (name);
	showpathn = findtype == CMD_WD && is_set("listpathnum");

	/*
	 Search the source lists for items that
	 match the user's typed in word.
	*/

	while (	entry = getentry (findtype, &context, dirctr)) 
	{
		if (!is_prefix (name, entry))
		continue;
		/*
		 Don't match . files on null prefix match in
		 directory lookups (DIR_WD and CMD_WD currently).
		*/
		if (name_length == 0
			&& entry[0] == '.'
			&& (findtype == DIR_WD || findtype == CMD_WD))
		continue;
		/*
		 * Skip non-executables if looking for commands:
		 * Only done for directory "." for speed.
		 * (Benchmarked with and without:
		 * With filetype check, a full search took 10 seconds.
		 * Without filetype check, a full search took 1 second.)
		 *                                   -Ken Greer
		 */
		if (findtype == CMD_WD && dot_scan && filetype (dir, entry) != '*')
		continue;

		if (command == LIST)		/* LIST command */
		{
		extern char *malloc();
		register int length;

		if (numitems >= MAXITEMS)
			{
			csh_printf ("\nToo many %s. Max %d\n",	/* 012 RNF */
			(findtype == LNAM_WD)
				? "names in password file"
				: "files",
			MAXITEMS);
			break;
			}

		length = strlen(entry) + 1;

		if (showpathn)
			length += strlen(dirflag);

		/* Allocate an arrayful of ptrs, if necessary. */

		if (items == NULL)
			{
			items = (char **)calloc((unsigned)sizeof (items[1]), (unsigned)MAXITEMS + 1);

			if (items == NULL)	/* Can't allocate: exit */
			break;
			}

		if ((items[numitems] = malloc ((unsigned)length)) == NULL)
			{
			csh_printf ("out of mem\n");		/* 012 RNF */
			break;
			}

		copyn (items[numitems], entry, MAXNAMLEN);

		if (showpathn)
			catn (items[numitems], dirflag, MAXNAMLEN);

		numitems++;
	}
	else				/* RECOGNIZE command */
	{
		int tmp = numitems; /* tmp is so I can get register addr */

		if (recognize (extended_name, entry, name_length, &tmp))
			{
			numitems = tmp;
			break;
			}
		numitems = tmp;
		}

		}/* end while "entries exist" */

	/*
	 Perform "finalizations" according
	 to type.
	*/
	switch (findtype)
		{
		case LNAM_WD:	/* Login names: close pw file */
		endpwent ();
		break;

		case DIR_WD:
		case CMD_WD:	/* cmd and dir lookups: close dir */
		if (dirctr > 0)
			FREE_DIR (context.dir_fd);
		break;

		}/* end switch (findtype) */

	}
	while (findtype == CMD_WD
		&& *path
		&& (path = get_dir_from_path (path, dir, &dirctr, dirflag), dir));
	
	switch (command)
	{
	case RECOGNIZE:
		if (numitems <= 0)
		break;

		switch (findtype)
		{
		case LNAM_WD:	/* Login name: put back tilde */
			copyn (word, "~", 1);
			break;

		case CMD_WD:	/* command word: put nothing back */
			word[0] = NULL;
			break;

		case DIR_WD:	/* dir lookup: put dir part back */
			copyn (word, dir, max_word_length);
			break;

		case VAR_WD:	/* variable: put '$' back */
			copyn (word, "$", 1);
			break;

		case HIST_WD:	/* History: put HIST back */
			word[0] = HIST;
			word[1] = NULL;
			break;
		}/* end switch (findtype) */

		/* add extended name */
		catn (word, extended_name, max_word_length);

		/* either append, or print the result */
		while (*wp)
		(*routine) (*wp++);

		break;

	case LIST:
		{
		int newcount = 0, maxwidth;

		/* sort everything */
		qsort (items, numitems, sizeof (items[1]), fcompare);

		/* remove duplicates and get max item width */
		newcount = remove_duplicates(items, numitems, &maxwidth);

		/* print the items */
		print_by_column (tilded_dir,
				 items,
				 newcount,
				 maxwidth,
				 findtype);
		}
		/* Fall through */

	default:
		numitems = 0;	/* set the return value */

	}/* end switch(command) */

	if (items != NULL)		/* release allocated item storage */
	FREE_ITEMS (items);

	return (numitems);		/* return number of pertinent items */

}/* end item_search() */



/*
	recognize

	Object: extend what user typed up to an ambiguity. Keep count
		of valid possibilities.

	Algorithm:
	On first match, copy full entry (assume it'll be the only match).
	On subsequent matches, shorten extended_name to the first
	character mismatch between extended_name and entry.
	If we shorten it back to the prefix length,  and we've
	seen more than one potential match, stop searching.
	If the current entry is identical to the extended name, don't
	count that entry.
*/
recognize (extended_name, entry, name_length, numitems)
char *extended_name, *entry;
int *numitems;
{
	if (*numitems == 0)				/* 1st match */
	{
	copyn (extended_name, entry, MAXNAMLEN);
	(*numitems)++;
	}
	else				/* 2nd and subsequent matches */
	{
	register char *x, *ent;
	register int len = 0;

	for (x = extended_name, ent = entry;
		*x && *x == *ent;
		x++, len++, ent++) /* do nothing */;

	if (*x != *ent)				/* identical? */
		{
		*x = '\0';			/* No: shorten at 1st diff */
		(*numitems)++;			/* Count this one */
		}

	/*
	 Ambiguous to prefix and more than one chice.
	 No need to look further.
	*/
	if (len == name_length && *numitems > 1)
		return(-1);

	}

	return (0);
}/* end recognize() */



/*
	is_prefix

	Return true if the check item's initial chars are in the
	template.  This differs from PWB imatch in that if
	check is null it prefixes anything.

*/
static
is_prefix (check, template)
char   *check,
	   *template;
{
	register char  *check_char,
				   *template_char;

	check_char = check;		/* get copies of string ptrs */
	template_char = template;

	do
	if (*check_char == 0)
		return (TRUE);
	while (*check_char++ == *template_char++);

	return (FALSE);
}


/*
	starting_a_command
	
	Returns TRUE if the word pointed to by wordstart
	is in a command position.

	A command position is defined as being the first
	position in an input line or the first position
	that occurs after a character in the cmdstart set.

	"Whitespace" is defined to be any character in the
	cmdalive set.
*/
starting_a_command (wordstart, inputline)
register char *wordstart, *inputline;
{
	static char cmdstart[] = ";&(|`";
	static char cmdalive[] = " \t'\"";

	/* search backward until start of line */
	while (--wordstart >= inputline)
	{
	/* position maybe a command position: exit. */
	if (index (cmdstart, *wordstart))
		break;

	/*
	 No command character.  If not whitespace,
	 then not a command position: return.
	*/
	if (!index (cmdalive, *wordstart))
		return (FALSE);
	}

	if (wordstart > inputline && *wordstart == '&')	/* Look for >& */
	{
	while (wordstart > inputline
		&& (*--wordstart == ' ' || *wordstart == '\t'))
		;

	if (*wordstart == '>')
		return (FALSE);
	}

	return (TRUE);
}/* end starting_a_command() */



/*
	tenematch

	Do the TENEX-style matching.

	It does this by extracting the word to be recognized from the passed
	input line, determining on the way whether or not the word is in
	a command position, and then returning the status and side-effects
	of the item_search() function.

	Note that a "word" is ordinarily those characters delimited by one
	of the characters in the delims[] set, but this definition has been
	modified to allow the start of a history or variable substitution
	to be the valid start of a word, yet be included in the word.
*/
tenematch (inputline, inputline_size, num_read, command, command_routine)
char   *inputline;		/* match string prefix */
int     inputline_size;		/* max size of string */
int	num_read;		/* # actually in inputline */
COMMAND command;		/* LIST or RECOGNIZE */
int	(*command_routine) ();	/* either append char or display char */

{
	/*
	 The first character of this delims array will be replaced
	 by HIST, the history substitution character.
	*/
	static char delims[] = "!$ '\"\t;&<>()|^%";	/* delimiter set */

	char cand_word [FILSIZ + 1];/* put the candidate word here */

	register char		/* fast pointers */
	*str_end,		/* end of user input */
	*cand_start,		/* start of candidate word */
	*cp,			/* copy pointer */
	*cwp;			/* candidate word ptr */

	int space_left;

	int is_a_cmd;		/* UNIX command rather than filename */

	delims[0] = HIST;		/* use proper history sub char */

	str_end = &inputline[num_read];

   /*
	Find LAST occurence of a delimiter in the inputline.
	The start of the candidate word is the next character.
   */
	for (cand_start = str_end; cand_start > inputline; --cand_start)
	if (index (delims, cand_start[-1]))
		break;

	/* Keep delimiter if we are history or variable substitution */
	if (cand_start[-1] == HIST || cand_start[-1] == '$')
	cand_start--;

	/* determine the expansion space left to us */
	space_left = inputline_size - (cand_start - inputline) - 1;

	/* set flag to show if we are in a command position */
	is_a_cmd = starting_a_command (cand_start, inputline);

	/* copy the candidate word from the input */
	for (cp = cand_start, cwp = cand_word; cp < str_end;
		 *cwp++ = *cp++);
	*cwp = 0;
   
	/* OK, go to work */
	return item_search (cand_word,	/* start of org word */
   		   cwp,		/* end of org word */
		   command,	/* LIST/RECOGNIZE */
		   command_routine,	/* append or print */
		   space_left,	/* expansion space left */
		   is_a_cmd);	/* command position? */

}/* end tenematch() */




/*
	CharAppend

	Routine to print recognized characters and
	append them to the input stream.
*/
char *CharPtr;
static
CharAppend (c)
{
	putchar (c);	/* print the character */
	*CharPtr++ = c;	/* extend the input stream */
	*CharPtr   = 0;
}



/*
	tenex

	This is the routine that is called by the csh proper.

	All control starts here.
*/
#include <sys/time.h>

tenex (inputline, inputline_size)
char   *inputline;
int     inputline_size;
{
	int tty_out = 0;
	register int numitems, num_read;
	struct sgttyb sgtty;

	setup_tty (ON);

#ifdef CURSES
	termchars ();		/* load vb, etc. from TERMCAP */
#endif

	while((num_read = read (SHIN, inputline, inputline_size)) > 0)
	{
	register char *str_end, last_char, should_retype;
	COMMAND command;
	int tty_local = 0;			/* tty "local mode" bits */

	last_char = inputline[num_read - 1] & TRIM;

	/* line ended or buffer full: return */
	if (last_char == '\n' || num_read == inputline_size)
		break;

	if (ioctl (SHIN, TIOCLGET, &tty_local) == -1) {	/* 010 - GAG */
		Perror ("ioctl");
		}

	/*
	 * Get into cmd line edit if the first char on the line is
	 * the ESC char.  I would also recognize other characters for
	 * this (^P, ^E) if the t_brkc field in the tchars struct
	 * allowed more than 1 break character, but it doesn't.
	 */
	if (last_char == ESC && num_read == 1)  /* 07: synonym for !!:v */
		{
		/*
		 * The following abomination of code is necessary in case
		 * the dumb user insists on hitting the uparrow key to get
		 * into command line edit mode (should use ESC key).
		 * The uparrow key transmits an ESC followed by 1 or 2
		 * characters.  The 1 or 2 following characters can be slow
		 * about arriving in the tty input queue and slow about
		 * echoing across the network for an rlogin session.  We want
		 * to be sure that they get read from the input queue and give
		 * them to echo BEFORE we have printed the NEWLINE, otherwise
		 * they show up on the edit line!
		 *
		 * Thus we set the tty to CBREAK mode to move the 2 chars from
		 * the raw to the canonical tty input queue;  so we can be sure
		 * (from select) that they have really arrived and can be read.
		 * We call select and if a char is really available we call select
		 * again, if another char is available, we call select a third
		 * time with only a timeout argument to give the characters echo
		 * time. Thus we always do one more select than the number of
		 * chars that are trailing the escape (assumes a max of 2 trailers).
		 *
		 * Finally we flush (throw away) I/O in case any characters somehow
		 * escaped the select and read on lines that don't behave properly.
		 *
		 * For the normal case where the user just typed the ESC key
		 * to get into edit mode we will only have one select timeout delay.
		 */
		int arg = 0;
		int count;
		struct timeval timeout;
		if (ioctl(SHIN, TIOCGETP, &sgtty) == -1) {	/* 010 - GAG */
			Perror ("ioctl");
			}
		sgtty.sg_flags |= CBREAK;
		if (ioctl(SHIN, TIOCSETP, &sgtty) == -1) {	/* 010 - GAG */
			Perror ("ioctl");
			}
		count = 0;
		arg = 1<<SHIN;
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;	/* 100,000 is 1/10 sec */

		if (select(SHIN+1, &arg, 0, 0, &timeout) > 0)
		{
		/* Got 1 trailing character, see if there is another */
		count++;
		num_read = read(SHIN, inputline, 1);
		arg = 1<<SHIN;
		if (select(SHIN+1, &arg, 0, 0, &timeout) > 0)
			{
		/* Got 2nd trailing char, give time for chars to echo */
			count++;
			num_read = read(SHIN, inputline, 1);
			arg = select(0, 0, 0, 0, &timeout); /* timeout only */
			}
		}
		sgtty.sg_flags &= ~CBREAK;
		if (ioctl(SHIN, TIOCSETP, &sgtty) == -1) {	/* 010 - GAG */
			Perror ("ioctl");
			}	/* end of abomination */
		arg = 0;
		if (ioctl(SHIN, TIOCFLUSH, &arg) == -1) {	/* 010 - GAG */
			Perror ("ioctl");
			}

		for (arg = 0; arg < count; arg++)
		csh_printf("%c\b",QUOTECHAR);	/* backover ESC trailers */ /* 012 RNF 014 RNF */
		if (tty_local & LCTLECH)
		csh_printf ("%c\b%c\b",QUOTECHAR,QUOTECHAR);	/* backover ^[ (ESC) */  /* 012 RNF  014 RNF */

		for (arg = 0; arg < count; arg++)
		csh_printf(" ");			/* blank ESC trailers */ /* 012 RNF */
		/*
		 * We must do a newline since we don't know how long the
		 * user's prompt was, ie. where on the line the cursor started.
		 * We need to know the starting cursor postion, otherwise we
		 * can't edit lines longer than the window size because
		 * we wouldn't know when to wrap the cursor to a new line
		 * when doing cursor positioning.
		 */
		if (tty_local & LCTLECH)
		csh_printf ("  \n");	/* blank ^[ (ESC) */	/* 012 RNF */
		else
		csh_printf ("\n");/* just output newline */	/* 012 RNF */
		strcpy(inputline, "!!:v\n");
		num_read = 5;
		pushback (inputline);
		}
	else
		{
		if (last_char == ESC)			/* RECOGNIZE */
		{
		if (tty_local & LCTLECH)
				/* 012 RNF   014 RNF */
			csh_printf ("%c\b%c\b  %c\b%c\b",QUOTECHAR,QUOTECHAR,QUOTECHAR,QUOTECHAR);	/* Erase ^[ */ 
		command = RECOGNIZE;
		num_read--;
		}
		else				/* LIST */
		command = LIST,
		putchar ('\n');

		CharPtr = str_end = &inputline[num_read];
		*str_end = '\0';

		numitems = tenematch (inputline, inputline_size, num_read, command,
				  command == LIST ? putchar : CharAppend);

		flush ();	/* OK, let's see 'em */

		if (command == RECOGNIZE)
		if (numitems != 1) 			/* Beep = No match/ambiguous */
			beep ();

		/*
		 * Tabs in the input line cause trouble after a pushback.
		 * tty driver won't backspace over them because column positions
		 * are now incorrect. This is solved by retyping over current line.
		 */
		should_retype = FALSE;

		if (index (inputline, '\t')	/* tab in input line? */
		|| (tty_local & LCTLECH) == 0)	/* Control chars don't echo? */
		{
		back_to_col_1 ();
		should_retype = TRUE;
		}

		if (command == LIST)		/* Always retype after LIST */
		should_retype = TRUE;

		if (should_retype)
		printprompt ();

		pushback (inputline);

		if (should_retype)
		retype ();
		}

	}/* end while (num_read > 0) */

	setup_tty (OFF);

	return (num_read);
}/* end tenex() */

#ifdef TEST

#include <stdio.h>

short SHIN = 0, SHOUT = 1;

flush()
{
	fflush(stdout);
}

printprompt ()
{
	(void) write (SHOUT, "-> ", 3);
	return (1);
}

main (argc, argv)
char **argv;
{
	char    string[128];
	int n;
	while (printprompt () && (n = tenex (string, 127)) > 0)
	{
	string[n] = '\0';
	csh_printf ("Tenex returns \"%s\"\n", string); /* 012 RNF */
	}
}
#endif TEST
#endif TENEX
