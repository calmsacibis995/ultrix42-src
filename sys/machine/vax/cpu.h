/*
 * @(#)cpu.h	4.3  (ULTRIX)        10/10/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985,86,87,88 by			*
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


/* ------------------------------------------------------------------------
 * Modification History:
 *
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 *	Added support for VAX9000.
 *
 * 14-Oct-89 -- robin
 *	Added CPU_HDR for multi include protection.
 *
 * 24-May-89 -- darrell
 *	Moved the cpu SID, SID extension, and cpu subtype defines from here
 *	to ../common/cpuconf.h
 *
 * 09-May-89 -- gmm (v3.1 merge)
 *	Added SB_TMII for cpu_sub_subtype (for MicroVAX 3100)
 *
 * 5-May-89 -- Adrian Thoms
 *	Merged in V_VAX definitions
 *	MOVED VAX_6200 from 9 to 14
 *	Fixed scsvar.c comment for new arch
 *
 * 27-Sep-88 -- Tom Kong
 *	Added support for Rigel VAX6400.  Although Rigel cpu returns
 *	a SID cpu type of 11, the global variable "cpu" is set to
 *	13.  This is to avoid conflict with VAX420 which uses the
 *	11th slot in the cpu switch table.
 *
 * 25-Sep-88 -- Fred Canter
 *	Clean up comments.
 *	Change VS_SCSIPR to VS_DRV2RX33.
 *
 * 16-Aug-88 -- Robin
 *	Added a define to allow the ka655 cpu type to be known. 
 *
 * 19-Jul-88 -- Fred Canter
 *	Change cfgtst register bit definitions for SCSI/SCSI
 *	controller from 0x8000 to 0x4000.
 *
 * 09-Jul-88 -- Todd M. Katz
 *	Add a comment to the effect that the vector cpu_types[] within
 *	../vaxscs/scsvar.c must be updated when new cpu values are defined.
 *
 * 07-Jun-88 -- darrell
 *	Added support for VAX60
 *
 * 19-May-88	fred (Fred Canter)
 *	Modified VAXstar CPU definitions for CVAXstar/PVAX.
 *
 * 26-Apr-88    jaw
 *	Add VAX8820 support.
 *
 * 25-Feb-1988- robin
 *	Clarify systype and subtype constants for Mayfair CVAX systems.
 *   		Added ST_CVAXQ for uniprocessor Qbus CVAX (ka640 & ka650)
 *		Changed ST_KA650 to SB_KA650
 *		Added SB_KA640 for ka640
 *
 * 15-Feb-88 -- fred (Fred Canter)
 *	Define new CPU ID for VAX420 (CVAXstar/PVAX).
 *	CPU ID is 11 and does NOT match the SID register,
 *	which contains 10 for the CVAX chip.
 *	Save ARCH_ID bits from SYS_TYPE register (may not be used).
 *
 * 18-Jan-88 -- jaw
 *	Add support for VAX6200
 *
 * 20-Apr-87 -- afd
 *	Changed name CVAXQ to VAX3600 for Mayfair.
 *
 * 06-Mar-87 -- afd
 *	Added C_VAX with cpu number 10; and ka650 sub-type of 1.
 *
 *  5-Aug-86 -- fred (Fred Canter)
 *	Changed VS_TEAMMATE to VS_MULTU to match VS410 spec.
 *
 * 28-Jul-86 -- bglover
 *	Added defines for cpu_subtype for Scorpio and Nautilus
 *
 *  2-Jul-86 -- fred (Fred Canter)
 *	Changed VS_JMPWX to VS_TEAMMATE (ID VAXstar vs TEAMmate CPU).
 *
 * 18-Jun-86 -- fred (Fred Canter)
 *	Changes for VAXstar kernel support.
 *
 * 03-Mar-86 -- darrell
 *	Removed the percpu structure - moved some of the fields in
 *	percpu to cpusw.
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *
 * 28 Jan 86 -- darrell
 *	The generic machine check stack frame structure is now 
 *	defined here, as part of the machine dependent code 
 *	restructuring.
 *
 *  3 Jul 85 -- jrs
 *   	add support for vax 8800.
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 *  20 Mar 85 -- jaw
 *   	add support for vax 8200.
 *
 *  4 Nov 84 -- rjl
 *	Generalized the percpu structure to handle the MicroVAX-II's treatment
 *	of the local register space (nexus) and the q-bus.
 *
 * 19 Aug 84 -- rjl
 *	Added support for MicroVAX-II
 *
 * 30 Nov 83 --tresvik
 *	Split the cp_eco field into eco and subeco levels
 *	for the 11/780.  Removed cp_hrev for the 11/730 as per CSSE
 *	the SRM (reserved field).
 *
 *  6 Oct 83 --jmcg
 *	Had to make a comment out of sccsid.
 *
 *  5 Oct 83 --jmcg
 *	Added structs and defines for VAX 730 and MicroVAX.
 *
 *  3 Oct 83 --jmcg
 *	Derived from 4.2BSD pre-release, labeled:
 *		cpu.h	6.1	83/07/29
 *	It must be RE-MASTERED when 4.2BSD final release arrives.
 * ------------------------------------------------------------------------
 */

