#ifndef lint
static char *sccsid = "@(#)subr_prf.c	4.7      (ULTRIX)  12/20/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1983,86,87, 89 by			*
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
 * Modification History
 *
 * 18-Dec-90 Jim Paradis
 *	Idle vector processor before stopping CPU (VAX only)
 *
 * 19-Sep-90 --	jas
 *	Modified putchar() routine to use TURBOchannel ROM callback
 *	interface if TURBOchannel console is present.
 *
 * 22-May-90 -- gmm
 *	Store the pid of the process on the paniced cpu in cur_pid in panic().
 *	This is for easier debugging with dbx. (mips only)
 *
 * 30-Dec-89 -- bp
 *	Removed atintr_level and use now a cpu flag to detect
 *	whether we are in an interrupt service routine.
 *
 * 08-Dec-89 -- gmm
 *	Changed panic_flag from regular int to volatile for secondary cpu's
 *	panic to work in MIPS MP systems.
 *
 * 14-Oct-89 -- robin
 *	Added include for ../io/uba/qdioclt to pick up define of
 *	QD_KERN_UNLOOP for VAX systems.  Somehow this was lost.
 *
 * 13-Oct-89 -- gmm
 *	Merged back panic() for VAX and MIPS. With SMP support in MIPS, 
 *	panic() should be common for all architectures.
 *
 * 28-Mar-89 -- Tim Burke
 *    Modified harderr() to be able to handle errors on MSCP (ra) type
 *    disks by also looking at the major number in calculating unit number.
 *
 * 10-Feb-89 -- Randall Brown
 *     	Merged with VAX.
 *		- included changed to queue interrupt level and other printfs
 *		  onto seperate queues.  Helps prevent boot time scribling.
 *		- moved the code for panic() into panic.c in the appropriate
 *		  machine directory.
 *
 * 29-Dec-88 -- Randall Brown
 *	In the 'panic' routine, changed the state of the variable 
 *	printstate so that printf (and cprintf) will use the prom
 *	putchar. (only if console is graphic device)
 *
 * 16 Dec 88 -- depp
 *	Put temorary fix into uprintf() to avoid crash.  The problem seems
 *	be in the sleep during ttycheckout(). To avoid, do not sleep 
 *	if the character queue overflows, simply drop the message.
 *
 * 10-Oct-88 - lp
 *	Fixed calls to various print routines from prf. Use masks
 *	instead of wired numbers. Caused 'panic sleep' from device
 *	interrupts as uprintf was inadvertently being called. A second
 *	error was also found in kern_errlog with the setting of 
 *	ipl to the wrong value (Al blew it).
 *
 * 06-Sep-88 -- afd
 *	Ported to PMAX.  Use varargs macros for variable number of args.
 *
 *	Cleaned up the "touser" argument to "prf" routine, it had become an
 *	"over-used" flag.  Replaced it by "whereto" flag which is a bit mask
 *	of where to send the output: TOCONS, TOBUF, TOUSER.
 *
 * 28-Jan-87 -- jaw 	add debug info for slaves on panic 
 *
 * 28-Dec-87 -- Tim Burke
 *  	Moved u.u_ttyp to u.u_procp->p_ttyp.
 *
 * 14-Dec-87 -- jaa
 *	Added new KM_ALLOC/KM_FREE macros
 *
 * 14-May-87 -- fred (Fred Canter)
 *	Turn off graphics console loopback of kernel printf's
 *	when the system panics.
 *
 * 14-Jan-87 -- pmk
 *	Added check for valid ksp in panic()
 *
 * 04-Dec-86 -- pmk
 *	Changed mprintf errlog amount to 256 char., added intr & 
 *	kernel stack address to errlog stack info. and changed
 *	how intr. stack amount is calculated
 *
 * 22-Jul-86 -- bjg
 *	Change appendflg to log ALL startup messages together
 *
 * 09-Jun-86 -- bjg
 *	Added check for appendflg to log startup mesgs
 *
 * 30-May-86 -- pmk 
 *	Added prtstk routine to format/print stack dumps with call frame
 *	lables.
 *
 * 02-Apr-86 -- jrs
 *	Clean up panic so slaves can panic and notify master to bring
 *	whole system down cleanly
 *
 * 19-Mar-86 -- pmk
 *	Cleaned up panic delay and changed output format.
 *
 * 18-Mar-86 -- jrs
 *	Cleaned up cpu determination.  Allow slave to printf now
 * 
 * 05-Mar-86 pmk
 *	Added append option to printf for autoconf.
 *	Changed delay depend for panic printout.
 *
 * 12-Feb-86 pmk
 *	Changed errlogging of asc printf & mprintf.
 *	Added cpu depend delay to panic printout.
 *
 * 05-Feb-86 pmk
 *	Fixed the way panic string is null terminated.
 *	Added use of NISP from param.h for stack size.
 *
 * 20-jan-86 bglover
 *	Added size field support for %s in printf, mprintf, cprintf
 *
 * 20-jan-86 pmk
 *	Added binary error logging support for panic, printf, mprintf.
 *	Added cprintf routine.
 *
 * 10/1/85 - Larry Cohen
 *	include ioctl.h so that tty.h knows about window size structure
 *
 * 22-Feb-84 tresvik
 *	Add mprintf for use with diagnostic printouts from drivers.
 *	Output does not go to the console, instead it goes to msgbuf.
 *
 * 	Added a carriage return to the end of the harderr output string
 *	format.
 */


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/seg.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/reboot.h"
#include "../h/vm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/types.h"
#include "../h/errlog.h"
#include "../h/kmalloc.h"
#include "../h/cpudata.h"
#include "../machine/cpu.h"

