#ifndef lint
static char *sccsid = "@(#)machdep.c	4.8	ULTRIX	3/7/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986,87,88,89 by			*
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
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 * Modification history: /sys/vax/machdep.c
 *
 * 24-Feb-91	jsd
 *	new allocation scheme for gateway screen buffers
 *
 * 4-Sep-90	dlh
 *	added vector processor support code
 *
 * 31-Aug-90	paradis
 *	Changed restart inhibit code from 'if' to 'switch' (more
 *	modular); added VAX9000 support for clearing restart
 *	inhibit.
 *
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 *	Added support for VAX9000.
 *
 * 09-Nov-89 -- jaw
 * 	change references to maxcpu to smp.
 *
 * 16-Oct-89 -- Alan Frechette
 *	No longer dump out the buffer cache during a system crash
 *	for partial selective crash dumps. Save the PFN of the last 
 *	kernel page to dump out for the crashdump code. The buffer 
 *	cache must be the last allocated data for this to work. 
 *
 * 25 Jul 89 -- chet
 *	Change unmount and cache flushing code in boot()
 *
 * 24-July-89 -- Alan Frechette
 *	Moved all the crashdump code to a new file. The new file
 *	containing the crashdump code is "/sys/sys/crashdump.c".
 *
 * 20-Jul-89 jaw
 *	move non-vax code to sys area.
 *
 * 19-Jun-89 -- condylis
 *	Tightened up unmounting of file systems in boot().
 *
 * 14 Jun 89 -- chet
 *	Make buffer header allocations based on maxcpu (uniprocessor
 *	or multiprocessor).
 *
 * 12-Jun-89	bp
 *	Repaired valloc of kmemusage to be dependent on CLSIZE units.
 *	Changed cpu_ip_intr to recognize kernel memory allocator TB
 *	synchronization requests.
 *	
 * 12-Jun-1989 -- jaw
 *  	IP interrupts doing reschedule (AST) need to set runrun flag.
 *
 * 12-Jun-1989 -- gg
 *	In dumpsys() added a check for the presence dumpdev.
 *
 * 07-Jun-1989 -- Tim Burke
 *    Modified gendump() to dump to MSCP (ra) disks.  This change 
 *    needed because ra disks can be within a range of major numbers.
 *
 * 25-May-89 -- fred (Fred Canter_
 *	Correct a mismerge. Remove two extra lines of code from the
 *	default case in gendump().
 *
 * 24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 * 24-May-89	darrell
 *	Removed the v_ prefix from all cpusw fields, removed cpup from any
 *	arguments being passed in function args.  cpup is now defined
 *	globally -- as part of the new cpusw.
 *
 * 12-May-89 -- darrell
 *	Merged V3.1 changes.
 *
 * 03-May-1989 -- gmm (merged from v3.1)
 * 	Added support for TMII. If cpu_sub_subtype indicates SB_TMII, 
 * 	use the parameters for a 90 nsec chip for delay.
 *
 * 10-Apr-89 -- gmm
 *	Fixed the path for kdb.h with the new source tree layout
 *
 * 06-Apr-89 -- prs
 *      Added SMP accounting lock in boot().
 *
 * 15-Feb-1989 -- darrell
 *	Added a case for VAX_60 to badaddr to disable mbus errors while
 *	doing a badaddr.
 *
 * 14-Feb-1989 -- prs
 *	Modified boot() to not sync out a remote accounting gnode when
 *	panicing. During a panic, some network interfaces shutdown before 
 *	calling boot().
 *
 * 26-Jan-89	jaw
 *	fix up start/stop cpu.
 *
 * 10-Jan-89 -- kong
 *	Added Rigel (VAX 6400) support.  Cleaned up microdelay routines
 *	by allowing them to be called at any IPL.
 *
 * 16-Jan-89 - jaw
 *	Bug in accounting.  When doing a reboot, if we panic once then
 *	don't try to turn off accouting the second time "boot" is called.
 *
 * 12-Jan-89 - jaw
 *	change to printf for attached processors so clists are not
 *	used.
 *
 * 1-Jan-1989 -- Fred Canter
 *	Modified gendump() to allow crash dumps to disks on either
 *	SCSI bus (even if root is on the opposite bus).
 *
 * 30-Dec-1988 -- Fred Canter
 *	Added reset of SCSI and ST506 I/O controllers to crash dump
 *	path (dumpsys) as required by thge VMB boot driver.
 *
 * 18-Nov-88 -- darrell
 *	Fixed a bug where the unit number of the dump device needed to
 *	be multiplied by 100 before being passed to VMB for Firefoxes
 *
 * 10-Oct-88 -- jaw
 *	initialize Istack variable on boot cpudata.
 *
 * 10-Oct-88 -- jaw
 *	replace switch_to_master with general routine switch_affinity
 *
 * 29-Sep-88 -- darrell
 *	Firefox Cleanup.  Removed Firefox specific code that called
 *	fccons_init.
 *
 * 19-Aug-88 -- miche
 *	Interrupt cpu goes through free_cpu and hold_cpu to
 *	hold and free processors.  This is coordinated with swtch
 *	We still need a routine which intr's everybody.
 *
 * 18-Jun-88 -- jaw  change to new cpu data format.
 *
 * 27-Apr-88 -- prs
 *      Enhanced code that turns accounting off in boot().
 *
 * 27 Apr 88 -- chet
 *	Add SMP buffer cache support.
 *
 * 27 Jan 88 -- gmm
 *	Added the new routine intrpt_cpu() to be used for causing an IP
 *	interrupt. Rewrote most of cpu_ip_intr() to conform to the new IPI
 *	interface definition.
 *
 * 27 Jan 88 -- us
 *	Added support for malloced network.  
 *
 * 21 Jan 88 -- jmartin
 *	Replace calls to the (inline) functions clearseg and copyseg
 *	respectively with blkclr (or bzero) and blkcpy (or bcopy).
 *	Establish a window in process memory through which a parent
 *	can write to (and read from) the memory of the child.  This
 *	window is UPAGES*NBPG bytes located between the u-area and the
 *	user stack.  Remove the following entities: CMAP1, CADDR1,
 *	CMAP2, CADDR2, Vfmap, vfutl, clearseg, copyseg.  Redefine
 *	Forkmap and forkutl.  Change the computation for the location
 *	of USRSTACK and the size of the process page table.
********************************************************************
		SMP changes above 
*********************************************************************
 *
 * 25-Sep-1988 -- Fred Canter
 *	Clean up comments.
 *
 * 19-Aug-1988 -- Fred Canter
 *	Removed last of PVAX BTD$ kludge.
 *
 * 30-July-1988 -- Fred Canter
 *	Fixed a bug in the PVAX microdelay routine. Caused delay to
 *	be 3 times what it should be if DELAY() called from probe.
 *	Caches not enabled yet when probe executed.
 *
 * 22-Jul-88 -- darrell
 *	Added IP interrupt support for VAX60 (Firefox).
 *
 * 13-Jul-88 -- chet
 *	Account for system table demand on physical memory
 *	in the buffer cache allocation in startup().
 *	Remove old code that ignored param.c parameter values.
 *	Add debugging messages and parameters for buffer cache allocation.
 *	Calculate usrptsize in startup().
 *
 * 28-June-1988 -- tresvik
 *      Move filling of installation specific rpb information from here
 *      to autoconf.c.  This is necessary to support get the
 *      ws_display_type for the LYNX which isn't filled in until
 *      lxprobe.
 *
 * 13-Jun-88 -- chet
 *	Added configurable buffer cache support
 *
 * 10-Jun-1988		Mark Parenti
 *	Modified signal mask so that SIGCONT can be ignored/blocked
 *	(POSIX 12.3)
 *
 * 08-Jun-88 -- fred (Fred Canter)
 *	Adjust microdelay routine for 90ns CPU and caches off.
 *
 * 07-Jun-88 -- fred (Fred Canter)
 *	Bug fix for cvs_cache_on global undefined if VAX420 not configured.
 *
 * 07-Jun-88 -- darrell
 *	Added Firefox support.
 *
 * 19-May-88 -- fred (Fred Canter)
 *	Improved microdelay routine for CVAXstar/PVAX (VAX420).
 *
 * 27-Apr-1988		David E. Eiche			DEE0032
 *	Modified generic dump code for all devices that use the
 *	Unibus structures to compare against the device name in
 *	the ubdinit structure.  This allows kernels built with
 *	"disk ran at mscp drive n" to dump.  Also made the check
 *	for the correct genericconf table entry more specific.
 *
 * 27-Apr-88 prs
 *      Enhanced code that turns accounting off in boot().
 *
 * 26-Apr-88    jaw
 *	Add VAX8820 support.
 *
 * 15-Apr-1988		Robin
 *	Added SII to allow dumps to rf disks.
 *
 * 07-Apr-1988		David E. Eiche			DEE0028
 *	Add code to enable dumping to the HSC.
 *
 * 4-Apr-88 jaa
 *	calculate and allocate swapmap in swapconf from now on
 *
 * 24-Mar-88 Robin (for Larry C.)
 *	moved setcache to autoconf to insure I/O space is mapped before
 *	init_main is called.  This fixes a problem introduced by the
 *	14-Mar-88 change (see note below).
 *
 * 15-May-88 tresvik
 *	move rpb initialization of cpu, cpusub, and ws_display_type
 *	to after vcons_init calls.  Need ws_display_type to be able
 *	to sense the environment during installation.  This should all
 *	move to getsysinfo in the future.
 *
 * 14-Mar-88 larry
 *	Move configure to init_main.c so that scs sysaps start up first.
 *
 * 8-Mar-88 jaa
 *     	Added variable usrptsize, now configurable by sysadm.
 *
 * 12-Feb-88 fred (Fred Canter)
 *	Changes for VAX420 (CVAXstar/PVAX) support.
 *
 * 5-Feb-88 tresvik
 *	increase MINMEM from 3 Meg to 4 Meg (hopefully temporarily)
 *
 * 19-Jan-88 -- jaw
 *	added calypso support.  Also moved in intrcpu into file from locore.s
 *
 * 15-Jan-88 Larry Palmer
 *	mbmap and nmbclusters are gone due to malloced mbufs.
 *	Also removed inclusion of ioctl.h and mbuf.h headers
 *	as we keep getting "too many defines from the preprocessor
 *
 * 14-Jan-1988 Tresvik
 *	Map up to a maximum of VMBINFOPAGES instead of all of the
 *	vmbinfo stack in high memory.  Only mapped access to vmb_info
 *	and ci ucode is needed anyway.  
 *
 *	Display the fact that physmem is being reduced, due to a small
 *	system page table, in a less frightful way.  The old way prompts
 *	too much undue concern.
 *
 * 11-Jan-1988  	Todd M. Katz
 *	Optionally invoke the SCS shutdown routine following synchronization
 *	of all disks.   The SCS shutdown routine disables all local ports.
 *	This automatically terminates all paths to all remote systems and
 *	provides the means for remote system discovery of this termination.
 *
 *	The SCS shutdown routine is indirectly invoked through the variable
 *	scs_disable.  It is only invoked if scs_disable has been initialized
 *	with the address of the SCS shutdown routine.  Such initialization
 *	occurs during SCS initialization itself which takes place during
 *	probing of the very first adapter requiring SCS for proper functioning.
 *
 * 12-11-87	Robin L. and Larry C.
 *      Added portclass/kmalloc support to the system.
 *
 * 28-Jul-87 -- prs
 *      Added code to gendump to allow dumping to an AIO device
 *      on a BI.
 *
 *
 * 11-Nov-87 -- prs
 *	Fixed a partial dump problem that would overwrite into
 *	the next partition.
 *
 * 14-Sep-87 -- afd
 *	Changed Mayfair CPMBX write to be a byte write rather than a short.
 *
 * 21-Jul-87 -- prs, Robin
 *	Removed ioctl call to get partition table information in
 *	dumpsys(). init_main.c now initializes part info.
 *
 * 25-Jun-87 -- prs
 *	Fixed netdump for consistent VAXstar dumps.
 *
 * 25-Jun-87 -- Robin
 *	Added code to get the dump partition size out of the system
 *	and not do a ioctl into the disk driver.  This insures that
 *	the IPL will not drop.
 *	
 * 02-Jun-87 -- logcher
 *	Added GSYNCG and crfree before GRELE of acctp to flush
 *	delayed write buffers.
 *
 * 20-Apr-87 -- afd
 *	Include ka650.h for Mayfair (after ubareg.h).
 *	Don't need to coerce types on ka650 console program mail box.
 *	Changed name CVAXQ to VAX3600 for Mayfair.
 *	microdelay now calls processor specific routines thru the cpu switch.
 *	There are now a few different routines which use
 *		different hardware features to accomplish micro delays.
 *
 * 25-Mar-87 -- logcher
 *	Change the hard coded /etc/init to /bin/init
 *
 * 19-Mar-87 -- prs
 *	Added partial crash dump code to the network dump routine 
 *	netdump.
 *
 * 18-Mar-87 -- logcher
 *	Added network dumping.
 *
 * 06-Mar-87 -- afd
 *	Added CPU specific support for Mayfair/CVAX.
 *
 * 13-Feb-87 -- Chase
 * 	change computation of nmbclusters to stay consistent with the
 *	formula	in /sys/vax/genassym.c.
 *
 * 12-Feb-87 -- depp
 *	Changed the sizing of dmempt
 *
 * 12-Feb-87 -- pmk
 *	Changed 3 gendump cprintf's for clearer meaning. Bar
 *
 * 15-Jan-87 -- Chase
 *	change dmemmap size to 512
 *
 * 15-jan-87 -- koehler
 *	change the calculation for the number of gnodes.
 *
 * 08-Jan-87 -- prs
 *	Added check to boot() to ensure a gnode has a mount device
 *	associated with it before an update is attempted.
 *
 * 16-Dec-86 -- prs
 *	Added code to boot() that turns accounting off if running. This
 *	allows the usr file system to umounted cleanly during shutdown.
 *
 * 04-Dec-86 -- prs
 *	Added conditional to allow the gendump routine to work on a
 *	microVAX I
 *
 * 04-Dec-86 -- Robin Lewis
 *	Removed the quota calls associated with setting login limits.  Login 
 *	limit code is now independent from MAXUSER,cpu type, and src code.
 *	(see /upgrade data file)
 *
 * 11-Sept-86  --  Tim Burke
 *	Reduce maxclistusers from 128 to 75 in order to tie up fewer resources.
 *
 * 30-Aug-86 -- fred (Fred Canter)
 *	Added/fixed comments in microdelay routine.
 *	Fixed crash dumps for VAXstar.
 *
 * 14-Aug-86 -- tresvik
 *	removed CAN BE HALTED message for MVAX II and VAXstar
 *	Physmem fixed again to act as memory size limit
 *	plug in CPU and CPU_SUBTYPE to the RPB for installation
 *	
 * 07-Aug-86 -- jaw	fixed microdelay to reflex change in VAX SRM
 *	to the ipl of the interval timer interrupt.
 *
 * 06-Aug-86 -- jaw	fixed baddaddr to work on running system.
 *	Also fixes to ka8800 reboot code.
 *
 *  5-Aug-86 -- fred (Fred Canter)
 *	Added VAXstar console program mail box support.
 *
 * 23-Jul-86 -- prs
 *	Removed the generic config table genericconf.
 *	The genericconf table is now built in conf.c. This will
 *	ensure that the table will only contain configured drivers.
 *
 * 26-Jul-86 -- bglover
 *	Append first startup printfs to startup mesg in error logger
 *
 * 18-Jun-86 -- fred (Fred Canter)
 *	Changes for VAXstar kernel support.
 *
 * 13-Jun-86 -- tresvik
 *	added support for network boot and installation by saving the
 *	address of vmb_info in the RPB for use be a user level program
 *	misc, fixes to Physmem, which got redefined to do something
 *	different.  It can now be preset to set an artificial top of
 *	memory as well as keep track of the actual amount for use by the
 *	sizer.  Increase min memory from 2 Meg to 3 Meg.
 *
 *  9-jun-86 -- bjg
 *	Set appendflg to log startup messages
 *
 *  2-Jun-86 -- depp
 *	Added passthough of "u.u_code" for SIGSEGV (see modification note
 *	"24 Feb 86 -- depp" below.
 *
 * 22-May-86 -- prs
 *	Added saving of u_area to partial dump code.
 *
 * 07-May-86 -- bjg
 *	Remove logstart() call; moved to clock.c.
 *
 * 16-Apr-86 -- darrell
 *	now calling badaddr with the macro BADADDR.
 *
 * 15-Apr-86 -- afd
 *	Re-wrote most of startup().  This changed the way we allocate
 *	system data structures.  The amount of space for "nbufs"
 *	is now calculated more intelligently.  The algorithm for
 *	this came from Berkeley 4.3.
 *
 *	The global variable "endofmem" now contains the last
 *	kernel virtual address that was used.
 *
 * 09 Apr 86 -- prs
 * 	Added common dump code taken from drivers to dumpsys.
 *	Also, added the generic dump routine, gendump, and
 *	partial crash dump support.
 *
 * 09-apr-86 -- jaw  use 8800 boot me command.
 *
 * 22-Mar-86 -- koehler
 *	changed the why the filesystem is handled when the system is 
 *	shutting down
 *
 * 19-Mar-86 -- pmk
 *	Changed microdelay so at ipl18 and above clock is used 
 *	and mvax delay now is appox. real microsec. delay.
 *
 * 12-mar-86 -- tresvik
 *	fixed loop for system page table too small; reduction of physmem
 *	to 2 meg.  Maxusers was being set too high and possibly higher
 *	than it's original setting.  Also, do not attempt to remap
 *	vmbinfo on retries.
 *
 * 11 Mar 86 -- robin
 *	Changed the login limit code to use a system call and counter.
 *
 * 11 Mar 86 -- lp
 *	Added n-buffered I/O support (in physstrat). Also
 *	added IOA reset in halt code for 86?0.
 *
 * 11 Mar 86 -- larry
 *	nmbclusters is now a function of maxusers
 *
 * 05 Mar 86 -- bjg
 *	Removed msgbuf from the kernel; replaced by error logger
 *
 * 24 Feb 86 -- depp
 *	Added new maps for kernel memory allocation mechanisms (dmempt*)
 *	and in the routine sendsig, added code passthrough for SIGBUS
 *	to complement the new system call "mprotect"
 *
 * 19-Feb-86 -- bjg  add call to log startup message (logstart();)
 *
 * 18-Feb-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *		     ka8800 cleanup.
 *
 * 12-Feb-86	Darrell Dunnuck
 *	Removed the routines memerr, memenable, setcache, and tocons
 *	from here and put them in the appropiate modules (kaXXXX.c)
 *	by processor type.
 *	Added a new routine cachenbl.
 *
 * 12-Feb-86 -- jrs
 *	Added tbsync() calls to control mp translation buffer
 *
 * 04-Feb-86 -- tresvik
 *	added VMB boot path support
 *
 * 04-feb-86 -- jaw  get rid of biic.h.
 *
 * 30 Jan 86 -- darrell
 *	All machine check headers, constants, and code has been
 *	removed from machdep.c and broken up by processor type
 *	and placed in separate modules.  The machine check 
 *	routines are now reached through the cpusw structure
 *	which is part of the machine dependent code
 *	restructuring.
 *
 *	The Generic machine check structure has been moved to 
 *	cpu.h
 *
 * 20 Jan 86 -- pmk
 *	Add getpammdata() for 8600.
 *	Add memory rountines for errlog and changed memerr().
 *	Add logmck() to mackinecheck() for errlog.
 *	Add rundown flag to boot() stop recursive panics.
 *
 * 02 Jan 86 -- darrell
 *	Removed 8600 memory array callout.  
 *
 *  2-jan-86 -- rjl
 *	Fixed single user kit login limits for uVAXen
 *
 * 23-sep-85 -- jaw
 *	fixed microdelay hang bug.
 *	VAX8200 must do "halt" on reboot system call so the message
 *	buffer remains in memory.
 *
 * 09 Sep 85 -- Reilly
 *	Modified to handle the new 4.3BSD namei code and lockf call.
 *
 * 15 Aug 85 -- darrell
 *	Removed a debug printf I left in, and fixed a spelling error.
 *
 * 26-jul-85 -- jaw
 *	fixed SRM violation....TBIA data MBZ.	
 *
 * 03-Jul-85 -- rjl
 *	Fixed calculation of machinecheck number for uVAX-II checks in
 *	the range of 80-83
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 06-Jun-85 -- jaw
 *	cleanup for 8200.
 *
 * 18-May-85 -- rjl
 *	Fixed dump code so that the uVAX-II would reboot after a dump
 *	attempt.
 *
 *  5-May-5  - Larry Cohen
 *	decrease number of clists to reflect larger cblocks
 *
 * 13-Mar-85 -jaw
 *	Changes for support of the VAX8200 were merged in.
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 * 29-Oct-84 tresvik
 *
 *	added recover from cache parity errors on the 11/750.
 *	added cache-on and cache-off functionality for machinecheck
 *	handling
 *	added timer to TB parity errors for 11/750.
 *	modified MAXUSERS to maxusers for MVAX for rjl
 *
 * 22-Feb-84 tresvik
 *
 *	fixed distributed bug in detecting 11/750 tb parity errors.
 *
 *	changed all printfs used to report memory failures to mprint's.
 *	added code to display on the console any hard memory failures
 *	detected after machine checks.
 *
 * In the beginning - tresvik
 *
 *	changed format of and corrected machine check logging.
 *	Corrections to the 730 list were made and addition of 750
 *	summary list.
 *
 *	added MS780E support.
 *
 *	changed format of memerr reporting for corrected ecc errors.
 *	This includes failing array call out.
 *
 *  9 Jan 84 --jmcg
 *	Need to alter validation of ptes for buffers to match new
 *	allocation scheme in binit.
 *
 *  2 Jan 84 --jmcg
 *	Added support for MicroVAX 1.
 *
 ************************************************************************/

/*	machdep.c	6.1	83/08/20	*/

#include "mba.h"

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"

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
#include "../h/clist.h"
#include "../h/callout.h"
#include "../h/cmap.h"
#include "../h/quota.h"
#include "../h/flock.h"
#include "../h/cpudata.h"
#include "../../machine/common/cpuconf.h"
#include "../h/dump.h"
#include "../h/errlog.h"
#include "../h/socket.h"  /* 8.9.88.us  Support for nonsymm net devices */
#include "../net/net/if.h"    /* 8.9.88.us  Support for nonsymm net devices */
#include "../fs/ufs/fs.h"
#include "../h/fs_types.h"

#include "../machine/cons.h"
#include "../machine/cpu.h"
#include "../machine/mem.h"
#include "../machine/mtpr.h"
#include "../machine/scb.h"
#include "../machine/clock.h"
#include "../machine/rpb.h"
#include "../machine/nexus.h"
#include "../machine/ioa.h"
#include "../machine/vmparam.h" 
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../machine/cvax.h"
#include "../machine/ka650.h"
#include "../machine/ka6200.h"
#include "../machine/ka6400.h"
#include "../io/mba/vax/mbavar.h"
#include "../machine/sas/vmb.h"
#include "../h/kmalloc.h"
#include "../machine/ka60.h"
#ifdef vax
#include "../machine/vectors.h"
#endif

struct lock_t lk_printf;

extern struct kmemusage *kmemusage;
extern kmeminit();
struct cpusw *cpuswitch_entry();
extern int	sysptsize;

int	icode[] = {

	0x9f19af9f,				/* pushab [&"init",0]; pushab */
	0x02dd09af,				/* "/bin/init"; pushl $2 */
	0xbc5c5ed0,				/* movl sp,ap; chmk */
	0x2ffe110b,				/* $exec; brb .; "/ */
	0x2f6e6962,				/* bin/ */
	0x74696e69,				/* init" */
	0x00000000,				/* \0\0\0";  0 */
	0x00000014,				/* [&"init", */
	0x00000000,				/* 0] */
};

int	szicode = sizeof (icode);

/* These global variables must be in data space */
caddr_t endofmem = 0;			/* last kernel virtual mem addr */
struct cpusw *cpup;			/* pointer to cpusw entry */


/*
 * Declare these as initialized data so we can patch them.
 */
int	nbuf = 0;
int	nswbuf = 0;
int	bufpages = 0;
int	Physmem = 0;

int	bufdebug = 0;
int	kperbuf = 0;	/* one buffer header for every kperbuf K of cache */

/*
 * Set up when using the VMB boot path for CI support and dump support
 */
int	*ci_ucode = 0;		/* If present, points to CI ucode */
int	ci_ucodesz = 0;		/* If present, size of CI ucode */
int	*vmbinfo = 0;		/* gets a physical address set in locore
				   which is passed in by the VMB boot path */

/*
 * Machine-dependent startup code
 */
startup (firstaddr)
int	firstaddr;
{
	register int	unixsize;
	register unsigned   i;
	struct pte *pte;
	int	mapaddr, j;
	register caddr_t v;
	int	maxbufs, base, residual;
	extern	char etext;
	int savefirstaddr;
	int vmbinfosz;
	int	requestpages, maxbufpages, tenpercent;
	int reducenbufs;
	char *screen_storage;

	int *ip;
	struct cpudata *pcpu;
	char *nxv;

	extern int nproc;
	extern int ntext;
	extern int ngnode;
	extern int nfile;
	extern int nclist;
	extern int ncallout;
	extern int nchrout;
	extern int nport;
	extern int maxusers;
	extern int appendflg;
#ifdef QUOTA
	extern int nquota;
	extern int ndquot;
#endif QUOTA
	extern long usrptsize;
	extern	int bufcache;	/* % of memory used for buffer cache data */

	int tries = 0;	/* number of times we have tried to reconfigure */
#ifndef UPGRADE
#define UPGRADE 0
#endif
	char x[UPGRADE+1];

#ifdef INET
#define NETSLOP 20			/* for all the lousy servers*/
#else
#define NETSLOP 0
#endif
	/* bare bones initialization of cpudata */

	if (cpu==VAX_6200) 
		ka6200mapcsr();
	if (cpu==VAX_6400) 
		ka6400mapcsr();
	if (cpu==VAX_9000)
		ka9000mapcsr();

	/* initialize this processor as boot cpu */
	init_boot_cpu();
	/* initialize the pointer to the cpusw entry */
	cpup = cpuswitch_entry(cpu);

/*
 * minimum free memory when leaving startup()
 */
#define MINMEM_FREE	400	/* make it big enough for two /bin/csh's */


	/*  
	 *  At this point the IPL MUST be 1f (hex).   The following
         *  code is intended to clean up any interrupts between 
         *  ipl 16 and 1f.  Note that the printf rountine sets the ipl
	 *  to 16.	
	 */
	if ((cpu == VAX_8800) || (cpu == VAX_8820)) 
		mtpr(CLRTOSTS,1);

	/*
	 * If we used adb on the kernel and set "Physmem" to some
	 *   value, then use that value for "physmem".
	 * This allows us to run in less physical memory than the
	 *   machine really has. (rr's idea).
	 */
	if (Physmem >= MINMEM_PGS) {
		physmem = Physmem;
	}
	/*
	 * Save the real amount of memory (or the above artificial
	 * amount) to allow the sizer to accurately configure physmem
	 * in the config file.  `Physmem' was originally added for the
	 * sizer utility - tomt
	 */
	Physmem = physmem;
	usrptsize = (int)(eUsrptmap - Usrptmap);

	savefirstaddr = firstaddr;

	nchrout = 4096;

if (bufdebug) {	/*
		 * recompute these when under the debug flag
		 * so that maxusers can be patched and the
		 * resulting table sizes will change
		 */
		ntext = 24 + maxusers + NETSLOP;
		ngnode = nproc + maxusers + ntext + 48;
		nfile = 16 * (nproc + 16 + maxusers) / 10  + 2 * NETSLOP + 32;
		nclist = 75 + 16 * maxusers;
		ncallout = 16 + nproc;
		nport = nproc / 2;
#ifdef QUOTA
		nquota = (maxusers * 9)/7 + 3;
		ndquot = (maxusers*NMOUNT)/4 + nproc;
#endif
}

tryagain:

	/*
	 * Initialize system tables and data structures
	 */

	firstaddr = savefirstaddr;

	/* The first time through the following
	 * variables should be the same value as determined
	 * in param.c .
	 * If the system page table is too small for the available
	 * physical memory, the physical memory (physmem) is artifically
	 * reduced to MINMEM_MB megabytes.
	 * This should allow booting generic kernels on systems with
	 * large physical memory.
	 * After booting successfully more accurate configuration parameters
	 * should be specified in the config file and the system rebooted.
	 */
	if(tries != 0) {
		
		nproc = 20 + 8 * maxusers;
		ntext = 24 + maxusers + NETSLOP;
		ngnode = nproc + maxusers + ntext + 48;
		nfile = 16 * (nproc + 16 + maxusers) / 10  + 2 * NETSLOP + 32;
		nclist = 75 + 16 * maxusers;
		ncallout = 16 + nproc;
		nport = nproc / 2;
#ifdef QUOTA
		nquota = (maxusers * 9)/7 + 3;
		ndquot = (maxusers*NMOUNT)/4 + nproc;
#endif
	}
	
	/*
	 * Initialize vmb information (at end of core).
	 * Size the information, adjust maxmem accordingly and map it into
	 * the kernel.  Flush the translation buffer.
	 */

	/*
	 * let maxmem be real memory as declared in locore
	 * until vmb_info is mapped in
	 */
	freemem=physmem;		/* should be the same at this point */
	if (vmbinfo && !tries) {	/* if VMB boot path and not a retry */
		vmbinfosz = maxmem - btop(vmbinfo);
		maxmem -= vmbinfosz;
		/*
		 * Now, make sure we won't try to map more pages than we
		 * have ptes for.
		 */
		if (vmbinfosz > VMBINFOPAGES)
			vmbinfosz = VMBINFOPAGES;
		pte = vmbinfomap;
		for (i=0; i<vmbinfosz; i++)
			*(int *) pte++ = PG_V | PG_KW | (maxmem + i);
		mtpr (TBIA,0);
		if (vmb_info.ciucodebas && vmb_info.ciucodesiz) {
			ci_ucode = (int *)&vmb_info +
				(vmb_info.ciucodebas - vmbinfo);
			ci_ucodesz = vmb_info.ciucodesiz;
		}
	}
	/* 
	 * If physmem has been artificially reduced, set maxmem to be
	 * the same.  Otherwise, don't touch it.
	 */
	if (maxmem > physmem)		
		maxmem = physmem;

 /*
  * Good {morning,afternoon,evening,night}.
  */
#if defined (MVAX) || defined (VAX3600) || defined (VAX420) || defined (VAX60)
	/*
	 * Setup the virtual console. vcons_init is an array of pointers
	 * to initialization functions in the qv/qdss & sm/sg drivers. They 
	 * return true if they could setup as the console.  This loop
	 * stops at the end of the list or when it finds a driver that
	 * succeeds in setting up the screen. Precidence is determined by
	 * order as is everything in life.
	 */
	if (cpu == MVAX_I || cpu == MVAX_II || cpu == VAXSTAR ||
		cpu == VAX_3600  || cpu == VAX_3400 || cpu == VAX_3900 ||
		cpu == C_VAXSTAR) {
		extern (*vcons_init[])();

		for( i = 0 ; vcons_init[i] && (*vcons_init[i])() == 0 ; i++ )
			;
	}
	/*
	 * Firefox only...
	 */
	if (cpu == VAX_60) 
		fccons_init();

#endif MVAX || VAX3600 || VAX420 || VAX60


	/*
	 * Allocate space for system data structures.
	 * The first available real memory address is in "firstaddr".
	 * As pages of memory are allocated, "firstaddr" is incremented.
	 * The first available kernel virtual address is in "v".
	 * As pages of kernel virtual memory are allocated, "v" is incremented.
	 * An index into the kernel page table corresponding to the
	 * virtual memory address maintained in "v" is kept in "mapaddr".
	 */
	mapaddr = firstaddr;
	v = (caddr_t) (0x80000000 | (firstaddr * NBPG));
#define valloc(name, type, num) \
		(name) = (type *)(v); (v) = (caddr_t)((name)+(num))
#define valloclim(name, type, num, lim) \
		(name) = (type *)(v); (v) = (caddr_t)((lim) = ((name)+(num)))
	valloclim (gnode, struct gnode , ngnode, gnodeNGNODE);
	valloclim (file, struct file   , nfile, fileNFILE);
	valloclim (proc, struct proc   , nproc, procNPROC);
	valloclim (text, struct text   , ntext, textNTEXT);
	valloc (cfree, struct cblock   , nclist);
	valloc (callout, struct callout, ncallout);
	valloc (chrout, struct chrout, nchrout);
	valloc (swapmap, struct map, nswapmap = nproc * 2);
	valloc (kernelmap, struct map  , nproc+32);
	valloc (nch, struct nch, nchsize);
 	valloc (flox, struct filock, flckinfo.recs );
 	valloc (flinotab, struct flino, flckinfo.fils );
	valloc (kmemmap, struct map, (ekmempt - kmempt) - km_wmapsize);
	valloc (kmemwmap, struct map, km_wmapsize);
	valloc (kmemusage, struct kmemusage, (ekmempt - kmempt) / CLSIZE);

	/* allocate space for gateway screen freelist */
	valloc(screen_storage, char, screen_space_needed());

#ifdef QUOTA
	valloclim (quota, struct quota , nquota, quotaNQUOTA);
	valloclim (dquot, struct dquot , ndquot, dquotNDQUOT);
#endif QUOTA
	/*
	 * Determine how many buffers to allocate, unless this has
	 * already been patched into the kernel.
	 *
	 * Use bufcache% (a config'able option) of memory.
	 * If bufpgs > (physmem - min mem reserved for system)
	 * then set bufpgs to (physmem - min mem reserved for system).
	 * Ensure that bufpages is at least 10% of memory.
	 *
	 * We allocate 1/2 as many swap buffer headers (nswbuf)
	 * as file i/o buffers (nbuf), but never more than 256.
	 */
	if (bufpages == 0) {

		/*
		 * requestpages, maxbufpages, tenpercent are measured 
		 * in NBPG byte units.
		 *
		 * requestpages is what was requested in the config file.
		 * maxbufpages is the upper limit of what is allowed.
		 * tenpercent is the lower limit of what is allowed.
		 *
		 * As silly as this sounds, the upper limit can be lower
		 * than the lower limit (even negative on minimum
		 * configuration machines), so make sure that the upper
		 * limit is >= tenpercent
		 *
		 * Since we only have MINMEM_PGS to guide us, and
		 * since this number is only meaningful for kernels
		 * with relatively small system table sizes, scale
		 * up the estimate of the demands that these tables
		 * make on memory as a function of maxusers when
		 * computing the upper limit.
		 * Allow for the buffer headers as well.
		 *
		 */
		requestpages = (physmem * ((float)bufcache/100));
		maxbufpages = physmem
			- MINMEM_PGS
			- (maxusers/32) * 1024	/* .5 MB per 32 users */
			- (sysptsize * sizeof(struct pte)) / NBPG
			- 1;
		/*
		 * kperbuf is set to 4 for
		 * uniprocessors, and 8 (no moving of physical
		 * memory under a virtual address) for multi-processors.
		 * Sanity test kperbuf on multiprocessors to
		 * guarantee that buffer memory is not moved around.
		 */
		if (!kperbuf) /* hasn't been manually patched */
			kperbuf = ((smp) ? 8 : 4);
		if (smp && kperbuf != 8)
			panic("buffer header allocation failure");
		maxbufs = maxbufpages / CLSIZE / kperbuf;
		maxbufpages -= (maxbufs * sizeof(struct buf)) / NBPG;
		tenpercent = physmem / 10;
		if (maxbufpages < tenpercent)
			maxbufpages = tenpercent;

if (bufdebug) {
printf("startup: physmem %d bufcache %d requestpages %d\n",
       physmem, bufcache, requestpages);
printf("startup: kperbuf %d maxbufs %d maxbufpages %d tenpercent %d\n",
       kperbuf, maxbufs, maxbufpages, tenpercent);
}

		if (requestpages > maxbufpages) {

if (bufdebug) {
printf("startup: bufcache request of %d bytes reduced to %d bytes\n",
       requestpages*NBPG, maxbufpages*NBPG);
}

			requestpages = maxbufpages;
		}

		/* bufpages is measured in page cluster units */
		bufpages = requestpages / CLSIZE;
	}			

	if (nbuf == 0) {
		/* nbuf is # of kperbuf-K objects that can fit in bufpages */
		nbuf = bufpages / kperbuf;
	}

	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) &~ 1;	/* force even */
		if (nswbuf > 256)
			nswbuf = 256;		/* sanity */
	}

if (bufdebug) {
printf("startup: bufpages %d nbuf %d nswbuf %d\n",
	bufpages, nbuf, nswbuf);
printf("startup: valloc swbuf v = 0x%x nswbuf %d\n", v, nswbuf);
}

	valloc(swbuf, struct buf, nswbuf);
	/*
	 * Now the amount of virtual memory remaining for buffers
	 * can be calculated, estimating needs for the cmap.
	 */
	ncmap = (maxmem*NBPG - ((int)v &~ 0x80000000)) /
		(CLBYTES + sizeof(struct cmap)) + 2;

if (bufdebug) {
printf("startup: maxmem %d NBPG %d v 0x%x CLBYTES %d sizeof(cmap) %d\n",
       maxmem, NBPG, v, CLBYTES, sizeof(struct cmap));
printf("startup: ncmap %d = (%d / %d) + 2\n",
       ncmap, (maxmem*NBPG - ((int)v &~ 0x80000000)),
       (CLBYTES + sizeof(struct cmap)) );
}

	maxbufs = ((sysptsize * NBPG) -
	    ((int)(v + ncmap * sizeof(struct cmap)) - 0x80000000)) /
		(MAXBSIZE + sizeof(struct buf));

if (bufdebug) {
printf("startup: sysptsize %d ncmap %d sizeof(cmap) %d\n",
	sysptsize, ncmap, sizeof(struct cmap));
printf("startup: ((0x%x + 0x%x) - 0x80000000) / 0x%x\n",
	v, ncmap*sizeof(struct cmap), sizeof(struct buf));
printf("startup: maxbufs %d\n", maxbufs);
}

	/*
	 * If the system page table is too small for the available
	 * physical memory (not enough room for nbufs after the core map)
	 * then the size of physical memory (physmem) is artifically
	 * reduced to MINMEM_PGS (the minimun amount supported).
	 * This should allow booting generic kernels on systems with
	 * large physical memory.
	 */
	if (maxbufs < 16) {
		/* 
		 * Let's not print this anymore. It scares the user and they
		 * complain about it alot.  They will be told in a less
		 * frightful way later, where 'real mem' is displayed.
		 */

if (bufdebug) {
printf("startup: system page table too small, reducing physmem to %d meg\n",MINMEM_MB);
}

		if (++tries > 1)
			panic ("sys pt too small");
		physmem=MINMEM_PGS; /* # of 512 byte pages = MINMEM_MB */
		maxusers=(maxusers > 8) ? 8 : maxusers;
		nbuf=bufpages=nswbuf=0;
		goto tryagain;
	}

	reducenbufs = 0;
	if (nbuf > maxbufs) {
if (bufdebug) {
printf("startup: sysptsize limits number of buffers to %d\n", maxbufs);
}
		nbuf = maxbufs;
		reducenbufs = 1;
	}

	if (bufpages > nbuf * (MAXBSIZE / CLBYTES)) {
		bufpages = nbuf * (MAXBSIZE / CLBYTES);

if (bufdebug) {
printf("startup: bufpages > than needed, reduced to %d\n",
       bufpages);
}

	}

if (bufdebug) {
printf("startup: valloc buf, nbuf %d(%d) bufpages %d v 0x%x\n",
	nbuf, sizeof(struct buf), bufpages, v);
}

	valloc(buf, struct buf, nbuf);
	/*
	 * Allocate space for core map.
	 * Allow space for all of phsical memory minus the amount 
	 * dedicated to the system. The amount of physical memory
	 * dedicated to the system is the total virtual memory of
	 * the system thus far, plus core map, buffer pages,
	 * and buffer headers not yet allocated.
	 * Add 2: 1 because the 0th entry is unused, 1 for rounding.
	 */
	ncmap = (maxmem*NBPG - ((int)(v + bufpages*CLBYTES) &~ 0x80000000)) /
		(CLBYTES + sizeof(struct cmap)) + 2;

if (bufdebug) {
printf("startup: maxmem %d bufpages %d v 0x%x\n",
       maxmem, bufpages, v);
printf("startup: ncmap %d = (%d / %d) + 2\n",
       ncmap, (maxmem*NBPG - ((int)(v + bufpages*CLBYTES) &~ 0x80000000)), 
       (CLBYTES + sizeof(struct cmap)) );
}

	if (ncmap <= 0) {
		if (++tries > 1)
			panic ("no memory (A)");
		printf("not enough memory after allocating buffer cache!\n");
		if (bufcache > 10) {
			bufcache = max(bufcache / 2, 10);
			printf("bufcache reduced to %d percent\n", bufcache);
		} else {
			maxusers = max(maxusers / 2, 2);
			printf("maxusers reduced to %d\n", maxusers);
		}
		printf("consider re-sizing kernel configuration parameters\n");
		nbuf=bufpages=nswbuf=0;
		goto tryagain;
	}

	valloclim(cmap, struct cmap, ncmap, ecmap);

	/*
	 * Clear space allocated thus far, and make r/w entries
	 * for the space in the kernel map.
	 */
	unixsize = btoc((int)v &~ 0x80000000);
	while (firstaddr < unixsize) {
		*(int *)(&Sysmap[firstaddr]) = PG_V | PG_KW | firstaddr;

		bzero(0x80000000 | (ctob(firstaddr)),NBPG);
		firstaddr++;
	}

	/* Save PFN of last kernel page to dump for crashdump code */
	lastkpage = firstaddr;

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 * Set endofmem to the last used kernel virtual address.
	 */
	v = (caddr_t) ((int)(v + PGOFSET) &~ PGOFSET);

if (bufdebug) {
printf("startup: valloc buffers, nbuf %d MAXBSIZE %d v 0x%x\n",
	nbuf, MAXBSIZE, v);
}

	valloc(buffers, char, MAXBSIZE * nbuf);
	endofmem = v;

if (bufdebug) {
printf("startup: endofmem = 0x%x\n", endofmem);
}

	base = bufpages / nbuf;	/* clusters of memory per buffer header */
	residual = bufpages % nbuf;

if (bufdebug) {
printf("startup: base (K/buf) = %d, residual = %d\n", base, residual);
}

	mapaddr = firstaddr;
	for (i = 0; i < residual; i++) {
		for (j = 0; j < (base + 1) * CLSIZE; j++) {
			*(int *)(&Sysmap[mapaddr+j]) = PG_V | PG_KW | firstaddr;
			bzero(0x80000000 | (ctob(mapaddr)),NBPG);
			firstaddr++;
		}
		mapaddr += MAXBSIZE / NBPG;
	}

	for (i = residual; i < nbuf; i++) {
		for (j = 0; j < base * CLSIZE; j++) {
			*(int *)(&Sysmap[mapaddr+j]) = PG_V | PG_KW | firstaddr;
			bzero(0x80000000 | (ctob(mapaddr)),NBPG);
			firstaddr++;
		}
		mapaddr += MAXBSIZE / NBPG;
	}
if (bufdebug) {
printf("startup: end(kernel real mem) %d, end(kernel virtual mem) %d\n",
	firstaddr*NBPG, ((int)endofmem & ~ 0x80000000));
printf("startup: sys virt pgs %d, sys real pgs %d\n",
   	((int)(v + 1) & ~0x80000000) / NBPG, firstaddr*NBPG);
printf("startup: # of sys ptes %d, # of sys ptes used %d\n",
        sysptsize, mapaddr);
}

	if (firstaddr >= physmem - MINMEM_FREE)
		panic ("no memory (B)");
	mtpr (TBIA, 0);

 /*
  * Initialize callouts
  */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i - 1].c_next = &callout[i];
 /*
  * Initialize interrupt queue
  */
	chrfree = chrcur = &chrout[0];
	for (i = 1; i < nchrout; i++)
		chrout[i - 1].c_next = &chrout[i];
	chrout[nchrout - 1].c_next = &chrout[0]; /* circular llist */

 /*
  * Initialize gateway screen freelist
  */
	screen_init_freelist(screen_storage);

 /*
  * Initialize memory allocator and swap
  * and user page table maps.
  *
  * THE USER PAGE TABLE MAP IS CALLED ``kernelmap''
  * WHICH IS A VERY UNDESCRIPTIVE AND INCONSISTENT NAME.
  */
	meminit (firstaddr, maxmem);
	maxmem = freemem;


	kmeminit();	
	rminit (kernelmap, (long)usrptsize, (long) 1, "usrpt", nproc+32);
 /*  
  * Log startup printfs
  */

	appendflg = 1; 	/* tell error logger we're starting up */
	printf (version);
	printf ("real mem  = %d\n", ctob (Physmem));
	if (tries  &&  Physmem != physmem) { /* we've reconfigured because of a small SPT */
		printf("Memory configuration adjusted to run with small system page table\n");
		printf ("real mem  = %d\n", ctob (physmem));
	}
	printf ("avail mem = %d\n", ctob (maxmem));

	if (reducenbufs) { /* # of buffer headers was limited by sysptsize */
		printf("Buffer configuration adjusted to run with small system page table\n");
	}

	printf ("using %d buffers containing %d bytes of memory\n",
		nbuf, bufpages * CLBYTES);


	/* allocate 1 pte for each proc structure to be used to 
	   double map the PCB in the u_area */
 	nxv = (char *)get_sys_ptes(nproc, &pte);
	/* put virtual address for double map in the proc structure */
	for(i=0;i<nproc; i++) {
		proc[i].p_pcb =(struct pcb *)((int)  nxv + (i*512));
	}

 /*
  * Clear restart inhibit flags.
  */

	switch(cpu) {

#ifdef VAX9000
		case VAX_9000:
			ka9000_clear_coldstart();
			ka9000_clear_warmstart();
			break;
#endif VAX9000

#ifdef VAX8800
		case VAX_8800:
		case VAX_8820:
			cons_putc (N_COMM|N_CLR_COLD);
			cons_putc (N_COMM|N_CLR_WARM);
			break;
#endif VAX8800

		default:
			cons_putc (TXDB_CWSI);
			cons_putc (TXDB_CCSI);
			break;
	}
}

