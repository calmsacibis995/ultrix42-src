#ifndef lint
static char *sccsid = "@(#)uqserv.c	4.12	(ULTRIX)	2/14/91";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1985 - 1990 by			*
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
 ************************************************************************
 *									
 *									
 *	Facility:  Systems Communication Architecture			
 *		   UQSSP Port-to-Port Driver				
 *									
 *	Abstract:  This module contains all of the UQSSP PPD service	
 *		   functions and the UQSSP-PPD Port Dispatch Table (PDT)
 *		   declaration.						
 *									
 *	Creator:	Mark Parenti	Creation Date:	July 15, 1985
 *
 *	History:
 *
 *	Feb 1991	Matthew Sacks
 *			In uq_map_buf(), initialize entire buffer
 *			address descriptor, which is a C union, to
 *			0.  This is done using one of the structures
 *			in the union that encompasses all of the bits in
 *			any variation of the union.  These bits are all
 *			zeroed to make sure that the MBZ bits in the
 *			buffer descriptor are always zero.
 *
 *	Dec 1990	Matthew Sacks
 *			Made uq_crash_path return if already re-initing
 *			the path.  Rewrote uq_ins_cring and uq_ins_rspring
 *			so that	all manipulation of the port rings is SMP
 *			protected. The smp safeness of this code has
 *			been fully exercised.  Also, added comments.
 *
 *	Oct 1990	Matthew Sacks
 *			Fix protocol bug in uq_poll_rspring by not calling up
 *			to scs_msg_rec if the path block is null. This
 *			could have happened when the port is crashing.
 *			Reset UQPath_Is_Up in uq_disable() so that
 *			re-reinits work. Change the PF_ERROR flag to
 *			PF_PORTERROR as per Pete Keilty's SCS fix.
 *
 *			Stop calling uq_poll_rspring from uq_cleanup as
 *			it is too optimistic to assume any packet we take
 *			is a valid one.	Also, clean up below fix by raising
 *			ipl in uq_timer around call to uq_dispatch.
 *
 *	Sep 1990	Matthew Sacks
 *			SMP bug fix: raised ipl around calls to Lock_pccb
 *			in uq_dispat, uq_conn, uq_dcon, and around call to
 *			Lock_scadb in uq_probe.  This is not actually a
 *			bug unless SMP is turned on for DSA.
 *
 *	05-Sep-90       Matthew Sacks
 *			Change uq_port_reset so that bisst is only called
 *			once.  This is to prevent kdb50 init thrashing.
 *
 *	09-Aug-90	Matthew Sacks
 *			SMP bug fix: added missing "else" before Unlock_pccb
 *			in uq_send_msg.
 *
 *	03-Aug-90	Ali Rafieymehr
 *			Changes for Stuart Hollander to allow multiple
 *			xmi support.
 *	
 *	May 1990	Matthew Sacks
 *			Implemented SMP safe manipulation of the SSP
 *			ring structures, and SMP safe initialization.
 *			This required substantial changes to most
 *			subroutines.
 *
 *			Also changed uq_init so that the wait loop on
 *			STEP1 will drop priority during each iteration.
 *
 *	26-Dec-89	Robin
 *		1.	Changed the code where RISC buff flush is done to
 *			make the default be a flush and the coded case to 
 *			be not flush.  This is because the most likely 
 *			case will be machines that need the
 *			help and this will require less cahnges in 
 *			the new systems we support (maybe?).
 *
 *	1-Dec-1989	Matthew S Sacks
 *		1.	Changed uq_port_reset to correctly init the
 *			the KDM70.
 *		2.	Changed uq_probe and uq_port_reset to save the
 *			BDA vector, etc. only once, at probe time, in
 *			case they are needed for a port re-init.
 *
 *	09-Nov-89	David E. Eiche		DEE0080
 *		Changed uq_probe() to fill in the new software and
 *		interconnect type fields in the LPIB.
 *
 *	09-Nov-89	David E. Eiche		DEE0079
 *		Changed uq_map_buf() to zero unused fields in the buffer
 *		handle.
 *
 *	20-Jul-1989	Mark A. Parenti
 *		1. Change access to ssp registers from structure template
 *			to pointer registers.  This is needed because
 *			XMI port registers are not the same size as
 *			the UQ and BI registers.
 *		2. Make the reset macro into a function call.
 *		3. Add support for XMI port.
 *		4. Add code for SSP scratchpad ECO.
 *
 *	17-Jan-1989	Todd M. Katz		TMK0003
 *		1. The macro Scaaddr_lol() has been renamed to Scaaddr_low().
 *		   It now accesses only the low order word( instead of low
 *		   order longword ) of a SCA system address.  The macro
 *		   Scaaddr_hos() has been renamed to Scaaddr_hi().  Make use of
 *		   the new macro Scaaadr_mid().
 *		2. Include header file ../vaxmsi/msisysap.h.
 *		3. Use the ../machine link to refer to machine specific header
 *		   files.
 *
 *	17-Oct-1988	Pete Keilty
 *		Changed uq_cleanup to Kfork uq_init if NULL pb otherwise
 *		we add up in a hung state, failed to init uq port.
 *
 *	28-Sept-1988	Mark Parenti
 *		Fix spl bracketing to prevent mchk panics
 *
 *	19-Aug-1988	Todd M. Katz			TMK0002
 *		1. Change all instances of PF_FATALERROR -> PF_ERROR as no
 *		   current instance of crashing the local port is fatal.
 *		2. Make the following modifications to uq_crash_path():
 *			1) Apply the path crash severity modifier( ESM_PC ) to
 *			   the path crash reason code whenever the path is
 *			   open.
 *			2) Use the SCS path crash reason mapping table
 *			   appropriate for the path crash code( scs_map_pc[]
 *			   has been split into scs_map_pc[] and scs_map_spc[]).
 *			3) The routine parameter scsbp ALWAYS points to a
 *		   	   character string of size NAME_SIZE instead of to the
 *			   SCS header of a datagram/message buffer whenever SCS
 *			   invokes the routine with a reason code of E_SYSAP.
 *			   This character string consists of the name of the
 *			   local SYSAP responsible for crashing the path.
 *
 *	18-July-1988 -- map 
 *		Enable last fail packet.  Dynamically allocate data structures.
 *		Use sca Cntr_from_num macro to determine controller name.
 *
 *	18-Mar-1988 -- map
 *		Clear QB bit in step1r in reinitialization case.  This is
 *		already done in probe().  Clearing this bit is necessary
 *		because of a KDB microcode bug.
 *
 *	08-Mar-1988		Todd M. Katz		TMK0001
 *		Make the following changes:
 *		1. Eliminate the routine uq_crash_lport().  It is not used
 *		   internally and port drivers are no longer required to supply
 *		   for use by SCS a routine for crashing a local port.
 *		2. Reference scs_map_pc instead of uq_map_genpc and delete the
 *		   latter.
 *		3. Include new header files ../vaxscs/scaparam.h,
 *		   ../vaxmsi/msisysap.h, and ../vaxmsi/msiscs.h.
 *
 *	08-Mar-1988 -- map
 *		Change ring access to be done in two steps.  This is required
 *		because of hardware which reads the ring entry in two reads.
 *		The first read would get the old buffer address and the
 *		second read would get the new high order resulting in an
 *		old command being issued twice.
 *		Also add spl() calls in various places to insure proper ipl.
 *
 *	22-Feb-1988 -- Robin
 *		Removed unused ref. to uqdinfo
 *
 *	18-Feb-1988 -- map
 *		Make sure have correct IPL when calling poll_cring().
 *
 *	15-Feb-1988 -- map
 *		Cleanup connection handshake code to allow for multiple
 *		simultaneous connect requests.
 *
 *	15-Jan-1988 -- map
 *		Enable last fail packet.	
 */

/* Libraries and Include Files.
 */

#include	"uq.h"

#include 	"../machine/pte.h"
#include 	"../h/param.h"
#include 	"../h/dyntypes.h"
#include 	"../h/systm.h"
#include 	"../h/buf.h"
#include 	"../h/conf.h"
#include 	"../h/kmalloc.h"
#include 	"../h/dir.h"
#include 	"../h/user.h"
#include 	"../h/map.h"
#include 	"../h/vm.h"
#include 	"../h/vmmac.h"
#include 	"../h/dk.h"
#include 	"../h/cmap.h"
#include 	"../h/uio.h"
#include 	"../h/ioctl.h"
#include 	"../h/fs.h"
#include	"../h/cpudata.h"
#include	"../h/smp_lock.h"

#include 	"../machine/cpu.h"
#include	"../../machine/common/cpuconf.h"
#include 	"../io/uba/ubareg.h"
#include 	"../io/uba/ubavar.h"
#ifdef vax
#include 	"../machine/mtpr.h"
#endif vax
#include 	"../io/sysap/mscp_msg.h"


#include 	"../io/bi/bireg.h"
#include 	"../io/bi/buareg.h"
#include 	"../io/bi/bdareg.h"

#include	"../io/xmi/xmireg.h"
#include	"../io/xmi/sspxmi.h"

#include	"../h/types.h"
#include	"../h/errlog.h"
#include	"../h/ksched.h"
#include	"../io/scs/sca.h"
#include        "../io/scs/scaparam.h"
#include	"../io/ci/cippdsysap.h"
#include	"../io/ci/cisysap.h"
#include	"../io/msi/msisysap.h"
#include	"../io/bi/bvpsysap.h"
#include	"../io/gvp/gvpsysap.h"
#include	"../io/uba/uqsysap.h"
#include	"../io/sysap/sysap.h"
#include	"../io/ci/cippdscs.h"
#include	"../io/ci/ciscs.h"
#include        "../io/msi/msiscs.h"
#include	"../io/bi/bvpscs.h"
#include	"../io/gvp/gvpscs.h"
#include	"../io/uba/uqscs.h"
#include	"../io/scs/scs.h"
#include	"../io/uba/uqppd.h"
#include	"../io/uba/uqport.h"

#ifdef mips
#define	WBFLUSH	wbflush()
#else
#define WBFLUSH ;
#endif mips

/* Routine definitions.
 */

extern struct xmidata xmidata[];
extern	void	uq_conn(),uq_dcon(),uq_init(),uq_cleanup(),uq_error_log(), 
	uq_cred(), uq_port_reset();
extern	void	scs_unix_to_vms();
extern	struct 	lock_t lk_scadb;
extern	PB	*uq_get_pb();
extern	SCSH	*uq_alloc_msg(), *uq_remove_msg(), *uq_alloc_dg(), *uq_remove_dg();
extern 	void	uq_unmap_buf(), uq_dealloc_msg(), uq_remove_pb(),
		scs_dealloc_pb(), scs_dealloc_sb(), uq_crash_path(),
                uq_dealloc_dg(), uq_send_msg(), uq_add_msg(),
		uq_add_dg(), uq_set_reg();

extern  u_short scs_map_pc[], scs_map_spc[];
extern	u_long	boottime;
extern	u_long	uq_map_buf(), uq_mstart(), uq_open_path(), uq_mreset(),
		uq_create_sys(), step_init(), step_wait();


PIB	uq_pib;			/* UQ Path Information Block		*/
SIB	uq_sib;			/* UQ System Information Block		*/
int	uqerror = 1;		/* Set if Last Fail Packet is desired	*/
int	uqdebug = 0;		/* Used to trigger debug printf's	*/
int	uq_alloc_cnt = 0;
int	uq_dealloc_ring = 0;
int	uq_dealloc_free = 0;


PDT	uqpdt	={		/* UQSSP Port Dispatch Table		*/
		uq_alloc_dg,	/* allocate_dg 		(supported)	*/
		uq_dealloc_dg,	/* deallocate_dg	(supported)	*/
		uq_add_dg,	/* add_dg 		(supported)	*/
		uq_remove_dg,	/* remove_dg 		(supported)	*/
		uq_alloc_msg,	/* allocate_msg 	(supported)	*/
		uq_dealloc_msg,	/* deallocate_msg 	(supported)	*/
		uq_add_msg,	/* add_msg 		(supported)	*/
		uq_remove_msg,	/* remove_msg 		(supported)	*/
		uq_send_msg,	/* send_msg 		(supported)	*/
		uq_map_buf,	/* map_buffer 		(supported)	*/
		uq_unmap_buf,	/* unmap_buffer 	(supported)	*/
		uq_open_path,	/* open_path 		(supported)	*/
		uq_crash_path,	/* crash_path 		(supported)	*/
		uq_get_pb,	/* get_pb 		(supported)	*/
		uq_remove_pb,	/* remove_pb 		(supported)	*/
		uq_mreset,	/* maint_reset 		(supported)	*/
		uq_mstart,	/* maint_start 		(supported)	*/
		0,		/* send_dg 		(unsupported)	*/
		0,		/* send_data 		(unsupported)	*/
		0,		/* request_data		(unsupported)	*/
                0,              /* crash_lport          (unsupported)   */
		0		/* shutdown 		(unsupported)	*/
	
	   };
int     uq_probe(), uqintr(), uq_attach(), uq_timer(), uq_slave();
/*
 *	External definitions
 */

extern pccbq scs_lport_db;
extern sbq scs_config_db;
extern int hz;
extern int cpu;
extern int numxmi;
extern int scs_initialize();
extern PB *scs_alloc_pb();
extern SB  *scs_alloc_sb();
extern struct xmidata *get_xmi();

/* 	Port Info Block
 */

extern struct port_info *port_info_ptr[];


extern struct	uba_ctlr *uqminfo[];

#define CTRLNAME "uq"

u_short udstd[] = { 0772150, 0772550, 0777550, 0 };
struct  uba_driver uqdriver =
{ uq_probe, 
uq_slave, 
uq_attach, 
0, 
udstd, 
"ra", 
0, 
CTRLNAME, 
uqminfo, 
0};



#define DEBUG 1


/* Define debugging stuff.
 */
#ifdef DEBUG
#define Cprintf if(uqdebug) printf
#define Dprintf if( uqdebug >= 2 )cprintf
#else
#define Cprintf ;
#define Dprintf ;
#endif
#ifdef DEBUG
#define printd  if (uqdebug) printf
#define printd1  if (uqdebug > 1) printf
#define	printd10 if(uqdebug >= 10) printf
#endif