/* Protect the file from multi includes.
 */

#ifndef CPU_HDR
#define CPU_HDR

#ifndef LOCORE
/*
 * Cpu identification, from SID (and SID extension) register.
 */
union cpusid {
	int	cpusid;
	struct cpuany {
		u_int	:24,
			cp_type:8;
	} cpuany;
	struct cpu9000 {
		u_int	cp_sno:12,		/* serial number */
			cp_plant:4,		/* plant number */
			cp_rev:6,		/* eco level */
			cp_type_id:2,		/* Type of system */
			cp_type:8;		/* VAX_9000 */
	} cpu9000;
	struct cpu8800 {
		u_int	cp_sno:16,
			cp_eco:7,
			cp_lr:1,
			cp_type:8;
	} cpu8800;
	struct cpu8820 {
		u_int	cp_sno:16,
			cp_eco:6,
			cp_0123:2,
			cp_type:8;
	} cpu8820;
	struct cpu8600 {
		u_int	cp_sno:12,		/* serial number */
			cp_plant:4,		/* plant number */
			cp_eco:8,		/* eco level */
			cp_type:8;		/* VAX_8600 */
	} cpu8600;
	struct cpu8200 {
		u_int	cp_urev:8,
			cp_secp:1,
			cp_patch:10,
			cp_hrev:5,
			cp_type:8;
	} cpu8200;
	struct cpu780 {
		u_int	cp_sno:12,		/* serial number */
			cp_plant:3,		/* plant number */
			cp_subeco:4,		/* sub-system-rev */
			cp_eco:5,		/* eco level */
			cp_type:8;		/* VAX_780 */
	} cpu780;
	struct cpu750 {
		u_int	cp_hrev:8,		/* hardware rev level */
			cp_urev:8,		/* ucode rev level */
			:8,
			cp_type:8;		/* VAX_750 */
	} cpu750;
	struct cpu730 {
		u_int	:8,			/* reserved */
			cp_urev:8,		/* ucode rev level */
			:8,			/* reserved */
			cp_type:8;		/* VAX_730 */
	} cpu730;
	struct cpuMVI {				/* MicroVAX I */
		u_int	cp_hrev:8,		/* hardware rev level */
			cp_urev:8,		/* ucode rev level */
			:8,
			cp_type:8;		/* MVAX_I */
	} cpuMVI;
	struct cpuMVII {			/* MicroVAX II */
		u_int	cp_hrev:8,		/* hardware rev level */
			cp_urev:8,		/* ucode rev level */
			:8,
			cp_type:8;		/* MVAX_II */
	} cpuMVII;
 	struct cpuCVAX {			/* CVAX and CVAXstar */
 		u_int	cp_urev:8,		/* ucode rev level */
 			:16,
 			cp_type:8;		/* CVAX */
 	} cpuCVAX;
 	struct cpuRIGEL {			/* RIGEL VAX64xx */
 		u_int	cp_urev:8,		/* ucode rev level */
 			cp_uopt:8,		/* ucode options */
			:8,
 			cp_type:8;		/* RIGEL */
 	} cpuRIGEL;
	struct cpuVVAX {                        /* Virtual VAX 	       */
		u_int   cp_eco:16,              /* eco level           */
			cp_vtype:8,             /* VVAX version number */
			cp_type:8;              /* VVAX                */
	} cpuVVAX;
	/* note: structs for 750, 730, and MicroVAX_I have the same layout */
	/* MicroVAX-chip-based systems will require additional logic to
	 * distinguish between implementations.
	 * Above also true for CVAX and CVAXstar.
	 */
};
#endif

