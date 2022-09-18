#ifndef lint
static	char	*sccsid = "@(#)conf.c	4.21	(ULTRIX)	4/11/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 - 1989, 1990 by		*
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
 *
 * Modification History:
 *
 *  09 Apr 91 -- chet
 *	Add Presto layer between character device calls to strategy
 *	routines (used by N-buf I/O).
 *
 *  07 Mar 91 Ken Kuscher
 *      Make sure msdup links properly when no dup hardware
 *
 *  06 Mar 91 -- jaw
 *	left "sdc" controllor out of genericconf.
 *
 *  23 Jan 91		Brian Nadeau
 *	Add multimedia drivers to cdevsw
 *
 *  03 Jan 91 -- darrell
 *	Fixed a packetfilter multiple define
 *
 *  19 Dec 90 -- JAW
 *	fix diskless microvax booting where controller is present
 *	but no disk are off the controller.
 *
 *  18 Dec 90 -- paradis
 *	Added stub for vp_idle()
 *
 *  05 Nov 90 -- stuarth
 *      Added nNXMI if no xmi busses present.
 *
 *  19 Aug 90 -- chet
 *      Further simplify optional presto scheme.
 *
 *  12-Sep-90		Charles Richmond IIS Corp
 *	Added stubs for kzqsa driver.
 *
 *  08 Sept 90		Stephen Reilly
 *	Added entry for IEEE driver call ek.
 *
 *  6-Sep-90 -- skc
 *      Add support for shadow device.
 *
 * 4-Sep-90		dlh
 *	added stubs for vector processor support routines
 *
 *  31-Aug-90		rafiey (Ali Rafieymehr)
 *	Added dummy reference to define nNVAXBI when the system doesn't
 *	have BI.
 *
 *  03-Aug-90		Randall Brown
 *	Added the xcons device and the dc device.
 *
 *  10 July 90 -- robin
 *	Made change that would allow lp code to work on MIPS systems.
 *	Simply removed ifdef's around include of lp.h
 *
 *  7 Jul 90 -- chet
 *	Simplify optional presto scheme.
 *
 *  15-June-90          Mark Parenti
 *      Add stub for xviaconf.
 *
 *  6-June-90		Pete Keilty
 *	Changed CIADAP to CIISR.
 *
 *  5-June-90		Paul G.
 *	Added stub on VAX side for xvmeconf routine.
 *
 * 30-May-90           Mark Parenti (map)
 *      Add dummy reference to allow xmi without vme.
 *
 * 25 May 90 -- chet
 *	Add support for presto NVRAM pseudo driver on
 *	scsi and mscp class disks
 *
 * 23-May-90		Mark Parenti
 *	Added dummy references to allow for xmi without bi and
 *	vme without xmi.
 *
 * 29-Mar-90 		Mitch McConnell
 *	Added stubs for szdinfo and sz_softc for 3max non-zero controller
 *	dump fix.
 *
 * 06-Mar-90 -- rafiey (Ali Rafieymehr)
 *	Added an entry in genericconf structure for KDM70.
 *
 * 28-Dec-1989		David E. Eiche		DEE0083
 *	Add dummy definition of log_xmierrs() to fix undefined reference
 *	in biinit.c
 *
 * 02-Dec-89		Fred Canter
 *	Added VAX pseudo device driver for user device on KA410/KA420 CPUs.
 *
 * 20-Nov-89		Tim Burke
 *	Added the "xa" driver to cdevsw[].  This driver is a layered product
 *	being developed by CSS/ISG for the DRV11. 
 *	Added the "utx" driver to cdevsw[].  This device special file is used
 *	by the Japanese ULTRIX group.
 *
 * 13-Nov-89		Randall Brown
 *	Added #ifdef vax around vcons_init.  This is defined in cons_sw_data.c
 *	for mips.
 *
 * 13-Nov-89    Janet Schank
 *      Added stub sdintr, asc_reset, and asc_dumpregs functions.
 *
 * 13-Nov-89		Bill Dallas
 *	Removed ifdef mips for ts
 *
 * 17-Oct-89		Mark A. Parenti
 *	Removed LYNX support.
 *
 * 12-Sep-89		Robin Lewis
 *	Added stub routine for siireset
 *
 * 22-Sep-89            Janet L. Schank
 *      Changed some defines to allow for mips autoconfiguration.
 *
 * 12-Sep-89	Mark Parenti (map)
 *	Fix problem in defines of uba stub routines. Include kdm in list.
 *
 * 15-Aug-89    Larry Scott
 *	add `#include "audit.h"` line
 *
 * 28-Jun-80	Fred Canter
 *	Add stubs for sz_dumpregs() and sii_dumpregs() for error logging.
 *
 * 17-Jun-89	Fred Canter
 *	Added sz_reset() and sii_reset() stubs for error log code.
 *
 * 14-Jun-1989	Uttam Shikarpur
 *	Added packet filter device. (Major device 70)
 *
 * 09-Jun-1989		Larry Scott
 *	Added audit device
 *
 * 28-May-1989		Tim Burke
 *	Created multiple major number entries for mscp disks (ra) in the
 *	bdev and cdev switch tables.  
 *
 *  8-May-1989	Giles Atkinson
 *	Add warning comment about ../io/uba/cons_maj.h
 *
 * 07-Mar-89		Todd M. Katz		TMK0002
 *	1. Change the name of the local MSI port probe routine.
 *	2. Declare the dummy routine msi_isr() and the dummy structure
 *	   msiintv[] whenever zero MSI local ports are configured.
 *	3. Change the dummy declaration of ciintv[] to reflect current
 *	   support for only a single CI port.
 *
 * 02-Mar-1989	jaw
 *	merge in SMP asymmetric driver support.
 *
 * 15-Feb-1989		afd (Al Delorey)
 *	Merged with V3.0 SDC
 *
 * 09-Jan-89		Fred canter
 *	Bug fix - szreset() stub in the wrong place.
 *
 * 30-Dec-88		Fred Canter
 *	Added dummy szreset() and sdreset() to satisfy references in
 *	machdep.c.
 *
 * 31-Jan-1989		Mark A. Parenti
 *	Removed xos support.
 *	Changed include syntax for merged pool.
 *
 * 25-Sep-88		Fred Canter
 *	Clean up comments.
 *
 * 19-Aug-88		Fred Canter
 *	Merge PVAX and FIREFOX SCSI drivers.
 *	Remove last of PVAx BTD$ kludge.
 *
 * 05-Aug-88		Todd M. Katz
 *	Changed the name of the CI780 interrupt service routine.
 *
 * 14-Jul-88		fred (Fred Canter)
 *	Change CDROM device name from cz to rz (it is a disk).
 *
 * 16-Jun-88		larry
 *	mscp drivers configured in when uq,bvpssp,ci, or msi are present.
 *
 * 09-Jun-88		darrell
 *	Added stub routines for fcrint, enafbiclog, and fccons_init to 
 *	to fix undefined symbols when VAX60 was not defined.
 *
 * 07-Jun-88		darrell
 *	Added Firefox diagnostic console and graphics console support
 *
 * 01-June-1988 	Robin
 *	Changed the name of the msi probe routine.
 *
 * 19-May-1988	fred (Fred Canter)
 *	Added SCSI device support to bdevsw and cdevsw tables.
 *
 * 20-Apr-1988		Ricky S. Palmer
 *	Removed a felonius reference to msiintv.
 *
 * 14-Apr-1988		David E. Eiche			DEE0030
 *	Change genericconf table entry for BVPSSP to specify "ra"
 *	disk type.
 *
 * 07-Apr-1988		David E. Eiche			DEE0028
 *	Add code to enable dumping to the HSC.
 *
 * 17-Mar-1988		David E. Eiche			DEE0017
 *	Change disk class driver open and close entries to use different
 *	routines for block and character devices.
 *
 * 1-25-88      Ricky Palmer
 *      Added MSI support.
 *
 * 1-12-88	Larry C.
 * 	Add dummy uba routines so autoconf will build when there is no
 *	uba device configured in.
 *
 * 12-11-87	Robin L. and Larry C.
 *		Added portclass/kmalloc support to the system.
 *
 * 19-Nov-87 -- Ricky Palmer (rsp)
 *	Added support for HSC type.
 *
 *  4-May-87 -- Fred Canter
 *	Added dummy Xstop() for kern_clock.c.
 *
 * 16-Apr-87 -- Fred Canter
 *	Moved dummy xos variables to xos_data.c file.
 *
 * 19-Mar-87 -- Fred Canter (+ Brian Stevens)
 *	Added X device entries to cdevsw[].
 *	Added global variables to identify the graphics device.
 *	Added dummy variables so X in the kernel can be optional.
 *
 * 11-Feb-87 -- rafiey (Ali Rafieymehr)
 *	Initialized two new variables for the Vaxstar color.
 *
 * 29-Jan-87 -- rsp (Ricky Palmer)
 *	Added support for its frame buffer driver.
 *
 * 06-Jan-87 -- gmm (George Mathew)
 *	Removed references to sdreset (Vaxstar disk driver does not need a 
 *	reset routine)
 *
 * 26-Sep-86 -- darrell
 *	This change undoes the temporary change put in by Fred Canter
 *	on 15-Sep-16.  The TZK50 driver now handles nbufio.
 *
 * 15-Sep-86 -- fred (Fred Canter)
 *	TEMPORARY CHANGE: remove ststrategy entry from cdevsw, to disable
 *	nbufio for VAXstar TZK50 driver until the driver can be fixed.
 *	This will allow the base-level I TK50 kit to install on VAXstar.
 *
 *  3-Sep-86 -- fred (Fred Canter)
 *	Removed unused sm driver entry points: smread, smwrite, smreset.
 *	Added dummy smcons_init & sgcons_init routines, to allow
 *	the kernel to build if they are not configured.
 *
 * 28-Aug-86 -- prs
 *	Removed declaration of ubdinit. Config declares an empty ubdinit
 *	structure, even if no uba's are in config file.
 *
 * 14-Aug-86 -- fred (Fred Canter)
 *	Changes VAXstar disk slaves from sd to rd/rx.
 *
 * 06-Aug-86 -- jaw	fixed baddaddr to work on running system.
 *
 *  5-Aug-86  -- fred (Fred Canter)
 *	Changed st to stc and misc. other VAXstar changes.
 *
 * 23-Jul-86  -- prs
 *	Added the genericconf table. This table needs to be built here
 *	because only configured devices can be in the table. Also if
 *	no uba's or mba's are configured, then set ubdinit and mbdinit
 *	structure pointers to NULL so swapgeneric, swapboot, and machdep
 *	won't complain.
 *
 *  2-Jul-86  -- fred (Fred Canter)
 *	Added more VAXstar devices to cdevsw[] (sg sm sh).
 *	Added dummy ??driver symbols to satisfy reference in ka630.c
 *	if the device is not configured.
 *
 * 18-Jun-86  -- fred (Fred Canter)
 *	Changes for VAXstar kernel support.
 *
 * 5-Jun-86   -- jaw 	changes to config.
 *
 * 15-Apr-86 -- afd
 *	Changed "v_console" to "v_consputc".
 *	Added definition of "v_consgetc" for qvss and qdss input.
 *	These are no longer enclosed by "#ifdef MVAX". Since the symbols
 *	    are referenced in cons.o they must be defined for all cpus.
 *	There is also a dummy "contigphys()" to define the symbol.
 *
 * 02-Apr-86 -- jaw  add support for nautilus console and memory adapter
 *
 * 11-Mar-86
 *	add support for SAS memory driver
 *
 * 11-Mar-86 -- lp
 *	Add strat to entire cdevsw for n-buffered I/O.
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *
 * 01-Mar-86 -- afd  Added DMB32 device to cdevsw.
 *
 * 12-Feb-86 -- jrs
 *	Move defn of null routine for buainit to bi dependency rather
 *	than 8200 cpu dependency.
 *
 * 03-Feb-86 -- jaw  for now define ka8200conf.
 *
 * 16-Jan-86 -- darrell
 *	removed the lines for the ka820machcheck, so it is not made
 *	as a psuedo device and defined it in cpuconf.c
 *
 * 20-Jan-86 -- Barb Glover
 *	Added error logger character device switch
 *
 * 01-Oct-85 -- Larry Cohen
 *	include ioctl.h so that window structure is defined.
 *
 * 03-Sep-85 -- jaw
 *	bi error interrupt added.
 *
 * 03-Aug-85 -- rjl
 *	added qdss and support to the microvax virtual console support.
 *
 * 21-Jun-85 -- jaw
 *	crx changes to cs...
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 20-Mar-85 jaw
 *	add VAX8200 console storage device.
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 * 26 Nov 84 -- rjl
 *	Added the system console as major number 38. Major #0 becomes the
 *	system virtual console. When it's something other than the real
 *	console, the real console can be accessed as 38,0.
 *
 *	14-Nov-84	Stephen Reilly
 * 001- Added ioctl to disk driver which will be used for the disk partitioning
 *	scheme.
 *
 *	conf.c	6.1	83/07/29
 ***********************************************************************/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/conf.h"