#ifdef DEBUG
#define cpu_printf(x,y,r) if(uqdebug) {char	char_cpu_tmp;\
printf("%s: current cpu is %d, the init leader is %d,\
the reset leader is %d\n",\
r, (char_cpu_tmp = (CURRENT_CPUDATA->cpu_num)), x, y);}
#else
#define	cpu_printf ;
#endif
#define STEP1MASK       0174177
#define STEP1GOOD       (UQ_STEP2|(NCMDL2<<3)|NRSPL2)
#define STEP2MASK       0174377
#define STEP2GOOD       (UQ_STEP3|UQ_IE|(Pccb.uq_ivec/4))
#define STEP3MASK       0174000
#define STEP3GOOD       UQ_STEP4
#define DELAYTEN 1000
#define	DEV_IPL 0x17
#define	b_ubinfo	b_resid		/* Unibus mapping info per buffer */

#define	isbda(pccb)	(Lpinfo.uq_type == BDA_TYPE)
#define	SA_W(pccb)	*(isbda(pccb)? (Pccb.Uqsaw): (Pccb.Uqsa) )
#define Wait_step(mask, result)	{ 					\
		if ((*Pccb.Uqsa & mask) != result) {			\
			int count = 0;					\
			while((*Pccb.Uqsa & mask) != result) {	\
				DELAY(10000);				\
				count++;				\
				if(count > DELAYTEN ) break;		\
			}						\
			if(count > DELAYTEN ) {				\
				Lpinfo.uq_state = S_IDLE;		\
				Lpinfo.sa = *Pccb.Uqsa;		\
				uq_error_log( pccb, UQ_RESET_FAIL );	\
				uq_disable(pccb, PF_PORTERROR);		\
				return;					\
			}						\
		}							\
	}

/*
 *
 *
 *	Name:		uq_probe
 *	
 *	Abstract:	Probe entry point for configure. This routine will
 *			initialize the controller and set up the required
 *			data structures for SCS.
 *			
 *	Inputs:
 *
 *	reg			- Address of controller
 *	ctlr			- Controller number
 *	
 *
 *	Outputs:
 *
 *	pccb			- Port Command and Control Block allocated
 *	  pdt 			- Address of this port's PDT
 *	  pd.uq.uq_ctlr		- Controller number
 *	  pd.uq.uqaddr		- Address of controller device
 *	  pd.uq.scs		- Address of SCS Accpt/Disconnect buffer
 *	  pd.uq.stepxr		- Step 1-4 read data
 *	  lpinfo.uq.uq_state	- Current state of controller
 *
 *	pcinfo			- Table of pccb's indexed by controller number
 *	  pc_ptr		- Pointer to pccb for this controller
 *	
 *	SMP:			lockinit is called to initialize the
 *				the pccb's lock structure
 *
 *	Return 
 *	Values:
 *
 *	0			- Controller does not exist or cannot be
 *				  initialized.
 *
 *	sizeof(device)		- If the controller exists and can be 
 *				  initialized the size of the uqdevice
 *				  structure is returned. The uqdevice
 *				  structure represents the I/O page device
 *				  registers.
 *	
 * 
 *	Side		
 *	Effects:		- The controller is initialized. The necessary
 *				  SCS structures are allocated and
 *				  initialized. The "system" is made known to
 *				  SCS which passes this information on to
 *				  interested SYSAP's.
 *
 *				
 */

int	uq_probe(reg, ctlr)
	caddr_t	reg;
	int	ctlr;
{
	register PCCB *pccb;
	int count, i, s;
	struct port_info *pcinfo;
	UQ_SCP *scp;
	int intr_dest;
	u_long intr_vec;
	struct xmidata *xmidata;
	struct bda_regs	*bdar;

#ifdef mips
	KM_ALLOC(pccb, PCCB *,sizeof(PCCB),KM_SCA, KM_NOW_CL_CA | KM_NOCACHE )
#else
	KM_ALLOC(pccb, PCCB *,sizeof(PCCB),KM_SCA, KM_NOW_CL_CA )
#endif mips
	if ( pccb == (PCCB *)NULL )
		return(0);
	U_long( pccb->size ) = sizeof( PCCB );	/* Size of PCCB		*/
	pccb->type = DYN_PCCB;		/* Structure type		*/
	pccb->pdt = &uqpdt;		/* Init PDT pointer		*/
	Pccb.uq_ctlr = ctlr;		/* Save ctlr number		*/
	Pccb.reinit_cnt = UQ_MAX_REINIT; /* Initialize reinit counter	*/

	if (uba_hd[numuba].uba_type & UBABDA)
		Lpinfo.uq_type = BDA_TYPE; /* BDA is a special case and must */
					 /* be flagged now		   */
	if (uba_hd[numuba].uba_type & UBAXMI) {
		Lpinfo.uq_type = KDM_TYPE; /* KDM is also a special case and */
					   /* must be flagged now	     */
		xmidata = get_xmi(numxmi);
		Pccb.xmipd = (xmidata->xmiintr_dst) | UQ_XMI_LEVEL15;
	}

	/* the next line must be done before calling uq_port_reset */
	Pccb.init_leader = Pccb.reset_leader = CURRENT_CPUDATA->cpu_num;

	Lpinfo.uq_flags |= UQ_PRB;	/* Flag that we are in probe	 */
	uq_set_reg(pccb, reg);		/* Set up register pointers	*/
	if (Lpinfo.uq_type == BDA_TYPE) {
		bdar = (struct bda_regs *)((long)(Pccb.Uqip) - UQB_IP);
		Pccb.bda_init_vec = bdar->bda_biic.biic_int_ctrl;
		Pccb.bda_init_dest = bdar->bda_biic.biic_int_dst;
		Pccb.bda_init_errvec = bdar->bda_biic.biic_err;
	}
	uq_port_reset(pccb);		/* Bang it on the head		 */
	count = 0;
	while (count < DELAYTEN){	/* Wait for at most 10 seconds 	 */
		if ((*Pccb.Uqsa & UQ_STEP1) != 0)
			break;		/* We woke it up		*/
		DELAY(10000);
		count = count +1;
	}
	if (count == DELAYTEN) return(0);	/* Not there or dead 	*/
	if ( (Lpinfo.sa = *Pccb.Uqsa) & UQ_ERR) {
		uq_error_log( pccb, UQ_SA_FATAL);
		return(0);		/* If error then return 	*/
	}
	

#ifdef mips
	KM_ALLOC( pcinfo, struct port_info *, sizeof(struct port_info), 
			KM_SCA, KM_NOW_CL_CA | KM_NOCACHE)
#else
	KM_ALLOC( pcinfo, struct port_info *, sizeof(struct port_info), 
			KM_SCA, KM_NOW_CL_CA )
#endif mips
	if ( pcinfo == (struct port_info *)NULL )
		return(0);
	pcinfo->pc_ptr = pccb;		/* Save address for later use	*/
	Pccb.uq = &pcinfo->uq;		/* Save address of comm area	*/
	port_info_ptr[ctlr] = pcinfo; /* Save pointer to port_info struct */
	uq_init_buffers( pccb );	/* Map communications area 	*/


/* Setup bus-specific information					*/
	if (uba_hd[numuba].uba_type & UBAXMI) {
		/* Allocate controller scratchpad area */
		KM_ALLOC( scp, UQ_SCP *, UQ_SCP_SIZE, KM_SCA, KM_NOW_CL_CA )
		if ( scp == (UQ_SCP *)NULL )
			return(0);
		Pccb.uqscp = (struct _uqscp *)svtophy(scp);
	}

/* Setup controller-specific information				*/
	switch(Lpinfo.uq_type) {

	      case BDA_TYPE:
		Pccb.uq_ivec = 0;	/* BDA gets vector from BIIC reg */
		break;

	      case KDM_TYPE:
		Pccb.uq_ivec = (uba_hd[numuba].uh_lastiv -= 4);	
		break;

	      default:
		Pccb.uq_ivec = (uba_hd[numuba].uh_lastiv -= 4);
		break;
	}

/*									*/
/* Initialize step information						*/
/*									*/

	Pccb.step1w = (((uba_hd[numuba].uba_type & UBAXMI) != 0) ? 0 : UQ_ERR )
		| (NCMDL2<<11) | (NRSPL2<<8) | UQ_IE | (Pccb.uq_ivec/4);
	Pccb.step2w = ((int)&Pccb.uqptr->uqca.ca_ringbase)|
		    (((uba_hd[numuba].uba_type & UBA780) != 0) ? UQ_PI : 0);
	Pccb.step3w = ((int)&Pccb.uqptr->uqca.ca_ringbase)>>16;


/*									*/
/*	initialize the SMP lock						*/
/*									*/
	Init_pccb_lock(pccb);


/*									*/
/*	Initialize the port						*/
/*									*/


	if (Lpinfo.uq_type == KDM_TYPE) {
		*Pccb.Uqpd = Pccb.xmipd; /* Write interrupt info in PD reg */
	}
	Lpinfo.uq_state = S_STEP1;	/* Port now in STEP1 state	   */
	Pccb.step1r = *Pccb.Uqsa;	/* Save step data in case of error */
	if (Lpinfo.uq_type == BDA_TYPE) 
		Pccb.step1r &= ~UQ_QB;	/* Necessary to turn off bit    */

	SA_W(pccb) = Pccb.step1w; /* Write step 1 data 		*/
	if(!(step_wait( pccb, STEP1MASK, STEP1GOOD))) /* Wait for step 2 */
		return(0);		/* Step	failed			*/
	Pccb.step2r = *Pccb.Uqsa;	/* Save step data in case of error */
	Lpinfo.uq_state = S_STEP2;	/* Port now in STEP2 state	*/

	SA_W(pccb) = Pccb.step2w; /* Write step 2 data 		*/
	if(!(step_wait( pccb, STEP2MASK, STEP2GOOD))) /* Wait for step 3 */
		return(0);		/* Step failed			*/
	Pccb.step3r = *Pccb.Uqsa;	/* Save step data in case of error */
	Lpinfo.uq_state = S_STEP3;	/* Port now in STEP3 state	*/

	SA_W(pccb) = Pccb.step3w; /* Write step 3 data 		*/
	if(!(step_wait( pccb, STEP3MASK, STEP3GOOD))) /* Wait for step 4 */
		return(0);		/* Step failed			*/
	Pccb.step4r = *Pccb.Uqsa;	/* Save step data in case of error */
	Lpinfo.uq_type = (Pccb.step4r>>4) & 0x7F;
	Lpinfo.uq_state = S_STEP4;	/* Port now in STEP3 state	*/


/*									*/
/*	Set up SCS info							*/
/*									*/

	pccb->lpinfo.name = Ctrl_from_num( "uq  ", Pccb.uq_ctlr); /* Local port name	*/
	Scaaddr_low( pccb->lpinfo.addr ) = *(u_short *)&Pccb.uq_ctlr; /* Remote port address */
	Scaaddr_mid( pccb->lpinfo.addr ) = 0;
	Scaaddr_hi( pccb->lpinfo.addr ) = 0;
	pccb->lpinfo.type.hwtype = HPT_UQSSP; /* Local port hardware type */
	pccb->lpinfo.type.swtype = SPT_UQSSP; /* Local port software type */
	if( Lpinfo.uq_type == BDA_TYPE ) {    /* Local port interconnect  */
	        pccb->lpinfo.type.ictype = ICT_BI;
        } else if( Lpinfo.uq_type == KDM_TYPE ) {
		pccb->lpinfo.type.ictype = ICT_XMI;
	} else if( Pccb.step1r & UQ_QB ) {
		pccb->lpinfo.type.ictype = ICT_QB;
	} else {
		pccb->lpinfo.type.ictype = ICT_UB;
	}
	pccb->lpinfo.type.dual_path = 0; /* Single path port		*/


/*									*/
/*	Insert PCCB on system queue					*/
/*									*/
	s = Splscs();
	Lock_scadb ();
	Insert_entry(pccb->flink, scs_lport_db); 
	Unlock_scadb ();

	/* keep ipl up for scs call */
	if((i = scs_initialize()) != RET_SUCCESS){ /* Init the SCS layer */
		(void)splx(s);		/* Lower IPL			*/
		Remove_entry(pccb->flink); /* Remove from lport_db queue */
		uq_port_reset(pccb);	 /* Reset the device		*/
		Lpinfo.uq_flags &= ~UQ_PRB; /* We are now out of probe	*/
		return(0);		/* Return with err if init fails */
	}

	(void)splx(s);		/* Restore IPL			*/
	uq_init_buffers( pccb );	/* Restock the response ring and */
					/* free queue after controller	*/
					/* stomps on ring.		*/

	Pccb.step4w =  (( (uq_cinfo[Lpinfo.uq_type].burst & UQ_BURST) > 0 ? 
			(uq_cinfo[Lpinfo.uq_type].burst & UQ_BURST) : 0) <<2)
				| UQ_GO | (uqerror ? UQ_LF : 0);
	if (uba_hd[numuba].uba_type & UBAXMI) {
	    Pccb.uq->uqca.ca_scp_size = UQ_SCP_SIZE; /* Size of scratchpad */
	    Pccb.uq->uqca.ca_scp_add = (u_long)Pccb.uqscp; /* Physical address of scratchpad */
	    Pccb.step4w |= UQ_CS;
        }
	SA_W(pccb) = Pccb.step4w; /* Enable the controller 	*/
	Lpinfo.uq_state = S_RUN;	/* Controller is online		*/

	if(!(uq_create_sys( pccb ))) {	/* Create new system		*/
		return(0);		/* Couldn't create the system	*/
	}

	if(Pccb.poll_rate == 0) 	/* Set the rate for the sa poll	*/
		Pccb.poll_rate = hz;

/*									*/
/*	Start the SA register watchdog timer. This time will not start	*/
/*	running until the timer queues are enabled.			*/
/*									*/

	if ( (Lpinfo.uq_flags & UQ_TIM) == 0 ) { /* If timer not yet on */
		Lpinfo.uq_flags |= UQ_TIM; /* Indicate timer now on	*/
		timeout(uq_timer,pccb,Pccb.poll_rate); /* Start timer	*/
	}

	Pccb.init_leader = Pccb.reset_leader = UQPath_Is_Up;

	Lpinfo.uq_flags &= ~UQ_PRB;	/* We are now out of probe	*/
/*
 *	Return size of device structure.  Because the uq device structure
 *	contains an extra word for the KDB50 we must check the device type
 *	and return the size based on that type.  For all but the KDB the
 *	size is struct uqdevice less one short.
 */

	return( Pccb.uqregsize );
}

