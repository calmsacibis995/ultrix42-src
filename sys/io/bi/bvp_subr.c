#ifndef lint
static char *sccsid = "@(#)bvp_subr.c	4.5	(ULTRIX)	11/13/90";
#endif lint
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
 *   Modification History
 *
 *   13-Nov-1990	Pete Keilty
 *	Change all instances of PF_ERROR to PF_PORTERROR because of a
 *	fix to SCS resource deallocation.
 *
 *   25-Sep-1990	JAW
 *	initial wait value of zero is incorrect for routine bvp_wait_prt.  
 *	According to the spec, -1 is the proper (non-legal) value.  Also
 *	found some missing "{}" in loop in the same routine.
 *	
 *   19-Sep-1989	Pete Keilty
 *	Added pccb to macro Pd_to_ppd.
 *
 *   23-Aug-1989	Pete Keilty
 *	Change wbflush() to macro define in scamachmac.h WBFLUSH.
 *
 *   25-May-1989	Pete Keilty
 *	Add wbflush() for mips cpu's ISIS.
 *	Add Splscs() to reinit & cleanup routines, fork no longer raises ipl.
 *
 *   05-Mar-1989	Todd M. Katz		TMK0003
 *	1. Include header file ../vaxmsi/msisysap.h.
 *	2. Use the ../machine link to refer to machine specific header files.
 *
 *   19-Aug-1988	Todd M. Katz		TMK0002
 *	1. Change all instances of PF_FATALERROR -> PF_ERROR as no current
 *	   instance of crashing the local port is fatal.
 *	2. SCA event codes have been completed revised.  All former BVP SSP
 *	   local port crash codes are now defined as severe error events.  The
 *	   local port crash attribute itself is applied by bvp_crash_lport().
 *	3. Currently bvp_proc_rsp() incorrectly crashes the path when it
 *	   encounters a packet with an unknown status.  Change it so that it
 *	   crashes the local port with a reason code of SE_UNKSTATUS.
 *
 *   02-Jun-1988	Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   09-Jan-1988	Todd M. Katz		TMK0001
 *	Included new header files ../vaxscs/scaparam.h, ../vaxmsi/msisysap.h,
 *	and ../vaxmsi/msiscs.h.
 *
 *   21-Dec-87		map
 *	Added missing {} in wait_prt() routine.  Caused port initialization
 *	to fail if Self-test not done.
 */

/*	Include files
 */
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/kmalloc.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/vmmac.h"
#include "../h/dk.h"
#include "../h/cmap.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/fs.h"

#include "../machine/cpu.h"
#include "../machine/scb.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax


#include "../io/bi/bireg.h"
#include "../io/bi/buareg.h"
#include "../io/bi/bvpreg.h"
#include "../io/bi/bvpport.h"
#include "../io/bi/bdareg.h"

#include	"../h/errlog.h"
#include	"../h/ksched.h"
#include	"../io/scs/sca.h"
#include	"../io/scs/scaparam.h"
#include	"../io/scs/scamachmac.h"
#include	"../io/ci/cippdsysap.h"
#include	"../io/ci/cisysap.h"
#include	"../io/msi/msisysap.h"
#include	"../io/bi/bvpsysap.h"
#include	"../io/gvp/gvpsysap.h"
#include	"../io/uba/uqsysap.h"
#include	"../io/sysap/sysap.h"
#include	"../io/ci/cippdscs.h"
#include	"../io/ci/ciscs.h"
#include	"../io/msi/msiscs.h"
#include	"../io/bi/bvpscs.h"
#include	"../io/gvp/gvpscs.h"
#include	"../io/uba/uqscs.h"
#include	"../io/scs/scs.h"
#include	"../io/gvp/gvp.h"
#include	"../io/bi/bvpppd.h"

/*	Data definitions
 */
u_long		bvp_wait_prt();
void		bvp_cleanup(), bvp_disable();
extern	u_long	gvp_queue_retry;
extern	struct	bidata	bidata[];
extern	int	hz;

