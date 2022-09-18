#ifndef lint
static char *sccsid = "@(#)tty.c	4.7      (ULTRIX)  4/4/91";
#endif
 
/************************************************************************
 *									*
 *			Copyright (c) 1986-1990 by			*
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
 * 									*
 *	3-Apr-91 - khh							*
 *		Fixed timeout table overflow panic at tty_pty_unblock.  *
 *									*
 *	9-Nov-90 - khh && Matt Thomas					*
 *		The total number of characters allowed in can. and raw  *
 *		queues are counted separately now.  This prevents data  *
 *		lost over lat terminal line.  Lat terminal line does    *
 *		not stop transmitting chars when total characters in    *
 *		both queue are exceeded TTYHOG.				*
 *		Fixed system hung in ttread while opening a lat         *
 *		terminal line.						*
 *									*
 *	10-Oct-90 - khh							*
 *	        Fixed the pseudo terminal hang problem due to the       *
 *		race condition in closing down the pair of pseudo	*
 *		terminals.						*
 *		Fixed termio mapping problems.                          *
 *									*
 *	05-Jun-90 - jaw							*
 *		missing smp locking.					*
 *									*
 *	4-Apr-90 - Kuo-Hsiung Hsieh					*
 *		Routines which handled VMIN and VTIME now treated       *
 *		these parameters correctly.     			*
 *									*
 *	28-Mar-90 - Kuo-Hsiung Hsieh					*
 *		Added ttydrain().  It was called by tcdrain().  It 	*
 *		will wait indefinitely for output written to the 	*
 *		device.  We spearated ttydrain() from ttywait(), so 	*
 *		process wished to exit will be protected by the 4 	*
 *		minutes timeout in ttywait().				*
 *									*
 *	24-Feb-90 - Kuo-Hsiung Hsieh					*
 *		A signal interrupted tcdrain() function was handled     *
 *		in ttioctl() and returned EINTR.			*
 *									*
 *	6-Mar-90 - jaw							*
 *		add missing splx's					*
 *									*
 *       1-Feb-90 - Tim Burke                                           *
 *              In ttywait do not flush the outq if no progress has     *
 *              been made while in posix mode.  		        *
 * 								        *
 *	10-Dec-89 - Matt Thomas						*
 *		Change CTRL references to reflect new semantics		*
 *		of CTRL.						*
 *									*
 *	17-Oct-89 - Randall Brown					*
 *		Added call to device t_baudrate to verify change	*
 *		in baudrate is valid.					*
 *									*
 *	22-Aug-89 - Randall Brown					*
 *		Changed sleep priority in ttwrite() to PERO-1. Also	*
 *		changed the ttyflush() to a break so that chars are	*
 *		just appended to already existed chars.			*
 *									*
 *	22-Aug-89 - Randall Brown					*
 *		In ttioctl(), in the TCSANOW cmd, release lock before	*
 *		calling device specific ioctl routine.			*
 *									*
 *	15-Aug-89 - Randall Brown					*
 *		Added a timeout in ttwrite for pseudo terminals. This	*
 *		is so the process will not hang if the controlling side *
 *		does not empty the queues.				*
 *									*
 *	15-Aug-89 - Randall Brown					*
 *		Changed all references of TCSADFLUSH to TCSAFLUSH	*
 *									*
 *	24-Jul-89 - Randall Brown					*
 *		No longer get lk_rq lock in ttselect().			*
 *									*
 *	21-Jul-89 - Randall Brown					*
 *		Added the functions tty_def_open() and tty_def_close()	*
 *		These are called by the drivers to do default action	*
 *		on open and close of each line.				*
 *									*
 *	12-Jun-89 - dws							*
 *		Added trusted path support.				*
 *									*
 *	 7-Jun-89 - Randall Brown					*
 *		Reworked the wakeups and unlocking of the terminal	*
 *		lock.  The wakeups are done after the terminal lock	*
 *		has been released.  This reduces the lock contention	*
 *		on the terminal lock.					*
 *									*
 *	24-Feb-89 - Randall Brown					*
 *		Changed the mapping routines so that HUPCL is no	*
 *		longer mapped to LNOHANG.				*
 *									*
 * 	13 Dec 88 -- jaw 						*
 *		use define in limits to do MAXPID check.		*
 *									*
 *	23-Sep-88 - Randall Brown					*
 *		- Moved the check for the suspend character before	*
 *		 icanon processing so that POSIX can use suspend	*
 *		  char regardless of ICANON setting.			*
 *		- Changed TCSETA and TCSANOW so that if CLOCAL has	*
 * 		  changed then the appropriate ioctl calls are made	*
 *		  to the individual drivers to monitor or ignore	*
 *		  modem status lines.					*
 *		- Changed ttread and ttwrite so that SVID will return 	*
 *		  0 chars on a read or write of a line with no		*
 *		  connection.  ttread also returns 0 chars for POSIX	*
 *		  on a read of a line with no connection.		*
 *									*
 *	15-Sep-88 - Randall Brown					*
 * 		Changed the TTIPRI of the sleep in the TCSBRK ioctl	*
 *		call to PZERO-1.  This is so the process can not be     *
 *		interrupted before returning the line to a non break	*
 *		state.							*
 *									*
 *	13-Sep-88 - Randall Brown					*
 *		Removed the TS_ISUSP flag from the state variable	*
 *		thus allowing multiple tcflow() to be called.  Fixes	*
 *		the problem when tcflush() is called after a call to    *
 *		tcflow(fildes, TCIOFF).  Also added ttstart() calls	*
 *		so that the start or stop char on the queue will be	*
 *		actually sent.						*
 *									*
 *	18-Aug-88 - Tim Burke						*
 *		Changed POSIX_V_DISABLE to POSIX_VDISABLE.		*
 *		Don't allow POSIX progs to create a new pgrp by		*
 *		doing a set process group to a non-existant group.	*
 *		Return EIO for reads and writes to orphaned process	*
 *		groups for POSIX progs.					*
 *									*
 *	29-Apr-88 - Tim Burke						*
 *		Changed how controlling terminals are assigned in	*
 *		the open routine to accomodate both Berkeley and 	*
 *		POSIX (draft 12) behavior.				*
 *									*
 *	24-Mar-88 - Tim Burke						*
 *		Added special character cc[VQUOTE] to allow quoting of  *
 * 		erase and kill on the '\' character to be disabled.	*
 *		Return SIGTTOU for background set of termio attributes. *
 *									*
 *	22-Mar-88 - Tim Burke						*
 *		Initial version of SMP support.  Lock terminal line	*
 *		when changing or examining tty state.			*
 *									*
 *  	15-Feb-88 - Tim Burke						*
 *		Removed all references to the "u" struct from 		*
 *		interrupt service routines.				*
 *									*
 *	5-Feb-88 - Tim Burke						*
 *		Added global variable ttykdb which when set to zero	*
 *		will disallow the input of KDBCHAR (^\) to cause an	*
 *		entry into kdbenter().					*
 *									*
 *	30-Dec-87 - George Mathew					*
 *		Calls kdbenter on typing ^\ from terminal		*
 * 		(only under KDEBUG)					*
 *									*
 *  	28-Dec-87 - Tim Burke						*
 *		Added checks for TIOCSPGRP and input flow control.	*
 *									*
 *  	1-Dec-87 - Tim Burke						*
 *									*
 *		Added support for both System V termio(7) and POSIX 	*
 *		termios(7).  These changes also include support for 	*
 *		8-bit canonical processing.  Changes involve:		*
 *									*
 *		- Default settings on first open depend on mode of 	*
 *	  	  open.  For termio opens the defaults are "RAW" style,	*
 *	  	  while non-termio opens default to the traditional 	*
 *		  "cooked" style.					*
 *		- The driver now represents its terminal attributes and *
 *	  	  special characters in the POSIX termios data structure*
 *	  	  This contrasts the original approach of storing 	*
 *	  	  attributes and special chars in the t_flags, ltchars 	*
 *		  and tchars.						*
 *		- New termio ioctls: TCSANOW, TCSADRAIN, TCSADFLUSH, 	*
 *	 	  TCSETA, TESETAW, TCSETAF, TCSBRK, TCXONC, TCFLSH,	*
 *	  	  TCGETA, TCGETP, TCSADRAIN, TCSADFLUSH.		*
 *		- Addition of LPASS8 to local mode word for 8-bit 	*
 *		  canonical support.					*
 *		- Mapping routines to correlate attributes between 	*
 *		  the termio and Berkeley data structures and special   *
 *		  characters.						*
 *		- MIN/TIME non-canonical termio reading capability.	*
 *		- Fill character capability for delays.			*
 *									*
 *      2-Sep-87 - Tim Burke	                                        *
 *      	Added support for hardware auto flow control on the 	*
 *      	outgoing side.  This will provide quick response to	*
 *       	start/stop characters which will reduce buffer overflow *
 *      	on the receiving  device.				*
 *                                                                      *
 *	Andy Gadsby - 7/24/87						*
 *		Fixed up to allow 8 bit character passthru              *
 *									*
 *	Fred Canter - 6/3/87						*
 *		Don't call ttywflush() in ttyopen() on workstations.	*
 *		This caused xterm and xcons to hang in ttywait()	*
 *		for 4 minutes.						*
 *									*
 *	Tim Burke   - 10/02/86						*
 *		In ttywait, change the priority level of sleep to	*
 *		PZERO-1 so that the sleep will not be interrupted.	*
 *		This should prevent a timeout table overflow which	*
 *		was resulting from the untimeout call following sleep	*
 *		not being executed due to interrupted sleeps.		*
 *									*
 *	Tim Burke    - 11/4/86						*
 *		Changed TIOCSETD so that input queues are flushed only  *
 *		when a CHANGE in line discipline is requested.		*
 *									*
 *	Tim Burke   - 08/14/86						*
 *		bug fixes: Mainly aimed at flow control.  Removed code  *
 *		that called untimeout upon receipt of a stop character  *
 *		because an incorrect assumption was made that we were in*
 *		the closing state when the ASLEEP flag is set (ohiwat). *
 *									*
 *	Miriam Amos - 06/06/86						*
 *		Change NOFLSH to BNOFLSH for svid.			*
 *									*
 *	Larry Palmer - 01/15/86						*
 *		Fixed typo in ttywait which made console crash		*
 *									*
 *	Larry Cohen  -  01/7/86						*
 *		bug fixes: control w will not erase line.		*
 *			   add timeout on close to prevent zombies.	*
 *			   send XON when cooked queue empties a little. *
 *			   prevent superfluous XON's.			*
 *									*
 *	Larry Cohen  -	10/1/85						*
 * 		Add window change ioctls: TIOCS/GWINSZ			*
 *									*
 ************************************************************************/
 
#include "../machine/reg.h"
 
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/devio.h"		/* Added as termio uses DEVIOCGET */
#include "../h/proc.h"
#include "../h/gnode.h"
#include "../h/file.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/dk.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/exec.h"

#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#ifdef mips
#include "../machine/cpu.h"
#endif mips

#include "../h/cpudata.h"
#include "../h/limits.h"
#include "../h/sys_tpath.h"
 
int ttydebug = 0;
#ifdef KDEBUG
int ttykdb = 1;			/* Default is to allow entry into kdb 	  */
				/* Make this value 0 to disallow kdbenter */
#define KDBCHAR 034		/* '^\' is the kdbenter character	  */
#endif KDEBUG


#ifndef PORTSELECTOR
#define ISPEED	B300
#define IFLAGS	(EVENP|ODDP|ECHO)
#define LFLAG (ISIG|ICANON|ECHO)
#else
#define ISPEED	B4800
#define IFLAGS	(EVENP|ODDP)
#define LFLAG (ISIG|ICANON)
#endif

/* termio flags will be set to these default values in non-termio mode to
 * provide a backward compatible ULTRIX environment.
 */
#define IFLAG (BRKINT|IGNPAR|ISTRIP|IXON|IXANY)
#define OFLAG (OPOST)
#define CFLAG (PARENB|CREAD|CS7)

/*
 * The following definitions also appear in <limits.h> and <unistd.h> for 
 * use by POSIX programs.  If any of these values change, those files must be
 * updated accordingly.
 */
#define POSIX_VDISABLE 0		/* Disable control character function */

/*
 * Look for line delimeters in canonical processing.
 * Avoid calling a subroutine to do this since it will be done for virtually
 * every character in non-raw mode.
 */
#define TTBREAKC(c,tp) \
	((tp->t_line == TERMIODISC) ? \
	 ((c == '\n') || ((c==tp->t_cc[VEOL] || c==tp->t_cc[VEOF]) && \
	  (c != POSIX_VDISABLE))) :\
	 (c == '\n' || c == tp->t_cc[VEOF] || c == tp->t_cc[VEOL] || \
	 (c == '\r' && (tp->t_iflag&ICRNL))))


/*
 * Table giving parity for characters and indicating
 * character classes to tty driver.  In particular,
 * if the low 6 bits are 0, then the character needs
 * no special processing on output.
 *
 * The parity aspect of this table is largely historical.  The most important
 * use of this table is by the ttyoutput routine which looks at the low order
 * bits to see if special (delay) processing may be needed on this character.
 */
 
char partab[] = {
	0001,0201,0201,0001,0201,0001,0001,0201,
	0202,0004,0003,0205,0007,0206,0201,0001,
	0201,0001,0001,0201,0001,0201,0201,0001,
	0001,0201,0201,0001,0201,0001,0001,0201,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0201,
	/*
	 * 7 bit ascii ends with the last character above,
	 * but we contine through all 256 codes for the sake
	 * of the tty output routines which use special vax
	 * instructions which need a 256 character trt table.
	 */
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000
};
 
/*
 * Input mapping table-- if an entry is non-zero, when the
 * corresponding character is typed preceded by "\" the escape
 * sequence is replaced by the table value.  Mostly used for
 * upper-case only terminals.
 */
char	maptab[] ={
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,'|',000,000,000,000,000,'`',
	'{','}',000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,'~',000,
	000,'A','B','C','D','E','F','G',
	'H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W',
	'X','Y','Z',000,000,000,000,000,
};
 
/* This table is used to map delay types between ULTRIX and termio styles */
 
struct ttdelaystruct {
	int vmode;		/* termio modes */
	int vmask;
	int umode;		/* ULTRIX modes */
	int umask;
} ttdelaystruct[] = {
	/*
	 * A table to map delay specifications - nonzero definitions only
	 * termio        	ULTRIX
	 * ------    		------
	 */
 
	NL1,  NLDLY,		NL2,   NLDELAY,
	CR1,  CRDLY,		CR3,   CRDELAY,	
	CR2,  CRDLY,		CR1,   CRDELAY,
	CR3,  CRDLY,		CR2,   CRDELAY,
	TAB1, TABDLY,		TAB1,  TBDELAY,
	TAB2, TABDLY,		TAB2,  TBDELAY,
	TAB3, TABDLY,		XTABS, TBDELAY,
	BS1,  BSDLY,		BS1,   BSDELAY,
	VT1,  VTDLY,		VTDELAY,   VTDELAY,
	FF1,  FFDLY,		VTDELAY,   VTDELAY,
	0
};
 
 
/*
 * The high and low water marks are based on line speed.  The faster the line
 * the higher the limits.
 */
short	tthiwat[16] =
   { 100,100,100,100,100,100,100,200,200,400,400,400,650,650,1300,2000 };
short	ttlowat[16] =
   {  30, 30, 30, 30, 30, 30, 30, 50, 50,120,120,120,125,125,125,125 };
 
struct winsize initwinsize = { 0, 0, 0, 0};
 
/*
 * Default settings for special characters which are stored in the cc array.
 */
u_char defaultchars[NCCS] =
	{CINTR, CQUIT, CERASE, CKILL, CEOF, CBRK, 0, 0, CMIN, CTIME,
	 CSTART, CSTOP, CSUSP, CDSUSP, CRPRNT, CFLUSH, CWERASE, CLNEXT,
	 CQUTE};

/*
 * It is not possible to map LNEXT, FLUSH, SUSP, and WERASE from an SVID
 * termio enviornment to a non-SVID environment.
 */
u_char defchars_sysv[NCCS] =
	{CINTR, CQUIT, CERASE, CKILL, CEOF, CBRK, 0, 0, CMIN, CTIME,
	 CSTART, CSTOP, POSIX_VDISABLE, CDSUSP, CRPRNT, POSIX_VDISABLE, 
	 POSIX_VDISABLE, POSIX_VDISABLE, POSIX_VDISABLE};

/*
 * Setup special characters to default settings.
 * SMP drivers are required to take out the tty lock prior to entry.
 */
ttychars(tp)
	struct tty *tp;
{
	register int s;
	/*
	 * Some of the default characters are not allowed in SYSV.
	 */
	if (u.u_procp->p_progenv == A_SYSV) 
		bcopy((caddr_t)defchars_sysv, (caddr_t)tp->t_cc, NCCS);
	else
		bcopy((caddr_t)defaultchars, (caddr_t)tp->t_cc, NCCS);
}
 
/*
 * Wait for output to drain, then flush input waiting.
 */
ttywflush(tp)
	register struct tty *tp;
{
	TTY_ASSERT(tp);
 
	ttywait(tp);
	ttyflush(tp, FREAD);
}
 

/*
 * Wait for output to drain on the specified line.  
 * Posix mode:
 *	No timers are used at all.  The implication is that the close or
 *	tcdrain could block indefinitely.  To help this situation sleep at
 *	an interruptable priority. 
 */
ttydrain(tp)
	register struct tty *tp;
{
	register int poststart_arg = NO_POST_START;

	TTY_ASSERT(tp);

	while ((tp->t_outq.c_cc || tp->t_state&(TS_BUSY|TS_TTSTOP)) &&
	    (tp->t_state&(TS_CARR_ON|TS_IGNCAR))
	    && tp->t_oproc) { /* klude for pty */
		poststart_arg = (*tp->t_oproc)(tp);
		tp->t_state |= TS_ASLEEP;
                if (poststart_arg && (tp->t_smp&S_POST_START)) {
                	tp->t_smp &= ~S_DO_UNLOCK;
			(*tp->t_poststart)(tp, poststart_arg);
	    	}
                TTY_SLEEP_RELOCK(tp,(caddr_t)&tp->t_outq, TTOPRI);
	}
        tp->t_state &= ~TS_ASLEEP;
}

