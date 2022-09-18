/*
 * spt.s - system page table.
 */

/*	
 * @(#)spt.s	4.3	(ULTRIX)	10/10/90	
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
 ************************************************************************
 * Modification History:
 *
 * 9-Sept-90	U. Sinkewicz
 *	Wide area driver changes (from NAC).
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 *	Added support for VAX9000.
 *
 * 06-Jun-90 -- Pete Keilty
 *	Modified XMI_map node space size to be 17k because of CIKMF needs.
 *
 * 01-May-09 -- Joe Szczypek
 *      Added MDA register mapping for XMP support.
 *
 * 08-Dec-89 -- Pete Keilty
 *	Modified XMI_map node space size to be 16k because of CIXCD needs.
 *
 * 02-Dec-89 -- Fred Canter
 *	Added SYSMAP for "sp" psuedo driver for user devices.
 *
 * 09-Nov-89 -- jaw
 *	ifdef pte's for CI network.
 *
 * 17-Oct-89 -- Mark A. Parenti
 *	Removed LYNX support.
 *
 * 8-May-89 -- Giles Atkinson
 *	Expand sgsys area to three pages
 *
 * 5-May-89 -- Adrian Thoms
 *      Added entry VVAXiomap to map virtual VAX I/O communication area
 *
 * 09-Feb-89 -- jaw
 *	fix undefines in RIGEL support.
 *
 * 10-Jan-89	kong
 *	Added Rigel (VAX6400) support.
 *
 * 27-Sep-88	darrell
 *	Changed the name of ctsia to ctsi, and fgctsistate to 
 *	fgctsixs, and FGCTSISTATEmap to FGCTSIXSmap.
 *
 * 01-Sep-88	darrell
 * 	Changed the name of ctsi to ctsia.
 *
 * 18-Jul-88	jaa
 *	Added flag to SYSMAP macro for makespt 
 *	added eUsrptmap[CLSIZE] guard pages to User page table map
 *	
 * 19-May-88 -- fred (Fred Canter)
 *	Added SYSMAP for CVAXstar/PVAX extended I/O mode 128K data buffer.
 *	Added SYSMAP for SCSI driver I/O space registers.
 *
 * 26-Apr-88 -- depp
 *	Added 2 ptes to usrptmap to prevent inadvertent rmalloc() one past
 *	the end of usrptmap (into forkmap).
 *
 * 26-Apr-88    jaw
 *	Add VAX8820 support.
 *
 * 01-Mar-88 -- templin
 *	Fixed spt allocation for the CVQNImap
 *
 * 15-Feb-88 -- fred (Fred Canter)
 *	Added map for CVAXstar/PVAX 2nd level cache data storage
 *	for cache initialization.
 * 06-Feb-88	Robin
 *	Added entries to map NI and DSSI areas for ka640 (Mayfair-II)
 *
 * 19-Jan-88 -- jaa
 *	added changes to move spt to bss
 *
 * 19-Jan-88 -- jaw
 *	added changes needed for calypso CVAX.
 *
 * 18-Jan-88	lp
 *	Removed Mbutl map (now use allocated mbufs).
 *
 * 12-15-87	Larry C.
 *		include LYNX ptes if NLX greater than 0
 *
 * 12-11-87	Robin L. and Larry C.
 *      Added portclass/kmalloc support to the system.
 *
 * 26-Sep-87 - Ricky Palmer (rsp)
 *	Upped vmbinfosz SYSMAP entry to 128*CLSIZE so ci boot will
 *	work.
 *
 * 16-July-87  -- Mark Longo,Larry Cohen
 *      Added entries for mapping the LYNX graphic subsystem
 *      Add entries for network sysap - Larry
 *  3-Aug-87 -- rafiey (Ali Rafieymehr)
 *	Added mapping for VAXstation 2000 system scratch RAM.
 *
 * 27-Jul-87 -- afd
 *	Added mapping for Mayfair/CVAX local ROM space so we can get
 *	to the sys_type register.
 *
 * 20-Apr-87 -- afd
 *	Added mapping for Mayfair/CVAX InterProcessor Com Regs
 *	Changed name CVAXQ to VAX3600 for Mayfair.
 *
 * 06-Mar-87 -- afd
 *	Added mapping for Mayfair/CVAX local register space, CPMBX,
 *	and cache flush.
 *
 * 12-Feb-87 -- depp
 *	changed sizing of dmempt
 *
 * 13-Dec-86  -- Fred Canter and Ali Rafieymehr
 *	Changed sh and sg SYSMAPs respectively.
 *
 * 27-Aug-86  -- Fred Canter
 *	Removed unnecessary comments.
 *  5-Aug-86  -- Darrell Dunnuck
 *	Added mapping for buffer in the VAXstar TZK50 driver.
 *
 *  2-Jul-86  -- fred (Fred Canter)
 *	Added mapping for TEAMmate 8 line SLU registers.
 *
 * 18-Jun-86  -- fred (Fred Canter)
 *	Changed for VAXstar kernel support.
 *
 * 5-Jun-86   -- jaw 	changes to config.
 *
 * 15-Apr-86 -- afd
 *	Added QMEMmap and qmem for microVAX QBUS space.
 *	MicroVAX nexus space is faked by claming that a MicroVAX has
 *	32 nexi (in nexus.h) so that we get 512 ptes for nexus space for
 *	the QBUS.  This should be made QBUS dependant not cpu dependant.
 *
 * 14-Apr-86 -- jaw  remove MAXNUBA referances.....use NUBA only!
 *
 * 02-Apr-86 -- jaw  add support for nautilus console and memory adapter
 *
 * 10-mar-86 -- tresvik
 *	added support for SAS memory device
 *
 * 05 Mar 86 -- bjg
 *	Removed SYSMAP for msgbufmap; (replaced with error logger)
 *
 * 24 Feb 86 -- depp
 *	Added in kernel memory allocation map dmemptmap
 *
 * 04-feb-86 -- tresvik
 *	added VMB boot path support
 *
 * 04-feb-86 -- jaw  add mapping for 8800 i/o adpters.
 *
 * 19-Jun-85 -- jaw
 *	VAX8200 name change.
 *
 * 18-Jun-85 -- jaw
 *	Reserve some map space for Rx50 driver for VAX8200.
 *
 * 13-Mar-85 -jaw
 *	Changes for support of the VAX8200 were merged in.
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 * 03 Nov 84 -- rjl
 *	MicroVAX-II needs to map 4 megabytes of space on the Q-BUS plus
 *	8k for the I/O page instead of 256k bytes on the UNIBUS. The extra
 *	map registers for the I/O page are necessary because it is not
 *	part of the memory space like UNIBUS adapters.
 *
 *	It also needs to map a 256k `nexus' space for the local registers.
 */