/*
 * Enable Cache
 * 
 * The actual routines are entered through cpusw, and are located
 * in the appropiate cpu dependent routine kaXXX.c
 */

cachenbl()
{
	if ((*cpup->cachenbl)() < 0 )
		panic("No cachenbl routine configured\n");
}

#ifdef PGINPROF
/*
 * Return the difference (in microseconds)
 * between the	current time and a previous
 * time as represented	by the arguments.
 * If there is a pending clock interrupt
 * which has not been serviced due to high
 * ipl, return error code.
 */
vmtime (otime, olbolt, oicr)
register int	otime, olbolt, oicr;
{

	if (mfpr (ICCS) & ICCS_INT)
		return (-1);
	else
		return (((time.tv_sec - otime) * 60 + lbolt - olbolt) * 16667 + mfpr (ICR) - oicr);
}
#endif

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by chmk
 * to sigcleanup routine below.  After sigcleanup
 * resets the signal mask and the stack, it
 * returns to user who then unwinds with the
 * rei at the bottom of sigcode.
 */
sendsig (p, sig, mask)
int	(*p) (), sig, mask;
{
	register struct sigcontext *scp;	/* know to be r11 */
	register int   *regs;
	register struct sigframe
		{
		int	sf_signum;
		int	sf_code;
		struct sigcontext  *sf_scp;
		int	(*sf_handler) ();
		struct sigcontext  *sf_scpcopy;
		} *fp;				/* known to be r9 */
	register int	oonstack;

	regs = u.u_ar0;
	oonstack = u.u_onstack;
	scp = (struct sigcontext   *) regs[SP] - 1;
	if (!u.u_onstack && (u.u_sigonstack & sigmask(sig))) {
		fp = (struct sigframe  *) u.u_sigsp - 1;
		u.u_onstack = 1;
	}
	else
		fp = (struct sigframe  *) scp - 1;
 /*
  * Must build signal handler context on stack to be returned to
  * so that rei instruction in sigcode will pop ps and pc
  * off correct stack.	The remainder of the signal state
  * used in calling the handler must be placed on the stack
  * on which the handler is to operate so that the calls
  * in sigcode will save the registers and such correctly.
  */
	if (!oonstack && (int) fp <= USRSTACK - ctob (u.u_ssize))
		grow ((unsigned) fp);
	;
#ifndef lint
	asm ("probew $3,$20,(r9)");
	asm ("jeql bad");
#else
	if (useracc ((caddr_t) fp, sizeof (struct sigframe), 1))
		goto bad;
#endif
	if (!u.u_onstack && (int) scp <= USRSTACK - ctob (u.u_ssize))
		grow ((unsigned) scp);
	;					/* Avoid asm() label botch */
#ifndef lint
	asm ("probew $3,$20,(r11)");
	asm ("beql bad");
#else
	if (useracc ((caddr_t) scp, sizeof (struct sigcontext) , 1))
		goto bad;
#endif
	fp -> sf_signum = sig;
	if (sig == SIGSEGV || sig == SIGBUS || sig == SIGILL || sig == SIGFPE){
		fp -> sf_code = u.u_code;
		u.u_code = 0;
	}
	else
		fp -> sf_code = 0;
	fp -> sf_scp = scp;
	fp -> sf_handler = p;
 /*
  * Duplicate the pointer to the sigcontext structure.
  * This one doesn't get popped by the ret, and is used
  * by sigcleanup to reset the signal state on inward return.
  */
	fp -> sf_scpcopy = scp;
	/* sigcontext goes on previous stack */
	scp -> sc_onstack = oonstack;
	scp -> sc_mask = mask;
	/* setup rei */
	scp -> sc_sp = (int) & scp -> sc_pc;
	scp -> sc_pc = regs[PC];
	scp -> sc_ps = regs[PS];
	regs[SP] = (int) fp;
	regs[PS] &= ~(PSL_CM | PSL_FPD);
	regs[PC] = (int) u.u_pcb.pcb_sigc;
	return;

bad:
	asm ("bad:");
 /*
  * Process has trashed its stack; give it an illegal
  * instruction to halt it in its tracks.
  */
	u.u_signal[SIGILL] = SIG_DFL;
	sig = sigmask(SIGILL);
	u.u_procp -> p_sigignore &= ~sig;
	u.u_procp -> p_sigcatch &= ~sig;
	u.u_procp -> p_sigmask &= ~sig;
	psignal (u.u_procp, SIGILL);
}

