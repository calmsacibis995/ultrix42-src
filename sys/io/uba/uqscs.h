/*
 *	@(#)uqscs.h	2.5	(ULTRIX)	10/12/89
 */

/************************************************************************
 *									*
 *			Copyright (c) 1987 - 1989 by			*
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
 *	Modification History
 *
 *	07-May-1990  - Matthew S.  Add the declarations of init_leader
 *		and reset_leader, and UQPath_Is_Up.  These are used for
 *		smp-safeness.  UQPath_Is_Up is a constant defined to be
 *		32.  32 is used because a Cpu processor number cannot 
 *		ever be 32.  (0 <= Cpu_Id <= 31).  When the port is up
 *		and connected, both init_leader and reset_leader equal
 *		UQPath_Is_Up.  When a port is down, or the decision is
 *		otherwise made to re-init the port, the CPUs contend in
 *		an smp-safe manner, for the position of init_leader.
 *		The cpu that wins the smp contention sets init_leader
 *		to the cpu_num value of itself.  This is the cpu
 *		that will actually start the init.  When the four
 *		step init process is complete, init_leader is set back
 *		to UQPath_Is_Up.  The reset routine, uq_reset, is comprised 
 *		of initiating the four-step init, rebuilding the simulated
 *		SCS layer, and remapping the buffers.  In uq_reset, the cpus 
 *		contend for the position of reset leader, the winner sets the
 *		reset_leader to itself, the current cpu_num, sets flags that
 *		indicate to rebuild the SCS layer and re-map the buffers.
 *		uq_reset then goes to pick an init_leader in the described
 *		way.  After this is done, reset_leader is set back to
 *		UQPath_Is_Up.
 *
 *	20-July-1989 - map (Mark A. Parenti)
 *	        Add regptrs structure for use in accessing device registers.
 *		Make pccb changes to regptrs structure.
 *
 *	18-July-1988 - map
 *		Dynamically allocate data structures.
 *
 *	15-Feb-1988 -- map
 *		Removed pointers to SCS buffers.
 *		Add scswaitq for use in connection handshakes.
 */

#define	NRSPL2	4		/* log2 number of response packets	*/
#define	NCMDL2	4		/* log2 number of command packets	*/
#define	NRSP	(1<<NRSPL2)
#define	NCMD	(1<<NCMDL2)
#define NBUF	NRSP + NCMD + 8 + 1
#define UQPath_Is_Up  32	/* This must be a value that cannot */
				/* be a cpu number */


/*
 * UQSSP device registers and structures
 */

typedef struct _uqregptrs {

	volatile unsigned short	*uqip;	/* Initialization and Polling Register*/
	volatile unsigned short	*uqsa;	/* Status, Address and Purge Register*/
	volatile unsigned short *uqsaw; /* Write SA - VAXBI Only	     */
	volatile unsigned long	*uqpd;  /* Port Data Register - XMI Only     */
} UQREGPTRS;


typedef struct _uqpccb {	/* UQ Port Command and Control Block 	*/
	struct	_uq	*uqptr;	/* Mapped address of uq structure	*/
	struct	_uq	*uq;	/* Unmapped address uq structure	*/
	uqbq	*uq_freel;	/* Free buffer list	 		*/
	uqbq	waitq;		/* command wait queue			*/
	uqbq	scswaitq;	/* scs wait queue			*/
	struct  _pb  *pb;	/* PB associated with this port		*/
	struct  _uqh *rspbtab[NRSP]; /* Physical address of buffer in ring*/
				/* entry				*/
	struct	_uqh  *cmdbtab[NCMD];	/* ditto			*/
	struct	_connid	contab[NCON]; /* SCS connection id mapping	*/
	struct	_uqh *lfptr[NCON]; /* Ptr to last fail packet 		*/
	struct	_uqregptrs uqregptrs; /* Ptr to IO regs		*/
	int	uqregsize;	/* Size of register space		*/
	struct	_uqscp *uqscp; /* Pointer to ssp scratchpad area	*/
	int	ncon;		/* Current connection count		*/
	int	uq_ctlr;	/* Controller number			*/
	short	uq_con;		/* Connection mask			*/
	short	uq_mapped;	/* UNIBUS map allocated for uq structure? */
	short	map_requests;	/* Number of outstanding mapping requests */
	short	reinit_cnt;	/* Number of reinit tries remaining	*/
	short	rip;		/* Recovery in progress flag		*/
	int	uq_ubainfo;	/* Unibus mapping info 			*/
	int	uq_ivec;	/* interrupt vector address 		*/
	short	uq_lastcmd;	/* pointer into command ring 		*/
	short	uq_lastrsp;	/* pointer into response ring 		*/
	short	rsprindx;	/* index into response ring		*/
	short	rsp_cnt;	/* count of used response ring entries	*/
	short	cmdrindx;	/* index into command ring		*/
	short	cmd_cnt;	/* count of used command ring entries	*/
	int	poll_rate;	/* Time between polls of SA register	*/
	short	step1r;		/* step 1 read data 			*/
	short	step1w;		/* step 1 write data 			*/
	short	step2r;		/* step 2 read data 			*/
	short	step2w;		/* step 2 write data 			*/
	short	step3r;		/* step 3 read data 			*/
	short	step3w;		/* step 3 write data 			*/
	short	step4r;		/* step 4 read data 			*/
	short	step4w;		/* step 4 write data 			*/
	u_long	xmipd;		/* Interrupt info for xmi uqpd register	*/
	u_long	bda_init_vec;	/* vector written by uq_port_reset after
					BI Start Self Test */
	u_long	bda_init_dest;	/* BI dest value written by
					uq_port_reset after BI SST */
	u_long	bda_init_errvec; /* BI errveca written by uq_port_reset
					after BI SST */
	short	init_leader;	/*  32, or the cpu number of the
					processor who is currently 
					initing the port */
	short	reset_leader;	/*  32, or the cpu number of the
					processor who is currently
					resetting the response ring,
					a reset, in this context, is
					an init plus buffer mapping */
} UQPCCB;




