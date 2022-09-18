#ifndef lint
static char *sccsid = "@(#)sys_trace.c	4.1 	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * trace.c
 *
 * Modification history
 *
 * TRACER system call trace special device
 *
 * 24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 * 19-May-87 - afd
 *	Don't access ICR register if processor doesn't have one (as
 *	determined by CPU_ICR bit in flags field of cpu switch).
 *
 * 11-Jul-86 - ricky palmer
 *
 *	Added new DEVIOCGET ioctl request code.  V2.0
 *
 *  5-Aug-86 - Fred Canter
 *
 *	Changed DEV_NOB to DEV_NB.
 *
 * 26-Aug-86 - rsp (Ricky Palmer)
 *
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry and (2) use strlen instead of fixed storage
 *	for bcopy's.
 *
 */

#ifdef SYS_TRACE
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/devio.h"
#include "../h/tty.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/systrace.h"
#include "../h/file.h"
#include "../h/buf.h"
#include "../h/kernel.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"

struct lock_t lk_systrace;

int nsysent;			/* number of system calls defined */
int tracebsize = TR_BUFSIZE;
int tracelost = 0;
extern char *syscallnames[];	/* from syscalls.c */

extern struct cpusw *cpup;	/* pointer to cpusw entry */

/* trace driver routines */

trace_open(dev,mode)	/* mode must be 1 -- FREAD (read only) */
	dev_t dev;
	register int mode;
{
	register int i;
	register struct trace *trp = &tr_user[0];
	struct buf *bp;

	if (mode != FREAD) return(ENXIO);

	/* do this out side lock... could sleep */ 
	bp = geteblk(tracebsize);

	smp_lock(&lk_systrace,LK_RETRY);
	for (i=0;i<TR_USRS;i++,trp++) if (trp->open == CLOSED) break;
	if (i >= TR_USRS) {
		smp_unlock(&lk_systrace);
		brelse(bp);  /* all full...release buffer we pre allocated */
		return(ENXIO);/* none available */
	}
	trp->bp=bp;
	trp->bp->b_flags &= ~B_BUSY;	/* not busy to start with */
	trp->bp->b_bcount = 0;		/* empty */
	trp->open = OPEN;		/* mark device open */
	trp->traceflag = OFF;		/* tracing nothing */
	trp->uid = u.u_uid;		/* save uid of opener */
	trp->ruid = u.u_ruid;		/* save ruid of opener */
	u.u_tracedev = i;		/* save the slot */
	traceopens++;			/* count the opens */
					/* used in trap.c & close */
	smp_unlock(&lk_systrace);
	return(0);
}

trace_read(dev,uio)
	dev_t dev;
	register struct uio *uio;
{
	register struct buf *bp;
	register int error = 0;
	register struct trace *trp = &tr_user[u.u_tracedev];

	smp_lock(&lk_systrace,LK_RETRY);
	if (trp->open == CLOSED){
		smp_unlock(&lk_systrace);
		return(ENXIO);
	}
	bp = trp->bp;
	while (bp->b_flags & B_BUSY) {
		sleep_unlock(bp,PRIBIO+1,&lk_systrace);
		smp_lock(&lk_systrace,LK_RETRY);
	}
	bp->b_flags |= B_BUSY;
	smp_unlock(&lk_systrace);
	error = uiomove(bp->b_un.b_addr,bp->b_bcount,UIO_READ,uio);
	smp_lock(&lk_systrace,LK_RETRY);
	bp->b_bcount = 0;
	bp->b_flags &= ~B_BUSY;
	smp_unlock(&lk_systrace);
	wakeup(bp);
	return(error);
}

trace_select(dev,flag)
	dev_t dev;
	register int flag;
{
	register struct trace *trp = &tr_user[u.u_tracedev];
	smp_lock(&lk_systrace,LK_RETRY);
	if (flag == FREAD && trp->open == OPEN &&
	    trp->bp->b_bcount > (int) (tracebsize * FRACTION)) {
		/* readable if FRACTION full */
		smp_unlock(&lk_systrace);
		return(1);
	}
	trp->tr_proc = u.u_procp;
	smp_unlock(&lk_systrace);
	return(0);
}


