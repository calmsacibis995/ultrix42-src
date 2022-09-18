#ifndef lint
static char *sccsid = "@(#)bvp_ssp.c	4.3	(ULTRIX)	10/11/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1988 - 1989 by			*
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
 *   Author:		Mark A. Parenti
 *
 *   Modification History
 *
 *   09-Nov-89		David E. Eiche		DEE0080
 *	Changed bvp_init_blks() to fill in the new software and
 *	interconnect type fields in the LPIB.
 *
 *   19-Sep-1989	Pete Keilty
 *	Added use of local port info. block ovhd_pd to BVP.
 *	Remove sizeof(GVPH).  Implicit command address format.
 *
 *   25-Aug-1989	Pete Keilty
 *	Cleaned up port enable wait on own bit and delay for 200000.
 *	Removed kfork set ipl no longer needed.
 *
 *   20-Jul-1989	Mark A. Parenti
 *	Use Ctrl_from_num() macro to generate local system name. Controller
 *	numbers large than 9 will now be correctly displayed.
 *
 *   25-May-1989	Pete Keilty
 *	Made changes so bvp tape runs on mips cpu's. WBFLUSH add,
 *	changed splx( IPL_SCS ) to Splscs(), Ksched fork routines now
 *	raise IPL.
 *	
 *   17-Jan-1989	Todd M. Katz		TMK0003
 *	1. The macro Scaaddr_lol() has been renamed to Scaaddr_low().  It now
 *	   accesses only the low order word( instead of low order longword ) of
 *	   a SCA system address.  The macro Scaaddr_hos() has been renamed to
 *	   Scaaddr_hi().  Make use of the new macro Scaaadr_mid().
 *	2. Include header file ../vaxmsi/msisysap.h.
 *	3. Use the ../machine link to refer to machine specific header files.
 *
 *   21-Aug-1988	Todd M. Katz		TMK0002
 *	Modify bvp_init_blks() to initialize all of the PCCB interlocked
 *	queue instqi/remqhi error code fields.
 *
 *   05-Aug-1988	David E. Eiche		DEE0049
 *	Fill in the hardware port type field in the LPIB with the hardware
 *	port rather than the adaptor type.
 *
 *   02-Jun-1988	Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   14-Apr-1988	David E. Eiche		DEE0030
 *	Change uba_driver table to specify "ra"	disk type.
 *
 *   09-Jan-1988	Todd M. Katz		TMK0001
 *	Included new header files ../vaxscs/scaparam.h, ../vaxmsi/msisysap.h,
 *	and ../vaxmsi/msiscs.h.
 */

/* Libraries and Include Files.
 */


#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/dyntypes.h"
#include "../h/vmmac.h"
#include "../h/dk.h"
#include "../h/kmalloc.h"
#include "../h/cmap.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/fs.h"

#include "../machine/cpu.h"
#include "../machine/scb.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../machine/common/cpuconf.h"
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


/* External defines							*/

extern	numbvp;	/* hack for ft1 km_alloc flag */

extern  int 	cpu;
extern	u_long	boottime, scs_msg_size, scs_dg_size, gvp_max_bds;
extern	GVPBDDB	*gvp_bddb;
extern	struct bidata	bidata[];
extern 	pccbq	scs_lport_db;
extern 	sbq	scs_config_db;
extern 	int	hz;
extern	int	sysptsize;
extern 	u_long	gvp_initialize(), scs_initialize();
extern 	PB	*scs_alloc_pb();
extern 	SB	*scs_alloc_sb();
extern	void	scs_unix_to_vms();
extern	SCSH	*gvp_alloc_msg(), *gvp_remove_msg(), *gvp_alloc_dg(),
		*gvp_remove_dg();
extern 	void	gvp_unmap_buf(), gvp_dealloc_msg(), gvp_add_msg(),
		gvp_send_msg(), scs_dealloc_pb(), scs_dealloc_sb(), 
		gvp_dealloc_dg(), gvp_add_dg(), gvp_send_dg();
extern	u_long	gvp_map_buf();
#define DELAYONE 5

/* 
 *	Data Definitions
 */
PIB	bvp_pib;
SIB	bvp_sib;

extern	int	bvpsspinit();
extern	void	bvp_remove_pb(), bvp_crash_path(), bvp_crash_lport(), 
		bvp_timer();
extern	u_long	bvp_open_path(), bvp_mreset(), bvp_mstart(), bvp_create_sys();
extern	PB	*bvp_get_pb();

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
extern struct	uba_ctlr *bvpminfo[];


int	bvp_probe();
void	bvp_init_blks(), bvp_qtrans();