/*
 * Routine to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Pop these values in preparation for rei which
 * follows return from this routine.
 */
sigcleanup ()
{
	register struct sigcontext *scp;

	scp = (struct sigcontext   *) fuword ((caddr_t) u.u_ar0[SP]);
	if ((int) scp == -1)
		return;
	;
#ifndef lint
	/* only probe 12 here because that's all we need */
	asm ("prober $3,$12,(r11)");
	asm ("bnequ 1f; ret; 1:");
#else
	if (useracc ((caddr_t) scp, sizeof (*scp), 0))
		return;
#endif
	u.u_onstack = scp -> sc_onstack & 01;
	u.u_procp -> p_sigmask =
	scp -> sc_mask & ~(sigmask(SIGKILL)|sigmask(SIGSTOP));
	u.u_ar0[SP] = scp -> sc_sp;
}

#ifdef notdef
dorti ()
{
	struct frame	frame;
	register int	sp;
	register int	reg, mask;
	extern int  ipcreg[];

	(void) copyin ((caddr_t) u.u_ar0[FP], (caddr_t) & frame, sizeof (frame));
	sp = u.u_ar0[FP] + sizeof (frame);
	u.u_ar0[PC] = frame.fr_savpc;
	u.u_ar0[FP] = frame.fr_savfp;
	u.u_ar0[AP] = frame.fr_savap;
	mask = frame.fr_mask;
	for (reg = 0; reg <= 11; reg++) {
		if (mask & 1) {
			u.u_ar0[ipcreg[reg]] = fuword ((caddr_t) sp);
			sp += 4;
		}
		mask >>= 1;
	}
	sp += frame.fr_spa;
	u.u_ar0[PS] = (u.u_ar0[PS] & 0xffff0000) | frame.fr_psw;
	if (frame.fr_s)
		sp += 4 + 4 * (fuword ((caddr_t) sp) & 0xff);
	/* phew, now the rei */
	u.u_ar0[PC] = fuword ((caddr_t) sp);
	sp += 4;
	u.u_ar0[PS] = fuword ((caddr_t) sp);
	sp += 4;
	u.u_ar0[PS] |= PSL_USERSET;
	u.u_ar0[PS] &= ~PSL_USERCLR;
	u.u_ar0[SP] = (int) sp;
}
#endif