trace_close(dev)
	dev_t dev;
{
	register struct trace *trp = &tr_user[0];
	register int i;


	smp_lock(&lk_systrace,LK_RETRY);
	traceopens = 0;
	for (i=0;i<TR_USRS;i++,trp++) {
		/* release all buffers */
		if (trp->bp) brelse(trp->bp);
		bzero(trp,sizeof(struct trace));
	}

	smp_unlock(&lk_systrace);
	return(0);
}

trace_ioctl(dev, cmd, addr, flag)
	dev_t dev;
	register int cmd;
	register caddr_t addr;
	register int flag;
{
	register struct trace *trp = &tr_user[u.u_tracedev];
	register struct devget *devget;
	int tracetemp;

	smp_lock(&lk_systrace,LK_RETRY);

	if (trp->open == CLOSED){

		smp_unlock(&lk_systrace);
		return(ENXIO);
	}
	switch (cmd) {
	/* start with the gets, ints and then arrays */
	case IOTR_GETOFF:
	case IOTR_GETON:
		tracetemp = trp->traceflag & ON;
		bcopy(&tracetemp,addr,sizeof(int));
		break;
	case IOTR_GETALL:
		tracetemp = trp->traceflag & ALL;
		bcopy(&tracetemp,addr,sizeof(int));
		break;
	case IOTR_GETPIDS:
		bcopy(trp->pids,addr,TR_PIDS*sizeof(int));
		break;
	case IOTR_GETUIDS:
		bcopy(trp->uids,addr,TR_UIDS*sizeof(int));
		break;
	case IOTR_GETSYSC:
		bcopy(trp->sysc,addr,TR_SYSC*sizeof(int));
		break;
	case IOTR_GETPGRP:
		bcopy(trp->pgrp,addr,TR_PGRP*sizeof(int));
		break;
	/* sets go below here, ints then arrays */
	case IOTR_SETOFF:
		trp->traceflag = OFF;
			/* turning it off kills all traceflags*/
		break;
	case IOTR_SETON:
		trp->traceflag |= ON;
		break;
	case IOTR_SETALL:
		trp->traceflag |= ALL;
		break;
	case IOTR_SETPIDS:
		bcopy(addr,trp->pids,TR_PIDS*sizeof(int));
		trp->traceflag |= PIDS;
		break;
	case IOTR_SETUIDS:
		bcopy(addr,trp->uids,TR_UIDS*sizeof(int));
		trp->traceflag |= UIDS;
		break;
	case IOTR_SETSYSC:
		bcopy(addr,trp->sysc,TR_SYSC*sizeof(int));
		trp->traceflag |= SYSC;
		break;
	case IOTR_SETPGRP:
		bcopy(addr,trp->pgrp,TR_PGRP*sizeof(int));
		trp->traceflag |= PGRP;
		break;
	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)addr;
		bzero(devget,sizeof(struct devget));
		devget->category = DEV_SPECIAL; 	/* special trace*/
		devget->bus = DEV_NB;			/* no bus con.	*/
		bcopy(DEV_UNKNOWN,devget->interface,
		      strlen(DEV_UNKNOWN));		/* n/a		*/
		bcopy(DEV_TRACE,devget->device,
		      strlen(DEV_TRACE));		/* trace dev.	*/
		devget->adpt_num = -1;			/* n/a		*/
		devget->nexus_num = -1; 		/* n/a		*/
		devget->bus_num = -1;			/* n/a		*/
		devget->ctlr_num = -1;			/* n/a		*/
		devget->slave_num = -1; 		/* n/a		*/
		bcopy("trace",devget->dev_name,6);	/* Ultrix "trace" */
		devget->unit_num = 0;			/* always 0	*/
		devget->soft_count = 0; 		/* always 0	*/
		devget->hard_count = 0; 		/* always 0	*/
		devget->stat = 0;			/* always 0	*/
		devget->category_stat = 0;		/* always 0	*/
		break;

	default:

	smp_unlock(&lk_systrace);
		return(EINVAL);
	}

	smp_unlock(&lk_systrace);
	return(0);
}

/*
 * macros to append one two or three(tri) bytes to the buffer
 * these are to avoid the call to strapp() which does a "calls" and sets up
 * a while loop for trivial cases of one two of three characters which we
 * do often. Remember to check the buffer for overflow!
 */
