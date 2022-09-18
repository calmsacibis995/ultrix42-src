/************************************************************************
 *									*
 *			Copyright (c) 1987,1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *		            Bull, France				*
 *			   Siemens AG, FR Germany			*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*                       Modification History
 *
 * 001  DECwest ANSI 4.9.1 vk 023, 05 Jan 1990
 *      closed pi->in_ifd before returning. This is to
 *      ensure that setlocale does not keep any file descriptors
 *      open. Also defined __opnsknrd in i_subr.c
 */

#ifndef lint
static char Sccsid[] = "@(#)i_init.c	4.1	(ULTRIX)	7/3/90";
#endif

#include "i_defs.h"
#include "i_errno.h"
#include "limits.h"

/*
 * constant definitions
 */
#define READ		0	/* open flag for read only		*/

/*
 * macro to allocate memory
 */
#define new(var, type, size)	((var = (type *)malloc(size)) != (type *)0)

/*
 * global variables (definition)
 */
intl *i_defdb = (intl *)0;	/* default intlinfo pointer		*/

/*
 * referenced external functions
 */
extern char *getenv();		/* get environment variable value	*/
extern char *malloc();		/* get memory				*/
extern char *strcpy();		/* string copy				*/
extern char *strcat();		/* string concatenate			*/
extern char *index();		/* search in a string			*/
extern char *mk_lpath();	/* make pathname			*/

/*
 * strsave -- save string in global memory
 *
 * SYNOPSIS:
 *	static char *
 *	strsave(s)
 *	char *s;
 *
 * DESCRIPTION:
 *	Strsave squirrels away a copy of the string given as argument and
 *	returns a pointer to the location where it was saved.
 *
 * RETURN:
 *	pointer to save location, if successful
 *	(char *)0 for no memory.
 */
static char *
strsave(s)
char *s;
{
	char *cp;

	if ((cp = malloc((unsigned)(strlen(s) + 1))) != (char *)0)
		strcpy(cp, s);
	return(cp);
}

/*
 * i_init -- initialize access to an International UNIX data base.
 *
 * SYNOPSIS:
 *	intl *
 *	i_init(langv)
 *	char *langv;
 *
 * DESCRIPTION:
 *	I_init opens the International UNIX data base for the named language,
 *	and reads all of the headers for the various tables to provide the
 *	information necessary for future accesses to the various internationa-
 *	lization routines.
 *	If langv is a null pointer or an empty string the contece of LANG
 *	is used instead.
 *
 * RETURN:
 *	handle of database if successful, i_errno set to 0.
 *	(intl *)0 otherwise, with i_errno set.
 *
 * LIMITATIONS:
 *	Maximal length for data base path name is limited to PATH_MAX
 *	characters. When successful one file descriptor is used up.
 */