#include "../h/errno.h"
#include "../h/time.h"
#include "../h/proc.h"
#include "../h/presto.h"

#include "../h/types.h"
#include "../io/uba/ubavar.h"
#ifdef vax
#include "../sas/vax/vmb.h"
#endif vax
#include "../io/ci/ciadapter.h"

int	nulldev();
int	nodev();

#include "uba.h"
#include "kdb.h"
#include "klesib.h"
#include "kdm.h"
#if NUBA==0 && NKDB==0 && NKDM==0 && NKLESIB==0
struct uba_hd uba_hd[1];
ubaerror () {}
ubainitmaps () {}
uballoc () {}
ubarelse () {}
ubareset () {}
#endif

#include "ci.h"

#if NCI == 0
CIISR ci_isr;
void ci780_isr() {};
void ci_probe() {};
void ci_log_initerr() {};
void ci_unmapped_isr() {};
int (*ciintv[])() = { 0 };
int nNCI = 0;
#endif

#include "msi.h"
#if NMSI == 0
int msi_probe() {};
void msi_isr() {};
int (*msiintv[])() = { 0 };
int nNMSI = 0;
#endif NMSI

#include "bvpni.h"
#if NBVPNI == 0
void niattach() {};
int ni_softc, niinfo;
int nNI = 0;
#endif

#ifdef mips
#include "vba.h"
#if NVBA == 0
xvmeconf() { /* make xbiinit happy */ }
xviaconf() { /* make kn220.c happy */ }
#endif
#endif /* mips */

#ifdef vax
xvmeconf() { /* make xbiinit happy */ }
#endif	/* vax */

#include "xmi.h"
#if NXMI == 0 
xmidev_vec() { /* temp for xna driver */ }
log_xmierrors() { /* make biinit happy */ }
xminotconf() { /* make xvmeinit happy */ }
xmi_io_space() { /* make xvmeinit happy */ }
int	nNXMI; /* used in biinit.c */
#endif

#include "vaxbi.h"
#if NVAXBI == 0
#include "../io/bi/bireg.h"
bidev_vec() { /* temp for xna driver */ }
probebi() { /* make locore happy */ }
bierrors() { /* make locore happy */ }
buainit() { /* make locore happy */ }
biclrint() {/*make machdep happy */ }
bisetint() {/*make machdep happy */ }
log_bierrors() { /* make xmiinit happy */ }
struct	bidata	bidata[1]; /* make xmiinit happy */
int nNVAXBI = 0;
#endif

#if defined(VAX8200) || defined(VAX8800)
int	cs_size(),cs_ioctl(),cs_open(),cs_close(),cs_strategy(),cs_read(),cs_write();
#else
#define cs_size nodev
#define cs_open nodev
#define cs_close nodev
#define cs_strategy nodev
#define cs_read nodev
#define cs_ioctl nodev
#define cs_write nodev
#endif

#ifndef VAX8200
bimemenable() { /*null */}
bimemerror() { /*null */}
ka820rxcd() { /*null */}
#endif

#include "uq.h"
#include "ci.h"
#include "bvpssp.h"
#include "msi.h"
#include "msdup.h"

#if NCI==0 && NBVPSSP == 0 && NUQ == 0 && NMSI == 0
scs_start_sysaps () {/* make init_main.c happy */}
mscp_poll_wait () {/* make init_main.c happy */}
#endif

#ifdef vax
#include "hp.h"
#else
#define NHP 0
#endif vax

#if NHP > 0
int	hpopen(),hpstrategy(),hpread(),hpwrite(),hpdump(),hpioctl(),hpsize();
#else
#define hpopen		nodev
#define hpstrategy	nodev
#define hpread		nodev
#define hpwrite 	nodev
#define hpdump		nodev
#define hpioctl 	nodev
#define hpsize		0
#endif

#ifdef vax
#include "tu.h"
#else
#define NHT 0
#endif vax

#if NHT > 0
int	htopen(),htclose(),htstrategy(),htread(),htwrite(),htdump(),htioctl();
#else
#define htopen		nodev
#define htclose 	nodev
#define htstrategy	nodev
#define htread		nodev
#define htwrite 	nodev
#define htdump		nodev
#define htioctl 	nodev
#endif

#ifdef vax
#include "st.h"
#else
#define NST 0
#endif vax

#if NST > 0
int	stopen(),stclose(),ststrategy(),stread(),stwrite(),stdump(),stioctl();
#else
#define stopen		nodev
#define stclose 	nodev
#define ststrategy	nodev
#define stread		nodev
#define stwrite 	nodev
#define stdump		nodev
#define stioctl 	nodev
int	stcdriver;
st_start()
{
}
#endif

#include "scsi.h"
#include "sii.h"

#ifdef mips
#include "asc.h"
#include "kzq.h"		/* Until supported on vax	*/
#else
#define NASC 0
#define NKZQ 0			/* ..these 2 lines are needed ifdefed	*/
#endif mips

#if NSCSI == 0
szprobe()
{
	return(0);
}

szreset()
{
}
sz_reset()
{
}
sz_dumpregs()
{
}
#endif NSCSI

#if NSCSI == 0 && NSII == 0 
siireset()
{
	return(0);
}
#endif

#if NSII == 0
sii_probe()
{
	return(0);
}
sii_intr()
{
	return(0);
}
sii_reset()
{
	return(0);
}
sii_dumpregs()
{
}
#endif NSII

#if NKZQ == 0
kzqreset()
{
	return(0);
}
kzq_probe()
{
	return(0);
}

kzq_intr()
{
	return(0);
}
kzq_reset()
{
	return(0);
}
kzq_dumpregs()
{
	return(0);
}
#endif

#if NASC == 0
ascprobe()
{
	return(0);
}

ascintr()
{
	return(0);
}
asc_reset()
{
	return(0);
}
asc_dumpregs()
{
	return(0);
}
#endif

#include "presto.h"

#if NSCSI > 0 || NSII > 0 || NASC > 0
int	tzopen(),tzclose(),tzstrategy(),tzread(),tzwrite(),tzioctl();
int	rzopen(),rzstrategy(),rzread(),rzwrite(),rzdump(),rzioctl();
int	rzsize();
#if NPRESTO > 0
/* declare presto NVRAM pseudo-driver interface routines and structs */
int   	RZbopen(), RZbclose(), RZstrategy(), RZcstrat();
int   	RZread(), RZwrite();
#else
#define	RZbopen		rzopen
#define	RZbclose	nodev
#define	RZstrategy	rzstrategy
#define	RZcstrat	rzstrategy
#define	RZread		rzread
#define	RZwrite		rzwrite
#endif /* NPRESTO > 0 */
#else
#define tzopen		nodev
#define tzclose 	nodev
#define tzstrategy	nodev
#define tzread		nodev
#define tzwrite 	nodev
#define tzioctl 	nodev
#define rzopen		nodev
#define rzstrategy	nodev
#define rzdump		nodev
#define rzioctl 	nodev
#define	rzsize		0
int	scsidriver;
int	siidriver;
int	ascdriver;
int	szdinfo, sz_softc;
/* declare stubs for presto interface routines and structs */
#define	RZbopen		nodev
#define	RZbclose	nodev
#define	RZstrategy	nodev
#define	RZcstrat	nodev
#define	RZread		nodev
#define	RZwrite		nodev
#endif /* NSCSI > 0 || NSII > 0 || NASC > 0 */

#ifdef mips
#include "xcons.h"
#else
#define NXCONS 0
#endif mips

#if NXCONS > 0
int	xconsopen(), xconsclose(), xconsread(), xconswrite();
int	xconsioctl(), xconsstop();
extern struct tty xcons_tty[];
#else
#define xconsopen          nodev
#define xconsclose         nodev
#define xconsread          nodev
#define xconswrite         nodev
#define xconsioctl         nodev
#define xconsstop          nodev
#define xcons_tty 0
#endif

#ifdef vax
#include "rk.h"
#else
#define NHK 0
#endif vax