#include "uba.h"
#include "klesib.h"
#include "../h/param.h"
#include "../machine/vmparam.h"
#include "vaxbi.h"
#include "scsnet.h"
#include "../io/uba/spreg.h"
#include "sp.h"


#if defined (VAX8600) || defined (MVAX) || defined (VAX420)
#define MAXNNEXUS 32
#else
#if defined (VAX780) || defined (VAX750) || defined (VAX730)
#define MAXNNEXUS 16
#else 
/* vax3600 vax8200 vax6200 */
#define MAXNNEXUS 0
#endif VAX3600
#endif VAX8600


/*
 * System page table
 */ 
#define	SYSMAP(mname, vname, npte)	SPT	mname	vname	npte

	SYSMAP(Sysmap	,Sysbase	,SYSPTSIZE	)
	SYSMAP(UMBAbeg	,umbabeg	,0		)
/*
	All PTE's used to map I/O space MUST be below this
	line.  If not, "/dev/kmem" algorithm for making
	sure a user cannot probe a non-existent address might
	break.
 */
/*
 * MAXNNEXUS is defined as 32 for MicroVAXen above
 * so that we get 512 ptes for nexus space for MicroVAX.
 * This should be made QBUS dependant, not cpu dependant.
 * VAXstation 2000 and MicroVAX 2000 share these maps with Q-bus CPUs.
 * CVAXstar also shares these maps and adds one of its own.
 */
#if (16*CVAXBI) > MAXNNEXUS
	SYSMAP(Nexmap	,nexus		,16*16*CVAXBI	)
#else
	SYSMAP(Nexmap	,nexus		,16*MAXNNEXUS	)
#endif
	SYSMAP(UMEMmap	,umem		,512*NUBA	)
	SYSMAP(KBMEMmap	,kbmem		,512*NKLESIB	)
#if defined (MVAX) || defined (VAX3600) || defined (VAX420) || defined (VAX60)
	SYSMAP(QMEMmap	,qmem		,(8192+16)	)
#else
	SYSMAP(QMEMmap	,qmem		,0		)
#endif
#if defined (VAX420)
	SYSMAP(SZMEMmap	,szmem		,2		)
#else
	SYSMAP(SZMEMmap	,szmem		,0		)