/*
 *	ttywait() is called to drain the output. Timers are used to check 
 *	for progress in the output drain.  If no progress is being made 
 *	with a specified time period (4 mins) then give up on the output 
 *	and drain the characters from the outq.  It differs from ttydrain()
 *	in two parts.  One is the four minutes timer protection (can use 
 *	shorter timer, but four minutes gave us a good idea of something
 *	might go wrong).  The other difference is it can not be interrupted
 *	by signal.
 */

ttywait(tp)
	register struct tty *tp;
{
	register int prev_cc, poststart_arg = NO_POST_START;
	extern int ttyunblock();
 
	TTY_ASSERT(tp);

	while ((tp->t_outq.c_cc || tp->t_state&(TS_BUSY|TS_TTSTOP)) &&
	    (tp->t_state&(TS_CARR_ON|TS_IGNCAR))
	    && tp->t_oproc) { /* klude for pty */
		prev_cc = tp->t_outq.c_cc;
		poststart_arg = (*tp->t_oproc)(tp);
		tp->t_state |= TS_ASLEEP;
                if (poststart_arg && (tp->t_smp&S_POST_START)) {
                	tp->t_smp &= ~S_DO_UNLOCK;
			(*tp->t_poststart)(tp, poststart_arg);
	    	}
		timeout(ttyunblock, tp, 240 * hz);
		TTY_SLEEP_RELOCK(tp,(caddr_t)&tp->t_outq, PZERO - 1);
		if ((tp->t_state & TS_ASLEEP) == 0) 
			/* If things are ok we got here because the
		 	* driver sent some characters and drained 
		 	* to the lowater mark.
		 	*/
			untimeout(ttyunblock, tp);
		else {
			/* we got here because of a timeout. If no
		 	* progress has been made then we will abort
		 	* the transmission and flush the queues.
                       	* This flush is being done to allow the close to
                       	* complete on a line that got stuck in the stopped
                       	* state.
		 	*/
			untimeout(ttyunblock, tp);
			tp->t_state &= ~TS_ASLEEP;
			if (prev_cc == tp->t_outq.c_cc)
				ttyflush(tp, FWRITE);
#ifdef TTYDEBUG
			if(ttydebug)
				mprintf("timeout on output queue\n");
#endif TTYDEBUG
		}
	}
        tp->t_state &= ~TS_ASLEEP;
}
 

/*
 * This routine is called to free up a line that gets stuck in a stopped or
 * busy state.  The intent is that this routine gets indirectly called from
 * the close routine if there are characters remaining to be sent and no
 * progress has been made.  
 */

ttyunblock(tp)
	register struct tty *tp;
{
	register int s;
	TTY_LOCK(tp,s);
	tp->t_state &= ~(TS_TTSTOP|TS_BUSY|TS_OABORT);
	TTY_UNLOCK(tp,s);
	wakeup((caddr_t)&tp->t_outq);
}
	
/*
 * Flush selected TTY queues
 */
ttyflush(tp, rw)
	register struct tty *tp;
{
	register int s;
 
	/*
	 * TTY lock must have been asserted prior to entering this routine.
	 *
	 * Wakeups are done here while tty lock is held.  This is unfortunate
	 * but necessary at this time due to the way this routine is called
	 * with the lock already held and it would be quite convoluted to 
	 * pass back the necessary information to defer wakeups.
 	 */
	if(tp->t_smp == 0 ) s = spltty();
	TTY_ASSERT(tp);

	/*
	 * Flush canonical input.
	 */
	if (rw & FREAD) {
		while (getc(&tp->t_canq) >= 0)
			;
		wakeup((caddr_t)&tp->t_rawq);
	}
	/*
	 * Flush output.  Call device's stop routine to abort any output
	 * which is presently in progress.
	 */
	if (rw & FWRITE) {
		wakeup((caddr_t)&tp->t_outq);
		tp->t_state &= ~TS_TTSTOP;
		(*cdevsw[major(tp->t_dev)].d_stop)(tp, rw);
		while (getc(&tp->t_outq) >= 0)
			;
	}
	/*
	 * Flush raw input.
	 */
	if (rw & FREAD) {
		while (getc(&tp->t_rawq) >= 0)
			;
		tp->t_rocount = 0;
		tp->t_rocol = 0;
		tp->t_state &= ~TS_LOCAL;
	}
	if(tp->t_smp == 0 )splx(s);
}
 
/*
 * Send stop character on input overflow.
 */
ttyblock(tp)
	register struct tty *tp;
{
	register int x, poststart_arg = NO_POST_START;
	TTY_ASSERT(tp);
 
	x = tp->t_rawq.c_cc + tp->t_canq.c_cc;
	if (tp->t_rawq.c_cc > TTYHOG) {
		ttyflush(tp, FREAD|FWRITE);
		tp->t_state &= ~TS_TBLOCK;
	}
	/*
	 * Block further input iff:
	 * Current input > threshold AND input is available to user program
	 */
	if ((tp->t_state&TS_TBLOCK)==0)
	    if (x >= TTYHOG/2 &&
	    (((tp->t_iflag_ext&PCBREAK) || !(tp->t_lflag&ICANON)) || 
		(tp->t_canq.c_cc > TTYHOG/3))) {
		if (putc(tp->t_cc[VSTOP], &tp->t_outq)==0) {
			tp->t_state |= TS_TBLOCK;
			poststart_arg = ttstart(tp);
			if (poststart_arg && (tp->t_smp&S_POST_START)) {
			    /* don't unlock lock because we are coming from */
			    /* ttyinput where the lock must be kept	    */
			    tp->t_smp &= ~S_DO_UNLOCK;
			    (*tp->t_poststart)(tp, poststart_arg);
			}
		}
	    }
}
 
/*
 * Restart typewriter output following a delay
 * timeout.
 * The name of the routine is passed to the timeout
 * subroutine and it is called during a clock interrupt.
 */
ttrstrt(tp)
	register struct tty *tp;
{
	register int s, poststart_arg = NO_POST_START;
 
	if (tp == 0)
		panic("ttrstrt");
	TTY_LOCK(tp,s);
	tp->t_state &= ~TS_TIMEOUT;
	poststart_arg = ttstart(tp);
	if (poststart_arg && (tp->t_smp&S_POST_START)) {
	    tp->t_smp |= S_DO_UNLOCK;
	    (*tp->t_poststart)(tp, poststart_arg);
	    splx(s);
	} else {
	    TTY_UNLOCK(tp,s);
	}
}
 
/*
 * Start output on the typewriter. It is used from the top half
 * after some characters have been put on the output queue,
 * from the interrupt routine to transmit the next
 * character, and after a timeout has finished.
 */
ttstart(tp)
	register struct tty *tp;
{
	register int s, poststart_arg = NO_POST_START;
 
	if (tp->t_smp == 0) s = spltty();

	TTY_ASSERT(tp);

	if ((tp->t_state & (TS_TIMEOUT|TS_TTSTOP|TS_BUSY)) == 0 &&
	    tp->t_oproc)		/* kludge for pty */
		poststart_arg = (*tp->t_oproc)(tp);

	if (tp->t_smp == 0) splx(s);
	return(poststart_arg);
}
/*
 * Used to resume.  Called from a timeout routine to indicate that a certian
 * time period has elapsed.
 */
ttywakeup(tp)
	struct tty *tp;
{
	wakeup((caddr_t)&tp->t_addr);
}
 
/*
 * Common code for tty ioctls.
 */