#if NHK > 0
int	rkopen(),rkstrategy(),rkread(),rkwrite(),rkioctl(),rkintr();
int	rkdump(),rkreset(),rksize();
#else
#define rkopen		nodev
#define rkstrategy	nodev
#define rkread		nodev
#define rkwrite 	nodev
#define rkioctl 	nodev
#define rkintr		nodev
#define rkdump		nodev
#define rkreset 	nodev
#define rksize		0
#endif

#ifdef vax
#include "te.h"
#else
#define NTE 0
#endif vax

#if NTE > 0
int	tmopen(),tmclose(),tmstrategy(),tmread(),tmwrite();
int	tmioctl(),tmdump(),tmreset();
#else
#define tmopen		nodev
#define tmclose 	nodev
#define tmstrategy	nodev
#define tmread		nodev
#define tmwrite 	nodev
#define tmioctl 	nodev
#define tmdump		nodev
#define tmreset 	nodev
#endif

#include "ts.h"

#if NTS > 0
int	tsopen(),tsclose(),tsstrategy(),tsread(),tswrite();
int	tsioctl(),tsdump(),tsreset();
#else
#define tsopen		nodev
#define tsclose 	nodev
#define tsstrategy	nodev
#define tsread		nodev
#define tswrite 	nodev
#define tsioctl 	nodev
#define tsdump		nodev
#define tsreset 	nodev
#endif


#include "shd.h"

#if NSHD > 0
int  shad_bopen(), shad_copen(), shad_close(), shad_read();
int  shad_write(), shad_strategy(), shad_ioctl(), shad_size();
#if NPRESTO > 0
/* declare presto NVRAM pseudo-driver interface routines and structs */
int 	SHAD_bopen(), SHAD_close(), SHAD_strategy(), SHAD_cstrat();
int	SHAD_read(), SHAD_write();
#else
#define SHAD_bopen	shad_bopen
#define SHAD_close	shad_close
#define SHAD_strategy	shad_strategy
#define SHAD_cstrat	shad_strategy
#define SHAD_read	shad_read
#define SHAD_write	shad_write
#endif /* NPRESTO > 0 */
#else
#define shad_bopen    nodev
#define shad_copen    nodev
#define shad_close    nodev
#define shad_read     nodev
#define shad_write    nodev
#define shad_strategy nodev
#define shad_ioctl    nodev
#define shad_size     0
/* declare stubs for presto interface routines and structs */
#define SHAD_bopen	nodev
#define SHAD_close	nodev
#define SHAD_strategy	nodev
#define SHAD_cstrat	nodev
#define SHAD_read	nodev
#define SHAD_write	nodev
#endif /* NSHD > 0 */

#ifdef vax
#include "mu.h"
#else
#define NMT 0
#endif vax

#if NMT > 0
int	mtopen(),mtclose(),mtstrategy(),mtread(),mtwrite();
int	mtioctl(),mtdump();
#else
#define mtopen		nodev
#define mtclose 	nodev
#define mtstrategy	nodev
#define mtread		nodev
#define mtwrite 	nodev
#define mtioctl 	nodev
#define mtdump		nodev
#endif

#if NCI>0 || NBVPSSP >0 || NUQ >0 || NMSI > 0
int	mscp_bopen(), mscp_copen(), mscp_bclose();
int	mscp_cclose(), mscp_strategy(), mscp_read();
int	mscp_write(), mscp_ioctl(), mscp_size();
#if NPRESTO > 0
/* declare presto NVRAM pseudo-driver interface routines and structs */
int 	MSCP_bopen(), MSCP_bclose(), MSCP_strategy(), MSCP_cstrat();
int	MSCP_read(), MSCP_write();
#else
#define MSCP_bopen	mscp_bopen
#define MSCP_bclose	mscp_bclose
#define MSCP_strategy	mscp_strategy
#define MSCP_cstrat	mscp_strategy
#define MSCP_read	mscp_read
#define MSCP_write	mscp_write
#endif /* NPRESTO > 0 */
#else
#define mscp_bopen	nodev
#define mscp_copen	nodev
#define mscp_bclose	nodev
#define mscp_cclose	nodev
#define mscp_strategy	nodev
#define mscp_read	nodev
#define mscp_write 	nodev
#define mscp_ioctl 	nodev
#define mscp_size	0
/* declare stubs for presto interface routines and structs */
#define MSCP_bopen	nodev
#define MSCP_bclose	nodev
#define MSCP_strategy	nodev
#define MSCP_cstrat	nodev
#define MSCP_read	nodev
#define MSCP_write	nodev
#endif /* NCI>0 || NBVPSSP >0 || NUQ >0 || NMSI > 0 */

#include "mscp.h"

#if NMSCP == 0
int nulldev();
struct uba_driver mscpdriver; int (*mscpint0[])() = { nulldev, 0 };  /* no mscpd
river */
#endif NMSCP

#ifdef vax
#include "sdc.h"
#else
#define NSDC 0
#endif vax

#if NSDC > 0
int	sdopen(),sdstrategy(),sdread(),sdwrite(),sdioctl();
int	sddump(),sdsize();
#else
#define sdopen		nodev
#define sdstrategy	nodev
#define sdread		nodev
#define sdwrite 	nodev
#define sdioctl 	nodev
#define sddump		nodev
#define sdsize		0
int	sdcdriver;
sdustart()
{
}
sdreset()
{
}
sdintr()
{
}
#endif NSDC

#ifdef vax
#include "up.h"
#else
#define NSC 0
#endif vax

#if NSC > 0
int	upopen(),upstrategy(),upread(),upwrite(),upioctl();
int	upreset(),updump(),upsize();
#else
#define upopen		nodev
#define upstrategy	nodev
#define upread		nodev
#define upwrite 	nodev
#define upioctl 	nodev
#define upreset 	nulldev
#define updump		nodev
#define upsize		0
#endif

#ifdef vax
#include "tj.h"
#else
#define NUT 0
#endif vax

#if NUT > 0
int	utopen(),utclose(),utstrategy(),utread(),utwrite(),utioctl();
int	utreset(),utdump();
#else
#define utopen		nodev
#define utclose 	nodev
#define utread		nodev
#define utstrategy	nodev
#define utwrite 	nodev
#define utreset 	nulldev
#define utioctl 	nodev
#define utdump		nodev
#endif

#ifdef vax
#include "rb.h"
#else
#define NIDC 0
#endif vax

#if NIDC > 0
int	idcopen(),idcstrategy(),idcread(),idcwrite(),idcioctl();
int	idcreset(),idcdump(),idcsize();;
#else
#define idcopen 	nodev
#define idcstrategy	nodev
#define idcread 	nodev
#define idcwrite	nodev
#define idcioctl	nodev
#define idcreset	nulldev
#define idcdump 	nodev
#define idcsize 	0
#endif

#if defined(VAX750) || defined(VAX730)
int	tuopen(),tuclose(),tustrategy();
#else
#define tuopen		nodev
#define tuclose 	nodev
#define tustrategy	nodev
#endif

#ifdef vax
#include "rx.h"
#else
#define NFX 0
#endif vax

#if NFX > 0
int	rxopen(),rxstrategy(),rxclose(),rxread(),rxwrite(),rxreset(),rxioctl();
#else
#define rxopen		nodev
#define rxstrategy	nodev
#define rxclose 	nodev
#define rxread		nodev
#define rxwrite 	nodev
#define rxreset 	nulldev
#define rxioctl 	nodev
#endif

#ifdef vax
#include "uu.h"
#else
#define NUU 0
#endif vax

#if NUU > 0
int	uuopen(),uustrategy(),uuclose(),uureset(),uuioctl();
#else
#define uuopen		nodev
#define uustrategy	nodev
#define uuclose 	nodev
#define uureset 	nulldev
#define uuioctl 	nodev
int	uu_softc;
int	uudinfo;
int	uudata;
#endif

#ifdef vax
#include "rl.h"
#else
#define NRL 0
#endif vax

#if NRL > 0
int	rlopen(),rlstrategy(),rlread(),rlwrite(),rlioctl();
int	rlreset(),rldump(),rlsize();
#else
#define rlopen		nodev
#define rlstrategy	nodev
#define rlread		nodev
#define rlwrite 	nodev
#define rlioctl 	nodev
#define rlreset 	nulldev
#define rldump		nodev
#define rlsize		0
#endif

#if NCI>0 || NBVPSSP >0 || NUQ >0 || NMSI > 0
int     tmscpopen(), tmscpclose(), tmscpstrategy(), tmscpread(),
        tmscpwrite(), tmscpioctl(), tmscpsize();
#else
#define tmscpopen	nodev
#define tmscpclose	nodev
#define tmscpstrategy	nodev
#define tmscpread	nodev
#define tmscpwrite	nodev
#define tmscpioctl	nodev
#define tmscpdump	nodev
#endif NCI

#include "md.h"

#if NMD > 0
int	mdstrategy(),md_size(),mdioctl();
#else
#define mdstrategy	nodev
#define md_size 	nodev
#define mdioctl 	nodev
#endif NMD

int	swstrategy(),swread(),swwrite();

