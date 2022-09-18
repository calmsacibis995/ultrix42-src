#ifndef lint
static	char	*sccsid = "@(#)symtab.c	4.1	(ULTRIX)	12/6/90";
#endif lint

/************************************************************************
 *									*
 *         Copyright (c) Digital Equipment Corporation, 1990		*
 *									*
 *   All Rights Reserved.  Unpublished rights  reserved  under  the	*
 *   copyright laws of the United States.				*
 *									*
 *   The software contained on this media  is  proprietary  to  and	*
 *   embodies  the  confidential  technology  of  Digital Equipment	*
 *   Corporation.  Possession, use, duplication or dissemination of	*
 *   the  software and media is authorized only pursuant to a valid	*
 *   written license from Digital Equipment Corporation.		*
 *									*
 *   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  by	*
 *   the U.S. Government is subject to restrictions as set forth in	*
 *   Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  or  in  FAR	*
 *   52.227-19, as applicable.						*
 *									*
 ************************************************************************/
/**/

/*
 * @OSF_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log:	symtab.c,v $
 * Revision 1.3  90/10/07  16:45:39  devrcs
 * 	Added EndLog Marker.
 * 	[90/09/28  14:53:53  gm]
 * 
 * Revision 1.2  90/05/13  20:40:13  devrcs
 * 	new for OSF/1
 * 	[90/05/03  13:04:23  mbrown]
 * 
 * $EndLog$
 */
/* static char sccsid[] = "@(#)symtab.c	1.7  com/cmd/msg,3.1,9013 12/21/89 12:35:02"; */
/*
 * COMPONENT_NAME: CMDMSG
 *
 * FUNCTIONS: insert, nsearch, hash
 *
 * ORIGINS: 9, 27
 *
 * This module contains IBM CONFIDENTIAL code. -- (IBM
 * Confidential Restricted when combined with the aggregated
 * modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 1988, 1989
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 * (Copyright statements and/or associated legends of other
 * companies whose code appears in any part of this module must
 * be copied here.)
 */


#define HASHSIZE 256			/* must be a power of 2 */
#define HASHMAX HASHSIZE - 1


/*
 * EXTERNAL PROCEDURES CALLED: None
 */

struct name {
  char *regname;
  int   regnr;
  struct name *left;
  struct name *right;
  };

struct name *symtab[HASHSIZE];		/* hashed pointers to binary trees */

char *calloc();
char *malloc();

/*
 * NAME: insert
 *
 * FUNCTION: Insert symbol
 *
 * EXECUTION ENVIRONMENT: 
 *  	User mode.
 * 
 * NOTES: These routines manipulate a symbol table for the mkcatdefs program.
 *  	  The symbol table is organized as a hashed set of binary trees.  If the
 *  	  symbol being passed in is found then a -1 is returned, otherwise the
 *  	  symbol is placed in the symbol table and a 0 is returned.  The purpose
 *  	  of the symbol table is to keep mkcatdefs from assigning two different
 *  	  message set / message numbers to the same symbol.
 *        Read the next line from the open message catalog descriptor file.
 *
 * RETURNS: 0 - symbol inserted.
 *         -1 - symbol exists.
 */

insert(tname, seqno)
char *tname;
int  seqno;

	/*
	  tname - pointer to symbol
	  seqno - integer value of symbol
	*/

{
	register struct name *ptr,*optr;
	int rslt = -1,i,hashval;

	hashval = hash(tname);
	ptr = symtab[hashval];

	/* search the binary tree for specified symbol */
	while (ptr && (rslt = strcmp(tname,ptr->regname))) {
		optr=ptr;  
		if (rslt<0)
			ptr = ptr->left;
		else
			ptr = ptr->right;
	}

	if (rslt == 0)		/* found the symbol already defined */
		return (-1);

	/* symbol not defined yet so put it into symbol table */
	else {
		ptr = (struct name *)calloc(sizeof(struct name), 1);
		ptr->regname = malloc(strlen(tname) + 1);
		strcpy (ptr->regname, tname);
		ptr->regnr = seqno;

		/* not first entry in tree so update branch pointer */
		if (symtab[hashval]) {
			if (rslt < 0)
				optr->left = ptr;
			else
				optr->right = ptr;

		/* first entry in tree so set the root pointer */
		} else
			symtab[hashval] = ptr;

		return (0);
	}
}

/*
 * NAME: insert
 *
 * FUNCTION: Insert symbol
 *
 * EXECUTION ENVIRONMENT: 
 *  	User mode.
 * 
 * NOTES: Searches for specific identifier. If found, return allocated number.
 * 	  If not found, return -1.
 *
 * RETURNS: Symbol sequence number if symbol is found.
 *          -1 if symbol is not found.
 */

nsearch (tname)
char *tname;

	/*
	  tname - pointer to symbol
	*/

{
	register struct name *ptr,*optr;
	int rslt = -1,i,hashval;

	hashval = hash(tname);
	ptr = symtab[hashval];

	/* search the binary tree for specified symbol */
	while (ptr && (rslt = strcmp(tname,ptr->regname))) {
		optr=ptr;  
		if (rslt<0)
			ptr = ptr->left;
		else
			ptr = ptr->right;
	}

	if (rslt == 0)		/* found the symbol already defined */
		return(ptr->regnr);
	else
		return (-1);
}


/*
 * NAME: hash
 *
 * FUNCTION: Create hash value from symbol name.
 *
 * EXECUTION ENVIRONMENT: 
 *  	User mode.
 * 
 * NOTES: Hash the symbol name using simple addition algorithm.
 * 	  Make sure that the hash value is in range when it is returned.
 *
 * RETURNS: A hash value.
 */

hash (name)
register char *name;

	/*
	  name - pointer to symbol
	*/

{
	register int hashval = 0;

	while (*name)
		hashval += *name++;

	return (hashval & HASHMAX);
}
