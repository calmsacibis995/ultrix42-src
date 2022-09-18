#ifdef lint
static char *sccsid = "@(#)cpuconf.c	4.16    ULTRIX  3/6/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,85,86,88,89 by		*
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

/***********************************************************************
 *
 * Modification History: cpuconf.c
 *
 * 06-Mar-91	jaw
 *	optimize 3min spl
 *
 * 15-Oct-90	Randall Brown
 *	Added errlogging routines to cpusw for 3min.
 *
 * 09-Oct-90    jaw
 * 	merge in MM changes for rigel.
 *
 * 09-Oct-90    Paul Grist
 *      added startclock routine to cpuswitch for mipsmate, this
 *      fixes hangs after power-fails when the system needs to
 *      acess non-root disks, which are not spun up, the cases
 *      that were seen were swap on non-root and presto buffers.
 *
 * 01-Sep-90	sekhar
 *	added functions and stubs for print_consinfo interface.
 *      kn02_print_consinfo, kn02_log_errinfo 	- DS5000 (3MAX)
 *	kn220_print_consinfo, kn220_log_errinfo	- DS5500 (MIPSFAIR2)
 *	nullcpu stubs for other machines (both mips and vax).
 *
 * 31-Aug-90	Jim Paradis
 *	Added additional stubs for VAX9000 routines
 *
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 *	Added support for VAX9000.
 *
 * 21-Jun-90	Fred L. Templin
 *	Added dummies for TURBOchannel data structures for case of
 *	DS5000 not defines. (Solution from afd).
 *
 * 20-Mar-90    Paul Grist
 *      Added MIPSMATE support (DS_5100).
 *
 * 30-Apr-90	Randall Brown
 *	Added new cpu entry for DS_5000_100.  Filled in the new values
 *	of the switch table for the MIPS systems. ( spls, intr, clock stuff)
 *
 * 06-Mar-90	afd
 *	Put mc146818startclocks() into cpu switch for pmax/3max.
 *
 * 18-Jan-90	robin
 *	Added kn220badaddr function to get badaddr to work with the
 *	way the KN220 memory intrrupts are cleared.
 *
 * 29-Dec-89	afd
 *	Added definitions for kn02erradr & kn02chksyn for when
 *	DS5000 not defined.
 *
 * 26-Dec-89	robin
 *	changed the kn220 write buffer routine used by 5500.
 *
 * 08-Dec-89	jaw
 *	fix 6200 entry from merge damage.
 *
 * 30-Nov-89    Paul Grist
 *      Added 8800 error logging routines as stubs for non-8800 
 *      VAXBI systems that will use biinit.c Did the same for
 *      ka6200 and ka6400.
 *
 * 14-Nov-89	gmm
 *	Remove kn5800_iscpu() and add kn5800_init_secondary().
 *
 * 30-Oct-89	afd
 *	Use kn01 cache flush routines for kn02 (DS5000 - 3max).
 *
 * 11-Aug-89	afd
 *	Set up 3MAX cpu struct in cpu switch table.
 *	
 * 10-Jul-89	burns
 *	For DS5800 moved several "vax" only fields into mips. Added
 *	the msize and cpuinit routines for afd. Added cache routines
 *	to the cpu switch for DS5800.
 *
 * 14-June-89	afd
 *	Fill in new HZ field in cpuswitch (used to be in param.c).
 *	hz, tick, tickadj are set in processor specific init routines.
 *
 * 23-May-89	darrell
 *	Merged VVAX support.
 *
 * 03-May-89	darrell
 *	Merged in VAX6400 support.
 *
 * 07-Apr-89	afd
 *	Created this file as a merged version of the old VAX cpuconf.c
 *	with new entries for MIPS based systems.  This file now supports
 *	both VAX and MIPS based systems.
 *
 **********************************************************************/


#include "../machine/pte.h"
#include "../h/param.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/cpu.h"
#include "../io/uba/ubareg.h"
#include "../machine/nexus.h"
#ifdef vax
#include "../machine/ioa.h"
#include "../machine/cvax.h"
#endif
#ifdef mips
#include "../machine/kn5800.h"
#endif mips

int	nocpu();
int	nullcpu();
int 	bbadaddr();
int	readtodr();
int 	writetodr();
int 	uICRdelay();
int 	uInoICRdelay();
int 	uIInoICRdelay();
int 	cVSnoICRdelay();
int 	uSSCdelay();
int	uRSSCdelay();
int	ssc_readtodr();
int	ssc_writetodr();

#ifdef vax
extern short nexty750[];
extern short nexty730[];
extern short nextyUVI[];
extern short *ioaaddr8600[];

#ifdef VAX780
int  	ka780nexaddr(); 
int	ka780umaddr();
int	ka780udevaddr();
int	ka780machcheck();
int	ka780memerr();
int	ka780setcache();
int	ka780memenable();
int	ka780tocons();
int	ka780cachenbl();
int	ka780logsbi();
int	ka780conf();
#endif VAX780