/*
 *
 *
 *	Name:		uq_port_reset
 *	
 *	Abstract:	Perform reset of controller
 *			
 *	Inputs:
 *
 *	pccb				- pointer to PCCB for this port
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	SMP:		This code should only be entered by the cpu
 *			currently equal to init_leader, although no
 *			lock is currently asserted.  Because of this
 *			design, no calls to lock primitives are done.
 *			In other words, the caller of uq_port_reset is
 *			obligated to take care of smp-safety.
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

void	uq_port_reset( pccb )	/* Reset controller */
PCCB	*pccb;
{
unsigned int	count;

						/* A debugging macro */
	cpu_printf (Pccb.init_leader, Pccb.reset_leader, "uq_port_reset");

	switch(Lpinfo.uq_type) {
	      case BDA_TYPE:

		{
		struct bda_regs	*bdar= (struct bda_regs *)((long)(Pccb.Uqip) - UQB_IP); 
		bisst(&bdar->bda_biic.biic_ctrl);

		bdar->bda_biic.biic_int_ctrl = (Pccb.bda_init_vec & 0x0ffff); 
		bdar->bda_biic.biic_int_dst = Pccb.bda_init_dest; 
		bdar->bda_biic.biic_err = Pccb.bda_init_errvec; 
	        }
		break;

	      case KDM_TYPE:

		{
		struct xmi_reg *xmir= (struct xmi_reg *)((long)(Pccb.Uqip) - UQX_IP); 
		u_long vec;

		if(Pccb.reinit_cnt >= (UQ_MAX_REINIT-1)) { 
			/* Try soft reset first */

			*Pccb.Uqip = 1;
			xmir->xmi_xbe = xmir->xmi_xbe & XMI_NHALT;
			DELAY(600000);		/* delay 600 msec. */
			count = 0;
			while (count < 400) {
				DELAY(1000)	  /* wait one msec. */
				count++;
				if   ((!(xmir->xmi_xbe & XMI_XBAD)) &&
				          (!(xmir->xmi_xbe & XMI_STF)))
						break;
			}  /* end while */
			/* if the while loop timed out, do hard init */
			if (count >= 400) xmisst(xmir);
				else break;  /* i.e, continue soft init */
		}
		else {		
		     /* Else hard init */
		     xmisst(xmir);
		}
	        }
		break;

	      default:

		*Pccb.Uqip = 0;  
		break;
	}
	return;
}

/*
 *
 *
 *	Name:		uq_set_reg
 *	
 *	Abstract:	Setup uq port register pointers.
 *			
 *	Inputs:
 *
 *	pccb				- pointer to PCCB for this port
 *	reg				- Base address of IP register
 *	
 *	
 *
 *	Outputs:	The port register offsets are added to the IP
 *			physical address to get the physical address for
 *			each of the port registers.  The offsets are 
 *			bus-dependent.
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
 *	Effects:	Port registers are accessible via the pccb fields.
 *
 */

void	uq_set_reg( pccb, reg )	/* Set up pointers to IO registers	*/
register PCCB *pccb;
caddr_t	reg;
{
	if (uba_hd[numuba].uba_type & UBABDA) {
		Pccb.Uqip = ( u_short * )(( u_char * )reg + UQB_IP_OFF);
		Pccb.Uqsa = ( u_short * )(( u_char * )reg + UQB_SA_OFF);
		Pccb.Uqsaw = ( u_short * )(( u_char * )reg + UQB_SAW_OFF);
		Pccb.uqregsize = 6;
	} else 
	if (uba_hd[numuba].uba_type & UBAXMI) {
		Pccb.Uqip = ( u_short * )(( u_char * )reg + UQX_IP_OFF);
		Pccb.Uqsa = ( u_short * )(( u_char * )reg + UQX_SA_OFF);
		Pccb.Uqpd = ( u_long * )(( u_char * )reg + UQX_PD_OFF);
		Pccb.uqregsize = 12;
	} else {
		Pccb.Uqip = ( u_short * )(( u_char * )reg + UQ_IP);
		Pccb.Uqsa = ( u_short * )(( u_char * )reg + UQ_SA);
		Pccb.uqregsize = 4;
	}


}



/*
 *
 *
 *	Name:		step_wait
 *	
 *	Abstract:	Waits for the specified initialization step to
 *			complete.
 *			
 *	Inputs:
 *
 *	pccb				- pointer to PCCB for this port
 *	mask				- mask of interesting bits
 *	result				- what those bits should be
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

u_long	step_wait( pccb, mask, result )		/* Wait for step to complete */
PCCB	*pccb;
short	mask;
short	result;
{
register int count;


	count = 0;
	while (count < DELAYTEN){	/* Wait for at most 10 seconds */
		if((*Pccb.Uqsa & mask) == result)
			break;		/* Got the step - hoo ray 	*/

		DELAY(10000);		/* Spin our wheels for 10 ms 	*/
		count = count+1;
	}
	if (count == DELAYTEN) 	{ /* If we timed out then return failure */
		return(0);
        }
	else 
		return(RET_SUCCESS);
	
}	

/*
 *
 *
 *	Name:		uq_create_sys	
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
 *	     pd.uq.uq_ctlr	- Controller number
 *	     pd.uq.step4r	- Data read during step 4 of initialization
 *	     lpinfo.name	- Local port name
 *	     
 *	
 *	
 *
 *	Outputs:
 *
 *	pccb			- Pointer to PCCB for this port
 *
 *	     lpinfo.uq.uq_type	- UQSSP model number for this device
 *	     pd.uq.pb		- Pointer to PB for this port
 *
 *	sb			- System block for this port
 *
 *	     various fields initialized
 *
 *	pb			- Path Block for this port
 *
 *	     various fields initialized
 *	
 *	SMP: 	locking not neccessary because only one cpu-thread is
 *		permitted to an initialization.
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
u_long	uq_create_sys(pccb)
PCCB		*pccb;
{
	register PIB	*pib = &uq_pib;
	register SIB	*sib  = &uq_sib;
	PB	*pb;
	SB	*sb;
	int	i, s;

	
	cpu_printf (Pccb.init_leader, Pccb.reset_leader, "uq_create_sys");

	(void)bzero( pib, sizeof(PIB)); /* Clear pib			*/
	(void)bzero( sib, sizeof(SIB));   /* Clear sib			*/

	pib->lport_name = pccb->lpinfo.name; /* Local port name	*/
	pib->type.hwtype = HPT_UQSSP;	/* Local port type		*/
	pib->type.dual_path = 0;	/* Single path port		*/
	Scaaddr_low( pib->rport_addr ) = *(u_short *)&Pccb.uq_ctlr; /* Remote port address */
	Scaaddr_mid( pib->rport_addr ) = 0;
	Scaaddr_hi( pib->rport_addr ) = 0;

/*	If we can't get a pb then it's no go				*/
	s = Splscs();	/* Set IPL to SCS level	*/
	if((pb = scs_alloc_pb( PS_OPEN, pccb, pib )) == (PB *)NULL) {
		(void)splx(s);		/* Reset IPL			*/
		return(0);
	}
	(void)splx(s);		/* Reset IPL			*/

	sib->max_msg = sizeof(MSCP_MAXBUF); /* 	Max message size	*/
	sib->npaths = 1;		/* Only 1 path to these devices	*/
	bcopy("UQ  ",&sib->swtype,4);	/* Software type is UQ port	*/
	bcopy("3.00",&sib->swver,4);	/* Software version		*/
	(void)scs_unix_to_vms((struct timeval *)&boottime, &sib->swincrn);
	bcopy(uq_cinfo[Lpinfo.uq_type].name,&sib->hwtype,4); /* Controller type in ASCII*/
	sib->hwver.val[0] = (long)(Pccb.step4r & 0xF); /* Microcode version number	*/
	sib->hwver.val[1] = (long)Pccb.uq_ctlr; /* Controller number	*/
	sib->hwver.val[2] = (long)Lpinfo.uq_type; /* Controller model code */
	*(u_long *)sib->node_name = pccb->lpinfo.name; /* SCA Node name	*/
	bcopy("    ",(u_long *)sib->node_name+1,4);
/*	The system id of uq controllers is as follows:			*/
/*									*/
/*      47             32 31                                 0          */
/*	+----------------+------------------------------------+		*/
/*	!		 !				      !		*/
/*	! UQSSP model #  !   Device Address (CSR)	      !		*/
/*	!		 !				      !		*/
/*	+----------------+------------------------------------+		*/
/*									*/
	{
	register u_long	addr_tmp = *(u_long *)&Pccb.Uqip;

	Scaaddr_low( sib->sysid ) = ( addr_tmp & 0xffff );
	Scaaddr_mid( sib->sysid ) = ( u_short )(( addr_tmp >> 16 ) & 0xffff );
	Scaaddr_hi( sib->sysid ) = *(u_short *)&Lpinfo.uq_type;
	}

/*	If we can't get a sb then it's no go				*/
	s = Splscs();	/* Set IPL to SCS level	*/
	if((sb = scs_alloc_sb( sib )) == (SB *)NULL) {
		(void)scs_dealloc_pb( pccb,pb ); /* Deallocate PB	*/
		(void)splx(s);		/* Reset IPL			*/
		return(0);
	}
	(void)splx(s);		/* Reset IPL			*/

	for (i=0; i<NCON; i++)
		Lpinfo.uq_credits[i] = 1; /* Give initial credit of 1	*/


	s = Splscs();	/* Set IPL to SCS level	*/

	Lock_pccb (pccb);
	Pccb.pb = pb;			/* Save on pccb			*/
	pb->sb = sb;			/* Point to System Block	*/
	Insert_entry(pb->flink,Sb->pbs); /* Insert PB on SB queue	*/
	Unlock_pccb (pccb);

	Lock_scadb ();
	Insert_entry(sb->flink,scs_config_db); /* Insert SB on system-wide */
					       /* configuration database   */
	Unlock_scadb ();

	scs_new_path( sb, pb );
	(void)splx(s);		/* Reset IPL			*/

	return(RET_SUCCESS);
}

/*
 *
 *
 *	Name:		uq_init_buffers
 *	
 *	Abstract:	Map the communications area and packet buffers.
 *			Initialize the ring pointers and place the
 *			packet buffers on the response ring or free queue.
 *			
 *	Inputs:
 *
 *	pccb			- Pointer to PCCB for this port
 *	     pd.uq.uq_mapped	- Set to 1 if the communications area has
 *				  been mapped.
 *
 *	ctlr			- Controller number
 *	
 *	
 *
 *	Outputs:
 *
 *	pccb
 *	     pd.uq.uqptr	- Unibus address of communcations/packet area
 *	     pd.uq.uq_ubainfo	- Mapping information for communications area
 *	     pd.uq.uq_mapped	- Set to 1 if the comm area is mapped
 *	     pd.uq.rsp_cnt	- Number of used response ring entries
 *	     pd.uq.cmd_cnt	- Number of used command ring entries
 *	     pd.uq.rsprindx	- Index of next empty response ring entry 
 *	     pd.uq.cmdrindx	- Index of next empty command ring entry
 *	     pd.uq.uq_lastrsp	- Index of next response ring entry to poll
 *	     pd.uq.uq_lastcmd	- Index of next command ring entry to poll
 *	     pd.uq.uq_freel	- List of free packet buffers
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
uq_init_buffers(pccb)
PCCB *pccb;
{
	register UQ *uq;
	register UQ *uuq;
	int i;

	cpu_printf (Pccb.init_leader, Pccb.reset_leader, "uq_init_buffers");
	uq = Pccb.uq;
	if (Pccb.uq_mapped == 0) {	/* Need to map communication area */
				/* and command/response packet buffers */
		if ((Lpinfo.uq_type == BDA_TYPE) || 
		    (Lpinfo.uq_type == KDM_TYPE)) {
			Pccb.uqptr = (UQ *) (Pccb.uq_ubainfo = svtophy(uq));
		}
		else {
		     if(Pccb.step1r & UQ_QB) 
				   /* it's a Q-22 bus so use all 8k map reg */
				Pccb.uq_ubainfo = qballoc(numuba, (caddr_t)uq,
						sizeof ( UQ ), 0);
			else
			    Pccb.uq_ubainfo = uballoc(numuba, (caddr_t)uq,
						sizeof ( UQ ), 0);
			if(uba_hd[numuba].uba_type & UBAUVI)
				Pccb.uqptr = (UQ *)(Pccb.uq_ubainfo & 0x3fffff);
			else 
			    if(Pccb.step1r & UQ_QB) 
				  /* it's a Q-22 bus so use all 8k map reg */
				Pccb.uqptr = (UQ *)(Pccb.uq_ubainfo & 0x3fffff);
			     else
				 Pccb.uqptr = (UQ *)(Pccb.uq_ubainfo & 0x3ffff);
		}
		Pccb.uq_mapped = 1;	/* Indicate mapping completed	*/
	}



	if(Lpinfo.uq_type == KDM_TYPE) {
	    uq->uqca.ca_xmi.flags = 0;	/* No flags set			*/
	    uq->uqca.ca_xmi.psi = SSP_PSI_512; /* Use 512 page size for now */
#ifdef notdef
	    uq->uqca.ca_xmi.psi = SSP_PSI_4096;	/* MIPS page size = 4096 */
#endif notdef
	    uq->uqca.ca_xmi.pfn = SSP_PFN_MASK;	/* PFN mask		*/
            }
	Pccb.rsp_cnt = 0;		/* Clear various counters and	*/
	Pccb.cmd_cnt = 0;		/* pointers.			*/
	Pccb.rsprindx = 0;		
	Pccb.cmdrindx = 0;		
	Pccb.uq_lastrsp = 0;
	Pccb.uq_lastcmd = 0;
	Pccb.ncon = 0;			/* Mark connection count as 0	*/
	Pccb.uq_con = 0;		/* Clear connection vector	*/

	Pccb.uq_freel = (uqbq *)NULL;	/* Initialize free list		*/
	Init_queue(Pccb.waitq);		/* Initialize command wait queue */
	Init_queue(Pccb.scswaitq);	/* Initialize SCS command wait queue */

	uuq = Pccb.uqptr;		/* Get unibus address of com area */
	for (i = 0;  i < NBUF; i++) {	/* Process buffers		*/

			/* Save unibus address of buffer */
		Pccb.uq->uq_buf[i].uqh.ua = (long)&uuq->uq_buf[i].app_buf;

		/* Place buffer in rsp ring or on free queue if ring is full */
		uq_ins_rspring((UQH *)&Pccb.uq->uq_buf[i],pccb);
					  	    
	}
}