/* 	Port Info Block
 */
extern	struct bvp_port_info bvp_port_info[];

/* BVP switch structure
 */
extern	struct bvp_sw bvp_sw[];
extern	int    nbvptypes;

/*	BVP command table
 */
extern	struct 	bvp_cmd	bvp_cmd[];

#define DELAYONE 5


/**/
/*
 *
 *
 *	Name:		bvp_init_port
 *	
 *	Abstract:	
 *			
 *	Inputs:
 *
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
short	bvp_init_port( pccb )
PCCB		*pccb;
{
GVPPQB		*pqb;
GVPPQB		*phys_pqb;
struct bvpregs	*bvrg;
u_long		ps;
int		count;

	if( bvp_wait_prt(pccb) != RET_SUCCESS) {
		return(RET_FAILURE);	/* Port is dead			*/
	}
	pqb = &pccb->Pqb;	/* Get pointer to PQB			*/
	phys_pqb = (GVPPQB *)svtophy(pqb); /* Physical address of PQB	*/
	bvrg = (struct bvpregs *)Pccb.port_regs; /* Get addr of port regs */
/*
 *	Issue Port Init command
 */


	bvrg->bvp_pc = BVP_PC_OWN | 
		       ((u_long)phys_pqb & 0xffffff00) |
		       BVP_CMD_PINIT; /* Write Port Init Instruction	*/
	WBFLUSH

	count = 0;
	while ( count < DELAYONE ) {	/* Wait for at most 1 second	*/
		if ( (bvrg->bvp_pc & BVP_PC_OWN) == 0 ) 
			break;
		DELAY(200000)
		count = count + 1;
	}
	if ( count == DELAYONE ) {
		return( RET_FAILURE );  /* Init Failed	*/
	}

	ps = bvrg->bvp_ps;
	if ( (ps & BVP_PS_PST) != BVP_PSTATE_INIT ) {
		return( RET_FAILURE );
	}
	bvrg->bvp_ps = ps & ~BVP_PS_OWN; /* Clear ownership bit		*/
	WBFLUSH
	return( RET_SUCCESS );	 /* and return success			*/
}
/**/
/*
 *
 *
 *	Name:		bvp_wait_prt
 *	
 *	Abstract:	
 *			
 *	Inputs:
 *
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
u_long	bvp_wait_prt(pccb)
PCCB	*pccb;
{
struct	bvpregs		*bvrg;
u_long			prog_count = -1;
u_long			bvp_status;
int			count;

	bvrg = Pccb.port_regs;	/* Get address of Port Registers	  */
	for(;;) {
	    for(;;) {
		if( (bvrg->bvp_ps & BVP_PS_STD) == 0) { /* Self test not done */
			if (prog_count != bvrg->bvp_pd) { /* Any progress ? */
				prog_count = bvrg->bvp_pd; /* Yes - update */
				DELAY(1000000)	/* Delay 1 second	*/
			}
			else {
				bvp_crash_lport( pccb, 
						 SE_NOPATH, 
						 (SCSH *)NULL ); /* No - we died  */
				return( RET_FAILURE );
			}
		}
		else
			break;

	    }

	    bvp_status = bvrg->bvp_ps;	/* Get copy of status register	  */
	    bvrg->bvp_ps = bvp_status & ~BVP_PS_OWN;
	    WBFLUSH
	    switch( bvp_status & BVP_PS_PST ) {

	    case BVP_PSTATE_STOP:

		bvrg->bvp_pc = BVP_PC_OWN | BVP_CMD_RESTART;
		WBFLUSH
		DELAY(1000000)	/* Delay 1 second	*/


		for(;;) {
		    if( (bvrg->bvp_pc & BVP_PC_OWN) != 0 ) { /* Cmd not complete */
			prog_count = bvrg->bvp_pd;	/* Reset counter	*/
			DELAY(1000000)	/* Delay 1 second	*/
			if (prog_count != bvrg->bvp_pd) { /* Any progress ? */
				prog_count = bvrg->bvp_pd; /* Yes - update */
			}
			else {
				bvp_crash_lport( pccb, 
						 SE_NOPATH, 
						 (SCSH *)NULL ); /* No - we died  */
				return( RET_FAILURE );
			}
		    }
		    else
			break;

		}
		bvp_status = bvrg->bvp_ps;  /* Get copy of status register */
		bvrg->bvp_ps = bvp_status & ~BVP_PS_OWN;
		WBFLUSH
		if ( (bvp_status & BVP_PS_PST) != BVP_PSTATE_UNDF ) {
			return( RET_FAILURE );
		}

		return( RET_SUCCESS );
		break;

	    case BVP_PSTATE_UNDF:

		if ( (BVP_PS_OWN | BVP_PS_XSTP | BVP_PS_ACC | BVP_PS_STD ) ==
		bvp_status ) 		/* All o.k. ?		  */
		    return ( RET_SUCCESS );	/* Yes indeed		  */

		if ( BVP_PS_ACC & bvp_status )	/* No - Can we at least communicate */
		    return ( RET_SUCCESS );	/* Yes find out what we can */

		return( RET_FAILURE );		/* Port is dead in the water */
		break;

	    case BVP_PSTATE_ENAB:
		return( RET_SUCCESS );
		break;

	    default:
		break;

	    }
	}

}
/**/
/*
 *
 *
 *	Name:		bvp_proc_rsp
 *	
 *	Abstract:	
 *			
 *	Inputs:
 *
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
void	bvp_proc_rsp(pccb)
PCCB	*pccb;
{
register GVPH	*bvpbp;
SCSH		*tmpscs;
u_long		port_crash = 0;
u_long		path_crash = 0;
 

/* Remove and process each packet on the local port's response queue until
 * either the queue is exhausted or the local port is crashed.
 */
    do	{
	Remqhi_rspq( pccb, bvpbp )

	if ( bvpbp == ( GVPH * )NULL )
	    break;


	if( bvpbp->status.failure ) {
	    switch((( bvpbp->status.type != T_OTHER )
			? bvpbp->status.type
			: ( T_OTHER + bvpbp->status.subtype ))) {


		/* Unknown Local Port Command.
		 *
		 * This status is generated whenever the local port executes a
		 * command with an invalid  opcode field.
		 *
		 * The local port is crashed and the current packet is disposed
		 * of.  Any packets remaining within the local port's response
		 * queue are flushed during crashing of the port.
		 */
		case ( T_OTHER + ST_URCMD ):
		    port_crash = SE_UNKCMD;
		    break;

		/* Unimplemented Port Command.
		 *
		 * This status is generated whenever the local port executes a
		 * command with an unimplemented  opcode field.
		 *
		 * The local port is crashed and the current packet is 
		 * disposed of. Any packets remaining within the local port's
		 * response queue are flushed during crashing of the port.
		 */
		case ( T_OTHER + ST_UICMD ):
		    port_crash = SE_UNKCMD;
		    break;

		/* Invalid FLAGS or STATUS field
		 *
		 * This status is generated whenever the local port executes a
		 * command with an invalid  flags or non-zero status field.
		 *
		 * The local port is crashed and the current packet is 
		 * disposed of. Any packets remaining within the local port's
		 * response queue are flushed during crashing of the port.
		 */
		case ( T_OTHER + ST_IVLP ):
		    port_crash = SE_UNKCMD;
		    break;

		/* All Other Error Statuses.
		 *
		 * The local port is crashed and the current packet is disposed
		 * of.  Any packets remaining within the local port's response
		 * queue are flushed during crashing of the port.
		 */
		default:
		    port_crash = SE_UNKSTATUS;
		    break;
		}


	    /* Crash the local port.  Crashing the local port immediately
	     *  terminates all response processing.
	     */
	       ( void )bvp_crash_lport( pccb,
					port_crash,
					Pd_to_scs( bvpbp, pccb ));
		return;

	}

	/* Process the current packet according to its port operation code.
	 *
	 * Received sequenced messages are checked for and processed BEFORE
	 * dispatching on the operation code contained within the packet.  Such
 	 * special handling results in significantly improved performance.
	 *
	 * Received messages are always processed and disposed of by SCS.
	 */
	if ( bvpbp->opcode == MSGREC ) {
	    if( (Lpinfo.bvp_type == BI_AIE) ||
		(Lpinfo.bvp_type == BI_AIE_TK) ||
		(Lpinfo.bvp_type == BI_AIE_TK70)) {
		tmpscs = Pd_to_scs( bvpbp, pccb ); 
	    }
	    ( void )scs_msg_rec( pccb,
				 Pd_to_scs( bvpbp, pccb ),
				 ( Pd_to_ppd( bvpbp, pccb )->length - 2 ));
	    continue;
	    }

	switch( bvpbp->opcode ) {

	    /* Transmitted sequenced messages are always processed and 
	     * disposed of by SCS.
	     */
	    case MSGSNT:
		( void )scs_msg_snt( pccb,
				     Pd_to_scs( bvpbp, pccb ),
				     ( Pd_to_ppd( bvpbp, pccb )->length - 2 ));
		continue;

	    /* Received datagrams are processed and disposed of by SCS if they
	     * contain application datagrams.  If they are not application
	     * datagrams then the port is sick and the path is crashed.
	     */
	    case DGREC: {
		register GVPPPDH		*bvpppdbp;

		if (( bvpppdbp = Pd_to_ppd( bvpbp, pccb ))->mtype == SCSDG )
		    ( void )scs_dg_rec( pccb,
					Pd_to_scs( bvpbp, pccb ),
					bvpppdbp->length - 2 );
		else {
		    ( void )bvp_crash_lport( pccb,
					    SE_INVOPCODE,
					    Pd_to_scs( bvpbp, pccb ));
		    return;
		}

		continue;
		}

	    /* Crash the local port for all other known port operation codes.
	     * Crashing the local port immediately terminates all response
	     * processing.
	     */
	    case DGSNT:
		( void )bvp_crash_lport( pccb,
					SE_INVOPCODE,
					Pd_to_scs( bvpbp, pccb ));
		return;

	    /* Crash the local port for all unknown port operation codes.
	     * Crashing the local port immediately terminates all response
	     * processing.
	     */
	    default:
		( void )bvp_crash_lport( pccb,
					SE_UNKOPCODE,
					Pd_to_scs( bvpbp, pccb ));
		return;
	    }

    }	while( pccb->Pqb.rspq.flink != ( gvpbq * )NULL );
}
/**/
/*
 *
 *
 *	Name:		bvp_qtrans
 *	
 *	Abstract:	Queue transition handler
 *			
 *	Inputs:
 *
 *	pccb		- Pointer to PCCB for the port
 *	mask		- Number representing queue that transitioned
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
void	bvp_qtrans( pccb, queno )
PCCB	*pccb;
u_long	queno;
{
struct	bvpregs	*bvrg;
u_long		cmd_pend, cmd_off;

	switch ( queno ) {

	case MFREEQ:
		Pccb.cmd_pend |= M_freeq;
		break;
	case DFREEQ:
		Pccb.cmd_pend |= D_freeq;
		break;
	case CMDQ0:
		Pccb.cmd_pend |= C_cmdq_0;
		break;
	case CMDQ1:
		Pccb.cmd_pend |= C_cmdq_1;
		break;
	case CMDQ2:
		Pccb.cmd_pend |= C_cmdq_2;
		break;
	case CMDQ3:
		Pccb.cmd_pend |= C_cmdq_3;
		break;
	default:
		panic("bvp_qtrans: Invalid queue\n");
	}
	bvrg = Pccb.port_regs;
	if (bvrg->bvp_pc & BVP_PC_OWN)  { /* We don't own it	*/
		return;
	}
		
	cmd_off = ffs(Pccb.cmd_pend);
	cmd_off--;			/* Get bit position	*/
	bvrg->bvp_pc = bvp_cmd[cmd_off].command; /* Issue command	*/
	WBFLUSH
	Pccb.cmd_pend &= ~(1<<cmd_off);
}
/**/
/*
 *
 *
 *	Name:		bvp_timer
 *	
 *	Abstract:	
 *			
 *	Inputs:
 *
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
void	bvp_timer( pccb )
PCCB	*pccb;
{
struct	bvpregs	*bvrg;
u_long		cmd_pend, cmd_off;

	bvrg = Pccb.port_regs;		/* Port regs address		*/
	if ( Pccb.cmd_pend != 0 ) {	/* Have commands pending	*/

		if ( (bvrg->bvp_pc & BVP_PC_OWN) == 0){  /* We own it	*/
		     cmd_off = ffs(Pccb.cmd_pend);
		     cmd_off--;			/* Get bit position	*/
		     bvrg->bvp_pc = bvp_cmd[cmd_off].command; /* Issue command */
		     WBFLUSH
		     Pccb.cmd_pend &= ~(1<<cmd_off);
		}
	}
	timeout(bvp_timer,pccb,Pccb.poll_rate); /* Requeue timeout	*/
}
/**/
/*
 *
 *
 *	Name:		bvp_log_err
 *	
 *	Abstract:	Log BVP port errors
 *			
 *	Inputs:
 *
 *	pccb	-	Pointer to pccb for port
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:	Error is logged.
 *
 */