#ifdef VAX750
int	ka750machcheck();
int	ka750memerr();
int	ka750setcache();
int	ka750memenable();
int	ka750tocons();
int	ka750cachenbl();
int  	ka750nexaddr(); 
int	ka750umaddr();
int	ka750udevaddr();
int	ka750conf();
#endif VAX750

#ifdef VAX730
int  	ka730nexaddr(); 
int	ka730umaddr();
int	ka730udevaddr();
int	ka730machcheck();
int	ka730memerr();
int	ka730memenable();
int	ka730tocons();
int	ka730conf();
#endif VAX730

#ifdef VAX8600
int  	ka8600nexaddr(); 
int	ka8600memerr();
int	ka8600setcache();
int	ka8600memenable();
int	ka8600tocons();
int	ka8600cachenbl();
int	ka8600umaddr();
int	ka8600udevaddr();
int	ka8600machcheck();
int	ka8600logsbi();
int	ka8600conf();
#else
crlintr() { /*keep locore happy*/ }  
#endif VAX8600

#ifdef MVAX
int	ka610machcheck();
int	ka610setcache();
int	ka610cachenbl();
int	ka610tocons();
int  	ka610nexaddr(); 
int	ka610umaddr();
int	ka610udevaddr();
int	ka610conf();
int	ka630setcache();
int	ka630cachenbl();
int	ka630tocons();
int  	ka630nexaddr(); 
int	ka630umaddr();
int	ka630udevaddr();
int	ka630machcheck();
int	ka630readtodr();
int	ka630writetodr();
int	ka630conf();
#endif MVAX

#ifdef VAX8200
int  	ka8200nexaddr(); 
int	ka8200umaddr();
int	ka8200udevaddr();
int	ka8200machcheck();
int	ka8200conf();
int	ka8200memerr();
int	ka8200setcache();
int	ka8200memenable();
int	ka8200tocons();
int	ka8200cachenbl();
int	ka8200readtodr();
int	ka8200writetodr();
int     ka8200startcpu();
int     ka8200stopcpu();
#else
ka820slavehalt() {/* fake routine for machdep*/ }
ka8200rxopen() {/* rx50 open routine */ }
ka8200startrx() {/* rx50 start routine */ }
rx5_intr() { /* rx50 interrupt routine */ }
#endif VAX8200

#ifdef VAX8800
int     ka8800startcpu();
int     ka8800stopcpu();
int  	ka8800nexaddr(); 
int	ka8800umaddr();
int	ka8800udevaddr();
int	ka8800machcheck();
int	ka8800memerr();
int	ka8800conf();
int 	ka8800badaddr();
int	ka8800tocons();
int	ka8800cachenbl();
int 	ka8800setcache();
int 	ka8800memenable();
int	ka8800readtodr();
int	ka8800writetodr();
#else
ka8800nmifault() { /*keep locore happy */}
nmifaultclear(){/* keep locore happy */}
ka8800requeue(){/* tty console driver */ }
ka8800startrx(){/* start rx50 */ }
rx8800_trans() {/* rx50 transmit routine */ }
log_ka8800memerrs() {/* keep biinit happy */}
log_ka8800bierrors() {/* keep biinit happy */}
nbia_log_err() { /* keep biinit happy */}
#endif VAX8800

#ifdef VAX3600
int	ka650machcheck();
int	ka650crderr();
int	ka650setcache();
int 	ka650memenable();
int	ka650tocons();
int	ka650conf();
int	ka650cachenbl();
int	ka650umaddr();
int	ka650udevaddr();
int	ka650memerr();
#endif VAX3600

#ifdef VAX420
int	ka420machcheck();
int	ka420crderr();
int	ka420setcache();
int	ka420tocons();
int	ka420conf();
int	ka420cachenbl();
int  	ka420nexaddr(); 
int	ka420umaddr();
int	ka420readtodr();
int	ka420writetodr();
#endif

#ifdef VAX6200
int  	ka6200nexaddr(); 
int	ka6200umaddr();
int	ka6200udevaddr();
int	ka6200machcheck();
int	ka6200conf();
int	ka6200memerr();
int	ka6200crderr();
int	ka6200setcache();
int	ka6200memenable();
int	ka6200tocons();
int	ka6200writetodr();
int 	ka6200cachenbl();
int 	ka6200badaddr();
#else
ka6200initslave() {/* init slave on calypso */ }
ka6200mapcsr() { /* map csr1 on calypso*/ }
ka6200halt()  { /* call out of machdep */}
ka6200reboot()  { /* call out of machdep */}
int *ka6200_ip;
ka6200_clear_xbe() {/* call out of machdep */ }
log_ka6200memerrs() {/* call out of biinit.c*/}
#endif

#ifdef VAX60
int	ka60machcheck();
int	ka60crderr();
int	ka60setcache();
int 	ka60memenable();
int	ka60tocons();
int	ka60conf();
int	ka60cachenbl();
int  	ka60nexaddr(); 
int	ka60umaddr();
int	ka60udevaddr();
int	ka60memerr();
int	ka60writetodr();
int	ka60readtodr();
#else
int *ka60_ip;
ka60initslave() {/* init slave on Firefox */ }
ka60memerr() { /* called from locore */}
ka60clrmbint() { /* clear mbus memerr interrupts */ }
ka60setmbint() { /* set (enable) mbus memerr interrupts */ }
#endif VAX60