/*
 *
 *
 *	Name:		uq_ins_rspring	- Insert buffer on response ring
 *	
 *	Abstract:	This routine takes as input a pointer to a buffer.
 *			It checks the response ring for empty slots. If
 *			the ring is not full it places the buffer on the ring.
 *			If the ring is full the buffer is placed on the
 *			free buffer queue.
 *			
 *	Inputs:		
 *
 *	bp			- Pointer to UQ_header portion of buffer to
 *				  insert.
 *	pccb			- Pointer to PCCB for this port
 *		pd.uq.rsp_cnt	- Count of "used" entries in response ring
 *		pd.uq.rsprindx	- Pointer to next location in ring to receive
 *				  packet.
 *	
 *	
 *
 *	Outputs:
 *
 *	pccb
 *	     pd.uq.rsp_cnt	- If buffer added to ring this value is 
 *				  incremented.
 *	     pd.uq.rsprinx	- If buffer added to ring this value is
 *				  incremented mod ring size.
 *	     pd.uq_freel	- If buffer is added to free list this
 *				  location now points to new buffer and
 *				  approriate links are updated.
 *
 *	
 *	SMP
 *		The index into the response ring and the count of buffers
 *		in the ring are incremented.  This must be SMP
 *		protected.
 *	
 *		If the buffer is placed on the Pccb's buffer free list, then
 *		that linked list is modified inside the scope of the lock.
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
uq_ins_rspring( uqbp,pccb )
UQH	*uqbp;				/* Pointer to UQ header in buffer */
PCCB	*pccb;				/* Pointer to PCCB		  */
{
UQPPDH	*uqpbp;
int 	s;

	s = splbio();
	Lock_pccb (pccb);

	if(Pccb.rsp_cnt < NRSP) {	/* Ring not full		*/
		uqpbp = Pos_to_ppdh(uqbp);
		uqpbp->uqp_msglen = sizeof(MSCP_MAXBUF) + sizeof(UQPPDH);

		/*
	 	* NOTE:
	 	*	The following two actions MUST be done in separate
 	 	*	statements.  This is due to hardware which reads the
	 	*	ring entries in two 16-bit reads. A race condition
	 	*	exists if the controller reads the low-order 16-bits
	 	*	first.  It is possible to get the old low-order and
	 	*	the new high-order, resulting in an old command being
	 	*	re-executed.  Make sure that the compiler never
	 	*	optimizes these two statements.
	 	*/
		Pccb.uq->uqca.ca_rspdsc[Pccb.rsprindx] =  uqbp->ua;
		WBFLUSH;
		Pccb.uq->uqca.ca_rspdsc[Pccb.rsprindx] |= (UQ_OWN | UQ_INT);
		WBFLUSH;

		/* Save physical address of buffer */
		Pccb.rspbtab[Pccb.rsprindx] = uqbp;
		Pccb.rsp_cnt++;		/* Increment count of used slots */
		Pccb.rsprindx++;	/* Increment index into the ring */
		Pccb.rsprindx %= NRSP;	/* modulo ring size		 */

	}
	else
	{	/* Ring full - place on free list */
		uqbp->flink = Pccb.uq_freel;
		Pccb.uq_freel = (uqbq *)uqbp;
	}
	WBFLUSH;

	Unlock_pccb (pccb);
	(void)splx(s);
}

/*
 *
 *
 *	Name:		uq_poll_rspring
 *	
 *	Abstract:	Check the response ring for any new responses.
 *			If there are any convert them to SCS messages
 *			and send them on their way. This routine will
 *			loop until the outstanding response count goes
 *			to zero or until it encounters an unused entry.
 *
 *	Inputs:
 *
 *	pccb			- Pointer to PCCB for this port.
 *	
 *	
 *
 *	Outputs:
 *
 *	pccb
 *	     pd.uq.rsp_cnt	- If a response is retrieved this value is
 *				  decremented.
 *	     pd.uq.lpinfo.uq_credits[con] - If a response is retrieved the 
 *				  credits given in the response packet are
 *				  added to the total for this connection.
 *	     pd.uq.last_rsp	- If a response is retrieved this value is
 *				  incremented mod ring size. This value is
 *				  the next ring entry to examine for a 
 *				  response.
 *	
 *	
 *
 *	SMP
 *		The count of buffers loaded into the ring and the ownership
 *		bits for the buffers in the ring must be modified in an
 *		smp-safe manner.  The response buffer count is tested before
 *		locking the pccb; then, after the pccb is locked, we read
 *		it again to make sure another cpu hasn't come in to do our work
 *		for us - if it is still not zero then we look at the
 *		ownership bit for the next response entry; if we own the
 *		entry, we save it for later in uqbp.  The Pccb gets unlocked. 
 *		We give uqbp to SCS.  And then we start over again.		
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
 *	If a response is found a call to scs_msg_rec or scs_dg_rec is generated
 *	depending on the message type of the response. If there is an entry
 *	on the free buffer queue that entry is placed in the response ring
 *	in place of used entry. If no buffer is available uq_poll_cring is
 *	called to scan the command ring for used entries. That routine will
 *	place any free buffers on the response ring/free queue as described
 *	in uq_ins_rspring.
 */
uq_poll_rspring(pccb)
PCCB	*pccb;
{
UQH	*uqbp;
UQH	*rp;
UQPPDH	*uqpbp;
SCSH	*scsbp;
char	con;
u_char	mtype;
short	msglen;
int	save_ipl, s;
int	pb_not_null = 1;

	save_ipl = splbio();	/* Raise IPL to muck with rings */
	while (Pccb.rsp_cnt != 0) {	/* There are outstanding responses */
	Lock_pccb(pccb);
	if (Pccb.rsp_cnt != 0) {  /* Is it still != 0, if not then leave */

		if (Pccb.uq->uqca.ca_rspdsc[Pccb.uq_lastrsp] & UQ_OWN) {
			Unlock_pccb(pccb);
			(void)splx(save_ipl);
			return;		/* But we don't own any		*/
		}

			/* Get buffer address */
		uqbp = Pccb.rspbtab[Pccb.uq_lastrsp]; 
			/* Decrement count of used slots */
		--Pccb.rsp_cnt;
		Pccb.uq_lastrsp++;	/* Increment response pointer	*/
		Pccb.uq_lastrsp %= NRSP;
		if (Pccb.pb == NULL)
			pb_not_null = 0;

		/* We just removed a buffer from the response ring,
			so we try to insert another one in */
		/* Are there any buffers on the freeq? */
		if ((rp = (UQH *)Pccb.uq_freel) != (UQH *)NULL) { 
			Pccb.uq_freel = rp->flink; /* Update list pointer */
			Unlock_pccb (pccb);
			uq_ins_rspring(rp,pccb); /* Fill the slot 	*/
		}
		else	/* No - Poll the command ring to see if we can 	*/
			/* scare some up.				*/
			{
			Unlock_pccb (pccb);
			uq_poll_cring(pccb);
			}

		uqpbp = Pos_to_ppdh(uqbp); /* position to uqssp header	*/
		con = uqpbp->uqp_cid;	/* get connection id		*/
		mtype = uqpbp->uqp_msgtype; /* message type		*/
		msglen = uqpbp->uqp_msglen; /* message length		*/

		/*  Was it a Last fail packet? */
		if((Pccb.uq_con & (short)(1 << con)) == 0) {
			Pccb.lfptr[con] = uqbp;
			(void)splx(save_ipl);
			return;
		}

		Lpinfo.uq_credits[con] += (u_short)uqpbp->uqp_credits;

		/*  The uqssp format packet must be converted into
			an SCS format packet before we send it up;
			see the comments to uq_ins_cring()	*/
		scsbp = SCS_msg_header(uqbp);	/* Position to scs header */
		scsbp->credit = (short)uqpbp->uqp_credits; /* Update the
								credits */
		scsbp->rconnid = Pccb.contab[con];    /* Insert remote
							connection id */
		Store_connid( scsbp->sconnid ) = con; /* Insert sending
							   connection id */

		if (mtype == UQT_SEQ) {	/* Sequential Message Type	*/
			s = Splscs();	/*  Set IPL to SCS level */
			scsbp->mtype = SCS_APPL_MSG; /* Application Message */
			if (pb_not_null)
			  scs_msg_rec( pccb, scsbp, (sizeof(SCSH) + msglen) );
			(void)splx(s);	/* Reset IPL			*/
		}


		if (mtype == UQT_DG) {	/*Datagram Message Type		*/
			s = Splscs();	/*  Set IPL to SCS level */
			scsbp->mtype = SCS_APPL_DG; 
			scs_dg_rec( pccb, scsbp, (sizeof(SCSH) + msglen) );
			(void)splx(s);	/* Reset IPL			*/
		}
	}  /*  end if */
		else Unlock_pccb(pccb);
	}  /*  end while */
	WBFLUSH;
	(void)splx(save_ipl);
}

/*
 *
 *
 *	Name:		uq_ins_cring
 *	
 *	Abstract:	This routine will place a buffer on the command
 *			ring if there is an available slot. If no slot is
 *			available the buffer is placed on the wait queue
 *			for later retrieval. If the command is placed on
 *			the ring the IP register on the controller is read
 *			to initiate polling of the ring.
 *			
 *
 *	Inputs:
 *
 *	scsbp			- Pointer to SCS header of buffer to use
 *	     uqpbp.msglen	- Size of application message
 *	pccb			- Pointer to PCCB for this port
 *	     pd.uq.cmd_cnt	- Number of used entries on the command ring
 *	     pd.uq.cmdrindx	- Index into ring to place next buffer
 *	     pd.uq.waitq	- Wait queue for queued commands
 *	     lpinfo.uq.uq_credits[i] - Number of credits available on this
 *								connection
 *	Outputs:
 *
 *	pccb			- Pointer to PCCB for this port
 *	     pd.uq.cmd_cnt	- Incremented if command placed on ring
 *	     pd.uq.cmdbtab[i]	- Address of buffer placed in ring entry i.
 *				- Valid if command 
 *
 *
 *
 *	The SCS layer has given us an sca format command and uq
 *	must convert it into an uqssp format command.  
 *
 *	In order to do this uq overwrites the scs sconnid field
 *	(1 long word) with a uqssp command packet header (1 long word);
 *	this is done in two steps - in uq_send_msg() uq writes
 *	in the uq header Size field; here, uq_ins_cring() writes in the
 *	Credit field, the Message Type field, and the uq Connid.  The uq
 *	Connid (1 byte) is copied from the low byte of the
 *	scs-packet's rconnid.  The scs-packet's rconnid is the scs
 *	sconnid uq_conn() gave to scs at connection startup.
 *
 *	When the response packet comes back from the uq controller,
 *	uq_poll_rspring() inserts the scs rconnid which was saved
 *	in Pccb.contab by uq_conn() at connection start-up.
 *	The uqssp header is then overwritten with an scs sconnid;
 *	this scs sconnid is equal to the uq connid.  (Technically,
 *	this scs sconnid may be incorrect in its seq-num field; but
 *	this doesn't matter because it is the rconnid, restored
 *	from Pccb.contab, that matters when we send this message
 *	up to scs.)
 *						--Matthew Sacks
 *
 *	
 *	SMP
 *		The index into the command ring and the count of buffers
 *		in the ring are incremented.  This manipulation is SMP
 *		protected.
 *	
 *		If the there is no room in the ring for the command message
 *		and it is then added to the the waitq, the waitq must be
 * 		modified within the scope of the lock.	
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
u_long	uq_ins_cring(scsbp,pccb)
SCSH	*scsbp;
PCCB	*pccb;
{
UQH	*uqbp;
UQPPDH	*uqpbp;
int i, s;
short indx;

	uqbp = UQ_header(scsbp);

	s = splbio();	/* Raise IPL to muck with rings */
	Lock_pccb(pccb);

	if (Pccb.cmd_cnt < NCMD) {	/* we have room at the inn	*/
		if( Pccb.uq->uqca.ca_cmddsc[Pccb.cmdrindx] & UQ_OWN ) {
			Unlock_pccb (pccb);
			panic("uqdriver: Command ring in invalid state");
		}

		uqpbp = Pos_to_ppdh(uqbp);
		uqpbp->uqp_cid = Load_connid(scsbp->rconnid);/* Load conn id */

		uqpbp->uqp_msgtype = UQT_SEQ; /* Sequential message */
		uqpbp->uqp_credits = 0;		/* Zero credit field */

		--Lpinfo.uq_credits[uqpbp->uqp_cid]; /* dec local credit cnt*/
		Pccb.cmdbtab[Pccb.cmdrindx] = uqbp;

		/*
	 	*	NOTE:
	 	*	The following two actions MUST be done in separate
 	 	*	statements.  This is due to hardware which reads the
	 	*	ring entries in two 16-bit reads. A race condition
	 	*	exists if the controller reads the low-order 16-bits
	 	*	first.  It is possible to get the old low-order and
	 	*	the new high-order, resulting in an old command being
	 	*	re-executed.  Make sure that the compiler never
	 	*	optimizes these two statements.
	 	*/
		Pccb.uq->uqca.ca_cmddsc[Pccb.cmdrindx] = uqbp->ua;
		WBFLUSH;
		Pccb.uq->uqca.ca_cmddsc[Pccb.cmdrindx] |= (UQ_OWN | UQ_INT);
		WBFLUSH;

		Pccb.cmdrindx++;
		Pccb.cmdrindx %= NCMD;
		Pccb.cmd_cnt++;
		i = *Pccb.Uqip;	/* Start the controller polling	*/
	}
	else 
	{	/* Put it on the wait queue	*/
		Insert_entry(uqbp->flink,Pccb.waitq);

	}
	WBFLUSH;

	Unlock_pccb (pccb);
	(void)splx(s);
		
}

