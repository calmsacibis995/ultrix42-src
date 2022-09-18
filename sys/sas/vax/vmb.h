/*	@(#)vmb.h	4.1	(ULTRIX)	7/2/90	*/
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 *
 *   Modification history:
 *
 * 06-Mar-90 -- rafiey (Ali Rafieymehr)
 *	Defined BTD$K_KDM70 for KDM70.
 *
 * 14-Jul-88 -- fred (Fred Canter)
 *	Changed PVAX SCSI disk BTD from 36 to 42 (match new VMB).
 *
 * 02-Mar-87 -- logcher
 *	Merged in diskless changes, removed asrvname, asrvipadr,
 *	selflg from vmb_info.netblk and added new fields
 */

#include "../mop.h"
#define IO$_SEEK       0x2
#define IO$_WRITELBLK  0x20
#define IO$_READLBLK   0x21
#define IO$_READPBLK   0x0c
#define IO$_REWIND     0x24
#define IO$_SKIPFILE   0x25
#define IO$_SKIPRECORD 0x26
#define PHYSMODE   0x00
#define VIRTMODE   0x01
/*
 * VMS $RPBDEF
 */
#define	RPB$L_BOOTR3	40
#define	RPB$L_BOOTR5	48
#define RPB$L_IOVEC	52
#define RPB$L_IOVECSZ	56
#define RPB$Q_PFNMAP	68
#define RPB$L_PFNCNT	76
#define RPB$L_CSRPHY	84
#define RPB$L_CSRVIR	88
#define RPB$L_ADPPHY	92
#define RPB$L_ADPVIR	96
#define RPB$W_UNIT	100
#define RPB$B_DEVTYP	102
#define RPB$T_FILE	104
/*
 * VMS $VMBARGDEF
 */
#define	VMB$L_ARGBYTCNT	0	/* offset to the arg count */
#define	VMB$Q_FILECACHE	4	/* FILEREAD Cache Descriptor */
#define	VMB$L_LO_PFN	12	/* Lowest PFN found by VMB */
#define	VMB$L_HI_PFN	16	/* Highest PFN exclusive */
#define	VMB$Q_PFNMAP	20	/* PFN Bitmap descriptor */
#define	VMB$Q_UCODE	28	/* Loaded ucode descriptor */
#define	VMB$B_SYSTEMID	36	/* 48 bit SCS systemid */
#define	VMB$L_FLAGS	44	/* Word of flags */
#define	VMB$L_CI_HIPFN	48	/* Highest PFN used by CI code */
#define	VMB$Q_NODENAME	52
#define	VMB$Q_HOSTADDR	60
#define	VMB$Q_HOSTNAME	68
#define	VMB$Q_TOD	76
#define	VMB$L_XPARAM	84
#define	VMB$L_BVP_PGTBL	88	/* Used with BVP boot driver */

#ifndef LOCORE
struct vmbargs {
	int	argbytcnt;
	int	filecache[2];
	int	lo_pfn;
	int	hi_pfn;
	int	pfnmap[2];
	int	ucode[2];
	char	systemid[8];
	int	flags;
	int	ci_hipfn;
	char	nodename[8];
	unsigned char	hostaddr[8];
	char	hostname[8];
	char	tod[8];
	int	xparam;
	int	bvp_pgtbl;
};
#endif /* LOCORE */

/*
 * ERROR Status
 */
#define SS$_NORMAL	1
#define SS$_CTLRERR	84
#define SS$_DEVOFFLINE	132
#define SS$_PARITY	500
#define SS$_BUFBYTALI	780
#define SS$_NOSUCHDEV	2312
#define SS$_ENDOFFILE	2160
/*
 * VMS $BQODEF
 */
#define	BQO$L_QIO	0		/*used*/
#define	BQO$L_MAP	4
#define	BQO$L_SELECT	8
#define	BQO$L_DRIVRNAME	12
#define	BQO$W_VERSION	16
#define	BQO$W_VERCHECK	18
#define	BQO$L_RESELECT	20
#define	BQO$L_MOVE	24
#define	BQO$L_UNIT_INIT	28		/*used*/
#define	BQO$L_AUXDRNAME	32
#define	BQO$L_UMR_DIS	36
#define	BQO$L_UCODE	40		/*used*/
#define	BQO$L_UNIT_DISC	44
#define	BQO$L_DEVNAME	48
#define	BQO$L_UMR_TMPL	52
#define	BQO$B_UMR_DP	56
#define	BQO$B_CPUTYPE	57
#define	BQO$L_CPUDATA	58
#define	BQO$L_TENUSEC	62
#define	BQO$L_UBDELAY	66