#ifdef VAX6400
int     ka6400nexaddr();
int     ka6400umaddr();
int     ka6400udevaddr();
int     ka6400machcheck();
int     ka6400conf();
int     ka6400harderr();
int     ka6400softerr();
int     ka6400setcache();
int     ka6400memenable();
int     ka6400tocons();
int     ka6400writetodr();
int     ka6400cachenbl();
int     ka6400badaddr();
#else
ka6400initslave() {/* init slave on rigel */ }
ka6400mapcsr() { /* map the RSSC registers on rigel*/ }
ka6400halt()  { /* call out of machdep */}
ka6400reboot()  { /* call out of machdep */}
int *calypso_ip;
ka6400_clear_xbe() {/* call out of machdep */}
ka6400_disable_cache() {/* call out of machdep */}
clear_xrperr() {/* call out of locore */}
rxma_check_errors() {/*called by biinit.c*/}
#endif

#ifdef VAX9000
int  	ka9000nexaddr(); 
int	ka9000memerr();
int	ka9000setcache();
int	ka9000memenable();
int	ka9000cachenbl();
int	ka9000umaddr();
int	ka9000udevaddr();
int	ka9000machcheck();
int	ka9000logsbi();
int	ka9000conf();
int     ka9000startcpu();
int     ka9000stopcpu();
int	ka9000memerr();
int	ka9000setcache();
int	ka9000memenable();
int	ka9000tocons();
int     ka9000cachenbl();
int	ka9000umaddr();
int	ka9000udevaddr();
int	*ka9000_ip;
#else
ka9000halt()  { /* call out of machdep */}
ka9000mapcsr() { /* map xja registers on VAX9000 */}
ka9000reboot()  { /* call out of machdep */}
ka9000_clear_xbe()  { /* call out of machdep */}
ka9000_rxfct_isr()  { /* called out of locore */ }
ka9000_reinit_spu()  { /* called out of cons.c */ }
ka9000_enable_errlog()  { /* called out of kern_errlog.c */ }
ka9000_clear_coldstart() { /* Called during startup */ }
ka9000_clear_warmstart() { /* called during startup */ }
#endif VAX9000

#if defined(VAX6200) || defined(VAX60) || defined(VAX6400) || defined(VAX3600)
int     cca_startcpu();
int     cca_stopcpu();
#else
int     cca_check_input() {/* */}
#endif

#ifdef VVAX
int 	vvaxmachcheck();
int 	vvaxsetcache();
int 	vvaxbadaddr();
int 	vvaxnexaddr();
int 	vvaxumaddr();
int 	vvaxudevaddr();
int 	vvaxdelay();
int	vvaxconf();
int 	vvaxtocons();
int	vvaxreadtodr();
int	vvaxwritetodr();
#else
/* No-op functions for clean link without VVAX option */
#include "../h/time.h"
struct timeval *get_vvax_time() { /* Should never be called */
	static struct timeval its_1970={ 0L, 0L };
	return &its_1970;
} /* keep kernclock.c happy */
int vvaxdump() {} /* keep locore.s happy */
#endif VVAX

#else vax	/* VAXes end here */

#ifdef DS3100
/* We are linking with kn01.o */
extern int	kn01conf();
extern int	kn01delay();
extern int	kn01ackrtclock();
extern int	kn01trap_error();
extern int	kn01memintr();
extern int	kn01flush_cache();
extern int	kn01_clean_icache();
extern int	kn01_clean_dcache();
extern int	kn01_page_iflush();
extern int	kn01_page_dflush();
extern int	kn01wbflush();
extern int	chk_cpe();
extern int	mc146818read_todclk();
extern int	mc146818write_todclk();
extern int	mc146818startclocks();
extern int	mc146818stopclocks();
extern int	mc146818ackrtclock();
extern int	kn01init();
extern int	msize_baddr();
extern int	wbadmemaddr();
extern int	kn01_getspl();
extern int	kn01_whatspl();
#else
/* Resolve dangling references */
int	parityerr;	
pmopen() {}
pmclose() {}
pmstop() {}
pmioctl() {}
#endif


#ifdef DS5100
/* We are linking with kn230.o */
extern int	kn230_conf();
extern int	kn230_delay();
extern int	kn01ackrtclock();
extern int	kn230_trap_error();
extern int	kn230_memintr();
extern int	kn01flush_cache();
extern int	kn01_clean_icache();
extern int	kn01_clean_dcache();
extern int	kn01_page_iflush();
extern int	kn01_page_dflush();
extern int	kn210wbflush();
extern int	chk_cpe();
extern int	mc146818read_todclk();
extern int	mc146818write_todclk();
extern int	mc146818startclocks();
extern int	mc146818stopclocks();
extern int	mc146818ackrtclock();
extern int	kn230_init();
extern int	msize_bitmap();
extern int	wbadmemaddr();
extern int	kn01_getspl();
extern int	kn01_whatspl();
#else
/* Resolve dangling references */
#endif