/*
 *
 *
 *	Name:		uq_poll_cring
 *	
 *	Abstract:	This routine scans the command ring for free
 *			entries (i.e. entries which the controller has
 *			already pulled across). Any free entries are
 *			used as input to uq_ins_rspring which places
 *			them on either to response ring or free queue.
 *			If there are any commands on the wait queue they
 *			are placed in the now free command ring slots.
 *			
 *	Inputs:
 *
 *	pccb			- Pointer to PCCB for this port
 *	     
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	SMP	
 *			The count of commnand messages currently in the ring
 *			and the ownership bits for the ring entries must be
 *			handled in an smp-safe manner.  See the comments to
 *			uq_poll_rspring because it is essentially the same
 *			situation.
 *			If a command message is removed from the waitq, the
 *			waitq must be modified in an smp safe manner.
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
uq_poll_cring(pccb)
PCCB	*pccb;
{
UQH	*uqbp;
UQH	*qp;
int	s;


	s = splbio();
	while (Pccb.cmd_cnt != 0) {
		Lock_pccb (pccb);
		if (Pccb.cmd_cnt != 0) {	/* Is it still != 0?  */
		if (Pccb.uq->uqca.ca_cmddsc[Pccb.uq_lastcmd] & UQ_OWN) {
			Unlock_pccb (pccb);
			(void)splx(s);
			return;	 /* We don't own the descriptor	*/
		}
		
		/* get phys address of buffer */
		uqbp = Pccb.cmdbtab[Pccb.uq_lastcmd];
		/* Decrement count of in-use slots */
		--Pccb.cmd_cnt;
		Pccb.uq_lastcmd++;
		Pccb.uq_lastcmd %= NCMD;
	
		if(Pccb.waitq.flink != &Pccb.waitq){
			qp = (UQH *)Pccb.waitq.flink;
			Pccb.waitq.flink = qp->flink;
			Pccb.waitq.flink->blink = qp->blink;
			Unlock_pccb (pccb);
			WBFLUSH;
			uq_ins_cring( SCS_msg_header(qp), pccb );
		}
		else	Unlock_pccb (pccb);

		/* Insert unused pkt on rspring or free	*/
		uq_ins_rspring( uqbp, pccb );
		       					/* queue				        */
	} /* end if */
	else Unlock_pccb(pccb);
	} /* end while */
	WBFLUSH;
	(void)splx(s);
}

/*
 *
 *
 *	Name:		uq_intr		-Interrupt processing routine
 *	
 *	Abstract:	This routine fields interrupts for UQ devices
 *			and performs the appropriate actions.
 *			
 *	Inputs:
 *		
 *	IPL_device	- Device IPL
 *	ctlr		- Index into port-specific structures
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
 *		Dependent on state of adapter.
 *			S_RUN: 	If a buffer purge was requested the
 *				purge is done.
 *				If a response interrupt was received
 *				the response(s) is(are) processed and
 *				the appropriate messages sent to SCS.
 *				In all cases the command ring is scanned
 *				for free entries and those entries are
 *				added to the response ring/free queue
 *				via uq_ins_rspring. If there are any
 *				commands queued for transmission they are
 *				placed in the now free slots (if any).
 *
 *			S_STEPx: The SA register is read and verified.
 *				 The next step data is written to the SA
 *				 and the adapter state is advanced to the
 *				 next step. The final step informs SCS
 *				 about the new system and enables the 
 *				 adapter.
 */
int	uqintr(ctlr)
int	ctlr;				/* Controller index number */
{
	register PCCB *pccb;
	struct uba_ctlr	*um = uqminfo[ctlr];
	struct _uq *uq;
	int count,i;
	struct port_info *pcinfo = port_info_ptr[ctlr];
	int	s;

	pccb = pcinfo->pc_ptr;		/* Get pointer to PCCB	*/

/*	We expect an interrupt in the probe routine. This interrupt is for
 *	autoconf's benefit. Actual initialization is done via the poll
 *	method at probe time.
 */
	if (Lpinfo.uq_flags & UQ_PRB)	
		return;			

	uq = Pccb.uq;			/* Get address of uq structure	*/

/*	If there is a fatal error in the SA register it is logged. The 
 *	port is then disabled and SCS is notified of the path crash.
 *	Port reinitialization may take place if reinit counters are not
 *	exceeded.
 */
	if ((Lpinfo.sa = *Pccb.Uqsa) & UQ_ERR) { 
		uq_error_log( pccb, UQ_SA_FATAL); 
		uq_disable(pccb, PF_PORTERROR); 
		return;
	}
	

/*	Interrupt processing switch
 */

	switch (Lpinfo.uq_state) {

	case S_RUN:
		if (uq->uqca.ca_bdp) {	/* Need to do a buffer purge	*/
			s = spl6();
			i = um->um_ubinfo; /* Save ubinfo		*/
			um->um_ubinfo = (uq->uqca.ca_bdp>>8)<<28; /* Get datapath #	*/
			ubapurge(um);	/* Purge the sucker		*/
			um->um_ubinfo = i; /* Restore ubinfo		*/
			uq->uqca.ca_bdp = 0; /* Clear purge request	*/
			SA_W(pccb) = 0; /* Signal purge complete	*/
			(void)splx(s);
		}
	
		if (uq->uqca.ca_rspint) { /* We got a response		*/
			Pccb.uq->uqca.ca_rspint = 0; /* Clear interrupt indicator	*/
			WBFLUSH;
			uq_poll_rspring(pccb); /* Go poll the ring	*/
		}
		if (uq->uqca.ca_cmdint) { /* Command ring interrupt	*/
			uq->uqca.ca_cmdint = 0; /* Clear indicator	*/
			WBFLUSH;
		}
		uq_poll_cring(pccb);	/* Check if any cmd slots free	*/
					/* and fill them if there are	*/
		WBFLUSH;
/*
 *	Check the response ring once more because of the race condition
 *	that might result in a response being placed in the ring after
 *	we have polled the ring. An interrupt might NOT be issued
 *	in this case and we could delay processing the response for a long
 *	period of time.
 */
		s = Splscs ();
		Lock_pccb (pccb);
		if ((Pccb.uq->uqca.ca_rspdsc[Pccb.uq_lastrsp] & UQ_OWN) == 0){
			Unlock_pccb (pccb);
			splx (s);
			uq_poll_rspring( pccb ); /* We got another response */
		}
		else	{
			Unlock_pccb (pccb);
			splx (s);
			}
			
		WBFLUSH;
		break;

	case S_STEP1:
		cpu_printf(Pccb.init_leader, Pccb.reset_leader, "uqintr STEP1");
		Wait_step(STEP1MASK, STEP1GOOD)
		Pccb.step2r = *Pccb.Uqsa;
		Lpinfo.uq_state = S_STEP2;
		SA_W(pccb) = Pccb.step2w;
		return;

	case S_STEP2:
		cpu_printf(Pccb.init_leader, Pccb.reset_leader, "uqintr STEP2");
		Wait_step(STEP2MASK, STEP2GOOD)
		Pccb.step3r = *Pccb.Uqsa;
		Lpinfo.uq_state = S_STEP3;
		SA_W(pccb) = Pccb.step3w;
		return;

	case S_STEP3:
		cpu_printf(Pccb.init_leader, Pccb.reset_leader, "uqintr STEP3");
		Wait_step(STEP3MASK, STEP3GOOD)
		Pccb.step4r = *Pccb.Uqsa;
		Lpinfo.uq_state = S_STEP4;
		if (um->um_hd->uba_type & UBAXMI) {
		    Pccb.uq->uqca.ca_scp_size = UQ_SCP_SIZE; /* Size of scratchpad */
	            Pccb.uq->uqca.ca_scp_add = (u_long)Pccb.uqscp; /* Physical address of scratchpad */
	        }
		SA_W(pccb) = Pccb.step4w;
		Pccb.reinit_cnt = UQ_MAX_REINIT; /* Reset reinit counter */
		Lpinfo.uq_state = S_RUN;
		/*
		 * Initialize the data structures.
		 */
		uq_init_buffers( pccb );
		uq_create_sys( pccb );
		Pccb.init_leader = UQPath_Is_Up; /* done initing the port */
		Pccb.reset_leader = UQPath_Is_Up;


		if (Pccb.poll_rate == 0)	/* Set the rate for sa poll */
			Pccb.poll_rate = 30 * hz;
		if ( (Lpinfo.uq_flags & UQ_TIM) == 0 ) { /* If timer not yet on */
			Lpinfo.uq_flags |= UQ_TIM; /* Indicate timer now on */
			timeout(uq_timer,pccb,Pccb.poll_rate); /* Start timer*/
		}
		return;
	}
}

/*
 *
 *
 *	Name:		uq_init
 *	
 *	Abstract:	This routine starts the initialization process.
 *			It is called as a result of a local port crash.
 *			It is only called via the fork mechanism.
 *			
 *	Inputs:
 *
 *	pccb		- Pointer to PCCB for this port
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
 *	SMP:	see the comments at uq_disable().
 *	
 * 
 *	Side		
 *	Effects:
 *
 *		The initialization process is started. If an error is 
 *		detected the local port is crashed.
 */
void uq_init(pccb)
 PCCB	*pccb;
{
int	count, s, s2;

	s = Splscs();
	Lock_pccb (pccb);
	Pccb.rip = 0;	/* Clear recovery in progress flag	*/

	if (Pccb.init_leader ==  UQPath_Is_Up)  {
	Pccb.init_leader = CURRENT_CPUDATA->cpu_num;
	Unlock_pccb (pccb);
	if (Pccb.init_leader == CURRENT_CPUDATA->cpu_num) {
	/* inside here only if we are the cpu who will start the init */

	/*
	 * Start the hardware initialization sequence.
	 */
	s2 = splbio();
	uq_port_reset(pccb);
	(void)splx(s2);

	count = 0;
	while (count < DELAYTEN){	/* Wait for at most 10 seconds 	 */
		if ((*Pccb.Uqsa & UQ_STEP1) != 0)
			break;		/* We woke it up		*/
		if ((Lpinfo.sa = *Pccb.Uqsa) & UQ_ERR) {
			/* Got a fatal port err */
			Cprintf("Called from uq_init, sa = %x\n", Lpinfo.sa);
			uq_error_log( pccb, UQ_SA_FATAL); /* Log the error */
			uq_disable(pccb, PF_PORTERROR );
			(void)splx(s);
			return;	/* CHECK */
		}
		(void)splx(s);		/* Don't do the delay at high ipl */
		DELAY(10000);
		s = Splscs();
		count = count +1;
	}
	if (count >= DELAYTEN) { 	/* Reinit failed		*/
		Cprintf("Called from uq_init, sa = %x\n", Lpinfo.sa);
		uq_error_log( pccb, UQ_RESET_FAIL); /* Log the error */
		uq_disable(pccb, PF_PORTERROR );
		(void)splx(s);
		return;	/* CHECK */
	}

	if (Lpinfo.uq_type == KDM_TYPE) {
		*Pccb.Uqpd = Pccb.xmipd; /* Write interrupt info in PD reg */
	}
	Pccb.step1r = *Pccb.Uqsa;
	if (Lpinfo.uq_type == BDA_TYPE) 
		Pccb.step1r &= ~UQ_QB;	/* Necessary to turn off bit    */
	Lpinfo.uq_state = S_STEP1;
	SA_W(pccb) = Pccb.step1w;
	}  /* end if */

	/*
	 * Initialization continues in interrupt routine.
	 */

	}  /* end if */
		else Unlock_pccb (pccb);

	(void)splx(s);
	return;

}

/*
 *
 *
 *	Name:		uq_alloc_dg	- Allocate Datagram Buffer
 *	
 *	Abstract:	This function allocates a UQSSP port specific
 *			message buffer.
 *
 *	Inputs:
 *
 *	IPL_SCS				- Interrupt processor level
 *	pccb				- Port Command and Control Block ptr
 *
 *	Outputs:
 *
 *	IPL_SCS				- Interrupt processor level
 *	scsbp				- Add of SCS header in message buffer
 *
 *	Return 
 *	Values:
 *
 *	Address of SCS header in message buffer on success
 *	Otherwise ( SCSH * )NULL
 * 
 *	Side		- Buffer is removed from the internal free queue
 *	Effects:
 *
 */
 
SCSH *uq_alloc_dg( pccb )
PCCB	*pccb;
{
UQH	*uqbp;

	Lock_pccb (pccb);
	if ((uqbp = (UQH *)Pccb.uq_freel) != (UQH *)NULL){
		Pccb.uq_freel = uqbp->flink;
		Unlock_pccb (pccb);
		return (SCS_msg_header(uqbp));
	}
	else	{
		Unlock_pccb (pccb);
		uq_poll_cring(pccb); /* See if there are any on the cmd ring */

		Lock_pccb (pccb);
		if ((uqbp = (UQH *)Pccb.uq_freel) != (UQH *)NULL){
			Pccb.uq_freel = uqbp->flink;
			Unlock_pccb (pccb);
			return (SCS_msg_header(uqbp));
		}
		else
			{
			Unlock_pccb (pccb);
			return(NULL);
			}
		}
}

/*
 *
 *
 *	Name:		uq_dealloc_dg	- Deallocate Datagram Buffer
 *	
 *	Abstract:	This function deallocates a port specific message buffer.
 *			
 *	Inputs:
 *
 *	IPL_SCS			- Interrupt processor level
 *	
 *	pccb			- Port Command and Control Block
 *	scsbp			- Address of SCS header in message buffer
 *
 *
 *	Outputs:
 *
 *	IPL_SCS			- Interrupt processor level
 *	
 *
 *	Return 
 *	Values:
 *
 *	RET_SUCCESS		- Successful deallocation
 *	
 * 
 *	Side			- Buffer is added either to the response
 *	Effects:		  ring (if not full) or to the internal
 *				  free queue
 *
 */
void	uq_dealloc_dg( pccb, scsbp)
PCCB	*pccb;
SCSH	*scsbp;
{
register UQH	*uqbp;

	uqbp = UQ_header( scsbp);
	uq_ins_rspring(uqbp,pccb);	/* Insert in response ring if not  */
					/* full. Else insert on free queue */
	return;
}

/*
 *
 *
 *	Name:		uq_add_dg	-  Add a Datagram to Free Pool
 *	
 *	Abstract:	This function adds a datagram buffer to the Free Pool
 *			
 *	Inputs:
 *
 *	IPL_SCS			- Interrupt processor level
 *	
 *	pccb			- Port Command and Control Block
 *	scsbp			- Address of SCS header in message buffer
 *
 *
 *	Outputs:
 *
 *	IPL_SCS			- Interrupt processor level
 *	
 *
 *	Return 
 *	Values:
 *
 * 
 *	Side			- Buffer is added to response ring (if not
 *	Effects:		  full) or to the internal free queue
 *
 */
void	uq_add_dg(pccb,scsbp)
PCCB	*pccb;
SCSH	*scsbp;
{
UQH	*uqbp;

	uqbp = UQ_header( scsbp );
	uq_ins_rspring(uqbp,pccb);
}