PDT	bvppdt	={		/* BVP Port Dispatch Table		*/
		gvp_alloc_dg,	/* allocate_dg 		(supported)	*/
		gvp_dealloc_dg,	/* deallocate_dg	(supported)	*/
		gvp_add_dg,	/* add_dg 		(supported)	*/
		gvp_remove_dg,	/* remove_dg 		(supported)	*/
		gvp_alloc_msg,	/* allocate_msg 	(supported)	*/
		gvp_dealloc_msg,/* deallocate_msg 	(supported)	*/
		gvp_add_msg,	/* add_msg 		(supported)	*/
		gvp_remove_msg,	/* remove_msg 		(supported)	*/
		gvp_send_msg,	/* send_msg 		(supported)	*/
		gvp_map_buf,	/* map_buffer 		(supported)	*/
		gvp_unmap_buf,	/* unmap_buffer 	(supported)	*/
		bvp_open_path,	/* open_path 		(supported)	*/
		bvp_crash_path,	/* crash_path 		(supported)	*/
		bvp_get_pb,	/* get_pb 		(supported)	*/
		bvp_remove_pb,	/* remove_pb 		(supported)	*/
		bvp_mreset,	/* maint_reset 		(supported)	*/
		bvp_mstart,	/* maint_start 		(supported)	*/
		gvp_send_dg,	/* send_dg 		(supported)	*/
		0,		/* send_data 		(unsupported)	*/
		0,		/* request_data		(unsupported)	*/
		bvp_crash_lport,/* crash_lport 		(supported)	*/
		0		/* shutdown 		(unsupported)	*/
		
	   };



u_short bvpstd[] = { 0 };
extern	int	bvp_slave();
struct  uba_driver bvpsspdriver =
{ bvpsspinit, 
bvp_slave, 
0, 
0, 
bvpstd, 
"ra", 
0, 
"bvp", 
bvpminfo, 
0};


/**/
/*
 *
 *
 *	Name:		bvp_probe
 *	
 *	Abstract:	Probe routine
 *			
 *	Inputs:
 *
 *	reg	-	Address of BIIC Registers
 *	binumber - 	BI number this controller on
 *	binode	-	BI node number for this controller
 *	um	-	Ptr to uba_ctlr struct
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
int	bvp_probe( nxv, binumber, binode, um )
int	nxv, binumber, binode;
struct	uba_ctlr	*um;
{
struct	biic_regs	*birg = (struct biic_regs *)nxv;	
struct	bvp_port_info	*pcinfo = &bvp_port_info[um->um_ctlr];
PCCB	*pccb;
volatile struct	bvpregs	*bvrg;
u_long	devtype,bvp_status;
int	i,j,s,count;


	devtype = birg->biic_typ & BITYP_TYPE; /* Device type	 */
	for (i = 0; i < nbvptypes; i++) {
		if (bvp_sw[i].type == devtype)
			break;
	}
	if (i == nbvptypes)	/* We don't know about this type	*/
		return(0);
	bvrg = (struct bvpregs *) (nxv + (long)bvp_sw[i].offset);

	KM_ALLOC( pccb, PCCB *, sizeof(PCCB), KM_SCA, KM_NOW_CL_CA )
	if ( pccb == (PCCB *)NULL )
		return(0);
	pcinfo->pc_ptr = pccb;		/* Save pointer to PCCB		 */
	U_long( pccb->size ) = sizeof( PCCB );	/* Size of PCCB		 */
	pccb->type = DYN_PCCB;		/* Structure type		 */
	Pccb.port_regs = (struct bvpregs *)bvrg; /* Address of BVP Port Registers */
	Pccb.bvp_ctlr = um->um_ctlr;	/* Save controller number	 */
	Pccb.bidata = &bidata[binumber]; /* Ptr to bidata structure	 */
	Pccb.binumber = binumber;	/* BI number			 */
	Pccb.binode = binode;		/* BI node number		 */
	Pccb.nxv = (struct biic_regs *)nxv; /* Base virtual address		 */
	Lpinfo.bvp_type = devtype; /* Device type	 */

/*
 *	Insert PCCB on system queue
 */
	Insert_entry(pccb->flink, scs_lport_db); 

	s = Splscs();		/*  Raise IPL to SCS level	 */
	if((i = scs_initialize()) != RET_SUCCESS ||/* Init the SCS layer */
	   (i = gvp_initialize()) != RET_SUCCESS){ /* Init the GVP layer */
		(void)splx(s);		/* Lower IPL			 */
		Remove_entry(pccb->flink); /* Remove from lport_db queue */
					/* Reset the device		 */
		return(0);		/* Return with err if init fails */
	}
	(void)splx(s);		/* Lower IPL				 */

	bvp_init_blks( pccb );		/* Init  PQB and PCCB	 */
	if (bvp_init_port( pccb ) != RET_SUCCESS)
		return(0);		/* Init failed or no such port	 */

