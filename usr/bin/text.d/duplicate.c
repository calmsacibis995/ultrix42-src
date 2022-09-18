#ifndef lint
static	char	*sccsid = "@(#)duplicate.c	4.1	(ULTRIX)	7/17/90";
#endif lint

/************************************************************************
 *									*
 *	 	      Copyright (c) 1987, 1988 by			*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 *
 *   File name: duplicate.c
 *
 *   Source file description:
 *	The functions in this file are used to look after strings in such
 *	a way that duplicates can be detected.
 *
 *   Functions:
 *	savestr()
 *	lookupstr()
 *	clearstr()
 *		calchash()
 *
 *   Modification history:
 *	Andy Gadsby, 20-Jan-1987.
 *		Created.
 *
 */

#include "defs.h"

static struct element *head[HASHMASK + 1];	/* head of lists	*/

/*
 * savestr()
 *	Save a copy of the given string so that it can be extracted 
 *	(efficently) later.
 */

savestr(string, ptr)
char *string;
struct element *ptr;
{	int hash;
	struct element *ep;

	if ((ep = (struct element *)malloc(sizeof(struct element) + ptr->len)) == (struct element *)0) 	
		return ERROR;
	hash = calchash(string, ptr->len);
						/* fill up new structure */
#ifdef ULTRIX
	bcopy(ptr, ep, sizeof(struct element));
	bcopy(string, ep->string, ptr->len);
#else
	memcpy(ep, ptr, sizeof(struct element));
	memcpy(ep->string, string, ptr->len);
#endif
						/* deal with links	 */
	ep->next = head[hash];
	head[hash] = ep;
#ifdef DEBUG
	fprintf(stderr, "saved: %s, %d hash %d\n", ep->string, ep->len, hash);
#endif
	return FALSE;
}

/* 
 * lookupstr()
 *	Lookup a string we squirreled away before if found return the
 *   	a pointer to it otherwise NULL
 */

struct element *
lookupstr(string, len)
char *string;
int   len;
{	struct element *ep;

	for (ep = head[calchash(string, len)]; ep; ep = ep->next)
		if (ep->len == len && strncmp(string, ep->string, len) == 0)
			return ep;
	return (struct element *)NULL;
}

/*
 * clearstr()
 *	Clear the table and free up used space ready for next file
 *	NOTE: we do not remove entries with non-zero flags. This is
 *	      to allow the user to continually add strings to the ignore
 *	      list during the run.
 */

clearstr()
{	struct element *ep, *next;
	struct element **prevptr;	/* ptr to the previous next field */
	int hash = HASHMASK;

	while (hash >= 0) {
		for (ep = head[hash], prevptr = &head[hash]; ep; ep = next) {
			next = ep->next;	/* setup to walk list	*/
			if (ep->flags == 0) {	/* free this entry	*/
				*prevptr = next;/* remove from list	*/
				free(ep);
			} else			/* leave this entry	*/
				prevptr = &ep->next;
		}
		hash--;
	}
}
			

/*
 * calchash()
 *	Return a suitable hash value for string,
 */

calchash(string, len)
char *string;
int len;
{	int val = 0;

	while (--len > 0) {
		val = (val >> 1) + *string++;
	}
	return (val & HASHMASK);
}
