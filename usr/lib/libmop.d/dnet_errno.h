/*	@(#)dnet_errno.h	4.1	(ULTRIX)	7/2/90	*/

/*
 *
 * Copyright (C) 1985 by
 * Digital Equipment Corporation, Maynard, Mass.
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Digital Equipment
 * Corporation.
 *
 * Digital assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by Digital.
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 * 1.00 10-Jul-1985
 *      DECnet-ULTRIX   V1.0
 *
 */

/*
 * Error codes
 */

#define EREJBYOBJ	0		/* Rejected by object */
#define	EINSNETRES	1		/* Insufficient network resources */
#define EUNRNODNAM	2		/* Unrecognized node name */
#define	EREMNODESHUT	3		/* Remote node shutting down */
#define	EUNROBJ		4		/* Unrecognized object */
#define	EINVOBJNAM	5		/* Invalid object name format */
#define	EOBJBUSY	6		/* Object too busy */
#define	EABTBYNMGT	8		/* Abort by network management */
#define	EINVNODNAM	10		/* Invalid node name format */
#define	ELOCNODESHUT	11		/* Local node shutting down */
#define	EACCCONREJ	34		/* Access control rejected */
#define	ENORESPOBJ	38		/* No response from object */
#define	ENODUNREACH	39		/* Node unreachable */