/*
 * TODO:
 *	Some of the BTD$ codes don't match the VMS
 *	VMB codes. The following changes are needed:
 *
 *	1. BTD$K_KA640_* should be replaced with BTD$K_KA410_*.
 *	2. BTD$K_KA420_DISK is 42 it should be 36.
 *	3. Need BTD$K_KA420_5380_DISK 42 and BTD$K_KA420_5380_TAPE 37
 *	   for PVAX SCSI disks and tapes.
 *
 *	It's too close to release time to make these changes now.
 */
#define BTD$K_MB	0		/* MASSBUS */
#define BTD$K_DM	1		/* RK06/RK07 */
#define BTD$K_DL	2		/* RL01/RL02 */
#define BTD$K_DQ	3		/* IDC/R80/RL02 */
#define BTD$K_PROM	8		/* On UVAX (Q-bus Prom) */
#define BTD$K_PROM_COPY	9		/* PROM copied to host memory */
#define BTD$K_UDA	17		/* RQDX/UDA50/KDA50 */
#define BTD$K_TK50	18		/* TK50 (Maya) */
#define BTD$K_HSCCI	32		/* HSC on a CI */
#define BTD$K_BDA	33		/* KDB50 on a BI */
#define BTD$K_BVPSSP	34		/* AIO on a BI */
#define BTD$K_AIE_TK50	35		/* AIE/TK50 */
#define BTD$K_KA640_DISK 36		/* ST506 disk */
#define BTD$K_KA410_DISK 36		/* */
#define BTD$K_KA640_TAPE 37		/* SCSI TK50 */
#define BTD$K_KA410_TAPE 37		/* */
#define BTD$K_KA420_TAPE 37		/* PVAX SCSI TZ30/TZK50 */
#define BTD$K_DISK9	38		/* */
#define BTD$K_SII	39		/* MSI (DSSI) disk */
#define BTD$K_KFSQSA	40		/* RF controller ? */
#define BTD$K_SHAC	41		/* */
#define	BTD$K_KA420_DISK 42		/* PVAX SCSI disk */
#define BTD$K_KDM70	43      	/* KDM70 */
#define BTD$K_CONSOLE	64		/* Console block storage device */
#define BTD$K_NET_DLL	96		/* */
#define BTD$K_QNA	96		/* DEQNA */
#define BTD$K_AIE_NI	98		/* DEBNA */
#define BTD$K_KA640_NI	99		/* NI Option */
#define	BTD$K_LANCE	99		/* LANCE NI device */
#define BTD$K_NISCS	128		/* */

#define BLKSIZ		512
#define BBLKS		40
#define BOOTSZ		BLKSIZ*BBLKS
#define RPBSIZ		512
#ifndef KERNEL
#define HIGH		31
#endif /* KERNEL */

/*
 * Type of boot
 */
#define VMB_BOOT	0x01
#define ROM_BOOT	0x02
#define TK50_BOOT	0x04
#define NET_BOOT	0x08
/*
 * Miscellaneous
 */
#define CONTROL_S 19
#define CONTROL_Q 17
#define INBUFSZ 256
/*
 * Structure in which to pass information between boot and vmunix
 * WARNING:  If this structure changes, it will be necessary to change the 
 * offsets below.
 */
#ifndef LOCORE
struct	vmb_info {
	int	vmbvers;	/* VMB version number */
	int	memsiz;		/* size of available physical memory in pages*/
	int	btdrsiz;	/* size of the boot driver */
	int	*btdrbas;	/* base address of the boot driver */
	int	rpbsiz;		/* size of the RPB */
	int	*rpbbas;	/* base address of the RPB */
	int	vmbargsiz;	/* size of the VMB argument list */
	int	*vmbargbas;	/* base address of the VMB argument list */
	int	ciucodesiz;	/* size of the CI ucode */
	int	*ciucodebas;	/* base address of the CI ucode */
	int	future[10];	/* Future expansion */
	struct netblk netblk;
};
#ifdef KERNEL
struct	vmb_info vmb_info;
#endif /* KERNEL */
#endif /* LOCORE */
/*
 * The above structure is filled in by the startup code which is assembly
 * code.  Therefore, it is necessary to build the following set of 
 * definitions to access the fields as offsets off the base address of
 * the structure.
 */
#define INFO_SIZE	40	/* Size of VMB info list */
#define INFO_VMBVERS	0x00	/* VMB version number */
#define INFO_MEMSIZ	0x04	/* max address of memory + 1 */
#define INFO_BTDRSIZ	0x08	/* size of the boot driver */
#define INFO_BTDRBAS	0x0c	/* phys address of the boot driver */
#define INFO_RPBSIZ	0x10	/* size of the RPB */
#define INFO_RPBBAS	0x14	/* phys address of the RPB */
#define INFO_VMBARGSIZ	0x18	/* size of the VMB arg list */
#define INFO_VMBARGBAS	0x1c	/* phys address of the VMB arg list */
#define INFO_CIUCODSIZ	0x20	/* size of the UCODE */
#define INFO_CIUCODBAS	0x24	/* phys address of the UCODE */