/*
 * this routine sets the cache to the state passed.  enabled/disabled
 * 
 * The actual routines are entered through cpusw, and are located
 * in the appropiate cpu dependent routine kaXXX.c
 */

setcache(state)
int state;
{
	if ((*cpup->setcache)(state) < 0 )
		panic("No setcache routine configured\n");
}


/*
 * Memenable enables the memory controller corrected data reporting.
 * This runs at regular intervals, turning on the interrupt.
 * The interrupt is turned off, per memory controller, when error
 * reporting occurs.  Thus we report at most once per memintvl.
 * 
 * The actual routines are entered through cpusw, and are located
 * in the appropiate cpu dependent routine kaXXX.c
 */
int	memintvl = MEMINTVL;

timer_action ()
{
	if ((*cpup->timer_action)() < 0 )
		panic("No timer_action routine configured\n");
	else
	if (memintvl > 0)
		timeout (timer_action, (caddr_t) 0, memintvl * hz);
}

/*
 * Memerr is the interrupt routine for corrected read data
 * interrupts.	It calls the apporpriate routine which looks
 * to see which memory controllers have unreported errors,
 * reports them, and disables further reporting for a time
 * on those controller.
 * 
 * The actual routines are entered through cpusw, and are located
 * in the appropiate cpu dependent routine kaXXX.c
 */