#ifdef DS5400
/* We are linking with kn210.o */
extern int	chk_cpe();
extern int	kn210conf();
extern int	kn210wbflush();
extern int	kn210trap_error();
extern int	kn210harderrintr();
extern int	kn01flush_cache();
extern int	kn01_clean_icache();
extern int	kn01_clean_dcache();
extern int	kn01_page_iflush();
extern int	kn01_page_dflush();
extern int	ssc_readtodr();
extern int	ssc_writetodr();
extern int	uSSCdelay();
extern int	kn210startrtclock();
extern int	kn210stopclocks();
extern int	bbadaddr();
extern int	msize_baddr();
extern int	kn210init();
extern int	kn01_getspl();
extern int	kn01_whatspl();
#else
/* Add the stubs for dangling references */
int kn210hardintr0() {}
int kn210hardintr1()	{}
int kn210hardintr2() {}
int kn210hardintr3()	{}
int kn210harderrintr() {}
int kn210haltintr() {}
#endif DS5400

#ifdef	DS5800
/* We are linking with kn5800.o */
extern int	chk_cpe();
extern int	kn5800_conf();
extern int	kn5800_flush_cache();
extern int	kn5800_clean_icache();
extern int	kn5800_clean_dcache();
extern int	kn5800_page_iflush();
extern int	kn5800_page_dflush();
extern int	kn5800_enable_cache();
extern int	kn5800badaddr();
extern int	kn5800_intr3();
extern int	kn5800_trap_error();
extern int	kn5800_wbflush();
extern int	kn5800flush_cache();
extern int	uSSCdelay();
extern int	kn5800_start_clock();
extern int	kn5800_stop_clock();
extern int	reprime_ssc_clock();
extern int	kn5800nexaddr();
extern int	kn5800udevaddr();
extern int	kn5800umaddr();
extern int	msize_bitmap();
extern int	kn5800_init();
extern int	cca_startcpu();
extern int	kn01_getspl();
extern int	kn01_whatspl();
extern int	kn5800_init_secondary();
#else	DS5800
struct	v5800csr *v5800csr;
int	*kn5800_wbflush_addr;
int	wbflush_dummy;
nxaccess() {}
kn5800_cpuid() {};
bbcci() {};
bbssi() {};
kn5800_init_secondary() {};
#endif	DS5800

#ifdef DS5000
/* We are linking with kn02.o */
extern int	kn02conf();
extern int	kn02delay();
extern int	kn02trap_error();
extern int	kn02errintr();
extern int	kn01flush_cache();
extern int	kn01_clean_icache();
extern int	kn01_clean_dcache();
extern int	kn01_page_iflush();
extern int	kn01_page_dflush();
extern int	kn01wbflush();
extern int	mc146818read_todclk();
extern int	mc146818write_todclk();
extern int	mc146818startclocks();
extern int	mc146818stopclocks();
extern int	mc146818ackrtclock();
extern int	msize_bitmap();
extern int	kn02init();
extern int	bbadaddr();
extern int	chk_cpe();
extern int	kn01_getspl();
extern int	kn01_whatspl();
extern int      kn02_print_consinfo();
extern int      kn02_log_errinfo();
#else
int kn02erradr;
int kn02chksyn;
#endif DS5000

#if !defined(DS5000) && !defined(DS5000_100)
struct tc_slot		*tc_slot;
struct tc_slotaddr	*tc_slotaddr;
struct tc_sw		*tc_sw;
u_int			*tc_romoffset;
int tc_addr_to_name()	{}
int tc_isolate_memerr()	{}
#endif

#ifdef DS5500
/* We are linking with kn220.o */
extern int	chk_cpe();
extern int	kn220conf();
extern int	kn220wbflush();
extern int	kn220trap_error();
extern int	kn220memintr();
extern int	kn01flush_cache();
extern int	kn01_clean_icache();
extern int	kn01_clean_dcache();
extern int	kn01_page_iflush();
extern int	kn01_page_dflush();
extern int	ssc_readtodr();
extern int	ssc_writetodr();
extern int	uSSCdelay();
extern int	kn220startrtclock();
extern int	kn220stopclocks();
extern int	kn220badaddr();
extern int	msize_bitmap();
extern int	kn220init();
extern int	kn01_getspl();
extern int	kn01_whatspl();
extern int	kn220_print_consinfo();
extern int	kn220_log_errinfo();
#else
/* Add the stubs for dangling references */
int kn220hardintr0() {}
int kn220hardintr1()	{}
int kn220hardintr2() {}
int kn220hardintr3()	{}
int kn220memintr() {}
int kn220haltintr() {}
int kn220badaddr() {}
#endif DS5500

