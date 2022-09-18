#ifndef lint
static	char	*sccsid = "@(#)fcatgets.c	4.1	(ULTRIX)	12/6/90";
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
 * $Log:	fcatgets.c,v $
 * Revision 1.5  90/09/13  12:28:05  devrcs
 * 	correct gold merge errors
 * 	[90/08/28  21:04:27  mbrown]
 * 
 * 	upgrade to AIX gold code
 * 	[90/08/26  15:47:03  mbrown]
 * 
 * Revision 1.4  90/08/24  13:40:42  devrcs
 * 	Made file thread safe
 * 	[90/08/16  16:33:14  encore]
 * 
 * Revision 1.3  90/04/27  22:57:29  devrcs
 * 	Latest code from AIX (build 9013a).
 * 	[90/04/20  08:58:18  stevem]
 * 
 * Revision 1.2  90/03/13  21:09:51  mbrown
 * 	New libc integrated for AIX code.
 * 	[90/03/06  00:39:29  stevem]
 * 
 */
/*
 * #if !defined(lint) && !defined(_NOIDENT)
 * static char rcsid[] = "@(#)$RCSfile: fcatgets.c,v $ $Revision: 1.5 $ (OSF) $Date: 90/09/13 12:28:05 $";
 * #endif
 */

/*
 * RESTRICTED RIGHTS LEGEND
 * Use, Duplication or Disclosure by the Government is subject to
 * restrictions as set forth in paragraph (b)(3)(B) of the rights in
 * Technical Data and Computer Software clause in DAR 7-104.9(a).
 *
 * sccsid[] = "fcatgets.c        1.10  com/lib/c/msg,3.1,9021 3/29/90 17:32:56";
 */ 

/*
 * COMPONENT_NAME: (opats name) descriptive name
 *
 * FUNCTIONS: LIBCMSG
 *
 * ORIGINS: 27
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

/*                                                                   
 * EXTERNAL PROCEDURES CALLED: 	_do_open, _do_read_msg
 */


/*
 * NAME: fcatgets
 *                                                                    
 * FUNCTION: Gets a pointer to a message from a message catalog.
 *
 * EXECUTION ENVIRONMENT:
 *
 *	Fcatgets executes under a process.
 *
 * NOTES: In the case that nl_types.h is included in a program and
 *	a program is compiled with -DCAT_MACRO, macro catgets() is
 *	used, and macro catgets() calls fcatgets().
 *	
 * RETURNS: Returns a pointer to the message on success.
 *	If the catd is invalid, the default string is returned.
 *	If the message or set number is invalid, a null string is returned.
 */  

#define _CAT_MACRO
#include "catio.h"
#include <errno.h>

#ifdef  _THREAD_SAFE
#include "rec_mutex.h"
extern  struct rec_mutex _catalog_rmutex;
#endif
/*                                                                   
 * EXTERNAL PROCEDURES CALLED: 	_do_open, _do_read_msg
 */

/*______________________________________________________________________
	The ifdef's around the function name are to provide a 'bailout'
	function in case the macro version runs into trouble.
  ______________________________________________________________________*/

char *fcatgets(catd, setno, msgno, def) 
nl_catd catd;
int setno;
int msgno;
char *def;

	/*---- catd: the catd to get the message from ----*/
	/*---- setno: the set number of the message ----*/
	/*---- msgno: the message number of the message ----*/
	/*---- def: the default string to be returned ----*/

{
	int errno_save = errno;
	char *_do_read_msg();
	char    *m; 

#ifdef  _THREAD_SAFE
#undef  RETURN
#define RETURN(s) \
        return(rec_mutex_unlock(&_catalog_rmutex), seterrno(errno_save), s)

        errno_save = geterrno();
#else
        errno_save = errno;
#endif
        if (catd == NULL || catd == CATD_ERR)
                return(def);
#ifdef  _THREAD_SAFE
        rec_mutex_lock(&_catalog_rmutex);
#endif
	if (!catd->_fd)
		catd = _do_open(catd);
	if (catd == CATD_ERR) 
		RETURN(def);
	if (catd->_mem) {	/*----  for mapped files ----*/
		if (setno <= catd->_hd->_setmax) {
			if (msgno < catd->_set[setno]._n_msgs) {
				if (catd->_set[setno]._mp[msgno]._offset) {
					RETURN(catd->_mem +
                                        catd->_set[setno]._mp[msgno]._offset);
				}
			}
		}
		RETURN(def);
	}
	else {	/*---- for unmapped files ----*/
		m = _do_read_msg(catd,setno,msgno);
                if((m) && (*m != '\0'))
                        RETURN(m);
                else
                        RETURN(def);
	}
}