/*ARGSUSED*/
ttioctl(tp, com, data, flag)
	register struct tty *tp;
	register caddr_t data;
{
	register int dev = tp->t_dev;
	register int s;
	register int newflags;
	extern int nldisp;
	struct tchars tchar_str;
 
	/*
	 * If the ioctl involves modification,
	 * insist on being able to write the device,
	 * and hang if in the background.
	 */
	switch (com) {
 
	case TIOCSETD:
	case TIOCSETP:
	case TIOCSETN:
	case TIOCFLUSH:
	case TIOCSETC:
	case TIOCSLTC:
	case TIOCSPGRP:
	case TIOCLBIS:
	case TIOCLBIC:
	case TIOCLSET:
	case TIOCSTI:
	case TIOCSWINSZ:
	case TCSANOW:
	case TCSADRAIN:
	case TCFLSH:
	case TCXONC:
	case TCSBRK:
	case TCSAFLUSH:{
	    register struct proc *p = u.u_procp;
	    while (((tp->t_line == NTTYDISC) || (tp->t_line == TERMIODISC))
		&& p->p_pgrp != tp->t_pgrp &&
		tp == u.u_procp->p_ttyp && ((p->p_vm&SVFORK) == 0) &&
		!(p->p_sigignore & sigmask(SIGTTOU)) &&
		!(p->p_sigmask & sigmask(SIGTTOU))) {
			gsignal(p->p_pgrp, SIGTTOU);
			sleep((caddr_t)&lbolt, TTOPRI);
	    }
	    break;
	}
	}
 
#ifdef TTYDEBUG
	if (ttydebug > 7)
		mprintf("ttioctl: com = %d, tp=%x\n",com,tp);
#endif TTYDEBUG
	/*
	 * Process the ioctl.
	 */
	switch (com) {
 
	/* get discipline number */
	case TIOCGETD:
		*(int *)data = tp->t_line;
		break;
 
	/* set line discipline */
	case TIOCSETD: {
		register int t = *(int *)data;
		int error = 0;
 
		if ((unsigned) t >= nldisp) 
			return (ENXIO);
		/*
		 * Only do this if a change in line discipline is requested.
		 * This will prevent unnecessary flushing of input queues if
		 * a TIOCSETD is done when the line is already set to the
  		 * specified discipline.
		 */
		if (t != tp->t_line){
			TTY_LOCK(tp,s);
			(*linesw[tp->t_line].l_close)(tp);
			error = (*linesw[t].l_open)(dev, tp);
			if (error) {
				(void) (*linesw[tp->t_line].l_open)(dev, tp);
				TTY_UNLOCK(tp,s);
				return (error);
			}
			tp->t_line = t;
			TTY_UNLOCK(tp,s);
		}
		break;
	}
 
	/* prevent more opens on channel */
	case TIOCEXCL:
		TTY_LOCK(tp,s);
		tp->t_state |= TS_XCLUDE;
		TTY_UNLOCK(tp,s);
		break;
 
	case TIOCNXCL:
		TTY_LOCK(tp,s);
		tp->t_state &= ~TS_XCLUDE;
		TTY_UNLOCK(tp,s);
		break;
 
	/* hang up line on last close */
	case TIOCHPCL:
		TTY_LOCK(tp,s);
		tp->t_cflag |= HUPCL;		/* Map to termio struct */
		TTY_UNLOCK(tp,s);
		break;
 
	case TIOCFLUSH: {
		register int flags = *(int *)data;
 
		if (flags == 0)
			flags = FREAD|FWRITE;
		else
			flags &= FREAD|FWRITE;
		TTY_LOCK(tp,s);
		ttyflush(tp, flags);
		TTY_UNLOCK(tp,s);
		break;
	}
 
	/* return number of characters immediately available */
	case FIONREAD:
		TTY_LOCK(tp,s);
		*(off_t *)data = ttnread(tp);
		TTY_UNLOCK(tp,s);
		break;
 
	case TIOCOUTQ:
		*(int *)data = tp->t_outq.c_cc;
		break;
 
	case TIOCSTOP:
		TTY_LOCK(tp,s);
		if ((tp->t_state&TS_TTSTOP) == 0) {
			tp->t_state |= TS_TTSTOP;
			(*cdevsw[major(dev)].d_stop)(tp, 0);
		}
		TTY_UNLOCK(tp,s);
		break;
 
	case TIOCSTART: {
	        register int poststart_arg = NO_POST_START;

		TTY_LOCK(tp,s);
		if ((tp->t_state&TS_TTSTOP) || (tp->t_oflag_ext&PFLUSHO)) {
			tp->t_state &= ~TS_TTSTOP;
			tp->t_oflag_ext &= ~PFLUSHO;
			poststart_arg = ttstart(tp);
		}
		if (poststart_arg && (tp->t_smp&S_POST_START)) {
		    tp->t_smp |= S_DO_UNLOCK;
		    (*tp->t_poststart)(tp, poststart_arg);
		    splx(s);
		} else {
		    TTY_UNLOCK(tp,s);
		}
		break;
	    }
 
	/*
	 * Simulate typing of a character at the terminal.
	 */
	case TIOCSTI:
		if (u.u_uid && u.u_procp->p_ttyp != tp)
			return (EACCES);
		TTY_LOCK(tp,s);
		(*linesw[tp->t_line].l_rint)(*(char *)data, tp);
		TTY_UNLOCK(tp,s);
		break;
 
	case TIOCSETP:
	case TIOCSETN: {
		register struct sgttyb *sg = (struct sgttyb *)data;
		int post_ttwakeup = NO_TTWAKEUP;
		int poststart_arg = NO_POST_START;
 
		newflags = (tp->t_flags&0xffff0000) | (sg->sg_flags&0xffff);

		TTY_LOCK(tp,s);
		tp->t_cc[VERASE]  = sg->sg_erase;
		tp->t_cc[VKILL]  = sg->sg_kill;

		if (tp->t_baudrate) {
		    /* validate change in baudrate before changing structures */
		    if ((*tp->t_baudrate)(sg->sg_ispeed & CBAUD)) {
			tp->t_cflag &= ~CBAUD;
			tp->t_cflag |= sg->sg_ispeed & CBAUD;
		    }
		    if ((*tp->t_baudrate)(sg->sg_ospeed & CBAUD)) {
			tp->t_cflag_ext &= ~CBAUD;
			tp->t_cflag_ext |= sg->sg_ospeed & CBAUD;
		    }
		} else {
		    tp->t_cflag &= ~CBAUD;
		    tp->t_cflag |= sg->sg_ispeed & CBAUD;
		    tp->t_cflag_ext &= ~CBAUD;
		    tp->t_cflag_ext |= sg->sg_ospeed & CBAUD;
		}
		if (tp->t_lflag_ext&PRAW || newflags&RAW || com == TIOCSETP) {
			ttywait(tp);
			ttyflush(tp, FREAD);
		} else if ((tp->t_iflag_ext&PCBREAK) != (newflags&CBREAK)) {
			if (newflags&CBREAK) {
				struct clist tq;
 
				catq(&tp->t_rawq, &tp->t_canq);
				tq = tp->t_rawq;
				tp->t_rawq = tp->t_canq;
				tp->t_canq = tq;
			} else {
				tp->t_flags |= PENDIN;
				newflags |= PENDIN;
				if (tp->t_smp&S_POST_WAKEUP) {
				    post_ttwakeup = DO_TTWAKEUP;
				} else {
				    ttwakeup(tp);
				}
			}
		}
		tp->t_flags = newflags;
		/*
		 * This is for consistency with the new termio calls as
		 * there are now multiple variables tracking the line
		 * speed, parity, and special characters.
		 */
		ttmapU_V(tp);
 
		if (tp->t_lflag_ext & PRAW) {
			tp->t_state &= ~TS_TTSTOP;
			poststart_arg = ttstart(tp);
			if (poststart_arg && (tp->t_smp & S_POST_START)) {
			    tp->t_smp &= ~S_DO_UNLOCK;
			    (*tp->t_poststart)(tp, poststart_arg);
			}
		}
		if (post_ttwakeup) { 
		    ttwakeup(tp);	/* ttwakeup will unlock tty lock */
		    splx(s);		/* but does not lower IPL 	 */
		} else {
		    TTY_UNLOCK(tp,s);
		}
		break;
	}
 
	/* send current parameters to user */
	case TIOCGETP: {
		register struct sgttyb *sg = (struct sgttyb *)data;
 
		/* Assert lock to get a consistent set of parameters returned */
		TTY_LOCK(tp,s);	
 		sg->sg_ispeed = CBAUD & tp->t_cflag;
		/* Input and output speed are equal under SVID termio */
		if (u.u_procp->p_progenv == A_SYSV) 
 			sg->sg_ospeed = sg->sg_ispeed; 
		else
 			sg->sg_ospeed = CBAUD & tp->t_cflag_ext;
		sg->sg_erase = tp->t_cc[VERASE];
		sg->sg_kill = tp->t_cc[VKILL];
		ttmapV_U(tp);	/* Assemble Berkeley flags from termio flags */
		sg->sg_flags = tp->t_flags;
		TTY_UNLOCK(tp,s);
		break;
	}
 
	case FIONBIO:
		TTY_LOCK(tp,s);
		if (*(int *)data)
			tp->t_state |= TS_NBIO;
		else
			tp->t_state &= ~TS_NBIO;
		TTY_UNLOCK(tp,s);
		break;

	case FIOASYNC:
		TTY_LOCK(tp,s);
		if (*(int *)data)
			tp->t_state |= TS_ASYNC;
		else
			tp->t_state &= ~TS_ASYNC;
		TTY_UNLOCK(tp,s);
		break;
 
	case FIOSINUSE:
		/*
		 * Used for shared lines.  Set the state to inuse and wakeup
		 * any process which is presently waiting for carrier to come
		 * up on the line.  This allows the open to return EALREADY
		 * which will cause the process to sleep on a gnode address
		 * instead.
		 */
		TTY_LOCK(tp,s);
		tp->t_state |= TS_INUSE;
		TTY_UNLOCK(tp,s);
		wakeup((caddr_t)&tp->t_rawq);
		break;
 
	case FIOCINUSE:
		/*
		 * Used for shared lines.  Clear the inuse state.  At the 
		 * gfs level a wakeup will be done on the gnode address to
		 * cause any process which was previously waiting for carrier
		 * to come up to resume waiting now that the line is available.
		 */
		TTY_LOCK(tp,s);
		tp->t_state &= ~(TS_INUSE);
		TTY_UNLOCK(tp,s);
		break;
 
	case TIOCGETC:
		/*
		 * TIOCGETC, SETC, SLTC, and GLTC should first do a bcopy to
		 * a local buffer before passing to "U" area to prevent 
		 * potential discrepencies if an interrupt occurs in the
		 * middle of a bcopy operation.
		 */
		/* Assemble tchars from termio control characters array. */
		/* Assert lock to get a consistent set of parameters returned */
		TTY_LOCK(tp,s);	
 		tchar_str.t_intrc = tp->t_cc[VINTR];
 		tchar_str.t_quitc = tp->t_cc[VQUIT];
                tchar_str.t_startc =  tp->t_cc[VSTART];
                tchar_str.t_stopc = tp->t_cc[VSTOP]; 
 		tchar_str.t_eofc = tp->t_cc[VEOF];
 		tchar_str.t_brkc = tp->t_cc[VEOL];
		TTY_UNLOCK(tp,s);

		bcopy((caddr_t)&tchar_str, data, sizeof (struct tchars));
		break;
 
	case TIOCSETC:
		bcopy(data, (caddr_t)&tchar_str, sizeof (struct tchars));
		/*
		 * No auto flow control allowed if startc != ^q and startc !=
		 * ^s.  Most drivers do not allow this to be changed.
		 */
		if ((tchar_str.t_stopc != CTRL('s')) || (tchar_str.t_startc != CTRL('q'))) {
			if  (tp->t_cflag_ext & PAUTOFLOW) {
				TTY_LOCK(tp,s);
				tp->t_cflag_ext &= ~PAUTOFLOW;
				TTY_UNLOCK(tp,s);
				/*
			 	 * Cause AUTOFLOW negation to take place in 
			 	 *  device's line parameter register.
			 	 */
                        	cdevsw[major(tp->t_dev)].d_ioctl(tp->t_dev,TIOAUTO);
			}
			/* startc and stopc are not changable for SVID termio */
			if (u.u_procp->p_progenv == A_SYSV) {
				tchar_str.t_startc = CTRL('q');
				tchar_str.t_stopc =  CTRL('s');
			}
		}
		/*
		 * Need to put in update of intrc, quitc, and eofc for
		 * consistency with duplicate special characters between
		 * ultrix and termio.
		 */
		TTY_LOCK(tp,s);
		tp->t_cc[VINTR] = tchar_str.t_intrc;
		tp->t_cc[VQUIT] = tchar_str.t_quitc;
		tp->t_cc[VSTART] = tchar_str.t_startc;
		tp->t_cc[VSTOP] = tchar_str.t_stopc;
                tp->t_cc[VEOF] = tchar_str.t_eofc;
		tp->t_cc[VEOL] = tchar_str.t_brkc;
		TTY_UNLOCK(tp,s);
		break;
 
	/* set/get local special characters */
	case TIOCSLTC:
		TTY_LOCK(tp,s);
		bcopy(data, (caddr_t)&tp->t_cc[VSUSP], sizeof (struct ltchars));
		TTY_UNLOCK(tp,s);
		break;
 
	case TIOCGLTC:
		TTY_LOCK(tp,s);
		bcopy((caddr_t)&tp->t_cc[VSUSP], data, sizeof (struct ltchars));
		TTY_UNLOCK(tp,s);
		break;
 
	/*
	 * Modify local mode word.
	 */
	case TIOCLBIS:
		TTY_LOCK(tp,s);
		tp->t_flags |= *(int *)data << 16;
                /*
                 * Clear the AUTOFLOW flag if this device does not provide
                 * that capability.
                 */
                if (tp->t_flags&AUTOFLOW) {
		    TTY_UNLOCK(tp,s);
                    if (cdevsw[major(tp->t_dev)].d_ioctl(tp->t_dev,TIOAUTO)) {
			TTY_LOCK(tp,s);
                        tp->t_flags &= ~AUTOFLOW;
		    }
		    else
			TTY_LOCK(tp,s);
		}
		/*
		 * This is for consistency with the new termio calls as
		 * there are now multiple variables tracking the line
		 * speed, parity, and special characters.
		 */
		ttmapU_V(tp);
		TTY_UNLOCK(tp,s);
		break;
 
        case TIOCLBIC: {
		TTY_LOCK(tp,s);
		tp->t_flags &= ~(*(int *)data << 16);
                /*
                 * Cause this change to hit the device's param routine.
                 */
                if ((tp->t_cflag_ext&PAUTOFLOW) && ((tp->t_flags&AUTOFLOW)==0)){
			/*
			 * tty lock must be free prior to calling ioctl().
			 */
			TTY_UNLOCK(tp,s);
                      	cdevsw[major(tp->t_dev)].d_ioctl(tp->t_dev,TIOAUTO);
			TTY_LOCK(tp,s);
		}
		/*
		 * This is for consistency with the new termio calls as
		 * there are now multiple variables tracking the line
		 * speed, parity, and special characters.
		 */
		ttmapU_V(tp);
		TTY_UNLOCK(tp,s);
		break;
	}
 
	case TIOCLSET:
		TTY_LOCK(tp,s);
		tp->t_flags &= 0xffff;
		tp->t_flags |= *(int *)data << 16;
                /*
                 * Clear the AUTOFLOW flag if this device does not provide
                 * that capability.
		 */
                if (tp->t_flags&AUTOFLOW) {
		    TTY_UNLOCK(tp,s);
                    if (cdevsw[major(tp->t_dev)].d_ioctl(tp->t_dev,TIOAUTO)) {
			TTY_LOCK(tp,s);
                        tp->t_flags &= ~AUTOFLOW;
		    }
		    else
			TTY_LOCK(tp,s);
		}
		/*
		 * This is for consistency with the new termio calls as
		 * there are now multiple variables tracking the line
		 * speed, parity, and special characters.
		 */
		ttmapU_V(tp);
		TTY_UNLOCK(tp,s);
		break;
 
	case TIOCLGET:
		TTY_LOCK(tp,s);
		ttmapV_U(tp);
		/* Assemble flags field from termio fields */
		*(int *)data = ((unsigned) tp->t_flags) >> 16;
		TTY_UNLOCK(tp,s);
		break;
 
	case TIOCSPGRP: {
		register int proc_group = *(int *)data;

		if ((proc_group <= 0) || (proc_group > PID_MAX)) 
			return(EINVAL);
		/*
		 * This check should probably always be done, not just in
		 * POSIX mode.
		 */
		if (u.u_procp->p_progenv == A_POSIX) {
			/*
			 * The file associated with the fd of this call must
		 	 * be the controling terminal of the calling process.
			 *
			 * The process must already have a controlling tty.
			 */
			if (((int)u.u_procp->p_ttyp == 0) ||
			    ((int)tp != (int)u.u_procp->p_ttyp)) 
				return(ENOTTY);	
		}
		/*
		 * Before changing the process group make sure that
		 * if another process is in proc_group, it must have
		 * the same controling tty as the caller.
		 */
		if (proc_search(proc_group)) 
			return(EPERM);	
		TTY_LOCK(tp,s);
		tp->t_pgrp = proc_group;
		TTY_UNLOCK(tp,s);
		break;
	}
 
	case TIOCGPGRP:
		/*
		 * For POSIX, if the calling process does not have a controlling
		 * terminal or the file is not the controlling terminal return
		 * ENOTTY.
		 */
		if (u.u_procp->p_progenv == A_POSIX) {
			if ((u.u_procp->p_ttyp == 0)||(u.u_procp->p_ttyp != tp))
				return(ENOTTY);
		}
		*(int *)data = tp->t_pgrp;
		break;
 
	case TIOCSWINSZ:
		if (bcmp((caddr_t)&tp->t_winsize, data,
		    sizeof (struct winsize))) {
			TTY_LOCK(tp,s);
			tp->t_winsize = *(struct winsize *)data;
			TTY_UNLOCK(tp,s);
			gsignal(tp->t_pgrp, SIGWINCH);
		}
		break;
 
	case TIOCGWINSZ:
		TTY_LOCK(tp,s);
		*(struct winsize *)data = tp->t_winsize;
		TTY_UNLOCK(tp,s);
		break;
 
	case TIOCNCAR:
		TTY_LOCK(tp,s);
		tp->t_state |= TS_IGNCAR;
		TTY_UNLOCK(tp,s);
		break;
	case TIOCCAR:
		TTY_LOCK(tp,s);
		tp->t_state &= ~(TS_IGNCAR);
		TTY_UNLOCK(tp,s);
		break;
	case TIOCMODEM:
		if (*(int *)data && u.u_uid) {
			/* if data>=0 then change is permanent */
			/* only super user can make permanent changes */
			u.u_error = EPERM;
			return(EPERM);
		}
		return(-1);
		break;
	case TIOCNMODEM:
		if (*(int *)data && u.u_uid) {
			u.u_error = EPERM;
			return(EPERM);
		}
		return(-1);
		break;
 
 
 	/*
 	 *  TCSBRK waits for the output to drain.  Depending on the
 	 *  value of data it will send a break down the line (ie drop
 	 *  the line for approximately .25 seconds).  This is for termio.
 	 */
 	case TCSBRK:	{
		int duration = 0, error = 0;
		/* 
		 * Tcdrain should return EINTR in posix environment.
		 * Inerrupt on system calls should be handled in
		 * sys_generic.  Tcdrain is a special case because
		 * it is the only ioctl which requirs EINTR to return.
		 */
		if ((u.u_procp->p_progenv == A_POSIX) && (*data == -1)) {
			if(setjmp(&u.u_qsave)) 
				return (EINTR);
			TTY_LOCK(tp,s);
 			ttydrain(tp);	/* tcdrain should wait until output drain */
			TTY_UNLOCK(tp,s);
			break;
		}
		TTY_LOCK(tp,s);
 		ttywait(tp);			/* Drain output first */
		TTY_UNLOCK(tp,s);
 		if (*data == 0) 
			duration = hz/4;	/* One quarter of a second */
		/*
		 * For SVID termio, if the argument is nonzero, don't send any
		 * break at all.
		 */
		else if ((u.u_procp->p_progenv == A_SYSV) || (*data < 0)) 
 			break;
		else
			duration = (*data)*(hz/10);	/* 10ths of a sec */
		/* 
		 * Call the device driver to start a spacing sequence.  End the
		 * spacing after the time period specified by duration.
		 */
 		error = cdevsw[major(tp->t_dev)].d_ioctl(tp->t_dev,TIOCSBRK,data,flag);
		if (error)
		    return(error);
 		timeout(ttywakeup,tp,duration);
 		sleep(&tp->t_addr, PZERO - 1);
 		cdevsw[major(tp->t_dev)].d_ioctl(tp->t_dev,TIOCCBRK,data,flag);
 		break;
	}
 
 	/*
 	 *  TCXONC is start/stop control depending of the value of data.
 	 *  This is for termio.
 	 */
 	case TCXONC: {
		struct termios cb;
		register int poststart_arg = NO_POST_START;

 		switch(*data) {
 		case TCOOFF:		/* TCOOFF - suspend output */
 			/* this is the same code as TIOCSTOP */
			TTY_LOCK(tp,s);
 			if ((tp->t_state&TS_TTSTOP) == 0) {
 				tp->t_state |= TS_TTSTOP;
 				(*cdevsw[major(tp->t_dev)].d_stop)(tp,0);
 			}
			TTY_UNLOCK(tp,s);
 			break;
 		case TCOON:		/* TCOON - restart suspended output */
			TTY_LOCK(tp,s);
 			if ((tp->t_state&TS_TTSTOP) || 
			    (tp->t_oflag_ext&PFLUSHO)) {
 				tp->t_state &= ~TS_TTSTOP;
 				tp->t_oflag_ext &= ~PFLUSHO;
 				poststart_arg = ttstart(tp);
 			}
			if (poststart_arg && (tp->t_smp & S_POST_START)) {
			    tp->t_smp |= S_DO_UNLOCK;
			    (*tp->t_poststart)(tp, poststart_arg);
			    splx(s);
			} else {
			    TTY_UNLOCK(tp,s);
			}
 			break;
			/*
			 * TCIOFF & TCION notes:
			 * As of Draft 12.3 it is unclear what this function
			 * is supposed to do. Why not just write out the STOP
			 * and start chacters from the user's process?
			 * 
			 * TCIOFF - send out a stop character.  This will not
			 *	    necessarily stop input.  To fully stop 
			 *	    input (and discard any input) clear the
			 *	    CREAD bit.
			 * TCION  - send a start character which should
			 *	    restart suspended input. 
			 *
			 */
		case TCIOFF:		/* TCIOFF - suspend input */
			if (u.u_procp->p_progenv != A_SYSV) {
				TTY_LOCK(tp,s);
				if (putc(tp->t_cc[VSTOP], &tp->t_outq)==0) {
				    poststart_arg = ttstart(tp);
				}
				if (poststart_arg && (tp->t_smp & S_POST_START)) {
				    tp->t_smp |= S_DO_UNLOCK;
				    (*tp->t_poststart)(tp, poststart_arg);
				    splx(s);
				} else {
				    TTY_UNLOCK(tp,s);
				}
			}
			break;
		case TCION:		/* TCION - restart suspended input */
			if (u.u_procp->p_progenv != A_SYSV) {
				TTY_LOCK(tp,s);
				if (putc(tp->t_cc[VSTART], &tp->t_outq)==0){
				    poststart_arg = ttstart(tp);
				}
				if (poststart_arg && (tp->t_smp & S_POST_START)) {
				    tp->t_smp |= S_DO_UNLOCK;
				    (*tp->t_poststart)(tp, poststart_arg);
				    splx(s);
				} else {
				    TTY_UNLOCK(tp,s);
				}
			}
			break;
 		default:
 			return(EINVAL);
 		}
 		break;
	}
 
 	/*
 	 *  TCFLSH flushes the queues.  If data = 0 it flushes the input
 	 *  queue.  If data = 1 it flushes the output queue.  If data =2
 	 *  it flushes both queues.  This is for termio.
 	 */
 	case TCFLSH: {
 		int flags = 0;
 
 		switch(*data) {
 		case 0:		/* TCIFLUSH - flush input queue */
 			flags = FREAD;
 			break;
 		case 1:		/* TCOFLUSH - flush output queue */
 			flags = FWRITE;
 			break;
 		case 2:		/* TCIOFLUSH - flush input & output queues */
 			flags = FREAD|FWRITE;
 			break;
 		default:
 			return(EINVAL);
 		}
		TTY_LOCK(tp,s);
 		ttyflush(tp,flags);
		TTY_UNLOCK(tp,s);
 		break;
 	}
 
 	/*
	 * SVID termio.
 	 *  TCGETA gets the parameters associated with the terminal
 	 *  as described in the struct termio for termio.
 	 */
 	case TCGETA: {
 		register struct	termio *cb = (struct termio *)data;
 
		TTY_LOCK(tp,s);
 		cb->c_iflag = tp->t_iflag;
 		cb->c_oflag = tp->t_oflag;
 		cb->c_cflag = tp->t_cflag;
 		cb->c_lflag = tp->t_lflag;
 		cb->c_line = tp->t_line;
 		bcopy(tp->t_cc,cb->c_cc,NCC);
		TTY_UNLOCK(tp,s);
 		break;
 	}
 
 	/*
	 * SVID termio.
 	 *  TCSETAW waits for output to drain.
 	 *  TCSETAF waits for output to drain and flushes the queues.
 	 *  Both TCSETAW and TCSETAF then fall through to TCSETA to
 	 *  set the parameters in the struct termio.
 	 */
 	case TCSETAW:
 	case TCSETAF:
		TTY_LOCK(tp,s);
 		ttywait(tp);
 		if (com == TCSETAF) 
 			ttyflush(tp,(FREAD|FWRITE));
		TTY_UNLOCK(tp,s);
 	case TCSETA: {
 		register struct	termio *cb = (struct termio *)data;
		int modem_changed = 0, new_data = 0;
 
		TTY_LOCK(tp,s);
	 	tp->t_iflag = cb->c_iflag;
	 	tp->t_oflag = cb->c_oflag;
		if ((tp->t_cflag & CLOCAL) != (cb->c_cflag & CLOCAL)) {
		    modem_changed++;
		}
		
		/* validate change in baudrate before changing structures */
		if (tp->t_baudrate) {
		    if (((*tp->t_baudrate)(cb->c_cflag & CBAUD)) == 0) {
			/* use old baudrate */
			cb->c_cflag &= ~CBAUD;
			cb->c_cflag |= tp->t_cflag & CBAUD;
		    }
		}
		tp->t_cflag = cb->c_cflag; 
		tp->t_cflag_ext &= ~CBAUD;
		tp->t_cflag_ext |= cb->c_cflag&CBAUD;
	 	tp->t_lflag = cb->c_lflag;
		ttmap_P(tp);	/* map POSIX extensions */
		/*
		 * Only support for termio line discipline 0 is provided.
		 * A new error flag should probably be created for the case
		 * where a user does TCSETA with lflag != 0.
		 * NTTYDISC can do both Berkeley and termio so don't change disc
		 */
		if (cb->c_line != NTTYDISC) 
			tp->t_line = TERMIODISC;

		bcopy((caddr_t)cb->c_cc,(caddr_t)tp->t_cc,NCC);
		tp->t_cc[VSTART] = CTRL('q');	/* Not changable in termio */
		tp->t_cc[VSTOP] = CTRL('s');
		/*
		 * For SVID termio it is not possible to map the following
		 * control characters and flags.  This is a result of the fact
		 * that u.u_procp->p_progenv is used to distinguish SVID termio 
		 * from POSIX termios.  The mapping is impossible because these
		 * attributes are looked at from the interrupt service routine
		 * which does not have access to the propper "u" structure.
		 */
		tp->t_lflag_ext &= ~PIEXTEN;  /* Disable extra cntrl chars */

		tp->t_lflag_ext &= ~(PCTLECH | PPRTERA);
		tp->t_oflag_ext &= ~(PTILDE);
		/*
		 * If the value of MIN is greater than the maximum number of
		 * allowable input characters then truncate the requested number
		 * to be less than the maximum so that it will have a chance
		 * of returning.
		 */
		if (tp->t_cc[VMIN] > MAX_INPUT)
			tp->t_cc[VMIN] = MAX_INPUT;
		/*
		 * SVID LOBLK in the cflag is equivalent to TOSTOP in the
		 * POSIX lflag.
		 */
		if (tp->t_cflag & LOBLK)
			tp->t_lflag_ext |= PTOSTOP;
		TTY_UNLOCK(tp,s);
 		
		if (modem_changed) {
		    if (tp->t_cflag & CLOCAL) {
			cdevsw[major(tp->t_dev)].d_ioctl(tp->t_dev,TIOCNMODEM,&new_data,0);
		    } else {
			cdevsw[major(tp->t_dev)].d_ioctl(tp->t_dev,TIOCMODEM,&new_data,0);
		    }
		}
 		break;
 	}
 
 	/*
	 * POSIX termio.
 	 *  TCGETP gets the parameters associated with the terminal
 	 *  as described in the struct termios for termio.
	 *  The termios data structure represents its flags as u_long.  The
 	 *  driver will internally store this as a lower half and an upper half
	 *  called the extension field.  This is being done to avoid any
	 *  possible dificulties in compiled code in the case of truncating
	 *  longs into shorts, etc.
 	 */
 	case TCGETP: {
 		register struct	termios *cb = (struct termios *)data;
 
		TTY_LOCK(tp,s);
 		cb->c_iflag = (tp->t_iflag_ext<<16) | tp->t_iflag;
 		cb->c_oflag = (tp->t_oflag_ext<<16) | tp->t_oflag;
 		cb->c_cflag = (tp->t_cflag_ext<<16) | tp->t_cflag;
 		cb->c_lflag = (tp->t_lflag_ext<<16) | tp->t_lflag;
 		cb->c_line = tp->t_line;
 		bcopy(tp->t_cc,cb->c_cc,NCCS);
		TTY_UNLOCK(tp,s);
 		break;
 	}
 
 	/*
	 * POSIX termio.
 	 *  TCSADRAIN waits for output to drain.
 	 *  TCSAFLUSH waits for output to drain and flushes the queues.
 	 *  Both TCSADRAIN and TCSAFLUSH then fall through to TCSANOW to
 	 *  set the parameters in the struct termios.
 	 */
 	case TCSADRAIN:
 	case TCSAFLUSH:
		TTY_LOCK(tp,s);
 		ttywait(tp);
 		if (com == TCSAFLUSH) 
 			ttyflush(tp,(FREAD|FWRITE));
		TTY_UNLOCK(tp,s);
 	case TCSANOW: {
 		register struct	termios *cb = (struct termios *)data;
		int modem_changed = 0, new_data = 0;
 
		TTY_LOCK(tp,s);
	 	tp->t_iflag = cb->c_iflag;
	 	tp->t_iflag_ext = cb->c_iflag >> 16;
	 	tp->t_oflag = cb->c_oflag;
	 	tp->t_oflag_ext = cb->c_oflag >> 16;
		/*
		 * POSIX states that setting the baud rate to unsupported
		 * speeds will be ignored.  Digital's terminal multiplexer's
		 * don't do 200 baud.  So if a user tries to set the speed to
		 * 200 baud, then leave the speed unchanged.  Check both the
		 * input and output baud rate.
		 */
		if ((tp->t_cflag & CLOCAL) != (cb->c_cflag & CLOCAL)) {
		    modem_changed++;
		}
		if (tp->t_baudrate) {
		    if (((*tp->t_baudrate)(cb->c_cflag & CBAUD)) == 0) {
			cb->c_cflag &= ~CBAUD;
			cb->c_cflag |= tp->t_cflag & CBAUD;
		    }
		    if (((*tp->t_baudrate)((cb->c_cflag & OBAUD) >> 16)) == 0) {
			cb->c_cflag &= ~OBAUD;
			cb->c_cflag |= ((tp->t_cflag_ext & CBAUD) << 16);
		    }
		}
	 	tp->t_cflag = cb->c_cflag; 
	 	tp->t_cflag_ext = cb->c_cflag >> 16;
	 	tp->t_lflag = cb->c_lflag;
	 	tp->t_lflag_ext = cb->c_lflag >> 16;
		/*
		 * Strictly speaking this should not be necessary, but it is
		 * done to prevent possible conflicts in functionality.
		 */
		ttmap_P(tp);	/* map POSIX extensions */
		/*
		 * Only support for termio line discipline 0 is provided.
		 * A new error flag should probably be created for the case
		 * where a user does TCSANOW with lflag != 0.
		 * NTTYDISC can do both Berkeley and termio so don't change disc
		 */
		if (cb->c_line != NTTYDISC) 
			tp->t_line = TERMIODISC;

		bcopy((caddr_t)cb->c_cc,(caddr_t)tp->t_cc,NCCS);
		/*
		 * If the value of MIN is greater than the maximum number of
		 * allowable input characters then truncate the requested number
		 * to be less than the maximum so that it will have a chance
		 * of returning.
		 */
		if (tp->t_cc[VMIN] > MAX_INPUT)
			tp->t_cc[VMIN] = MAX_INPUT;
		TTY_UNLOCK(tp,s);
 		
		if (modem_changed) {
		    if (tp->t_cflag & CLOCAL) {
			cdevsw[major(tp->t_dev)].d_ioctl(tp->t_dev,TIOCNMODEM,&new_data,0);
		    } else {
			cdevsw[major(tp->t_dev)].d_ioctl(tp->t_dev,TIOCMODEM,&new_data,0);
		    }
		}
 		break;
 	}
 
 
 
	default:
		return (-1);
	}
	return (0);
}
 
