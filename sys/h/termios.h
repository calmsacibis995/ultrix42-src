
/*
 * 	@(#)termios.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * termios.h
 *
 * Modification history
 *
 * Terminal Attribute Definitions
 *
 * 05-june-90 - Kuo-Hsiung Hsieh
 *	Get rid of macro definition for tcsendbreak().   tcsendbreak()
 *	is defined in C library.  
 *	
 * 02-Feb-90 - Kuo-Hsiung Hsieh
 *	Added external function declarations to make VSX test suite happy.
 *
 * 01-Dec-87 - Tim Burke
 *	Created this file for terminal specific definitions as specified by
 *	the POSIX termios(7) standard.
 *
 */

#ifndef _TERMIOS_
#define _TERMIOS_

#ifdef KERNEL
#include "../h/termio.h"
#else /* KERNEL */
#include <sys/termio.h>
#endif /* KERNEL */

/*
 * control characters 
 *
 * The first 9 control characters are defined in termio.h.  Those characters
 * will be common to both the System V termio and POSIX termios data structures.
 */
#define NCCS 19
#define VSTART	10
#define VSTOP	11
#define VSUSP 	12

#if !defined(_POSIX_SOURCE)
#define VDSUSP	13
#define VRPRNT	14
#define VFLUSH	15
#define VWERASE	16
#define VLNEXT	17
#define VQUOTE	18
#endif

/*
 * Unsigned integral type definitions for members of the termios data structure.
 * "speed_t" is the same type as the c_cflag because it masks of this field.
 */

typedef unsigned long tcflag_t;
typedef unsigned long speed_t;
typedef unsigned char cc_t;

/*
 * termios data structure
 *
 * This structure is used to represent all terminal attributes.  
 */

struct termios {
	tcflag_t	c_iflag;		/* Input Modes 		*/
	tcflag_t	c_oflag;		/* Output Modes		*/
	tcflag_t	c_cflag;		/* Control Modes	*/
	tcflag_t	c_lflag;		/* Local Modes 		*/
	cc_t		c_cc[NCCS];		/* Control Characters	*/
	cc_t		c_line;			/* line disc. -local ext*/
};



/*
 * Set terminal attributes as specified by the termios data structure.
 * The action fields are defined in ioctl.h, they include:
 *	TCSANOW		- Changes occur immediately.
 *	TCSADRAIN	- Changes occur after output drain.
 *	TCSADFLUSH	- Changes occur after output drain and input flush. 
 */
#if !defined(_POSIX_SOURCE)
#define tcsetattr(fildes,action,termios_p) 	\
	ioctl(fildes,action,termios_p)

#define tcgetattr(fildes,termios_p) 	\
	ioctl(fildes,TCGETP,termios_p)
#else
extern	int tcsetattr();
extern	int tcgetattr();
#endif

/*
 * Send a break after output has drained.  The length of the break is specified
 * by the duration parameter (in 10th's of a second).  If duration is zero the
 * duration will be 1/4 of a second.
 * tcsendbreak do not use macro definition now.  Because negative duration
 * is allowed in the second argument, implemented tcsendbreak in macro will
 * cause second argument to be evaluated more than once.
 */
extern	int tcsendbreak();

/*
 * Wait for output to drain.
 */
#if !defined(_POSIX_SOURCE) 
#define tcdrain(fildes)		\
	ioctl(fildes,TCSBRK,-1)
#else
extern	int tcdrain();
#endif

/*
 * Flush the input or output queues as specified by the queue parameter.  The
 * queue specifications are as follows:
 */
#define TCIFLUSH 	0		/* Flush input queue */
#define TCOFLUSH 	1		/* Flush output queue */
#define TCIOFLUSH 	2		/* Flush input & output queue */
#if !defined(_POSIX_SOURCE)
#define tcflush(fildes,queue)	\
	ioctl(fildes,TCFLSH,queue)
#else
extern	int tcflush();
#endif

/* 
 * Suspend transmission or reception of data as specified by one of the 
 * following action flags:
 */