/*
 *
 *
 *	Name:		uq_remove_dg	- Remove a Datagram Buff from Free Pool
 *	
 *	
 *	Abstract:	This function removes a UQSSP port specific
 *			datagram buffer from the internal free pool
 *
 *	Inputs:
 *
 *	IPL_SCS				- Interrupt processor level
 *	pccb				- Port Command and Control Block ptr
 *
 *	Outputs:
 *
 *	IPL_SCS				- Interrupt processor level
 *	scsbp				- Add of SCS header in message buffer
 *
 *	Return 
 *	Values:
 *
 *	Address of SCS header in message buffer on success
 *	Otherwise ( SCSH * )NULL
 *
 * 
 *	Side		- Buffer is removed from the internal free queue
 *	Effects:	  on success
 *
 */
SCSH	*uq_remove_dg(pccb)
PCCB	*pccb;
{
UQH	*uqbp;

	Lock_pccb (pccb);
	if ((uqbp = (UQH *)Pccb.uq_freel) != (UQH *)NULL){
		Pccb.uq_freel = uqbp->flink;
		Unlock_pccb (pccb);
		return(SCS_msg_header(uqbp));
	}
	else
		{
		Unlock_pccb (pccb);
		uq_poll_cring(pccb); /* See if there are any on the cmd ring */

		Lock_pccb (pccb);
		if ((uqbp = (UQH *)Pccb.uq_freel) != (UQH *)NULL){
			Pccb.uq_freel = uqbp->flink;
			Unlock_pccb (pccb);
			return (SCS_msg_header(uqbp));
			}
		else
			{
			Unlock_pccb (pccb);
			return(NULL);
			}
		}
}

/*
 *
 *
 *	Name:		uq_alloc_msg	- Allocate Message Buffer
 *	
 *	Abstract:	This function allocates a UQSSP port specific
 *			message buffer.
 *
 *	Inputs:
 *
 *	IPL_SCS				- Interrupt processor level
 *	pccb				- Port Command and Control Block ptr
 *
 *	Outputs:
 *
 *	IPL_SCS				- Interrupt processor level
 *	scsbp				- Add of SCS header in message buffer
 *
 *	Return 
 *	Values:
 *
 *	Address of SCS header in message buffer on success
 *	Otherwise ( SCSH * )NULL
 * 
 *	Side		- Buffer is removed from the internal free queue
 *	Effects:
 *
 */
 
SCSH *uq_alloc_msg( pccb )
PCCB	*pccb;
{
UQH	*uqbp;

	uq_alloc_cnt++;

	Lock_pccb (pccb);
	if ((uqbp = (UQH *)Pccb.uq_freel) != (UQH *)NULL){
		Pccb.uq_freel = uqbp->flink;
		Unlock_pccb (pccb);
		return (SCS_msg_header(uqbp));
	}
	else
		{
		Unlock_pccb (pccb);
		uq_poll_cring(pccb); /* See if there are any on the cmd ring */

		Lock_pccb (pccb);
		if ((uqbp = (UQH *)Pccb.uq_freel) != (UQH *)NULL){
			Pccb.uq_freel = uqbp->flink;
			Unlock_pccb (pccb);
			return (SCS_msg_header(uqbp));
		}
		else
			{
			Unlock_pccb (pccb);
			return(NULL);
			}
		}
}

/*
 *
 *
 *	Name:		uq_dealloc_msg	- Deallocate Message Buffer
 *	
 *	Abstract:	This function deallocates a port specific message buffer.
 *			
 *	Inputs:
 *
 *	IPL_SCS			- Interrupt processor level
 *	
 *	pccb			- Port Command and Control Block
 *	scsbp			- Address of SCS header in message buffer
 *
 *
 *	Outputs:
 *
 *	IPL_SCS			- Interrupt processor level
 *	
 *
 *	Return 
 *	Values:
 *
 *	RET_SUCCESS		- Successful deallocation
 *	
 * 
 *	Side			- Buffer is added either to the response
 *	Effects:		  ring (if not full) or to the internal
 *				  free queue
 *
 */
void	uq_dealloc_msg( pccb, scsbp)
PCCB	*pccb;
SCSH	*scsbp;
{
register UQH	*uqbp;

	uqbp = UQ_header( scsbp);
	uq_ins_rspring(uqbp,pccb);	/* Insert in response ring if not  */
					/* full. Else insert on free queue */
	return;
}

/*
 *
 *
 *	Name:		uq_add_msg	-  Add a Message to Free Pool
 *	
 *	Abstract:	This function adds a message buffer to the Free Pool
 *			
 *	Inputs:
 *
 *	IPL_SCS			- Interrupt processor level
 *	
 *	pccb			- Port Command and Control Block
 *	scsbp			- Address of SCS header in message buffer
 *
 *
 *	Outputs:
 *
 *	IPL_SCS			- Interrupt processor level
 *	
 *
 *	Return 
 *	Values:
 *	
 * 
 *	Side			- Buffer is added to response ring (if not
 *	Effects:		  full) or to the internal free queue
 *
 */
void	uq_add_msg(pccb,scsbp)
PCCB	*pccb;
SCSH	*scsbp;
{
UQH	*uqbp;

	uqbp = UQ_header( scsbp );
	uq_ins_rspring(uqbp,pccb);
}

/*
 *
 *
 *	Name:		uq_remove_msg	- Remove a Message Buff from Free Pool
 *	
 *	
 *	Abstract:	This function removes a UQSSP port specific
 *			message buffer from the internal free pool
 *
 *	Inputs:
 *
 *	IPL_SCS				- Interrupt processor level
 *	pccb				- Port Command and Control Block ptr
 *
 *	Outputs:
 *
 *	IPL_SCS				- Interrupt processor level
 *	scsbp				- Add of SCS header in message buffer
 *
 *	Return 
 *	Values:
 *
 *	Address of SCS header in message buffer on success
 *	Otherwise ( SCSH * )NULL
 *
 * 
 *	Side		- Buffer is removed from the internal free queue
 *	Effects:	  on success
 *
 */
SCSH	*uq_remove_msg(pccb)
PCCB	*pccb;
{
UQH	*uqbp;

	Lock_pccb (pccb);
	if ((uqbp = (UQH *)Pccb.uq_freel) != (UQH *)NULL){
		Pccb.uq_freel = uqbp->flink;
		Unlock_pccb (pccb);
		return(SCS_msg_header(uqbp));
	}
	else	{
		Unlock_pccb (pccb);
		uq_poll_cring(pccb); /* See if there are any on the cmd ring */

		Lock_pccb (pccb);
		if ((uqbp = (UQH *)Pccb.uq_freel) != (UQH *)NULL){
			Pccb.uq_freel = uqbp->flink;
			Unlock_pccb (pccb);
			return (SCS_msg_header(uqbp));
		}
		else
			{
			Unlock_pccb (pccb);
			return(NULL);
			}
		}
}

/*
 *
 *
 *	Name:		uq_send_msg
 *	
 *	Abstract:	This routine is called by SCS to send a message
 *			across the port. Since UQSSP devices do not
 *			handle SCS level messages, those messages are
 *			processed by this routine. Application level
 *			messages are sent to the port via uq_ins_cring.
 *			
 *	Inputs:
 *
 *	pccb				- Port Command and Control Block ptr
 *	pb				- Path Block ptr
 *	scsbp				- Ptr to SCS portion of message buffer
 *	size				- Size of message buffer
 *	disposal			- Flag for disposal of buffer
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
void	uq_send_msg( pccb, pb, scsbp, size, disposal)
PCCB		*pccb;
PB		*pb;
SCSH		*scsbp;
u_long		size;
u_long		disposal;
{
ACCEPT_RSP	*accrsp;
struct _connid	tcon;
CREDIT_RSP	*credrsp;
UQH		*uqbp;			/* Pointer to UQ header in buffer */
UQPPDH		*uqpbp;
SCSH		*lfbp;
u_long		con;
short		msglen;


/* If the SCS message type is not an application message then we must	*/
/* handle it in the driver because UQSSP devices do not support SCA.	*/

	if (scsbp->mtype != SCS_APPL_MSG) {	/* Not an application msg */


/* Since UQSSP devices do not support SCA this driver must fake the 	*/
/* connection handshake. 						*/

		switch (scsbp->mtype){

		/* For the following requests, we queue to a wait queue */
		/* This queue is processed via the timer routine.  This */
		/* is necessary because it is possible to have multiple */
		/* SCS requests simultaneously				*/
		/* Connection Request - We fork to the connect routine	*/
		/* and then return to the caller (SCS)			*/

		/* Disconnect Request - To process a disconnect we must	*/
		/* fork so we can generate a response and start our own	*/
		/* disconnect sequence.					*/

		/* Credit Request - This operation is essentially a no-op. */
		/* We fork and then generate a response.		   */

		case SCS_CREDIT_REQ:
		case SCS_DISCONN_REQ:
		case SCS_CONN_REQ:


			uqbp = UQ_header(scsbp);
			Lock_pccb (pccb);
			Insert_entry(uqbp->flink,Pccb.scswaitq);
			Unlock_pccb (pccb);
			break;
	
		/* Accept Response - If the response is not  success	*/
		/* then the record of the connection is removed.	*/
		/* In either case we then return to the caller (SCS)	*/

		case SCS_ACCEPT_RSP:
			accrsp = (ACCEPT_RSP *)scsbp;
			if (accrsp->reason != ADR_SUCCESS) {
				Pccb.uq_con &= 
				      (short)~(1<<Load_connid(accrsp->rconnid));
				Pccb.ncon--;
			}
			else {

		/* If last fail packet waiting to be sent then send it */
		/* This probably doesn't need to smp-locked; but MSCP  */
		/*	controllers are permitted to send duplicate    */
		/*	datagrams, and lets play it safe anyway.       */
				con = Load_connid(accrsp->rconnid);
				if (Pccb.lfptr[con]) {
				Lock_pccb (pccb);
				if (Pccb.lfptr[con]) { /* Is it still != 0? */
				   uqbp = Pccb.lfptr[con];
				   Pccb.lfptr[con] = 0;
				   Unlock_pccb (pccb);
				   uqpbp = Pos_to_ppdh(uqbp);
				   msglen = uqpbp->uqp_msglen;
				   lfbp = SCS_msg_header(uqbp);
		  		   lfbp->credit = 0;
				   lfbp->rconnid = Pccb.contab[con];
				   Store_connid( lfbp->sconnid ) = con;
				   lfbp->mtype = SCS_APPL_DG; 
				   scs_dg_rec(pccb,lfbp,(sizeof(SCSH)+msglen));
				}
				   else Unlock_pccb (pccb);
				}
			}

		/* FALL THROUGH */

		/* Reject Response - This is the last step of the	*/
		/* reject. Decrement connection count and return.	*/
		/* Disconnect Response - This is the last step of the	*/
		/* disconnect. Simply return.				*/

		case SCS_REJECT_RSP:
		case SCS_DISCONN_RSP:

			KM_FREE(scsbp, KM_SCA);
			break;

		/* Any other message type is a fatal error		*/

		default:
		    panic ("uqdriver: Invalid SCS Sequenced Message Type\n");
		}
	}
	else
		{
		uqbp = UQ_header( scsbp );   /* Position to UQ header	*/	
		uqpbp = Pos_to_ppdh(uqbp);   /* Position to uqssp header */
		/* Need to save size for later use */
		uqpbp->uqp_msglen = size - sizeof(SCSH);
		WBFLUSH;
		uq_ins_cring( scsbp, pccb ); /* Place on command ring	*/
		}
}

/*
 *
 *
 *	Name:		uq_dispatch
 *	
 *	Abstract:	This routine dispatches waiting SCS requests to
 *			the appropriate handlers.
 *			
 *	Inputs:
 *
 *	pccb		- Pointer to pccb
 *	
 *	IPL		was raised to ipl bio in uq_timer
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
void 
uq_dispatch(pccb)
PCCB	*pccb;
{
SCSH	*scsbp;
UQH	*qp;

		while (Pccb.scswaitq.flink != &Pccb.scswaitq) {
			Lock_pccb (pccb);
			if (Pccb.scswaitq.flink != &Pccb.scswaitq) {
				qp = (UQH *)Pccb.scswaitq.flink;
				Pccb.scswaitq.flink = qp->flink;
				Pccb.scswaitq.flink->blink = qp->blink;
				scsbp = SCS_msg_header(qp);
				Unlock_pccb (pccb);
	
				switch(scsbp->mtype) {

				case SCS_CONN_REQ:
					uq_conn(pccb, scsbp);
					break;
				case SCS_CREDIT_REQ:
					uq_cred(pccb, scsbp);
					break;
				case SCS_DISCONN_REQ:
					uq_dcon(pccb, scsbp);
					break;
				}
			} /* end if */
			else 
				{
				Unlock_pccb (pccb);
				}
		} /* end while */
}


/*
 *
 *
 *	Name:		uq_cred
 *	
 *	Abstract:	This routine processes the SCA credit request.
 *			It is called via the fork mechanism. This request
 *			is a NOP for the uq port. The message type is set
 *			to CREDIT_RSP and the message is returned.
 *			
 *	Inputs:
 *
 *	pccb		- Pointer to pccb
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
void uq_cred(pccb,scsbp)
PCCB	*pccb;
SCSH	*scsbp;
{
CREDIT_RSP	*credrsp;
struct _connid	tcon;
int		s;


	credrsp = (CREDIT_RSP *)scsbp; /* Get credit request block */
	tcon = credrsp->rconnid;
	credrsp->mtype = SCS_CREDIT_RSP;
	credrsp->rconnid = credrsp->sconnid;
	credrsp->sconnid = tcon;
	s = Splscs();	/*  Set IPL to SCS level	*/
	scs_msg_rec( pccb, (SCSH *)credrsp, sizeof(CREDIT_RSP) );
	(void)splx(s);
}

/*
 *
 *
 *	Name:		uq_conn
 *	
 *	Abstract:	This routine processes the SCA connect request.
 *			It is called via the fork mechanism.
 *			
 *	Inputs:
 *
 *	pccb		- Pointer to pccb
 *	scsbp		- Pointer to request buffer
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
void uq_conn(pccb,scsbp)
PCCB	*pccb;
SCSH	*scsbp;
{
PB		*pb;
CONN_REQ	*conreq;
CONN_RSP	*conrsp;
ACCEPT_REQ	*rspbp;
UQH		*uqbp;
u_short		result, ctype;
u_long		rconn;
struct _connid	rem_connid;
int		s;


	conreq = (CONN_REQ *)scsbp; /* Get connection request block */