ttnread(tp)
	register struct tty *tp;
{
	register int nread = 0;
 
        TTY_ASSERT(tp);
	if (tp->t_iflag_ext & PPENDIN)
		ttypend(tp);
	nread = tp->t_canq.c_cc;
	if ((tp->t_iflag_ext & PCBREAK) || ((tp->t_lflag & ICANON) == 0))
		nread += tp->t_rawq.c_cc;
	return (nread);
}
 
ttselect(dev, rw)
	dev_t dev;
	int rw;
{
	register struct tty *tp = &cdevsw[major(dev)].d_ttys[minor(dev)];
	register int nread;
	register int s;
 
	TTY_LOCK(tp,s);
	switch (rw) {
 
	case FREAD:
		nread = ttnread(tp);
		/*
		 * Input available, no need to sleep.
		 */
		if (nread > 0)
			goto win;
		/*
		 * Here there is already somebody else who is waiting on this
		 * channel.  RCOLL is set to do a collective wakeup of all
		 * processes sleeping on this channel. 
		 */
		if (tp->t_rsel && tp->t_rsel->p_wchan == (caddr_t)&selwait)
			tp->t_state |= TS_RCOLL;
		/*
		 * This (active) process is the only one doing a select on this
		 * channel.  Don't set RCOLL to avoid the overhead of waking up
		 * all select sleepers.  This process will be placed on the
		 * run queue without the overhead of waking up other processes.
		 */
		else
			tp->t_rsel = u.u_procp;
		break;
 
	case FWRITE:
		/*
		 * Device ready to accept more output, no need to sleep.
		 */
		if (tp->t_outq.c_cc <= TTLOWAT(tp))
			goto win;
		/*
		 * Here there is already somebody else who is waiting on this
		 * channel.  WCOLL is set to do a collective wakeup of all
		 * processes sleeping on this channel. 
		 */
		if (tp->t_wsel && tp->t_wsel->p_wchan == (caddr_t)&selwait)
			tp->t_state |= TS_WCOLL;
		/*
		 * This (active) process is the only one doing a select on this
		 * channel.  Don't set WCOLL to avoid the overhead of waking up
		 * all select sleepers.  This process will be placed on the
		 * run queue without the overhead of waking up other processes.
		 */
		else
			tp->t_wsel = u.u_procp;
		break;
	}
	/*
	 * Select must sleep.
	 */
	TTY_UNLOCK(tp,s);
	return (0);
	/*
	 * No need to sleep.  Terminal condition exists.
	 */
win:
	TTY_UNLOCK(tp,s);
	return (1);
}

#ifdef vax 
/*
 * Non zero if the kernel is running on a workstation
 * and the console is a graphics device (sg, sm, qd, qv).
 */
 extern	int	ws_display_type;
#endif vax
 
/*
 * Establish a process group for distribution of
 * quits and interrupts from the tty.
 */
ttyopen(dev, tp)
	dev_t dev;
	register struct tty *tp;
{
	register struct proc *pp;
	register int s;

        if ( tp->t_smp == 0)s=spltty();

        TTY_ASSERT(tp);

        pp = u.u_procp;
	tp->t_dev = dev;
	/*
	 * POSIX open:
	 * If the process is a process group leader (which means that the
	 * pid == pgrp) which does not already have a controlling terminal
	 * (p_ttyp == 0) opens a file not already associated with a
	 * process group (tp->t_pgrp == 0), the terminal becomes the controlling
	 * terminal.
	 *
	 * Don't change the pp->p_pgrp, that's handled elsewhere.
	 */
	if (u.u_procp->p_progenv == A_POSIX) {
		if ((pp->p_ttyp == 0) && (pp->p_pid == pp->p_pgrp)) {
			if ((tp->t_pgrp==0) && ((tp->t_state&TS_ONOCTTY)==0)) {
				/* MBH not sure lock required here */
				smp_lock(&lk_procqs, LK_RETRY);
				tp->t_pgrp = pp->p_pgrp;
				smp_unlock(&lk_procqs);
				pp->p_ttyp = tp;
				u.u_ttyd = dev;
			}
		}
		/*
		 * The TS_ONOCTTY attribute should not be left around after
		 * the open call so that it doesn't influence future opens.
		 */
		tp->t_state &= ~TS_ONOCTTY;
	}
	/*
	 * BERKELEY open:
	 * If the process is not already in a process group put it in the 
	 * group which is presently associated with this terminal.  If the
	 * terminal doesn't already belong to a process group, create a new
	 * process group with this terminal as the controlling tty.
	 */
	else if (pp->p_pgrp == 0) {
		if (tp->t_pgrp == 0) {
			tp->t_pgrp = pp->p_pid;
		}
		pp->p_ttyp = tp;
		u.u_ttyd = dev;
		/* count on the spltty above */
		/* MBH not sure lock required here */
		smp_lock(&lk_procqs, LK_RETRY);
		pp->p_pgrp = tp->t_pgrp;
		smp_unlock(&lk_procqs);
	}
	tp->t_state &= ~TS_WOPEN;
	if ((tp->t_state&TS_ISOPEN) == 0) {
		/*
		 * Set state to indicate this is an open.
		 * Initialize the window size.
		 */
		tp->t_state |= TS_ISOPEN;
		tp->t_winsize = initwinsize;
		/*
	 	 * Perform ULTRIX - termio mappings.
	 	 * This will insure that the initial default setting are mapped 
	 	 * correctly even if no ioctl calls are made.
	 	 */
		if (tp->t_line == TERMIODISC)
			ttmap_P(tp);
		else {
			ttmapU_V(tp);
		}
		/*
		 * 6/3/87 -- Fred Canter
		 *	The following causes xterm, xcons, and other workstation
		 *	programs to hang in a 4 minute ttywait. So, we don't
		 *	do the flush on a workstation.
		 */
#ifdef vax
		if (ws_display_type == 0) {
#endif vax
		    if ((tp->t_line != NTTYDISC) && (tp->t_line != TERMIODISC))
 			ttywflush(tp);
#ifdef vax
		}
#endif vax
        }
	if ( tp->t_smp == 0) splx(s);
	return (0);
}
 
/*
 * clean tp on last close
 */
ttyclose(tp)
	register struct tty *tp;
{
	register int s;

	if ( tp->t_smp == 0) s=spltty();
 
	TTY_ASSERT(tp);
	if (tp->t_line) {
		/*
		 * Flush the buffers and reset the line to default
		 * discipline (ie old)
		 */
		/* Call ttywait and ttyflush on both input and output
		 * queues if there is no start routine to drain the
		 * characters.  We used to call ttywflush(tp), the routine 
		 * will only flush the input queue.  There is a race 
		 * condition when using pseudo terminal which causes
		 * characters left over in the output queue from the
		 * slave pseudo-terminal.  This causes the next open
		 * of the pseudo-terminal hang.
		 */
		ttywait(tp);
	    	if (tp->t_oproc) 
			ttyflush(tp, FREAD);
		else
			ttyflush(tp, FREAD | FWRITE);
		tp->t_line = 0;
		if ( tp->t_smp == 0) splx(s);
		return;
	
	}
	if (tp == u.u_procp->p_ttyp)
	    u.u_procp->p_ttyp = 0;
	tp->t_pgrp = 0;

	/* 
	 * Call ttywait and ttyflush instead of ttywflush(tp). 
	 */
	ttywait(tp);
	if (tp->t_oproc) 
		ttyflush(tp, FREAD);
	else
		ttyflush(tp, FREAD | FWRITE);

	tp->t_state = 0;
	if ( tp->t_smp == 0) splx(s);
}
 
/*
 * reinput pending characters after state switch
 * call at spltty().
 */
ttypend(tp)
	register struct tty *tp;
{
	struct clist tq;
	register int c, save_postwakeup;
      	register struct clist *tqp = &tq;
 
	TTY_ASSERT(tp);
	/*
	 * Do not provide this functionality in a pure termio environment.
	 */
	if (tp->t_line == TERMIODISC)
		return;
	tp->t_iflag_ext &= ~PPENDIN;
	tp->t_state |= TS_TYPEN;
	tq = tp->t_rawq;
	tp->t_rawq.c_cc = 0;
	tp->t_rawq.c_cf = tp->t_rawq.c_cl = 0;
	save_postwakeup = tp->t_smp; /* save status of postwakeup flag */
	tp->t_smp &= ~S_POST_WAKEUP;
	while ((c = getc(tqp)) >= 0)
		if (ttyinput(c, tp)) /* if we missed a wakeup then something is wrong */
		    panic("missed ttwakeup in ttypend");
	tp->t_smp = save_postwakeup;
	tp->t_state &= ~TS_TYPEN;
}
 
/*
 * Place a character on raw TTY input queue,
 * putting in delimiters and waking up top
 * half as needed.  Also echo if required.
 * The arguments are the character and the
 * appropriate tty structure.
 */
 
