#ifndef lint
static char *sccsid = "@(#)crashdump.c	4.7      (ULTRIX)  12/6/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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

/************************************************************************
 * Modification history: /sys/sys/crashdump.c
 *
 * 03-Dec-1990	Joe Szczypek
 *	Added return(0) to dumpsys() to satify LINT.
 *
 * 15-Sep-1990	Darrell Dunnuck
 *	Added an external definition for netdevice[].
 *
 * 13-Sep-1990	Joe Szczypek
 *	Added support for new 3max/3min TURBOchannel console ROM.  If this
 *	console is present, netdump and gendump will initialize console IO 
 *	drivers rex_execute_cmd and rex_bootinit callbacks, then do reads
 *	and writes with rex_bootread and rex_bootwrite.  Note that code has
 *	been added to build the proper boot command for rex_execute_cmd.
 *  
 * 05-Jul-1990  Pete Keilty
 *	Changed DEV_CI to DEV_BICI for booting drive from CI on BI.
 *
 * 06-Jun-1990  Joe Szczypek
 *	Mask out upper two bits of rpbp->csrphy in gendump() for
 *	BDA, BVPSSP controllers and default case.  This is done
 *	in order to support VAX6500 (Mariah).  It is only done
 *	for Mariah cpu.
 *
 * 29-May-1990  Paul Grist
 * 	mipsmate support, need to reset sii on crahdumps like pmax.
 *
 * 08-Mar-1990  Mitch McConnell
 *	Fix SCSI dump code to work for more than one controller.
 *
 * 06-Mar-90 -- rafiey (Ali Rafieymehr)
 *	Added KDM70 support.
 *
 * 28-Dec-1989  Alan Frechette
 *	Fix getting dumps on VAX8200/8300 systems over the CI. Do
 *	not turn off the caches.
 *
 * 11-Dec-1989  Alan Frechette
 *	Fixed checking whether a network boot was performed for MIPS. 
 *	A RESET blows away "bootdev". Added the variable "dumpnetboot"
 *	which gets set in init_main() if a network boot was performed.
 *
 * 28-Nov-1989  Alan Frechette
 *	Removed the call to flush_tlb() because we need to save the
 *	TLB information for MIPS dumps. Fixed being able to get dumps 
 *	on hung MIPS systems. Added the subroutine dumpsetupvectors().
 *
 * 09-Nov-1989  Alan Frechette
 *	Write a final dump descriptor of magic numbers at the end of
 *	a dump to notify savecore that the dump completed successfully.
 *	Made a few additional changes also.
 *
 * 19-Oct-1989  Alan Frechette
 *	Moved bdevsw() call from gendump() to init_main(). Save the
 *	DEVIOCGET status and information globally.
 *
 * 17-Oct-89	Fred Canter
 *	Bug fix: crashdump.c would not compile on the mips.
 *	Include ubareg.h and ubavar.h on vax and mips.
 *
 * 16-Oct-1989  Alan Frechette
 *	No longer dump out the buffer cache for partial selective
 *	crash dumps but allow a patchable option to dump it out in
 *	the case that it is needed. Full dumps are now a configureable 
 *	option.
 *
 *	Added an interface routine for IO DRIVERS that have dump
 *	routines. This routine will aid in dumping to a non generic
 *	dump device. It is called by the device driver and it returns
 * 	the next physical memory address and the number of blocks to 
 *	dump out. It will return (1) as long as there are more pages 
 *	to dump out and (0) when we are done.
 *
 *	Dump out all the processes PAGE TABLES. Before we only dumped
 *	out the user area page table but now we dump out all the PAGE
 *	TABLES.
 *
 *	Fixed problem of getting dumps on hung MIPS machines. After we
 *	hit the RESET button we reload the operating system exception 
 *	vector handling code, we flush the cache and the entire tlb.
 *
 * 12-Sep-1989 Jim Woodward/Alan Frechette
 *	Fixed a bug to dump out the runable "non-active" processes.
 *	This was a result of a logic flaw in the selective dump code.
 *
 * 24-Aug-1989 Alan Frechette
 *	Added a call to siireset() to reset the SII SCSI controllers
 *	before taking a crash dump on MIPS machines that use the SII.
 *
 * 15-Aug-1989 Jim Woodward/Alan Frechette
 *	Fixed dump code to work on SMP machines. Convert CPUDATA
 *	structure pointer from a virtual address to a physical
 *	address because dump code runs in physical mode for VAX.
 *
 * 25-July-1989	Fred Canter
 *	Conditionally include devio.h (errlog.h includes devio.h).
 *
 * 24-July-1989  Alan Frechette
 *	Created this file. Contains all the crash dump code.
 *
 ***********************************************************************/
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/mount.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/reboot.h"
#include "../h/conf.h"
#include "../h/inode.h"
#include "../h/gnode.h"
#include "../h/file.h"
#include "../h/text.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/clist.h"
#include "../h/callout.h"
#include "../h/cmap.h"
#include "../h/quota.h"
#include "../h/flock.h"
#include "../h/cpudata.h"
#include "../machine/common/cpuconf.h"
#include "../sas/mop.h"
#include "../h/dump.h"
#include "../h/errlog.h"
#include "../h/socket.h"  
#include "../net/net/if.h" 
#include "../fs/ufs/fs.h"
#include "../h/fs_types.h"
#include "../h/ioctl.h"
#ifdef mips
#include "../io/tc/tc.h"
#endif mips
#ifndef	DEVIO_INCLUDE
#include "../h/devio.h"
#endif	DEVIO_INCLUDE
#include "../h/kmalloc.h"
#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../machine/cpu.h"
#include "../machine/vmparam.h" 
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#ifdef vax
#include "../io/mba/vax/mbavar.h"
#include "../machine/cons.h"
#include "../machine/mem.h"
#include "../machine/mtpr.h"
#include "../machine/scb.h"
#include "../machine/clock.h"
#include "../machine/rpb.h"
#include "../machine/nexus.h"
#include "../machine/ioa.h"
#include "../machine/cvax.h"
#include "../machine/ka650.h"
#include "../machine/ka6200.h"
#include "../machine/ka6400.h"
#include "../machine/ka60.h"
#include "../machine/sas/vmb.h"
#endif vax

/* #define SCSI_DUMP_DEBUG		1	*/

/* Global declarations for crashdump support */
int	full_dumpmag = 0x8fca0101;	/* full dump magic number */
int	partial_dumpmag = 0x8fca0104;	/* partial dump magic number */
int	dumpmag = 0;			/* magic number for savecore */
int	dumpsize = 0;			/* initial dumpsize for savecore */
int	dumpsize2 = 0;			/* additional dumpsize for savecore */
int 	dumpdescriptors = 0;		/* dump descriptors for savecore */
int	dumpsoftpgsize = NBPG * CLSIZE;	/* software pagesize for savecore */
int	dumphardpgsize = NBPG;		/* hardware pagesize for savecore */
int	dump_io_chan;			/* io channel for mips prom driver */
int	dumpdescnum = 0;		/* for debugging purposes */
int	dumpdebug = 0;			/* for debugging purposes */
int	dumpprogpages = 0;		/* dump the per process program pages */
int	dumpbuffercache = 0;		/* dump the buffer cache pages */
int	lastkpage = 0;			/* holds last kernel page to dump */
struct	pt part_tbl;			/* partition info for system dumps */
struct	pt *parttbl;			/* pointer to the partition table */
extern 	struct smem smem[];		/* shared memory id structure */
extern 	struct sminfo sminfo;		/* shared memory info structure */
extern 	int fulldumps_on;		/* config option for full dumps on */ 
extern 	int dumptype;			/* dumptype value for network dumps */
extern 	int physmem;			/* size of physical memory */
extern 	int maxmem;			/* available physical memory */
extern 	int usrptsize;			/* size of Usrptmap[] */
extern  int cpu_sub_subtype;		/* cpu variant */
#ifdef mips
extern	rex_base;			/* Base of Callback vector table      */
extern	char netdevice[];		/* defined in machdep.c */
extern 	u_int printstate;		/* how to print to the console */
extern 	int cpu;			/* the cpu system type */
struct tlb_dump {			/* dump tlb information */
	union tlb_hi tlb_high;
	union tlb_lo tlb_low;
} tlb_dump[NTLBENTRIES];
extern	struct devget dumpdg;		/* DEVIOCGET info of dump device */
extern	int dumpdeviocgetstatus;	/* DEVIOCGET status of dump device */
extern	int dumpnetboot;		/* set if network boot performed */
extern 	struct uba_ctlr ubminit[];
extern 	uqdriver();
union 	mop_packets mop_output;		/* mop packets for network dumps */
union 	mop_packets mop_input;		/* mop packets for network dumps */

char mop_destination[6];
char broadcast[6]={0xff,0xff,0xff,0xff,0xff,0xff};
char mopdl_multicast[6]={0xab, 0,0,1,0,0};
extern  char **ub_argv;                 /* Contains boot arguments */

#define FORMAT  			/* ugly hack for scsi -do not delete */
extern struct uba_device *szdinfo[];	/*  ditto */
extern struct sz_softc sz_softc[];	/* for scsi multi-controller dumps */
#endif mips
#ifdef vax
extern 	int nodev();			/* check for no dump routine */
extern 	int szreset();			/* reset SCSI controllers */
extern 	int sdreset();			/* reset ST506 controller */
extern	int *vmbinfo;			/* physical address of vmbinfo */
extern 	struct uba_driver scsidriver;	/* scsi driver structure */
struct 	vmb_info *vmbinfop;		/* pointer to vmbinfo */
struct 	rpb *rpbp;			/* pointer to system rpb */
/*
 * Allocate and Initialize param_buf as follows
 *
 *	param_buf.pad = 1;
 *	param_buf.prot = 0x160;
 *
 * Start with a network broadcast address 
 *	0x0000010000ab 
 * 
 *	param_buf.dest[0] = 0xab; 
 *	param_buf.dest[1] = 0x00; 
 *	param_buf.dest[2] = 0x00; 
 *	param_buf.dest[3] = 0x01; 
 *	param_buf.dest[4] = 0x00; 
 *	param_buf.dest[5] = 0x00;
 */
struct {
	u_short	pad;
	u_short	prot;
	u_char	dest[6];
} param_buf = {1, 0x160, 0xab, 0, 0, 1, 0, 0};
#endif vax


/* Defines for crashdump support */
#define phys(number)	((number) & 0x7fffffff)
#define DBSIZE 		16
#define XDUMP		0x8000

/* Macro to verify if a (PAGE TABLE) is valid */
#define VALID_PTABLE(kmx) 					\
	((((kmx) >= 0) && ((kmx) < usrptsize)) && 		\
	    (Usrptmap[(kmx)].pg_v) && 				\
	        (Usrptmap[(kmx)].pg_pfnum > 0) && 		\
		     (Usrptmap[(kmx)].pg_pfnum < physmem))

/* Macro to verify if a (ZERO FOD PTE) is valid */
#define VALID_FODPTE(pte) 					\
	(((pte)->pg_fod == 0) && ((pte)->pg_pfnum > 0) &&	\
	    	((pte)->pg_pfnum < physmem))

/* Macro to verify if a (PTE) is valid */
#define VALID_PTE(pte) 						\
	(((pte)->pg_v) && ((pte)->pg_pfnum > 0) &&		\
	    	((pte)->pg_pfnum < physmem))

/* Macro to verify if a (SHM POINTER) is valid */
#define VALID_SHMP(sp) 						\
	(((sp) >= &smem[0]) && ((sp) < &smem[sminfo.smmni]))

/* Macro to verify if a (TEXT POINTER) is valid */
#define VALID_TEXTP(tp) 					\
	(((tp) >= text) && ((tp) < textNTEXT))

/*************************************************************
 *
 * dumpsys()
 *
 * System dump routine to setup and perform a system dump after
 * the system crashed.
 *
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 *
 ************************************************************/
