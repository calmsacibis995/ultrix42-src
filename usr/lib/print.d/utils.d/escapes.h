/*	@(#)escapes.h	4.1      ULTRIX 7/2/90 */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 * escapes.h -- escape replacement structure
 *
 * Description:
 *	Contains struct escape which implements an object
 *	for performing parameter substitution according to a
 *	table which is supplied on initialisation of the table
 *
 *	The operation functions on the escape object are exported
 *	be extern declarations here
 */

struct escapes {
	struct es_pair *es_pair; /* array of char, string pairs */
	struct es_pair *es_sentinel; /* last element of pair array */
	char *es_unknown;	/* pointer to error message string */
};

/*
 * Note that the vector of struct es_pairs supplied on initialisation
 * must obey these rules:-
 *	1. the first pair identifies the escape char, which escapes itself
 *	2. The end of the array is flagged initially by the value
 *	    '\0' , this slot is stomped on by the lookup routine.
 *	3. The replacement string is initialised by reference,
 *	   i.e. it is a pointer to the char pointer.
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  02/06/88 -- thoms
 * date and time created 88/06/02 17:46:08 by thoms
 * 
 * ***************************************************************
 *
 * 1.2  15/07/88 -- thoms
 * Added copyright notice, modification history, improved comments
 * SCCS history end
 */


struct es_pair {
	int ep_ch;		/* the character */
	char **ep_repl;		/* reference to replacement string */
};

/*
 * typedef ESP -- shorthand for pointer to escapes object
 */
typedef struct escapes *ESP;

extern void es_init(/* ESP esp, struct es_pair *pairs */);
extern void es_delete(/* ESP esp */);
extern char *es_lookup(/* ESP esp, int ch */);
extern int do_escapes(/* FCP fcp, ESP esp */);