memerr ()
{
	if ((*cpup->softerr_intr)() < 0 )
		panic("No softerr_intr handler configured\n");
}

/*
 * Invalidate single all pte's in a cluster
 */
tbiscl (v)
unsigned	v;
{
	register	caddr_t addr;		/* must be first reg var */
	register int	i;

	asm (".set TBIS,58");

#ifdef vax
	/* Quiesce vector processor if necessary */
	VPSYNC();
#endif vax

	addr = ptob (v);
	for (i = 0; i < CLSIZE; i++) {
#ifdef lint
		mtpr (TBIS, addr);
#else
		asm ("mtpr r11,$TBIS");
#endif
		addr += NBPG;
	}
	tbsync();
}

int	waittime = -1;
int	shutting_down = 0;

boot(paniced, arghowto)
int	paniced, arghowto;
{
	register int	howto;			/* r11 == how to boot */
	register int	devtype;		/* r10 == major of root dev */
	register struct mount *mp;
	register struct gnode *gp;
	struct gnode *rgp;
	extern struct gnode *fref();
	extern struct gnode *acctp;
	extern struct gnode *savacctp;
	extern struct cred *acctcred;
	extern struct lock_t lk_acct;
	extern void ( *scs_disable )();
	void ( *disable )();
	int s;
#ifdef lint
	howto = 0;
	devtype = 0;
	printf ("howto %d, devtype %d\n", arghowto, devtype);
#endif
	howto = arghowto;
	rundown++;
	shutting_down++;

	if ((howto & RB_NOSYNC) == 0 && waittime < 0 && bfreelist[0].b_forw) {
		/*
		 * If accounting is on, turn it off. This allows the usr
		 * filesystem to be umounted cleanly.
		 */
		smp_lock(&lk_acct, LK_RETRY);
		if (savacctp) {
			acctp = savacctp;
			savacctp = NULL;
		}
		if (gp = acctp) {
		        gfs_lock(gp);
			if (acctp != NULL) {
			        acctp = NULL;
				GSYNCG(gp, acctcred);
				crfree(acctcred);
				acctcred = NULL;
				gput(gp);
			} else
			        gfs_unlock(gp);
		}
		smp_unlock(&lk_acct);

		waittime = 0;
		(void) spl4 ();

		gfs_gupdat(NODEV);
		if (paniced != RB_PANIC) {
			update(); /* flush dirty blocks */
			/* unmount all but root fs */
			/* include paranoid checks for active unmounts */
			/* and preclude new unmounts */
			for (mp = &mount[NMOUNT - 1]; mp > mount; mp--) { 
				smp_lock(&mp->m_lk, LK_RETRY);
				if ((mp->m_flgs & MTE_DONE) &&
				    !(mp->m_flgs & MTE_UMOUNT)) {
					mp->m_flgs |= MTE_UMOUNT;
					smp_unlock(&mp->m_lk);
					GUMOUNT(mp, 1);
				}
				else {
					smp_unlock(&mp->m_lk);
				}
			}
		}

		printf ("syncing disks... ");
		/* 
		 * spl5 because we don't want to have a user program
		 * scheduled
		 */
		s = spl5();
		bflush (NODEV, (struct gnode *) 0, 1); 
		splx(s);
		printf ("done\n");
	}

	/*
	 * Optionally invoke the SCS shutdown routine to disable all local
	 * ports.
	 */
	if(( disable = scs_disable )) {
		u_long	save_ipl = splextreme();
		(void)(*disable)();
		(void)splx( save_ipl );
	}
#if defined(VAX8600)
 	if(cpu == VAX_8600) {
 		register int i;
 		int *ip;
 		struct sbia_regs *sbiad;
 		extern int ioanum;
 		extern char Sysbase[];
 		sbiad = (struct sbia_regs *)ioa;
 		ip = (int *)Sysmap+1; *ip &= ~PG_PROT; *ip |= PG_KW;
 		mtpr(TBIS, Sysbase);	
 		for(i=0; i<ioanum; i++) {
 			if(BADADDR((caddr_t)sbiad, 4))
 				continue;
 			sbiad->sbi_unjam = 0;
 			sbiad = (struct sbia_regs *)((char *)sbiad + 
 				cpup->pc_ioasize);
 		}
 		ip = (int *)Sysmap+1; *ip &= ~PG_PROT; *ip |= PG_KR;
 		mtpr(TBIS, Sysbase);	
 	}
#endif VAX8600
	splx (0x1f);				/* extreme priority */
	devtype = major (rootdev);
	if (howto & RB_HALT) {
		mtpr (IPL, 0x1f);
#if defined (MVAX) || defined (VAX420)
		if( cpu == MVAX_II || cpu == VAXSTAR || cpu == C_VAXSTAR ) {
			if(cpu_subtype == ST_MVAXII)
			    ((struct qb_regs *)nexus)->qb_cpmbx = RB_HALTMD;
			if(cpu_subtype == ST_VAXSTAR)
			    ((struct nb_regs *)nexus)->nb_cpmbx = RB_VS_HALTMD;
			for (;;)
			asm ("halt");
		}
#endif MVAX || VAX420
#ifdef VAX6200
		if (cpu == VAX_6200)
			ka6200halt();
#endif VAX6200
#ifdef VAX6400
		if (cpu == VAX_6400)
			ka6400halt();
#endif VAX6400
#ifdef VAX9000
		if (cpu == VAX_9000)
			ka9000halt();
#endif VAX9000

#if defined (VAX3600) || defined (VAX60)
		if (cpu == VAX_3600 || cpu == VAX_3400 ||
			cpu == VAX_3900 || cpu == VAX_60) {
			cvqssc->ssc_cpmbx = RB_CV_HALTMD;
			for (;;)
				asm ("halt");
		}
#endif VAX3600 || VAX60
		/* halt the slaves please */
		if (cpu == VAX_8200) ka820slavehalt();
		printf ("\nTHE PROCESSOR CAN NOW BE HALTED.\n");
		for (;;);
	} else {
		if (paniced == RB_PANIC) {
			doadump ();		/* TXDB_BOOT's itsself */
			/* NOTREACHED */
		}
#ifdef VAX6200
		if (cpu==VAX_6200)
			ka6200reboot();
#endif VAX6200
#ifdef VAX6400
		if (cpu==VAX_6400)
			ka6400reboot();
#endif VAX6400
#ifdef VAX9000
		if (cpu==VAX_9000)
			ka9000reboot();
#endif VAX9000
#ifdef VAX8800
		if ((cpu == VAX_8800) || (cpu == VAX_8820)) {
			cons_putc(N_COMM | N_BOOT_ME);
			asm("halt");
		}
#endif
		cons_putc (TXDB_BOOT);
	}
#if defined(VAX750) || defined(VAX730) || defined(MVAX) || defined(VAX8200) || defined(VAX3600) || defined(VAX60) || defined(VAX420) || defined(VAX6200) || defined(VAX6400) || defined(VAX9000)
	if ((cpu != VAX_780) && (cpu != VAX_8600)) {
		asm ("movl r11,r5");
	}					/* boot flags go in r5 */
	if (cpu == MVAX_II || cpu == VAXSTAR || cpu == C_VAXSTAR) {
		if(cpu_subtype == ST_MVAXII)
			((struct qb_regs *)nexus)->qb_cpmbx = RB_REBOOT;
		if(cpu_subtype == ST_VAXSTAR)
			((struct nb_regs *)nexus)->nb_cpmbx = RB_VS_REBOOT;
	}
	if (cpu == VAX_3600 || cpu == VAX_3400 ||
		cpu == VAX_3900 || cpu == VAX_60) {
			cvqssc->ssc_cpmbx = RB_CV_REBOOT;
	}
#endif
	for (;;)
	asm ("halt");
	/* NOTREACHED */
}

