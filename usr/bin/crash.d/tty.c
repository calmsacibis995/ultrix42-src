#ifndef lint
static char *sccsid = "@(#)tty.c	4.1	(ULTRIX)	7/17/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988-1989 by			*
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

#include	"crash.h"
#include	<sys/proc.h>
#include	<machine/pte.h>
#include	<sys/cpudata.h>
#include	<sys/vm.h>
#include	<frame.h>
#include	<stdio.h>
#include 	<strings.h>
#include 	<sys/ttychars.h>
#include 	<sys/ttydev.h>
#include 	<sys/ttyio.h>
#include 	<sys/tty.h>
#include	<sys/clist.h>
#include	<sys/termio.h>
#include	<sys/termios.h>

static char cb[10];

char *
getcharrep(c)
	unsigned char c;
{
	char *t = cb;
	if(c > 0177) {
		c &= 0x7f;
		*t++ = 'M';
		*t++ = '-';
	}
	if(c < 033) {
		*t++ = '^';
		*t++ = c + '@';
		*t = '\0';
		return(cb);
	}
	if(c < ' ') {
		char *str;
		switch(c) {
			case 033 :
				str = "esc";
				break;
			case 034 :
				str = "fs";
				break;
			case 035 :
				str = "gs";
				break;
			case 036 :
				str = "rs";
				break;
			case 037 :
				str = "rs";
			
		}
		strcpy(t, str);
		return(cb);
	}
	if(c == 0177) {
		*t++ = 'd';
		*t++ = 'e';
		*t++ = 'l';
		*t = '\0';
		return(cb);
	}
	*t++ = c;
	*t = '\0';
	return(cb);
}

char *
getspeed(speed)
	int speed;
{
	switch(speed) {
		case B0 : return("0");
		case B50 : return("50");
		case B75 : return("75");
		case B110 : return("110");
		case B134 : return("134");
		case B150 : return("150");
		case B200 : return("200");
		case B300 : return("300");
		case B600 : return("600");
		case B1200 : return("1200");
		case B1800 : return("1800");
		case B2400 : return("2400");
		case B4800 : return("4800");
		case B9600 : return("9600");
		case EXTA : return("EXTA");
		case EXTB : return("EXTB");
	}
	return("???");
}

extern struct uarea u;
static char *flags[] = {
	"tandem",	"cbreak", 	"lcase",	"echo",
	"crmod",	"raw",		"oddp",		"evenp",
	"nl1",		"nl2",		"tab1",		"tab2",
	"cr1",		"cr2",		"vtdelay",	"bs1",
	"crtbs",	"prtera",	"crtera",       "tilde",
	"mdmbuf",	"litout",	"tostop",       "flusho",
	"nohang",	"autoflow",	"crtkil",       "pass8",
	"ctlech",	"pendin",	"decctq",       "bnoflsh"
};

static char *state[] = {
	"timeout",	"wopen",	"isopen",	"flush",
	"carr_on",	"busy",		"asleep",	"xclude",
	"ttstop",	"tblock",	"rcoll",	"wcoll",
	"nbio",		"async",	"ondelay",	"bksl",
	"quot",		"erase",	"lnch",		"typen",
	"cnttb",	"igncar",	"closing",	"inuse",
	"lrto",		"ltact",	"isusp",	"oabort",
	"onoctty"
};


prtty(do_clist, slot)
	int do_clist;
	int slot;
{
	struct proc *p;

	p = &proctab[slot];
	if(p->p_ttyp == NULL) {
		printf("process %d not attached to tty\n", p->p_pid);
		return(-1);
	}
	print_tty(p->p_ttyp, do_clist);
}

