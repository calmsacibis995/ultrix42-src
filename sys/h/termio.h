
/*
 * 	@(#)termio.h	4.2	(ULTRIX)	9/4/90
 */

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

/************************************************************************
 *
 *		modification history
 *
 *	05-02-86 Tim Burke - Added CMIN & CTIME to defaults tchars.
 *
 *	07-02-86 Tim Burke - Put an ifdef around the termio structure
 *			     to avoid redefinition with ttyio.h  .
 *
 *	11-19-87 Tim Burke - Made VMIN and VTIME separate control characters
 *			     as opposed to overlaping them with VEOF and VEOL.
 *
 *	3-29-90	Kuo Hsieh  - Added _POSIX_SOURCE to enclose non-posix
 *			     symbols.
 *
 */

#ifndef _TERMIO_
#define _TERMIO_
#ifdef KERNEL
#include "../h/ansi_compat.h"
#include "../h/ioctl.h"
#else /* KERNEL */
#include <ansi_compat.h>
#ifndef _IOCTL_
#include <sys/ioctl.h>
#endif /*  _IOCTL_ */
#endif /* KERNEL */
/* control characters */
#define	VINTR	0
#define	VQUIT	1
#define	VERASE	2
#define	VKILL	3
#define	VEOF	4
#define	VEOL	5
#if !defined(_POSIX_SOURCE)
#define	VEOL2	6
#define	VSWTCH	7
#endif
#define	VMIN	8
#define	VTIME	9

#if !defined(_POSIX_SOURCE)
#define	CNUL	0
#define	CDEL	0377
/* default control chars */
#define	CESC	'\\'
#define	CSWTCH	032	/* cntl z */
#define	CNSWTCH	0
#endif


/* input modes */
#define	IGNBRK	0000001
#define	BRKINT	0000002
#define	IGNPAR	0000004
#define	PARMRK	0000010
#define	INPCK	0000020
#define	ISTRIP	0000040
#define	INLCR	0000100
#define	IGNCR	0000200
#define	ICRNL	0000400
#if !defined(_POSIX_SOURCE)
#define	IUCLC	0001000
#endif
#define	IXON	0002000
#if !defined(_POSIX_SOURCE)
#define	IXANY	0004000
#endif
#define	IXOFF	0010000

/* output modes */
#define	OPOST	0000001
#if !defined(_POSIX_SOURCE)
#define	OLCUC	0000002
#define	ONLCR	0000004
#define	OCRNL	0000010
#define	ONOCR	0000020
#define	ONLRET	0000040
#define	OFILL	0000100
#define	OFDEL	0000200
#define	NLDLY	0000400
#define	CRDLY	0030000
#define	TABDLY	0006000
#define	TAB3	0006000
#define	BSDLY	0100000
#define	VTDLY	0001000
#define	VT0	0
#define	VT1	0001000
#define	FFDLY	0040000
#endif

/* control modes */
#if !defined(_POSIX_SOURCE)
#define	CBAUD	0000017
#endif
#define	B0	0
#define	B19200	0000016
#define	B38400	0000017
#define	CSIZE	0000060
#define	CS5	0
#define	CS6	0000020
#define	CS7	0000040
#define	CS8	0000060
#define	CSTOPB	0000100
#define	CREAD	0000200
#define	PARENB	0000400
#define	PARODD	0001000
#define	HUPCL	0002000
#define	CLOCAL	0004000
#if !defined(_POSIX_SOURCE)
#define	LOBLK	0010000
#endif

/* line discipline 0 modes */
#define	ISIG	0000001
#define	ICANON	0000002
#if !defined(_POSIX_SOURCE)
#define	XCASE	0000004
#endif
#define	ECHOE	0000020
#define	ECHOK	0000040
#define	ECHONL	0000100
#define	NOFLSH	0000200

#if !defined(_POSIX_SOURCE)
#define	SSPEED	7	/* default speed: 300 baud */

#define	IOCTYPE	0xff00

#define	TIOC	('T'<<8)
#define	TCDSET	(TIOC|32)

#define	LDIOC	('D'<<8)
#define	LDOPEN	(LDIOC|0)
#define	LDCLOSE	(LDIOC|1)
#define	LDCHG	(LDIOC|2)
#define	LDGETT	(LDIOC|8)
#define	LDSETT	(LDIOC|9)

