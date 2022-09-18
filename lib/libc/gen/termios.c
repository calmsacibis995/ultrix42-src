
#ifndef lint
static char *sccsid = "@(#)termios.c	4.1      ULTRIX  7/3/90";
#endif
 
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
 *			Modification History				*
 *									*
 *	Tim Burke  -	2/8/88						*
 *		Added cfgetospeed,cfgetispeed,cfsetospeed,cfsetispeed.	*
 *									*
 *	Tim Burke  -	1/6/88						*
 * 		Created file to handle the following POSIX functions:	*
 *		(tcdrain, tcflow, tcflush, tcsendbreak, tcgetattr,	*
 *		 tcsetattr)						*
 *									*
 *	Tim Burke  -	5/20/88						*
 *		Change speed type to speed_t and establish a return	*
 *		value for the cfsetispeed and cfsetospeed routines.	*
 *									*
 ************************************************************************/

#include <sys/ioctl.h>
#include <sys/termios.h>


/*
 * All of the functions in this file are defined as macros in <sys/termios.h>.
 * They are redefined here for specific cases that require that they be
 * functions.  If a user level program wishes to use any of these functions
 * they must first do an '#undef tcsetattr' for example in order to have the
 * function call used as opposed to the macros.
 */

#undef tcsetattr
#undef tcgetattr
#undef tcsendbreak
#undef tcdrain
#undef tcflush
#undef tcflow
#undef cfgetospeed
#undef cfgetispeed
#undef cfsetospeed
#undef cfsetispeed

#ifndef OBUAD
#define OBAUD (CBAUD << 16)
#endif OBAUD
/*
 * Set terminal attributes as specified by the termios data structure.
 * The action fields are defined in ioctl.h, they include:
 *	TCSANOW		- Changes occur immediately.
 *	TCSADRAIN	- Changes occur after output drain.
 *	TCSADFLUSH	- Changes occur after output drain and input flush. 
 */
int tcsetattr (fildes,action,termios_p)
		int fildes;
		int action;
		struct termios *termios_p;
{
	return(ioctl(fildes,action,termios_p));
}

/*
 * Get terminal attributes.
 */
int tcgetattr (fildes,termios_p)
		int fildes;
		struct termios *termios_p;
{
	return(ioctl(fildes,TCGETP,termios_p));
}

/*
 * Send a break after output has drained.  The length of the break is specified
 * by the duration parameter (in 10th's of a second).  If duration is zero the
 * duration will be 1/4 of a second.
 */
int tcsendbreak (fildes,duration)
		int fildes;
		int duration;
{
	if (duration < 0)
		duration = -(duration);
	return(ioctl(fildes,TCSBRK,duration));
}

/*
 * Wait for output to drain.
 */
int tcdrain (fildes)
		int fildes;
{
	return(ioctl(fildes,TCSBRK,-1));
}

/*
 * Flush the input or output queues as specified by the queue parameter.
 */
int tcflush (fildes,queue)
		int fildes;
		int queue;
{
	return(ioctl(fildes,TCFLSH,queue));
}

/* 
 * Suspend transmission or reception of data as specified the action field.
 */
int tcflow (fildes,action)
		int fildes;
		int action;
{
	return(ioctl(fildes,TCXONC,action));
}

/*
 * Return the output baud rate stored in the c_flag.
 */
speed_t cfgetospeed (termios_p)
	struct termios *termios_p;
{
	return((termios_p->c_cflag & OBAUD) >> 16);
}

/*
 * Return the input baud rate stored in the c_flag.
 */
speed_t cfgetispeed (termios_p)
	struct termios *termios_p;
{
	return(termios_p->c_cflag & CBAUD);
}

/*
 * Set the output baud rate stored in the c_flag.
 * 
 * Should return -1 on error, but it is unclear what "error" could mean.
 * This routine shouldn't have to verify a valid termios_p or that speed
 * is between B0 and B38400.
 */
int cfsetospeed (termios_p, speed)
	struct termios *termios_p;
	speed_t speed;
{
	termios_p->c_cflag = (termios_p->c_cflag & (~OBAUD)) |
		((speed & CBAUD) << 16);	
	return(0);
}

/*
 * Set the input baud rate stored in the c_flag.
 */
int cfsetispeed (termios_p, speed)
	struct termios *termios_p;
	speed_t speed;
{
	termios_p->c_cflag &= ~CBAUD;
	/*
	 * If the inoput baud rate is zero, the input baud rate will be
 	 * specified by the value of the output baud rate.
	 */
	if (speed == 0)
		termios_p->c_cflag |= (termios_p->c_cflag & OBAUD) >> 16;
	else
		termios_p->c_cflag |= (speed & CBAUD);
	return(0);
}