print_tty(tty_ptr, do_clist)
	struct tty *tty_ptr;
	int do_clist;
{
	struct tty tty;
	int i;
	int numcnt;
	int done = 0;
	char *malloc();
	struct clist *cl;
	struct cblock *cb;
	struct cblock *vcb;
	char *cp;

	if(readmem((char *) &tty, (int)tty_ptr, sizeof(struct tty)) !=
	    sizeof(struct tty)) {
		printf("could not get tty struct at 0x%x\n",tty_ptr);
		return(-1);
	}
	printf("\nTTY Structure: address 0x%x \(",tty_ptr);
	praddr(tty_ptr);
	printf("\)\n");
	printf("dev 0x%x pgrp %d ispeed %s ospeed %s\n",
	    tty.t_dev, tty.t_pgrp, getspeed(tty.t_cflag & CBAUD),
	    getspeed(tty.t_cflag_ext & CBAUD));
	printf("flags: \(0x%x\) ",tty.t_flags);
	numcnt = 0;
	/* only print bit values */
	for(i = 0; i < (sizeof (flags) / sizeof (char *)); i++) {
		if(tty.t_flags & (1 << i)) {
			if((numcnt++ % 5) == 0)
				printf("\n\t");
			printf("%s\t", flags[i]);
		}
	}
	if(--numcnt % 5)
		printf("\n");
		
	printf("state: \(0x%x\) ",tty.t_state);
	numcnt = 0;
	/* only print bit values */
	for(i = 0; i < (sizeof (flags) / sizeof (char *)); i++) {
		if(tty.t_state & (1 << i)) {
			if((numcnt++ % 5) == 0)
				printf("\n\t");
			printf("%s\t", state[i]);
		}
	}
	if(--numcnt % 5)
		printf("\n");
		
	printf("Control characters: \n");
	printf("\tintr '%s' ", getcharrep((u_char)tty.t_cc[VINTR]));
	printf("\tquit '%s' ", getcharrep((u_char)tty.t_cc[VQUIT]));
	printf("\terase '%s' ", getcharrep((u_char)tty.t_cc[VERASE]));
	printf("\tkill '%s' ", getcharrep((u_char)tty.t_cc[VKILL]));
	printf("\n");
	printf("\teof '%s' ", getcharrep((u_char)tty.t_cc[VEOF]));
	printf("\teol '%s' ", getcharrep((u_char)tty.t_cc[VEOL]));
	printf("\teol2 '%s' ", getcharrep((u_char)tty.t_cc[VEOL2]));
	printf("\tswtch '%s' ", getcharrep((u_char)tty.t_cc[VSWTCH]));
	printf("\n");
	printf("\tmin  %d  ", tty.t_cc[VMIN]);
	printf("\ttime  %d  ", tty.t_cc[VTIME]);
	printf("\tstart '%s' ", getcharrep((u_char)tty.t_cc[VSTART]));
	printf("\tstop '%s' ", getcharrep((u_char)tty.t_cc[VSTOP]));
	printf("\n");
	printf("\tsusp '%s' ", getcharrep((u_char)tty.t_cc[VSUSP]));
	printf("\tdsusp '%s' ", getcharrep((u_char)tty.t_cc[VDSUSP]));
	printf("\trprnt '%s' ", getcharrep((u_char)tty.t_cc[VRPRNT]));
	printf("\tflush '%s' ", getcharrep((u_char)tty.t_cc[VFLUSH]));
	printf("\n");
	printf("\twerase '%s' ", getcharrep((u_char)tty.t_cc[VWERASE]));
	printf("\tlnext '%s' ", getcharrep((u_char)tty.t_cc[VLNEXT]));
	printf("\tquote '%s' ", getcharrep((u_char)tty.t_cc[VQUOTE]));
	printf("\n");
	
	printf("t_oproc: \(0x%x\) ",tty.t_oproc);
	praddr(tty.t_oproc);
	printf("\n");
	printf("t_poststart: \(0x%x\) ",tty.t_poststart);
	praddr(tty.t_poststart);
	printf("\n");
	printf("rows %d cols %d xpix %d ypix %d\n",
	    tty.t_winsize.ws_row, tty.t_winsize.ws_col,
	    tty.t_winsize.ws_xpixel, tty.t_winsize.ws_ypixel);
	printf("t_rsel: 0x%x, t_wsel: 0x%x, ",
		tty.t_rsel, tty.t_wsel);
	printf("T_LINEP: 0x%x, t_addr: 0x%x\n", tty.T_LINEP, tty.t_addr);
	printf("t_iflag: 0x%x, t_iflag_ext: 0x%x\n",
		tty.t_iflag, tty.t_iflag_ext);
	printf("t_oflag: 0x%x, t_oflag_ext: 0x%x\n",
		tty.t_oflag, tty.t_oflag_ext);
	printf("t_cflag: 0x%x, t_cflag_ext: 0x%x\n",
		tty.t_cflag, tty.t_cflag_ext);
	printf("t_lflag: 0x%x, t_lflag_ext: 0x%x\n",
		tty.t_lflag, tty.t_lflag_ext);
	printf("t_delct: %d, t_line: %d, ", tty.t_delct, tty.t_line);
	printf("t_col: %d, t_rocount: %d\n", tty.t_col, tty.t_rocount);
	printf("t_rocol: %d, t_language: 0x%x, ", tty.t_rocol, tty.t_language);
	printf("t_smp: 0x%x, t_lk_tty: 0x%x\n",tty.t_smp, tty.t_lk_tty);
	printf("t_sid: 0x%x, t_tpath: 0x%x\n",tty.t_sid, tty.t_tpath);
	if(!do_clist)
		return(0);
	if((cb = (struct cblock *)malloc(sizeof (struct cblock))) == NULL) {
		printf("can't malloc space!\n");
		return(-1);
	}
	cp = (char *) cb;
	
	cl = &tty.t_nu.t_t.T_rawq;
	numcnt = cl->c_cc;
	if(numcnt) {
		int count = 1;
		printf("\ninput queue:\n\traw\n\t");
		done++;
		vcb = (struct cblock *) ((int)cl->c_cf & ~CROUND);
		i = (int)cl->c_cf - (int)vcb;
		while(vcb != NULL) {
			if(readmem((char *)cb, (int)vcb,
			    sizeof(struct cblock)) != sizeof(struct cblock)) {
				printf("can't read raw cblock at 0x%x\n",
				    vcb);    
				free((char *)cb);
				return(-1);
			}
			for(;i < CBSIZE; i++) {
				printf("'%s'   ", getcharrep((u_char)cp[i]));
				if((++count % 10) == 0)
					printf("\n\t");
				if(--numcnt == 0)
					goto out0;
			}
			i = sizeof(int);
			vcb = cb->c_next;
		}
	}
out0:
	cl = &tty.t_nu.t_t.T_canq;
	numcnt = cl->c_cc;
	if(numcnt) {
		int count = 1;
		if(done == 0)
			printf("\ninput queue:\n\t");
		printf("cannonical:\n\t");
		vcb = (struct cblock *) ((int)cl->c_cf & ~CROUND);
		i = (int)cl->c_cf - (int)vcb;
		while(vcb != NULL) {
			if(readmem((char *)cb, (int)vcb,
			    sizeof(struct cblock)) != sizeof(struct cblock)) {
				printf("can't read raw cblock at 0x%x\n",
				    vcb);    
				free((char *)cb);
				return(-1);
			}
			for(;i < CBSIZE; i++) {
				printf("'%s'   ", getcharrep((u_char)cp[i]));
				if((++count % 10) == 0)
					printf("\n\t");
				if(--numcnt == 0)
					goto out1;
			}
			i = sizeof(int);
			vcb = cb->c_next;
		}
	}
out1:
	cl = &tty.t_outq;
	numcnt = cl->c_cc;
	if(numcnt) {
		int count = 1;
		printf("\noutput queue:\n\t");
		vcb = (struct cblock *) ((int)cl->c_cf & ~CROUND);
		i = (int)cl->c_cf - (int)vcb;
		while(vcb != NULL) {
			if(readmem((char *)cb, (int)vcb,
			    sizeof(struct cblock)) != sizeof(struct cblock)) {
				printf("can't read output cblock at 0x%x\n",
				    vcb);    
				free((char *)cb);
				return(-1);
			}
			for(;i < CBSIZE; i++) {
				printf("'%s'   ", getcharrep((u_char)cp[i]));
				if((++count % 10) == 0)
					printf("\n\t");
				if(--numcnt == 0)
					goto out2;
			}
			i = sizeof(int);
			vcb = cb->c_next;
		}
	}
out2:
	printf("\n");
	free(cb);	
	return(0);
}