ttyinput(c, tp)
	register int c;
	register struct tty *tp;
{
        int tty_timeout();
	register u_short t_iflag = tp->t_iflag;		/* TERMIO */
	register int i,s, post_ttwakeup = NO_TTWAKEUP;
	register int poststart_arg = NO_POST_START;
	register char spec_char = 1;

#ifdef KDEBUG
	if (ttykdb && (c == KDBCHAR))	/* enter KDB if control-\ is typed */
		kdbenter();
#endif KDEBUG
        if (tp->t_smp == 0) s = spltty();
        TTY_ASSERT(tp);
 
	/*
	 * If input is pending take it first.
	 */
	if (tp->t_iflag_ext & PPENDIN)
		ttypend(tp);
	CURRENT_CPUDATA->cpu_ttyin++;	/* increment char count for stats */
	c &= CHAR_MASK;			/* clear the high bits */

	/*
	 * If the character equals POSIX_VDISABLE, then the special character
	 * function shall be disabled.
	 */
	if ((c == POSIX_VDISABLE) && (tp->t_line == TERMIODISC))
		spec_char = 0;
	    	
	/*
	 * If interrupt routine recieved a SAK, and tty has a session leader,
	 * notify handler.
	 */
	if ((do_tpath) && ((tp->t_tpath & TP_DOSAK) != 0)) {
		tp->t_tpath &= ~TP_DOSAK;
		if ((tp->t_lflag & NOFLSH) == 0) ttyflush(tp, FREAD|FWRITE);
		if ((tp->t_sid != 0) && ((tp->t_tpath & TP_DOLOGIN) == 0)) {
			tp->t_tpath |= TP_DOLOGIN;
			TPATH_WAKEUP;
		}
		if (tp->t_smp == 0) splx(s);
		return(post_ttwakeup);
	}
	    	
	/*
	 * In tandem mode, check high water mark.
	 * Cause system to transmit start/stop characters when the input
	 * queue is nearly empty/full. iflag & IXOFF maps to tandem.
	 */
 	if (t_iflag&IXOFF) 
		ttyblock(tp);	/* mucks w/ flow & possibly sends a stop char */
 
	/*
	 * Flow control mechanism.  Termio does not allow the user to
	 * modify their start and stop characters, however Berkeley does
	 * so look at these values (^S/^Q are the defaults).
	 * This code has been moved before RAW mode as termio allows for
	 * flow control even for MIN/TIME reading. To maintain backward
	 * compatibility, don't allow XON/XOFF while in RAW mode.
 	 */
	if (t_iflag&IXON) {
		register char ctmp;
		ctmp = c & CHAR_MASK;
		if (tp->t_state&TS_TTSTOP) {
			/*
			 * Restart for a start character or if any char
			 * can restart.  IXANY is opposite to DECCTQ.
			 */
			if (t_iflag&IXANY || ((ctmp == tp->t_cc[VSTART]) &&
				spec_char)) {
				tp->t_state &= ~TS_TTSTOP;
				tp->t_oflag_ext &= ~PFLUSHO;
#ifdef TTYDEBUG
				if (ttydebug > 4)
					mprintf("ttyinput: startc or IXANY, 	ctmp=(o)%o,tp=%x\n",ctmp,tp);
#endif TTYDEBUG
				poststart_arg = ttstart(tp);
				if (poststart_arg && (tp->t_smp & S_POST_START)) {
				    tp->t_smp &= ~S_DO_UNLOCK;
				    (*tp->t_poststart)(tp, poststart_arg);
				}
			}
		} else {
			/*
			 * Call device's stop routine for a stop char.
			 */
			if ((ctmp == tp->t_cc[VSTOP]) && spec_char) {
#ifdef TTYDEBUG
				if (ttydebug > 4)
					mprintf("ttyinput: stopping on stopc, 	tp=%x\n",tp);
#endif TTYDEBUG
				tp->t_state |= TS_TTSTOP;
        			if ((tp->t_cflag_ext & PAUTOFLOW) == 0) 
					(*cdevsw[major(tp->t_dev)].d_stop)(tp,0);
				if (tp->t_smp == 0) splx(s);
				return(post_ttwakeup);
			}
		}
		/* 
 		 * Remove flow control chars from input stream.
		 */
		if ((ctmp == tp->t_cc[VSTOP] || ctmp == tp->t_cc[VSTART]) &&
			spec_char) {
				if (tp->t_smp == 0) splx(s);
				return(post_ttwakeup);
		}
	}
 
	/*
	 * Look for interrupt/quit chars.
	 * This too must be done before RAW mode.
	 */
	if (tp->t_lflag & ISIG) {
		register char ctmp;
		ctmp = c & CHAR_MASK;
		if ((ctmp == tp->t_cc[VINTR] || ctmp == tp->t_cc[VQUIT]) &&
		    spec_char) {
			/*
		 	 * if interrupt or quit (^C and ^\ are the defaults)
		 	 * if NOFLUSH not set flush both buffers
		 	 * echo the char and send the appropriate sig to parent
		 	 */
			if ((tp->t_lflag&NOFLSH) == 0){
			    ttyflush(tp, FREAD|FWRITE);
			}
			ttyecho(c, tp);
			gsignal(tp->t_pgrp, ctmp == tp->t_cc[VINTR] ? SIGINT : SIGQUIT);
			goto endcase;
		}
		if ((c == tp->t_cc[VSUSP]) && ((tp->t_line == NTTYDISC) || 
	    		((tp->t_line==TERMIODISC) && spec_char))) {
			/*
			 * if suspend char (^Z is the default)
			 *	if NOFLUSH is not set, then flush the READ
			 *	buffer.
			 *	echo the suspend char and send terminal stop
			 *	signal to parent.
			 */
			if ((tp->t_lflag&NOFLSH) == 0) {
			    if (tp->t_line == NTTYDISC) {
				ttyflush(tp, FREAD); /* done for backward compatability */
			    } else {
				ttyflush(tp, FREAD | FWRITE);
			    }
			}
			ttyecho(c, tp);
			gsignal(tp->t_pgrp, SIGTSTP);
			goto endcase;
		}  
	}

 
	/* Mappings of : NL/CR and upper case to lower on input.
	 * Note: These mappings differ from LCASE and CRMOD in that they may
	 *       be done when canonical processing is not enabled.  (LCASE
	 *	 and XCASE are only used in canonical - cooked mode.  This 
	 *	 code is positioned early in the ttyinput routine to avoid
	 *	 unwanted attributes associated with cooked,  or cbreak.
	 *	 This also assumes that no input processing is to be done
	 *	 in RAW mode (same for output processing).
	 * The defaults for these flags should be false and set only when
	 * explicitly called as there are no ULTRIX counterparts.
	 */
	if (t_iflag & (INLCR|IGNCR|ICRNL|IUCLC) ) {	
		if (c == '\n' && t_iflag&INLCR)
			c = '\r';
		else if (c == '\r')
			if (t_iflag&IGNCR) {
				if (tp->t_smp == 0) splx(s);
				return(post_ttwakeup);	/* throw char away */
			}
			else if (t_iflag&ICRNL)
				c = '\n';
		/* Not the same as XCASE or LCASE */
		if (t_iflag&IUCLC && 'A' <= c && c <= 'Z')
			c += 'a' - 'A';
	}
 
	if (((tp->t_lflag & ICANON) == 0) && ((tp->t_iflag_ext&PCBREAK) == 0)) {
		/*
		 * Raw mode, just put character
		 * in input q w/o interpretation.
		 */
		if (tp->t_rawq.c_cc > TTYHOG) 
			ttyflush(tp, FREAD|FWRITE);
		else {
			if (putc(c, &tp->t_rawq) >= 0) {
				/* Min/Time has not been processed */
				/* 
				 * In the first case, VMIN should not be zero, 
				 * otherwise a character coming from the lower 
				 * half will satisfy the read request regardless 
				 * of the read timer may not expire yet.  - khh
				 */
				if ((tp->t_rawq.c_cc >= tp->t_cc[VMIN]) && 
					(tp->t_cc[VMIN])) {
				    if (tp->t_smp&S_POST_WAKEUP) {
					post_ttwakeup = DO_TTWAKEUP;
				    } else {
					ttwakeup(tp);
				    }
				} else if ((tp->t_cc[VMIN] == 0) &&
					(tp->t_cc[VTIME] == 0)) {
				    if (tp->t_smp&S_POST_WAKEUP) {
					post_ttwakeup = DO_TTWAKEUP;
				    } else {
					ttwakeup(tp);
				    }
				} else if ((tp->t_rawq.c_cc < tp->t_cc[VMIN]) && 
					(tp->t_cc[VTIME])){
				        /* If there is a timeout pending */
				        /* restart inter-byte timer */
					if (tp->t_state & TS_LTACT)
					    untimeout(tty_timeout, tp);
					tp->t_state &= ~TS_LRTO;
					post_ttwakeup = tttimeo(tp);
				} else if ((tp->t_cc[VMIN] == 0) &&
					(tp->t_cc[VTIME])) {
				        /* we received a char, remove 	*/
				        /* pending timeout 		*/
				 	if (tp->t_state & TS_LTACT)
					    untimeout(tty_timeout, tp);
					tp->t_state &= ~TS_LTACT;
				    	if (tp->t_smp&S_POST_WAKEUP) {
						post_ttwakeup = DO_TTWAKEUP;
				    	} else {
						ttwakeup(tp);
					}
				}	
			}
			ttyecho(c, tp);		/* echo char to terminal */
		}
		goto endcase;
	}
 
	/*
	 * Ignore any high bit added during
	 * previous ttyinput processing.
	 */
	/*
	 * Presently, if the QUOTE_FLAG is set, it signifies that the character
	 * is to be taken literally, avoiding most canonical input processing.
	 */
	if ((tp->t_state&TS_TYPEN) == 0)
		c &= CHAR_MASK;			
	/*
	 * Check for literal nexting very first
	 */
	if (tp->t_state&TS_LNCH) {
		c |= QUOTE_FLAG;			
		tp->t_state &= ~TS_LNCH;	/* clears literal next flag */
	}
 
	/*
	 * Scan for special characters.  This code
	 * is really just a big case statement with
	 * non-constant cases.  The bottom of the
	 * case statement is labeled ``endcase'', so goto
	 * it after a case match, or similar.
	 */
	if ((tp->t_line==NTTYDISC) || ((tp->t_line==TERMIODISC) &&
	    spec_char && (tp->t_lflag_ext & PIEXTEN))) {
		if (c == tp->t_cc[VLNEXT]) {
			/*
			 * If literal next char (^V is the default)
			 *	if echoing put out ^\b as literal next char
			 */
			if (tp->t_lflag&ECHO)
				ttyout("^\b", tp);
			tp->t_state |= TS_LNCH;	/* set literal next flag */
			goto endcase;
		}
		if (c == tp->t_cc[VFLUSH]) {
			/*
			 * if flush char (^O is the default)
			 *	if flag already set, clear it
			 *	else flush the write buffer and echo flush char
			 */
			if (tp->t_oflag_ext & PFLUSHO)
				tp->t_oflag_ext &= ~PFLUSHO;
			else {
				ttyflush(tp, FWRITE);
				ttyecho(c, tp);
				if (tp->t_rawq.c_cc + tp->t_canq.c_cc)
				/*
				 * if the raw and canonical queues contain
				 * anything then reprint the contents of the raw
				 * queue
				 */
					ttyretype(tp);
				tp->t_oflag_ext |= PFLUSHO;  /* set for flush */
			}
			goto startoutput;
		}
	}
 
	/*
	 * Cbreak mode, don't process line editing
	 * characters; check high water mark for wakeup.
	 */
	if (tp->t_iflag_ext & PCBREAK) {
		if (tp->t_rawq.c_cc > TTYHOG) {
			if (tp->t_outq.c_cc < TTHIWAT(tp) &&
			    tp->t_line == NTTYDISC)
				/*
				 * sound the terminal bell
				 */
				(void) ttyoutput(CTRL('g'), tp);
		} else if (putc(c, &tp->t_rawq) >= 0) {
		        if (tp->t_smp&S_POST_WAKEUP) {
			    post_ttwakeup = DO_TTWAKEUP;
			} else {
			    ttwakeup(tp);
			}
			ttyecho(c, tp);
		}
		goto endcase;
	}
 
	/*
	 * From here on down cooked mode character
	 * processing takes place.  Canonical input processing.
	 *	We're COOKING now.
	 */
	if (tp->t_lflag & ICANON){
	    	if ((c == tp->t_cc[VERASE] || c == tp->t_cc[VKILL]) && 
		    spec_char) {
			/*
		 	 * if last char \ & char is either erase or kill
		 	 *	remove char from rawq and set quote flag.
		 	 */
			if (tp->t_state&TS_QUOT) {
				ttyrub(unputc(&tp->t_rawq), tp);
				c |= QUOTE_FLAG;
			}
 
			/*
		 	 * if erase char remove it from the rawq (if its there)
		 	 */
			if (c == tp->t_cc[VERASE]) {
				if (tp->t_rawq.c_cc)
					ttyrub(unputc(&tp->t_rawq), tp);
				goto endcase;
			}
			else if (c == tp->t_cc[VKILL]) {
				if (tp->t_lflag_ext & PCRTKIL &&
			    	tp->t_rawq.c_cc == tp->t_rocount) {
					while (tp->t_rawq.c_cc)
						ttyrub(unputc(&tp->t_rawq), tp);
					if (tp->t_lflag&ECHOK)
						ttyecho('\n',tp);
				} else {
					ttyecho(c, tp);		/* echo char */
					ttyecho('\n', tp);	/* echo nl */
					while (getc(&tp->t_rawq) > 0)
						;
					tp->t_rocount = 0;	/* reset row */
				}
				/*
			 	 * reset state clearing backspace, quotes, erase
			 	 * literal next,retype suspend, and counting tab				 * width.
			 	 */
				tp->t_state &= ~TS_LOCAL;
				goto endcase;
			}
	    	}
	
		/*
		 * New line discipline,
		 * check word erase/reprint line.
		 */
		if ((tp->t_line == NTTYDISC) ||
		    ((tp->t_line==TERMIODISC) && spec_char && (tp->t_lflag_ext & PIEXTEN))) {
			if (c == tp->t_cc[VWERASE]) {	/* if word erase char */
				if (tp->t_rawq.c_cc == 0)	/* if rawq empty */
					goto endcase;		/* then done */
				/*
				 * remove char from queue til space or tab hit
				 */
				do {
					c = unputc(&tp->t_rawq);
					if (c != ' ' && c != '\t')
						goto erasenb;
					ttyrub(c, tp);
				} while (tp->t_rawq.c_cc);
				goto endcase;
		erasenb:
				do {
					ttyrub(c, tp);
					if (tp->t_rawq.c_cc == 0)
						goto endcase;
					c = unputc(&tp->t_rawq);
				} while (c != ' ' && c != '\t' && c != '/');
				(void) putc(c, &tp->t_rawq);
				goto endcase;
			}
			if (c == tp->t_cc[VRPRNT]) {
				ttyretype(tp);
				goto endcase;
			}
		}
	
		/*
		 * Check for input buffer overflow
		 * if the rawq and canonicalq exceed 255 chars
		 *	if new line discipline then sound the bell
		 * then done
		 */
		if ((tp->t_rawq.c_cc > TTYHOG) || (tp->t_canq.c_cc > TTYHOG)) {
			if (tp->t_line == NTTYDISC)
				(void) ttyoutput(CTRL('g'), tp);
			goto endcase;
		}
	
		/*
		 * Put data char in q for user and
		 * wakeup on seeing a line delimiter.
		 */
		if (putc(c, &tp->t_rawq) >= 0) {
			if (TTBREAKC(c, tp)) {
				tp->t_rocount = 0;
				/*
				 * place contents of rawq at end of canonicalq
				 */
				catq(&tp->t_rawq, &tp->t_canq);
				if (tp->t_smp&S_POST_WAKEUP) {
				    post_ttwakeup = DO_TTWAKEUP;
				} else {
				    ttwakeup(tp);
				}
			} else if (tp->t_rocount++ == 0)
				tp->t_rocol = tp->t_col;
			/* Termio only echoes eofc if it is escaped */
			if (((c == tp->t_cc[VEOF]) && (tp->t_line == TERMIODISC)) &&
			   spec_char) {
				if (tp->t_state & TS_QUOT)
					ttyrub(unputc(&tp->t_rawq),tp);
				else
					goto endcase;
			}
			tp->t_state &= ~TS_QUOT;
			if ((c==tp->t_cc[VQUOTE]) && ((tp->t_line==NTTYDISC) ||
			     ((tp->t_line==TERMIODISC) && spec_char &&
	     		      (tp->t_lflag_ext & PIEXTEN)))) 
				tp->t_state |= TS_QUOT;
			if (tp->t_state&TS_ERASE) {
				tp->t_state &= ~TS_ERASE;
				(void) ttyoutput('/', tp);
			}
			i = tp->t_col;
			ttyecho(c, tp);			/* echo char */
			if (c == tp->t_cc[VEOF] && tp->t_lflag&ECHO && 
			   spec_char) {
				i = MIN(2, tp->t_col - i);
				while (i > 0) {
					(void) ttyoutput('\b', tp);
					i--;
				}
			}
		}
	}
 
endcase:
	if (tp->t_state&TS_TTSTOP) {
		if (tp->t_smp == 0) splx(s);
		return(post_ttwakeup);
	}
 
restartoutput:
	tp->t_state &= ~TS_TTSTOP;
	tp->t_oflag_ext &= ~PFLUSHO;
 
startoutput:
	poststart_arg = ttstart(tp);
	if (poststart_arg && (tp->t_smp & S_POST_START)) {
	    tp->t_smp &= ~S_DO_UNLOCK;
	    (*tp->t_poststart)(tp, poststart_arg);
	}
	if (tp->t_smp == 0) splx(s);
	return(post_ttwakeup);
}
 
 
/*
 * Put character on TTY output queue, adding delays,
 * expanding tabs, and handling the CR/NL bit.
 * This is called both from the top half for output,
 * and from interrupt level for echoing.
 * The arguments are the character and the tty structure.
 * Returns < 0 if putc succeeds, otherwise returns char to resend
 * Must be recursive.
 */
ttyoutput(c, tp)
	register int c;
	register struct tty *tp;
{
	register char *colp;
	register int ctype;
	register u_short oflag = tp->t_oflag;
	register int fill = 0;	/* Number of fill chars for delay */
 
        TTY_ASSERT(tp);

	/* These modes avoid processing of output characters
	 * OPOST is mapped to non-raw in mapping routines.
	 */
	if (((tp->t_lflag_ext & PRAW) || (tp->t_oflag_ext & PLITOUT)) || 
	    ((oflag & OPOST) == 0)) {
		if (tp->t_oflag_ext & PFLUSHO) 
			return (-1);
		if (putc(c, &tp->t_outq))
			return (c);
		CURRENT_CPUDATA->cpu_ttyout++;	/* For stats only */
		return (-1);
	}
 
	c &= CHAR_MASK;
	/* Presently on output if the DELAY_FLAG bit is set it means that this
	 * character is to be used as a delay character and the remaining
	 * bits determine the delay length.
	 */
 
	/*
	 * Ignore EOT in normal mode to avoid hanging up certain terminals.
	 * CEOT = ctrl(d).  This hardcoded value could differ from t_eofc!
	 * This should probably be removed.
	 */
	if((tp->t_line != TERMIODISC) && 
	    (c == CEOT && (tp->t_iflag_ext&PCBREAK) == 0)) 
		return (-1);
	/*
	 * Turn tabs to spaces as required
	 */
	if (c == '\t' && (tp->t_oflag&TABDLY) == TAB3) {	/* XTABS */
 
		c = 8 - (tp->t_col&7);
		if ((tp->t_oflag_ext & PFLUSHO) == 0) {
			/*
			 * expand tab to spaces
			 */
			c -= b_to_q("        ", c, &tp->t_outq);
			CURRENT_CPUDATA->cpu_ttyout += c;	/* Statistics */
		}
		tp->t_col += c;
		return (c ? -1 : '\t');
	}
	CURRENT_CPUDATA->cpu_ttyout++;	/* increment char count for stats */
	/*
	 * for upper-case-only terminals,
	 * generate escapes.
	 */
	if ((tp->t_lflag&(XCASE|ICANON)) == (XCASE|ICANON)) {
		colp = "({)}!|^~'`";
		while (*colp++)
			if (c == *colp++) {
				if (ttyoutput('\\', tp) >= 0)
					return (c);
				c = colp[-2];
				break;
			}
		if ('A' <= c && c <= 'Z') {
			if (ttyoutput('\\', tp) >= 0)
				return (c);
		}
	}
	if (oflag&OLCUC  && ('a' <= c && c <= 'z') )
		
		c += 'A' - 'a';		/* make upper case */
 
	colp = &tp->t_col;
 
	/*
	 * These next two map from <cr> to <nl> and vice-versa.  I think
	 * that in the case of ONLCR and OCRNL both being set, there 
	 * could be a possible CYCLE developing if I recursively call
	 * the ttyoutput routine as was previously done.  To solve this
	 * problem I am using a putc followed by goto.
	 */
	if (c == '\n'){
		/*
		 * If ONLRET, still output the newline character, but adjust
		 * the column pointer to 0 and use delays as if it was a cr.
		 */
		if (oflag&ONLRET){	
			if ((tp->t_oflag_ext&PFLUSHO)==0 && putc(c,&tp->t_outq))
				return (c);
			c = 0;	/* Prevent outputting a \n in the delay code */
			goto cr;
		}
		     /* Map <nl> to <cr><lf> after checking to see that
		      * column position is not already 0 for ONOCR.
		      */
		else if (oflag&ONLCR &&
			!(oflag&ONOCR  && *colp == 0)) {
				if (ttyoutput('\r', tp) >= 0)
					return (c);
		     }
	}
	if (c == '\r') {		/* Map <cr> to <nl> */
		if(oflag&OCRNL){
			if ((tp->t_oflag_ext & PFLUSHO) == 0 && 
			    putc('\n', &tp->t_outq))
				return (c);
			c = 0;	/* Prevent outputting a \r in the delay code */
			goto nl;
		}
		/* Don't output a <cr> if already at column 0 */
		if (oflag&ONOCR && *colp == 0)
			return(-1);
	}
	if (c == '~' && tp->t_oflag_ext&PTILDE)
		c = '`';
	if ((tp->t_oflag_ext & PFLUSHO) == 0 && putc(c, &tp->t_outq))
		return (c);
	/*
	 * Calculate delays.
	 * The numbers here represent clock ticks
	 * and are not necessarily optimal for all terminals.
	 * The delays are indicated by characters with DELAY_FLAG set.
	 * In raw mode there are no delays and the
	 * transmission path is 8 bits wide.
	 *
	 * SHOULD JUST ALLOW USER TO SPECIFY DELAYS
	 */
 
	ctype = partab[c];		/* ctype is the char's parity & class */
	c = 0;
	fill = 0;
	switch (ctype&077) {		/* looking at the low 6 bits */
 
	case ORDINARY:
		(*colp)++;
 
	case CONTROL:
		break;
	/*
	 * This macro is close enough to the correct thing;
	 * it should be replaced by real user settable delays
	 * in any event...
	 */
#define	mstohz(ms)	(((ms) * hz) >> 10)
 
	case BACKSPACE:
		if (oflag&BSDLY){
			c = mstohz(100);
			fill = 2;
		}
		if (*colp)
			(*colp)--;
		break;
 
	case NEWLINE:
nl:
		ctype = ((oflag&NL1) >> 7) | ((tp->t_oflag_ext&PNL2) >> 3);
		if (ctype == 1) { /* tty 37 */
		/*
		 * NL1
		 */
			fill = 5;
			if (*colp > 0)
				c = max((((unsigned)*colp) >> 4) + 3,
				    (unsigned)6);
		} else if (ctype == 2){ /* vt05 */
			/*
			 * Termio NL1 results in this delay. Berkeley NL2
			 */
			fill = 5;
			c = mstohz(100);
		}
		/*
		 * NL3 is unimplemented and should delay 0.
		 * else if (ctype == 3){}
		 */
		/*
		 * In ULTRIX, the newline will cause LF-CR.  SYSV will not set
		 * the column pointer to 0 on newline.
		 */
		if (tp->t_line != TERMIODISC)
			*colp = 0;
		break;
 
	case TAB:
		ctype = (oflag & TABDLY) >> 10;	/* calculate tab type */
		if (ctype == 1) { /* tty 37 */
			/*
			 * TAB1
			 */
			c = 1 - (*colp | ~07);
			if (c < 5){
				c = 0;		
			}
			else
				fill = c;
		}
			/*
			 * TAB2
			 */
		else if (ctype ==2){
			c = mstohz(166);
			fill = 2;
		}
			/*
			 * Note XTABS (TAB3) is handled earlier in this routine 
			 */
		*colp |= 07;
		(*colp)++;
		break;
 
	case VTAB:
	case FORM_FEED:
		if (oflag&(VTDLY|FFDLY))	 /* tty 37 */
			c = DEL_CHAR;		/* char is DEL */
		break;
 
	case RETURN:
cr:
		ctype = (oflag & CRDLY) >> 12;	/* calculate cr type */
		if (ctype == 2){ 
			/*
			 * Termio CR2 = Berkeley CR1
			 */
			c = mstohz(83);
			fill = 2;
		}
		else if (ctype == 3){
			/*
			 * Termio CR3 = Berkeley CR2
			 */
			c = mstohz(166);
			fill = 4;
		}
		else if (ctype == 1) { 
			/*
			 * Termio CR1 = Berkeley CR3
			 */
			int i;
			int fillc;	/* Fill character used for delay */
			if ((tp->t_line != TERMIODISC) || (oflag & OFDEL))
				fillc = DEL_CHAR;	/* DEL is fill char */
			else
				fillc = 0;	/* NULL is fill char */
 
			if ((i = *colp) >= 0) {
				CURRENT_CPUDATA->cpu_ttyout+=(9-i); /* stats */
				for (; i < 9; i++) 
					(void) putc(fillc, &tp->t_outq);
			}
		}
		*colp = 0;
	}
	if ((c || fill) && ((tp->t_oflag_ext&PFLUSHO) == 0)){
		/* Fill characters transmitted for a delay instead of 
		 * a timed delay.
		 */
		if (oflag & OFILL) {
			if (oflag & OFDEL)
				c = DEL_CHAR;	/* DEL is fill character */
			else
				c = 0;		/* NULL is fill character */
			/* Output the specified number of fill characters */
			if (fill > 3)
				fill = 2;
			else
				fill = 1;
			CURRENT_CPUDATA->cpu_ttyout += fill; /* stats */
			while (fill--)
				(void) putc(c, &tp->t_outq);	
		}
		else
			/* Setting delay bit to indicate that this is a delay
			 * character to be output.
			 */
			(void) putc(c|DELAY_FLAG, &tp->t_outq);	
	}
	return (-1);
}
#undef mstohz
 