#define charapp(c,bp) \
	if(bp->b_bcount<tracebsize) { \
		bp->b_un.b_addr[bp->b_bcount++]=((c)); \
	}

#define oneapp(cp,bp) \
	if(bp->b_bcount<tracebsize) { \
		bp->b_un.b_addr[bp->b_bcount++]=(*(cp)); \
	}

#define twoapp(cp,bp) \
	if(bp->b_bcount<tracebsize-1) { \
		bp->b_un.b_addr[bp->b_bcount++]=(*(cp)); \
		bp->b_un.b_addr[bp->b_bcount++]=(*((cp)+1)); \
	}

#define triapp(cp,bp) \
	if(bp->b_bcount<tracebsize-2) { \
		bp->b_un.b_addr[bp->b_bcount++]=(*(cp)); \
		bp->b_un.b_addr[bp->b_bcount++]=(*((cp)+1)); \
		bp->b_un.b_addr[bp->b_bcount++]=(*((cp)+2)); \
	}

syscall_trace(code, nargs, before)
	unsigned code, nargs, before;
{
	register int i,j;
	register char *cp;
	register struct buf *buffer,*bp;
	struct timeval mytime;
	int pid;

	for (i=0;i<TR_USRS;i++) {
		if (tr_user[i].open == CLOSED) continue;
		if (tr_user[i].traceflag == OFF) continue;
		if (u.u_uid != u.u_ruid) continue;	/* can't be setuid */
		if (u.u_gid != u.u_rgid) continue;	/* can't be setgid */
		if (tr_user[i].uid != u.u_uid) continue; /* only trace yours */
		if (tr_user[i].ruid != u.u_ruid) continue;/* only trace yours */
		if ((tr_user[i].traceflag & ALL) &&
			(tr_user[i].uid == 0 && tr_user[i].ruid == 0)) {
			break;	/* root wants all syscalls */
		}
		if (tr_user[i].uid != 0 && tr_user[i].ruid != 0 &&
		    tr_user[i].uid != u.u_uid && tr_user[i].ruid != u.u_ruid) {
			continue; /* have to be same guy who opened trace */
		}
		/* at this point the user doing the tracing owns the proc or */
		/* is root so all we have to check is whether or not the user */
		/* has requested a subset of the pgrps, sycalls, pids, uids */
		if (tr_user[i].traceflag & PGRP) {	/* pgrp check loop */
			register int pgrp = u.u_procp->p_pgrp,
				   *pgrps = tr_user[i].pgrp;
			for (j=0;j<TR_PGRP;j++) {
				if (pgrps[j] == 0) continue;
				if (pgrp == pgrps[j]) break;	/* want this */
			}
			if (j < TR_PGRP) {
				break;	/* if we broke above, then want it */
			}
		}
		if (tr_user[i].traceflag & PIDS) {	/* pid check loop */
			register int pid = u.u_procp->p_pid,
				   *pids = tr_user[i].pids;
			for (j=0;j<TR_PIDS;j++) {
				if (pids[j] == 0) continue;
				if (pid == pids[j]) break;	/* want this */
			}
			if (j < TR_PIDS) {
				break;	/* if we broke above, then want it */
			}
		}
		if (tr_user[i].traceflag & SYSC) {	/* syscall check loop */
			register int *sysc = tr_user[i].sysc;
			for (j=0;j<TR_SYSC;j++) {
				if (sysc[j] == 0) continue;
				if (code == sysc[j]) break;	/* want this */
			}
			if (j < TR_SYSC) {
				break;	/* if we broke above, then want it */
			}
		}
		if (tr_user[i].traceflag & UIDS) {	/* uid check loop */
			register int uid = u.u_procp->p_uid,
				   *uids = tr_user[i].uids;
			for (j=0;j<TR_UIDS;j++) {
				if (uids[j] == 0) continue;
				if (uid == uids[j]) break;	/* want this */
			}
			if (j < TR_UIDS) {
				break;	/* if we broke above, then want it */
			}
		}
		continue;
	}
	/* if we fall through and i was exhausted we don't want this one */
	if (i >= TR_USRS) return;	/* not tracing this call */
	/* if we have broken out this far someone wants it */
	if (!before && code == 59 && u.u_error == 0)  {
		return; 	/* don't log successful execve - 59 */
	}
	buffer = geteblk(tracebsize);
	buffer->b_bcount = 0;
	/* output time */
	mytime = time;	/* have to get your own copy since it can change */
	mytime.tv_sec &= 255;	/* small value instead of 10 digits */
#ifdef vax
	if ((cpup->flags & CPU_ICR) != 0)
		mytime.tv_usec += mfpr(ICR) + 1000000/hz;
#endif vax
	if (mytime.tv_usec >= 1000000) {
		mytime.tv_usec -= 1000000;
		mytime.tv_sec++;
	}
	if (mytime.tv_sec == 0) {
		triapp("000",buffer);
	}
	else {
		if (mytime.tv_sec < 10) {
			twoapp("00",buffer);
		}
		else if (mytime.tv_sec < 100) {
			oneapp("0",buffer);
		}
		decout(mytime.tv_sec,buffer);
	}
	oneapp(".",buffer);
	if (mytime.tv_usec == 0) {
		triapp("000",buffer);
		triapp("000",buffer);
	}
	else {
		if (mytime.tv_usec < 10) {
			triapp("000",buffer);
			twoapp("00",buffer);
		}
		else if (mytime.tv_usec < 100) {
			twoapp("00",buffer);
			twoapp("00",buffer);
		}
		else if (mytime.tv_usec < 1000) {
			triapp("000",buffer);
		}
		else if (mytime.tv_usec < 10000) {
			twoapp("00",buffer);
		}
		else if (mytime.tv_usec < 100000) {
			oneapp("0",buffer);
		}
		decout(mytime.tv_usec,buffer);
	}
	oneapp(" ",buffer);
	pid = u.u_procp->p_pid;
	if (pid < 10) oneapp(" ",buffer);
	if (pid < 100) oneapp(" ",buffer);
	if (pid < 1000) oneapp(" ",buffer);
	if (pid < 10000) oneapp(" ",buffer);
	decout(pid,buffer);
	if (before) {
		triapp(" C ",buffer);
	}
	else {
		triapp(" R ",buffer);
	}
	if (code >= nsysent) {
		triapp("Und",buffer);
		triapp("efi",buffer);
		triapp("ned",buffer);
		oneapp(" ",buffer);
		if (code < 10) oneapp(" ",buffer);
		if (code < 100) oneapp(" ",buffer);
		decout(code,buffer);
	}
	else {
		strapp(syscallnames[code],buffer); /* only call to strapp */
	}
	oneapp(" ",buffer);
	if (before) {
		cp = "( ";			/* must be two chars */
		for (i=0; i < nargs; i++) {
			twoapp(cp,buffer);	/* add two chars to buf */
			dumparg(code,i,buffer);
			cp = ", ";		/* must be two chars */
		}
		if (i) twoapp(" )",buffer);
	}
	else {	/* not before -- after */
	/*
	 * On exit output the arguments based on the system call or the
	 * value of u.u_error if nonzero
	 */
		if (u.u_error) {
			triapp("Err",buffer);
			triapp("or ",buffer);
			decout(u.u_error,buffer);
		}
		else {
			switch (code) {
			case 87:	/* gethostname */
				if (u.u_arg[1] > 0) strout(0,0,buffer);
				break;
			case 3: 	/* read */
			case 58:	/* readlink */
			case 102:	/* recv */
			case 125:	/* recvfrom */
			case 164:	/* getdirentries */
				if (u.u_r.r_val1 > 0) {
					decout(u.u_r.r_val1,buffer);
					triapp(" by",buffer);
					triapp("tes",buffer);
					twoapp(": ",buffer);
					strout(1,u.u_r.r_val1,buffer);
				}
				else numout(u.u_r.r_val1,buffer);
				break;
			default:
				numout(u.u_r.r_val1,buffer);
			}
		}
	}
	oneapp("\n",buffer);
	for (i=0;i<TR_USRS;i++) {
		if (tr_user[i].open == CLOSED) continue;
		if (tr_user[i].traceflag == OFF) continue;
		if (u.u_uid != u.u_ruid) continue;	/* can't trace setuid */
		if (u.u_gid != u.u_rgid) continue;	/* can't trace setgid */
		if (tr_user[i].uid != u.u_uid) continue; /* only trace yours */
		if (tr_user[i].ruid != u.u_ruid) continue;/* only trace yours */
		if ((tr_user[i].traceflag & ALL) &&
			(tr_user[i].uid == 0 && tr_user[i].ruid == 0)) {
			goto add;
		}
		if (tr_user[i].uid != 0 && tr_user[i].ruid != 0 &&
		    tr_user[i].uid != u.u_uid && tr_user[i].ruid != u.u_ruid) {
			continue;
		}
		if (tr_user[i].traceflag & PGRP) {	/* pgrp check loop */
			register int pgrp = u.u_procp->p_pgrp,
				   *pgrps = tr_user[i].pgrp;
			for (j=0;j<TR_PGRP;j++) {
				if (pgrps[j] == 0) continue;
				if (pgrp == pgrps[j]) goto add; /* want it */
			}
		}
		if (tr_user[i].traceflag & PIDS) {	/* pid check loop */
			register int pid = u.u_procp->p_pid,
				   *pids = tr_user[i].pids;
			for (j=0;j<TR_PIDS;j++) {
				if (pids[j] == 0) continue;
				if (pid == pids[j]) goto add;	/* want it */
			}
		}
		if (tr_user[i].traceflag & SYSC) {	/* syscall check loop */
			register int *sysc = tr_user[i].sysc;
			for (j=0;j<TR_SYSC;j++) {
				if (sysc[j] == 0) continue;
				if (code == sysc[j]) goto add;	/* want this */
			}
		}
		if (tr_user[i].traceflag & UIDS) {	/* uid check loop */
			register int uid = u.u_procp->p_uid,
				   *uids = tr_user[i].uids;
			for (j=0;j<TR_UIDS;j++) {
				if (uids[j] == 0) continue;
				if (uid == uids[j]) goto add;	/* want this */
			}
		}
		continue;
add:
		bp = tr_user[i].bp;
		smp_lock(&lk_systrace,LK_RETRY);
		while (bp->b_flags & B_BUSY) {
			sleep_unlock(bp,PRIBIO+1,&lk_systrace);
			smp_lock(&lk_systrace,LK_RETRY);
		}
		bp->b_flags |= B_BUSY;
		if (tracebsize - bp->b_bcount > buffer->b_bcount) {
		   bcopy(buffer->b_un.b_addr,bp->b_un.b_addr+bp->b_bcount,
			 buffer->b_bcount);
		   bp->b_bcount += buffer->b_bcount;
		}
		else {
			tracelost++;	/* lost this record due to filled buf */
			/* one thing we can do is make tracebsize bigger */
		}
		bp->b_flags &= ~B_BUSY;
		smp_unlock(&lk_systrace);
		wakeup(bp);
		/* see if poor guy did a select
		 * and if buffer is full enough
		 * wake him up
		 */
		if (tr_user[i].tr_proc) {
			if (tr_user[i].bp->b_bcount >
					(int) (tracebsize * FRACTION))
				selwakeup(tr_user[i].tr_proc,0);
		}
	}
	/* done with buffer - release it */
	brelse(buffer);
}