#endif
#if defined (MVAX) || defined (VAX420)
	SYSMAP(NMEMmap	,nmem		,256		)
	SYSMAP(sdbufmap	,SD_bufmap	,129		)
	SYSMAP(stbufmap	,ST_bufmap	,33		)
	SYSMAP(SGMEMmap	,sgmem		,192		)
	SYSMAP(SGSYSmap	,sgsys		,3		)
	SYSMAP(sgbufmap	,SG_bufmap	,129		)
	SYSMAP(SHMEMmap	,shmem		,1		)
#ifdef WDD
#if defined(MVAX) || defined(VAX420)
#include "dsh.h"
#if NDSH > 0
	SYSMAP(DSHMEMmap  ,dshmem	,2		)
	SYSMAP(DSHSRAMmap  ,dshsram	,64		)
#endif
#endif
#endif WDD
	SYSMAP(CVSCACHEmap ,cvscachemem	,64		)
	SYSMAP(CVSEDDBmap ,cvseddbmem	,256		)
#else
	SYSMAP(NMEMmap	,nmem		,0		)
	SYSMAP(sdbufmap	,SD_bufmap	,0		)
	SYSMAP(stbufmap	,ST_bufmap	,0		)
	SYSMAP(SGMEMmap	,sgmem		,0		)
	SYSMAP(SGSYSmap	,sgsys		,0		)
	SYSMAP(sgbufmap	,SG_bufmap	,0		)
	SYSMAP(SHMEMmap	,shmem		,0		)
	SYSMAP(CVSCACHEmap ,cvscachemem	,0		)
	SYSMAP(CVSEDDBmap ,cvseddbmem	,0		)
#endif MVAX || VAX420
/*
 * Page table maps for MV2000/MV3100 user device pseudo driver.
 */
#if defined (MVAX) || defined (VAX420)
#if NSP > 0
	SYSMAP(SPCSRmap	,spcsr		,SPCSR_PAGES	)
	SYSMAP(SPROMmap	,sprom		,SPROM_PAGES	)
	SYSMAP(SPIOSmap	,spios		,SPIOS_PAGES	)
#else
	SYSMAP(SPCSRmap	,spcsr		,0		)
	SYSMAP(SPROMmap	,sprom		,0		)
	SYSMAP(SPIOSmap	,spios		,0		)
#endif
#else
	SYSMAP(SPCSRmap	,spcsr		,0		)
	SYSMAP(SPROMmap	,sprom		,0		)
	SYSMAP(SPIOSmap	,spios		,0		)
#endif MVAX || VAX420
#if defined(VAX6200) || defined(VAX3600) || defined(VAX60)
	SYSMAP(CVQSSCmap,cvqssc		,3		)
#else
	SYSMAP(CVQSSCmap,cvqssc		,0		)
#endif
#if defined(VAX9000)
	SYSMAP(XJAmap	,xja_mem	,4		)
#else
	SYSMAP(XJAmap	,xja_mem	,0		)
#endif
	SYSMAP(V6200csr	,v6200csr	,1		)
#if defined(VAX6200) || defined(VAX60) || defined(VAX6400)
	SYSMAP(CCAmap	,ccabase	,12		)
#else
	SYSMAP(CCAmap	,ccabase	,0		)
#endif VAX6200 || VAX60 || VAX6400
#ifdef VAX3600
	SYSMAP(CVQMERRmap,cvqmerr	,1		)
	SYSMAP(CVQCBmap	,cvqcb		,1		)
	SYSMAP(CVQCACHEmap,cvqcache	,128		)
	SYSMAP(CVQIPCRmap,cvqipcr	,1		)
	SYSMAP(CVQROMmap,cvqrom		,1		)
#else
	SYSMAP(CVQMERRmap,cvqmerr	,0		)
	SYSMAP(CVQCBmap	,cvqcb		,0		)
	SYSMAP(CVQCACHEmap,cvqcache	,0		)
	SYSMAP(CVQIPCRmap,cvqipcr	,0		)
	SYSMAP(CVQROMmap,cvqrom		,0		)
#endif VAX3600
#if defined (VAX3600) || defined (VAX60)
	SYSMAP(CVQBMmap	,cvqbm		,68		)
	SYSMAP(CVQNImap,cvqni		,258		)
	SYSMAP(CVQMSImap,cvqmsi		,1		)
	SYSMAP(CVQMSIRBmap,cvqmsirb	,256		)
#else
	SYSMAP(CVQBMmap	,cvqbm		,0		)
	SYSMAP(CVQNImap,cvqni		,0		)
	SYSMAP(CVQMSImap,cvqmsi		,0		)
	SYSMAP(CVQMSIRBmap,cvqmsirb	,0		)
