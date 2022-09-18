
/* @(#)mesg.h	4.1	(ULTRIX)	12/6/90	*/

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

/*
 * @OSF_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log:	mesg.h,v $
 * Revision 1.4  90/10/07  20:31:29  devrcs
 * 	Added EndLog Marker.
 * 	[90/09/28  20:51:23  gm]
 * 
 * Revision 1.3  90/04/27  23:06:54  devrcs
 * 	Updated to latest AIX code.
 * 	[90/04/16  13:48:21  tom]
 * 
 * Revision 1.2  90/03/13  21:22:52  mbrown
 * 	Removed unnessary ifdef protection from stdio include.
 * 	[90/03/12  14:22:32  tom]
 * 
 * 	AIX merge first cut - new file.
 * 	[90/02/12  18:24:03  tom]
 * 
 * $EndLog$
 */
/* @(#)mesg.h 1.11  com/inc,3.1,9013 2/28/90 09:08:25 */

/*
 * COMPONENT_NAME: INC
 *
 * FUNCTIONS: mesg.h
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
 * EXTERNAL PROCEDURES CALLED: NLfcatgets, fcatgets, fcatgetmsg
 */

#ifndef _MESG_H_
#define _MESG_H_

#include "nl_types.h"
#include <limits.h>

#define CAT_MAGIC 	505
#define CATD_ERR 	((nl_catd) -1)
#define NL_MAXOPEN	10

nl_catd _do_open();

#ifdef _CAT_MACRO
#define NLcatgets(catd,setno,msgno,def) \
(((catd == NULL || catd == CATD_ERR || !catd->_mem)) ? NLfcatgets(catd,setno,msgno,def) : \
	(setno > catd->_setmax) ? def : \
	        (msgno >= catd->_set[setno]._n_msgs) ? def : \
		    	(!catd->_set[setno]._mp[msgno]._offset) ? def: \
				catd->_mem + catd->_set[setno]._mp[msgno]._offset)

#define catgets(catd,setno,msgno,def) \
(((catd == NULL || catd == CATD_ERR || !catd->_mem)) ? fcatgets(catd,setno,msgno,def) : \
	(setno > catd->_setmax) ? "" : \
	        (msgno >= catd->_set[setno]._n_msgs) ? "" : \
		    	(!catd->_set[setno]._mp[msgno]._offset) ? "": \
				catd->_mem + catd->_set[setno]._mp[msgno]._offset)

#define catgetmsg(catd,setno,msgno,buf,buflen) \
((catd == NULL || catd == CATD_ERR) ? "" : \
	(!catd->_mem) ? 	fcatgetmsg(catd,setno,msgno,buf,buflen) : \
			(setno > catd->_hd->_setmax) ? "" :\
			        (msgno >= catd->_set[setno]._n_msgs) ? "" : \
				   (!catd->_set[setno]._mp[msgno]._offset) ? "" : \
					(strncpy(buf, \
						(char *) (catd->_mem + catd->_set[setno]._mp[msgno]._offset),\
						buflen) , \
					buf))

#endif  /* _CAT_MACRO */

struct _message {
	unsigned short 	_set,
			_msg;
	char 		*_text;
	unsigned	_old;
};

struct _header {
	int 		_magic;
	unsigned short	_n_sets,
			_setmax;
	char 		_filler[20];
};
struct _catset {
	unsigned short 	_setno,
			_n_msgs;
	struct _msgptr 	*_mp;
	char	**_msgtxt;
};

#include <stdio.h>

struct catalog_descriptor {
	char		*_mem;
	char		*_name;
	FILE 		*_fd;
	struct _header 	*_hd;
	struct _catset 	*_set;
	int		_setmax;
	int		_count;
	int		_pid;
};


struct _msgptr {
	unsigned short 	_msgno,
			_msglen;
	unsigned long	_offset;
};
#endif  /* _MESG_H_ */