#ifdef vax
#include "../machine/mtpr.h"
#include "../io/uba/qdioctl.h"	/* for QD_KERN_UNLOOP used below */
#endif vax
#ifdef mips
#include "../h/varargs.h"
extern u_int printstate;
extern  int     rex_base;
#endif mips
#ifdef GPROF
#include "../h/gprof.h"
#endif GPROF

/*
 * Where to send to output.
 * These flags are passed from printf/uprintf/cprintf/mprintf to prf().
 */
#define TOCONS	0x1		/* print to system console */
#define TOBUF	0x2		/* print to error log buffer */
#define TOUSER	0x4		/* print to user's tty */

/* For configuration printf's to be errlogged together */
int	appendflg = 0;
char	*cfgbufp;
char	*cfgbufp2;

/* Used for logging ascii messages */
static int msgsize;

char	panic_buffer;	/* one character buffer to pass data to boot cpu */
volatile int 	panic_flag;	/* flag is set if panic_buffer has data. 
				   This should be volatile for panic to work
				   on secondary processor in MIPS systems */


/*
 * In case console is off,
 * panicstr contains argument to last
 * call to panic.
 */
char	paniccpu;  	/* which cpu first paniced */
char	*panicstr;	/* panic string for first cpu */
int lk_panic;

#ifdef vax
#ifdef KDEBUG
int radix = 16;
extern int enter_kdb_now, in_kdb,kdb_req_cpu,ttykdb;
#endif KDEBUG

extern	int	ws_display_type;
#endif vax

/*
 * Scaled down version of C Library printf.
 * Used to print diagnostic information directly on console tty.
 * Since it is not interrupt driven, all system activities are
 * suspended.  Printf should not be used for chit-chat.
 *
 * Additional formats:
 *
 * %b is supported to decode error registers.
 * Usage is:
 *	printf("reg=%b\n", regval, "<base><arg>*");
 * Where <base> is the output base expressed as a control character,
 * e.g. \10 gives octal; \20 gives hex.  Each arg is a sequence of
 * characters, the first of which gives the bit number to be inspected
 * (origin 1), and the next characters (up to a control character, i.e.
 * a character <= 32), give the name of the register.  Thus
 *	printf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 * would produce output:
 *	reg=2<BITTWO,BITONE>
 *
 * %r and %R formats
 * printf("%r %R", val, reg_descp);
 * struct reg_desc *reg_descp;
 *
 * the %r and %R formats allow formatted print of bit fields.  individual
 * bit fields are described by a struct reg_desc, multiple bit fields within
 * a single word can be described by multiple reg_desc structures.
 * %r outputs a string of the format "<bit field descriptions>"
 * %R outputs a string of the format "0x%x<bit field descriptions>"
 *
 * The fields in a reg_desc are:
 *	unsigned rd_mask;	An appropriate mask to isolate the bit field
 *				within a word, and'ed with val
 *
 *	int rd_shift;		A shift amount to be done to the isolated
 *				bit field.  done before printing the isolate
 *				bit field with rd_format and before searching
 *				for symbolic value names in rd_values
 *
 *	char *rd_name;		If non-null, a bit field name to label any
 *				out from rd_format or searching rd_values.
 *				if neither rd_format or rd_values is non-null
 *				rd_name is printed only if the isolated
 *				bit field is non-null.
 *
 *	char *rd_format;	If non-null, the shifted bit field value
 *				is printed using this format.
 *
 *	struct reg_values *rd_values;	If non-null, a pointer to a table
 *				matching numeric values with symbolic names.
 *				rd_values are searched and the symbolic
 *				value is printed if a match is found, if no
 *				match is found "???" is printed.
 *				
 * For examples of register descriptions see the kernel file mips/debug.c
 *
 * printf will also accept a field number, zero filling to length.
 * 	printf(" %8x\n",regval); max field size is 11.
 *
 * printf will also log the ascii to the errlog.
 */
