/*	@(#)fifo.h	4.1	(ULTRIX)	7/2/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/************************************************************************
 *			Modification History
 *
 * 11-Jul-88 -- prs
 *	Added two defines : IF_WASYNC IF_RASYNC.
 *
 * 27-Apr-88 -- prs
 *	Added two defines : IF_WCOLL IF_RCOLL.
 *
 * 06-Apr-88 -- prs
 *	Increased the total number of bytes to be allocated for all
 *	fifos.
 *
 * 10-Feb-88 -- prs
 *	New fifo code.
 *
 ************************************************************************/

#ifdef KERNEL
/*
 *	Configuration Constants
 *	SHOULD BE SETTABLE BY "/etc/config".
 */
#define FIFOBUF	4092	/* max # bytes stored in a fifo */
#define FIFOMAX ~0	/* largest size of a single write to a fifo */
#define FIFOBSZ 4092	/* number of data bytes in each fifo data buffer */
#define FIFOMNB FIFOBUF*10	/* total # bytes allowed for all fifos */


/*
 *	Implementation Constants.
 */

#define IFIR	  0x001	/* # blocked readers */
#define IFIW	  0x002	/* # blocked writers */
#define IF_WCOLL  0x004	/* Collision for a write select */
#define IF_RCOLL  0x008	/* Collision for a read select */
#define IF_WASYNC 0x020   /* SIGIO enabled for write side */
#define IF_RASYNC 0x040   /* SIGIO enabled for read side */


struct fifo_bufhdr {
	struct fifo_bufhdr *fb_next;
	char fb_data[1];
};


#define FIFO_BUFHDR_SIZE (sizeof(struct fifo_bufhdr *))
#define FIFO_BUFFER_SIZE (FIFO_BUFHDR_SIZE + fifoinfo.fifobsz)


/*
 *	Fifo information structure.
 */
struct fifoinfo {
	int	fifobuf,	/* max # bytes stored in a fifo */
		fifomax,	/* largest size of a single write to a fifo */
		fifobsz,	/* # of data bytes in each fifo data buffer */
		fifomnb;	/* max # bytes reserved for all fifos */
};

#define PIPE_BUF (fifoinfo.fifobuf)
#define PIPE_MAX (fifoinfo.fifomax)
#define PIPE_BSZ (fifoinfo.fifobsz)
#define PIPE_MNB (fifoinfo.fifomnb)

int fifo_alloc;			/* total number of bytes reserved for fifos */
struct fifoinfo	fifoinfo = {4096, ~0, 4096, 4096000};	 /* fifo parameters */

#endif KERNEL