#ifdef DS5000_100
/* We are linking with kn02ba.o */
extern int	kn02ba_conf();
extern int	kn02ba_delay();
extern int	kn02ba_trap_error();
extern int	kn02ba_errintr();
extern int	kn01flush_cache();
extern int	kn01_clean_icache();
extern int	kn01_clean_dcache();
extern int	kn01_page_iflush();
extern int	kn01_page_dflush();
extern int	kn02ba_print_consinfo();
extern int	kn02ba_log_errinfo();
extern int	kn02ba_wbflush();
extern int	mc146818read_todclk();
extern int	mc146818write_todclk();
extern int	mc146818startclocks();
extern int	mc146818stopclocks();
extern int	mc146818ackrtclock();
extern int	msize_bitmap();
extern int	kn02ba_init();
extern int	bbadaddr();
extern int	chk_cpe();
extern int	kn02ba_getspl();
extern int	kn02ba_whatspl();
#else
int kn02ba_exception_exit()	{}
#endif DS5000_100

#endif vax	/* if - else - endif - Add machine support to proper area */


struct cpusw	cpusw[] =
{
#ifdef VAX780
    {	VAX_780,		ka780machcheck,		ka780logsbi,
	ka780memerr,		ka780memenable,		ka780tocons,
	nullcpu,		ka780conf,		ka780cachenbl,	
	nullcpu,		nullcpu,		bbadaddr,
	readtodr,		writetodr,		uICRdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	ka780nexaddr,		ka780umaddr,		ka780udevaddr,
	nullcpu,		nullcpu,		ka780setcache,
	UMEMSIZE780,		1,			10000,
	NNEX780,		NEXSIZE,		0,
	0,			0,			0,
	CPU_ICR },
#endif VAX780

#ifdef VAX750
    {	VAX_750,		ka750machcheck,		nullcpu,
	ka750memerr,		ka750memenable,		ka750tocons,
	nullcpu,		ka750conf,		ka750cachenbl,	
	nullcpu,		nullcpu,		bbadaddr,
	readtodr,		writetodr,		uICRdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	ka750nexaddr,		ka750umaddr,		ka750udevaddr,
	nullcpu,		nullcpu,		ka750setcache,
	UMEMSIZE750,		0,			10000,
	NNEX750,		NEXSIZE,		nexty750,
	0,			0,			0,
	CPU_ICR },
#endif VAX750

#ifdef VAX730
    {	VAX_730,		ka730machcheck,		nullcpu,
	ka730memerr,		ka730memenable,		ka730tocons,
	nullcpu,		ka730conf,		nullcpu,	
	nullcpu,		nullcpu,		bbadaddr,
	readtodr,		writetodr,		uICRdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	ka730nexaddr,		ka730umaddr,		ka730udevaddr,
	nullcpu,		nullcpu,		nullcpu,
	UMEMSIZE730,		0,			10000,
	NNEX730,		NEXSIZE,		nexty730,
	0,			 0,			0,
	CPU_ICR },
#endif VAX730

#ifdef VAX8600
	{ VAX_8600,		ka8600machcheck,	ka8600logsbi,
	ka8600memerr,		ka8600memenable,	ka8600tocons,
	nullcpu,		ka8600conf,		ka8600cachenbl,	
	nullcpu,		nullcpu,		bbadaddr,
	readtodr,		writetodr,		uICRdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	ka8600nexaddr,		ka8600umaddr,		ka8600udevaddr,
	nullcpu,		nullcpu,		ka8600setcache,
	UMEMSIZE8600,		1,			10000,
	NNEX8600,		NEXSIZE,		0,
	NIOA8600,		ioaaddr8600,		IOAMAPSIZ,
	CPU_ICR	},
#endif VAX8600

#ifdef MVAX
    {	MVAX_I,			ka610machcheck,		nullcpu,
	nullcpu,		nullcpu,		ka610tocons,
	nullcpu,		ka610conf,		ka610cachenbl,	
	nullcpu,		nullcpu,		bbadaddr,
	readtodr,		writetodr,		uInoICRdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	ka610nexaddr,		ka610umaddr,		ka610udevaddr,
	nullcpu,		nullcpu,		ka610setcache,
	QMEMSIZEUVI,		1,			10000,
	NNEXUVI,		NEXSIZE,		nextyUVI,
	0,			0,			0,
	0 },

    {	MVAX_II,		ka630machcheck,		nullcpu,
	nullcpu,		nullcpu,		ka630tocons,
	nullcpu,		ka630conf,		ka630cachenbl,	
	nullcpu,		nullcpu,		bbadaddr,
	ka630readtodr,		ka630writetodr,		uIInoICRdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	ka630nexaddr,		ka630umaddr,		ka630udevaddr,
	nullcpu,		nullcpu,		ka630setcache,
	QMEMSIZEUVI,		0,			10000,
	NNEXUVI,		QNEXSIZE,		nextyUVI,
	0,			0,			0,
	0 },