/*
 * In special cases dump out the character string passed to
 * the system call, otherwise use the heuristic routine numout.
 */
dumparg(code,argno,buffer)
	unsigned code, argno;
	struct buf *buffer;
{
	switch (code) {
	case 33:	/* access */
	case 51:	/* acct */
	case 12:	/* chdir */
	case 15:	/* chmod */
	case 16:	/* chown */
	case 61:	/* chroot */
	case 8: 	/* creat */
	case 59:	/* execve */
	case 40:	/* lstat */
	case 136:	/* mkdir */
	case 14:	/* mknod */
	case 5: 	/* open */
	case 58:	/* readlink */
	case 137:	/* rmdir */
	case 18:	/* stat */
	case 85:	/* swapon */
	case 129:	/* truncate */
	case 22:	/* umount */
	case 10:	/* unlink */
	case 138:	/* utimes */
		if (argno == 0) {
			strout(argno,0,buffer);
			return;
		}
		break;
	case 9: 	/* link */
	case 21:	/* mount */
	case 128:	/* rename */
	case 148:	/* setquota */
	case 57:	/* symlink */
		if (argno < 2) {
			strout(argno,0,buffer);
			return;
		}
		break;
	case 101:	/* send */
	case 133:	/* sendto */
	case 4: 	/* write */
		if (argno == 1) {
			strout(argno,u.u_arg[2],buffer);
			return;
		}
		break;
	case 54:	/* ioctl, output second argument in hex only */
		if (argno == 1) {
			hexout(u.u_arg[argno],buffer);
			return;
		}
		break;
	}
	/* otherwise use rules of thumb to format the argument */
	numout(u.u_arg[argno],buffer);
}