struct bdevsw	bdevsw[] =
{
	{ hpopen,	nulldev,	hpstrategy,	hpdump, 	/*0*/
	  hpsize,	0,	hpioctl,	0},
	{ htopen,	htclose,	htstrategy,	htdump, 	/*1*/
	  0,		B_TAPE, nodev,	0},
	{ upopen,	nulldev,	upstrategy,	updump, 	/*2*/
	  upsize,	0,	nodev,	0},
	{ rkopen,	nulldev,	rkstrategy,	rkdump, 	/*3*/
	  rksize,	0,	rkioctl,	0},
	{ nodev,	nodev,		swstrategy,	nodev,		/*4*/
	  0,		0,	nodev,	0},
	{ tmopen,	tmclose,	tmstrategy,	tmdump, 	/*5*/
	  0,		B_TAPE, nodev,	0},
	{ tsopen,	tsclose,	tsstrategy,	tsdump, 	/*6*/
	  0,		B_TAPE, nodev,	0},
	{ mtopen,	mtclose,	mtstrategy,	mtdump, 	/*7*/
	  0,		B_TAPE, nodev,	0},
	{ tuopen,	tuclose,	tustrategy,	nodev,		/*8*/
	  0,		B_TAPE, nodev,	0},
        { nodev,   	nodev,    	nodev,  	nodev,          /*9*/
          nodev,    	0,      nodev,	0},
	{ utopen,	utclose,	utstrategy,	utdump, 	/*10*/
	  0,		B_TAPE, nodev,	0},
	{ idcopen,	nodev,		idcstrategy,	idcdump,	/*11*/
	  idcsize,	0,	idcioctl,	0},
	{ rxopen,	rxclose,	rxstrategy,	nodev,		/*12*/
	  0,		0,	nodev,	0},
	{ uuopen,	uuclose,	uustrategy,	nodev,		/*13*/
	  0,		0,	nodev,	0},
	{ rlopen,	nodev,		rlstrategy,	rldump, 	/*14*/
	  rlsize,	0,	rlioctl,	0},
	{ tmscpopen,	tmscpclose,	tmscpstrategy,	nodev,		/*15*/
	  0,		B_TAPE, nodev,	0},
	{ cs_open,	cs_close,	cs_strategy,	nodev,		/*16*/
	  cs_size,	0,	cs_ioctl,	0},
	{ nulldev,	nulldev,	mdstrategy,	nodev,		/*17*/
	  md_size,	0,	mdioctl,	0},
	{ stopen,	stclose,	ststrategy,	stdump, 	/*18*/
	  0,		B_TAPE, nodev,	0},
	{ sdopen,	nulldev,	sdstrategy,	sddump, 	/*19*/
	  sdsize,	0,	sdioctl,	0},
	{ tzopen,	tzclose,	tzstrategy,	nodev,	 	/*20*/
	  0,		B_TAPE, nodev,	0},
	/*
	 * Insert presto NVRAM pseudo-driver interface routines (RZ...).
	 * Warning: If these device major numbers change, then
	 *          data/presto_data.c must be modified.
	 */
	{ RZbopen,       RZbclose,       RZstrategy,     rzdump,        /*21*/
	  rzsize,       0,      rzioctl,	0},
	{ nodev,	nodev,		nodev,		nodev,		/*22*/
	  0,		0,	nodev,	0},
	/*
	 * Multiple major numbers for the mscp devices.  
	 * Each major number allows for 32 disks.  Here there are 8 majors
	 * declared allowing for 8*32 or 256 disks.
	 * 
	 * Warning: if the base major number is changed from 23 then the
	 * value of MSCP_BASE in buf.h and RA_BASE in mscp_defs.h.  Also 
	 * devices.vax and config/MAKEDEV changes.
	 */
	/*
	 * Insert presto NVRAM pseudo-driver interface routines (MSCP_...).
	 * Warning: If these device major numbers change, then
	 *          data/presto_data.c must be modified.
	 */
        { MSCP_bopen,   MSCP_bclose,    MSCP_strategy,  nodev,          /*23*/
          mscp_size,    0,      mscp_ioctl ,	0},
        { MSCP_bopen,   MSCP_bclose,    MSCP_strategy,  nodev,          /*24*/
          mscp_size,    0,      mscp_ioctl ,	0},
        { MSCP_bopen,   MSCP_bclose,    MSCP_strategy,  nodev,          /*25*/
          mscp_size,    0,      mscp_ioctl ,	0},
        { MSCP_bopen,   MSCP_bclose,    MSCP_strategy,  nodev,          /*26*/
          mscp_size,    0,      mscp_ioctl ,	0},
        { MSCP_bopen,   MSCP_bclose,    MSCP_strategy,  nodev,          /*27*/
          mscp_size,    0,      mscp_ioctl ,	0},
        { MSCP_bopen,   MSCP_bclose,    MSCP_strategy,  nodev,          /*28*/
          mscp_size,    0,      mscp_ioctl ,	0},
        { MSCP_bopen,   MSCP_bclose,    MSCP_strategy,  nodev,          /*29*/
          mscp_size,    0,      mscp_ioctl ,	0},
        { MSCP_bopen,   MSCP_bclose,    MSCP_strategy,  nodev,          /*30*/
          mscp_size,    0,      mscp_ioctl ,	0},
        { SHAD_bopen,   SHAD_close,     SHAD_strategy,  nodev,          /*31*/
          shad_size,        0,      shad_ioctl,      0},	                
};

#ifdef vax
#include "mba.h"
#else
#define NMBA 0
#endif vax

#if NMBA == 0
struct mba_device *mbdinit = NULL;
#endif

#if NHP > 0
extern struct mba_driver hpdriver;
#endif

#if NUQ > 0
extern struct uba_driver uqdriver;
int uq_reset();
#else
#define uq_reset	nulldev
#endif

#if NBVPSSP > 0
extern struct uba_driver bvpsspdriver;
#endif

#include "hsc.h"

#if NHSC > 0
extern struct uba_driver hscdriver;
#endif

#include "dssc.h"

#if NDSSC > 0
extern struct uba_driver dsscdriver;
#endif

#if NRB > 0
extern struct uba_driver idcdriver;
#endif

#if NRL > 0
extern struct uba_driver hldriver;
#endif

#if NHK > 0
extern struct uba_driver hkdriver;
#endif

#if NSDC > 0
extern struct uba_driver sdcdriver;
#endif

#if NSCSI > 0
extern struct uba_driver scsidriver;
#endif

#if NSII > 0
extern struct uba_driver siidriver;
#endif

#if NASC > 0
extern struct uba_driver ascdriver;
#endif

/*
 * Initialize genericconf structure, if the driver is configured.
 */

/* On a mips machine we don't include vmb.h */
#ifdef mips
#define BTD$K_MB 	0
#define	BTD$K_UDA	0
#define	BTD$K_BDA	0
#define BTD$K_HSCCI	0
#define BTD$K_SII	0
#define BTD$K_ASC	0
#define BTD$K_BVPSSP	0
#define BTD$K_DQ	0
#define BTD$K_DL	0
#define BTD$K_DM	0
#define BTD$K_KA640_DISK 0
#define BTD$K_KA620_DISK 0
#define	BTD$K_KDM70	0
#endif mips
struct genericconf  genericconf[] =
{
#if NHP > 0
	{ (caddr_t)&hpdriver,	"hp",	makedev(0,0),	BTD$K_MB },
#endif
#if NUQ > 0 
	{ (caddr_t)&uqdriver,	"ra",	makedev(23,0),	BTD$K_UDA },
	{ (caddr_t)&uqdriver,	"ra",	makedev(23,0),	BTD$K_BDA },
	{ (caddr_t)&uqdriver,	"ra",	makedev(23,0),	BTD$K_KDM70 },
#endif
#if NHSC > 0
        { (caddr_t)&hscdriver,  "ra",  makedev(23,0),   BTD$K_HSCCI },
#endif
#if NDSSC > 0
        { (caddr_t)&dsscdriver,  "ra",  makedev(23,0),   BTD$K_SII },
#endif
#if NBVPSSP > 0 
        { (caddr_t)&bvpsspdriver,       "ra",  makedev(23,0),   BTD$K_BVPSSP },
#endif
#if NRB > 0
	{ (caddr_t)&idcdriver,	"rb",	makedev(11,0),	BTD$K_DQ },
#endif
#if NRL > 0
	{ (caddr_t)&hldriver,	"rl",	makedev(14,0),	BTD$K_DL },
#endif
#if NHK > 0
	{ (caddr_t)&hkdriver,	"rk",	makedev(3,0),	BTD$K_DM },
#endif
#if NSDC > 0
	{ (caddr_t)&sdcdriver,	"rd",	makedev(19,0),	BTD$K_KA640_DISK },
#endif
#if NSCSI > 0
	{ (caddr_t)&scsidriver,	"rz",	makedev(21,0),	BTD$K_KA420_DISK },
#endif
#if NSII > 0
	{ (caddr_t)&siidriver,	"rz",	makedev(21,0),	BTD$K_SII },
#endif
#if NASC > 0
	{ (caddr_t)&ascdriver,	"rz",	makedev(21,0),	BTD$K_ASC },
#endif
	{ 0 },
};

int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);

#ifdef vax
int	cnopen(),cnclose(),cnread(),cnwrite(),cnioctl();
#endif vax
extern struct tty cons[];

#ifdef mips
int	cnopen(),cnclose(),cnread(),cnwrite(),cnioctl(),cnselect(), cnstop();
#endif mips

#ifdef vax
#include "acc.h"
#else
#define NACC 0
#endif vax

#if NACC > 0
int	accreset();
#else
#define accreset nulldev
#endif

#ifdef vax
#include "ct.h"
#else
#define NCT 0
#endif vax

#if NCT > 0
int	ctopen(),ctclose(),ctwrite();
#else
#define ctopen	nulldev
#define ctclose nulldev
#define ctwrite nulldev
#endif

#ifdef vax
#include "dh.h"
#else
#define NDH 0
#endif vax

#if NDH == 0
#define dhopen	nodev
#define dhclose nodev
#define dhread	nodev
#define dhwrite nodev
#define dhioctl nodev
#define dhstop	nodev
#define dhreset nulldev
#define dh11	0
int	nNDH = NDH;
dhtimer(){/* to keep the undefines in locore happy */ }
#else
int	dhopen(),dhclose(),dhread(),dhwrite(),dhioctl(),dhstop(),dhreset();
struct	tty dh11[];
#endif

#include "dhu.h"
#if NDHU == 0
#define dhuopen nodev
#define dhuclose	nodev
#define dhuread nodev
#define dhuwrite	nodev
#define dhuioctl	nodev
#define dhustop nodev
#define dhureset	nulldev
#define dhu11	0
int	nNDHU = NDHU;
dhutimer(){/* to keep the undefines in locore happy */ }
#else NDHU
int	dhuopen(),dhuclose(),dhuread(),dhuwrite(),dhuioctl(),dhustop(),dhureset();
extern struct	tty dhu11[];
#endif NDHU

#ifdef vax
#include "dmf.h"
#else
#define NDMF 0
#endif vax

#if NDMF == 0
#define dmfopen nodev
#define dmfclose	nodev
#define dmfread nodev
#define dmfwrite	nodev
#define dmfioctl	nodev
#define dmfstop nodev
#define dmfreset	nulldev
#define dmf_tty 0
#else
int	dmfopen(),dmfclose(),dmfread(),dmfwrite(),dmfioctl(),dmfstop(),dmfreset();
struct	tty dmf_tty[];
#endif

#include "dmb.h"

#if NDMB == 0
#define dmbopen nodev
#define dmbclose	nodev
#define dmbread nodev
#define dmbwrite	nodev
#define dmbioctl	nodev
#define dmbstop nodev
#define dmbreset	nulldev
#define dmb_tty 0
#else
int	dmbopen(),dmbclose(),dmbread(),dmbwrite(),dmbioctl(),dmbstop(),dmbreset();
extern struct	tty dmb_tty[];
#endif NDMB

#ifdef vax
#include "dmz.h"
#else
#define NDMZ 0
#endif vax