void	bvp_log_error(pccb)
PCCB	*pccb;
{
struct	bvpregs	*bvrg;
register struct el_rec *elrp;
register struct el_bvp *elbod;
int	i,unit;
u_char	type;

	bvrg = Pccb.port_regs;
	cprintf("Port error: bvp_ps = %x\n", bvrg->bvp_ps);
	cprintf("            bvp_pe = %x\n", bvrg->bvp_pe);
	cprintf("            bvp_pd = %x\n", bvrg->bvp_pd);
	for (i = 0; i < nbvptypes; i++) {
		if (bvp_sw[i].type == Lpinfo.bvp_type)
			break;
	}
	if (i == nbvptypes) {	/* Not found in BVP switch table		*/
		panic("bvp_log_err: Invalid port type");
	}
	if( (elrp = ealloc( (sizeof(struct el_bvp)), EL_PRIHIGH)) == EL_FULL)
		return;
	elbod = &elrp->el_body.elbvp;
	elbod->bvp_biic_typ = Pccb.nxv->biic_typ;
	elbod->bvp_biic_csr = Pccb.nxv->biic_ctrl;
	elbod->bvp_pcntl = bvrg->bvp_pc;
	elbod->bvp_pstatus = bvrg->bvp_ps;
	elbod->bvp_perr = bvrg->bvp_pe;
	elbod->bvp_pdata = bvrg->bvp_pd;
	LSUBID(elrp,ELCT_DCNTL,ELBI_BVP,bvp_sw[i].errlog_typ,
		Pccb.binode,Pccb.bvp_ctlr,bvrg->bvp_pe);
	EVALID(elrp);

}
/**/
/*
 *
 *
 *	Name:		bvp_port_error
 *	
 *	Abstract:	Take action on port error.
 *			
 *	Inputs:
 *
 *	pccb	-	Pointer to pccb for port in error
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
u_long	bvp_port_error(pccb)
PCCB	*pccb;
{
struct	bvpregs	*bvrg;
u_long		errcode;

	bvrg = Pccb.port_regs;
	errcode = bvrg->bvp_ps & BVP_PS_ETYPE;

	switch( errcode ) {

	/*
	 * 	Port operation continues
	 */

	case	BVP_ETYPE_TBI:
	case	BVP_ETYPE_EXC:
	case	BVP_ETYPE_NFBI:

		return( RET_SUCCESS );
		break;

	/*
	 * 	This port is shutdown
	 */

	case	BVP_ETYPE_FBI:
	case	BVP_ETYPE_DSE:
	case	BVP_ETYPE_PLE:

		bvp_disable( pccb, PF_PORTERROR );
		return( RET_FAILURE );
		break;

	/*
	 *	All ports are shutdown
	 */

	case	BVP_ETYPE_AHE:

		bvp_disable( pccb, PF_PORTERROR );
		return( RET_FAILURE );
		break;

	default:

		break;
	}
}
/**/
/*
 *
 *
 *	Name:		bvp_reinit
 *	
 *	Abstract:	
 *			
 *	Inputs:
 *
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
void	bvp_reinit( pccb )
PCCB	*pccb;
{
struct	bvpregs		*bvrg;
u_long	devtype;
u_long	bvp_status;
int	i,j,s,count;

	s = Splscs();	/*  Raise IPL to SCS level	 */

	Pccb.rip = 0;
	bvrg = Pccb.port_regs;
	bvp_init_blks( pccb );	/* Init  PQB and PCCB	 */
	if (bvp_init_port( pccb ) != RET_SUCCESS) {
		(void)splx(s);		/* Lower IPL		*/
		return;		/* Init failed or no such port	 */
	}

