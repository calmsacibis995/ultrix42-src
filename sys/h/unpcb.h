/* @(#)unpcb.h	4.1  (ULTRIX)        7/2/90     */

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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

/************************************************************************
 *			Modification history				*
 *									*
 *	15-Jan-88	lp						*
 *		Merge of final 43BSD changes.				*
 *									*
 *	001 Larry Cohen 05-March-85					*
 *		- add field to protocol control block to support out	*
 *		of band data on UNIX domain sockets			*
 ************************************************************************/
 

/*	unpcb.h	6.1	83/07/29	*/

/*
 * Protocol control block for an active
 * instance of a UNIX internal protocol.
 *
 * A socket may be associated with an inode in the
 * file system.  If so, the unp_inode pointer holds
 * a reference count to this inode, which should be irele'd
 * when the socket goes away.
 *
 * A socket may be connected to another socket, in which
 * case the control block of the socket to which it is connected
 * is given by unp_conn.
 *
 * A socket may be referenced by a number of sockets (e.g. several
 * sockets may be connected to a datagram socket.)  These sockets
 * are in a linked list starting with unp_refs, linked through
 * unp_nextref and null-terminated.  Note that a socket may be referenced
 * by a number of other sockets and may also reference a socket (not
 * necessarily one which is referencing it).  This generates
 * the need for unp_refs and unp_nextref to be separate fields.
 * An mbuf has been added to allow OOB data. -dap@tek
 */
struct	unpcb {
	struct	socket *unp_socket;	/* pointer back to socket */
	struct	gnode *unp_inode;	/* if associated with file */
	struct	unpcb *unp_conn;	/* control block of connected socket */
	struct	unpcb *unp_refs;	/* referencing socket linked list */
	struct 	unpcb *unp_nextref;	/* link in unp_refs list */
        struct mbuf *unp_addr;          /* bound address of socket */
        struct mbuf *unp_oob;           /* out of band data */
        int     unp_cc;                 /* copy of rcv.sb_cc */
        int     unp_mbcnt;              /* copy of rcv.sb_mbcnt */
	int	unp_disconn;		/* SMP disconnect synchronization flag */
};

#define	sotounpcb(so)	((struct unpcb *)((so)->so_pcb))