cons_putc (c)
{
	if ((*cpup->cons_putc)(c) < 0 )
		panic("No cons_putc routine configured\n");
}

/*
 * Machine check handlers.
 * 
 * The actual routines are entered through cpusw, and are located
 * in the appropiate cpu dependent routine kaXXX.c
 */

machinecheck (cmcf)
caddr_t cmcf;
{
	if ((*cpup->machcheck)(cmcf) < 0 )
		panic("No machine check handler configured\n");
}

/*
 * delay for n microseconds,
 * call through cpu switch to specific delay routine.
 */

microdelay(usecs)
int usecs;
{
	if ((*cpup->microdelay)(usecs) < 0)
		panic("No microdelay routine configured\n");
}

/*
 * delay for n microseconds, limited to somewhat over 2000 microseconds
 * using standard vax ICR.
 */

uICRdelay(n)
int n;
{
	struct timeval et, nowt;
	int saveiccs,s;

	/* if clock not enabled or ipl above 0x17 */
	/* change so if ipl > 0x15.  this was change to VAX SRM */
	if ( !(mfpr(ICCS) & ICCS_RUN) || (mfpr(IPL) >= 0x16)) {
		s=spl6();
		saveiccs = mfpr(ICCS);	/* save value */
		mtpr(NICR, -n); 	/* load neg n */
		mtpr(ICCS, ICCS_RUN+ICCS_TRANS+ICCS_INT+ICCS_ERR);
		while ( !(mfpr(ICCS) & ICCS_INT));	/* wait */

		/* restore interval counter to previous state */
		mtpr(NICR,-1000000/hz);
		mtpr(ICCS, saveiccs+ICCS_TRANS+ICCS_ERR); /*restore*/
		splx(s);
	} else {
		/* clock is running so call mircotime */
		microtime(&et);
		et.tv_sec += n/1000000;
		et.tv_usec += n%1000000;
		if( et.tv_usec > 1000000) {
			et.tv_usec -= 1000000;
			et.tv_sec++;
		}
		do
			microtime(&nowt);
		while ( nowt.tv_sec < et.tv_sec || 
			(nowt.tv_usec < et.tv_usec && nowt.tv_sec <= et.tv_sec));
	}
	return(0);
}