/*
 *	Issue port Enable instruction
 */

	bvrg->bvp_pc = BVP_PC_OWN | BVP_CMD_ENAB; /* Enable the port	 */
	WBFLUSH
	Wait_own_nr( bvrg )		/* Wait for PC own bit to reset	*/

	bvp_status = bvrg->bvp_ps;
	if ( (bvp_status & BVP_PS_PST) != BVP_PSTATE_ENAB ) {
		(void)splx(s);		/* Lower IPL		 */
		return;
	}

	bvrg->bvp_ps = bvp_status & ~BVP_PS_OWN; /* Clear ownership bit	*/
	WBFLUSH
/*
 *	Issue Read PIV instruction to turn on interrupts
 */

	Bvpqb.piv = Pccb.ivec;	/* Set in interrupt vector		 */

	bvrg->bvp_pc = BVP_PC_OWN | BVP_CMD_RPIV; /* Read PIV command	 */
	WBFLUSH
	Wait_own_nr( bvrg )		/* Wait for PC own bit to reset	*/


	if (bvp_create_sys( pccb ) != RET_SUCCESS) {
		(void)splx(s);		/* Lower IPL		 */
		return;		/* Failed to make system known	 */
	}

	if(Pccb.poll_rate == 0) 	/* Set the rate for the polling	*/
		Pccb.poll_rate = 2 * hz;