#if NDMZ == 0
#define dmzopen nodev
#define dmzclose	nodev
#define dmzread nodev
#define dmzwrite	nodev
#define dmzioctl	nodev
#define dmzstop nodev
#define dmzreset	nulldev
#define dmz_tty 0
#else
int	dmzopen(),dmzclose(),dmzread(),dmzwrite(),dmzioctl(),dmzstop(),dmzreset();
struct	tty dmz_tty[];
#endif

#ifdef mips
#include "dc.h"
#else
#define NDC 0
#endif

#if NDC == 0
#define dcopen 	nodev
#define dcclose	nodev
#define dcread 	nodev
#define dcwrite	nodev
#define dcioctl	nodev
#define dcstop 	nodev
#define dcselect nodev
#define dc_tty	0
#else NDC
int	dcopen(),dcclose(),dcread(),dcwrite(),dcioctl(),dcstop(), dcselect();
extern struct	tty dc_tty[];
#endif NDC

#ifdef mips
#include "scc.h"
#else
#define NSCC 0
#endif

#if NSCC == 0
#define sccopen 	nodev
#define sccclose	nodev
#define sccread 	nodev
#define sccwrite	nodev
#define sccioctl	nodev
#define sccstop 	nodev
#define sccselect 	nodev
#define scc_tty	0
#else NDC
int	sccopen(),sccclose(),sccread(),sccwrite(),sccioctl(),sccstop(), sccselect();
extern struct	tty scc_tty[];
#endif NDC

#ifdef vax
#include "qv.h"
#else
#define NQV 0
#endif vax

#if NQV == 0
#define qvopen	nodev
#define qvclose nodev
#define qvread	nodev
#define qvwrite nodev
#define qvioctl nodev
#define qvstop	nodev
#define qvreset nulldev
#define qvselect nodev
#define qv_tty	0
#define qvcons_init nulldev
#else NQV
int	qvopen(),qvclose(),qvread(),qvwrite(),qvioctl(),qvstop(),qvreset(),
	qvselect(),qvcons_init();
struct	tty qv_tty[];
#endif NQV

#ifdef vax
#include "qd.h"
#else
#define NQD 0
#endif vax

#if NQD == 0
#define qdopen	nodev
#define qdclose nodev
#define qdread	nodev
#define qdwrite nodev
#define qdioctl nodev
#define qdstop	nodev
#define qdreset nulldev
#define qdselect nodev
#define qd_tty	0
#define qdcons_init nulldev
#else NQD
int	qdopen(),qdclose(),qdread(),qdwrite(),qdioctl(),qdstop(),qdreset(),
	qdselect(),qdcons_init();
struct	tty qd_tty[];
#endif NQD


#ifdef vax
#include "sm.h"
#else
#define NSM 0
#endif vax

/*
 * NOTE:
 *	VAXstar bitmap driver has a cdevsw[]
 *	entry, but it is never used. All calls
 *	to bitmap driver come from SLU driver (ss.c).
 */
#if NSM == 0
#define smopen	nodev
#define smclose nodev
#define smread	nodev
#define smwrite nodev
#define smioctl nodev
#define smstop	nodev
#define smreset nulldev
#define smselect nodev
smcons_init()
{
	return(ENXIO);
}
int	smdriver;
#else NSM
#define smread	nodev
#define smwrite nodev
int	smopen(),smclose(),smioctl(),smstop(),smreset(),
	smselect();
#endif NSM

#ifdef vax
#include "sg.h"
#else
#define NSG 0
#endif vax

#if NSG == 0
#define sgopen	nodev
#define sgclose nodev
#define sgread	nodev
#define sgwrite nodev
#define sgioctl nodev
#define sgstop	nodev
#define sgreset nulldev
#define sgselect nodev
#define sg_tty	0
sgcons_init()
{
	return(ENXIO);
}
int	sgdriver;
#else NSG
int	sgopen(),sgclose(),sgread(),sgwrite(),sgioctl(),sgstop(),sgreset(),
	sgselect();
struct	tty sg_tty[];
#endif NSG

#ifdef vax
#include "sh.h"
#else
#define NSH 0
#endif vax

#if NSH == 0
#define shopen nodev
#define shclose	nodev
#define shread nodev
#define shwrite	nodev
#define shioctl	nodev
#define shstop nodev
#define shreset	nulldev
#define sh_tty	0
int	shdriver;
int	nNSH = NSH;
shtimer(){/* to keep the undefines in locore happy */ }
#else NSH
int	shopen(),shclose(),shread(),shwrite(),shioctl(),shstop(),shreset();
struct	tty sh_tty[];
#endif NSH

/*
 * Pseudo device driver for user added devices
 * on KA410/KA420 (MV2000, MV3100) processors.
 */
#ifdef vax
#include "sp.h"
#else
#define NSP 0
#endif vax

#if NSP == 0
#define spopen nodev
#define spclose	nodev
#define spread nodev
#define spwrite	nodev
#define spioctl	nodev
#define spstop nodev
#define spreset	nulldev
#define sp_tty	0
int	spdriver;
int	nNSP = NSP;
#ifdef	notdef
sptimer(){/* to keep the undefines in locore happy */ }
#endif	notdef
#else NSP
int	spopen(),spclose(),spread(),spwrite(),spioctl(),spstop(),spreset();
struct	tty sp_tty[];
#endif NSP

#ifdef vax
#include "ss.h"
#else
#define NSS 0
#endif vax

#if NSS == 0
#define ssopen	nodev
#define ssclose nodev
#define ssread	nodev
#define sswrite nodev
#define ssioctl nodev
#define ssstop	nodev
#define ssselect nodev
#define ss_tty	0
#define sscons_init nulldev
int	nNSS = NSS;
sstimer(){/* to get rid of undefines in locore */ }
int	sspdma;
int	ssdriver;
#else
int	ssopen(),ssclose(),ssread(),sswrite(),ssioctl(),ssstop(),
	ssselect(),sscons_init();
struct	tty ss_tty[];
#endif NSS

#ifndef VAX60
fcrint() { /* null */ }
enafbiclog() { /* null */ }
fccons_init() { /* null */ }
#endif VAX60

#ifdef vax
#include "fc.h"
#else
#define NFC 0
#endif vax

#if NFC == 0
#define fcopen	nodev
#define fcclose nodev
#define fcread	nodev
#define fcwrite nodev
#define fcioctl nodev
#define fcstop	nodev
#define fcselect nodev
#define fc_tty	0
#define fccons_init nulldev
int	nNFC = NFC;
fctimer(){/* to get rid of undefines in locore */ }
int	fcpdma;
int	fcdriver;
#else
int	fcopen(),fcclose(),fcread(),fcwrite(),fcioctl(),fcstop(),
	fcselect(),fccons_init();
struct	tty fc_tty[];
#endif NFC

#ifdef vax
#include "fg.h"
#else
#define NFG 0
#endif vax

#if NFG == 0
#define fgopen  nodev
#define fgclose nodev
#define fgread  nodev
#define fgwrite nodev
#define fgioctl nodev
#define fgstop  nodev
#define fgreset nulldev
#define fgselect nodev
#define fg_tty  0
fgcons_init()
{
        return(ENXIO);
}
int   fgdriver;
#else NFG
int     fgopen(),fgclose(),fgread(),fgwrite(),fgioctl(),fgstop(),fgreset(),
        fgselect();
struct  tty fg_tty[];
#endif NFG

#include "ln.h"
#if NLN == 0
int	lndriver;
#endif NLN

#ifdef mips
#include "ne.h"
#if NNE == 0
int	nedriver;
#endif NNE
#endif mips

/*
 * On a workstation, these variables identify
 * the type of graphics display device and
 * which units are present. This info is filled
 * in by the graphics device drivers (qd qv sm sg).
 */

int	ws_display_type = 0;	/* Major device number of display device */
int	ws_display_units = 0;	/* Bit field of units (bit0 = unit 0, etc) */

/*
 * MicroVAX and VAXstar virtual console support
 */
/*
 * Virtual console character display and read routines.
 * The drivers will fill these in as they configure.
 */
int (*v_consputc)() = 0;
int (*v_consgetc)() = 0;

/*
 * VAXstar graphics device driver entry points,
 * can't use cdevsw[] like QVSS & QDSS because the
 * VAXstar console 4 line SLU is shared between the graphics
 * device and the EIA and printer ports.
 * The graphics device drivers fill in the entry
 * points in their ??cons_init() routines.
 * The SLU driver (ss.c) uses these entry points to call
 * the graphics driver if the operation if for the graphics device.
 */
int (*vs_gdopen)() = 0;
int (*vs_gdclose)() = 0;
int (*vs_gdread)() = 0;
int (*vs_gdwrite)() = 0;
int (*vs_gdselect)() = 0;
int (*vs_gdkint)() = 0;
int (*vs_gdioctl)() = 0;
int (*vs_gdstop)() = 0;

/*
 * Console initialization routines
 *
 * These routines are called to set up the system console so
 * that it can be used by cnputc to display characters before
 * the driver probe routines are hit. (catch 22) The search is
 * complete when the list is ended or a driver reports that it
 * setup.
 */

/*
 * CAUTION:	sscons_init must be first so that the serial line cntlr
 *		will be configured as the console on a VAXstar/TEAMmate.
 *		Something in the VAXstar's address space looks like a
 *		QDSS and fools autoconf. If sscons_init is not first, then
 *		qvcons_init and qdcons_init must be modified to return 0
 *		if the CPU is a VAXstar.
 */
#ifdef vax
(*vcons_init[])() = {
	sscons_init,	/* MUST BE FIRST */
	qdcons_init,
	qvcons_init,
	0
};
#endif vax
/*
 * contigphys is only used by the MicroVAX I. But the routine
 * must be defined for all processors to resolve the reference
 * from uba.c.
 */
#ifndef MVAX
contigphys()
{
}
#endif

#include "lta.h"

#if NLTA == 0
#define ltaopen 	nodev
#define ltaclose	nodev
#define ltaread 	nodev
#define ltawrite	nodev
#define ltaioctl	nodev
#define ltastop 	nodev
#define ltareset	nulldev
#define lata	0
#else
int ltaopen(),ltaclose(),ltaread(),ltawrite(),ltaioctl(),ltastop();
#define ltareset	nulldev
extern	struct tty lata[];
#endif NLTA

#if VAX8600
int	crlopen(),crlclose(),crlread(),crlwrite();
#else
#define crlopen 	nodev
#define crlclose	nodev
#define crlread 	nodev
#define crlwrite	nodev
#endif