dumpsys()
{
#ifdef mips
	extern int roottype;		/* root type */
	extern char *bootdev;		/* boot device */
#endif mips
#ifdef vax
	unsigned int *cache_reg;
#endif vax
	int status;
	int maxsize;

#ifdef mips
	/*
	 * Check whether dumpdev exists.
	 */
	if(dumpdev == -1)
		return(0);

	/*
	 * Check for recursize dump entry. Then set dumpsize so 
	 * it will be set for network and disk dump.
	 */
	if(dumpsize)
		return(0);

	dumpsize = btodb(ptob(physmem));	/* # disk blocks */

	/* 
	 * Save away the TLB information needed to analyze MIPS dumps.
	 */
	save_tlb(tlb_dump);

	/*
	 * Reset the SII SCSI controllers before taking a crash
	 * dump. Needed for the dumb PROM drivers on MIPS to work.
	 */
	switch(cpu) {
#if defined(DS3100)
	case DS_3100:
		if(dumpdebug)
			cprintf("resetting all the SII SCSI controllers\n");
		siireset();
		break;
#endif
#if defined(DS5100)
	case DS_5100:
		if(dumpdebug)
			cprintf("resetting all the SII SCSI controllers\n");
		siireset();
		break;
#endif
	default:
		break;
	}

	/*
	 * If console is a graphics device, force printf messages directly
	 * to screen. This is needed here for the case when we manually start
	 * the dump routine from console mode.
	 */
	printstate |= PANICPRINT;

	/*
	 * Check if a network boot was performed.
	 */
/*	if((strstr(bootdev,"mop") || dumpnetboot) && (roottype==GT_NFS)) { */
	if(((!strncmp(bootdev,"mop",3) || dumpnetboot) ||
	   (!strncmp(&bootdev[2],"mop",3) || dumpnetboot)) && 
	    (roottype==GT_NFS)) {
		if(netdump(REQ_DUMP_SERVICE, 0, RCV_BUF_SZ))
			cprintf("Network dump failed\n");
		else
			cprintf("Network dump succeeded\n");
		return(0);
	}
#endif mips
#ifdef vax
	/* 
         *  The dumpsys routine uses VMB drivers to dump memory
         *  to disk.  VMB assumes caches are turned off when its 
         *  drivers are called. If caches are on then loop timings
	 *  will be to fast and the dump might fail.  
	 *  
	 *  The following code turns off cache for all VAX's that
	 *  have caches...
	 *
	 * NOTE: VMB does not require caches be turned off for
	 *	 KA8200, KA650 and KA420 processors.
	 *
	 * NOTE: VMB requires a reset of the SCSI and ST506 I/O
	 *	 controllers prior to calling the boot driver
	 *	 on the KA420 (VS3100) processor.
	 */
	switch(cpu) {
#if defined(VAX420) 
	case C_VAXSTAR:
		szreset();	/* reset SCSI controllers (if any) */
		sdreset();	/* reset ST506 controllers (if any) */
		break;
#endif
#if defined(VAX8800) 
	case VAX_8800:
	case VAX_8820:
		mtpr(CCR,0);
		break;
#endif
#if defined(VAX8600)
	case VAX_8600:
		mtpr(CSWP,0);
		break;		
#endif
#if defined(VAX750)
	case VAX_750:
		mtpr(CADR,1);
		break;
#endif
#if defined(VAX6200) 
	case VAX_6200:
		/* disable secondary cache */
		cache_reg = (unsigned int *)CSRV6200;
		*cache_reg = ((*cache_reg) & 0x1ffff) | 0x40000;
		
		/* disable primary cache */
		mtpr(CADR,(CVAX_SEN1 | CVAX_SEN2));
		break;
#endif
#if defined(VAX6400) 
	case VAX_6400:
		ka6400_disable_cache();	/* Disable primary & backup caches */
		break;
#endif
#if defined(VAX780)
	case VAX_780:
		mtpr (SBIMT,0x218000);
		break;
#endif
	default:
		break;

	}

	/*
	 * Check whether dumpdev exists.
	 */
	if(dumpdev == -1)
		goto fini;

	dumpsize = btodb(ptob(physmem));	/* # disk blocks */

	/*
	 * Check to see if a network boot was performed.
	 */
	if(vmbinfo) {
		vmbinfop = (struct vmb_info *)(phys((int)vmbinfo));
		rpbp	 = (struct rpb *)(vmbinfop->rpbbas);
		/*
		 * If a network boot was performed...
		 */
		if((rpbp->devtyp == BTD$K_NET_DLL) ||
		    (rpbp->devtyp == BTD$K_QNA)     ||
		    (rpbp->devtyp == BTD$K_KA640_NI)) {
			if(netdump(REQ_DUMP_SERVICE, 0, RCV_BUF_SZ))
				cprintf("Network dump failed\n");
			else
				cprintf("Network dump succeeded\n");
			goto fini;
		}
	}
#endif vax

	/* The part_tbl has the dump partition info set in 
	 * init_main() on the way up. This will insure the
	 * IPL stays high and the data is good.
	 */
	parttbl = &part_tbl;	
	if (parttbl->pt_part[minor(dumpdev)&07].pi_nblocks <= 0) {
		cprintf("dump area improper\n");
		return(-1);
	}

	/* Set blkoffs to the beginning of the dump device in blocks. */
	dumpinfo.blkoffs = parttbl->pt_part[minor(dumpdev)&07].pi_blkoff;

	/* Get the size of the dump device in blocks */
	maxsize = parttbl->pt_part[minor(dumpdev)&07].pi_nblocks - dumplo;
	if(maxsize < dumpsize)
		dumpsize = maxsize;

	/*
	 * Check if the dump offset is valid.
	 */
	if(dumplo < 0) {
		cprintf("dump offset improper\n");
		return(-1);
	}

	/*
	 * Check if we need to do a full dump or a partial
	 * selective dump.
	 */
	if(fulldumps_on && (maxsize >= btodb(ptob(physmem)))) {
		dumpmag = full_dumpmag;
		/* Setup dumpinfo structure for a full dump */
		dumpinfo.size_to_dump = dumpsize = btodb(ptob(physmem));
		dumpinfo.blkoffs += dumplo;
		dumpinfo.partial_dump = 0;
		dumpinfo.pagesdumped = 0;
		dumpinfo.totaldumppages = physmem;
		dumpinfo.totaldumpblks = btodb(ptob(physmem));
	}
	else {
		dumpmag = partial_dumpmag;   
		selectdumppages();
	}
	
	cprintf("\ndumping to dev 0x%x, offset %d\n", dumpdev, dumplo);
	if(dumpmag == full_dumpmag)
		cprintf("full dump of %d pages ", dumpinfo.totaldumppages);
	else
		cprintf("partial dump of %d pages ", dumpinfo.totaldumppages);

#ifdef mips
	status = gendump(dumpdev, dumpinfo);

	/*
	 * A return code of -1 means a fatal error
	 * occurred in the generic dump driver.
	 */
	if(status == -1)
		cprintf("failed\nentire dump aborted\n");
	else
		cprintf("%s\n", status ? "succeeded" : "failed");
#endif mips
#ifdef vax
	/*
	 * If there is a possibility that the dump and boot device
	 * are on the same controller then call the generic dump
	 * driver. 
	 */
	status = 0;
	if(major(dumpdev) == major(rootdev)) {
		status = gendump(dumpdev, dumpinfo);

		/*
		 * A return code of -1 means a fatal error
		 * occurred in the generic dump driver.
		 */
		if(status == -1)
			cprintf("failed\nentire dump aborted\n");
		else if(status == 1)
			cprintf("succeeded\n");

		/*
		 * If the generic dump driver returned a 0,
		 * then it could not determine if the dump
		 * and boot devices were on the same controller.
		 */
	}
	if(status == 0) {
		if(bdevsw[major(dumpdev)].d_dump == nodev)
			cprintf("\nDump routine does not exist\n");
		else	
			switch((*bdevsw[major(dumpdev)].d_dump)
				(dumpdev, dumpinfo)) {

			case ENXIO:
				cprintf("\ndump device bad\n");
				break;

			case EFAULT:
				cprintf("\ndump device not ready\n");
				break;

			case EINVAL:
				cprintf("\ndump area improper\n");
				break;

			case EIO:
				cprintf("\ndump i/o error\n");
				break;

			default:
				cprintf("succeeded\n");
				break;
		} 
	}  
fini:
	if(dumpdebug)
		return(0);
#if defined (MVAX) || defined (VAX420)
#define CPMBX	 0x200B801c	/* MVAXII physical addr of console prm com */
#define VS_CPMBX 0x200B0038	/* VAXstar physical addr of console prm com */
	/*
	 * Clear restart in progress flag in cpmbx register and tell it
	 * to reboot.
	 */
	if(cpu == MVAX_II || cpu == VAXSTAR || cpu == C_VAXSTAR) {
		if(cpu_subtype == ST_MVAXII)
			*(u_short *)CPMBX = RB_REBOOT;
		if(cpu_subtype == ST_VAXSTAR)
			*(u_long *)VS_CPMBX = RB_VS_REBOOT;
	}
#endif MVAX || VAX420

#if defined(VAX3600) || defined(VAX60)
#define VAX3600_CPMBX 0x20140400  /* VAX3600 physical addr of console prm com */
	/*
	 * Clear restart in progress flag in cpmbx register and tell it
	 * to reboot.
	 */
	if(cpu == VAX_3600 || cpu == VAX_3400 ||
		cpu == VAX_3900 || cpu == VAX_60) {
		*(u_char *)VAX3600_CPMBX = RB_CV_REBOOT;
	}
#endif VAX3600 || VAX60
#endif vax
	return(0);
}

/*************************************************************
 *
 * netdump()	
 *
 * Network dump driver to perform a system dump. It uses the 
 * VMB/PROM drivers and MOP to dump out physical memory to 
 * the dump device.
 *
 ************************************************************/