/*
 * Copy a string into the trace buffer.  The address of the first
 * byte is taken from u.u_arg[argno].
 * If the maximum length is given as zero then copying will
 * terminate when a \0 is reached.
 * If the maximum length is given as nonzero then the number
 * of bytes copied will be the minimum of maxlen and DUMPSIZE - 1.
 * In all cases, copying will stop if a memory fault is
 * encountered. (More than that stops!)
 */
strout(argno,maxlen,buffer)
	unsigned argno;
	register int maxlen;
	register struct buf *buffer;
{
	register caddr_t ptr;
	register int s,c;
#define DUMPSIZE 32	/* first 32 bytes of read, or whatever */

	ptr = (caddr_t)u.u_arg[argno];
	if (maxlen == 0 || maxlen > DUMPSIZE - 1)
		s = DUMPSIZE - 1;
	else
		s = maxlen;
	oneapp("\"",buffer);
	do {
		c = fubyte(ptr++);
		if (c == 0 && maxlen == 0 || c == -1) break;
		/* change newlines and tabs to blanks, all other ctrl to ? */
		if (c == '\t') c = ' ';
		if (c == '\n') c = ' ';
		if (c < ' ' || c > 127) c = '?';
		/* put char c in buffer */
		charapp(c,buffer);
		s--;
	} while (s > 0);
	oneapp("\"",buffer);
}