/*
 * VAXstar/CVAXstar/PVAX I/O reset,
 * configuration and test register definitions.
 */

#define	VS_IORESET	0x20020000	/* VAXstar I/O reset register (w/o) */
#define	VS_CFGTST	0x20020000	/* VAXstar config & test reg (r/o) */

/*
 * The memory option type bits are not
 * defined because they are not used.
 */
#define	VS_MTYPEMASK	0x7		/* Memory option type (mask) */

#define	VS_VIDOPT	0x8		/* Video option present (color) */
#define	VS_CURTEST	0x10		/* Monochrome video present */
#define	VS_L3CON	0x20		/* SLU line 3 is the console, */
					/* same as CVAXstar/PVAX ALCON bit */
#define	VS_NETOPT	0x40		/* VAXstar, Network option present */
#define	VS_CACHPR	0x40		/* CVAXstar/PVAX, cache present */
#define	VS_MULTU	0x80		/* Set = TEAMmate, clear = VAXstar */
					/* NOTE: above not true for VS3100 */

/*
 * CFGTST register bits 15:14.
 * Storage controller configuration.
 */
#define	VS_SC_TYPE	0x0c000		/* Storage controller type mask */
#define	VS_SC_NONE	0x0c000		/* No storage controller present */
#define	VS_ST506_SCSI	0x00000		/* ST506/SCSI controller */
#define	VS_SCSI_SCSI	0x04000		/* SCSI/SCSI controller */

/*
 * CFGTST register bits 13:8.
 * Apply only to ST506-SCSI controller.
 * ST506 disk and SCSI device configuration.
 * Bit 13 is always one.
 *
 * NOTE: these bits are really masks, because a zero
 *	 in the bit indicates device present.
 */
#define	VS_DRV2RX33	0x01000		/* VS3100 floppy type (1=RX33 0=RX23) */
#define	VS_DRV3PR	0x00800		/* Third hard disk drive present */
#define	VS_DRV2PR	0x00400		/* Floppy diskette drive present */
#define	VS_DRV1PR	0x00200		/* Second hard disk drive present */
#define	VS_DRV0PR	0x00100		/* First hard disk drive present */

#ifndef LOCORE
#ifdef KERNEL
int	cpu_avail; /* number of processors that are in the system */
int	cpu_systype;	/* the real sys type register */
int	cpu;		/* CPU ID (defined above) not all match SID reg. */
int	cpu_subtype;	/* CPU subtype (defined above) from SYS_TYPE reg. */
int	cpu_archident;	/* saved ARCH_IDENT bits from SYS_TYPE register */
int	vs_cfgtst;	/* VAXstar/CVAXstar configuration and test register */
int	mb_slot;	/* M-Bus slot containing the Firefox I/O module */
#endif /* KERNEL */

/*
 * Generic Machinecheck Stack Frame
 */
struct mcframe	{
	int	mc_bcnt;		/* byte count */
	int	mc_summary;		/* summary parameter */
};
#endif /* LOCORE */

#define SPLEXTREME 0x1f
#define SPLCLOCK 0x16
#define SPLDEVHIGH 0x15
#define SPLTTY	0x15
#define SPLIMP	0x15
#define SPLBIO	0x15
#define SPLNET	0x0c		/* software */
#define SPLSOFTC 0x08
#define SPLNONE	0		/* no interrupts blocked */

/* VM fault parameter bit definitions */

#define	VM_FLT_L 0x01		/* Length violation */
#define VM_FLT_P 0x02		/* PTE reference */
#define VM_FLT_M 0x04		/* Modify or write intent */
#define VM_FLT_VAL 0x08		/* Vector alignment error */
#define VM_FLT_VIO 0x10		/* Vector I/O space reference */
#define VM_FLT_VAS 0x20		/* Vector async memory exception */

#endif /* CPU_HDR			 *//* Multi include protection ends here */