/*
 *	Issue port Enable instruction
 */


	WBFLUSH
	bvrg->bvp_pc = BVP_PC_OWN | BVP_CMD_ENAB; /* Enable the port	 */
	WBFLUSH
	Wait_own( bvrg )	/* Wait for PC own bit to reset	*/
	DELAY(200000);		/* Wait for port give it enough time. */

	bvp_status = bvrg->bvp_ps;
	if ( (bvp_status & BVP_PS_PST) != BVP_PSTATE_ENAB ) {
		cprintf("bvp_probe: Enable failed - Wrong state\n");
		return( 0 );
	}

	bvrg->bvp_ps = bvp_status & ~BVP_PS_OWN; /* Clear ownership bit */
	WBFLUSH
/*
 *	Issue Read PIV instruction to turn on interrupts
 */

	Bvpqb.piv = Pccb.ivec;	/* Set in interrupt vector		 */


	bvrg->bvp_pc = BVP_PC_OWN | BVP_CMD_RPIV; /* Read PIV command	 */
	WBFLUSH

	Wait_own( bvrg )		/* Wait for PC own bit to reset	*/


	s = Splscs();		/*  Raise IPL to SCS level	 */
	if (bvp_create_sys( pccb ) != RET_SUCCESS) {
		(void)splx(s);		/* Lower IPL			 */
		return(0);		/* Failed to make system known	 */
	}
	(void)splx(s);		/* Lower IPL				 */

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

 	numbvp++;  /* hack for ft1 km_alloc flag */

	return( RET_SUCCESS );
}

/*
 *
 *
 *	Name:		bvp_init_blks
 *	
 *	Abstract:	Initialize fields in pqb and pccb
 *			
 *	Inputs:
 *
 *	pccb	-	Ptr to pccb for this port
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
void	bvp_init_blks( pccb )
PCCB		*pccb;
{
int binumber;
int intr_dest;
u_long intr_vec;

	binumber = Pccb.binumber;
/*
 *	Initialize PCCB
 */
	pccb->pdt = &bvppdt;		/* Init pdt pointer		 */
	Vpccb.qtransition = bvp_qtrans; /* Queue transition handler	 */
	Vpccb.pcommq.error = SE_ICMDQ0;	/* Cmd queue 0 insqti error	 */
	Vpccb.pcommq.cmask = CMDQ0;	/* Cmd queue 0 mask		 */
	Vpccb.pcommq.header = &Vpqb.cmdq0; /* Use cmd queue 0 for comm	 */
	Vpccb.pcontrolq.error = SE_ICMDQ1; /* Cmd queue 1 insqti error	 */
	Vpccb.pcontrolq.cmask = CMDQ1; /* Cmd queue 1 mask		 */
	Vpccb.pcontrolq.header = &Vpqb.cmdq1; /* Use cmd queue 1 for control*/
	Vpccb.pdfreeq.error = SE_IDFREEQ;/* Datagram free queue insqti error*/
	Vpccb.pmfreeq.error = SE_IMFREEQ;/* Message free queue insqti error*/
	Vpccb.pdfreeq.cmask = DFREEQ;	/* Datagram free queue mask	 */
	Vpccb.pmfreeq.cmask = MFREEQ;	/* Message free queue mask	 */
	Vpccb.pdfreeq.header = &Pccb.dfreeq; /* Init datagram free queue hdr*/
	Vpccb.pmfreeq.header = &Pccb.mfreeq; /* Init message free queue hdr */
	Vpccb.dfreeq_remerr = SE_RDFREEQ; /* Datagram free queue remqhi err*/
	Vpccb.mfreeq_remerr = SE_RMFREEQ; /* Message free queue remqhi err*/
	Vpccb.rspq_remerr = SE_RRSPQ;	/* Response queue remqhi error	*/

	Vpinfo.ovhd_pd = Gvph_size_imp;	/* Implicit address format */
	Vpinfo.dg_size = scs_dg_size + 
			 sizeof(GVPPPDH) +
			 Vpinfo.ovhd_pd +
			 sizeof(SCSH);
	Vpinfo.msg_size = scs_msg_size +
			  sizeof(GVPPPDH) +
			  Vpinfo.ovhd_pd +
			  sizeof(SCSH);
	Vpinfo.ovhd = sizeof(GVPPPDH) + Vpinfo.ovhd_pd;