microtime (tvp)
struct timeval *tvp;
{
	int	s = spl6 ();

	tvp -> tv_sec = time.tv_sec;
	tvp -> tv_usec = time.tv_usec + (1000000/hz);
	if ((cpu != VAX_6400) && (cpu != VAX_6200)) 
		tvp -> tv_usec += mfpr(ICR);
	while (tvp -> tv_usec > 1000000) {
		tvp -> tv_sec++;
		tvp -> tv_usec -= 1000000;
	}
	splx (s);
}

/*
 * delay for n microseconds, limited to somewhat over 2000 microseconds
 * using counter for lack of ICR.   "n" set for uVAX I.
 */

uInoICRdelay(n)
int n;
{
	n /= 6;
	while (--n >= 0)
		;			/* wait */
	return(0);
}

/*
 * delay for n microseconds, limited to somewhat over 2000 microseconds
 * using counter for lack of ICR.   "n" set for uVAX II.
 */

uIInoICRdelay(n)
int n;
{
	/*
	 * For VAXstation 2000 (AKA, VAXstar) and MicroVAX 2000 (AKA, TEAMmate),
	 * measurements with 1 second granularity (using TOY seconds register)
	 * show a delay of n = 10000000 (10 sec) yields an actual delay
	 * between 11 and 12 seconds (+ 10 to 20 %). -- Fred Canter 8/30/86
	 */
	n /= 2;
	while (--n >= 0)
		;		/* wait */
	return(0);
}

/*
 * CVAXstar/PVAX/PVAX1 (KA420 processor) microdelay routine.
 *
 * NOTE: DELAY() is called with caches off because configure() is called
 *	 before setcache() in startup() above. This means DELAY()
 *	 must be accurate for all all cache states.
 *
 * The goal is to delay for "n" microseconds. The KA420 CPU
 * does not have a hardware timer, so we must use a software
 * insruction counted loop. The software overhead of this
 * routine increases as the delay decreases (about 20% @ 2000 Usec).
 * CAUTION: this routine is not intended for delays < 2000 microseconds!
 *
 * This routine is somewhat complicated by the fact that the KA420 CPU
 * has 3 clock speeds (100, 90, 60 nanoseconds) and 2 levels of cache
 * with 4 possible combinations of enabled/disabled. The strategy is:
 *
 *	Read the CPU speed from the cache control register (CACR)
 *	and adjust the value of n accordingly.
 *
 *	The value of n is adjusted again if 1st level or both
 *	caches are off.
 *
 *	If the 1st level cache is on, we don't care about the state
 *	of the 2nd level cache (1st level cache masks it).
 *
 *	Here are actual delay times measured on 100ns, 90ns, and
 *	60ns CPUs when a 10 second delay was requested:
 *
 *	100ns - 10.64 seconds - both caches on
 *	 90ns - 10.62 seconds - both caches on
 *	 60ns - 10.33 seconds - both caches on
 *
 *	100ns - 10.69 seconds - 1st level cache on, 2nd level cache off
 *	 90ns - 10.68 seconds - 1st level cache on, 2nd level cache off
 *	 60ns - 10.36 seconds - 1st level cache on, 2nd level cache off
 *
 *	100ns - 10.45 seconds - 1st level cache off, 2nd level cache on
 *	 90ns - 10.47 seconds - 1st level cache off, 2nd level cache on
 *	 60ns - 10.70 seconds - 1st level cache off, 2nd level cache on
 *
 *	100ns - 10.85 seconds - both caches off
 *	 90ns - 10.83 seconds - both caches off
 *	 60ns - 10.42 seconds - both caches off
 *
 */