#if VAX780
int	flopen(),flclose(),flread(),flwrite();
#else
#define flopen	nodev
#define flclose nodev
#define flread	nodev
#define flwrite nodev
#endif

#ifdef vax
#include "dz.h"
#else
#define NDZ 0
#endif vax

#if NDZ == 0
#define dzopen	nodev
#define dzclose nodev
#define dzread	nodev
#define dzwrite nodev
#define dzioctl nodev
#define dzstop	nodev
#define dzreset nulldev
#define dz_tty	0
int	nNDZ = NDZ;
dztimer(){/* to get rid of undefines in locore */ }
int	dzpdma;
#else
int	dzopen(),dzclose(),dzread(),dzwrite(),dzioctl(),dzstop(),dzreset();
struct	tty dz_tty[];
#endif

#include "lp.h"
#if NLP > 0
int	lpopen(),lpclose(),lpwrite(),lpreset();
#else
#define lpopen		nodev
#define lpclose 	nodev
#define lpwrite 	nodev
#define lpreset 	nulldev
#endif

int	syopen(),syread(),sywrite(),syioctl(),syselect();

int	mmread(),mmwrite();
#define mmselect	seltrue

#ifdef vax
#include "va.h"
#else
#define NVA 0
#endif vax

#if NVA > 0
int	vaopen(),vaclose(),vawrite(),vaioctl(),vareset(),vaselect();
#else
#define vaopen		nodev
#define vaclose 	nodev
#define vawrite 	nodev
#define vaopen		nodev
#define vaioctl 	nodev
#define vareset 	nulldev
#define vaselect	nodev
#endif NVA

#ifdef vax
#include "vp.h"
#else
#define NVP 0
#endif vax

#if NVP > 0
int	vpopen(),vpclose(),vpwrite(),vpioctl(),vpreset(),vpselect();
#else
#define vpopen		nodev
#define vpclose 	nodev
#define vpwrite 	nodev
#define vpioctl 	nodev
#define vpreset 	nulldev
#define vpselect	nodev
#endif NVP

#include "pty.h"
#if NPTY > 0
int	ptsopen(),ptsclose(),ptsread(),ptswrite(),ptsstop();
int	ptcopen(),ptcclose(),ptcread(),ptcwrite(),ptcselect();
int	ptyioctl();
extern	struct	tty pt_tty[];
#else
#define ptsopen 	nodev
#define ptsclose	nodev
#define ptsread 	nodev
#define ptswrite	nodev
#define ptcopen 	nodev
#define ptcclose	nodev
#define ptcread 	nodev
#define ptcwrite	nodev
#define ptyioctl	nodev
#define pt_tty		0
#define ptcselect	nodev
#define ptsstop 	nulldev
ptyinit() { }
#endif NPTY

#ifdef vax
#include "lpa.h"
#else
#define NLPA 0
#endif vax

#if NLPA > 0
int	lpaopen(),lpaclose(),lparead(),lpawrite(),lpaioctl();
#else
#define lpaopen 	nodev
#define lpaclose	nodev
#define lparead 	nodev
#define lpawrite	nodev
#define lpaioctl	nodev
#endif NLPA

#ifdef vax
#include "dn.h"
#else
#define NDN 0
#endif vax

#if NDN > 0
int	dnopen(),dnclose(),dnwrite();
#else
#define dnopen		nodev
#define dnclose 	nodev
#define dnwrite 	nodev
#endif NDN

#ifdef vax
#include "gpib.h"
#else
#define NGPIB 0
#endif vax

#if NGPIB > 0
int	gpibopen(),gpibclose(),gpibread(),gpibwrite(),gpibioctl();
#else
#define gpibopen	nodev
#define gpibclose	nodev
#define gpibread	nodev
#define gpibwrite	nodev
#define gpibioctl	nodev
#endif NGPIB

#ifdef vax
#include "ik.h"
#else
#define NIK 0
#endif vax

#if NIK > 0
int	ikopen(),ikclose(),ikread(),ikwrite(),ikioctl(),ikreset();
#else
#define ikopen nodev
#define ikclose nodev
#define ikread nodev
#define ikwrite nodev
#define ikioctl nodev
#define ikreset nodev
#endif NIK

#ifdef vax
#include "its.h"
#else
#define NITS 0
#endif vax

#if NITS > 0
int	itsopen(),itsclose(),itsioctl(),itsread();
#else
#define itsopen nodev
#define itsclose nodev
#define itsioctl nodev
#define itsread nodev
#endif NITS

#ifdef vax
#include "ps.h"
#else
#define NPS 0
#endif vax

#if NPS > 0
int	psopen(),psclose(),psread(),pswrite(),psioctl(),psreset();
#else
int	nNPS = NPS;
psextsync(){	/* Got to keep locore happy */}
#define psopen nodev
#define psclose nodev
#define psread nodev
#define pswrite nodev
#define psopen nodev
#define psioctl nodev
#define psreset nodev
#endif NPS

#ifdef vax
#include "ib.h"
#else
#define NIB 0
#endif vax

#if NIB > 0
int	ibopen(),ibclose(),ibread(),ibwrite(),ibioctl();
#else
#define ibopen	nodev
#define ibclose nodev
#define ibread	nodev
#define ibwrite nodev
#define ibioctl nodev
#endif NIB

#ifdef vax
#include "ad.h"
#else
#define NAD 0
#endif vax

#if NAD > 0
int	adopen(),adclose(),adioctl(),adreset();
#else
#define adopen nodev
#define adclose nodev
#define adioctl nodev
#define adreset nodev
#endif NAD

#ifdef vax
#include "vs.h"
#else
#define NVS 0
#endif vax

#if NVS > 0
int	vsopen(),vsclose(),vsioctl(),vsreset(),vsselect();
#else
#define vsopen nodev
#define vsclose nodev
#define vsioctl nodev
#define vsreset nodev
#define vsselect nodev
#endif NVS

#include "sys_trace.h"
#if NSYS_TRACE > 0
int	trace_open(),trace_close(),trace_ioctl(),trace_select(),trace_read();
#else NSYS_TRACE
#define	trace_open nodev
#define	trace_close nodev
#define	trace_ioctl nodev
#define	trace_read nodev
#define	trace_select nodev
int syscall_trace(x,y,z){}
#endif NSYS_TRACE

#include "audit.h"
#if AUDIT > 0
int     auditopen(), auditclose(), auditread(), auditwrite(), auditsel();
#else AUDIT
#define auditopen nodev
#define auditclose nodev
#define auditread nodev
#define auditwrite nodev
#define auditsel nodev
#endif AUDIT

/*
 * LNV21 printer/scanner controller.  Layered product driver.
 */
#define ldopen nodev
#define ldclose nodev
#define ldioctl nodev
#define ldreset nodev
#define ldread nodev
#define ldwrite nodev

/*
 * DRV11 CSS/ISG.  Layered product driver.
 */
#define xaopen nodev
#define xaclose nodev
#define xaread nodev
#define xawrite nodev
#define xaioctl nodev
#define xareset nodev
#define xastrategy nodev

/*
 * Japanese ULTRIX specific device
 */
#define utxopen nodev
#define utxclose nodev
#define utxread nodev
#define utxwrite nodev
#define utxioctl nodev
#define utxselect nodev

/*
 *	IEEE driver
 */
#define ekopen nodev
#define ekclose nodev
#define ekread nodev
#define ekwrite nodev
#define ekioctl nodev
#define ekstrategy nodev

#include "packetfilter.h"
#if NPACKETFILTER > 0
int pfoption=1;
int	Pfilt_open(), Pfilt_close(), Pfilt_read(), Pfilt_write(), Pfilt_ioctl(),
    	Pfilt_select();
#else NPACKETFILTER
int pfoption=0;
#define Pfilt_open nodev
#define Pfilt_close nodev
#define Pfilt_read nodev
#define Pfilt_write nodev
#define Pfilt_ioctl nodev
#define Pfilt_select nodev
#ifdef notdef /* DAD for JSD */
/* declare these here since it's the most convenient place */
struct mbuf *pfilt_filter() {};
pfilt_attach() {};
pfilt_newaddress() {};
#endif notdef /* DAD for JSD */
#endif NPACKETFILTER

#include "ether.h"
#include "fddi.h"
#if NPACKETFILTER + NETHER + NFDDI == 0 
/* declare these here since it's the most convenient place */
int pfactive=0;		/* TEMPORARY - fix for sas kernels  */
struct mbuf *pfilt_filter() {};
pfilt_attach() {};
pfilt_newaddress() {};
#endif /* NPACKETFILTER + NETHER + NFDDI */

#if NMSDUP > 0 && (NMSI > 0 || NCI > 0 || NBVPSSP > 0 || NUQ > 0)
int     msdup_open(), msdup_close(), msdup_select(),
	msdup_read(), msdup_write(), msdup_ioctl();

#else
#define msdup_open	nodev
#define msdup_close	nodev
#define msdup_select	nodev
#define msdup_read	nodev
#define msdup_write	nodev
#define msdup_ioctl	nodev
#endif

/*
 * Define presto NVRAM pseudo-driver device control routines
 */
#if NPRESTO > 0
int	propen(), prioctl();
#else
#define propen		nodev
#define prioctl		nodev
int presto_init() { /* dummy */ }
int presto_reboot() { /* dummy */ }
int prbounceio()  { /* dummy */ }
int prunbounceio()  { /* dummy */ }
int prdirectio()  { /* dummy */ }
struct presto_interface presto_interface0; /* dummy */
struct nvram_battery_info nvram_batteries0; /* dummy */
int prattached = 0; /* dummy */
#endif /* NPRESTO > 0 */

#ifdef mips
#include "mmlp.h"
#else
#define NMMLP 0
#endif	/*mips*/