intl *
i_init(langv)
char *langv;			/* lang value (usually from environment)*/
{
	int ifd;		/* tempory fd of the database		*/
	register i_dbhead *hdr;	/* pointer to database header		*/
	register intl *pi;	/* pointer to the database		*/
	register intl *spi;	/* temporary save of ptr to a database	*/
	char lang[NL_LANGMAX+1];	/* language name string		*/
	char terr[NL_LANGMAX+1];	/* territory name string	*/
	char code[NL_LANGMAX+1];	/* codeset name string 		*/
	char name[PATH_MAX+1];	/* real name of file to open	*/

	/*
	 * preset langv if not given
	 */
	if (langv == (char *)0 || *langv == '\0') {
		langv = getenv("LANG");
	}

	/*
	 * format the LANG environment variable.
	 * form_lang will return -1 if langv is not valiD
	 */
	if (form_lang(langv, lang, terr, code, (char *)0) != 0)
	{
		i_errno = I_EBADC;
		return ((intl *)0);
	}

	ini_lpath("INTLINFO",INTLPATH);

	/*
	 * test all possibilities given by INTLINFO
	 */

	for (;;)
	{
		if (mk_lpath(lang, terr, code, (char *)0, (char *)0, name) != 0)
		{
			i_errno = I_EBACC;
			return ((intl *)0);
		}

		/*
		 * First look in the table of already loaded databases
		 * whether we already have that one loaded.
		 */
		for (spi = pi = i_defdb; pi; pi = pi->in_nxt)
			if (strcmp(pi->in_name, name) == 0)
			{
				/*
				 * move to head of chain, if not already
				 */
				if (pi != i_defdb) 
				{
					spi->in_nxt = pi->in_nxt;
					pi->in_nxt = i_defdb;
					i_defdb = pi;
				}
				return((intl *)pi);
			}
			else
				spi = pi; /* save preceed db. in the chain */


		/*
		 * open the database
		 */
		if ((ifd = open(name, READ)) >= 0)
			break;
	}

	/*
	 * reset i_errno
	 */
	i_errno = 0;

	/*
	 * Allocate, initialize, read and check the data base header
	 */
	if (!new(pi, intl, sizeof(intl)))
		goto nomem;

	pi->in_name = (char *)0;	pi->in_prdflt = (prp_tab *)0;
	pi->in_ifd = ifd;		pi->in_clhead = (cl_head *)0;
	pi->in_ilower = (cnv_tab *)0;	pi->in_iupper = (cnv_tab *)0;
	pi->in_cldflt = (col_tab *)0;	pi->in_cvhead = (cv_head *)0;
	pi->in_dblt = (i_dblt *)0;	pi->in_prhead = (pr_head *)0;
	pi->in_propt = (prp_tab *)0;
	pi->in_sthead = (st_head *)0;	pi->in_nxt = (intl *)0;

	hdr = &pi->in_dbhead;

	if (read(ifd, (char *)hdr, sizeof(i_dbhead)) != sizeof(i_dbhead)
	    || hdr->i_magic != I_MAGIC)
	{
		i_errno = I_EINTL;
		goto badfile;
	}

	if((pi->in_name = strsave(name)) == (char *)0)
		goto nomem;

	/*
	 * Load the double letter table
	 */
	if (hdr->i_nbdbl != 0)
	{
		if (!new(pi->in_dblt, i_dblt, hdr->i_dblsz))
			goto nomem;

		if (!sknrd(ifd, hdr->i_dbtab, (char *)pi->in_dblt, hdr->i_dblsz))
			goto badfile;
	}

	/*
	 * Load the property table header
	 */
	if (!new(pi->in_prhead, pr_head, hdr->i_prhsz))
		goto nomem;

	if (!sknrd(ifd, hdr->i_prtab, (char *)pi->in_prhead, hdr->i_prhsz))
		goto badfile;

	/*
	 * Load the collation table header
	 */
	if (!new(pi->in_clhead, cl_head, hdr->i_clhsz))
		goto nomem;

	if (!sknrd(ifd, hdr->i_cltab, (char *)pi->in_clhead, hdr->i_clhsz))
		goto badfile;

	/*
	 * Load the conversion table header
	 */
	if (!new(pi->in_cvhead, cv_head, hdr->i_cvhsz))
		goto nomem;

	if (!sknrd(ifd, hdr->i_cvtab, (char *)pi->in_cvhead, hdr->i_cvhsz))
		goto badfile;

	/*
	 * Load the string table header 
	 */
	if (!new(pi->in_sthead, st_head, hdr->i_sthsz))
		goto nomem;

	if (!sknrd(ifd, hdr->i_sttab, (char *)pi->in_sthead, hdr->i_sthsz))
		goto badfile;

	/*
	 * set the default header if not already set
	 * and add the database to the list of loaded databases
	 */
	if (i_defdb == (intl *)0)
		i_defdb = pi;
	else
	{
		pi->in_nxt = i_defdb;
		i_defdb = pi;
	}
        if (close(ifd) < 0) {
              i_errno = I_EBADF;
              goto badclose;
        }


	return((intl *)pi);
	
badfile:
	if (i_errno == 0)
		i_errno = I_EBACC;

nomem:
	close(ifd);

badclose:

	/*
	 * free unused memory
	 */
	if (pi != (intl *)0)
	{
		/*
		 * WARNING: below the list of loaded property tables may NOT
		 * be freed, as we have loaded only one and this is freed
		 * as in_prdflt below!
		 */
		if (pi->in_name)
			free((char *)pi->in_name);
		if (pi->in_clhead)
			free((char *)pi->in_clhead);
		if (pi->in_cvhead)
			free((char *)pi->in_cvhead);
		if (pi->in_dblt)
			free((char *)pi->in_dblt);
		if (pi->in_prhead)
			free((char *)pi->in_prhead);
		if (pi->in_sthead)
			free((char *)pi->in_sthead);
		free((char *)pi);
	}

	if (i_errno == 0)
		i_errno = I_ENMEM;

	return((intl *)0);
}