/*
 * Called from device's read routine after it has
 * calculated the tty-structure given as argument.
 */
ttread(tp, uio)
	register struct tty *tp;
	struct uio *uio;
{
	register struct clist *qp;
	register int c;
	register u_short t_lflag;		/* termio line mode flag */
	int s, first, error = 0, poststart_arg = NO_POST_START;
 

	TTY_LOCK(tp,s);
	t_lflag = tp->t_lflag;
	while ((tp->t_state&(TS_CARR_ON|TS_IGNCAR))==0)
		if (tp->t_state&TS_ONDELAY) { /* open O_NDELAY */
			/*
			 * POSIX does non-blocking reads on a file descriptor
			 * basis, while others do it on a per-line basis.
			 */
			if (u.u_procp->p_progenv == A_POSIX) {
				if (uio->uio_flag & FNBLOCK) {
					TTY_UNLOCK(tp,s);
					return(EAGAIN);
				}
			}
			else if (tp->t_state&TS_NBIO) {
				TTY_UNLOCK(tp,s);
				return(EWOULDBLOCK);
			}
			/* wake up on carrier transition */
			TTY_SLEEP_RELOCK(tp,(caddr_t)&tp->t_rawq, TTIPRI);
		}
		else {
			TTY_UNLOCK(tp,s);
			/* System V and POSIX return 0 chars when
			/* connection is not established */
			if (tp->t_line == TERMIODISC) {
				return(0);
			} else {
				return(EIO);
			}
		}
	TTY_UNLOCK(tp,s);
loop:
	/*
	 * Take any pending input first.
	 */
	TTY_LOCK(tp,s);
	if (tp->t_iflag_ext & PPENDIN)
		ttypend(tp);
	TTY_UNLOCK(tp,s);
 
	/*
	 * Hang the process if it's in the background.
	 */
	while (tp == u.u_procp->p_ttyp && u.u_procp->p_pgrp != tp->t_pgrp) {
		/*
		 * For POSIX programs, return EIO on orphaned processes.
		 * At this time "orphaned" means that the parent is init.
		 */
		if ((u.u_procp->p_sigignore & sigmask(SIGTTIN)) ||
		   (u.u_procp->p_sigmask & sigmask(SIGTTIN)) ||
		   (u.u_procp->p_vm&SVFORK) ||
		   ((u.u_procp->p_pptr == &proc[1]) && 
		    (u.u_procp->p_progenv == A_POSIX))) {
			return (EIO);
		}
		gsignal(u.u_procp->p_pgrp, SIGTTIN);
		sleep((caddr_t)&lbolt, TTIPRI);
	}
 
	/*
	 * In raw mode take characters directly from the
	 * raw queue w/o processing.  Interlock against
	 * device interrupts when interrogating rawq.
	 */
	if (((t_lflag & ICANON) == 0) && ((tp->t_iflag_ext&PCBREAK) == 0)) {
		TTY_LOCK(tp,s);
		if (tp->t_rawq.c_cc <= 0) {

			if ((tp->t_state&(TS_CARR_ON|TS_IGNCAR))==0) {
				TTY_UNLOCK(tp,s);
				return(EIO);
			}
			/*
			 * POSIX does non-blocking reads on a file descriptor
			 * basis, while others do it on a per-line basis.
			 */
			if (u.u_procp->p_progenv == A_POSIX) {	
				if (uio->uio_flag & FNBLOCK) {
					TTY_UNLOCK(tp,s);
					return(EAGAIN);
				}
			}
			else if (tp->t_state&TS_NBIO) {
				TTY_UNLOCK(tp,s);
				return (EWOULDBLOCK);
			}
			/*
			 * If c_cc[VTIME] is set, return after timeout.
			 * c_cc[VMIN] = 0 infers intercharacter timing is done.
			 */
			if (tp->t_cc[VMIN] == 0){
				if (tp->t_cc[VTIME] == 0) /* return immediate */
					goto retchars;
				/*
				 * timeout as specified via c_cc[VTIME].
				 */
				tp->t_state &= ~TS_LRTO;
				if (!(tp->t_state & TS_LTACT))
					tttimeo(tp);
			}
 
			TTY_SLEEP(tp,(caddr_t)&tp->t_rawq, TTIPRI);
			splx(s);
			/*
			 * Don't sleep again for more if wakeup was due to 
			 * c_cc[VTIME] expiration.
			 */
			if (!(tp->t_state&TS_LRTO)) 
				goto loop;
			TTY_LOCK(tp,s);
		} else {
			/* 
			 * VMIN > 0 and VTIME > 0.
			 * The timer should be started from lower half,
			 * If there is a timer, we sleep easy and wait for
			 * the timer expired.  If there is no timer set,
			 * oops, the timer must already expired way before
			 * we read.   
			 * VMIN > 0 and VTIME = 0.
			 * No timer involved, read blocks until chars
			 * are greater than VMIN - khh.
			 */
		        if (tp->t_rawq.c_cc < tp->t_cc[VMIN]) {
			    if (tp->t_cc[VTIME]) {
			    	if (!(tp->t_state & TS_LTACT)) 
					goto retchars;
			    }
			    TTY_SLEEP(tp, (caddr_t)&tp->t_rawq, TTIPRI);
			    splx(s);
			    if (!(tp->t_state&TS_LRTO))
				goto loop;
			    TTY_LOCK(tp, s);
			}
		}
retchars:
		TTY_UNLOCK(tp,s);
 		while (!error && tp->t_rawq.c_cc && uio->uio_resid){
			TTY_LOCK(tp,s);
			c = getc(&tp->t_rawq);
			TTY_UNLOCK(tp,s);
 			error = ureadc(c, uio);
		}
		goto checktandem;
	}
 
	/*
	 * In cbreak mode use the rawq, otherwise
	 * take characters from the canonicalized q.
	 */
	qp = tp->t_iflag_ext&PCBREAK ? &tp->t_rawq : &tp->t_canq;
 
	/*
	 * No input, sleep on rawq awaiting hardware
	 * receipt and notification.
	 */
	TTY_LOCK(tp,s);
	if (qp->c_cc <= 0) {

		if ((tp->t_state&(TS_CARR_ON|TS_IGNCAR))==0) {
			TTY_UNLOCK(tp,s);
			return(EIO);
		}
		/*
		 * POSIX does non-blocking reads on a file descriptor
		 * basis, while others do it on a per-line basis.
		 */
		if (u.u_procp->p_progenv == A_POSIX) {	
			if (uio->uio_flag & FNBLOCK) {
				TTY_UNLOCK(tp,s);
				return(EAGAIN);
			}
		}
		else if (tp->t_state&TS_NBIO) {
			TTY_UNLOCK(tp,s);
			return (EWOULDBLOCK);
		}
 
		TTY_SLEEP(tp,(caddr_t)&tp->t_rawq, TTIPRI);
		splx(s);
		goto loop;
	}
 	/*
	 * Input present, perform input mapping
	 * and processing (we're not in raw mode).
	 */
	first = 1;
	while ((c = getc(qp)) >= 0) {
		TTY_UNLOCK(tp,s);
		/* Perform these mappings if in CBREAK mode or COOKED mode
		 * in which case ICANON would be set.
		 */
		if ( (t_lflag&ICANON) || (tp->t_iflag_ext&PCBREAK) ){
			if (c == '\r' && tp->t_iflag&ICRNL)
				c = '\n';
			/*
			 * Hack lower case simulation on
			 * upper case only terminals.
			 */
			/* In this case if the QUOTE_FLAG set it would indicate 
			 * that this character is to be taken literally.
 			 * we don't try and map characters with the 8th bit
			 * set. We should due true fallback here.....
			 * No mappings are presently defined for valid chars
 			 * in the range 0200-0377.
			 */
			if (t_lflag&XCASE && c <= 0177)
				if (tp->t_state&TS_BKSL) {
					/*
					 * non-zero entries in the maptable are
					 * to be proceeded by a slash
					 */
					if (maptab[c])
						c = maptab[c];
					TTY_LOCK(tp,s);
					tp->t_state &= ~TS_BKSL;
					TTY_UNLOCK(tp,s);
				} else if (c >= 'A' && c <= 'Z')
					c += 'a' - 'A';
				else if (c == '\\') {
					TTY_LOCK(tp,s);
					tp->t_state |= TS_BKSL;
					continue;
				}
		}
		/*
		 * Check for delayed suspend character.
		 */
		if (c == tp->t_cc[VDSUSP] && ((tp->t_line == NTTYDISC) || 
		    ((u.u_procp->p_progenv == A_POSIX) &&
		    (c != POSIX_VDISABLE) &&
		    (tp->t_lflag_ext & PIEXTEN)))) {
			gsignal(tp->t_pgrp, SIGTSTP);
			if (first) {
				sleep((caddr_t)&lbolt, TTIPRI);
				goto loop;
			}
			TTY_LOCK(tp,s);
			break;
		}
		/*
		 * Interpret EOF only in cooked mode.
		 * Return characters to user and discard the EOF character.
		 */
		if (c == tp->t_cc[VEOF]) {
		    if (((tp->t_iflag_ext&PCBREAK)==0) ||
		        (u.u_procp->p_progenv == A_SYSV)) {
			    if (u.u_procp->p_progenv != A_POSIX) {
				TTY_LOCK(tp,s);
			        break;
			    } else if (c != POSIX_VDISABLE) {
				TTY_LOCK(tp,s);
				break;
			    }
		    }
		}
		/*
		 * Give user character.
		 * Clear flag bits before calling ureadc.
		 */
 		error = ureadc(c & CHAR_MASK, uio);
		if (error) {
			TTY_LOCK(tp,s);
			break;
		}
 		if (uio->uio_resid == 0) {
			TTY_LOCK(tp,s);
			break;
		}
		TTY_LOCK(tp,s);
		/*
		 * In cooked mode check for a "break character"
		 * marking the end of a "line of input".
		 */
		if ((tp->t_iflag_ext&PCBREAK) == 0 && TTBREAKC(c, tp) && 
			t_lflag&ICANON) 
			break;
		first = 0;
	}
	tp->t_state &= ~TS_BKSL;
	TTY_UNLOCK(tp,s);
 
checktandem:
	/*
	 * Look to unblock output now that (presumably)
	 * the input queue has gone down.
	 */
	TTY_LOCK(tp,s);
	if (tp->t_state&TS_TBLOCK)  
		if ((tp->t_iflag_ext&PCBREAK) || ((tp->t_lflag&ICANON) == 0)){
			if (tp->t_rawq.c_cc<TTYHOG/5)
			    if (putc(tp->t_cc[VSTART], &tp->t_outq) == 0) {
			       tp->t_state &= ~TS_TBLOCK;
			       poststart_arg = ttstart(tp);
			    }
		} else {
			if (tp->t_canq.c_cc < TTYHOG/5)
			    if (putc(tp->t_cc[VSTART], &tp->t_outq) == 0) {
			       tp->t_state &= ~TS_TBLOCK;
			       poststart_arg = ttstart(tp);
			    }
		}
	if (poststart_arg && (tp->t_smp & S_POST_START)) {
	    tp->t_smp |= S_DO_UNLOCK;
	    (*tp->t_poststart)(tp, poststart_arg);
	    splx(s);
	} else {
	    TTY_UNLOCK(tp,s);
	}
	return (error);
}

tty_pty_unblock(tp)
    register struct tty *tp;
{
    wakeup(&tp->t_outq.c_cc);
}
 
/*
 * Called from the device's write routine after it has
 * calculated the tty-structure given as argument.
 */