/*									*/
/*	Start the timer running. The bvp_timer routine will check	*/
/*	for pending port commands and issuing one if possible.		*/
/*									*/

	if ( (Lpinfo.bvp_flags & BVP_TIM) == 0 ) { /* If timer not yet on */
		Lpinfo.bvp_flags |= BVP_TIM; /* Indicate timer now on	*/
		timeout(bvp_timer,pccb,Pccb.poll_rate); /* Start timer	*/
	}
	(void)splx(s);		/* Lower IPL			 */
	return;
}
/**/
/*
 *
 *
 *	Name:		bvp_disable
 *	
 *	Abstract:	
 *			
 *	Inputs:
 *
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
void	bvp_disable( pccb, reason  )
PCCB	*pccb;
u_long	reason;
{
PB			*pb;
int			s, lock;
struct	bvpregs		*bvrg;
u_long			bvp_status;
int			count;


	if (Pccb.rip == 0) {	/* If recovery not already in progress  */
	     Pccb.rip = 1;		/* Set recovery in progress	*/

	    /*	Mark path as failed
	     */
	     if( (pb = Pccb.pb) != (PB *)NULL ) {
		pb->pinfo.state = PS_PATH_FAILURE;
		pb->pinfo.reason = reason;
	     }


	     bvrg = Pccb.port_regs;
	     bvp_status = bvrg->bvp_ps;

	    /* 	Attempt graceful shutdown first.
	     */

	     count = 0;
	     if ( (( bvp_status & BVP_PS_OWN ) == 0) &&
			((bvp_status & BVP_PS_PST) == BVP_PSTATE_ENAB) ) {

		bvrg->bvp_pc = BVP_PC_OWN | BVP_CMD_SHUT; /* Issue Shutdown cmd  */
		WBFLUSH
		while ( count < DELAYONE ) {	/* Wait for at most 1 second	*/
		    if ( ( bvrg->bvp_pc & BVP_PC_OWN) == 0 ) 
			break;
		    DELAY(200000)
		    count = count + 1;
		}

	     }	
	     if ( ( count == DELAYONE ) || 
			((bvp_status & BVP_PS_PST) != BVP_PSTATE_ENAB) ) {

		s = spl7();
		if(Pccb.bidata->biinfo[Pccb.binode].lock == 0) {
			Pccb.bidata->biinfo[Pccb.binode].lock = 1; /* Set lock */
		}
		else {
			/* Wait for unlock ?? */
		}
		(void)splx(s);
		if( Pccb.bidata->biinfo[Pccb.binode].incarn ==
			Pccb.incarn ) {
			(void)bisst(&Pccb.nxv->biic_ctrl);
			Pccb.bidata->biinfo[Pccb.binode].incarn++;
		}
		Pccb.incarn = Pccb.bidata->biinfo[Pccb.binode].incarn;
		s = spl7();
		Pccb.bidata->biinfo[Pccb.binode].lock = 0;
		(void)splx(s);

	    }
		
	     if ( (Lpinfo.bvp_flags & BVP_TIM) != 0) { /* Turn off timer if on */
		(void)untimeout(bvp_timer, pccb);
		Lpinfo.bvp_flags &= ~BVP_TIM; /* Timer now off		*/
	     }
	     Kfork(&pccb->forkb, bvp_cleanup, pccb )
	}
	return;				/* If already forking then	*/
					/* forget this request.		*/


}
/**/
/*
 *
 *
 *	Name:		bvp_cleanup
 *	
 *	Abstract:	
 *			
 *	Inputs:
 *
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
void bvp_cleanup( pccb )
PCCB	*pccb;

{
PB			*pb;
int			s = Splscs();

	bvp_proc_rsp( pccb ); 	/* Return remaining responses		*/

	/* Remove and deallocate all packets on all port queues.  
	 * The queue is zeroed whenever the queue interlock can not 
	 * be obtained. This results in permanent loss of the entries 
	 * that had been on that queue.
	 */
	{
	register gvpbq	*q, *qend;

	for( q = &Vpqb.cmdq0, qend = &Vpqb.rspq; q <= qend; ++q )
	    Flushq( pccb, q )
	for( q = &Pccb.dfreeq, qend = &Pccb.mfreeq; q <= qend; ++q )
	    Flushq( pccb, q )
	}

	if ( (pb = Pccb.pb) != (PB *)NULL )	/* If we have a path open */
		scs_path_crash( Pccb.pb );	 /* Notify SCS	*/
	else
	    Kfork(&pccb->forkb, bvp_reinit, pccb )
	splx(s);
	return;			/* Return from fork		*/
}
/**/
/*
 *
 *
 *	Name:		bvp_log_packet
 *	
 *	Abstract:	
 *			
 *	Inputs:
 *
 *	
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
void	bvp_log_packet( pccb, pb, bvpppdbp, reason)
PCCB	*pccb;
PB	*pb;
GVPPPDH	*bvpppdbp;
int	reason;
{
}
/**/
/*
 *
 *
 *	Name:		bvp_slave
 *	
 *	Abstract:	This routine simply returns 0 indicating the drive
 *			is offline. The class driver will configure the
 *			drive on its own.
 *			
 *	Inputs:
 *
 *	ui		- Pointer to unibus device structure
 *	nxv		- Base address of controller
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *	
 *
 *	Return 
 *	Values:		0
 *
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
bvp_slave( ui, nxv )
struct	uba_device	*ui;
struct	bi_nodespace	*nxv;
{
	return(0);
}