#if NMMLP > 0
int	audio_a_open(), audio_a_close(), audio_a_read(), audio_a_write();
int	audio_a_ioctl(), audio_a_stop(), audio_a_reset();
int	audio_a_select(), audio_a_mmap(), audio_a_strat();
int	audio_b_open(), audio_b_close(), audio_b_read(), audio_b_write();
int	audio_b_ioctl(), audio_b_stop(), audio_b_reset();
int	audio_b_select(), audio_b_mmap(), audio_b_strat();
int	video_in_a_open(), video_in_a_close(), video_in_a_read(), video_in_a_write();
int	video_in_a_ioctl(), video_in_a_stop(), video_in_a_reset();
int	video_in_a_select(), video_in_a_mmap(), video_in_a_strat();
int	video_out_a_open(), video_out_a_close(), video_out_a_read(), video_out_a_write();
int	video_out_a_ioctl(), video_out_a_stop(), video_out_a_reset();
int	video_out_a_select(), video_out_a_mmap(), video_out_a_strat();
#else
#define audio_a_open nodev
#define audio_a_close nodev
#define audio_a_read nodev
#define audio_a_write nodev
#define audio_a_ioctl nodev
#define audio_a_stop nodev
#define audio_a_reset nodev
#define audio_a_select nodev
#define audio_a_mmap nodev
#define audio_a_strat nodev
#define audio_b_open nodev
#define audio_b_close nodev
#define audio_b_read nodev
#define audio_b_write nodev
#define audio_b_ioctl nodev
#define audio_b_stop nodev
#define audio_b_reset nodev
#define audio_b_select nodev
#define audio_b_mmap nodev
#define audio_b_strat nodev
#define video_in_a_open nodev
#define video_in_a_close nodev
#define video_in_a_read nodev
#define video_in_a_write nodev
#define video_in_a_ioctl nodev
#define video_in_a_stop nodev
#define video_in_a_reset nodev
#define video_in_a_select nodev
#define video_in_a_mmap nodev
#define video_in_a_strat nodev
#define video_out_a_open nodev
#define video_out_a_close nodev
#define video_out_a_read nodev
#define video_out_a_write nodev
#define video_out_a_ioctl nodev
#define video_out_a_stop nodev
#define video_out_a_reset nodev
#define video_out_a_select nodev
#define video_out_a_mmap nodev
#define video_out_a_strat nodev
#endif	/*MMLP*/

int	ttselect(), seltrue(), asyncsel();
int	erropen(), errclose(), erread(), errwrite(), errioctl(), errsel();

/* WARNING: if the order of entries in the cdevsw is changed then the
 * file  ../io/uba/console_majors.h must be checked and revised.
 */

struct cdevsw	cdevsw[] =
{
	{cnopen, 	cnclose,	cnread, 	cnwrite,	/*0*/
#ifdef vax
	cnioctl,	nulldev,	nulldev,	cons,
	ttselect,	nodev,		0,	0},
#endif vax
#ifdef mips
	/* d_ttys will be filled in by cninit */
	cnioctl,	cnstop,		nulldev,	0, 
	cnselect,	nodev,		0,	0},
#endif mips
	{dzopen, 	dzclose,	dzread, 	dzwrite,	/*1*/
	dzioctl,	dzstop, 	dzreset,	dz_tty,
	ttselect,	nodev,		0,	0},
	{syopen, 	nulldev,	syread, 	sywrite,	/*2*/
	syioctl,	nulldev,	nulldev,	0,
	syselect,	nodev,		0,ALLCPU},
	{nulldev,	nulldev,	mmread, 	mmwrite,	/*3*/
	nodev,		nulldev,	nulldev,	0,
	mmselect,	nodev,		0,ALLCPU},
	{hpopen, 	nulldev,	hpread, 	hpwrite,	/*4*/
	hpioctl,	nodev,		nulldev,	0,
	asyncsel,	nodev,		hpstrategy,	0},
	{htopen, 	htclose,	htread, 	htwrite,	/*5*/
	htioctl,	nodev,		nulldev,	0,
	asyncsel,	nodev,		htstrategy,	0},
	{vpopen, 	vpclose,	nodev,		vpwrite,	/*6*/
	vpioctl,	nulldev,	vpreset,	0,
	vpselect,	nodev,		0,	0},
	{nulldev,	nulldev,	swread, 	swwrite,	/*7*/
	nodev,		nodev,		nulldev,	0,
	nodev,		nodev,		0,	0},
	{flopen, 	flclose,	flread, 	flwrite,	/*8*/
	nodev,		nodev,		nulldev,	0,
	seltrue,	nodev,		0,	0},
        {nodev,     	nodev,    	nodev,      	nodev,     	/*9*/
        nodev,     	nodev,          nulldev,        0,
        nodev,       	nodev,          nodev,	0},
	{vaopen, 	vaclose,	nodev,		vawrite,	/*10*/
	vaioctl,	nulldev,	vareset,	0,
	vaselect,	nodev,		0,	0},
	{rkopen, 	nulldev,	rkread, 	rkwrite,	/*11*/
	rkioctl,	nodev,		rkreset,	0,
	seltrue,	nodev,		0,	0},
	{dhopen, 	dhclose,	dhread, 	dhwrite,	/*12*/
	dhioctl,	dhstop, 	dhreset,	dh11,
	ttselect,	nodev,		0,	0},
	{upopen, 	nulldev,	upread, 	upwrite,	/*13*/
	upioctl,	nodev,		upreset,	0,
	seltrue,	nodev,		0,	0},
	{tmopen, 	tmclose,	tmread, 	tmwrite,	/*14*/
	tmioctl,	nodev,		tmreset,	0,
	seltrue,	nodev,		0,	0},
	{lpopen, 	lpclose,	nodev,		lpwrite,	/*15*/
	nodev,		nodev,		lpreset,	0,
	seltrue,	nodev,		0,	0},
	{tsopen, 	tsclose,	tsread, 	tswrite,	/*16*/
	tsioctl,	nodev,		tsreset,	0,
	asyncsel,	nodev,		tsstrategy,	0},
	{utopen, 	utclose,	utread, 	utwrite,	/*17*/
	utioctl,	nodev,		utreset,	0,
	seltrue,	nodev,		0,	0},
	{ctopen, 	ctclose,	nodev,		ctwrite,	/*18*/
	nodev,		nodev,		nulldev,	0,
	seltrue,	nodev,		0,	0},
	{mtopen, 	mtclose,	mtread, 	mtwrite,	/*19*/
	mtioctl,	nodev,		nodev,		0,
	asyncsel,	nodev,		mtstrategy,	0},
	{ptsopen,	ptsclose,	ptsread,	ptswrite,	/*20*/
	ptyioctl,	ptsstop,	nodev,		pt_tty,
	ttselect,	nodev,		0,ALLCPU},
	{ptcopen,	ptcclose,	ptcread,	ptcwrite,	/*21*/
	ptyioctl,	nulldev,	nodev,		pt_tty,
	ptcselect,	nodev,		0,ALLCPU},
	{dmfopen,	dmfclose,	dmfread,	dmfwrite,	/*22*/
	dmfioctl,	dmfstop,	dmfreset,	dmf_tty,
	ttselect,	nodev,		0,	0},
	{idcopen,	nulldev,	idcread,	idcwrite,	/*23*/
	idcioctl,	nodev,		idcreset,	0,
	seltrue,	nodev,		0,	0},
	{dnopen, 	dnclose,	nodev,		dnwrite,	/*24*/
	nodev,		nodev,		nodev,		0,
	seltrue,	nodev,		0,	0},
/* 25-29 reserved to local sites */
	{gpibopen,	gpibclose,	gpibread,	gpibwrite,	/*25*/
	gpibioctl,	nulldev,	nodev,		0,
	seltrue,	nodev,		0,	0},
	{lpaopen,	lpaclose,	lparead,	lpawrite,	/*26*/
	lpaioctl,	nodev,		nulldev,	0,
	seltrue,	nodev,		0,	0},
	{psopen, 	psclose,	psread, 	pswrite,	/*27*/
	psioctl,	nodev,		psreset,	0,
	seltrue,	nodev,		0,	0},
	{ibopen, 	ibclose,	ibread, 	ibwrite,	/*28*/
	ibioctl,	nodev,		nodev,		0,
	seltrue,	nodev,		0,	0},
	{adopen, 	adclose,	nodev,		nodev,		/*29*/
	adioctl,	nodev,		adreset,	0,
	seltrue,	nodev,		0,	0},
	{rxopen, 	rxclose,	rxread, 	rxwrite,	/*30*/
	rxioctl,	nodev,		rxreset,	0,
	seltrue,	nodev,		0,	0},
	{ikopen, 	ikclose,	ikread, 	ikwrite,	/*31*/
	ikioctl,	nodev,		ikreset,	0,
	seltrue,	nodev,		0,	0},
	{rlopen, 	nodev,		rlread, 	rlwrite,	/*32*/
	rlioctl,	nodev,		rlreset,	0,
	seltrue,	nodev,		0,	0},
/*
 * The DHU driver includes the DHV driver.
 */
	{dhuopen,	dhuclose,	dhuread,	dhuwrite,	/*33*/
	dhuioctl,	dhustop,	dhureset,	dhu11,
	ttselect,	nodev,		0,	0},
#ifdef vax
	{dmzopen,	dmzclose,	dmzread,	dmzwrite,	/*34*/
	dmzioctl,	dmzstop,	dmzreset,	dmz_tty,
	ttselect,	nodev,		0,	0},
#endif vax
#ifdef mips
        {dcopen,	dcclose,	dcread,		dcwrite,
	dcioctl,	dcstop,		nulldev,	dc_tty,
	dcselect,	nodev,		0,		0},
#endif mips
	{qvopen, 	qvclose,	qvread, 	qvwrite,	/*35*/
	qvioctl,	qvstop, 	qvreset,	qv_tty,
	qvselect,	nodev,		0,	0},
	{tmscpopen,	tmscpclose,	tmscpread,	tmscpwrite,	/*36*/
	tmscpioctl,	nodev,		uq_reset,	0,
	asyncsel,	nodev,		tmscpstrategy,	0},
	{vsopen, 	vsclose,	nodev,		nodev,		/*37*/
	vsioctl,	nodev,		vsreset,	0,
	vsselect,	nodev,		0,	0},
#ifdef vax
	{cnopen, 	cnclose,	cnread, 	cnwrite,	/*38*/
	cnioctl,	nulldev,	nulldev,	cons,
	ttselect,	nodev,		0,	0},
#endif vax
#ifdef mips
	{nulldev, 	nulldev,	nulldev, 	nulldev,	/*38*/
	nulldev,	nulldev,	nulldev,	0,
	nulldev,	nodev,		0,	0},
#endif mips
	{ltaopen,	ltaclose,	ltaread,	ltawrite,	/*39*/
	ltaioctl,	ltastop,	ltareset,	lata,
	ttselect,	nodev,		0,ALLCPU},
	{crlopen,	crlclose,	crlread,	crlwrite,	/*40*/
	nodev,		nodev,		nulldev,	0,
	seltrue,	nodev,		0,	0},
	{cs_open,	cs_close,	cs_read,	cs_write,	/*41*/
	cs_ioctl,	nodev,		nulldev,	0,
	seltrue,	nodev,		0,	0},
	{qdopen, 	qdclose,	qdread, 	qdwrite,	/*42*/
	qdioctl,	qdstop, 	qdreset,	qd_tty,
	qdselect,	nodev,		0,	0},
	{erropen,	errclose,	erread, 	errwrite,	/*43*/
	errioctl,	nodev,		nulldev,	0,
	errsel, 	nodev,		0,	0},
	{dmbopen,	dmbclose,	dmbread,	dmbwrite,	/*44*/
	dmbioctl,	dmbstop,	dmbreset,	dmb_tty,
	ttselect,	nodev,		0,ALLCPU},