/*
 * This variable defines the state of the
 * 1st and 2nd level caches, i.e., bit 0 is set
 * if 1st level cache is on and bit 1 is set if
 * 2nd level cache is on. The normal state is
 * both caches on (cvs_cache_on = 3).
 */
int	cvs_cache_on = 3;

extern	int cache_state;
extern  int cpu_sub_subtype;


cVSnoICRdelay(n)
int n;
{
	register struct nb_regs *addr = (struct nb_regs *)nexus;
	register int cacr;
	
	if(cpu_sub_subtype == SB_TMII) { /* TMII uses 90 nanosecond chip
					  * and does not have CACR register */
	    cacr  = 0;			 /* clear cacr */
	    n += (n /3);		 /* add 33% */
	    n += (n / 10);		 /* add 10% more */
	} else {  /* for PVAX */
		cacr = addr->nb_cacr & 0x0c0;
		if (cacr == 0x0c0) {		/* 100 nanosecond CPU */
		    n += (n / 4);		/* add 25% */
		    n += (n / 20);		/* add 5% more */
		}
		else if (cacr == 0x080) {	/*  90 nanosecond CPU */
		    n += (n / 3);		/* add 33% */
		    n += (n / 10);		/* add 10% more */
		}
		else {				/*  60 nanosecond CPU */
		    n += (n / 2);		/* add 50% */
		    n += (n / 5);		/* add 20% more */
		    n += (n / 5);		/* add 20% more */
		}
	}
	if (cache_state == 0) {		/* Caches not enabled yet (probe) */
	    if (cacr == 0x040)		/* 60 nanosecond CPU */
		n /= 4;			/* divide by 4 */
	    else
		n /= 3;			/* divide by 3 */
	    n += (n / 10);		/* add 10% */
	    if (cacr == 0x040)		/* 60 nanosecond CPU */
		n += (n / 20);		/* add 5% */
	}
	else if (cvs_cache_on == 2) {	/* Only 2nd level cache on */
	    n /= 3;			/* divide by 3 */
	    n += (n / 5);		/* add 20% */
	}
	else if (cvs_cache_on == 0) {	/* Both 1st & 2nd level cache off */
	    if (cacr == 0x040)		/* 60 nanosecond CPU */
		n /= 4;			/* divide by 4 */
	    else
		n /= 3;			/* divide by 3 */
	    n += (n / 10);		/* add 10% */
	    if (cacr == 0x040)		/* 60 nanosecond CPU */
		n += (n / 20);		/* add 5% */
	}

	while (--n >= 0)
		;		/* wait */

	return(0);
}

/*
 * delay for n microseconds, limited to somewhat over 2000 microseconds
 * using SSC (CVAX system support chip) programmable timer for lack of ICR.
 */

uSSCdelay(n)
int n;
{
	int s;

	s = spl6();
	cvqssc->ssc_tnir0 = -n; 	/* load neg n */
	cvqssc->ssc_tcr0 = ICCS_RUN+ICCS_TRANS+ICCS_INT+ICCS_ERR+TCR_STP;
	while ( !(cvqssc->ssc_tcr0 & ICCS_INT))
		;			/* wait */
	splx(s);
	return(0);
}

/*
 * delay for n microseconds, limited to somewhat over 2000 microseconds
 * using RSSC (Rigel system support chip) programmable timer for lack of ICR.
 */
#ifdef VAX6400
uRSSCdelay(n)
int n;
{
	int s,caller_ipl;

	caller_ipl = mfpr(IPL);
	if (caller_ipl < 0x18)
		s = splclock();
	rssc->s_tnir0 = -n; 	/* load neg n */
	rssc->s_tcr0 = ICCS_RUN+ICCS_TRANS+ICCS_INT+ICCS_ERR+TCR_STP;
	while ( !(rssc->s_tcr0 & ICCS_INT))
		;			/* wait */
	if (caller_ipl < 0x18)
		splx(s);
	return(0);
}
#endif


physstrat (bp, strat, prio)
register struct buf *bp;
int	(*strat) (), prio;
{
	int	s;

	(*strat) (bp);
	/* pageout daemon doesn't wait for pushed pages or N-buffered */
	if (bp->b_flags & (B_DIRTY|B_RAWASYNC))
		return;
	s = spl6 ();
	while ((bp->b_flags & B_DONE) == 0)
	sleep ((caddr_t) bp, prio);
	splx (s);
}

extern int cold;

badaddr(addr,len) 
caddr_t addr;
int len;
{
	int status,s;
	int *ip;

	if (cold) status=(((*cpup->badaddr)(addr, len)));
	
	else {
 		ip = (int *)Sysmap+ (btop(((int)&scb.scb_stray)&0x7fffffff));
		*ip &= ~PG_PROT; 
		*ip |= PG_KW;
 		mtpr(TBIS, &scb.scb_stray);	

		s=spl7();
		switch(cpu) {

		case VAX_8600:
		case VAX_780:
			ubaclrint(); 
			status=(((*cpup->badaddr)(addr, len)));
			status|=ubasetint(); 
			break;
			
		case VAX_6200:
		case VAX_6400:
		case VAX_8800:
		case VAX_8820:
		case VAX_8200:
		case VAX_9000:
			biclrint();
			status=(((*cpup->badaddr)(addr, len)));
			bisetint();
			if (cpu == VAX_6200)
				ka6200_clear_xbe();
			if (cpu == VAX_6400)
				ka6400_clear_xbe();
			if (cpu == VAX_9000)
				ka9000_clear_xbe();
			break;

		case VAX_60:
			ka60clrmbint();
			status=(((*cpup->badaddr)(addr, len)));
			enafbiclog();
			ka60setmbint();
			break;

		default:
			status=(((*cpup->badaddr)(addr, len)));
			break;
		}


 		*ip &= ~PG_PROT; *ip |= PG_KR;
 		mtpr(TBIS, &scb.scb_stray);	
		splx(s);
	}

	return(status);

}

extern int nNUBA;

ubaclrint()
{
	struct uba_regs *ubap;
	int i;

	for (i=0; i<nNUBA; i++) {
		
		if (ubap = uba_hd[i].uh_uba) 
			ubap->uba_sr=ubap->uba_sr;
	
	}
}


ubasetint()
{
	struct uba_regs *ubap;
	int i,ubaerror;
	
	ubaerror=0;
	for (i=0; i<nNUBA; i++) {
		if (ubap = uba_hd[i].uh_uba) {
		
			if(ubap->uba_sr) ubaerror=1;
			ubap->uba_sr=ubap->uba_sr;
			ubap->uba_cr=	UBACR_IFS | UBACR_BRIE |
					UBACR_USEFIE | UBACR_SUEFIE |
					(ubap->uba_cr & 0x7c000000);
		}
	}
	return(ubaerror);
}


/*
 * interrupt one of the processors
 */
extern u_long *ka60_ip[];
extern char *ka6200_ip[];
extern char *calypso_ip[];
int *ka8820_ip;

intrcpu(whichcpu)
int whichcpu;
{
	union cpusid cpusid;

 	switch (cpu) {

	case VAX_8200:
		mtpr(IPIR, 1<< whichcpu);
		break;

	case VAX_6200:
		*ka6200_ip[whichcpu] = 0;
		break;

	case VAX_6400:
		*calypso_ip[whichcpu] = 0;
		break;

	case VAX_8820:
		*ka8820_ip = 1 << ((whichcpu * 8) + 7);
		break;

	case VAX_8800:
		mtpr(INOP,0);
		break;

	case VAX_60:
		*ka60_ip[((whichcpu >> 1) & 0xf)] |= FIPD_IPL16;
		break;

	case VAX_9000:
		mtpr(ICIR, 1<< whichcpu);
		break;

	}

}
/*
 * called from init_main to see if a network boot has occurred.   If
 * so, available information is read into a local copy of the netblk
 * structure.  As per original design the changing of 'roottype' is what
 * triggers init_main to assume a diskless network environment.
 */

char boottype[4];

netbootchk()
{
	extern struct netblk *netblk_ptr;
	extern int roottype;
	extern int swaptype;
	extern int dumptype;
	extern int swapsize;

	/*
	 * determine if remote or local root or swap
	 * get the device name from the structure set
	 * up at autoconf time
	 */
	switch(rpb.devtyp) {
	case BTD$K_QNA:
	case BTD$K_LANCE:
		netblk_ptr = (struct netblk *)&vmb_info.netblk;
		if (netblk_ptr->rootfs == GT_NFS) {
			/*
			 * We've determined that we are running diskless
			 */
			if (rpb.devtyp == BTD$K_QNA)
				bcopy("qe0",boottype,sizeof(boottype));
			else
				bcopy("ln0",boottype,sizeof(boottype));
			roottype= (int) netblk_ptr->rootfs;
			swaptype= (int) netblk_ptr->swapfs;
			swapsize= ((int) netblk_ptr->swapsz) * 1024;
			if (netblk_ptr->dmpflg != -1)
				dumptype= ((int) netblk_ptr->dmpflg) * 1024;
		}
		break;
	default:
		break;
	}
}

/*
 * Get pointer to cpusw table entry for the system we are currently running
 * on.  The pointer returned by this routine will go into "cpup".
 *
 * The "cpu" variable (ULTRIX system type) is passed in and compared to the
 * system_type entry in the cpusw table for a match.
 */
struct cpusw *
cpuswitch_entry(cpu)
	int cpu;			/* the ULTRIX system type */
{
	register int i;			/* loop index */

	for (i = 0; cpusw[i].system_type != 0; i++) {
		if (cpusw[i].system_type == cpu)
			return((struct cpusw *)&cpusw[i]);
	}
	panic("processor type not configured");
}

