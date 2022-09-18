/*
 * @(#)defs.h	4.2	ULTRIX	9/4/90
 */

/************************************************************************
 *			Modification History				*
 *									*
 *	David L Ballenger, 16-May-1985					*
 * 001	Clean up definitions and take them from

/*
 *	defs.h	4.3	82/12/19	
 */
/*
 * adb - vax string table version; common definitions
 */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#include "../machine/psl.h"
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"

#include <ctype.h>
#include <a.out.h>

#include "mac.h"
#include "mode.h"
#include "head.h"
#include "kdb.h"

/* access modes */
#define RD	0
#define WT	1

#define NSP	0
#define	ISP	1
#define	DSP	2
#define STAR	4
#define STARCOM 0200

/*
 * Symbol types, used internally in calls to findsym routine.
 * One the VAX this all degenerates since I & D symbols are indistinct.
 * Basically we get NSYM==0 for `=' command, ISYM==DSYM otherwise.
 */
#define NSYM	0
#define DSYM	1		/* Data space symbol */
#define ISYM	DSYM		/* Instruction space symbol == DSYM on VAX */

#define BKPTSET	1
#define BKPTEXEC 2

#define BPT	03
#define TBIT	020
#define FD	0200
#define	SETTRC	0
#define	RDUSER	2
#define	RIUSER	1
#define	WDUSER	5
#define WIUSER	4
#define	RUREGS	3
#define	WUREGS	6
#define	CONTIN	7
#define	EXIT	8
#define SINGLE	9


/* the quantities involving ctob() are located in the kernel stack. */
/* the others are in the pcb. */

#define K_KSP	&u.u_pcb.pcb_ksp
#define K_ESP	&u.u_pcb.pcb_esp
#define K_SSP	&u.u_pcb.pcb_esp
#define K_USP	&u.u_pcb.pcb_usp
#define K_R0	&u.u_pcb.pcb_r0
#define K_R1	&u.u_pcb.pcb_r1
#define K_R2	&u.u_pcb.pcb_r2
#define K_R3	&u.u_pcb.pcb_r3
#define K_R4	&u.u_pcb.pcb_r4
#define K_R5	&u.u_pcb.pcb_r5
#define K_R6	&u.u_pcb.pcb_r6
#define K_R7	&u.u_pcb.pcb_r7
#define K_R8	&u.u_pcb.pcb_r8
#define K_R9	&u.u_pcb.pcb_r9
#define K_R10	&u.u_pcb.pcb_r10
#define K_R11	&u.u_pcb.pcb_r11
#define K_AP	&u.u_pcb.pcb_ap
#define K_FP	&u.u_pcb.pcb_fp
#define K_PC	&u.u_pcb.pcb_pc
#define K_PSL	&u.u_pcb.pcb_psl
#define K_P0BR	&u.u_pcb.pcb_p0br
#define K_P0LR	&u.u_pcb.pcb_p0lr
#define K_P1BR	&u.u_pcb.pcb_p1br
#define K_P1LR	&u.u_pcb.pcb_p1lr

#define MAXOFF	65536
#define MAXPOS	80
#define MAXLIN	128
#define EOR	'\n'
#define SP	' '
#define TB	'\t'
#define SINGLE_QUOTE '\''
#define DOUBLE_QUOTE '\"'
#define QUOTE	0200
#define STRIP	0177
#define LOBYTE	0377
#define EVEN	-2

/* long to ints and back (puns) */
union {
	INT	I[2];
	L_INT	L;
} 
itolws;

#ifndef __vax
#define leng(a)		((long)((unsigned)(a)))
#define shorten(a)	((int)(a))
#define itol(a,b)	(itolws.I[0]=(a), itolws.I[1]=(b), itolws.L)
#else
#define leng(a)		itol(0,a)
#define shorten(a)	((short)(a))
#define itol(a,b)	(itolws.I[0]=(b), itolws.I[1]=(a), itolws.L)
#endif

/* result type declarations */
L_INT		inkdot();
POS		get();
POS		chkget();
STRING		exform();
L_INT		round();
BKPTR		scanbkpt();
VOID		fault();

struct	pte *kdb_sbr;
int	kdb_slr;