/*
 * Terminal types
 */
#define	TERM_NONE	0	/* tty */
#define	TERM_TEC	1	/* TEC Scope */
#define	TERM_V61	2	/* DEC VT61 */
#define	TERM_V10	3	/* DEC VT100 */
#define	TERM_TEX	4	/* Tektronix 4023 */
#define	TERM_D40	5	/* TTY Mod 40/1 */
#define	TERM_H45	6	/* Hewlitt-Packard 45 */
#define	TERM_D42	7	/* TTY Mod 40/2B */

/*
 * Terminal flags
 */
#define TM_NONE		0000	/* use default flags */
#define TM_SNL		0001	/* special newline flag */
#define TM_ANL		0002	/* auto newline on column 80 */
#define TM_LCF		0004	/* last col of last row special */
#define TM_CECHO	0010	/* echo terminal cursor control */
#define TM_CINVIS	0020	/* do not send esc seq to user */
#define TM_SET		0200	/* must be on to set/res flags */

/*
 * structure of ioctl arg for LDGETT and LDSETT
 */
struct	termcb	{
	char	st_flgs;	/* term flags */
	char	st_termt;	/* term type */
	char	st_crow;	/* gtty only - current row */
	char	st_ccol;	/* gtty only - current col */
	char	st_vrow;	/* variable row */
	char	st_lrow;	/* last row */
};

#ifndef _TTYCHARS_
#define	CERASE	'#'
#define	CKILL	'@'
#define	CINTR	0177	/* DEL */
#define	CQUIT	034	/* FS, cntl | */
#define	CSTART	021	/* cntl q */
#define	CSTOP	023	/* cntl s */
#define	CEOF	04	/* cntl d */
#define	CMIN	06	/* satisfy read at 6 chars */
#define	CTIME	01	/* .1 sec inter-character timer */
#endif /* _TTYCHARS_ */

#ifndef _TTYDEV_
#define	B50	0000001
#define	B75	0000002
#define	B110	0000003
#define	B134	0000004
#define	B150	0000005
#define	B200	0000006
#define	B300	0000007
#define	B600	0000010
#define	B1200	0000011
#define	B1800	0000012
#define	B2400	0000013
#define	B4800	0000014
#define	B9600	0000015
			/* EXTA and EXTB should be deleted when cleaned */
			/* out of the source */
#define	EXTA	0000016
#define	EXTB	0000017
			/* they are aliases for b19200 and B38400 */
#endif /* _TTYDEV_ */



#ifdef __SYSTEM_FIVE
#define	ECHO	0000010
#define	NL0	0
#define	NL1	0000400
#define	TAB0	0
#define	TAB1	0002000
#define	TAB2	0004000
#define	CR0	0
#define	CR1	0010000
#define	CR2	0020000
#define	CR3	0030000
#define	FF0	0
#define	FF1	0040000
#define	BS0	0
#define	BS1	0100000

#endif /* __SYSTEM_FIVE */
/*
 * Ioctl control packet
 */
#ifndef _TERM_ST_
#define _TERM_ST_
#define	NCC	10

struct termio {
	unsigned short	c_iflag;	/* input modes */
	unsigned short	c_oflag;	/* output modes */
	unsigned short	c_cflag;	/* control modes */
	unsigned short	c_lflag;	/* line discipline modes */
	char	c_line;		/* line discipline */
	unsigned char	c_cc[NCC];	/* control chars */
};
#endif /* _TERM_ST_ */
/*
 * These flags are termio-only functions not provided by the Berkeley terminal
 * subsystem.  These are used in the mapping routine as well as in the close
 * routine of each driver.
 */
#define TERMIO_ONLY_IFLAG  (IGNBRK|IGNPAR|PARMRK|INLCR|IGNCR|BRKINT|INPCK|ISTRIP|IXON)
#define TERMIO_ONLY_OFLAG  (ONOCR|ONLRET|OFILL|OFDEL|OCRNL)
#define TERMIO_ONLY_CFLAG  (CREAD|PARENB|LOBLK)
#define TERMIO_ONLY_LFLAG  (ISIG|ECHOK|ECHONL)

#endif

#endif