/* The following code checks which remote server is the target of this	
 * Connect Request. 
 * First a check is made to verify that this port 	
 * supports the remote server requested. If it does not a response 	
 * reason of NOSUPPORT is returned. If the server is supported a check	
 * is made to see if there is already a connection to the requested	
 * server. Only one connection per server is allowed. If a connection	
 * already exists a response reason of SUCCESS is returned and a REJECT
 * Request is generated with a reason of BUSY. 
 * The number of current connections is then checked against the maximum
 * concurrent connections for this port type. If that limit has been reached
 * the connect is rejected with a reason code of NORESOURCE.
 * If the server is supported and there is no current connection the 		
 * connection is marked active and a response reason of SUCCESS is	
 * returned. An Accept Request message is then generated to the SCS	
 * layer as the next step in the connection protocol.			
 */


    if(!(bcmp(conreq->rproc_name,DISK_NAME,NAME_SIZE))) {
	if (uq_cinfo[Lpinfo.uq_type].servers & UQCB_DISK) {
		rconn = UQC_DISK;
		if(!(Pccb.uq_con & UQCB_DISK)){
			ctype = UQCB_DISK;
			result = ADR_SUCCESS;
			Pccb.contab[rconn] = conreq->sconnid;
		}
		else {
			result = ADR_BUSY;
			rem_connid = conreq->sconnid;
		}
	}
	else
		result = ADR_NOSUPPORT;

    }
    else if(!(bcmp(conreq->rproc_name,TAPE_NAME,NAME_SIZE))) {
	if (uq_cinfo[Lpinfo.uq_type].servers & UQCB_TAPE) {
		rconn = UQC_TAPE;
		if(!(Pccb.uq_con & UQCB_TAPE)){
			ctype = UQCB_TAPE;
			result = ADR_SUCCESS;
			Pccb.contab[rconn] = conreq->sconnid;
		}
		else {
			rem_connid = conreq->sconnid;
			result = ADR_BUSY;
		}
	}
	else
		result = ADR_NOSUPPORT;
    }
    else if(!(bcmp(conreq->rproc_name,DUP_NAME,NAME_SIZE))) {
	if (uq_cinfo[Lpinfo.uq_type].servers & UQCB_DUP) {
		rconn = UQC_DUP;
		if(!(Pccb.uq_con & UQCB_DUP)){
			ctype = UQCB_DUP;
			result = ADR_SUCCESS;
			Pccb.contab[rconn] = conreq->sconnid;
		}
		else {
			result = ADR_BUSY;
			rem_connid = conreq->sconnid;
		}
	}
	else
		result = ADR_NOSUPPORT;
    }
    else	
	result = ADR_NOSUPPORT;	/* Never even heard of the server	*/




    if ((result == ADR_SUCCESS) || (result == ADR_BUSY)) {
	KM_ALLOC(rspbp, ACCEPT_REQ *, sizeof(ACCEPT_REQ), KM_SCA, KM_NOW_CL_CA)
	if (rspbp == (ACCEPT_REQ *)NULL) {
		uqbp = UQ_header( scsbp );   /* Position to UQ header	*/	
		s = Splscs ();
		Lock_pccb (pccb);
		Insert_entry(uqbp->flink,Pccb.scswaitq);
		Unlock_pccb (pccb);
		splx (s);
		return;
	}
    }

    if( result == ADR_SUCCESS ) {
	if( Pccb.ncon >= uq_cinfo[Lpinfo.uq_type].max_con) {
		result = ADR_NOSUPPORT;
	}
	else
		Pccb.uq_con |= ctype;
    }

    conrsp = (CONN_RSP *)conreq;
    conrsp->rconnid = conreq->sconnid; /* Set up the connid regardless */
    if ((result == ADR_SUCCESS) || (result == ADR_BUSY)) {
	Store_connid( conrsp->sconnid ) = rconn;
    }

    /* If we can form a connection save some info for the rest of
     * the handshake.
     */
    if (result == ADR_SUCCESS) {
		Pccb.ncon++;

	bcopy(conreq->sproc_name,
	      ( ACCEPT_REQ * )rspbp->rproc_name,
	      NAME_SIZE);
	bcopy(conreq->rproc_name,
	      ( ACCEPT_REQ * )rspbp->sproc_name,
	      NAME_SIZE);
    }

    conrsp->mtype = SCS_CONN_RSP; /* Set in message type		*/
    if( result == ADR_BUSY )
	conrsp->reason = ADR_SUCCESS; /* Success for now		*/
    else
	conrsp->reason = result; /* Set in reason			*/
    s = Splscs();	/*  Set IPL to SCS level	*/
    scs_msg_rec( pccb, (SCSH *)conrsp, sizeof(CONN_RSP) );	/* Call SCS with response */
    (void)splx(s);	/* Reset IPL */


    /* If the connection can be formed an Accept Request message is
     * generated. The buffer used for this request is attached to
     * the pccb. It will be returned with the Accept Response
     * message. 
     */
    if (result == ADR_SUCCESS){ 	
	Store_connid( rspbp->sconnid ) = rconn;
	rspbp->rconnid = Pccb.contab[rconn];
	rspbp->mtype = SCS_ACCEPT_REQ;
	rspbp->credit = Lpinfo.uq_credits[rconn];
	s = Splscs();	/*  Set IPL to SCS level	*/
	scs_msg_rec( pccb, rspbp, sizeof(ACCEPT_REQ));
	(void)splx(s);	/* Reset IPL */
    }

    /* If the connection is busy a Reject Request message is
     * generated. The buffer used for this request is attached to
     * the pccb. It will be returned with the Reject Response
     * message. 
     */
    else if (result == ADR_BUSY){ 	
	Store_connid( rspbp->sconnid ) = rconn;
	rspbp->rconnid = rem_connid;
	rspbp->mtype = SCS_REJECT_REQ;
	rspbp->credit = 0;
	(( REJECT_REQ * )rspbp)->reason = result;
	s = Splscs();	/*  Set IPL to SCS level	*/
	scs_msg_rec( pccb, (REJECT_REQ *)rspbp, sizeof(REJECT_REQ));
	(void)splx(s);	/* Reset IPL */
    }	

    return;
}

/*
 *
 *
 *	Name:		uq_dcon
 *	
 *	Abstract:	This routine processes a SCA disconnect request.
 *			
 *	Inputs:
 *
 *	
 *	IPL:		raised to bio by uq_timer
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
void uq_dcon(pccb,scsbp)
PCCB	*pccb;
SCSH	*scsbp;
{
DISCONN_REQ	*dreq, *reqbp;
DISCONN_RSP	*drsp;
UQH		*uqbp;
struct _connid	rem_connid;
u_long		rconn;
int		s;

	dreq = (DISCONN_REQ *)scsbp;
	drsp = (DISCONN_RSP *)scsbp;

	KM_ALLOC(reqbp, DISCONN_REQ *, sizeof(DISCONN_REQ), KM_SCA, KM_NOW_CL_CA)
	if (reqbp == (DISCONN_REQ *)NULL) {
		uqbp = UQ_header( scsbp );   /* Position to UQ header	*/	
		Lock_pccb (pccb);
		Insert_entry(uqbp->flink,Pccb.scswaitq);
		Unlock_pccb (pccb);
		return;
	}
/* After a Disconnect Request is received a Disconnect Response is
 * generated. Then a Disconnect Request from the Remote Sysap (in this
 * case the port performs that function) is generated.
 */

/* Generate the Disconnect Response
 * Swap connection identifiers	
 */
	rem_connid = dreq->rconnid;
	rconn = Load_connid(rem_connid);
	drsp->rconnid = dreq->sconnid;
	reqbp->rconnid = dreq->sconnid;
	drsp->sconnid = rem_connid;

	drsp->mtype = SCS_DISCONN_RSP; /* Set in message type		*/
	s = Splscs();	/*  Set IPL to SCS level	*/
	scs_msg_rec( pccb, (SCSH *)drsp, sizeof(DISCONN_RSP) );
	(void)splx(s);

/* Generate the Disconnect Request from the remote SYSAP		*/

	reqbp->sconnid  = rem_connid;
	reqbp->mtype = SCS_DISCONN_REQ;
	s = Splscs();	/*  Set IPL to SCS level	*/
	scs_msg_rec( pccb, reqbp, sizeof(DISCONN_REQ));
	(void)splx(s);

/* Clear flag which indicates connection exists				*/
	Pccb.uq_con &= (short)~(1<<rconn);
	Pccb.ncon--;

	
}

/*
 *
 *
 *	Name:		uq_map_buf
 *	
 *	Abstract:	This routine maps a given buffer into Unibus
 *			space and returns to the caller a buffer handle
 *			(Unibus address).
 *			
 *	Inputs:
 *
 *	pccb			- Port Command and Control Block ptr
 *	bhp			- Buffer handle ptr
 *	sbh			- Ptr to buf structure for buffer to be
 *				  mapped.
 *	scsid			-
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
u_long	uq_map_buf( pccb, bhp, sbh, scsid)
PCCB			*pccb;
BHANDLE			*bhp;
register struct buf 	*sbh;
u_long			scsid;
{
int i, ctlr;
struct uba_ctlr	*um;


	ctlr = Pccb.uq_ctlr;		/* Get controller number	*/
	um = uqminfo[ctlr];		/* Get pointer to uba_ctlr struct */

/*								
 *	On machines with buffered data paths a data path is semi-
 *	permanently allocated. It is released whenever no map requests
 *	are outstanding and re-allocated when a map request comes in
 *	and there are no outstanding map requests. The following code
 *	checks if there are any outstanding map requests. If there are
 *	none then a buffered datapath is allocated.		
 */

	if ((um->um_hd->uba_type &(UBA750|UBABUA))
		&& (Pccb.map_requests == 0)) {

		if (um->um_ubinfo != 0)
			mprintf("uq_map_buf: ubinfo 0x%x\n",um->um_ubinfo);
		else
			um->um_ubinfo =
			   uballoc(um->um_ubanum, (caddr_t)0, 0,
				UBA_NEEDBDP);
	}



	if (um->um_hd->uba_type&(UBABDA|UBAXMI))
			i = UBA_CANTWAIT;
	else if (um->um_hd->uba_type&UBA780)
			i = UBA_NEEDBDP|UBA_CANTWAIT;
	else if (um->um_hd->uba_type&(UBABUA|UBA750))
			i = um->um_ubinfo|UBA_HAVEBDP|UBA_CANTWAIT;
	else if (um->um_hd->uba_type&UBA730)
			i = UBA_CANTWAIT;
	else if (um->um_hd->uba_type&(UBAUVI|UBAUVII))
			i = UBA_CANTWAIT|UBA_MAPANYWAY;

	/* get the buffer mapped */
	if(Pccb.step1r & UQ_QB) /* it's a Q-22 bus so use all 8k map reg */
	{
		if ((i = qbasetup(um->um_ubanum, sbh, i)) == 0) 
			return (RET_ALLOCFAIL);
	
	} else {
		if ((i = ubasetup(um->um_ubanum, sbh, i)) == 0) {
	
			if ((um->um_hd->uba_type &(UBA750|UBABUA))
				&& (Pccb.map_requests == 0))  /* Release data path if necessary */
			 	  ubarelse(um->um_ubanum, &um->um_ubinfo);
	
			return (RET_ALLOCFAIL);
		}
	}

/*	
 *	Fill in the buffer handle passed
 */

	Pccb.map_requests++;	/* Increment number of outstanding maps	*/

	/*  These two lines of code are intended to zero out all
		of the bits in the buffer handle; we are assuming
		that the two fields in the un_mapped form of
		the union cover every bit in every variation on
		the union. */
	Bh.un_mapped.buf_add = 0;
	Bh.un_mapped.unused = 0;

	if( uba_hd[um->um_ubanum].uba_type & (UBAUVI )) {

		Bh.mapped.map_idx = (i & 0x3ffff) | UQ_MAP;
		Bh.mapped.map_base = (long)
			&(uba_hd[um->um_ubanum].uh_physuba->uba_map[0]);

	}
	else 	if (uba_hd[um->um_ubanum].uba_type&UBABDA) {
			Bh.mapped.map_idx = (i & 0x3ffff) | UQ_MAP;
			Bh.mapped.map_base = 
				(long)svtophy(&(uba_hd[um->um_ubanum].uh_uba->uba_map[0]));
		}
        else	if(uba_hd[um->um_ubanum].uba_type & UBAXMI) {
			Bh.mapped.map_idx = (i & 0x3ffff) | UQ_MAP;
			Bh.mapped.map_base = 
				(long)svtophy(&(uba_hd[um->um_ubanum].uh_uba->uba_map[0]));
		        
	}
	else	if(Pccb.step1r & UQ_QB) {/* it's a Q-22 bus so use all 8k map reg */
			Bh.un_mapped.buf_add = (i & 0x3fffff);
			Bh.un_mapped.unused = 0;
		}
	else	{
		Bh.un_mapped.buf_add = (i & 0x3ffff) | (((i>>28)&0xf)<<24);
		Bh.un_mapped.unused = 0;
	}
        bhp->scsid = scsid;
	if (um->um_hd->uba_type&(UBABUA|UBA750|UBABDA|UBAXMI))
		i &= 0xfffffff;         /* mask off bdp */
	sbh->b_ubinfo = i;		/* Save mapping info for unmap	*/
	return(RET_SUCCESS);
}

/*
 *
 *
 *	Name:		uq_unmap_buf
 *	
 *	Abstract:	This routine releases the Unibus mapping resources
 *			allocated by uq_map_buf for the given buf structure.
 *			
 *	Inputs:
 *
 *	pccb			- Port Command and Control Block ptr
 *	bhp			- Buffer Handle ptr
 *	sbh			- Ptr to buf structure to "unmap"
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
void uq_unmap_buf(pccb, bhp, sbh)
PCCB			*pccb;
BHANDLE			*bhp;
register struct	buf	*sbh;
{
int ctlr;
struct uba_ctlr	*um;

	ctlr = Pccb.uq_ctlr;		/* Get controller number	*/
	um = uqminfo[ctlr];		/* Get pointer to uba_ctlr struct */

	--Pccb.map_requests;		/* One less outstanding map	*/
#ifdef mips
/*
 * On a mips machine, if we did a DMA read, we need the flush
 * the cache.
 */
	if (sbh->b_flags & B_READ)
	{
		switch (cpu) {
		case DS_5800:		/* Hardware assist here.	*/
			break;
		default:
			bufflush(sbh);
		}
	}