#endif VAX3600 || VAX60
#if defined (VAX60)
	SYSMAP(FFCONSmap,ffcons		,1		)
	SYSMAP(FFQREGmap,ffqregs	,1		)
	SYSMAP(FFCROMmap,ffcrom		,1		)
	SYSMAP(FFIOmap,ffiom		,8		)
	SYSMAP(FFIOCSRmap,ffiocsr	,1		)
	SYSMAP(CTSImap,ctsi		,1		)
	SYSMAP(FGCTSIXSmap,fgctsixs	,2		)
        SYSMAP(FGMEMmap ,fgmem          ,65536          )
	SYSMAP(FFFQAMCSRmap, fqamcsr	,1		)
#else
	SYSMAP(FFCONSmap,ffcons		,0		)
	SYSMAP(FFQREGmap,ffqregs	,0		)
	SYSMAP(FFCROMmap,ffcrom		,0		)
	SYSMAP(FFIOmap,ffiom		,0		)
	SYSMAP(FFIOCSRmap,ffiocsr	,0		)
	SYSMAP(CTSImap,ctsi		,0		)
	SYSMAP(FGCTSIXSmap,fgctsixs	,0		)
        SYSMAP(FGMEMmap ,fgmem          ,0              )
	SYSMAP(FFFQAMCSRmap, fqamcsr	,0		)
#endif VAX60
	SYSMAP(Ioamap	,ioa		,MAXNIOA	)
#ifdef VAX8200
	SYSMAP(V8200wmap ,v8200watch	,1		)
	SYSMAP(V8200pmap ,v8200port	,1		)
	SYSMAP(V8200rmap ,v8200rx50	,1		)
#endif
#if defined(VAX8200) || defined(VAX8800)
	SYSMAP(rxbufmap	,RX_bufmap	,129		)
#endif
#ifdef VAX8800
	SYSMAP(V8800mem	,v8800mem	,1		)
	SYSMAP(ADPT_loopback	,adpt_loopback	,4	)
	SYSMAP(Nbia	,nbia_addr 	,3		)
	SYSMAP(STAR_csr_map,star_csr	,4		)
	SYSMAP(NEMO_csr_map,nemo_csr	,4		)
#endif VAX8800
#ifdef VAX6200
	SYSMAP(KA6200_ip_map,ka6200_ip_addr	,8	)
#endif VAX6200
#ifdef VAX6400
	SYSMAP(calypso_ip_map,calypso_ip_addr	,8	)
	SYSMAP(RSSCmap,rssc,3)
	SYSMAP(MDAmap,mda,1)
#else
	SYSMAP(RSSCmap,rssc,0)
#endif
#ifdef VVAX
        SYSMAP(VVAXiomap, VVAXio,       200             )
        SYSMAP(VVAXiomap_end, VVAXio_end        ,0      )
#endif VVAX

#include "xmi.h"
#if NXMI > 0
/* 16 nodes * 17k per * 2 ptes per 1k */
	SYSMAP(XMI_map	,xmi_start	,544*NXMI	)
#endif
#ifdef KDEBUG
	SYSMAP(Kdbmap	,kdbusr		,UPAGES		)
#endif KDEBUG

#if NSCSNET > 0
#define SCS_PTES (16+1)*5*17*2 /* sites*max_per_site*xfer_size*2 */
        SYSMAP(scsmemptmap,scsmempt     ,SCS_PTES       )
#else
        SYSMAP(scsmemptmap,scsmempt     ,0              )
#endif
/*
	All PTE's used to map I/O space MUST be above this
	line.  If not, "/dev/kmem" algorithm for making
	sure a user cannot probe a non-existent address might
	break.
 */

	SYSMAP(UMBAend	,umbaend	,0		)
	SYSMAP(Usrptmap	,usrpt		,USRPTSIZE+CLSIZE	)
	SYSMAP(eUsrptmap,eusrpt		,CLSIZE	)
	SYSMAP(Xswapmap	,xswaputl	,UPAGES		)
	SYSMAP(Xswap2map,xswap2utl	,UPAGES		)
	SYSMAP(Swapmap	,swaputl	,UPAGES		)
	SYSMAP(Pushmap	,pushutl	,UPAGES		)
	SYSMAP(Vfmap	,vfutl		,UPAGES		)
	SYSMAP(CMAP1	,CADDR1		,1		)
	SYSMAP(CMAP2	,CADDR2		,1		)
	SYSMAP(mcrmap	,mcr		,1		)
	SYSMAP(mmap	,vmmap		,1		)
	SYSMAP(kmempt	,kmembase	,KMEMMAP)
	SYSMAP(ekmempt	,kmemlimit	,2		)
	SYSMAP(vmbinfomap,vmb_info	,VMBINFOPAGES	)
#ifdef SAS
	SYSMAP(mdbufmap	,MD_bufmap	,129		)
#endif SAS