/*									*/
/*	Set up SCS info							*/
/*									*/

	pccb->lpinfo.name = Ctrl_from_num( "bv  ", Pccb.bvp_ctlr); /* Local port name	*/
	Scaaddr_low( pccb->lpinfo.addr ) = *(u_short *)&Pccb.bvp_ctlr; /* Remote port address */
	Scaaddr_mid( pccb->lpinfo.addr ) = 0;
	Scaaddr_hi( pccb->lpinfo.addr ) = 0;
	pccb->lpinfo.type.hwtype = HPT_BVPSSP; /* Local port type	*/
        pccb->lpinfo.type.swtype = SPT_BVPSSP; /* Software port type    */
        pccb->lpinfo.type.ictype = ICT_BI;     /* Interconnect type     */
	pccb->lpinfo.type.dual_path = 0; /* Single path port		*/

/*
 *	Initialize PQB
 */


	intr_dest = (bidata[binumber].biintr_dst) << 16;
	intr_vec = 
	   (SCB_BI_VEC_ADDR(binumber,Pccb.binode,LEVEL14) - 
		 &scb.scb_stray) << 4;

	Bvpqb.piv = 0;	/* Disable interrupts during initialization	*/

/*
 *	Compute interrupt vector for later use
 *	Use BI interrupt level 15 but use LEV14 slot in SCB
 */

	Pccb.ivec =
		 intr_dest | intr_vec | BVP_LEV_15; /* intr_dest, vector, level */



/*
 *	Initialize various PQB fields
 */

	Bvpqb.dfreeq_hdr = BVP_DFREEQ;
	Bvpqb.mfreeq_hdr = BVP_MFREEQ;

	Bvpqb.dqe_len = scs_dg_size + sizeof(GVPPPDH) + 
			Vpinfo.ovhd_pd + sizeof(SCSH);
	Bvpqb.mqe_len = scs_msg_size + sizeof(GVPPPDH) + 
			Vpinfo.ovhd_pd + sizeof(SCSH);

	Bvpqb.vpqb_base = VPQB_BASE;
	Bvpqb.bdt_base = BDT_BASE;

	Bvpqb.bdt_len = ( u_short ) gvp_max_bds;

	Bvpqb.spt_base = SPT_BASE;
	Bvpqb.spt_len = SPT_LEN;
	Bvpqb.gpt_base = GPT_BASE;
	Bvpqb.gpt_len = Bvpqb.spt_len;
	Bvpqb.keep_alive = 0;	/* Disable timer			*/

	Dm_pccbifISIS

	Bvpqb.bvp_level = 1;
	Bvpqb.pd_prtvrs = 1;

	Bvpqb.pd_max_dg = ( u_short ) scs_dg_size;
	Bvpqb.pd_max_msg = ( u_short ) scs_msg_size;

	bcopy("U-32",(u_char *)&Bvpqb.pd_sw_type , 4);
	bcopy(RELEASE,(u_char *)&Bvpqb.pd_sw_version, 4); 

	Bvpqb.pd_hw_type = Lpinfo.bvp_type;
	(void)scs_unix_to_vms((struct timeval *)&boottime, &Bvpqb.pd_cur_time);
	
}

/*
 *
 *
 *	Name:		bvp_create_sys	
 *					
 *	
 *	Abstract:	Obtain the necessary data structures to make a system
 *			known to SCS. Initialize these structures and call the
 *			appropriate routine to make the system known.
 *			
 *	Inputs:
 *
 *	pccb			- Pointer to PCCB for this port
 *
 *	     
 *	
 *	
 *
 *	Outputs:
 *
 *	pccb			- Pointer to PCCB for this port
 *
 *
 *	sb			- System block for this port
 *
 *	     various fields initialized
 *
 *	pb			- Path Block for this port
 *
 *	     various fields initialized
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	0			- System could not be created
 *	1			- System could be created
 *	
 * 
 *	Side		
 *	Effects:
 *
 *	The SB and PB for the port are allocated and initialized. The PB
 *	is placed on the System Block PB queue and the SB is place on the
 *	scs_config_db systemwide queue. scs_new_path is called to notify
 *	SCS of the existence of a new path to the system.
 */