#endif mips


/*
 *	Only release buffered data path if there are no outstanding mapped
 *	requests.
 */
	if ( (um->um_hd->uba_type & (UBA750|UBABUA)) ) {
		if (Pccb.map_requests == 0) {  /* Release data path if necessary */

			if (um->um_ubinfo == 0)
				mprintf("uq_unmap_buf: ubinfo == 0");
			else
				ubarelse(um->um_ubanum, &um->um_ubinfo);
		}
		else 
			ubapurge(um);
	}


		
	if(Pccb.step1r & UQ_QB) /* it's a Q-22 bus so use all 8k map reg */
		qbarelse(um->um_ubanum, (int *)&sbh->b_ubinfo);
	else
		ubarelse(um->um_ubanum, (int *)&sbh->b_ubinfo);
	return;
}

/*
 *
 *
 *	Name:		uq_open_path
 *	
 *	Abstract:	This routine should never be called for UQ ports.
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
u_long	uq_open_path(pccb,pb)
PCCB	*pccb;
PB	*pb;
{
/*
 * UQSSP  paths never initiate connects and therefore this routine 
 * should never be called.
 */

	panic("uqdriver: Attempt to open path");
}

/*
 *
 *
 *	Name:		uq_crash_path
 *	
 *	Abstract:	This routine is called to crash the  path
 *			specified by the Path Block.
 *			
 *	Inputs:
 *
 *	pccb			- Port Command and Control Block ptr
 *	pb			- Path Block ptr
 *	reason			- Reason for path crash
 *	disposal		- Flag for disposal of optional buffer
 *	scsbp			- Optional buffer
 *	
 *	NOTE: 		When reason == E_SYSAP, scsbp is a pointer to a
 *	      		charecter string of size NAME_SIZE containing the name
 *	      		of the SYSAP responsible for crashing the path.
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
void	uq_crash_path(pccb,pb,reason,disposal,scsbp)
PCCB	*pccb;
PB	*pb;
u_long	reason;
u_long	disposal;
SCSH	*scsbp;
{

u_long	nreason = 0;

    Lock_pccb (pccb);

    if ((Pccb.init_leader == UQPath_Is_Up) &&
      (Pccb.reset_leader == UQPath_Is_Up) &&
      (Pccb.pb != 0))
    {

    /* Exit if the path is already failed.  SHOULD LOG THIS CASE */
        if( pb->pinfo.state == PS_PATH_FAILURE) {
    		Unlock_pccb(pccb);
		return;
	}

    /* Set the PC flag within the event code when the path is currently open.
     */

	if( pb->pinfo.state == PS_OPEN ) {
	    Set_pc_event( reason )
	}

    Unlock_pccb(pccb);

    /* Map the specific reason for crashing the path into the reason which 
     * will be transmitted to the local SYSAPs
     */

	if( Test_spc_event( reason )) {
	    if( Ecode( reason ) <= Ecode( SE_MAX_SCS )) {
		nreason = scs_map_spc[ Ecode( reason ) - 1 ];
	    }
	} else if( Test_pc_event( reason )) {
	    if( Ecode( reason ) <= Ecode( E_MAX_SCS )) {
		nreason = scs_map_pc[ Ecode( reason ) - 1 ];
	    }
	}
	if( nreason == 0 ) {
	    panic("uqdriver: invoked with illegal path crash reason");
	}
	Cprintf("Called from crash_path, reason = %x\n", nreason);
	uq_disable( pccb, nreason );
 	}
	 else Unlock_pccb(pccb);
}

/*
 *
 *
 *	Name:		uq_get_pb
 *	
 *	Abstract:	This routine returns a pointer to the PB for
 *			the specified port. In the case of the UQ port
 *			there is only one PB.
 *			
 *	Inputs:
 *
 *	pccb			- Pointer to PCCB for this port
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	
 *
 *	Return 
 *	Values:
 *
 *	pccb
 *	     pd.uq.pb		- Pointer to PB for this port
 *	
 *	
 * 
 *	Side		
 *	Effects:
 *
 */
PB	*uq_get_pb(pccb,scsbp,type)
PCCB	*pccb;
SCSH	*scsbp;
u_long  type;
{
		return(Pccb.pb);
}

/*
 *
 *
 *	Name:		uq_remove_pb
 *	
 *	Abstract:	This is where the PB and SB are removed from system 
 *			queues and deallocated. A check is made on the 
 *			reinit counter. If it is 0 then we give up 
 *			else a fork is done to re-init the controller and 
 *			control is returned to SCS.
 *			
 *	Inputs:
 *
 *	pccb			- Port Command and Control Block ptr
 *	pb			- Path Block ptr
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
void	uq_remove_pb(pccb,pb)
PCCB	*pccb;
PB	*pb;
{
SB	*sb;

	Lock_pccb (pccb);
	sb = pb->sb;
	Pccb.pb = 0;	/* Zero pb pointer		*/
	Remove_entry( pb->flink );
	Remove_entry( sb->flink );
	Unlock_pccb (pccb);	
	(void)scs_dealloc_pb( pccb,pb );
	(void)scs_dealloc_sb( sb );	
	if ( Pccb.reinit_cnt ) {
		--Pccb.reinit_cnt;
		Kfork(&pccb->forkb, uq_init, pccb )
	}
}


/*
 *
 *
 *	Name:		uq_mstart
 *	
 *	Abstract:	This function is not used in the UQ port. A return
 *			status of RET_SUCCESS is always returned.
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
u_long	uq_mstart(pccb, rport_addr, start_addr)
PCCB	*pccb;
scaaddr	*rport_addr;
u_long	start_addr;
{
	return(RET_SUCCESS);
}

/*
 *
 *
 *	Name:		uq_mreset
 *	
 *	Abstract:	Maintenance reset causes the port to crash and 
 *			attempt to restart.
 *			
 *	Inputs:
 *
 *	pccb			- Port Command and Control Block pointer
 *	rport_addr		- Address of remote port to reset
 *	force
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
u_long	uq_mreset(pccb, rport_addr, force)
PCCB	*pccb;
scaaddr	*rport_addr;
u_long	force;
{

	/*  We are at the appropriate IPL because we were called by SCS */
	Lock_pccb (pccb)
	if (Pccb.reset_leader == UQPath_Is_Up) {
	Pccb.reset_leader = CURRENT_CPUDATA->cpu_num;
	Unlock_pccb(pccb);
	Cprintf("Called from uq_mreset\n");
	uq_disable( pccb, PF_SYSAP );	/* SYSAP requested port crash	*/
	}
	else Unlock_pccb (pccb);
	/* We return success whether or not we are the reset_leader,
		because SCS success means we have successfully initiated the
		reset process, not completed it; and either we have or
		else a different cpu has done it for us */
	return(RET_SUCCESS);
}

/*
 *
 *
 *	Name:		uq_disable
 *	
 *	Abstract:	This routine controls the orderly shutdown of a
 *			UQ port and starts the sequence leading to possible
 *			restart of the port.
 *			
 *	Inputs:
 *
 *	pccb			- Port Command and Control Block pointer
 *	reason			- Reason for port shutdown
 *	
 *	
 *
 *	Outputs:
 *
 *	
 *	SMP:		We lock the pccb to set the rip flag and
 *			clear the leader flags.  The leader flags
 *			enforce having just one cpu call uq_disable().
 *			The rip flag enforces having just one chain
 *			of execution get from here to uq_init ().
 *			uq_init() will lock the pccb to reset the rip
 *			flag and set an init leader.
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
uq_disable( pccb, reason )
PCCB	*pccb;
u_long	reason;
{
PB	*pb;
int s;

	s = splbio();
	Cprintf("uq_disable entered, reason = %x, pccb = %x\n", reason, pccb);
	if (Pccb.rip == 0) {	/* If recovery not already in progress  */
	     Lock_pccb (pccb);
	     if (Pccb.rip == 0) {  /* does it still == 0?  if not, leave */
	     Pccb.rip = 1;		/* Set recovery in progress	*/
	     Pccb.init_leader = UQPath_Is_Up; /* in case it is a reinit */
	     Pccb.reset_leader = UQPath_Is_Up;
	     Unlock_pccb (pccb);

	     uq_port_reset(pccb);

	     if( (pb = Pccb.pb) != (PB *)NULL ) {
		pb->pinfo.state = PS_PATH_FAILURE;
		pb->pinfo.reason = reason; /* Reason for port crash */
	     }

	     if ( (Lpinfo.uq_flags & UQ_TIM) != 0) { /* Turn of timer if on */
		(void)untimeout(uq_timer, pccb);
		Lpinfo.uq_flags &= ~UQ_TIM; /* Timer now off		*/
	     }
	     Kfork(&pccb->forkb, uq_cleanup, pccb )
	} /* end if */
	     else Unlock_pccb (pccb);
	} /* end if */
	(void)splx(s);
	return;				/* If already forking then	*/
					/* forget this request.		*/
}

/*
 *
 *
 *	Name:		uq_cleanup
 *	
 *	Abstract:	This routine processes any outstanding port responses
 *			and then notifies SCS of port shutdown. SCS will
 *			eventually call uq_remove_pb which will restart the
 *			port.
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
void uq_cleanup(pccb)
PCCB	*pccb;

{
PB	*pb;
int 	s = Splscs();

/*
This is a last ditch attempt to save some remaining responses.  This
has been deemed to dangerous.
	uq_poll_rspring( pccb );
*/
	if ( (pb = Pccb.pb) != (PB *)NULL ) { /* If we have a path open */
		scs_path_crash( Pccb.pb ); /* Notify SCS		*/
	}
	else if ( Pccb.reinit_cnt ) {
		--Pccb.reinit_cnt;
		Kfork(&pccb->forkb, uq_init, pccb )
	}
	splx(s);
	return;			/* Return from fork			*/
}

/*
 *
 *
 *	Name:		
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
uq_attach(ui)
struct uba_device *ui;
{
	return;
}

/*
 *
 *
 *	Name:		
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
uq_slave(ui,reg)
struct uba_device	*ui;
u_char			*reg;
{
	return( 0 );
}

u_short	reset_uq = 0;
/*
 *
 *
 *	Name:		uq_timer
 *	
 *	Abstract:	This routine is called every second. It polls the
 *			sa register to check on the state of the controller.
 *			If the controller has a fatal error the port is
 *			crashed.
 *			
 *	Inputs:
 *
 *	pccb			- Port Command and Control Block pointer
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
uq_timer(pccb)
PCCB	*pccb;
{
	int s, i;
	u_short	local_reset_uq = 0;

	s = Splscs ();
	Lock_pccb (pccb);
	if (reset_uq) {
		uq_port_reset(pccb);
		local_reset_uq = 1;
		reset_uq = 0;
	}
	Unlock_pccb (pccb);
	splx(s);
	
	Lpinfo.uq_flags &= ~UQ_TIM;		/* Timer off		*/
	if (((Lpinfo.sa = *Pccb.Uqsa) & UQ_ERR) || local_reset_uq) {
		/* Got a fatal port err */
		uq_error_log( pccb, UQ_SA_FATAL); /* Log the error	*/
		uq_disable(pccb, PF_PORTERROR );
		return;
	}
	else {	
		/* If SCS requests pending, then process them		*/
		s = Splscs ();
		Lock_pccb (pccb);
		if(Pccb.scswaitq.flink != &Pccb.scswaitq) {
			Unlock_pccb (pccb);
			uq_dispatch(pccb);
		}
		else	Unlock_pccb (pccb);
		splx (s);

		uq_poll_rspring( pccb ); /* Check response ring for grins */
		s = splbio();		 /* Raise IPL to muck with rings */
		i = *Pccb.Uqip;		 /* Poll for grins also		*/
		splx(s);
					/* Reset timer */
		timeout(uq_timer,pccb,Pccb.poll_rate);
		Lpinfo.uq_flags |= UQ_TIM;	/* Timer on		*/
	}
}

/*
 *
 *
 *	Name:		uq_reset
 *	
 *	Abstract:	This routine is called when the uba is reset. All
 *			mapping is gone at the time of the call.
 *
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
uq_reset(uban)
int	uban;
{
register PCCB		*pccb;
struct port_info	*pcinfo;
struct uba_ctlr		*um;
int			d, s;


	for(d = 0; d < NUQ; d++) {
		if ((um = uqminfo[d]) == 0 || um->um_ubanum != uban ||
			um->um_alive == 0)
			continue;
		pcinfo = port_info_ptr[d];
		pccb = pcinfo->pc_ptr;


		/*  Now bang the port on the head unless somebody else
			happens to be doing it for us */
		s = splbio();
		Lock_pccb (pccb);

		Pccb.uq_mapped = 0;/* All mapping undone */

		if (Pccb.init_leader == UQPath_Is_Up) {
		Pccb.init_leader = CURRENT_CPUDATA->cpu_num;
		Unlock_pccb (pccb);
		if (Pccb.init_leader == CURRENT_CPUDATA->cpu_num) {
			uq_port_reset(pccb);
			(void)splx(s);
			uq_disable(pccb, PF_PORTERROR);
			} /* end if */
		}	
		   else
		   {
		   Unlock_pccb (pccb);
		   (void)splx(s);
		   }
		return;
	  	}  /* end for */
}

/*
 *
 *
 *	Name:		uq_error_log
 *	
 *	Abstract:	This routine provides the interface to the system
 *			error log facility. It is called with the error
 *			type.
 *			
 *	Inputs:
 *
 *	pccb		- Pointer to PCCB for offending port
 *	code		- Error code:
 *				Fatal SA register error
 *				Reset failed
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
 *		The error log code is called and a error log packet is
 *		reserved. The appropriate values are stored in the packet
 *		and the packet is then validated.
 */
void	uq_error_log( pccb, code)
PCCB	*pccb;
u_long	code;
{
register struct el_rec	*elp;

/*								*/
/*	Allocate error log packet				*/
/*								*/
	if ((elp = ealloc(sizeof(struct el_uq), 
		EL_PRIHIGH)) == EL_FULL)
		return;

/*								*/
/*	Initialize subid fields					*/
/*								*/
	LSUBID( elp,
		ELCT_DCNTL,
		ELUQ_ATTN,
		Lpinfo.uq_type,
		EL_UNDEF,
		Pccb.uq_ctlr,
		code)

	elp->el_body.eluq.sa_contents = Lpinfo.sa; /* Store sa value	*/
/*								*/
/*	Validate error log packet				*/
/*								*/
	EVALID( elp )
}