#ifdef vax
	{ssopen, 	ssclose,	ssread, 	sswrite,	/*45*/
	ssioctl,	ssstop, 	nulldev,	ss_tty,
	ssselect,	nodev,		0,	0},
#endif vax
#ifdef mips
        {sccopen,	sccclose,	sccread,	sccwrite,
	sccioctl,	sccstop,	nulldev,	scc_tty,
	sccselect,	nodev,		0,		0},
#endif mips
	{stopen, 	stclose,	stread, 	stwrite,	/*46*/
	stioctl,	nodev,		nulldev,	0,
	asyncsel,	nodev,		ststrategy,	0},
	{sdopen, 	nulldev,	sdread, 	sdwrite,	/*47*/
	sdioctl,	nodev,		nulldev,	0,
	asyncsel,	nodev,		sdstrategy,	0},
	{trace_open,	trace_close,	trace_read,	nodev,		/*48*/
	trace_ioctl,	nodev,		nodev,		0,
	trace_select,	nodev,		0,ALLCPU},
	{smopen, 	smclose,	smread, 	smwrite,	/*49*/
	smioctl,	smstop, 	smreset,	0,
	smselect,	nodev,		0,	0},
	{sgopen, 	sgclose,	sgread, 	sgwrite,	/*50*/
	sgioctl,	sgstop, 	sgreset,	sg_tty,
	sgselect,	nodev,		0,	0},
	{shopen, 	shclose,	shread, 	shwrite,	/*51*/
	shioctl,	shstop, 	shreset,	sh_tty,
	ttselect,	nodev,		0,	0},
	{itsopen,	itsclose,	itsread,	nodev,          /*52*/
	itsioctl,	nodev,		nodev,		0,
	seltrue,	nodev,		0,	0},
	{nodev,		nodev,		nodev,		nodev,		/*53*/
	nodev,		nodev,		nodev,		0,
	nodev,		nodev,		0,	0},
        {nodev,         nodev,          nodev,          nodev,          /*54*/
        nodev,          nodev,          nodev,          0,
        nodev,          nodev,          0,	0},
	{tzopen, 	tzclose,	tzread, 	tzwrite,	/*55*/
	tzioctl,	nodev,		nulldev,	0,
	asyncsel,	nodev,		tzstrategy,	0},
	/*
	 * Insert presto NVRAM pseudo-driver interface routines (RZ...).
	 * Warning: If these device major numbers change, then
	 *          data/presto_data.c must be modified.
	 */
        {rzopen,        nulldev,       	RZread,         RZwrite,        /*56*/
        rzioctl,        nodev,          nulldev,        0,
	asyncsel,	nodev,		RZcstrat,	0},
	{nodev,		nodev,		nodev,		nodev,		/*57*/
	nodev,		nodev,		nodev,		0,
	nodev,		nodev,		0,	0},
	{fcopen,         fcclose,        fcread,         fcwrite,        /*58*/
	fcioctl,        fcstop,         nulldev,        fc_tty,
	fcselect,       nodev,          0,	0},
        {fgopen,         fgclose,        fgread,         fgwrite,        /*59*/
        fgioctl,        fgstop,         fgreset,        fg_tty,
        fgselect,       nodev,          0,	0},
	/*
	 * Multiple major numbers of mscp devs.  
	 */
	/*
	 * Insert presto NVRAM pseudo-driver interface routines (MSCP_...).
	 * Warning: If these device major numbers change, then
	 *          data/presto_data.c must be modified.
	 */
        {mscp_copen,     mscp_cclose,    MSCP_read,      MSCP_write,     /*60*/
        mscp_ioctl,     nodev,          nulldev,                0,
        asyncsel,       nodev,          MSCP_cstrat,	0},
        {mscp_copen,     mscp_cclose,    MSCP_read,      MSCP_write,     /*61*/
        mscp_ioctl,     nodev,          nulldev,                0,
        asyncsel,       nodev,          MSCP_cstrat,	0},
        {mscp_copen,     mscp_cclose,    MSCP_read,      MSCP_write,     /*62*/
        mscp_ioctl,     nodev,          nulldev,                0,
        asyncsel,       nodev,          MSCP_cstrat,	0},
        {mscp_copen,     mscp_cclose,    MSCP_read,      MSCP_write,     /*63*/
        mscp_ioctl,     nodev,          nulldev,                0,
        asyncsel,       nodev,          MSCP_cstrat,	0},
        {mscp_copen,     mscp_cclose,    MSCP_read,      MSCP_write,     /*64*/
        mscp_ioctl,     nodev,          nulldev,                0,
        asyncsel,       nodev,          MSCP_cstrat,	0},
        {mscp_copen,     mscp_cclose,    MSCP_read,      MSCP_write,     /*65*/
        mscp_ioctl,     nodev,          nulldev,                0,
        asyncsel,       nodev,          MSCP_cstrat,	0},
        {mscp_copen,     mscp_cclose,    MSCP_read,      MSCP_write,     /*66*/
        mscp_ioctl,     nodev,          nulldev,                0,
        asyncsel,       nodev,          MSCP_cstrat,	0},
        {mscp_copen,     mscp_cclose,    MSCP_read,      MSCP_write,     /*67*/
        mscp_ioctl,     nodev,          nulldev,                0,
        asyncsel,       nodev,          MSCP_cstrat,	0},
        {ldopen,        ldclose,        ldread,          ldwrite,        /*68*/
        ldioctl,        nodev,          nulldev,                0,
        seltrue,        nodev,          0,	0},
        {auditopen,      auditclose,     auditread,      auditwrite,     /*69*/
        nodev,          nodev,          nulldev,        0,
        auditsel,       nodev,          0, ALLCPU},
	{Pfilt_open,     Pfilt_close,    Pfilt_read,     Pfilt_write,	 /*70*/
	Pfilt_ioctl,    nodev,          nulldev,        0,
	Pfilt_select,   nodev,          0, ALLCPU},
#ifdef mips
        {xconsopen,	xconsclose,	xconsread,      xconswrite,        	 /*71*/
        xconsioctl,   	xconsstop, 	nodev,        	xcons_tty,
        ttselect,    	nodev,          0,		0},
#else
	/* PLACE HOLDER:needs to be fixed when the console code come together */
	{nulldev,	nulldev,	nulldev,	nulldev,	/*71*/
	nulldev,	nulldev,	nulldev,	0,
	nodev,		nodev,		0,		0},
#endif mips
        {xaopen,        xaclose,        xaread,         xawrite,        /*72*/
        xaioctl,        nulldev,        xareset,        0,
        asyncsel,       nodev,          xastrategy,	0},
        {utxopen,       utxclose,       utxread,        utxwrite,	/*73*/
        utxioctl,       nulldev,        nodev,          0,
        utxselect,      nodev,          0, ALLCPU},
	{spopen, 	spclose,	spread, 	spwrite,	/*74*/
	spioctl,	spstop, 	spreset,	sp_tty,
	nodev,		nodev,		0,	0},
	/* presto NVRAM pseudo-driver control "device" */
        {propen,        nulldev,        nulldev,	nulldev,        /*75*/
        prioctl,        nulldev,        nulldev,        0,
        nodev,          nodev,          0,	0},
        {shad_copen,    shad_close,     SHAD_read,      SHAD_write,     /*76*/
        shad_ioctl,     nulldev,        nodev,          0,
        nodev,          nodev,          SHAD_cstrat,  0},
	{ekopen,	ekclose,	ekread,		ekwrite,	/*77*/
	ekioctl,	nodev,		nulldev,	0,
	nodev,		nodev,		ekstrategy,	0},
	{msdup_open,	msdup_close,	msdup_read,	msdup_write,	/*78*/
	msdup_ioctl,	nodev,		nodev,		0,
	msdup_select,	nodev,		0,		0},
 	/* Multimedia drivers - 79, 80, 81, 82 */
 	{audio_a_open,  audio_a_close,  audio_a_read,   audio_a_write,	/*79*/
 	audio_a_ioctl,  audio_a_stop,   audio_a_reset,  0,
 	audio_a_select, audio_a_mmap,   audio_a_strat,  0},
 	{audio_b_open,  audio_b_close,  audio_b_read,   audio_b_write,	/*80*/
 	audio_b_ioctl,  audio_b_stop,   audio_b_reset,  0,
 	audio_b_select, audio_b_mmap,   audio_b_strat,  0},
 	{video_in_a_open,   video_in_a_close,  video_in_a_read,   video_in_a_write, /*81*/
 	video_in_a_ioctl,   video_in_a_stop,   video_in_a_reset,  0,
 	video_in_a_select,  video_in_a_mmap,   video_in_a_strat,  0},
 	{video_out_a_open,  video_out_a_close, video_out_a_read,  video_out_a_write, /*82*/
 	video_out_a_ioctl,  video_out_a_stop,  video_out_a_reset, 0,
 	video_out_a_select, video_out_a_mmap,  video_out_a_strat, 0}
};
int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);

int	mem_no = 3;	/* major device number of memory special file */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t	swapdev = makedev(4, 0);

/*
 * vector support stub routines to make the linker happy.
 */
#ifdef VECTORS

#if (VECTORS == default) || (VECTORS == 1)	/* VECTORS is set to default */
int	max_vec_procs = MAXUPRC / 10;

#else 						/* VECTORS is set to a value */
int	max_vec_procs = VECTORS;

#endif

#else						/* VECTORS is not defined */
int	max_vec_procs = 0;
vp_allocate			() {}
vp_cleanup			() {}
vp_contextlimbo			() {}
vp_contextsave			() {}

#include	"../h/user.h"

vp_disabled_fault_handler	()
{
/* This stub will cover the occurence of a vector disabled fault in a
 * system which does not have vector capability configured into it's
 * kernel.  If a vector disabled fault should occur, the the process will
 * be sent an illegal instruction signal.
 */
	psignal (u.u_procp, SIGILL);
}

vp_ls_bug			() {}
vp_remove			() {}
vp_reset			() {}
vp_idle				() {}
#endif VECTORS