ttwrite(tp, uio)
	register struct tty *tp;
	register struct uio *uio;
{
	register char *cp;
	register int cc, ce, c, poststart_arg = NO_POST_START;
	int i, hiwat, cnt, error, s, prev_cc;
	char obuf[OBUFSIZ];
 
	TTY_LOCK(tp,s);
	while ((tp->t_state&(TS_CARR_ON|TS_IGNCAR))==0) {
		if (tp->t_state&TS_ONDELAY) { /* open O_NDELAY */
			/*
			 * POSIX does non-blocking writes on a file descriptor
			 * basis, while others do it on a per-line basis.
			 */
			if (u.u_procp->p_progenv == A_POSIX) {
				if (uio->uio_flag & FNBLOCK) {
					TTY_UNLOCK(tp,s);
					return(EAGAIN);
				}
			}
			else if (tp->t_state&TS_NBIO) {
				TTY_UNLOCK(tp,s);
				return(EWOULDBLOCK);
			}
			/* wake up on carrier transition */
			TTY_SLEEP_RELOCK(tp,(caddr_t)&tp->t_rawq, TTIPRI);
		}
		else {
		    	if (u.u_procp->p_progenv == A_SYSV) {
			    TTY_UNLOCK(tp,s);
			    return(0);
			} else {
			    TTY_UNLOCK(tp,s);
			    return(EIO);
			}
		}
	}
	TTY_UNLOCK(tp,s);
	hiwat = TTHIWAT(tp);
	cnt = uio->uio_resid;
	error = 0;
loop:
	/*
	 * Hang the process if it's in the background and a stop has been
	 * received.
	 */
	while (u.u_procp->p_pgrp != tp->t_pgrp && tp == u.u_procp->p_ttyp &&
	    tp->t_lflag_ext&PTOSTOP && (u.u_procp->p_vm&SVFORK)==0 &&
       	    
           !(u.u_procp->p_sigignore & sigmask(SIGTTOU)) &&
            !(u.u_procp->p_sigmask & sigmask(SIGTTOU))
	    ) {
                /*
                 * For POSIX programs, return EIO on orphaned processes.
                 * At this time "orphaned" means that the parent is init.
                 */
		if ((u.u_procp->p_pptr == &proc[1]) &&
		    (u.u_procp->p_progenv == A_POSIX))
			return (EIO);
		gsignal(u.u_procp->p_pgrp, SIGTTOU);
		sleep((caddr_t)&lbolt, TTIPRI);
	}
 
	/*
	 * Process the user's data in at most OBUFSIZ
	 * chunks.  Perform lower case simulation and
	 * similar hacks.  Keep track of high water
	 * mark, sleep on overflow awaiting device aid
	 * in acquiring new space.
	 */
	while (uio->uio_resid > 0) {
		/*
		 * Grab a hunk of data from the user.
		 */
		cc = uio->uio_iov->iov_len;
		if (cc == 0) {
			uio->uio_iovcnt--;
			uio->uio_iov++;
			if (uio->uio_iovcnt < 0)
				panic("ttwrite");
			continue;
		}
		if (cc > OBUFSIZ)
			cc = OBUFSIZ;
		cp = obuf;
		error = uiomove(cp, cc, UIO_WRITE, uio);
		if (error)
			break;
		if (tp->t_outq.c_cc > hiwat)
			goto ovhiwat;
		if (tp->t_oflag_ext & PFLUSHO)
			continue;
		/*
		 * If we're mapping lower case or kludging tildes,
		 * then we've got to look at each character, so
		 * just feed the stuff to ttyoutput...
		 */
		if (tp->t_oflag_ext & PTILDE || tp->t_oflag & OLCUC ||
		     tp->t_lflag & XCASE) {
			while (cc > 0) {
				c = *cp++;
				tp->t_rocount = 0;
				TTY_LOCK(tp,s);
				while ((c = ttyoutput(c, tp)) >= 0) {
				    /* out of clists, wait a bit */
				    poststart_arg = ttstart(tp);
				    if (poststart_arg && (tp->t_smp&S_POST_START)){
					tp->t_smp &= ~S_DO_UNLOCK;
					(*tp->t_poststart)(tp, poststart_arg);
				    }
				    TTY_SLEEP_RELOCK(tp,(caddr_t)&lbolt, TTOPRI);
				    tp->t_rocount = 0;
				}
				TTY_UNLOCK(tp,s);
				--cc;
				if (tp->t_outq.c_cc > hiwat)
					goto ovhiwat;
			}
			continue;
		}
		/*
		 * If nothing fancy need be done, grab those characters we
		 * can handle without any of ttyoutput's processing and
		 * just transfer them to the output q.  For those chars
		 * which require special processing (as indicated by the
		 * bits in partab), call ttyoutput.  After processing
		 * a hunk of data, look for FLUSHO so ^O's will take effect
		 * immediately.
		 */
		if (tp->t_smp & S_PTY) {
		    TTY_LOCK(tp, s);
		    while (tp->t_outq.c_cc) {
			prev_cc = tp->t_outq.c_cc;
			poststart_arg = ttstart(tp);
			if (poststart_arg && (tp->t_smp&S_POST_START)) {
			    tp->t_smp &= ~S_DO_UNLOCK;
			    (*tp->t_poststart)(tp, poststart_arg);
			}
			timeout(tty_pty_unblock, tp, hz);
			TTY_SLEEP_RELOCK(tp, (caddr_t)&tp->t_outq.c_cc, PZERO - 1);
			/* If we cannot guarantee that the following case is
 			 * due to timeout, then we need to untimeout 
 			 * regardless whether there is progress in output. 
			 */
			untimeout(tty_pty_unblock, tp);
			if (prev_cc == tp->t_outq.c_cc) 
			    	break;
		    }
		    TTY_UNLOCK(tp, s);
		}
		while (cc > 0) {
			if (((tp->t_lflag_ext & PRAW) || (tp->t_oflag_ext&PLITOUT)) &&
			     ((tp->t_oflag & OPOST) == 0)) 
				ce = cc;
			else {
				ce = cc - scanc((unsigned)cc, (caddr_t)cp,
				   (caddr_t)partab, 077);
				/*
				 * If ce is zero, then we're processing
				 * a special character through ttyoutput.
				 */
				if (ce == 0) {
				    TTY_LOCK(tp,s);
				    tp->t_rocount = 0;
				    if (ttyoutput(*cp, tp) >= 0) {
					/* no c-lists, wait a bit */
					poststart_arg = ttstart(tp);
					if (poststart_arg && (tp->t_smp&S_POST_START)) {
					    tp->t_smp &= ~S_DO_UNLOCK;
					    (*tp->t_poststart)(tp, poststart_arg);
					}
					TTY_SLEEP(tp,(caddr_t)&lbolt, TTOPRI);
					splx(s);
					continue;
				    }
				    TTY_UNLOCK(tp,s);
				    cp++, cc--;
				    if (tp->t_oflag_ext&PFLUSHO ||
					tp->t_outq.c_cc > hiwat)
					goto ovhiwat;
				    continue;
				}
			}
			/*
			 * A bunch of normal characters have been found,
			 * transfer them en masse to the output queue and
			 * continue processing at the top of the loop.
			 * If there are any further characters in this
			 * <= OBUFSIZ chunk, the first should be a character
			 * requiring special handling by ttyoutput.
			 */
			TTY_LOCK(tp,s);
			tp->t_rocount = 0;
			i = b_to_q(cp, ce, &tp->t_outq);
			ce -= i;
			tp->t_col += ce;
			cp += ce, cc -= ce, CURRENT_CPUDATA->cpu_ttyout += ce;
			if (i > 0) {
				/* out of c-lists, wait a bit */
				poststart_arg = ttstart(tp);
				if (poststart_arg && (tp->t_smp & S_POST_START)) {
				    tp->t_smp &= ~S_DO_UNLOCK;
				    (*tp->t_poststart)(tp, poststart_arg);
				}
				TTY_SLEEP_RELOCK(tp,(caddr_t)&lbolt, TTOPRI);
			}
			TTY_UNLOCK(tp,s);
			if (tp->t_oflag_ext&PFLUSHO || tp->t_outq.c_cc > hiwat)
				goto ovhiwat;
		}
		TTY_LOCK(tp,s);
		poststart_arg = ttstart(tp);
		if (poststart_arg && (tp->t_smp&S_POST_START)) {
		    tp->t_smp |= S_DO_UNLOCK;
		    (*tp->t_poststart)(tp, poststart_arg);
		    splx(s);
		} else {
		    TTY_UNLOCK(tp,s);
		}
	}
	TTY_LOCK(tp,s);
	poststart_arg = ttstart(tp);
	if (poststart_arg && (tp->t_smp&S_POST_START)) {
	    tp->t_smp |= S_DO_UNLOCK;
	    (*tp->t_poststart)(tp, poststart_arg);
	    splx(s);
	} else {
	    TTY_UNLOCK(tp,s);
	}
	return (error);
 
ovhiwat:
	TTY_LOCK(tp,s);
	if (cc != 0) {
		uio->uio_iov->iov_base -= cc;
		uio->uio_iov->iov_len += cc;
		uio->uio_resid += cc;
		uio->uio_offset -= cc;
	}
	/*
	 * This can only occur if PFLUSHO
	 * is also set in t_oflag.
	 * It can also happen if the device routine called by
	 * ttstart (oproc) removes characters from t_outq.
	 */
	poststart_arg = ttstart(tp);
	if (tp->t_outq.c_cc <= hiwat) {
	    if (poststart_arg && (tp->t_smp&S_POST_START)) {
		tp->t_smp |= S_DO_UNLOCK;
		(*tp->t_poststart)(tp, poststart_arg);
		splx(s);
	    } else {
		TTY_UNLOCK(tp,s);
	    }
	    goto loop;
	}
	/*
	 * POSIX does non-blocking writes on a file descriptor
	 * basis, while others do it on a per-line basis.
	 */
	if (u.u_procp->p_progenv == A_POSIX) {
		if (uio->uio_flag & FNBLOCK) {
			if (uio->uio_resid == cnt) {
			    /* Block if no progress has been made. */
			    error = EAGAIN;
			} else {
			    /* Some progress has been made on the output */
			    error = 0;
			}
			goto writeend;
		}
	}
	else if (tp->t_state&TS_NBIO) {
		if (uio->uio_resid == cnt) {
		    error = EWOULDBLOCK;
		} else {
		    error = 0;
		}
		goto writeend;
	}
	tp->t_state |= TS_ASLEEP;
	if (poststart_arg && (tp->t_smp&S_POST_START)) {
	    tp->t_smp &= ~S_DO_UNLOCK;
	    (*tp->t_poststart)(tp, poststart_arg);
	}
	TTY_SLEEP(tp,(caddr_t)&tp->t_outq, TTOPRI);
	splx(s);
	goto loop;

writeend:
	if (poststart_arg && (tp->t_smp&S_POST_START)) {
	    tp->t_smp |= S_DO_UNLOCK;
	    (*tp->t_poststart)(tp, poststart_arg);
	    splx(s);
	} else {
	    TTY_UNLOCK(tp,s);
	}
	return(error);
}
 
/*
 * Rubout one character from the rawq of tp
 * as cleanly as possible.
 */
ttyrub(c, tp)
	register int c;
	register struct tty *tp;
{
	register char *cp;
	register int savecol;
	int gotchar;
	char *getnextc();
        TTY_ASSERT(tp);
 
	/*
	 * If ECHO is not set we still have to echo the erase character
	 * as SP-BS.
	 */
	if ((tp->t_lflag&(ECHO|ECHOE)) == 0)
		return;
	tp->t_oflag_ext &= ~PFLUSHO;
	c &= CHAR_MASK;
	if (tp->t_lflag_ext & PCRTBS) {
		if (tp->t_rocount == 0) {
			/*
			 * Mangled by ttwrite; retype
			 */
			ttyretype(tp);
			return;
		}
		if (c == ('\t'|QUOTE_FLAG) || c == ('\n'|QUOTE_FLAG))
			ttyrubo(tp, 2);
		else switch (partab[c &= CHAR_MASK] & 0177) {
 
		case ORDINARY:
			if ( tp->t_lflag&XCASE &&c >= 'A' && c <= 'Z') 
				/*
				 * Delete two since an upper case letter will
				 * be preceded by a "\".
				 */
				ttyrubo(tp, 2);
			else
				ttyrubo(tp, 1);
			break;
 
		case VTAB:
		case FORM_FEED:
		case BACKSPACE:
		case CONTROL:
		case NEWLINE:
		case RETURN:
			if (tp->t_lflag_ext&PCTLECH)
				ttyrubo(tp, 2);
			break;
 
		case TAB:
			if (tp->t_rocount < tp->t_rawq.c_cc) {
				ttyretype(tp);
				return;
			}
			savecol = tp->t_col;
			tp->t_state |= TS_CNTTB;
			tp->t_oflag_ext |= PFLUSHO;
			tp->t_col = tp->t_rocol;
 
			cp = tp->t_rawq.c_cf - 1;	/* predec for getnext */
			while (cp = getnextc(&tp->t_rawq, cp, &gotchar))
				ttyecho(gotchar, tp);
 
			tp->t_oflag_ext &= ~PFLUSHO;
			tp->t_state &= ~TS_CNTTB;
			/*
			 * savecol will now be length of the tab
			 */
			savecol -= tp->t_col;
			tp->t_col += savecol;
			if (savecol > 8)
				savecol = 8;	    /* overflow compensation */
			while (--savecol >= 0)
				(void) ttyoutput('\b', tp);
			break;
 
		default:
			panic("ttyrub");
		}
	} else if (tp->t_lflag_ext&PPRTERA) {
		if ((tp->t_state&TS_ERASE) == 0) {
			(void) ttyoutput('\\', tp);
			tp->t_state |= TS_ERASE;
		}
		ttyecho(c, tp);
	} else
		ttyecho(tp->t_cc[VERASE], tp);
	tp->t_rocount--;
}
 
/*
 * Crt back over cnt chars perhaps
 * erasing them.
 */
ttyrubo(tp, cnt)
	register struct tty *tp;
	register int cnt;
{
	register char *rubostring ;
 
	/*	TERMIO	(ECHOE)
	 * ECHOE is the roughly the same as CRTERA with the exception
	 * that caninical processing must be enabled for ECHOE.  Note that
	 * to get to this routine CRTBS must have been set so if ECHOE is set
	 * then CRTBS and CRTERA must be set also. Similarly if CRTBS&CRTERA
	 * then ECHOE should be set.
	 */
        TTY_ASSERT(tp);
	if ((tp->t_lflag&(ECHOE|ICANON|ECHO)) == (ECHOE|ICANON))
		rubostring = " \b";
	else if (tp->t_lflag_ext & PCRTERA || 
		((tp->t_lflag&(ICANON|ECHOE|ECHO)) == (ICANON|ECHOE|ECHO)))
		rubostring = "\b \b";
	else
		rubostring = "\b";
 
	while (--cnt >= 0)
		ttyout(rubostring, tp);
}
 
/*
 * Reprint the rawq line.
 * We assume c_cc has already been checked.
 */
ttyretype(tp)
	register struct tty *tp;
{
	register char *cp;
	char *getnextc();
	int gotchar;
 
        TTY_ASSERT(tp);
	if (tp->t_cc[VRPRNT] != 0377)
		ttyecho(tp->t_cc[VRPRNT], tp);
	(void) ttyoutput('\n', tp);
 
	cp = tp->t_canq.c_cf - 1; 	/* predecrement for getnextc */
	while (cp = getnextc(&tp->t_canq, cp, &gotchar))
		ttyecho(gotchar, tp);
 
	cp = tp->t_rawq.c_cf - 1; 	/* predecrement for getnextc */
	while (cp = getnextc(&tp->t_rawq, cp, &gotchar))
		ttyecho(gotchar, tp);
 
	tp->t_state &= ~TS_ERASE;
	tp->t_rocount = tp->t_rawq.c_cc;
	tp->t_rocol = 0;
}
 
/*
 * Echo a typed character to the terminal
 */
ttyecho(c, tp)
	register int c;
	register struct tty *tp;
{
 
        TTY_ASSERT(tp);
	if ((tp->t_state&TS_CNTTB) == 0)
		tp->t_oflag_ext &= ~PFLUSHO;
	c &= CHAR_MASK;
	/*
	 * If ECHO is not set, do not echo characters, unless ECHONL is set 
	 * which means echo the newline character even if echo is not set.
	 */
	if ((tp->t_lflag&ECHO) == 0)
		if ((c != '\n') || ((tp->t_lflag&(ICANON|ECHONL)) != (ICANON|ECHONL))) 
			return;
	if ((tp->t_lflag_ext&PRAW) || ((tp->t_oflag&OPOST) == 0)) {
		(void) ttyoutput(c, tp);
		return;
	}
	if (c == '\r' && tp->t_oflag&ONLCR)
		c = '\n';
	if (tp->t_lflag_ext&PCTLECH) {
		if ((c&CHAR_MASK) <= 037 && c!='\t' && c!='\n' || (c&CHAR_MASK)==DEL_CHAR) {
			(void) ttyoutput('^', tp);
			c &= CHAR_MASK;
			if (c == DEL_CHAR)
				c = '?';
			else if (tp->t_oflag&OLCUC)
				c += 'a' - 1;
			else
				c += 'A' - 1;
		}
	}
	/*
	 * Map upper case to lower before calling ttyoutput.  I believe that
	 * this is done to prevent a "\" from preceding each upper case letter.
	 */
	if ((c >= 'A' && c <= 'Z') && (tp->t_oflag&OLCUC)) 
		c += 'a' - 'A';
	/*
	 * Strip character before sending to output routine.
	 */
	(void) ttyoutput(c&CHAR_MASK, tp);
}

/*
 * send string cp to tp
 */
ttyout(cp, tp)
	register char *cp;
	register struct tty *tp;
{
	register char c;
 
        TTY_ASSERT(tp);
	while (c = *cp++)
		(void) ttyoutput(c, tp);
}
 
/*
 * Alert the top half that it's request for input can now be satisfied.
 * This doesn't necessarity mean that characters are present.
 */
ttwakeup(tp)
	register struct tty *tp;
{
	struct proc *p = 0;
	int flag, async, pgrp;
	/*
	 * Wakeups are done here while tty lock is held.  This is unfortunate
	 * but necessary at this time due to the way this routine is called
	 * with the lock already held and it would be quite convoluted to 
	 * pass back the necessary information to defer wakeups.
	 */
        TTY_ASSERT(tp);
	if (tp->t_rsel) {
		p = tp->t_rsel;
		flag = tp->t_state&TS_RCOLL;
		tp->t_state &= ~TS_RCOLL;
		tp->t_rsel = 0;
	}
	if (tp->t_state & TS_ASYNC) {
		async = 1;
		pgrp = tp->t_pgrp;
	}
	if (tp->t_smp&S_POST_WAKEUP) {
	    smp_unlock(&tp->t_lk_tty);
	}
	if (p) {
		selwakeup(p, flag);
	}
	if (async) {
		gsignal(pgrp, SIGIO); 
	}
	wakeup((caddr_t)&tp->t_rawq);
}
 
 
#if !defined(vax) && !defined(mips)
scanc(size, cp, table, mask)
	register int size;
	register char *cp, table[];
	register int mask;
{
	register int i = 0;
 
	while ((table[*(u_char *)(cp + i)]&mask) == 0 && i < size)
		i++;
	return (size - i);
}
#endif !defined(vax) && !defined(mips)
 
/* ULTRIX to termio mapping.
 * When setting parameters in the sgtty structure via TIOCSETP
 * it is also necessary map these changes where possible into
 * the termio terminal structure.  The following shows this
 * mapping on a flag-by-flag basis.
 */
