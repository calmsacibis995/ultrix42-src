/* SCCSID: @(#)gw_screen.h	4.1	ULTRIX	9/11/90 */

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

/************************************************************************
 *			Modification History				*
 *									*
 *	16 December 1988	Jeffrey Mogul/DECWRL			*
 *		Created.						*
 *									*
 ************************************************************************/

/*
 * Definitions for Gateway Screening mechanism
 */

/*
 * Access to this facility should be by a new system call, but
 * to keep things simple, we use several new ioctls instead.
 */

#define	SIOCSCREENON	_IOWR('i', 49, int)
		/*
		 * Turns screening on/off (based on arg)
		 * and returns previous setting (value-result arg)
		 */
#define	SCREENMODE_OFF		0	/* THIS MUST BE ZERO */
#define	SCREENMODE_ON		1	/* THIS MUST BE NONZERO */
#define	SCREENMODE_NOCHANGE	2
		/* any other value == NOCHANGE as well */

#define	SIOCSCREEN	_IOWR('i', 50, struct screen_data)
		/*
		 * Transmits from user to kernel an screen_data
		 * struct, and then copies the next one from kernel
		 * to user into the same buffer (value-result arg).
		 * This allows us to do each transaction with
		 * only one system call.
		 *
		 * This ioctl blocks until the next transaction
		 * is available.
		 */

#define	SIOCSCREENSTATS	_IOR('i', 51, struct screen_stats)
		/*
		 * Provides current statistics block
		 */

/*
 * For each packet, a transaction occurs where the kernel
 * passes the packet header out to the user process, which
 * checks the values and then tells the kernel whether or
 * not to allow the packet.
 *
 * We stick this header struct before the packet itself.
 * Some fields of this struct are "output" fields (kernel write,
 * user read), and some are "input" (user write, kernel read).
 */

struct screen_data_hdr {
	short	sdh_count;	/* length of entire record */	/* OUT */
	short	sdh_dlen;	/* bytes of packet header */	/* OUT */
	u_long	sdh_xid;	/* transaction ID */		/* OUT */
	struct timeval sdh_arrival;				/* OUT */
				/* time at which this packet arrived */
	short	sdh_family;	/* address family */		/* OUT */

	int	sdh_action;	/* disposition for this pkt */	/* IN */
				/*	see defs below      */

	/* Other fields: incoming i/f name? */
};

/*
 * Possible dispositions of the packet
 */
#define	SCREEN_ACCEPT	0x0001	/* Accept this packet */
#define	SCREEN_DROP	0x0000	/* Don't accept this packet */
#define	SCREEN_NOTIFY	0x0002	/* Notify the sender of failure */
#define	SCREEN_NONOTIFY	0x0000	/* Don't notify the sender */

/*
 * Screening information + the actual packet
 */

#define	SCREEN_MAXLEN	120	/* length of struct screen_data */
#if	SCREEN_MAXLEN > _IOCPARM_MASK
THIS IS TOO BIG TO BE USED WITH IOCTL!
#endif	SCREEN_MAXLEN > IOCPARM_MASK

struct screen_data {
	struct screen_data_hdr sd_hdr;				/* IN/OUT */

#define	SCREEN_DATALEN	(SCREEN_MAXLEN - sizeof(struct screen_data_hdr))
	char	sd_data[SCREEN_DATALEN];
				/* sd_dlen bytes of pkt hdr */	/* OUT */
};

#define	sd_count	sd_hdr.sdh_count
#define	sd_dlen		sd_hdr.sdh_dlen
#define	sd_xid		sd_hdr.sdh_xid
#define	sd_action	sd_hdr.sdh_action
#define	sd_arrival	sd_hdr.sdh_arrival
#define	sd_family	sd_hdr.sdh_family

struct screen_stats {
	u_long	ss_packets;	/* total packets screened */
	u_long	ss_nobuffer;	/* dropped because buffer was full */
	u_long	ss_accept;	/* total accepted */
	u_long	ss_reject;	/* total rejected */
	u_long	ss_badsync;	/* dropped because user was out of sync */
	u_long	ss_stale;	/* dropped because too old */
};