u_long	bvp_create_sys(pccb)
PCCB		*pccb;
{
	register PIB	*pib = &bvp_pib;
	register SIB	*sib = &bvp_sib;
	PB	*pb;
	SB	*sb;
	int	i;


	(void)bzero( pib, sizeof(PIB));	/* Clear pib			*/
	(void)bzero( sib, sizeof(SIB)); /* Clear sib			*/

	pib->lport_name = pccb->lpinfo.name; /* Local port name		*/
	pib->type.hwtype = HPT_BVPSSP;	/* Local port type		*/
	pib->type.dual_path = 0;	/* Single path port		*/
	Scaaddr_low( pib->rport_addr ) = *(u_short *)&Pccb.bvp_ctlr; /* Remote port address */
	Scaaddr_mid( pib->rport_addr ) = 0;
	Scaaddr_hi( pib->rport_addr ) = 0;

/*	If we can't get a pb then it's no go				*/
	if((pb = scs_alloc_pb( PS_OPEN, pccb, pib )) == (PB *)NULL) {
		return(RET_FAILURE);
	}

	sib->max_dg = Bvpqb.ad_max_dg;   /* Max datagram size		*/
	sib->max_msg = Bvpqb.ad_max_msg; /* Max message size		*/
	sib->npaths = 1;		/* Only 1 path to these devices	*/
	sib->swtype = Bvpqb.ad_sw_type; /* Adapter software type	*/
	sib->swver = Bvpqb.ad_sw_version; /* Adapter software version	*/
	(void)scs_unix_to_vms((struct timeval *)&boottime, &sib->swincrn);
	sib->hwtype = Bvpqb.ad_hw_type;	/* Adapter hardware type	*/
	sib->hwver = Bvpqb.ad_hw_version; /* Adapter hardware version	*/
	*(u_long *)sib->node_name = pccb->lpinfo.name; /* SCA Node name	*/
	bcopy("    ",(u_long *)sib->node_name+1, 4);

/*	The system id of bvp controllers is as follows:			*/
/*									*/
/*      47             32 31                                 0          */
/*	+----------------+------------------------------------+		*/
/*	!		 !				      !		*/
/*	! BVP dev type   !   Device Address (Port Regs)	      !		*/
/*	!		 !				      !		*/
/*	+----------------+------------------------------------+		*/
/*									*/
/*									*/

	{
	register u_long	devaddr = *(u_long *)&Pccb.port_regs;

	Scaaddr_low( sib->sysid ) = ( u_short )( devaddr & 0xffff );
	Scaaddr_mid( sib->sysid ) = ( u_short )(( devaddr >> 16 ) & 0xffff );
	Scaaddr_hi( sib->sysid ) = *(u_short *)&Lpinfo.bvp_type;
	}

/*	If we can't get a sb then it's no go				*/
	if((sb = scs_alloc_sb( sib )) == (SB *)NULL) {
		return(RET_FAILURE);
	}


	Pccb.pb = pb;			/* Save on pccb			*/
	pb->sb = sb;			/* Point to System Block	*/
	Insert_entry(pb->flink,Sb->pbs); /* Insert PB on SB queue	*/
	Insert_entry(sb->flink,scs_config_db); /* Insert SB on system-wide */
					/* configuration database	*/
	scs_new_path( sb, pb );
	return(RET_SUCCESS);
}

/*
 *
 *
 *	Name:		bvp_intr
 *	
 *	Abstract:	Interrupt service handler for ssp bvp
 *			
 *	Inputs:
 *
 *	ctlr		- Controller number. Used as an index into
 *			  port-specific structures
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
void	bvpsspintr( ctlr )
int	ctlr;
{
PCCB		*pccb;
struct bvp_port_info *pcinfo = &bvp_port_info[ctlr];
struct bvpregs	*bvrg;
int	s;
u_long	bvp_status;

	s = Splscs();		/*  Raise IPL to SCS level	 */
	pccb = pcinfo->pc_ptr;	/* Get pointer to PCCB			*/
	bvrg = (struct bvpregs *)Pccb.port_regs; /* Get pointer to regs */
	bvp_status = bvrg->bvp_ps;
	Pccb.port_state = bvrg->bvp_ps & BVP_PS_PST;
	if (bvp_status & BVP_PS_SUME){ /* Summary Error set - handle error */
		bvp_log_error(pccb);		/* Log port error	*/
		bvp_port_error(pccb);		/* Take action on port error */
	}
	if (bvp_status & BVP_PS_RSQ)  /* Response Queued - process resp */
		bvp_proc_rsp(pccb);
/*
 *	Clear PS ownership bit
 */
	bvrg->bvp_ps = bvp_status & ~BVP_PS_OWN;

	(void)splx(s);		/*  Lower IPL to interrupt level	*/

	return;
}