ttmapU_V(tp)
	register struct tty *tp;
{
	register struct ttdelaystruct *te;
	register unsigned int tpt_flags;

#ifdef TTYDEBUG
	if (ttydebug > 4)
		mprintf("ttmapU_V: tp = %x\n",tp);
#endif TTYDEBUG

        TTY_ASSERT(tp);
	if (tp->t_line != TERMIODISC) {
		/*
		 * The following flags have no map between ULTRIX and termio
	 	 * but it is necessary to set them to provide a backward 
	 	 * compatible ULTRIX environment.
		 */
		tp->t_iflag |= (BRKINT|IGNPAR|ISTRIP|INPCK|IXON);
		tp->t_lflag |= ISIG;
		tp->t_cflag |= (CREAD|PARENB);
	}
	/*
	 * Preserve those termio flags that do not have a mapping
	 * to the former terminal state representation.  This is done to 
	 * prevent the loss of termio specific settings.
	 * Note: If any of these change, it must also change in the drivers
	 * close routine!
	 */
	tp->t_iflag &= (IGNBRK|IGNPAR|PARMRK|INLCR|IGNCR|BRKINT|INPCK|ISTRIP|IXON);	
	tp->t_iflag_ext = 0; 
	tp->t_oflag &= (ONOCR|ONLRET|OFILL|OFDEL|OCRNL);
	tp->t_oflag_ext = 0; 
	tp->t_cflag &= (CREAD|PARENB|CLOCAL|HUPCL|CBAUD);/* keep input speed */
	tp->t_cflag_ext &= (CBAUD);			/* keep output speed */
	tp->t_lflag &= (ISIG|ECHOK|ECHONL);
	tp->t_lflag_ext &= PIEXTEN;

	/*
	 *	INPUT MODES
	 */
		if (tp->t_flags & CRMOD) {
			tp->t_iflag |= ICRNL; 
			tp->t_oflag |= ONLCR;
			tp->t_oflag &= ~ONOCR;
		}
		
		/*
		 * If ANYP or (ANYP == 0) set cflag to the parity setting that
		 * gives propper backward compatibility with Berkeley.
		 */
		if ((tp->t_flags&ANYP) == ANYP) {		/* ANYP */
			tp->t_cflag &= ~PARODD;
			tp->t_iflag &= ~(INPCK | IGNPAR);
		}
		else if ((tp->t_flags&ANYP) == 0) {		/* NO-parity */
			tp->t_cflag |= PARODD;
			tp->t_iflag &= ~(INPCK | IGNPAR);
		}
		else if ((tp->t_flags&EVENP) == EVENP) {	/* EVENP */
			tp->t_cflag &= ~PARODD;
			tp->t_iflag |= (INPCK | IGNPAR);
		}
		else { 						/* ODDP */
			tp->t_cflag |= PARODD;
			tp->t_iflag |= (INPCK | IGNPAR);
		}
		if ((tp->t_flags & DECCTQ) == 0)
			tp->t_iflag |= IXANY;
		if (tp->t_flags & TANDEM)
			tp->t_iflag |= IXOFF;

		/* Note ICRNL and IUCLC in local modes, IUCLC in output modes */ 

		if (tp->t_flags & PENDIN)
			tp->t_iflag_ext |= PPENDIN;
		if (tp->t_flags & CBREAK)
			tp->t_iflag_ext |= PCBREAK;

	/*
	 *	OUTPUT MODES
	 */
		if ((tp->t_flags & (RAW|LITOUT)) == 0)
			tp->t_oflag |= OPOST;
		if (tp->t_flags & LCASE) {
			tp->t_oflag |= OLCUC;
			tp->t_lflag |= XCASE;
			tp->t_iflag |= IUCLC;
		}
		if (tp->t_flags & TILDE)
			tp->t_oflag_ext |= PTILDE;
		if (tp->t_flags & FLUSHO)
			tp->t_oflag_ext |= PFLUSHO;
		/* ONLCR is mapped above with input modes */

		/* Convert ULTRIX style delays to termio style via the
		 * ttdelaystruct mapping table.
		 */
		tpt_flags = tp->t_flags;
		for(te = ttdelaystruct; te->vmode; te++)
			if ((tpt_flags & te->umask) == te->umode){
				tp->t_oflag |= te->vmode;
			}
		if (tpt_flags & NL1)
			tp->t_oflag_ext |= PNL2;
	/*
	 *	LOCAL MODES
	 */
		if (tp->t_flags & RAW) {
			tp->t_cc[VTIME] = 0; 	/* RAW reads return immediate */
			tp->t_cc[VMIN] = 1;
			tp->t_cflag |= CS8;
			tp->t_cflag &= ~PARENB;
			tp->t_lflag_ext |= PRAW;
			/*
			 * Take these out because they are above RAW in ttyinput
			 */
			tp->t_iflag &= ~(ICRNL|IUCLC|INLCR|IGNCR|IXON|ISTRIP);
			tp->t_lflag &= ~(ISIG);
		}
		else if (tp->t_flags & LITOUT) {
			tp->t_cflag |= CS8;
			tp->t_cflag &= ~PARENB;
			tp->t_oflag_ext |= PLITOUT;
		}
		else {
			if (tp->t_flags & PASS8) {
				/*
				 * Since there is no PARENB in the Berkeley
				 * structures, PASS8 must mean no parity.  This
				 * could cause the loss of termio parity specs
				 * if used in combo-mode.
				 */
				tp->t_cflag |= CS8;	
				tp->t_cflag &= ~PARENB;
				tp->t_iflag &= ~ISTRIP;
			}
			else	
				tp->t_cflag |= (CS7 | PARENB);
		}
		if ((tp->t_flags & (CBREAK|RAW)) == 0) 	/* COOKED */
				tp->t_lflag |= ICANON;
		if (tp->t_flags & ECHO)
			tp->t_lflag |= ECHO;
		if (tp->t_flags & CRTBS)
                        tp->t_lflag_ext |= PCRTBS;
		if (tp->t_flags & CRTERA)
                        tp->t_lflag_ext |= PCRTERA;
		if (tp->t_flags & CRTKIL)
                        tp->t_lflag_ext |= PCRTKIL;
		if ((tp->t_flags & (CRTBS|CRTERA|CRTKIL)) == (CRTBS|CRTERA|CRTKIL))
			tp->t_lflag |= ECHOE;
		if (tp->t_flags & BNOFLSH)
			tp->t_lflag |= NOFLSH;
		if (tp->t_flags & CTLECH)
			tp->t_lflag_ext |= PCTLECH;
		if (tp->t_flags & PRTERA)
			tp->t_lflag_ext |= PPRTERA;

		/* Note: XCASE mapped in output modes */

	/*
	 *	CONTROL MODES
	 */
		/* Baud rate is set in TIOCSETP */
		if ( tp->t_flags & TOSTOP )
			tp->t_lflag_ext |= PTOSTOP;
		if ( tp->t_flags & AUTOFLOW )
			tp->t_cflag_ext |= PAUTOFLOW;
		/* Parity is set above in input modes. */
		/* Note CS7 or CS8 in local modes */

}


 
/* System V to ULTRIX mapping.
 * When setting parameters in the termio structure via TCSETA
 * it is also necessary map these changes where possible into
 * the sgtty terminal structure.  The following shows this
 * mapping on a flag-by-flag basis.
 */

ttmapV_U(tp)
	register struct tty *tp;
{
	register struct ttdelaystruct *te;

        TTY_ASSERT(tp);
	tp->t_flags &= CRMOD;		/* Start from scratch. Keep CRMOD */

#ifdef TTYDEBUG
	if (ttydebug > 4)
		mprintf("ttmapV_U: tp = %x\n",tp);
#endif TTYDEBUG
	/*
	 *	INPUT MODES
	 */
		if ((tp->t_iflag&ICRNL) && (tp->t_oflag&ONLCR))
			tp->t_flags |= CRMOD;
		if ((tp->t_iflag & IXANY) == 0)
			tp->t_flags |= DECCTQ;
		if ( tp->t_iflag & IXOFF )
			tp->t_flags |= TANDEM;
		if (tp->t_iflag&IUCLC && tp->t_oflag&OLCUC && tp->t_lflag&XCASE)
			tp->t_flags |= LCASE;
		if ( tp->t_iflag_ext & PPENDIN )
			tp->t_flags |= PENDIN;
		/* IGNPAR handled below in cflags where parity is examined */
			

	/*
	 *	OUTPUT MODES
	 */
		if (((tp->t_lflag_ext&PRAW) == 0) && ((tp->t_oflag&OPOST) == 0))
			tp->t_flags |= LITOUT;
		if (tp->t_oflag_ext & PTILDE)
			tp->t_flags |= TILDE;
		if (tp->t_oflag_ext & PFLUSHO)
			tp->t_flags |= FLUSHO;
		if (tp->t_iflag_ext & PCBREAK)
			tp->t_flags |= CBREAK;
		/* ONLCR and OLCUC are mapped above with input modes */

		/* Convert termio style delays to ULTRIX style via the
		 * ttdelaystruct mapping table.
		 */
		for (te = ttdelaystruct; te->vmode; te++){
			if ( (tp->t_oflag & te->vmask) == te->vmode )
				tp->t_flags |= te->umode;
		}
		if (tp->t_oflag_ext & PNL2)
			tp->t_flags |= NL1;

	/*
	 *	LOCAL MODES
	 */
		/*
		 * RAW mode is miserable to map because it stands for a group
		 * of terminal attributes, or rather a lack thereof.
		 */
		if ((tp->t_lflag & (ICANON|ISIG)) == 0) {
		    if (((tp->t_oflag & OPOST) == 0) &&
			((tp->t_cflag & (CS8|PARENB)) == CS8) &&
			(tp->t_cc[VMIN] == 1 && tp->t_cc[VTIME] == 0) &&
			((tp->t_iflag & (ICRNL|IUCLC|IGNCR|INLCR|IXON)) == 0))

			tp->t_flags |= (RAW | PASS8);	
		}
		else {
			tp->t_flags &= ~RAW;
			if ((tp->t_cflag & CS8) == CS8)
				tp->t_flags |= PASS8;
		}

		if (tp->t_lflag & ECHO)
			tp->t_flags |= ECHO;
		if (tp->t_lflag & ECHOE)
			tp->t_flags |= (CRTBS|CRTERA|CRTKIL);
		if (tp->t_lflag & NOFLSH) 
			tp->t_flags |= BNOFLSH;
		if (tp->t_lflag_ext & PCTLECH) 
			tp->t_flags |= CTLECH;
		if (tp->t_lflag_ext & PPRTERA) 
			tp->t_flags |= PRTERA;
		if (tp->t_lflag_ext & PCRTBS) 
			tp->t_flags |= CRTBS;
		if (tp->t_lflag_ext & PCRTERA) 
			tp->t_flags |= CRTERA;
		if (tp->t_lflag_ext & PCRTKIL) 
			tp->t_flags |= CRTKIL;
		/* XCASE is mapped above in input modes */

	/*
	 *	CONTROL MODES
	 */

		if ((tp->t_iflag & (INPCK | IGNPAR)) == 0) {
			if ((tp->t_flags & RAW) == 0)
				tp->t_flags |= ANYP;
		}
 		else if (tp->t_cflag & PARODD)
 			tp->t_flags = (tp->t_flags | ODDP) & ~EVENP;
 		else
 			tp->t_flags = (tp->t_flags | EVENP) & ~ODDP;
		if (tp->t_lflag_ext & PTOSTOP)
			tp->t_flags |= TOSTOP;
		if (tp->t_cflag_ext & PAUTOFLOW)
			tp->t_flags |= AUTOFLOW;

}

/*
 * Some of the extensions to the termios data structure actually are merely
 * functions of finer granularity than the corresponding termio flag, or
 * for example in the case of RAW mode they represent a group of attributes.
 * When setting the attributes of the termios data structure, clear out
 * those settings that are no longer valid.
 */
ttmap_P(tp)
        register struct tty *tp;
{
       	TTY_ASSERT(tp);
	/*
	 * RAW mode is miserable to map because it stands for a group
	 * of terminal attributes, or rather a lack thereof.
	 */
	if ((tp->t_lflag & ICANON) == 0) {
		if (((tp->t_oflag&OPOST) == 0) && ((tp->t_lflag&ISIG) == 0)
		    && ((tp->t_cflag & (CS8|PARENB)) == CS8) &&
		    (tp->t_cc[VMIN] == 1 && tp->t_cc[VTIME] == 0) &&
		    ((tp->t_iflag & (ICRNL|IUCLC|IGNCR|INLCR|IXON)) == 0)) {
			tp->t_lflag_ext |= PRAW ;	
			tp->t_iflag_ext &= ~PCBREAK ;	
		}
	}
	else {
		tp->t_lflag_ext &= ~PRAW ;	
		tp->t_iflag_ext &= ~PCBREAK ;	
	}

	if (((tp->t_oflag & OPOST) != 0) || ((tp->t_cflag & CS8) != CS8)
		|| (tp->t_cflag & PARENB))
		tp->t_oflag_ext &= ~PLITOUT;

        if (tp->t_lflag & ECHOE) 
                tp->t_lflag_ext |= (PCRTBS|PCRTERA|PCRTKIL);

}

/*
 * Min/Time timeout mechanism.  This is used as an inter-character timer
 * in cases where there are not at least MIN characters on the raw input 
 * queue.  TIME is a timer of .10 second length.  Min/ Time is only used
 * in RAW mode of the System V style line discipline.
 */

/* need a timeout routine for tttimeo to regain locks */
tty_timeout(tp)
struct tty *tp;
{
        int post_ttwakeup, saveipl;

        TTY_LOCK(tp, saveipl);
	post_ttwakeup = tttimeo(tp);
	if (post_ttwakeup) {
	    ttwakeup(tp);	/* ttwakeup will unlock tty lock */
	    splx(saveipl);	/* but does not lower IPL	 */
	} else {
	    TTY_UNLOCK(tp,saveipl);
	}
}

/*
 * tttimeo() was activated in two cases.  One situation was that timer
 * expired.  The other one was that we need to set the timer.  
 * TS_LRTO was used to signal which one we really want.
 */
tttimeo(tp)
struct tty *tp;
{
	extern int hz;
	int post_ttwakeup = NO_TTWAKEUP;

        TTY_ASSERT(tp);
	/* Indicate that a timeout is not already active. */
	tp->t_state &= ~TS_LTACT;
	/*
	 * Should only timeout in raw mode and a timeout interval is specified 
	 */
	if (((tp->t_lflag & ICANON) == 0) && (tp->t_cc[VTIME])) {
		/* Returns if characters are called for and none on queue */
		if( (tp->t_rawq.c_cc == 0) && (tp->t_cc[VMIN]))
			return(post_ttwakeup);
		/* If a raw timeout is not already in progress. */
		if(!(tp->t_state & TS_LRTO)) {
			tp->t_state |= TS_LRTO|TS_LTACT;
			timeout(tty_timeout,(caddr_t)tp,tp->t_cc[VTIME]*(hz/10));
		} else {
		        if (tp->t_smp&S_POST_WAKEUP) {
			    post_ttwakeup = DO_TTWAKEUP;
			} else {
			    ttwakeup(tp);
			}
		}
	}
	return(post_ttwakeup);
}

/*
 * Check the output queue on tp for space for a kernel message
 * (from uprintf/tprintf).  Allow some space over the normal
 * hiwater mark so we don't lose messages due to normal flow
 * control, but don't let the tty run amok.
 */
ttycheckoutq(tp, wait)
	register struct tty *tp;
	int wait;
{
	int hiwat, s;
	int saveaffinity;
	if (tp->t_smp==0)  
	    saveaffinity = switch_affinity(boot_cpu_mask);
	TTY_LOCK(tp,s);
	hiwat = TTHIWAT(tp);
	if (tp->t_outq.c_cc > hiwat + 200)
	    while (tp->t_outq.c_cc > hiwat) {
		ttstart(tp);
		if (wait == 0) {
			TTY_UNLOCK(tp,s);
			return (0);
		}
		tp->t_state |= TS_ASLEEP;
		TTY_SLEEP_RELOCK(tp,(caddr_t)&tp->t_outq, TTOPRI);
	}
	TTY_UNLOCK(tp,s);
	if (tp->t_smp==0) (void)switch_affinity(saveaffinity);
	return (1);
}

tty_def_open(tp, dev, flag, clocal)
    struct tty *tp;
    dev_t dev;
    int flag;
    int clocal;
{
    /*
     * Look at the compatibility mode to specify correct default parameters
     * and to insure only standard specified functionality.
     */
#ifdef TTYDEBUG
    if (ttydebug > 1)
	mprintf("tty_def_open: u_progenv = %x\n",u.u_procp->p_progenv);
#endif TTYDEBUG
    if ((u.u_procp->p_progenv == A_SYSV) || (u.u_procp->p_progenv == A_POSIX)) {
	flag |= O_TERMIO;
	tp->t_line = TERMIODISC;
    }
    /*
     * If this is first open, initialize tty state to default.
     * This relies on the close routine setting the baud rate to zero
     * meaning that the line is no longer in use.
     */
    if ((tp->t_state&TS_ISOPEN) == 0) {
	ttychars(tp);
	tp->t_tpath = 0;
	tp->t_dev = dev;  /* timeouts need this */
	if (((tp->t_cflag & CBAUD) == 0) || (u.u_procp->p_progenv == A_POSIX)) {
	    tp->t_cflag = ISPEED;
	    tp->t_cflag_ext = ISPEED;
	    tp->t_iflag_ext = 0;
	    tp->t_oflag_ext = 0;
	    tp->t_lflag_ext = 0;
	    /*
	     * Ultrix defaults to a "COOKED" mode on the first
	     * open, while termio defaults to a "RAW" style.
	     * Base this decision by a flag set in the termio
	     * emulation routine for open, or set by an explicit
	     * ioctl call.
	     */
	    if ( flag & O_TERMIO ) {
#ifdef TTYDEBUG
		if (ttydebug)
		    mprintf("tty_def_open: flag & O_TERMIO, setup termio environment\n");
#endif TTYDEBUG
		/* Provide a termio style environment.
		 * "RAW" style by default.
		 */
		tp->t_flags = RAW;   
		tp->t_iflag = 0;
		tp->t_oflag = 0;
		tp->t_cflag |= CS8|CREAD|HUPCL; 
		tp->t_lflag = 0;
		
		/*
		 * Change to termio line discipline.
		 */
		tp->t_line = TERMIODISC;
		/*
		 * The following three control chars have 
		 * different default values than ULTRIX.
		 */
		tp->t_cc[VERASE] = '#';
		tp->t_cc[VKILL] = '@';
		tp->t_cc[VINTR] = 0177;
		tp->t_cc[VMIN] = 6;
		tp->t_cc[VTIME] = 1;
	    } else {
		/* Provide a backward compatible ULTRIX 
		 * environment.  "COOKED" style.
		 */
		tp->t_flags = IFLAGS;  /* ULTRIX - moved */
		tp->t_iflag = IFLAG;
		tp->t_oflag = OFLAG;
		tp->t_lflag = LFLAG;
		tp->t_cflag |= CFLAG;
	    }
	    if (clocal)
		tp->t_cflag |= CLOCAL;
	}
    }
}

tty_def_close(tp)
    register struct tty *tp;
{
    tp->t_iflag &= ~TERMIO_ONLY_IFLAG;
    tp->t_oflag &= ~TERMIO_ONLY_OFLAG;
    tp->t_cflag &= ~TERMIO_ONLY_CFLAG;
    tp->t_lflag &= ~TERMIO_ONLY_LFLAG;
}