#define TCOOFF		0		/* suspend output */
#define TCOON		1		/* restart suspended output */
#define TCIOFF		2		/* suspend input */
#define TCION		3		/* restart suspended input */
#if !defined(_POSIX_SOURCE)
#define tcflow(fildes,action)	\
	ioctl(fildes,TCXONC,action)
#else 
extern 	int tcflow();
#endif

/*
 * Get input and output baud rates.
 */
#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)
#define OBAUD (CBAUD << 16)
#define cfgetospeed(termios_p)		\
	(((termios_p)->c_cflag & OBAUD) >> 16)

#define cfgetispeed(termios_p)		\
	((termios_p)->c_cflag & CBAUD)
#else
extern	speed_t cfgetospeed();
extern	speed_t cfgetispeed();
extern 	int 	cfsetispeed();
extern	int 	cfsetospeed();
#endif


/*
 * cfsetospeed() and cfsetispeed() have been moved to libc/gen now that
 * they require a return value.
 */

#if !defined(_POSIX_SOURCE)

/*
 * IEEE P1003 Extensions and locals over SVID termio structure.
 */
/*
 * c_iflag; 
 *	The low 16 bits are the c_iflag from SVID termio.  The following
 *	apply to the upper 16 bits.
 */ 	
#define PPENDIN 0x1		/* Retype pending input at next read or input */
#define PCBREAK 0x2		/* Limited canonical processing */

/*
 * c_oflag; 
 *	The low 16 bits are the c_oflag from SVID termio.  The following
 *	apply to the upper 16 bits.
 */ 	
#define PTILDE 0x1		/* Convert ~ to ` on output */
    /* These are for local translation only.  Not intended for general use! */
#define PFLUSHO 0x2		/* Actually a state flag.Output being flushed */
#define PLITOUT 0x4		/* Supress output translations */
    /* Note: if PNL2  definition changes, also change the cases in tty.c */
#define PNL2 	0x8		/* Newline delay type 2	*/


/*
 * c_cflag; 
 *	The low 16 bits are the c_cflag from SVID termio.  The following
 *	apply to the upper 16 bits.
 */ 	
				/* Low 4 bits are for output baud rate CBAUD */
#define PAUTOFLOW 0x10		/* Hardware controled flow control */

#endif

/*
 * c_lflag; 
 *	The low 16 bits are the c_lflag from SVID termio.  The following
 *	apply to the upper 16 bits.
 */ 	

#define _PIEXTEN 0x80		/* Enable local special characters */

#if !defined(_POSIX_SOURCE)
#define PCTLECH 0x1		/* Echo input control chars as ^X */
#define PPRTERA 0x2		/* Hardcopy terminal erase mode using \c */
#define PCRTBS  0x4		/* Backspace on erase */
#define PCRTERA 0x8		/* Printing terminal erase mode */
#define PCRTKIL 0x10		/* BS-space-BS erase entire line on kill */
#define PRAW	0x20		/* Berkeley non-canonical I/O */
#define PTOSTOP 0x40		/* Send SIGTTOU for bg output - must be 0x40 !*/
#define PIEXTEN _PIEXTEN

/*
 * User level code has these bits in the upper half of the longword to represent
 * termios extensions over System V termio.
 */
#define TPENDIN 	(PPENDIN << 16)
#define TTILDE 		(PTILDE << 16)
#define TFLUSHO 	(PFLUSHO << 16)
#define TLITOUT 	(PLITOUT << 16)
#define TCBREAK 	(PCBREAK << 16)
#define TNL2	 	(PNL2 << 16)
#define TAUTOFLOW 	(PAUTOFLOW << 16)
#define TCTLECH 	(PCTLECH << 16)
#define TPRTERA 	(PPRTERA << 16)
#define TCRTBS 		(PCRTBS << 16)
#define TCRTERA 	(PCRTERA << 16)
#define TCRTKIL 	(PCRTKIL << 16)
#define TRAW	 	(PRAW << 16)
#endif
#define IEXTEN	 	(_PIEXTEN << 16)

#endif /* _TERMIOS_ */