/*VARARGS1*/
printf(fmt, va_alist)
	char *fmt;
#ifdef vax
	unsigned va_alist;			/* first arg */
{
	char *ap = (char *) &va_alist;		/* declare & init ap */
#endif vax
#ifdef mips
	va_dcl					/* va_alist: first arg */
{
	va_list(ap);				/* declare ap */
#endif mips
	struct el_rec *elrp;
	struct el_msg *elmsgp;
	struct el_msg *elmsgp2;
	int i,s;
#ifdef mips
	va_start(ap);				/* init ap = &va_alist */
	/*
	 * If memory subsystem not initialized yet, just print to console
	 */
	if ((printstate & MEMPRINT) == 0) {
	        prf((struct el_msg *)0, fmt, ap, TOCONS);
		return;
	}
#endif mips
	/*
	 * "appendflg" is set for logging startup/config messages.
	 * "cfgbufp" is NULL on first kernel printf, so allocate space
	 * for the startup messages.
	 */
	if ((appendflg) && (cfgbufp == NULL)) {
	    KM_ALLOC(cfgbufp, caddr_t, EL_SIZE2048, KM_TEMP, KM_CLEAR | KM_NOWAIT);
	    KM_ALLOC(cfgbufp2, caddr_t, EL_SIZE2048, KM_TEMP, KM_CLEAR | KM_NOWAIT);
	    if (cfgbufp != NULL) {
	        elmsgp = (struct el_msg *)cfgbufp;
	        elmsgp2 = (struct el_msg *)cfgbufp2;
	        elmsgp->msg_len = 1;
	        elmsgp2->msg_len = 1;
	    }
	}
	/*
	 * Still printing startup/config messages
	 */
	if (cfgbufp != NULL) {
	    elmsgp = (struct el_msg *)cfgbufp;
	    elmsgp2 = (struct el_msg *)cfgbufp2;
	    s = splhigh();
	    msgsize = EL_SIZE2048 - 1;
#ifdef vax
	    if ((getpsl()) & 0x4000000) {
#endif vax
#ifdef mips
	    if (CURRENT_CPUDATA->cpu_inisr) {
#endif mips
		/*
		 *  interrupt level printfs are logged now but printed later.
		 */
	    	elmsgp2 = (struct el_msg *)cfgbufp2;
	    	elmsgp2->msg_len--;
	    	prf(elmsgp2, fmt, ap, TOBUF);  
	    	elmsgp2->msg_asc[elmsgp2->msg_len++] = '\0';
	    } else { 
	    	elmsgp = (struct el_msg *)cfgbufp;
	    	elmsgp->msg_len--;
	    	prf(elmsgp, fmt, ap, TOBUF|TOCONS);
	    	elmsgp->msg_asc[elmsgp->msg_len++] = '\0';
	    }
	    /*
	     * When "appendflg" is cleared (after autoconfig is done)
	     * then we are done with configuration messages.
	     * So allocate an errlog packet and copy the
	     * startup/config messages, then validate the pkt for elcsd.
	     */
	    if ((appendflg == 0) || (elmsgp->msg_len > EL_SIZEAPPND)
	    		|| (elmsgp2->msg_len > EL_SIZEAPPND)) {
	        elrp = ealloc(elmsgp->msg_len + 2 + elmsgp2->msg_len,EL_PRILOW);
	        if (elrp != NULL) {
	            LSUBID(elrp,ELMSGT_SU,EL_UNDEF,EL_UNDEF,
			        EL_UNDEF,EL_UNDEF,EL_UNDEF);
		    elrp->el_body.elmsg.msg_len = elmsgp->msg_len + elmsgp2->msg_len;
	    	    bcopy(elmsgp->msg_asc, elrp->el_body.elmsg.msg_asc,
			  elmsgp->msg_len);
	    	    bcopy(elmsgp2->msg_asc, &elrp->el_body.elmsg.msg_asc[elmsgp->msg_len - 1],
			  elmsgp2->msg_len);
	            EVALID(elrp);
		}
		/* print interrupt level messages now */
		for (i = 0 ; i < elmsgp2->msg_len ; i++) {
		    cnputc((elmsgp2->msg_asc[i]));
		}
	        elmsgp->msg_len = 1;
	        elmsgp2->msg_len = 1;
		/*
		 * When "appendflg" is cleared (done with config messages)
		 * free the memory we were using for the startup/config
		 * messages.
		 */
		if (appendflg == 0) {
		    KM_FREE(cfgbufp, KM_TEMP);
		    cfgbufp = NULL;
		    KM_FREE(cfgbufp2, KM_TEMP);
		    cfgbufp2 = NULL;
		}
	    }
	    splx(s);
	}
	/*
	 * Not doing startup/config messages
	 */
	else {
	    elrp = ealloc(EL_SIZE128 + 2,EL_PRILOW);
	    if (elrp != NULL) {
	        LSUBID(elrp,(appendflg == 0)?ELMSGT_INFO:ELMSGT_SU,
			    EL_UNDEF,EL_UNDEF,
			    EL_UNDEF,EL_UNDEF,EL_UNDEF);
	        elmsgp = &elrp->el_body.elmsg;
	        elmsgp->msg_len = 0;
		msgsize = EL_SIZE128 - 1;
	        prf(elmsgp, fmt, ap, TOCONS);
	        elmsgp->msg_asc[elmsgp->msg_len++] = '\0';
	        EVALID(elrp);
	    } else {
	        prf((struct el_msg *)0, fmt, ap, TOCONS);
	    }
	}
}

/*
 * Uprintf prints to the current user's terminal,
 * guarantees not to sleep (so can be called by interrupt routines)
 * and does no watermark checking - (so no verbose messages).
 * Also uprintf doesn't log msg to errlog pass null pointer.
 */
/*VARARGS1*/
uprintf(fmt, va_alist)
	char *fmt;
#ifdef vax
	unsigned va_alist;			/* first arg */
{
	char *ap = (char *) &va_alist;		/* declare & init ap */
#endif vax
#ifdef mips
	va_dcl					/* va_alist: first arg */
{
	register struct tty *tp = u.u_procp->p_ttyp;
	va_list(ap);				/* declare ap */
	if (tp == NULL)
		return;

	va_start(ap);				/* init ap = &va_alist */
#ifdef notdef	/* this is a temp fix panic segment faults ... depp */
	(void)ttycheckoutq(tp, 1);		/* make sure there's rm in Q */
#else  notdef
	if (ttycheckoutq(tp,0) == 0)
		return;
#endif notdef
#endif mips
	prf((struct el_msg *)0, fmt, ap, TOUSER);
}

/* 
 * Cprintf prints ONLY to the console. This is for reporting 
 * information when the errlog mech. has a problem. You don't
 * want to call your self, passes null pointer.
 */
/*VARARGS1*/
cprintf(fmt, va_alist)
	char *fmt;
#ifdef vax
	unsigned va_alist;			/* first arg */
{
	char *ap = (char *) &va_alist;		/* declare & init ap */
#endif vax
#ifdef mips
	va_dcl					/* va_alist: first arg */
{
	va_list(ap);				/* declare ap */

	va_start(ap);				/* init ap = &va_alist */
#endif mips
	prf((struct el_msg *)0, fmt, ap, TOCONS);
}

/* 
 * Mprintf is used whenever Kernel printout should appear in the messages
 * file via the Kernel message buffers but need not be printed on the console
 * terminal, i.e. hardware failures which are considered soft and corrected.
 *
 * Mprintf will log all messages to errlog by ealloc and evalid.
 */
/*VARARGS1*/
mprintf(fmt, va_alist)
	char *fmt;
#ifdef vax
	unsigned va_alist;			/* first arg */
{
	char *ap = (char *) &va_alist;		/* declare & init ap */
#endif vax
#ifdef mips
	va_dcl					/* va_alist: first arg */
{
	va_list(ap);				/* declare ap */
#endif mips
	struct el_rec *elrp;
	struct el_msg *elmsgp;
#ifdef mips
	va_start(ap);				/* init ap = &va_alist */
	/*
	 * If memory subsystem not initialized yet, just print to console
	 */
	if ((printstate & MEMPRINT) == 0) {
	        prf((struct el_msg *)0, fmt, ap, TOCONS);
		return;
	}
#endif mips
	elrp = ealloc(EL_SIZE256 + 2,EL_PRILOW);
	if (elrp != NULL) {
	    LSUBID(elrp,ELMSGT_INFO,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
	    elmsgp = &elrp->el_body.elmsg;
	    elmsgp->msg_len = 0;
	    msgsize = EL_SIZE256 - 1;
	    prf(elmsgp, fmt, ap, TOBUF);
	    elmsgp->msg_asc[elmsgp->msg_len++] = '\0';
	    EVALID(elrp);
	}
}

/*
 * Prf now accepts a logmsg pointer which in turn it passes to printn()
 * and putchar(). Also prf now looks for a field length number to
 * pass to printn().
 */
prf(msgp, fmt, adx, whereto)
	struct el_msg *msgp;		/* ptr to msg buffer (or NULL) */
	register char *fmt;		/* ptr to print format string */
	register u_int *adx;		/* ptr to first arg to print */
	int whereto;			/* where to send the printout */
{
	register int b, c, i;
	char *s;
	int any;
	int num;
loop:
	while ((c = *fmt++) != '%') {
		if(c == '\0') {
			if (!BOOT_CPU) intrpt_cpu(boot_cpu_num,IPI_PRINT); 
			return;
		}
		putchar(msgp, c, whereto);
	}
	num = 0;
again:
	c = *fmt++;
	/* THIS CODE IS 32-bit-word DEPENDENT IN HANDLING %l */
	/* %c assumes signed chars and only uses 7 bits */
	/* %C assumes unsigned chars and uses 8 bits */
	switch (c) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		if (num == 0)
			num = c - '0';
		else
			num = 10 * num + c - '0';
		goto again;
	case 'l':
		goto again;
	case 'x': case 'X':
		b = 16;
		goto number;
	case 'd': case 'D':
	case 'u':		/* what a joke */
		b = 10;
		goto number;
#ifdef vax
#ifdef KDEBUG
	case 'r': case 'R':
		b=radix;
		goto number;
		break;
#endif KDEBUG
#endif vax
	case 'o': case 'O':
		b = 8;
number:
		printn(msgp, num, (u_long)*adx, b, whereto);
		break;
	case 'c':
		b = *adx;
		for (i = 24; i >= 0; i -= 8)
			if (c = (b >> i) & 0x7f)
				putchar(msgp, c, whereto);
		break;
	case 'C':
		b = *adx;
		for (i = 24; i >= 0; i -= 8)
			if (c = (b >> i) & 0xff)
				putchar(msgp, c, whereto);
		break;
	case 'b':
		b = *adx++;
		s = (char *)*adx;
		printn(msgp, num, (u_long)b, *s++, whereto);
		any = 0;
		if (b) {
			putchar(msgp, '<', whereto);
			while (i = *s++) {
				if (b & (1 << (i-1))) {
					if (any)
						putchar(msgp, ',', whereto);
					any = 1;
					for (; (c = *s) > 32; s++)
						putchar(msgp, c, whereto);
				} else
					for (; *s > 32; s++)
						;
			}
			if (any)
				putchar(msgp, '>', whereto);
		}
		break;
#ifdef mips
	case 'r':
	case 'R':
		b = *adx++;
		s = (char *)*adx;
		if (c == 'R') {
			puts(msgp, "0x", whereto);
			printn(msgp, num, (u_long)b, 16, whereto);
		}
		any = 0;
		if (c == 'r' || b) {
			register struct reg_desc *rd;
			register struct reg_values *rv;
			register unsigned field;

			putchar(msgp, '<', whereto);
			for (rd = (struct reg_desc *)s; rd->rd_mask; rd++) {
				field = b & rd->rd_mask;
				field = (rd->rd_shift > 0)
				    ? field << rd->rd_shift
				    : field >> -rd->rd_shift;
				if (any &&
				      (rd->rd_format || rd->rd_values
				         || (rd->rd_name && field)
				      )
				)
					putchar(msgp, ',', whereto);
				if (rd->rd_name) {
					if (rd->rd_format || rd->rd_values
					    || field) {
						puts(msgp, rd->rd_name, whereto);
						any = 1;
					}
					if (rd->rd_format || rd->rd_values) {
						putchar(msgp, '=', whereto);
						any = 1;
					}
				}
				if (rd->rd_format) {
					if (whereto & TOCONS)
						cprintf(rd->rd_format, field);
					if (whereto & TOBUF)
						mprintf(rd->rd_format, field);
					if (whereto & TOUSER)
						uprintf(rd->rd_format, field);
					any = 1;
					if (rd->rd_values)
						putchar(msgp, ':', whereto);
				}
				if (rd->rd_values) {
					any = 1;
					for (rv = rd->rd_values;
					    rv->rv_name;
					    rv++) {
						if (field == rv->rv_value) {
							puts(msgp, rv->rv_name, whereto);
							break;
						}
					}
					if (rv->rv_name == NULL)
						puts(msgp, "???", whereto);
				}
			}
			putchar(msgp, '>', whereto);
		}
		break;

	case 'n':
	case 'N':
		{
			register struct reg_values *rv;

			b = *adx++;
			s = (char *)*adx;
			for (rv = (struct reg_values *)s; rv->rv_name; rv++) {
				if (b == rv->rv_value) {
					puts(msgp, rv->rv_name, whereto);
					break;
				}
			}
			if (rv->rv_name == NULL)
				puts(msgp, "???", whereto);
			if (c == 'N' || rv->rv_name == NULL) {
				putchar(msgp, ':', whereto);
				printn(msgp, num, (u_long)b, 10, whereto);
			}
		}
		break;
#endif mips

	case 's':
		s = (char *)*adx;
		if (num > 0) {
			while ((c = *s++) && (num-- > 0))
				putchar(msgp, c, whereto);
		}
		else {
			while (c = *s++) 
				putchar(msgp, c, whereto);
		}
		break;

	case '%':
		putchar(msgp, '%', whereto);
		break;
	}
	adx++;
	goto loop;
}

/*
 * Printn prints a number n in base b.
 * We don't use recursion to avoid deep kernel stacks.
 *
 * Printn now accepts msgp which it passes to putchar().
 * Also the num parm is used to determine field length, zero filling.
 */
printn(msgp, num, n, b, whereto)
	struct el_msg *msgp;
	int num;
	u_long n;
{
	char prbuf[11];
	register char *cp;
	int i = 0;

	if (num > 11)
		num = 11;

	if (b == 10 && (int)n < 0) {
		putchar(msgp, '-', whereto);
		n = (unsigned)(-(int)n);
	}
	cp = prbuf;
	do {
		*cp++ = "0123456789abcdef"[n%b];
		n /= b;
		i++;
		if (n == 0 && i < num )
			while ( i < num) {
				*cp++ = '0';
				i++;
			}
	} while (n);
	do
		putchar(msgp, *--cp, whereto);
	while (cp > prbuf);
}

/*
 * Warn that a system table is full.
 */
tablefull(tab)
	char *tab;
{
	printf("%s: table is full\n", tab);
}

/*
 * Hard error is the preface to plaintive error messages
 * about failing disk transfers.
 */
harderr(bp, cp)
	struct buf *bp;
	char *cp;
{
	register int unit_num;
	register char partition;

	unit_num = minor(bp->b_dev) >> 3;
	partition = 'a'+(minor(bp->b_dev)&07);
	/*
	 * The unit number calculation is different for MSCP disks.  First
	 * determine if it is an MSCP disk and then get the unit number.
 	 *
	 * MSCP (ra) type disks can occupy a range of major numbers.  Also
	 * look at the major number in unit number calculations.
	 */
        if (bp->b_flags & B_PHYS) {			/* Char device */
		if (MSCP_C_DEV(bp->b_dev))  {
                	unit_num += MSCP_C_UNIT_OFFSET(bp->b_dev);
		}
        }
	else {						/* Block device */
        	if (MSCP_B_DEV(bp->b_dev)) {
                	unit_num += MSCP_B_UNIT_OFFSET(bp->b_dev);
        	}
	}
	
	printf("%s%d%c: hard error sn%d ", cp,
	    unit_num, partition, bp->b_blkno);
}

/*
 * Print a character on console or users terminal.
 * If destination is console then the last MSGBUFS characters
 * are saved in msgbuf for inspection later.
 */
/*ARGSUSED*/
putchar(msgp, c, whereto)
	struct el_msg *msgp;
	register int c;
	int whereto;			/* key for where to print */
{

	if (whereto & TOUSER) {
		register struct tty *tp = u.u_procp->p_ttyp;
		register int s,saveaffinity,poststart_arg;
		if (tp) {
			if (tp->t_smp == 0) saveaffinity =switch_affinity(boot_cpu_mask);
			TTY_LOCK(tp,s);

			if ((tp->t_state & (TS_CARR_ON | TS_ISOPEN)) ==
		    		(TS_CARR_ON | TS_ISOPEN)) {
				if (c == '\n')
					(void) ttyoutput('\r', tp);
				(void) ttyoutput(c, tp);
				poststart_arg = ttstart(tp);
				if (poststart_arg && (tp->t_smp&S_POST_START)) {
				    tp->t_smp &= ~S_DO_UNLOCK;
				    (*tp->t_poststart)(tp, poststart_arg);
				}
			}
			TTY_UNLOCK(tp,s);
			if (tp->t_smp==0) (void)switch_affinity(saveaffinity);
		}
		return;
	}
	if (c != '\0' && c != '\r' && c != 0177
#ifdef vax
	    && mfpr(MAPEN)
#endif vax
	    ) {
		/* put item in el_msg struct for errlogging */
		if (msgp != 0) {
		    if (msgp->msg_len < msgsize)
		            msgp->msg_asc[msgp->msg_len++] = c;
		}
	}
	if (c == 0)
		return;
	/* 
	 * Should this message go to the console?
	 * Yes for printf and cprintf, No for mprintf and uprintf.
	 */
	if (whereto & TOCONS) {
		/*
		 * If console device driver not initialized, use PROM print
		 */
		if (BOOT_CPU) {
 
#ifdef mips
			if ((printstate & CONSPRINT) == 0) {
				if(rex_base) {
					rex_printf("%c",c);
				}
				else
					prom_putchar(c);
			} else
#endif mips
				cnputc(c);

		} else {
			if (CURRENT_CPUDATA->cpu_state & CPU_PANIC) {
				while(panic_flag) { 
					/* wait for master */ 
				}
				if (c == '\n') {
					panic_buffer = '\r';
					setlock(&panic_flag);
					while(panic_flag) { 
						/* wait for master */ 
					}
 				}
				panic_buffer = c;
				setlock(&panic_flag);
			} else {
				cnputc_slave(c);
		
			}
			
		}

	}
}

cnputc_slave(c) 
char c;
{
	register int s;
	register int diff;
	struct cpudata *pcpu;
	
	pcpu = CURRENT_CPUDATA;

	s = splextreme();
	smp_lock(&lk_printf, LK_RETRY);
	diff = pcpu->cpu_bufin - pcpu->cpu_bufout;

	if (diff >= 0) {
		/* check for buffer full */
		if ((pcpu->cpu_bufin == (CPUDATA_BUFSIZE-1)) &&
		    (pcpu->cpu_bufout == 0)) {
			if (cnputc_block(pcpu,s)) return;
		}

		pcpu->cpu_buf[pcpu->cpu_bufin++] = c;
		if (pcpu->cpu_bufin == CPUDATA_BUFSIZE)
			pcpu->cpu_bufin = 0;
		smp_unlock(&lk_printf);
		splx(s);

		if (c == '\n') {
			c = '\r';
			cnputc_slave(c);	
		}
	} else {
		/* check for buffer full */
		if (diff == -1)
			if(cnputc_block(pcpu,s)) return;
		pcpu->cpu_buf[pcpu->cpu_bufin++] = c;
		smp_unlock(&lk_printf);
		splx(s);

		if (c == '\n') {
			c = '\r';
			cnputc_slave(c);	
		}
	}
}

/* this routine is called if buffer is full.  Wait until buffer is 
   completely empty */

cnputc_block(pcpu,s) 
struct cpudata *pcpu;
register int s;

{
	int saved_bufout;

	/* save off out buffer pointer to make sure progress is 
	   being made */
	saved_bufout = pcpu->cpu_bufout;

	while (pcpu->cpu_bufin != pcpu->cpu_bufout) {
		smp_unlock(&lk_printf);
		splx(s);

		DELAY(100000);
		s=splextreme();
		smp_lock(&lk_printf, LK_RETRY);		
		/* if not progress after 1/10 of a second then 
		   drop character */
		if (pcpu->cpu_bufout == saved_bufout) {
			/* NO PROGRESS */
			smp_unlock(&lk_printf);
			splx(s);
			return(1);
		}
	}
	DELAY(100000); /* give master a little more time */

	return(0);
}


#ifdef mips
/* 
 * dprintf prints ONLY to the console. This is what the mips dprintf does.
 */
/*VARARGS1*/
dprintf(fmt, va_alist)
	char *fmt;
	va_dcl					/* va_alist: first arg */
{
	va_list(ap);				/* declare ap */

	va_start(ap);				/* init ap = &va_alist */
	prf((struct el_msg *)0, fmt, ap, TOCONS);
	va_end(ap);
}
#endif mips

/*
 * Put out a string (by calling putchar with each character
 */
puts(msgp, s, whereto)
	struct el_msg *msgp;
	register char *s;
	int whereto;			/* key for where to print */
{
	while (*s)
		putchar(msgp, *s++, whereto);
}

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then reboots.
 * If we are called twice, then we avoid trying to
 * sync the disks as this often leads to recursive panics.
 */

#ifdef mips
extern int cur_pid;
#endif mips

panic(s)
	char *s;
{

#ifdef vax
	register int *reg11;		/* must be first reg. declaration */
#endif vax
	int bootopt = RB_AUTOBOOT;
	int i;
	register int cpunum;
	register struct cpudata *pcpu;

#ifdef vax
#ifdef GPROF
	extern int profiling;
	profiling = PROFILING_OFF;	/* turn off profiling */
#endif GPROF
	asm("movl r13,r11"); /* grab frame pointer */
	
	
	/*
	 * If console is a graphics device,
	 * force cprintf messages directly to screen.
	 */
	if (ws_display_type) {
	    i = ws_display_type << 8;
	    (*cdevsw[ws_display_type].d_ioctl)(i, QD_KERN_UNLOOP, 0, 0);
	}
#endif vax

#ifdef mips
	printstate |= PANICPRINT;
#endif mips

	smp_enable_trace=0;

	/* please don't interrupt */
	(void)spl7();
	pcpu = CPUDATA((cpuident()));
	cpunum = pcpu->cpu_num;

	if ((pcpu->cpu_state & CPU_PANIC) != 0) {
		if (!BOOT_CPU)  {
#if defined(__vax)
			/* If this is a vector-capable CPU, then wait
			 * for the attached vector processor to go idle.
			 * This is a no-op on non-vector-capable VAXen
			 */
                        vp_idle();
#endif /* __vax */

			/* if we are slave, just stop 
				on multiple panic */
			stop_secondary_cpu();			
		}
		bootopt |= RB_NOSYNC;
		boot(RB_PANIC, bootopt);
		/* NO RETURN */
	} else {
		/* save off frame pointer for stack trace */
#ifdef vax
		pcpu->cpu_stack = (char *)reg11; 
#endif vax
		/* set flag so we don't loop */
		pcpu->cpu_state |= CPU_PANIC;

		/* save panic string in per-cpu area */
		pcpu->cpu_panicstr = s;
#if defined(__vax)
		/* If this is a vector-capable CPU, then wait
		 * for the attached vector processor to go idle.
		 * This is a no-op on non-vector-capable VAXen
		 */
                vp_idle();
#endif /* __vax */
	}

	/* wait for panic lock to clear */
	while(setlock(&lk_panic) == 0) {
		/* if we are boot cpu we must print characters
		   for the slave cpus */
		if (BOOT_CPU) {
			if (panic_flag) { 
				cnputc(panic_buffer);
				clearlock(&panic_flag);
			}
		} else {
			 /* some one else paniced...save state and stop */
#if defined(__vax)
			/* If this is a vector-capable CPU, then wait
			 * for the attached vector processor to go idle.
			 * This is a no-op on non-vector-capable VAXen
			 */
                        vp_idle();
#endif /* __vax */
			stop_secondary_cpu();
			/* no return */
		}
	}

	/* check if first panic */
	if (panicstr!=0) {	
		/* not first panic */
		if (!BOOT_CPU){	/* if this is not the boot cpu just stop */
			clearlock(&lk_panic);
#if defined(__vax)
			/* If this is a vector-capable CPU, then wait
			 * for the attached vector processor to go idle.
			 * This is a no-op on non-vector-capable VAXen
			 */
                        vp_idle();
#endif /* __vax */
			stop_secondary_cpu();
			/* no return */
		}
	} else {
		/* first panic */
		panicstr = s;
		paniccpu = cpunum;

		/* stop the world ..... */
		for(i=lowcpu; i<=highcpu; i++) {
		
			if (CPUDATA(i) && cpunum != i) intrpt_cpu(i,IPI_PANIC);
		}
		cprintf("\n\ncpu %d panic: %s\n",cpunum, s);
        	DELAY(250000);
		if (panicstr == s) {
			for(i=lowcpu; i<=highcpu; i++) {
				if (CPUDATA(i) == 0) continue;
				print_locks_held(i);
				print_lock_trace(i);
				DELAY(25000);
			}
			print_process_locks();
#ifdef vax
#ifdef KDEBUG
			kdb_req_cpu = cpunum;
#endif KDEBUG
#endif vax
		}
#ifdef mips
		/* save pid for dbx */
		if(pcpu->cpu_noproc)
			cur_pid = pcpu->cpu_idleproc->p_pid;
		else
			cur_pid = pcpu->cpu_proc->p_pid; 
#endif mips  
	}

	
#ifdef vax
#ifdef KDEBUG
	if (BOOT_CPU && in_kdb< 3 && ttykdb) {
		int a = 0, b = 1;
		enter_kdb_now = 1;
		b/=a;
	}
#endif KDEBUG
	panic_log(s,cpunum,reg11);
#endif vax

#ifdef mips
	panic_log(s);
#endif mips

	if (!BOOT_CPU) {
		/* if we are slave, let master do the reboot */
		clearlock(&lk_panic);
#if defined(__vax)
		/* If this is a vector-capable CPU, then wait
		 * for the attached vector processor to go idle.
		 * This is a no-op on non-vector-capable VAXen
		 */
                vp_idle();
#endif /* __vax */
		stop_secondary_cpu();		
		/* no return */
	}
	boot(RB_PANIC, bootopt);
}