    {	VAXSTAR,		ka630machcheck,		nullcpu,
	nullcpu,		nullcpu,		ka630tocons,
	nullcpu,		ka630conf,		ka630cachenbl,	
	nullcpu,		nullcpu,		bbadaddr,
	ka630readtodr,		ka630writetodr,		uIInoICRdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	ka630nexaddr,		ka630umaddr,		ka630udevaddr,
	nullcpu,		nullcpu,		ka630setcache,
	QMEMSIZEUVI,		0,			10000,
	NNEXUVI,		QNEXSIZE,		nextyUVI,
	0,			0,			0,
	0 },
#endif MVAX

#ifdef VAX8200
    {	VAX_8200,		ka8200machcheck,	nullcpu,
	ka8200memerr,		ka8200memenable,	ka8200tocons,
	nullcpu,		ka8200conf,		ka8200cachenbl,	
	nullcpu,		nullcpu,		bbadaddr,
	ka8200readtodr,		ka8200writetodr,	uICRdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		ka8200startcpu,		ka8200stopcpu,
	ka8200nexaddr,		ka8200umaddr,		ka8200udevaddr,
	nullcpu,		nullcpu,		ka8200setcache,
	UMEMSIZE8200,		1,			10000,
	NNEX8200,		NEXSIZE,		0,
	0,			0,			0,
	CPU_ICR	},
#endif VAX8200

#ifdef VAX8800
    {	VAX_8800,		ka8800machcheck,	nullcpu,
	ka8800memerr,		ka8800memenable,	ka8800tocons,
	nullcpu,		ka8800conf,		ka8800cachenbl,	
	nullcpu,		nullcpu,		ka8800badaddr,
	ka8800readtodr,		ka8800writetodr,	uICRdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		ka8800startcpu,		ka8800stopcpu,
	ka8800nexaddr,		ka8800umaddr,		ka8800udevaddr,
	nullcpu,		nullcpu,		ka8800setcache,
	UMEMSIZE8800,		1,			10000,
	NNEX8800,		NEXSIZE,		0,
	0,			0,			0,
	CPU_ICR	},

    {	VAX_8820,		ka8800machcheck,	nullcpu,
	ka8800memerr,		ka8800memenable,	ka8800tocons,
	nullcpu,		ka8800conf,		ka8800cachenbl,	
	nullcpu,		nullcpu,		ka8800badaddr,
	ka8800readtodr,		ka8800writetodr,	uICRdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		ka8800startcpu,		ka8800stopcpu,
	ka8800nexaddr,		ka8800umaddr,		ka8800udevaddr,
	nullcpu,		nullcpu,		ka8800setcache,
	UMEMSIZE8800,		1,			10000,
	NNEX8800,		NEXSIZE,		0,
	0,			0,			0,
	CPU_ICR	},
#endif VAX8800
 
#ifdef VAX3600
    {	VAX_3400,		ka650machcheck,		ka650memerr,
	ka650crderr,		ka650memenable,		ka650tocons,
	nullcpu,		ka650conf,		ka650cachenbl,	
	nullcpu,		nullcpu,		bbadaddr,
	readtodr,		writetodr,		uSSCdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		ka650umaddr,		ka650udevaddr,
	nullcpu,		nullcpu,		ka650setcache,
	QMEMSIZECVQ,		0,			10000,
	0,			0,			0,
	0,			0,			0,
	0 },
 
    {	VAX_3600,		ka650machcheck,		ka650memerr,
	ka650crderr,		ka650memenable,		ka650tocons,
	nullcpu,		ka650conf,		ka650cachenbl,	
	nullcpu,		nullcpu,		bbadaddr,
	readtodr,		writetodr,		uSSCdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		ka650umaddr,		ka650udevaddr,
	nullcpu,		nullcpu, 		ka650setcache,
	QMEMSIZECVQ,		0,			10000,
	0,			0,			0,
	0,			0,			0,
	0 },
 
