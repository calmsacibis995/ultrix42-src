#ifndef lint
static	char	*sccsid = "@(#)kdb_misc.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 *	UNIX debugger
 */

#include "mac.h"
#include "mode.h"
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/uio.h"
#include "../h/errno.h"
char kdb_redzone[1025] = "junk";
char kdb_stack[8192] = "a";
char * kdb_stack_ptr = &kdb_stack[8188];
int kdb_saved_ipl;	/* don't use locals if stack changes!!*/
int *kdb_regs_ptr;
int *kdb_pc_ptr;
int *kdb_psl_ptr;
int in_kdb = 0;
int kdb_bkpt_type;
char *saved_stack_ptr;
int enter_kdb_now = 0;
int kdb_req_cpu,kdb_intr_req,kdb_slavehold; 
/*
 * return a pointer to space for a breakpoint
 * return -1 if no more are available.
 */
#define BSZ	16
/* static allocation */
struct {
	int alloc;
	BKPT breakpoint_space;
} kdb_breakpoint_array[BSZ];

BKPTR
get_bkpt_space()
{
	register int i;

	for ( i = 0 ; i < BSZ ; i++ ) {
		if (kdb_breakpoint_array[i].alloc == 0) {
			kdb_breakpoint_array[i].alloc = 1;
			return(&kdb_breakpoint_array[i].breakpoint_space);
		}
	}
	cprintf("maximum number of breakpoint is %d\n", BSZ);
	return((BKPTR)-1);
}

release_bkpt_space(bkptr)
BKPTR bkptr;
{
	register int i;
	for ( i = 0 ; i < BSZ ; i++)
		if (&kdb_breakpoint_array[i].breakpoint_space == bkptr) {
			kdb_breakpoint_array[i].alloc = 0;
			return;
		}
	cprintf("not releasing an illegal breakpoint %x\n", bkptr);
}

/*ARGSUSED*/
kdb_read(d, buf, nbytes)
char *buf;
{
	register int i;
	int v;
	for (i = 1 ; i <= nbytes ; i++) {
		if ((v = kdb_get_one_char()) == -1)
			return(i-1);
		if ((*buf = (char)v) == '\0')
			return(i);
		buf++;
	}
	return(nbytes);
}

/*
 * Emulation of cooked mode input.
 */
#define KDB_INPUT_BUFSIZE 128
char kdb_input_buf[KDB_INPUT_BUFSIZE];
char *kdb_input_cp = 0;
int kdb_input_crt = 1;  /* true if console is a crt */

int kdb_input_debug = 0;
kdb_get_one_char()
{
	int c;

	if (!kdb_input_cp) {
		kdb_filbuf(kdb_input_buf, KDB_INPUT_BUFSIZE);
		kdb_input_cp = kdb_input_buf;
	}
	c = 0177 & *kdb_input_cp++;
	if (*kdb_input_cp == '\0')
		kdb_input_cp = 0;  /* eol - invalidate char pointer */

	return c;
}

/* canonicalize input */
kdb_filbuf(cp, max)
	char *cp;
{
	register char *lp;
	register c;

	lp = cp;
	for (;;) {
		c = getchar() & 0177;
		switch (c) {
		case 0021:	/* ignore XON & XOFF */
		case 0023:
			continue;
		case '\n':
		case '\r':		/*CRMOD*/
			*lp++ = '\n';
			*lp++ = '\0';
			return;
		case '\177':
		case '\b':
		case '#':
			lp--;
			if (lp < cp) {
				lp = cp;
			}
			else {
				if (kdb_input_crt) {	/*CRTBS*/
					cnputc('\b');
					cnputc(' ');
					cnputc('\b');
				}
				else {
					cnputc('\\');	/*PRTBS*/
					cnputc(*(lp+1));
				}
			}
			continue;
		case '@':
		case 'u'&037:
		case 'x'&037:
			if (kdb_input_crt) {	/*CRTERASE*/
				register char *dp = cp;
				cnputc('\r');
				while (dp++ <= lp)
					cnputc(' ');
				cnputc('\r');
			}
			else {
				cnputc('@');	/*PRTERASE*/
				cnputc('\n');
			}
			lp = cp;
			continue;
		default:
			*lp++ = c;
		}
	}
}

kdb_longjmp(l)
struct state *l;
{
	register int rr11;
	register int rr10;
	register int rr9;
	register int rr8;

	rr11 = l->kdb_sp;
	rr10 = l->kdb_ap;
	rr9 = l->kdb_fp;
	rr8 = l->kdb_pc;

	asm("movl r11,sp");
	asm("movl r10,ap");
	asm("movl r9,fp");
	asm("jmp (r8)");
}

static char km[]="No kdb:  kdbload was not run over the vmunix image";

verify_kdb()
{
	if (strcmp(kdb_stack,"kdbloaded") == 0)
		return(1);
	else	{
		cprintf("%s\n", km);
		return(0);
	}
}