/*
 * append the number num to the input string.  Conditionally output
 * decimal, hex or both representations based on the value of num.
 * -1..DECLIMIT 			-> decimal only
 * DECLIMIT..BOTHLIMIT OR < -1		-> decimal and hex
 * > BOTHLIMIT				-> hex only
 */
numout(num,buffer)
	register int	num;
	register struct buf *buffer;
{
#define DECLIMIT	100
#define BOTHLIMIT	50000
	register int n;

	n = num;
	if (n < 0)
		n = -n;
	if (n <= BOTHLIMIT || num < -1)
		decout(num,buffer);
	if (n > DECLIMIT && n <= BOTHLIMIT || num < -1)
		oneapp("/",buffer);
	if (n > DECLIMIT || num < -1)
		hexout(num,buffer);
}

/* dump the number in hex */
hexout(n,buffer)
	register unsigned n;
	register struct buf *buffer;
{
	register int d;
	char tmp[20];
	register char *tptr = tmp, *tmpptr = tmp;

	twoapp("0x",buffer);
	do {
		d = n & 15;
		*tptr++ = d + (d<10 ? '0' : 'a' - 10);
		n >>= 4;
	} while (n != 0);
	/* back out the string since it was calculated in reverse */
	for (tptr--;tptr >= tmpptr;tptr--) {
		oneapp(tptr,buffer);
	}
}

/* dump the number in decimal */
decout(n,buffer)
	register int n;
	register struct buf *buffer;
{
	register int d;
	char tmp[20];
	register char *tptr = tmp, *tmpptr = tmp;

	if (n == 0) {
		oneapp("0",buffer);
		return;
	}
	if (n<0) {
		n = -n;
		oneapp("-",buffer);
	}
	while (n != 0) {
		d = n % 10;
		*tptr++ = d + '0';
		n /= 10;
	}
	/* back out the string since it was calculated in reverse */
	for(tptr--;tptr >= tmpptr;tptr--) {
		oneapp(tptr,buffer);
	}
}

/*
 * append the string str to the trace buffer, stop when the trace buffer is full
 * or the end of str is reached. Only used when don't know length!
 */
strapp(str,buffer)
	char	*str;
	register struct buf *buffer;
{
	register char *ptr = str;
	while (*ptr && buffer->b_bcount < tracebsize) {
		buffer->b_un.b_addr[buffer->b_bcount++] = *ptr++;
	}
}
#endif SYS_TRACE