netdump(prog, addr, bufsz)
int prog, addr, bufsz;
{
#ifdef vax
	union mop_packets *mop_i = (union mop_packets *)(char *)vmbinfo-2048;
	union mop_packets *mop_o = (union mop_packets *)(char *)vmbinfo-4096;
#endif vax
#ifdef mips
	union mop_packets *mop_i = &mop_input;
	union mop_packets *mop_o = &mop_output;
	char	*cp, dump_name[80];
	extern  int console_magic;
#endif mips
	short num_bytes;
	char *start;
	int wrt_cnt, wrt_retry=5, rd_retry=5;
	int tmp, i, j, length, next, count, descindex;
	int prev_dump_addr = -1, curr_dump_addr = -1;

	/* 
	 * Initialize the network dump driver.
	 */
	bufsz += 6;
	mop_i->MOPCODE = REQ_DUMP_SERVICE;
#ifdef mips
	if (!rex_base) {
		if ((dump_io_chan = prom_open(netdevice, 2)) < 0) {
#endif mips
#ifdef vax
	if (!(qioinit() & 1)) {
#endif vax
		cprintf("\nNetwork dump driver open failed\n");
		return(-1);
	}
#ifdef mips
        }
	else {
		cp = dump_name;
		cp[0] = 'b';
		cp[1] = 'o';
		cp[2] = 'o';
		cp[3] = 't';
		cp[4] = ' ';
		cp[5] = netdevice[0];
		cp[6] = '/';
		cp[7] = 'm';
		cp[8] = 'o';
		cp[9] = 'p';
/*		strncpy(&cp[5],bootdev,5); */
		cp[10] = '/';
		cp[11] = 'n';
		cp[12] = 'u';
		cp[13] = 'l';
		cp[14] = 'l';
		cp[15] = ' ';
		cp[16] = '-';
		cp[17] = 'N';
		cp[18] = 'o';
		cp[19] = 'b';
		cp[20] = 'o';
		cp[21] = 'o';
		cp[22] = 't';
		cp[23] = '\0';
		*(int *)(rex_base + 0x54) = 0;
		rex_execute_cmd(cp); 
		bcopy(mopdl_multicast,mop_destination,6);
		if(*(int *)(rex_base + 0x54) == 0) {
		 	cprintf("\nNetwork dump initialization failed\n");
			return(-1);
	  	}
		if(rex_bootinit() < 0) {
		 	cprintf("\nNetwork dump driver open failed\n");
			return(-1);
	  	}
	}
#endif mips

	/*
 	 * Check the size of dumptype to determine the type
	 * of dump we need to do.
	 */
	if(dumptype >= btodb(ptob(physmem)))
		dumptype = -1;
	if(dumptype == -1 && !fulldumps_on)
		dumptype = btodb(ptob(physmem));
	if(dumptype < -1)
		dumptype = 0;
	/*
	 * Switch on type of dump to be performed.
	 * dumptype =  0 --> No dump.
	 * dumptype = -1 --> Full dump.
	 * dumptype = +# --> Partial selective dump.
	 */
	switch(dumptype) {
	case NET_NO_DUMP:	/* No dump */
#ifdef mips
		if (!rex_base)
			prom_close(dump_io_chan);
#endif mips
		return(0);
		break;

	case NET_FULL_DUMP:	/* Full dump */
		dumpmag = full_dumpmag;
		/* Setup dumpinfo structure for a full dump */
		dumpinfo.size_to_dump = dumpsize = btodb(ptob(physmem));
		dumpinfo.blkoffs += dumplo;
		dumpinfo.partial_dump = 0;
		dumpinfo.pagesdumped = 0;
		dumpinfo.totaldumppages = physmem;
		dumpinfo.totaldumpblks = btodb(ptob(physmem));
		break;

	default:		/* Partial selective dump */
		dumpmag = partial_dumpmag;
		dumpsize = dumptype;
		selectdumppages();
		break;
	}
	if(dumpmag == full_dumpmag)
		cprintf("\nNetwork full dump of %d pages\n", 
					dumpinfo.totaldumppages);
	else
		cprintf("\nNetwork partial dump of %d pages\n", 
					dumpinfo.totaldumppages);
	next = WRITE_DUMPDESC;
	start = (char *)0x80000000;
	length = dumpinfo.size_to_dump * DEV_BSIZE;
	count = 0;
	if(dumpdebug && dumpinfo.partial_dump) {
		cprintf("writing out kernel pages\n");
		cprintf("kernelblks = %d totaldumpblks = %d\n",
			dumpinfo.size_to_dump,dumpinfo.totaldumpblks);
	}
	for(;;) {
		/*
		 * First we initiate a dump request by REQ_DUMP_SERVICE.
		 * The packet contains the number of bytes we want to dump.
		 * The server responds by sending a VOLASS_CODE.
		 * The next request from the server will be a REQ_MEMORY_DUMP.
		 * When we receive the packet, we should fill the data buffer
		 * with the contents of memory the server asks for. But we
		 * need to fake it out during a partial crash dump.
		 * The final packet sent by the server will be DUMP_COMPLETE.
		 */
		switch (mop_i->MOPCODE) {
		case REQ_MEMORY_DUMP:	/* Server wants another packet */
			/*
			 * Check for a duplicate dump packet.
			 */
			prev_dump_addr = curr_dump_addr;
			bcopy(mop_i->req_mem_dump.memaddr,
				(char *)&curr_dump_addr,
					sizeof(mop_i->req_mem_dump.memaddr));
			if(curr_dump_addr == prev_dump_addr)
				goto do_write_net;

			mop_o->MOPCODE = MEMORY_DUMP_DATA;
			/*
			 * start = starting memory location for the MOP request.
			 */
			bcopy(mop_i->req_mem_dump.memaddr,
				mop_o->dump_data.memaddr,
				sizeof(mop_i->req_mem_dump.memaddr));
			/*
			 * num_bytes = Size of the transfer requsted by MOP.
			 */
			bcopy(mop_i->req_mem_dump.count, (char *)&num_bytes,
				sizeof(mop_i->req_mem_dump.count));
			/*
			 * This for loop will copy the contents of memory
		 	 * locations into the transfer buffer for MOP. 
			 */
			for(i=0; i<num_bytes; i++) {
				if((count == length) && dumpinfo.partial_dump) {
				    switch(next) {
				    /* Write out dump descriptor block */
				    case WRITE_DUMPDESC:
				    	if(!getdumppages())
					    break;
				        if(dumpdebug) {
					    cprintf("writing out dump descr");
					    cprintf("iptor block\nwriting ");
					    cprintf("out selected pages\n");
					}
				    	start = (char *)&dumpdesc[0];
				    	length = DEV_BSIZE;
					count = 0;
					descindex = 0;
					next = WRITE_DUMPPAGES;
					break;

				    /* Write out selected dump pages */
				    case WRITE_DUMPPAGES:
					if((dumpdesc[descindex] == 0) ||
					    (dumpdesc[descindex] == dumpmag))
					    break;
					start = (char *) 
					    ((dumpdesc[descindex] * NBPG)
								| 0x80000000);
					length = (NBPG * CLSIZE);
					count = 0;
					if(++descindex == NPFNDESC)
					    next = WRITE_DUMPDESC;
					else
					    next = WRITE_DUMPPAGES;
					break;

				    default:
					break;
				    }
				}
				if(count == length) {
				    /*
	 			     * Write a final dump descriptor of magic 
				     * numbers to signify a successful dump.
	 			     */
				    for(j=0; j<NPFNDESC; j++)
					dumpdesc[j] = dumpmag;
				    start = (char *)&dumpdesc[0];
				    length = DEV_BSIZE;
				    count = 0;
				    descindex = 0;
				}
				mop_o->dump_data.data[i] = *start++;
				count++;
			}
			wrt_cnt = sizeof(mop_o->dump_data);
			break;

		case VOLASS_CODE:	/* Server responded from dmp serv req */
			if(dumpdebug)
				cprintf("VOLASS_CLASS\n");
			break;

		case DUMP_COMPLETE:	/* All done */
			if(dumpdebug)
				cprintf("DUMP_COMPLETE\n");
#ifdef mips
			if (!rex_base)
				prom_close(dump_io_chan);
#endif mips
			return(0);
			break;

		case REQ_DUMP_SERVICE:	/* I need help dumping !!! */
			if(dumpdebug)
				cprintf("REQ_DUMP_SERVICE bytes = %d\n",
					dumpinfo.totaldumpblks*DEV_BSIZE);
			mop_o->MOPCODE = REQ_DUMP_SERVICE;
			mop_o->req_dump_srvc.devtype = NET_QNA;
			mop_o->req_dump_srvc.mopver = MOP_VERSION;
			tmp = (dumpinfo.totaldumpblks * DEV_BSIZE) + DEV_BSIZE;
			bcopy((char *)&tmp, mop_o->req_dump_srvc.memsiz,
				sizeof(mop_o->req_dump_srvc.memsiz));
			mop_o->req_dump_srvc.bits = 2;
			tmp = XTRA_BUFSZ;
			bcopy((char *)&tmp, mop_o->req_dump_srvc.rbufsz_param,
				sizeof(mop_o->req_dump_srvc.rbufsz_param));
			mop_o->req_dump_srvc.sz_field = 2;
			bcopy((char *)&bufsz, mop_o->req_dump_srvc.rcvbufsz,
				sizeof(mop_o->req_dump_srvc.rcvbufsz));
			wrt_cnt = sizeof(mop_o->req_dump_srvc);
			break;

		default:
			if(dumpdebug)
				cprintf("BAD_MOPCODE = %d\n", mop_i->MOPCODE);
			break;
		}
do_write_net:
		while (wrt_retry--) {
			if (write_net(dump_io_chan, wrt_cnt,
			    		&mop_o->req_dump_srvc.code))
				break;
		}
		if (wrt_retry <= 0 )		/* if we ran out of retries */
			goto error;
		wrt_retry=5;

do_read_net:
		while (rd_retry--) {
			if (read_net(dump_io_chan, bufsz,
			    		&mop_i->req_mem_dump.code))
				break;
		}
		if (rd_retry <= 0 )		/* if we ran out of retries */
			goto error;
		rd_retry=5;
	}
error:
#ifdef mips
	if (!rex_base)
		prom_close(dump_io_chan);
#endif mips
	return(-1);
}

/*************************************************************
 *
 * write_net()	
 *
 * Writes using the VMB/PROM driver to support network dumps.
 *
 ************************************************************/
write_net(io, size, addr)
int io, size, *addr;
{
	int status;

#ifdef mips
	if (rex_base) {
		unsigned char buffer[1600];

		/*
		 * We are being called with only the data portion of
		 * the mop packet.  We must stick the ethernet header
		 * on and set the destination, the protocol and the
		 * length field.
		 */
		bcopy(addr,(void *)&buffer[16],size);
		bcopy(mop_destination,(void *)buffer,6);
		buffer[12]= 0x60;
		buffer[13]= 0x01;
		buffer[14]= size &0xff;
		buffer[15]= (size>>8) && 0xff;
		status = rex_bootwrite(0,buffer,size+16);
	}
	else
		status = prom_write(io, addr, size);
	return((status < 0) ? 0 : 1);
#endif mips
#ifdef vax
	status = qio(PHYSMODE,IO$_WRITELBLK,&param_buf,size,addr);
	return(status & 1);
#endif vax
}

/*************************************************************
 *
 * read_net()	
 *
 * Reads using the VMB/PROM driver to support network dumps.
 *
 ************************************************************/
read_net(io, size, addr)
int io, size, *addr;
{
	int status;

#ifdef mips

	if (rex_base) {
		while(1) {
			u_char buffer[1600];
			int len;

			while ((status = rex_bootread(0,buffer,1600)) == 0); 
			if (status <= 0)
				return(status);
			/*
			 * If we got a message, check that it is not broadcast,
			 * that it is a mop message, and if we have "bound",
			 * that is from right source.  If it's ok, copy it to
			 * the callers buffer and return the proper length.
			 */
			if ((buffer[12]==0x60 && buffer[13]==0x1) &&
			!eaddr_match((char*)buffer,broadcast)) {
				len = buffer[14] | (buffer[15]<<8);
				if (mop_destination[0] & 1){
					bcopy((void *)&buffer[6],mop_destination,6);
					bcopy((void *)&buffer[16],addr,len);
					return(len);
				}
		  		else {
					if (eaddr_match(mop_destination,(char*)&buffer[6])){
						bcopy((void *)&buffer[16],addr,len);
						return(len);
					}
				} 
			} /* END IF */
		} /* END WHILE */
	}
	else {
		status = prom_read(io, addr, size);
		return((status < 0) ? 0 : 1);
	}
#endif mips
#ifdef vax
	status = qio(PHYSMODE,IO$_READLBLK,&param_buf,size,addr);
	return(status & 1);
#endif vax
}

#ifdef mips
/*
 * eaddr_match - used  by read/write mop routines.
 */

int
eaddr_match(ea1, ea2)
char *ea1, *ea2;
{
	int i;

	for (i=0;i<6;i++)
		if( *ea1++ != *ea2++)
			return(0);
	return(1);
}
#endif mips

/*************************************************************
 *
 * gendump()	
 *
 * Generic dump driver to perform a system dump. It uses the 
 * VMB/PROM drivers to dump out physical memory to the dump 
 * device.
 *
 ************************************************************/
gendump(dev, dumpinfo)
dev_t dev;			/* dump device */
struct dumpinfo dumpinfo; 	/* dump info */
{
#ifdef mips
	struct 	uba_ctlr *um;
	char	*cp, dump_name[80];
	int	i, h, float_num, slot;
	extern  int console_magic;
	extern  struct tc_slot tc_slot[8];
#endif mips
#ifdef vax
	int 	unit_ultrix;		/* unit plug num of dump device */
	int 	unit_physical;		/* physical unit num of dump device */
	int 	cntlr_physical;		/* physical cntlr num of dump device */
	long 	longp;
	struct 	genericconf *gb;
	struct 	uba_driver *uba_drive;
	struct 	uba_hd	  *uba_head;
	struct 	uba_device *uba_dev;
	struct 	mba_driver *mba_drive;
	struct 	mba_hd	  *mba_head;
	struct 	mba_device *mba_dev;
#endif vax
	int 	status;
	char 	*start;
	int 	index;

#ifdef mips
	um = ubminit;
	/* 
	 * Check dump DEVIOCGET status from init_main().
	 */
	if (dumpdeviocgetstatus) {
		cprintf("\nDEVIOGET failed\n");
		return(0);
	} else {

		cp = dump_name;
		i = 0;

		/* determine what type of disk it is 
		 * At this time the boot device always starts with two
 		 * chars of unit name.  If the number of characters ever changes
		 * this will break the following case statement.
		 */
		switch(dumpdg.bus) {
		case DEV_UB:
		case DEV_QB:
		case DEV_BI:
		case DEV_BICI:
			/* The dump device is a 'ra' device. */
			cp[i++]='r';
			cp[i++]='a';
			break;
		case DEV_MSI:
			/* The dump device is a 'rf' device. */
			cp[i++]='r';
			cp[i++]='f';
			break;
		case DEV_SCSI:
			if (!rex_base) {
			/* The dump device is a 'rz' device. */
				cp[i++]='r';
				cp[i++]='z';
			}
			else {
				cp[i++]='b';
				cp[i++]='o';
				cp[i++]='o';
				cp[i++]='t';
				cp[i++]=' ';
			}
			break;
		default:
			cprintf("can not open dump device\n");
			return (0);
		}

		if (!rex_base) {
			cp[i++]='(';
		}

		/* Fill in the dump ctlr part of the string.
		 * We need to process one char at a time
		 * and the number may be bigger that one
		 * digit.  
		 */


		switch(dumpdg.bus) {
			case DEV_BI:
				cp[i++] = '/';
				cp[i++] = 'X';
				i = i + itoa(dumpdg.adpt_num,&cp[i]);
				cp[i++] = '/';
				cp[i++] = 'B';
				i = i + itoa(dumpdg.nexus_num,&cp[i]);
				break;
			case DEV_BICI:
				cp[i++] = '/';
				cp[i++] = 'X';
				i = i + itoa(dumpdg.adpt_num,&cp[i]);
				cp[i++] = '/';
				cp[i++] = 'B';
				i = i + itoa(dumpdg.nexus_num,&cp[i]);
				cp[i++] = '/';
				cp[i++] = 'C';
				i = i + itoa(dumpdg.rctlr_num,&cp[i]);
				break;
			case DEV_QB:
				/* Go get the CSR for this controller number
				 * and convert it into a hardware controller 
				 * number for the open prom call.  The same
				 * algorithm is used here as is used in the
				 * console for the Q-bus.  That is:
				 *	- If its at the fixed address its
				 *	controller number 0.
				 *
				 *	- If its at the first floating address
				 *	its controller number 1
				 *
				 *	- If its at the second floating address
				 *	its controller number 2
				 *
				 *	- And so on, for a total of 16 
				 *	or up to controller 15.  The first
				 *	floating address is at 760334 (octal)
				 * 	and the go up by a value of 4.
				 */

				for (um = ubminit; um->um_driver; um++) {
					/* check the ctlr number from the ioctl against whats
					 * in the ubminit table until we find a match or the um_driver
					 * field goes zero (end of list).  If the driver type is not
					 * correct then we found and match on ctlr number must be wrong.
					 */
					if ((dumpdg.ctlr_num != um->um_ctlr) || 
					    (um->um_driver != (struct uba_driver *) uqdriver)){
						continue;
					}

					/* Because the um_addr is a virtual address we can match on only
					 * the low bits of the Q-bus address that we are looking for.
					 */
					if(((int)um->um_addr & 0x1fff) == (0172150 & 0x1fff)){
					       i = i + itoa(0,&cp[i]);
					       break;
					} else {
						for( float_num = 0160334, h = 1;h < 16; h++)
						{
						       /* Find a match */
						       if ((float_num & 0x1fff) == ((int)um->um_addr & 0x1fff)){
							       i = i + itoa(h,&cp[i]);
							       break;
						       }
						       float_num += 4;
						}
						/* If they don't match now its a miss so
						 * don't do a dump.  Print out and return.
						 */
						if ((float_num & 0x1fff) != ((int)um->um_addr & 0x1fff)) {
							cprintf("Can't open dump device\n");
							return (0);

						}
				       }

				}
 				break;

			case DEV_MSI:
				/* RESTRICTION that the unit number and the
				 * controller number must match.
				 */
				i = i + itoa(dumpdg.slave_num,&cp[i]);
 				break;
			case DEV_SCSI:
				{
				int scsi_unit, scsi_cntlr;
				struct uba_device *ui;

				  scsi_unit = minor(dumpdev) >> 3;
                		  ui = szdinfo[scsi_unit];

#ifdef SCSI_DUMP_DEBUG
			 cprintf("crashdump: dumpdev=%x, scsi_unit = %x\n",
					 dumpdev, scsi_unit);
#endif SCSI_DUMP_DEBUG

                		  if ((ui == 0) || (ui->ui_alive == 0)) {
				   cprintf("Can't Open SCSI Dump Device 0x%x\n",
				        dumpdev);
				    return (0);
                    		  }
                		  else {
#ifdef SCSI_DUMP_DEBUG
			 cprintf("crashdump: scsi cntlr = %d\n",
					      ui->ui_ctlr);
#endif SCSI_DUMP_DEBUG
                		     scsi_cntlr = ui->ui_ctlr;
				  }

				  if (!rex_base) {
				    i = i + itoa(scsi_cntlr,&cp[i]);
				  }
				  else {
				    slot = 0;
				    while((tc_slot[slot].unit != scsi_cntlr) ||
					  (strcmp(tc_slot[slot].devname,"asc") != 0)) {
				      slot++;
				    }
				    i = i + itoa(tc_slot[slot].slot,&cp[i]);
				    cp[i++]='/';
				    cp[i++]='r';
				    cp[i++]='z';
				  }
				}
				break;
			default:
				cprintf("Can't Open Dump Device\n");
				return (0);
		}
		/* Now load up the unit number part of the dump device */

		if (!rex_base) {
			cp[i++] = ',';	/* comma seperator */
		}

		/* Now load up the unit number part of the dump device
		 * string.
		 */

		if (!rex_base) {
			i = i + itoa(dumpdg.slave_num,&cp[i]);
			cp[i++] = ',';	/* comma seperator */
		}
		else {
			i = i + itoa(dumpdg.slave_num,&cp[i]);
		}

		/*
		 * Horses in midstream - Third arg to console used to
		 * be partition, now is a block offset for the device.
		 * The console will no longer have knowledge of 'partitions'.
		 * Here we assume that PMAX and MIPSfair will not change
		 * to the new format and that everything else will.
		 */
		if (!rex_base) {
			if ((cpu == DS_3100) || (cpu == DS_5400))
				cp[i++] = 'c';	/* make it the C partition */
			else
				cp[i++] = '0';	/* give it a zero offset */
		  
			cp[i++] = ')';	/* end string here, don't */
			cp[i++] = '\0';	/* need the file name part*/
		}
		else {
			cp[i++] = '/';
			cp[i++] = 'l';         /* pick any old name! */
			cp[i++] = ' ';
			cp[i++] = '-';
			cp[i++] = 'N';
			cp[i++] = 'o';
			cp[i++] = 'b';
			cp[i++] = 'o';
			cp[i++] = 'o';
			cp[i++] = 't';
			cp[i] = '\0';
		}

		if (rex_base) {
			/*
			 * Check here to see if this is the boot disk.
			 * If not boot disk we must do more setup,
			 */

			*(int *)(rex_base + 0x54) = 0;
			rex_execute_cmd(cp);
			if(*(int *)(rex_base + 0x54) == 0) {
		 		cprintf("Console dump initialization failed\n");
				return(-1);
	  		}
			if (rex_bootinit() < 0) {
				cprintf("can't init console boot device\n");
				return(0);
			}
		}
		else {
			/*
			 * Open the 'c' partition so that we have full access
			 * to the device.  We will provide the offset through
			 * prom_lseek.
			 * Printing the following will only scare people
			 * when they see the 'c' partition. It is for debug.
			 */
			if(dumpdebug)
				cprintf("dump device name is (%s)\n",dump_name);
			if ((dump_io_chan = prom_open(dump_name, 2)) < 0) {
				cprintf("can't open dump device\n");
		 		return (0);
		 	}
		}
	}
	if (!rex_base) {
		if (prom_lseek(dump_io_chan,dumpinfo.blkoffs*DEV_BSIZE,0) < 0) {
			cprintf("can't lseek to dump device\n");
			prom_close(dump_io_chan);
			return(0);
		}
	}
#endif mips
#ifdef vax
	if(!vmbinfo) {
		cprintf("no vmbinfo structure exists for gendump\n");
		cprintf("calling drivers dump routine ");
		return(0);
	}

	/*
	 * Get the address of the RPB (in high memory, not low).
	 */
	vmbinfop = (struct vmb_info *)(phys((int)vmbinfo));
	rpbp     = (struct rpb *)(vmbinfop->rpbbas);

	/*
	 * Search the genericconf table for an entry that matches the
	 * RPB device type and device major number of the dump device.
         *
         * MSCP (ra) disks could be in a range of major numbers.  For this
         * reason it is not sufficient to just examine the major number.
	 */
	status = 0;
	for(gb = genericconf; gb->gc_driver; gb++)
	    if(gb->gc_BTD_type == rpbp->devtyp && 
		((major(dev) == major(gb->gc_root)) ||
		 (MSCP_B_DEV(dev) && MSCP_B_DEV(gb->gc_root)))) {
		    status = 1;
		    break;
	    }
	if(!status) {
		cprintf("dump device type not found in genconf table\n");
		cprintf("calling drivers dump routine ");
		return(0);
	}

	/*
	 * Set unit equal to unit plug number of dump device.
	 */
	unit_ultrix = minor(dev) >> 3;
	unit_physical = -1;
	cntlr_physical = -1;
	/*
	 * MSCP (ra) type disks can occupy a range of major numbers. Also
	 * look at the major number in unit number calculations.
	 */
        if (MSCP_B_DEV(dev)) {
                unit_ultrix += MSCP_B_UNIT_OFFSET(dev);
        }

	switch(rpbp->devtyp) {

	/*
	 * Traverse unibus and massbus structures to determine physical
	 * unit number dump device is on.
	 */
	case BTD$K_MB:
#if (NMBA > 0)
	    for(mba_dev = (struct mba_device *)mbdinit;
		 mba_dev->mi_driver;
		 mba_dev++) {
		if(mba_dev->mi_alive) {
		    mba_drive = mba_dev->mi_driver;
		    if(*mba_drive->md_dname == *gb->gc_name) {
			if(mba_dev->mi_unit == unit_ultrix) {
			    mba_head = mba_dev->mi_hd;
			    longp = (long)mba_head->mh_physmba;
			    if(longp == rpbp->adpphy) {
				unit_physical=(long)mba_dev->mi_drive;
				break;
			    }
			}
		    }
		}
	    }
#endif
	    break;

	case BTD$K_BDA:
	    for(uba_dev = (struct uba_device *)ubdinit;
		 uba_dev->ui_driver;
		 uba_dev++) {
	      if(uba_dev->ui_alive) {
		    longp = (long)uba_dev->ui_physaddr;
		    longp &= 0xffffff00;
		    if(cpu == VAX_6400 && cpu_sub_subtype == MARIAH_VARIANT) {
		      if(longp == (rpbp->csrphy & 0x3fffffff)) {
			if(strcmp(uba_dev->ui_devname, gb->gc_name) == 0) {
			  if(uba_dev->ui_unit == unit_ultrix) {
			    unit_physical =(long)uba_dev->ui_slave;
			    break;
			  }
			}
		      }
		    }
		    else if(longp == rpbp->csrphy) {
		      if(strcmp(uba_dev->ui_devname, gb->gc_name) == 0) {
			if(uba_dev->ui_unit == unit_ultrix) {
			  unit_physical =(long)uba_dev->ui_slave;
			  break;
			}
		      }
		    }
		  }
	    }
	    break;

	case BTD$K_KDM70:
	    for(uba_dev = (struct uba_device *)ubdinit;
		 uba_dev->ui_driver;
		 uba_dev++) {
		if(uba_dev->ui_alive) {
		    if(strcmp(uba_dev->ui_devname, gb->gc_name) == 0) {
			if(uba_dev->ui_unit == unit_ultrix) {
			     unit_physical =(long)uba_dev->ui_slave;
			     break;
			}
		    }
		}
	    }
	    break;

	case BTD$K_BVPSSP:
	    for(uba_dev = (struct uba_device *)ubdinit;
		 uba_dev->ui_driver;
		 uba_dev++) {
		if(uba_dev->ui_alive) {
	  	  if(cpu == VAX_6400 && cpu_sub_subtype == MARIAH_VARIANT) {
		    if((long)uba_dev->ui_physaddr == (rpbp->csrphy & 0x3fffffff)) {
		      if(strcmp(uba_dev->ui_devname, gb->gc_name) == 0) {
			if(uba_dev->ui_unit == unit_ultrix) {
			  unit_physical =(long)uba_dev->ui_slave;
			  break;
			}
		      }
		    }
		  }
		  else if((long)uba_dev->ui_physaddr == rpbp->csrphy) {
		    if(strcmp(uba_dev->ui_devname, gb->gc_name) == 0) {
		      if(uba_dev->ui_unit == unit_ultrix) {
			unit_physical =(long)uba_dev->ui_slave;
			break;
		      }
		    }
		  }
		}
	    }
	    break;

        case BTD$K_SII:
	case BTD$K_HSCCI: 
	  /*
	   * The SII chip is used for both DSSI disks and some SCSI disks.
	   */
	  if(cpu == VAX_60) {
	    for(uba_dev = (struct uba_device *)ubdinit;
		 uba_dev->ui_driver;
		 uba_dev++) {
		if(uba_dev->ui_alive) {
		    if(strcmp(uba_dev->ui_devname, gb->gc_name) == 0) {
			if(uba_dev->ui_unit == unit_ultrix) {
			    /*
			     * The ui_slave number is multiplied by 100 because
			     * the unit number passed to VMB for a SCSI disk
			     * expects the format to be: txx - where "t" is
			     * the SCSI target ID, and "xx" is the Logical
			     * Unit Number (LUN).  Therefore, if we assume the 
			     * LUN is always zero, for rz1, we must pass unit
			     * 100 to VMB.  If the LUN is ever nonzero, this
			     * approach breaks.
			     */
			    unit_physical =(long)(uba_dev->ui_slave * 100);
			    break;
			}
		    }
		}
	    }
	  }
	  else {
	    for(uba_dev = (struct uba_device *)ubdinit;
		 uba_dev->ui_driver;
		 uba_dev++) {
		if(uba_dev->ui_alive) {
		    if((long)uba_dev->ui_rctlr == (rpbp->bootr2 & 0xff)) {

			if(strcmp(uba_dev->ui_devname, gb->gc_name) == 0) {
			    if(uba_dev->ui_unit == unit_ultrix) {
				unit_physical =(long)uba_dev->ui_slave;
				break;
			    }
			}
		    }
		}
	    }
	  }
	  break;

	case BTD$K_DQ:
	    for(uba_dev = (struct uba_device *)ubdinit;
		uba_dev->ui_driver;
		uba_dev++) {
		if(uba_dev->ui_alive) {
		    if(strcmp(uba_dev->ui_devname, gb->gc_name) == 0) {
			if(uba_dev->ui_unit == unit_ultrix) {
			    uba_head = uba_dev->ui_hd;
			    longp = (long)uba_head->uh_physuba;
			    if(longp == rpbp->adpphy) {
				unit_physical = (long)uba_dev->ui_slave;
				break;
			    }
			}
		    }
		}
	    }
	    break;

	case BTD$K_KA420_DISK:
	    /*
	     * VS3100 SCSI/ST506 or SCSI/SCSI controller.
	     * There can be two SCSI busses, so the dump device can be
	     * on a different controller from the boot device. In that
	     * case we must tell the VMB boot driver init routine
	     * which controller the dump device is on.
	     * Controller addresses: A = 0x200c0080, B = 0x200c0180.
	     * RPB cntlr (rpbp->ctrllr): A = 1, B = 2.
	     */
	    if(cpu == C_VAXSTAR) {
		for(uba_dev = (struct uba_device *)ubdinit;
		     uba_dev->ui_driver;
		     uba_dev++) {

		    if(uba_dev->ui_alive == 0)
			continue;
		    if(uba_dev->ui_driver != &scsidriver)
			continue;

		    /* allow either SCSI cntlr A or B */
		    if(((long)uba_dev->ui_physaddr & ~0x100) ==
			(rpbp->csrphy & ~0x100)) {
			if(strcmp(uba_dev->ui_devname, gb->gc_name) == 0) {
			    if(uba_dev->ui_unit == unit_ultrix) {
				unit_physical = (long)uba_dev->ui_slave;
				unit_physical *= 100;
				cntlr_physical = ((unit_ultrix >> 3) & 1) + 1;
				break;
			    }
			}
		    }
		}
	    }
	    break;

	default:
	    for(uba_dev = (struct uba_device *)ubdinit;
		 uba_dev->ui_driver;
		 uba_dev++) {
		if(uba_dev->ui_alive) {
		  if(cpu == VAX_6400 && cpu_sub_subtype == MARIAH_VARIANT) {
		    if((long)uba_dev->ui_physaddr == (rpbp->csrphy & 0x3fffffff)) {
		      if(strcmp(uba_dev->ui_devname, gb->gc_name) == 0) {
			if(uba_dev->ui_unit == unit_ultrix) {
			  uba_head = uba_dev->ui_hd;
			  longp = (long)uba_head->uh_physuba;
			  if((longp == rpbp->adpphy) ||
			     (cpu_subtype == ST_VAXSTAR) ||
			     (cpu == MVAX_I)) {
			    unit_physical = (long)uba_dev->ui_slave;
			    break;
			  }
			}
		      }
		    }
		  }
		  else if((long)uba_dev->ui_physaddr == rpbp->csrphy) {
		    if(strcmp(uba_dev->ui_devname, gb->gc_name) == 0) {
		      if(uba_dev->ui_unit == unit_ultrix) {
			uba_head = uba_dev->ui_hd;
			longp = (long)uba_head->uh_physuba;
			if((longp == rpbp->adpphy) ||
			   (cpu_subtype == ST_VAXSTAR) ||
			   (cpu == MVAX_I)) {
			  unit_physical = (long)uba_dev->ui_slave;
			  break;
			}
		      }
		    }
		  }
		}
	    }
	} 

	if(unit_physical == -1) {
		cprintf("gendump can't determine dump devices slave #\n");
		cprintf("calling drivers dump routine ");
		return(0);
	}

	/*
	 * The restart parameter block rpbp points to is used by the qio
	 * calls, qioinit and qio. Set the physical unit number in the rpb
	 * to the dump devices physical unit plug.
	 * If VS3100 SCSI, set cntlr (A = 1, B = 2).
	 * If VS3100 SCSI, set cntlr CSR (A = 0x200c0080, B = 0x200c0180).
	 * NOTE: cntlr_physical wil be -1 if not a VS3100 SCSI.
	 */
	rpbp->unit = unit_physical;
	if(cntlr_physical > 0) {
	    rpbp->ctrllr = cntlr_physical;
	    rpbp->csrphy &= ~0x100;
	    if(cntlr_physical == 2)
		rpbp->csrphy |= 0x100;
	}

	/*
	 * Call the drivers init routine, if one exists.
	 */
	status = qioinit();
	if(!(status & 1)) {
		cprintf("dump device not ready\n");
		return(0);
	}
#endif vax

	/*
	 * If a Full Dump is being performed then this write will
	 * dump out all of physical memory to the dump device. If
	 * a Partial Selective Dump is being performed then this 
	 * write will dump out all the kernel (text/data/bss/valloc) 
	 * pages to the dump device.
	 */
	if(dumpdebug && dumpinfo.partial_dump) {
		cprintf("writing out kernel pages\n");
		cprintf("kernelblks = %d totaldumpblks = %d\n",
			dumpinfo.size_to_dump,dumpinfo.totaldumpblks);
	}
	start = (char *)0x80000000;
	if(!write_disk(start, dumpinfo.size_to_dump))
		return(0);
	/*
	 * If a Partial Selective Dump is being performed then this
	 * loop will dump out all the selected physical memory pages
 	 * to the dump device.
	 */
	while(dumpinfo.partial_dump && getdumppages()) {
		/*
		 * Write out the dump descriptor block which contains 
		 * the starting page frame numbers (PFN's) of the next
		 * set of cluster pages of physical memory to be dumped.
		 */
		if(dumpdebug) {
			cprintf("writing out dump descriptor block\n");
			cprintf("writing out selected pages\n");
		}
		start = (char *)&dumpdesc[0];
		if(!write_disk(start, 1))
			return(0);
		/*
		 * Write out the selected physical memory pages. Write out
		 * the set of cluster pages defined in the dump descriptor
		 * block.
		 */
		for(index=0; index<NPFNDESC; index++) {
			if(dumpdesc[index])
				start = (char *)
				    ((dumpdesc[index] * NBPG) | 0x80000000);
			else
				break;
			if(!write_disk(start, ((NBPG * CLSIZE)/DEV_BSIZE)))
				return(0);
		}
	}
	
	/*
	 * Write a final dump descriptor of magic numbers to 
	 * signify a successful dump.
	 */
	for(index=0; index<NPFNDESC; index++)
		dumpdesc[index] = dumpmag;
	start = (char *)&dumpdesc[0];
	if(!write_disk(start, 1))
		return(0);
#ifdef mips
	if (!rex_base)
		prom_close(dump_io_chan);
#endif mips
	return(1);
}

/*************************************************************
 *
 * write_disk()	
 *
 * Write out physical memory to the dump device. It uses the
 * VMB/PROM drivers to dump out physical memory to the dump 
 * device.
 *
 ************************************************************/
write_disk(start, blocks)
char *start;
int blocks;
{
	int blk, bc, status;

	while(blocks > 0) {
		blk = blocks > DBSIZE ? DBSIZE : blocks;
		bc  = blk * DEV_BSIZE;
#ifdef mips
		if (rex_base)
			status = (rex_bootwrite(dumpinfo.blkoffs,start,bc) == bc) ? 1 : 0; 
		else
			status = (prom_write(dump_io_chan,start,bc)==bc) ? 1 :0; 
#endif mips
#ifdef vax
		status = qio(PHYSMODE, IO$_WRITELBLK, 
					dumpinfo.blkoffs, bc, start);
#endif vax
		if(status & 1) {
			start += bc;
			blocks -= blk;
			dumpinfo.blkoffs += blk;
		}
		else {
			cprintf("dump i/o error: bn = %d, ", dumpinfo.blkoffs);
#ifdef vax
			if(status == SS$_CTLRERR)
				cprintf("fatal controller error\n");
			else
				cprintf("dump driver status = 0x%x\n", status);
#endif vax
#ifdef mips
			if(!rex_base)
				prom_close(dump_io_chan);
#endif mips
			return(0);
		}
	}
	return(1);
}

/*****************************************************************
 *
 * getdumpdata()	
 *
 * Interface routine used for IO DRIVERS that have dump routines. 
 * This routine returns the next physical memory address to dump
 * from and the number of blocks to dump out back to the caller. 
 * It will return (1) as long as there are more pages to dump out
 * and (0) when we are done.
 * 
 ****************************************************************/
getdumpdata(start, blocks)
char *start;
int *blocks;
{

	static char *startaddr = (char *)0x80000000;
	static int nextthing = WRITE_DUMPDESC;
	static int nextindex = 0;
	static int done = 0;

	/*
	 * If a Full Dump is being performed then write out all of
	 * physical memory to the dump device. If a Partial Selective 
	 * Dump is being performed then write out all of the kernel
	 * (text/data/bss/valloc) pages to the dump device.
	 */
	if(dumpinfo.size_to_dump > 0) {
		start = startaddr;
		*blocks = dumpinfo.size_to_dump > DBSIZE 
				? DBSIZE : dumpinfo.size_to_dump;
		startaddr += (*blocks * DEV_BSIZE);
		dumpinfo.size_to_dump -= *blocks;
		return(1);
	}

	/*
	 * If a Partial Selective Dump is being performed then write
	 * out all of the selected physical memory pages to the dump 
	 * device.
	 */
	if(dumpinfo.partial_dump) {
		switch(nextthing) {
		case WRITE_DUMPDESC:
			getdumppages();
			start = (char *)&dumpdesc[0];
			*blocks = 1;
			nextthing = WRITE_DUMPPAGES;
			nextindex = 0;
			break;

		case WRITE_DUMPPAGES:
			start = (char *)
				((dumpdesc[nextindex] * NBPG) | 0x80000000);
			*blocks = (NBPG * CLSIZE) / DEV_BSIZE;
			if(!dumpdesc[nextindex])
				done = 1;
			else if(++nextindex == NPFNDESC)
			    	nextthing = WRITE_DUMPDESC;
			else
			    	nextthing = WRITE_DUMPPAGES;
			break;

		default:
			break;
		}
	}
	else
		done = 1;

	if(done)
		return(0);
	else
		return(1);
}

/*************************************************************
 *
 * selectdumppages()
 *
 * Selectively choose which physical memory pages need to get
 * dumped when a system crash occurs. Calculate the total number
 * of physical memory pages that will be dumped. Calculate the
 * total number of dump descriptor blocks that will be dumped.
 * Make neccessary adjustments if all the the selected physical
 * memory pages will not fit on the dump device. Finally setup 
 * the "dumpinfo" structure before leaving.
 *
 ************************************************************/
selectdumppages()
{
	int i, j, k, kmx, pgtype, numofpages;
	int dumpsizepages, dumpdescpages; 
	int totaldumpblks, totaldumppages;
	struct pte *pte;
	struct proc *rpp;
	struct cpudata *pcpu;

	/* Get the selected pages of physical memory we need to dump */
	for(pgtype=0; pgtype<NPAGETYPES; pgtype++) {
		numofpages = 0;
		switch(pgtype) {
		/* Get size of the KERNEL IMAGE in pages */
		case KERNEL_PAGES:	
			if(dumpbuffercache)
			    numofpages = (physmem - maxmem);
			else
			    numofpages = lastkpage;
			if(numofpages == 0)
			    numofpages = physmem;
			dumppages[pgtype] = numofpages;
#ifdef mips
			if(dumppages[pgtype] != physmem)
			    dumppages[pgtype] += 0x30000/NBPG;
#endif mips
			break;

		/* Get size of the KMALLOC DATA in pages */
		case KMALLOC_PAGES:	
			for(i=0; i<(ekmempt - kmempt); i += CLSIZE) {
			    if(VALID_PTE(&kmempt[i]))
				numofpages += CLSIZE;
			}
			dumppages[pgtype] = numofpages;
			break;

		/* Get size of the ACTIVE UAREAS/UPROGS in pages */
		case AUAREA_PAGES:	
		case AUPROG_PAGES:	
			for(i=lowcpu; i<=highcpu; i++) {
			    /* Check if a process is assigned to this cpu */
			    if ((pcpu=CPUDATA(i)) == 0) continue;
			    pcpu = (struct cpudata *) 
					(svtophy(pcpu) | 0x80000000);
			    if (pcpu->cpu_noproc) continue;
			    rpp = pcpu->cpu_proc;
			    if(pgtype == AUAREA_PAGES) {
			        /* Get size of the PAGE TABLES */
        		        selectpagetables(rpp, &numofpages);
#ifdef vax
			        /* Get start of u_area page table */
			        kmx = btokmx(rpp->p_addr);
				if(!VALID_PTABLE(kmx))
				    continue;

			        /* Get start of u_area ptes */
			        pte = (struct pte *)
			            ((Usrptmap[kmx].pg_pfnum * NBPG) +
			      	        ((int)rpp->p_addr & VA_BYTEOFFS));
#endif vax
#ifdef mips
			        /* Get start of u_area ptes */
			        pte = rpp->p_addr;
#endif mips
			        /* Get size of (U_AREA) */
	                        for(k=0; k<UPAGES; 
					k += CLSIZE, pte += CLSIZE) {
				    if(VALID_PTE(pte))
					numofpages += CLSIZE;
				}
			    }
			    else {
			        /* Get size of (TEXT,DATA,STACK,SHM) */
				selectprogpages(rpp, &numofpages);
			    }
			}
			dumppages[pgtype] = numofpages;
			break;
		
		/* Get size of the INACTIVE UAREAS/UPROGS in pages */
		case IUAREA_PAGES:	
		case IUPROG_PAGES:
			FORALLPROC(
			    for(i=lowcpu; i<=highcpu; i++) {
			    	/* Check if a process is assigned to this cpu */
			    	if ((pcpu=CPUDATA(i)) == 0) continue;
			        pcpu = (struct cpudata *) 
					    (svtophy(pcpu) | 0x80000000);
			    	if (pcpu->cpu_noproc) continue;

			    	if (pp == pcpu->cpu_proc)
				    NEXTPROC;
			    }
			    if(pgtype == IUAREA_PAGES) {
			        /* Check if u_area pages are in core memory */
			        if(pp->p_sched & SLOAD) {
			            /* Get size of the PAGE TABLES */
        		            selectpagetables(pp, &numofpages);
#ifdef vax
			            /* Get start of u_area page table */
			            kmx = btokmx(pp->p_addr);
				    if(!VALID_PTABLE(kmx)) 
				        NEXTPROC;

			            /* Get start of u_area ptes */
			            pte = (struct pte *)
			                ((Usrptmap[kmx].pg_pfnum * NBPG) +
			      	            ((int)pp->p_addr & VA_BYTEOFFS));
#endif vax
#ifdef mips
			            /* Get start of u_area ptes */
			            pte = pp->p_addr;
#endif mips
			            /* Get size of (U_AREA) */
	                            for(k=0; k<UPAGES; 
					    k += CLSIZE, pte += CLSIZE) {
				        if(VALID_PTE(pte))
					    numofpages += CLSIZE;
				    }
				}
			    }
			    else {
			        /* Check if u_prog pages are in core memory */
			        if((pp->p_sched & SLOAD) && !(pp->p_vm & SNOVM))
			            /* Get size of (TEXT,DATA,STACK,SHM) */
				    selectprogpages(pp, &numofpages);
			    }
			)
			dumppages[pgtype] = numofpages;
			break;
		
		default:
			break;
		}
	}

	/* Check if all the selected pages will fit on the dump device */
	numofpages = 0;
	dumpsizepages = btop(dbtob(dumpsize));
	for(i=0; i<NPAGETYPES; i++) {
		numofpages += dumppages[i];
		/* Make adjustments if the selected pages will not all fit */
		if(numofpages >= dumpsizepages) {
			numofpages -= dumppages[i];
			dumppages[i] = dumpsizepages - numofpages;
			if(dumpdebug)
				cprintf("making adjustments 1 pgtype = %d\n",i);
			for(j=i+1; j<NPAGETYPES; j++)
				dumppages[j] = 0;
			break;
		}
	}

	/* Calculate the total number of selected pages to be dumped */
	totaldumppages = 0;
	for(i=0; i<NPAGETYPES; i++)
		totaldumppages += dumppages[i];
	totaldumpblks = btodb(ptob(totaldumppages));

	/* Calculate the total number of dump descriptor blocks to be dumped */
	numofpages = totaldumppages - dumppages[KERNEL_PAGES];
	dumpdescriptors = numofpages / (NPFNDESC * CLSIZE);
	if((numofpages) % (NPFNDESC * CLSIZE))
		dumpdescriptors++;

	/* Make adjustments for these dump descriptor blocks if necessary */
	dumpdescpages = rbtop(dbtob(dumpdescriptors));
	if((totaldumppages + dumpdescpages) > dumpsizepages) {
		for(i=NPAGETYPES-1; i>=0; i--) {
			if(dumppages[i]) {
			    if(dumpdebug)
				cprintf("making adjustments 2 pgtype = %d\n",i);
			    if(dumppages[i] >= dumpdescpages) {
				dumppages[i] -= dumpdescpages;
				break;
			    }
			    else {
				dumpdescpages -= dumppages[i];
				dumppages[i] = 0;
			    }
			}
		}
	}

	/* Recalculate the total number of selected pages to be dumped */
	totaldumppages = 0;
	for(i=0; i<NPAGETYPES; i++)
		totaldumppages += dumppages[i];
	totaldumpblks = btodb(ptob(totaldumppages)) + dumpdescriptors;

	/* Setup dumpinfo structure for a partial dump */
	dumpinfo.size_to_dump = dumpsize = btodb(ptob(dumppages[KERNEL_PAGES]));
	dumpinfo.blkoffs += dumplo;
	dumpinfo.partial_dump = 1;
	dumpinfo.pagesdumped = dumppages[KERNEL_PAGES];
	dumpinfo.totaldumppages = totaldumppages;
	dumpinfo.totaldumpblks = totaldumpblks;
	dumpsize2 = btodb(ptob(totaldumppages - dumppages[KERNEL_PAGES]));

	/* Restore TEXT and SHM structures to their orignal state */
	restore_text_shm();

	/* Debug information */
	if(dumpdebug) {
		cprintf("# of KERNEL_PAGES = %d\n",dumppages[KERNEL_PAGES]);
		cprintf("# of KMALLOC_PAGES = %d\n",dumppages[KMALLOC_PAGES]);
		cprintf("# of AUAREA_PAGES = %d\n",dumppages[AUAREA_PAGES]);
		cprintf("# of IUAREA_PAGES = %d\n",dumppages[IUAREA_PAGES]);
		cprintf("# of AUPROG_PAGES = %d\n",dumppages[AUPROG_PAGES]);
		cprintf("# of IUPROG_PAGES = %d\n",dumppages[IUPROG_PAGES]);
		cprintf("# of descriptors = %d\n",dumpdescriptors);
		cprintf("totaldumppages = %d\n",totaldumppages);
		cprintf("totaldumpblks = %d\n",totaldumpblks);
	}
}

/*************************************************************
 *
 * selectprogpages()
 *
 * Select the user program pages to be dumped. Calculate the
 * total number of (TEXT, DATA, STACK, SHM) pages that need
 * to get dumped per process.
 *
 ************************************************************/
selectprogpages(p, npages)
struct proc *p;
int *npages;
{

	int i, j, kmx;
	struct smem *sp;
	struct pte *pte;

	if(!dumpprogpages || (p->p_type & SSYS))
		return;

	/* Get size of TEXT region in pages */
	for(i=0; i<p->p_tsize; i += CLSIZE) {
		if(!VALID_TEXTP(p->p_textp))
		     break;
		if(p->p_textp && (p->p_textp->x_flag & XDUMP))
			break;
		kmx = btokmx(tptopte(p,i));
		if(!VALID_PTABLE(kmx))
		     break;
		pte = (struct pte *)((Usrptmap[kmx].pg_pfnum * NBPG) +
		      ((int)tptopte(p,i) & VA_BYTEOFFS) | 0x80000000);
		if(VALID_FODPTE(pte))
		    *npages += CLSIZE;
	}
	if(p->p_textp && VALID_TEXTP(p->p_textp) 
			&& !(p->p_textp->x_flag & XDUMP))
		p->p_textp->x_flag |= XDUMP;
		
	/* Get size of DATA region in pages */
	for(i=0; i<p->p_dsize; i += CLSIZE) {
		kmx = btokmx(dptopte(p,i));
		if(!VALID_PTABLE(kmx))
		     break;
		pte = (struct pte *)((Usrptmap[kmx].pg_pfnum * NBPG) +
		      ((int)dptopte(p,i) & VA_BYTEOFFS) | 0x80000000);
		if(VALID_FODPTE(pte))
		    *npages += CLSIZE;
	}

	/* Get size of STACK region in pages */
	for(i=0, j=p->p_ssize-1; i<p->p_ssize; i += CLSIZE, j -= CLSIZE) {
		kmx = btokmx(sptopte(p,j));
		if(!VALID_PTABLE(kmx))
		     break;
		pte = (struct pte *)((Usrptmap[kmx].pg_pfnum * NBPG) +
		      ((int)sptopte(p,j) & VA_BYTEOFFS) | 0x80000000);
		if(VALID_FODPTE(pte))
		    *npages += CLSIZE;
	}

	/* Get size of SHM region in pages */
	if(p->p_smbeg) {
		for(i=0; i<sminfo.smseg; i++) {
	            if(va_topsm_phys(p->p_sm,i,&sp)) 
			break;
		    if(sp == NULL || !VALID_SHMP(sp)) 
			continue;
		    for(j=0; j<btoc(sp->sm_size); j += CLSIZE) {
		        if(sp->sm_flag & XDUMP)
			    break;
		        kmx = btokmx(sp->sm_ptaddr+j);
			if(!VALID_PTABLE(kmx))
		     	    break;
			pte = (struct pte *)((Usrptmap[kmx].pg_pfnum * NBPG) +
		        ((int)(sp->sm_ptaddr+j) & VA_BYTEOFFS) | 0x80000000);
			if(VALID_FODPTE(pte))
		       	    *npages += CLSIZE;
		    }
		    sp->sm_flag |= XDUMP;
		}
	}
}

/*************************************************************
 *
 * selectpagetables()
 *
 * Select the user program page tables to be dumped. Calculate 
 * the total number of (TEXT, DATA, STACK, SHM) page tables that
 * need to get dumped per process.
 *
 ************************************************************/
selectpagetables(p, npages)
struct proc *p;
int *npages;
{

	int i, j, kmx;
	struct smem *sp;

#ifdef vax
	/* Get the size of the (TEXT,DATA,STACK) page tables */
	kmx = btokmx(p->p_p0br);
	for(i=0; i<p->p_szpt; i += CLSIZE) {
		if(!VALID_PTABLE(kmx+i))
		    break;
		else
		    *npages += CLSIZE;
	}
#endif vax
#ifdef mips
	/* Get the size of TEXT page tables */
	kmx = btokmx(p->p_textbr);
	for(i=0; i<p->p_textpt; i += CLSIZE) {
		if(!VALID_TEXTP(p->p_textp))
		    break;
		if(p->p_textp && (p->p_textp->x_flag & XDUMP))
		    break;
		if(!VALID_PTABLE(kmx+i))
		    break;
		else
		    *npages += CLSIZE;
	}
	if(!dumpprogpages) {
		if(p->p_textp && VALID_TEXTP(p->p_textp) 
				&& !(p->p_textp->x_flag & XDUMP))
			p->p_textp->x_flag |= XDUMP;
	}

	/* Get the size of DATA page tables */
	kmx = btokmx(p->p_databr);
	for(i=0; i<p->p_datapt; i += CLSIZE) {
		if(!VALID_PTABLE(kmx+i))
		    break;
		else
		    *npages += CLSIZE;
	}

	/* Get the size of STACK page tables */
	kmx = btokmx(p->p_stakbr);
	for(i=0; i<p->p_stakpt; i += CLSIZE) {
		if(!VALID_PTABLE(kmx+i))
		    break;
		else
		    *npages += CLSIZE;
	}
#endif mips

	/* Get the size of SHM page tables */
	if(p->p_smbeg) {
		for(i=0; i<sminfo.smseg; i++) {
	            if(va_topsm_phys(p->p_sm,i,&sp)) 
			break;
		    if(sp == NULL || !VALID_SHMP(sp)) 
			continue;
		    kmx = btokmx(sp->sm_ptaddr);
		    for(j=0; j<ctopt(btoc(sp->sm_size)); j += CLSIZE) {
		        if(sp->sm_flag & XDUMP)
			    break;
		        if(!VALID_PTABLE(kmx+j))
		   	    break;
		        *npages += CLSIZE;
		    }
		    if(!dumpprogpages)
		        sp->sm_flag |= XDUMP;
		}
	}
}

/*************************************************************
 *
 * getdumppages()
 *
 * Get the selected pages to dump out to the dump device. This
 * routine will create a dump descriptor block which will contain 
 * the starting page frame numbers of the next set of cluster pages 
 * of physical memory to dump. It will return (1) as long as there 
 * are more pages to dump and (0) when we are done.
 *
 ************************************************************/
getdumppages()
{
	int type, loc, count, pgtype, kmx; 
	int i, j, k, index, npagesdumped;
	struct pte *pte;
	struct proc *rpp;
	struct cpudata *pcpu;

	/* Initialize */
	index = count = 0;

	/* Zero out the dump descriptor block */
	bzero(&dumpdesc[0], sizeof(dumpdesc));

	/* Check if we are done dumping out the selected pages */
	if(dumpinfo.pagesdumped >= dumpinfo.totaldumppages)
		return(0);

	/* Calculate where to get next pages to dump from */
	npagesdumped = dumpinfo.pagesdumped;
	for(i=0; i<NPAGETYPES; i++) {
		if(npagesdumped >= dumppages[i]) {
			npagesdumped -= dumppages[i];
		}
		else {
			type = i;
			loc = npagesdumped;
			break;
		}
	}

	/* Debug information */
	if(dumpdebug) {
		cprintf("pagesdumped = %d totaldumppages = %d\n",
			dumpinfo.pagesdumped,dumpinfo.totaldumppages);
		cprintf("pagetype = %d pageloc = %d\n",type,loc);
	}

	/* Update the number of pages being dumped */
	dumpinfo.pagesdumped += (NPFNDESC * CLSIZE);
	
	/* Get next set of cluster pages to dump */
	for(pgtype=type; pgtype<NPAGETYPES; pgtype++) {
		switch(pgtype) {
		/* Create dump descriptors for the KERNEL IMAGE pages */
		case KERNEL_PAGES:	
		    break;

		/* Create dump descriptors for the KMALLOC DATA pages */
		case KMALLOC_PAGES:
		    for(j=0; j<(ekmempt - kmempt); j += CLSIZE) {
			if(VALID_PTE(&kmempt[j])) {
		            if(count < loc)
			        count += CLSIZE;
			    else {
		    	        dumpdesc[index++] = kmempt[j].pg_pfnum;
		        	if(index == NPFNDESC)
			    	    goto out;
			    }
		        }
		    }
		    break;

	        /* Create dump descriptors for the ACTIVE UAREA/UPROG pages */
	        case AUAREA_PAGES:
	        case AUPROG_PAGES:
		    for(j=lowcpu; j<=highcpu; j++) {
		        /* Check if a process is assigned to this cpu */
			if ((pcpu=CPUDATA(j)) == 0) continue;
			pcpu = (struct cpudata *) 
				    (svtophy(pcpu) | 0x80000000);
			if (pcpu->cpu_noproc) continue;
		        rpp = pcpu->cpu_proc;
			if(pgtype == AUAREA_PAGES) {
			    /* Get the page tables pfns */
        		    getpagetables(rpp, &count, &loc, &index);
			    if(index == NPFNDESC)
			        goto out;
#ifdef vax
			    /* Get start of u_area page table */
			    kmx = btokmx(rpp->p_addr);
			    if(!VALID_PTABLE(kmx))
			        continue;

			    /* Get start of u_area ptes */
			    pte = (struct pte *)
			        ((Usrptmap[kmx].pg_pfnum * NBPG) +
			      	    ((int)rpp->p_addr & VA_BYTEOFFS));
#endif vax
#ifdef mips
			    /* Get start of u_area ptes */
			    pte = rpp->p_addr;
#endif mips
			    /* Get the u_area pfns */
	                    for(k=0; k<UPAGES; 
					k += CLSIZE, pte += CLSIZE) {
				if(VALID_PTE(pte)) {
		                    if(count < loc)
				        count += CLSIZE;
				    else {
	    	                        dumpdesc[index++] = pte->pg_pfnum;
	    	                        if(index == NPFNDESC)
			                    goto out;
				    }
				}
	                    }
			}
			else {
			    /* Get the u_prog (TEXT,DATA,STACK,SHM) pfns */
			    getprogpages(rpp, &count, &loc, &index);
			    if(index == NPFNDESC)
				goto out;
			}
		    }
		    break;

		/* Create dump descriptors for the INACTIVE UAREA/UPROG pages */
		case IUAREA_PAGES:
		case IUPROG_PAGES:
		    FORALLPROC(
			for(i=lowcpu; i<=highcpu; i++) {
			    /* Check if a process is assigned to this cpu */
			    if ((pcpu=CPUDATA(i)) == 0) continue;
			    pcpu = (struct cpudata *) 
				        (svtophy(pcpu) | 0x80000000);
			    if (pcpu->cpu_noproc) continue;

			    if (pp == pcpu->cpu_proc)
			        NEXTPROC;
			}
			if(pgtype == IUAREA_PAGES) {
		            /* Check if u_area pages are in core memory */
		            if(pp->p_sched & SLOAD) {
			        /* Get the page tables pfns */
        		    	getpagetables(pp, &count, &loc, &index);
			    	if(index == NPFNDESC)
			            goto out;
#ifdef vax
			        /* Get start of u_area page table */
			        kmx = btokmx(pp->p_addr);
			        if(!VALID_PTABLE(kmx))
				    NEXTPROC;

			        /* Get start of u_area ptes */
				kmx = btokmx(pp->p_addr);
			        pte = (struct pte *)
			            ((Usrptmap[kmx].pg_pfnum * NBPG) +
			      	     	((int)pp->p_addr & VA_BYTEOFFS));
#endif vax
#ifdef mips
			        /* Get start of u_area ptes */
			        pte = pp->p_addr;
#endif mips
			        /* Get the u_area pfns */
		                for(j=0; j<UPAGES; 
					j += CLSIZE, pte += CLSIZE) {
				    if(VALID_PTE(pte)) {
		            	        if(count < loc)
			        	    count += CLSIZE;
				        else {
	    	                            dumpdesc[index++] = pte->pg_pfnum;
		    	                    if(index == NPFNDESC)
				                goto out;
				        }
				    }
		                }
		            }
		  	}
			else {
		            /* Check if u_prog pages are in core memory */
		            if((pp->p_sched & SLOAD) && !(pp->p_vm & SNOVM)) {
			        /* Get the u_prog (TEXT,DATA,STACK,SHM) pfns */
			    	getprogpages(pp, &count, &loc, &index);
			    	if(index == NPFNDESC)
				    goto out;
			    }
			}
		    )
		    break;

		default:
		    break;
		}
	}
out:
	/* Make sure we didn't get to much on the last dump descriptor block */
	if(dumpinfo.pagesdumped > dumpinfo.totaldumppages) {
		k = (dumpinfo.pagesdumped - dumpinfo.totaldumppages) / CLSIZE;
		for(i=NPFNDESC-k; i<NPFNDESC; i++)
			dumpdesc[i] = 0;
	}

	/* Restore TEXT and SHM structures to their orignal state */
	restore_text_shm();

	/* Calculate where next pages will be gotten from */
	npagesdumped = dumpinfo.pagesdumped;
	for(i=0; i<NPAGETYPES; i++) {
		if(npagesdumped >= dumppages[i]) {
			npagesdumped -= dumppages[i];
		}
		else {
			type = i;
			loc = npagesdumped;
			break;
		}
	}

	/* Mark the TEXT and SHM structures that are done */
	if(type == IUPROG_PAGES)
		mark_text_shm();

	/* Debug information */
	if(dumpdebug) {
		cprintf("DUMP OF DUMPDESCRIPTOR BLOCK #%d\n",dumpdescnum++);
		for(i=0; i<NPFNDESC; i += 8) {
		    cprintf("PFN (%d-%d): %d %d %d %d %d %d %d %d\n", i, i+7,
			dumpdesc[i], dumpdesc[i+1], dumpdesc[i+2],
			dumpdesc[i+3], dumpdesc[i+4], dumpdesc[i+5],
			dumpdesc[i+6], dumpdesc[i+7]);
		}
	}

	if(index != 0)
		return(1);
	else
		return(0);
}

/*************************************************************
 *
 * getprogpages()
 *
 * Get the user program pages to dump out to the dump device.
 * Get the (TEXT, DATA, STACK, SHM) pages that need to get
 * dumped per process.
 *
 ************************************************************/
getprogpages(p, count, loc, index)
struct proc *p;
int *count;
int *loc;
int *index;
{

	int i, j, offset, kmx;
	struct smem *sp;
	struct pte *pte;

	if(!dumpprogpages || (p->p_type & SSYS))
		return;

	/* Create dump descriptors for the TEXT region pages */
	for(i=0; i<p->p_tsize; i += CLSIZE) {
		if(!VALID_TEXTP(p->p_textp))
		     break;
		if(p->p_textp && (p->p_textp->x_flag & XDUMP))
			break;
		kmx = btokmx(tptopte(p,i));
		if(!VALID_PTABLE(kmx))
		     break;
		pte = (struct pte *)((Usrptmap[kmx].pg_pfnum * NBPG) +
		      ((int)tptopte(p,i) & VA_BYTEOFFS) | 0x80000000);
		if(VALID_FODPTE(pte)) {
		    if(*count < *loc)
		        *count += CLSIZE;
		    else {
		        dumpdesc[(*index)++] = pte->pg_pfnum;
		        if(*index == NPFNDESC)
			    return;
		    }
		}
	}
	if(p->p_textp && VALID_TEXTP(p->p_textp) 
			&& !(p->p_textp->x_flag & XDUMP))
		p->p_textp->x_flag |= XDUMP;
		

	/* Create dump descriptors for the DATA region pages */
	for(i=0; i<p->p_dsize; i += CLSIZE) {
		kmx = btokmx(dptopte(p,i));
		if(!VALID_PTABLE(kmx))
		     break;
		pte = (struct pte *)((Usrptmap[kmx].pg_pfnum * NBPG) +
		      ((int)dptopte(p,i) & VA_BYTEOFFS) | 0x80000000);
		if(VALID_FODPTE(pte)) {
		    if(*count < *loc)
		        *count += CLSIZE;
		    else {
		        dumpdesc[(*index)++] = pte->pg_pfnum;
		        if(*index == NPFNDESC)
			    return;
		    }
		}
	}

	/* Create dump descriptors for the STACK region pages */
	for(i=0, j=p->p_ssize-1; i<p->p_ssize; i += CLSIZE, j -= CLSIZE) {
		kmx = btokmx(sptopte(p,j));
		if(!VALID_PTABLE(kmx))
		     break;
		pte = (struct pte *)((Usrptmap[kmx].pg_pfnum * NBPG) +
		      ((int)sptopte(p,j) & VA_BYTEOFFS) | 0x80000000);
		if(VALID_FODPTE(pte)) {
		    if(*count < *loc)
		        *count += CLSIZE;
		    else {
		        dumpdesc[(*index)++] = pte->pg_pfnum;
		        if(*index == NPFNDESC)
			    return;
		    }
		}
	}

	/* Create dump descriptors for the SHM region pages */
	if(p->p_smbeg) {
		for(i=0; i<sminfo.smseg; i++) {
	            if(va_topsm_phys(p->p_sm,i,&sp)) 
			break;
		    if(sp == NULL || !VALID_SHMP(sp)) 
		    	continue;
		    for(j=0; j<btoc(sp->sm_size); j += CLSIZE) {
		        if(sp->sm_flag & XDUMP)
			    break;
		        kmx = btokmx(sp->sm_ptaddr+j);
			if(!VALID_PTABLE(kmx))
		     	    break;
			pte = (struct pte *)((Usrptmap[kmx].pg_pfnum * NBPG) +
		        ((int)(sp->sm_ptaddr+j) & VA_BYTEOFFS) | 0x80000000);
			if(VALID_FODPTE(pte)) {
		            if(*count < *loc)
		                *count += CLSIZE;
			    else {
		                dumpdesc[(*index)++] = pte->pg_pfnum;
		                if(*index == NPFNDESC)
			            return;
			    }
		        }
		    }
		    sp->sm_flag |= XDUMP;
		}
	}
}

/*************************************************************
 *
 * getpagetables()
 *
 * Get the user program page tables to dump out to the dump
 * device. Get the (TEXT, DATA, STACK, SHM) page tables that
 * need to get dumped per process.
 *
 ************************************************************/
getpagetables(p, count, loc, index)
struct proc *p;
int *count;
int *loc;
int *index;
{

	int i, j, kmx;
	struct smem *sp;

#ifdef vax
	/* Create dump descriptors for the (TEXT,DATA,STACK) page tables */
	kmx = btokmx(p->p_p0br);
	for(i=0; i<p->p_szpt; i += CLSIZE) {
		if(!VALID_PTABLE(kmx+i))
		    break;
		if(*count < *loc)
		    *count += CLSIZE;
		else {
		    dumpdesc[(*index)++] = Usrptmap[kmx+i].pg_pfnum;
		    if(*index == NPFNDESC)
		    	return;
		}
	}
#endif vax
#ifdef mips
	/* Create dump descriptors for the TEXT page tables */
	kmx = btokmx(p->p_textbr);
	for(i=0; i<p->p_textpt; i += CLSIZE) {
		if(!VALID_TEXTP(p->p_textp))
		    break;
		if(p->p_textp && (p->p_textp->x_flag & XDUMP))
		    break;
		if(!VALID_PTABLE(kmx+i))
		    break;
		if(*count < *loc)
		    *count += CLSIZE;
		else {
		    dumpdesc[(*index)++] = Usrptmap[kmx+i].pg_pfnum;
		    if(*index == NPFNDESC)
		    	return;
		}
	}
	if(!dumpprogpages) {
		if(p->p_textp && VALID_TEXTP(p->p_textp) 
				&& !(p->p_textp->x_flag & XDUMP))
			p->p_textp->x_flag |= XDUMP;
	}

	/* Create dump descriptors for the DATA page tables */
	kmx = btokmx(p->p_databr);
	for(i=0; i<p->p_datapt; i += CLSIZE) {
		if(!VALID_PTABLE(kmx+i))
		    break;
		if(*count < *loc)
		    *count += CLSIZE;
		else {
		    dumpdesc[(*index)++] = Usrptmap[kmx+i].pg_pfnum;
		    if(*index == NPFNDESC)
		    	return;
		}
	}

	/* Create dump descriptors for the STACK page tables */
	kmx = btokmx(p->p_stakbr);
	for(i=0; i<p->p_stakpt; i += CLSIZE) {
		if(!VALID_PTABLE(kmx+i))
		    break;
		if(*count < *loc)
		    *count += CLSIZE;
		else {
		    dumpdesc[(*index)++] = Usrptmap[kmx+i].pg_pfnum;
		    if(*index == NPFNDESC)
		    	return;
		}
	}
#endif mips

	/* Create dump descriptors for the SHM page tables */
	if(p->p_smbeg) {
		for(i=0; i<sminfo.smseg; i++) {
	            if(va_topsm_phys(p->p_sm,i,&sp)) 
			break;
		    if(sp == NULL || !VALID_SHMP(sp)) 
		    	continue;
		    kmx = btokmx(sp->sm_ptaddr);
		    for(j=0; j<ctopt(btoc(sp->sm_size)); j += CLSIZE) {
		        if(sp->sm_flag & XDUMP)
			    break;
		        if(!VALID_PTABLE(kmx+j))
		   	    break;
		    	if(*count < *loc)
		            *count += CLSIZE;
		        else {
		            dumpdesc[(*index)++] = Usrptmap[kmx+j].pg_pfnum;
		            if(*index == NPFNDESC)
			        return;
		        }
		    }
		    if(!dumpprogpages)
		        sp->sm_flag |= XDUMP;
		}
	}
}

/*************************************************************
 *
 * restore_text_shm()
 *
 * Restore TEXT and SHM structures to their original state.
 *
 ************************************************************/
restore_text_shm()
{

	struct text *xp;
	struct smem *sp;
	int i;

	for(xp = text; xp<textNTEXT; xp++) {
		if(xp->x_flag & XDUMP)
			xp->x_flag &= ~XDUMP;
	}

	for(sp = &smem[0], i=0; i<sminfo.smmni; i++, sp++) {
		if(sp->sm_flag & XDUMP)
			sp->sm_flag &= ~XDUMP;
	}
}

/*************************************************************
 *
 * mark_text_shm()
 *
 * Mark the TEXT and SHM structures that are done.
 *
 ************************************************************/
mark_text_shm()
{

	int i, j;
	struct proc *p;
	struct smem *sp;
	struct cpudata *pcpu;

	for(i=lowcpu; i<=highcpu; i++) {
		/* Check if a process is assigned to this cpu */
		if ((pcpu=CPUDATA(i)) == 0) continue;
		pcpu = (struct cpudata *) 
			    (svtophy(pcpu) | 0x80000000);
		if (pcpu->cpu_noproc) continue;
		p = pcpu->cpu_proc;

		/* Mark TEXT structure as done */
		if(p->p_textp && VALID_TEXTP(p->p_textp))
		    p->p_textp->x_flag |= XDUMP;

		/* Mark SHM structures as done */
		if(p->p_smbeg) {
		    for(j=0; j<sminfo.smseg; j++) {
	                if(va_topsm_phys(p->p_sm,j,&sp)) 
			    break;
		        if(sp == NULL || !VALID_SHMP(sp)) 
			    continue;
		 	sp->sm_flag |= XDUMP;
		    }
		}
	}
}

/*************************************************************
 *
 * va_topsm_phys()
 *
 * Fetch the shared memory structure pointer for a process.
 *
 ************************************************************/
va_topsm_phys(psmp,index,spp)
register struct p_sm *psmp;
int index;
struct smem **spp;
{
	register struct pte *pte;
	register struct smem **tspp;
	long kindex;

	tspp = &psmp[index].sm_p;
	kindex = btop(((caddr_t)tspp - kmembase));
	if((kindex < 0) || (kindex > (ekmempt - kmempt)) 
		|| !VALID_PTE(&kmempt[kindex]))
		return(-1);
	*spp = *(struct smem **)((kmempt[kindex].pg_pfnum * NBPG) +
		((int)tspp & VA_BYTEOFFS) | 0x80000000);
	return(0);
}

#ifdef mips
/*************************************************************
 *
 * dumpsetupvectors()
 *
 * Reset the operating system exception vector handling code
 * after a RESET. This is needed to get dumps from MIPS machines
 * that are hung.
 *
 ************************************************************/
dumpsetupvectors()
{
	extern 	char utlbmiss[], eutlbmiss[];
	extern 	char exception[], eexception[];

	bcopy(utlbmiss, UT_VEC, eutlbmiss - utlbmiss);
	bcopy(exception, E_VEC, eexception - exception);
	flush_cache();
}
#endif mips

itoa(number, string)	/* integer to ascii routine */
int number;
char string[];
{
	int a, b, c;
	int i, sign;
	if (( sign = number) < 0)
		number = number * -1;
	i = 0;
	do {
		string[i++] = number % 10 + '0';
	} while ((number /= 10) > 0);
	if (sign < 0)
		string[i++] = '-';
	string[i] = '\0';
	/* flip the string in place */
	for (b = 0, c = strlen(string)-1; b < c; b++, c--) {
		a = string[b];
		string[b] = string[c];
		string[c] = a;
	}
	return( strlen(string) );
}