    {	VAX_3900,		ka650machcheck,		ka650memerr,
	ka650crderr,		ka650memenable,		ka650tocons,
	nullcpu,		ka650conf,		ka650cachenbl,	
	nullcpu,		nullcpu,		bbadaddr,
	readtodr,		writetodr,		uSSCdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		ka650umaddr,		ka650udevaddr,
	nullcpu,		nullcpu,		ka650setcache,
	QMEMSIZECVQ,		0,			10000,
	0,			0,			0,
	0,			0,			0,
	0 },
#endif VAX3600
 
#ifdef VAX6200
    {	VAX_6200,		ka6200machcheck,	ka6200memerr,
	ka6200crderr,		ka6200memenable,	ka6200tocons,
	nullcpu,		ka6200conf,		ka6200cachenbl,	
	nullcpu,		nullcpu,		ka6200badaddr,
	readtodr,		writetodr,		uSSCdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		cca_startcpu,		cca_stopcpu,
	ka6200nexaddr,		ka6200umaddr,		ka6200udevaddr,
	nullcpu, 		nullcpu,		ka6200setcache,
	UMEMSIZE8200,		1,			10000,
	NNEX8200,		NEXSIZE,		0,
	0,			0,			0,
	0 },
#endif VAX6200

#ifdef VAX420
    {	C_VAXSTAR,		ka420machcheck,		nullcpu,
	ka420crderr,		nullcpu,		ka420tocons,
	nullcpu,		ka420conf,		ka420cachenbl,	
	nullcpu,		nullcpu,		bbadaddr,
	ka420readtodr,		ka420writetodr,		cVSnoICRdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	ka420nexaddr,		ka420umaddr,		nullcpu,
	nullcpu,		nullcpu,		ka420setcache,
	0,			0,			10000,
	NNEXUVI,		QNEXSIZE,		nextyUVI,
	0,			0,			0,
	0 },
#endif VAX420

#ifdef VAX60
    {	VAX_60,			ka60machcheck,		nullcpu,
	ka60crderr,		ka60memenable,		ka60tocons,
	nullcpu,		ka60conf,		ka60cachenbl,	
	nullcpu,		nullcpu,		bbadaddr,
	ka60readtodr,		ka60writetodr,		uSSCdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		cca_startcpu,		cca_stopcpu,
	ka60nexaddr,		ka60umaddr,		ka60udevaddr,
	nullcpu,		nullcpu,		ka60setcache,
	QMEMSIZECVQ,		0,			10000,
	0,			0,			0,
	0,			0,			0,
	0 },
#endif VAX60

#ifdef VAX9000
    {	VAX_9000,		ka9000machcheck,	ka9000logsbi,
	ka9000memerr,		ka9000memenable,	ka9000tocons,
	nullcpu,		ka9000conf,		ka9000cachenbl,	
	nullcpu,		nullcpu,		bbadaddr,
	readtodr,		writetodr,		uICRdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		ka9000startcpu,		ka9000stopcpu,
	ka9000nexaddr,		ka9000umaddr,		ka9000udevaddr,
	nullcpu,		nullcpu,		ka9000setcache,
	UMEMSIZE9000,		1,			10000,
	64,			NEXSIZE,		0,
	0,			0,			0,
	CPU_ICR	},
#endif VAX9000

#ifdef DS3100
    {	DS_3100,		kn01trap_error,		kn01memintr,
	nullcpu,		chk_cpe,		nullcpu,
	nullcpu,		kn01conf,		nullcpu,
	nullcpu,		kn01flush_cache,	wbadmemaddr,
	mc146818read_todclk,	mc146818write_todclk,	kn01delay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	kn01wbflush,		mc146818startclocks,  mc146818stopclocks,
	mc146818ackrtclock,	kn01init,		msize_baddr,
	kn01_clean_icache,	kn01_clean_dcache,	kn01_page_iflush,
	kn01_page_dflush,	kn01_getspl,
	kn01_whatspl,		0,			0,
	20000,			256,			(1 << 26),
	1,			0 },
#endif DS3100

#ifdef DS5100
    {	DS_5100,		kn230_trap_error,	kn230_memintr,
	nullcpu,		chk_cpe,		nullcpu,
	nullcpu,		kn230_conf,		nullcpu,
	nullcpu,		kn01flush_cache,	wbadmemaddr,
	mc146818read_todclk,	mc146818write_todclk,	kn230_delay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	kn210wbflush,		mc146818startclocks,  mc146818stopclocks,
	mc146818ackrtclock,	kn230_init,		msize_bitmap,
	kn01_clean_icache,	kn01_clean_dcache,	kn01_page_iflush,
	kn01_page_dflush,	kn01_getspl,
	kn01_whatspl,		0,			0,
	20000,			256,			(1 << 26),
	1,			0 },
#endif DS5100

#ifdef DS5400
#define DS5400_FLAGS	(SCS_START_SYSAPS | MSCP_POLL_WAIT)
    {	DS_5400,		kn210trap_error,	kn210harderrintr,
	nullcpu,		chk_cpe,		nullcpu,
	nullcpu,		kn210conf,		nullcpu,
	nullcpu,		kn01flush_cache,	bbadaddr,
	ssc_readtodr,		ssc_writetodr,		uSSCdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	kn210wbflush,		kn210startrtclock,	kn210stopclocks,
	nullcpu,		kn210init,		msize_baddr,
	kn01_clean_icache,	kn01_clean_dcache,	kn01_page_iflush,
	kn01_page_dflush,	kn01_getspl,
	kn01_whatspl,		0,			0,
	50000,			100,			(1 << 28),
	100,			DS5400_FLAGS },
#endif DS5400

#ifdef DS5800
#define DS5800_FLAGS	(SCS_START_SYSAPS | MSCP_POLL_WAIT)
    {	DS_5800,		kn5800_trap_error,	kn5800_intr3,
	nullcpu,		chk_cpe,		nullcpu,
	nullcpu,		kn5800_conf,		kn5800_enable_cache,
	nullcpu,		kn5800_flush_cache,	kn5800badaddr,
	ssc_readtodr,		ssc_writetodr,		uSSCdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		cca_startcpu,		nullcpu,
	kn5800nexaddr,		kn5800umaddr,		kn5800udevaddr,
	nullcpu,		nullcpu,		nullcpu,
	kn5800_wbflush,		kn5800_start_clock,	kn5800_stop_clock,
	reprime_ssc_clock,	kn5800_init,		msize_bitmap,
	kn5800_clean_icache,	kn5800_clean_dcache,	kn5800_page_iflush,
	kn5800_page_dflush,	kn01_getspl,
	kn01_whatspl,		UMEMSIZE8800,		0,
	10000,			100,			(1 << 28),
	100,			DS5800_FLAGS },
#endif DS5800

#ifdef DS5000
    {	DS_5000,		kn02trap_error,		kn02errintr,
	nullcpu,		chk_cpe,		nullcpu,
	nullcpu,		kn02conf,		nullcpu,
	nullcpu,		kn01flush_cache,	bbadaddr,
	mc146818read_todclk,	mc146818write_todclk,	kn02delay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	kn02_print_consinfo,	kn02_log_errinfo,	nullcpu,
	kn01wbflush,		mc146818startclocks,	mc146818stopclocks,
	mc146818ackrtclock,	kn02init,		msize_bitmap,
	kn01_clean_icache,	kn01_clean_dcache,	kn01_page_iflush,
	kn01_page_dflush,	kn01_getspl,
	kn01_whatspl,		0,			0,
	20000,			256,			(1 << 26),
	1,			0 },
#endif DS5000

#ifdef VAX6400
    {	VAX_6400,		ka6400machcheck,	ka6400harderr,
	ka6400softerr,		ka6400memenable,	ka6400tocons,
	nullcpu,		ka6400conf,		ka6400cachenbl,
	nullcpu,		nullcpu,		ka6400badaddr,
	readtodr,		writetodr,		uRSSCdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		cca_startcpu,		cca_stopcpu,
	ka6400nexaddr,		ka6400umaddr,		ka6400udevaddr,
	nullcpu,		nullcpu,		ka6400setcache,
	UMEMSIZE8200,		1,			10000,
	NNEX8200,		NEXSIZE,		0,
	0,			0,			0,
	0	},
#endif VAX6400

#ifdef VVAX
    {	V_VAX,			vvaxmachcheck,		nullcpu,
	nullcpu,		nullcpu,		vvaxtocons,
	nullcpu,		vvaxconf,		nullcpu,
	nullcpu,		nullcpu,		vvaxbadaddr,
	vvaxreadtodr,		vvaxwritetodr,		vvaxdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	vvaxnexaddr,		vvaxumaddr,		vvaxudevaddr,
	nullcpu,		nullcpu,		vvaxsetcache,
	0,			0,			10000,
	0,			0,			0,
	0,			0			0,
	0 },
#endif VVAX
 
#ifdef DS5500
#define DS5500_FLAGS	(SCS_START_SYSAPS | MSCP_POLL_WAIT)
    {	DS_5500,		kn220trap_error,	kn220memintr,
	nullcpu,		chk_cpe,		nullcpu,
	nullcpu,		kn220conf,		nullcpu,
	nullcpu,		kn01flush_cache,	kn220badaddr,
	ssc_readtodr,		ssc_writetodr,		uSSCdelay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	kn220_print_consinfo,	kn220_log_errinfo,	nullcpu,
	kn220wbflush,		kn220startrtclock,	kn220stopclocks,
	nullcpu,		kn220init,		msize_bitmap,
	kn01_clean_icache,	kn01_clean_dcache,	kn01_page_iflush,
	kn01_page_dflush,	kn01_getspl,
	kn01_whatspl,		0,			0,
	50000,			100,			(1 << 28),
	100,			DS5500_FLAGS },
#endif DS5500

#ifdef DS5000_100
    {	DS_5000_100,		kn02ba_trap_error,	kn02ba_errintr,
	nullcpu,		chk_cpe,		nullcpu,
	nullcpu,		kn02ba_conf,		nullcpu,
	nullcpu,		kn01flush_cache,	bbadaddr,
	mc146818read_todclk,	mc146818write_todclk,	kn02ba_delay,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	nullcpu,		nullcpu,		nullcpu,
	kn02ba_print_consinfo,	kn02ba_log_errinfo,	nullcpu,
	kn02ba_wbflush,		mc146818startclocks,	mc146818stopclocks,
	mc146818ackrtclock,	kn02ba_init,		msize_bitmap,
	kn01_clean_icache,	kn01_clean_dcache,	kn01_page_iflush,
	kn01_page_dflush,	kn02ba_getspl,
	kn02ba_whatspl,		0,			0,
	20000,			256,			(1 << 26),
	1,			0 },
#endif DS5000_100

	/*
	 * We have to be able to find the end of the table
	 */
    {	0,			0,			0,
	0,			0,			0,
	0,			0,			0,
	0,			0,			0,
	0,			0,			0,
	0,			0,			0,
	0,			0,			0,
	0,			0,			0,
	0,			0,			0,
	0 }
};

/*
 * When this routine is called, we are doing something wrong.
 */
nocpu()
{
	return (-1);
}

/*
 * null routine to pass back a success since this cpu type
 * doesn't need one of these routines.
 */
nullcpu()
{
	return(0);
}
