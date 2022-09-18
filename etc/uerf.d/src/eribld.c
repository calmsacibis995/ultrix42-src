#ifndef lint
static char sccsid[]  =  "@(#)eribld.c	4.12   (ULTRIX)   3/6/91";
#endif  lint

/*
*	.TITLE	ERIBLD - Builds the EIMS standard event record
*	.IDENT	/1-001/
*
* COPYRIGHT (C) 1986 DIGITAL EQUIPMENT CORP.,
* CSSE SOFTWARE ENGINEERING
* MARLBOROUGH, MASSACHUSETTS
*
* THIS SOFTWARE IS FURNISHED UNDER A LICENSE FOR USE ONLY ON A 
* SINGLE COMPUTER SYSTEM AND MAY BE COPIED ONLY WITH THE INCLUSION
* OF THE ABOVE COPYRIGHT NOTICE.  THIS SOFTWARE,  OR ANY OTHER
* COPIES THEREOF, MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE
* TO ANY OTHER PERSON EXCEPT FOR USE ON SUCH SYSTEM AND TO ONE WHO
* AGREES TO THESE LICENSE TERMS.  TITLE TO AND OWNERSHIP OF THE
* SOFTWARE SHALL AT ALL TIMES REMAIN IN DEC.
*
* THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT
* NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL
* EQUIPMENT CORPORATION.
*
* DEC ASSUMES NO RESPONSIBILITY FOR THE USE OR RELIABILITY OF
* ITS SOFTWARE ON EQUIPMENT WHICH IS NOT SUPPLIED BY DEC.
*
*++
*
* FACILITY:		FMA - Event Information Management System
*
* ABSTRACT:
*
*	This module sets up the segment buffers and calls the transformation
*	engine to build each segment of the standard record.
*	
* ENVIRONMENT:	VAX/VMS C,  ULTRIX-32 C
*
* AUTHOR:  Bob Winant,  CREATION DATE:  11-Dec-85
*
* MODIFIED BY:
*
*   5/2/89	Lisa Hartman	Add DEBNI and DEMNA support
*
*   3/13/89	Lisa Hartman	Change msi and add scs support to correspond
*				with errlog.h changes.
*
*   2/2/88	Luis Arce	Change mscp pointers to use mscp_msg.h
*				add device support
*
*   8/9/88	Luis Arce	Changed to allow for MIPS machines.
*				if compiled on mips - only mips stuff
*				if compiled on vax  - both mips and vax.
*
*--
*/



#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/errlog.h>
#include <machine/cpu.h>
#include "eims.h"
#include "erms.h"
#include "select.h"
#include "generic_dsd.h"
#include "std_dsd.h"
#include "os_dsd.h"
#include "eiliterals.h"
#include "ueliterals.h"


/*
*++
*=
*=
*=MODULE FLOW - eribld.c
*=
*=  a - ei$bld(seg_ptrs...,os_rec_ptr)              Processes record items
*=          ini_seg(seg_ptr)                                (eixfrm.c)
*=          os_std(item_ptr,os_item_id, std_item_id)        (eixfrm.c)
*=          mscp_bld(seg_ptrs...,os_rec_ptr)                (* b)
*=          scsi_bld(seg_ptrs...,os_rec_ptr)                (* c)
*=          make_addr_vector(rec_ptr,data_ptr)              (* f)
*=
*=  b - mscp_bld(seg_ptrs...,os_rec_ptr)            Processes mscp items
*=          ini_seg(seg_ptr)                                (eixfrm.c)
*=          os_std(item_ptr,os_item_id, std_item_id)        (eixfrm.c)
*=          make_addr_vector(rec_ptr,data_ptr)              (* f)
*=
*=  c - scsi_bld(seg_ptrs...,os_rec_ptr)            Processes scsi items
*=          ini_seg(seg_ptr)                                (eixfrm.c)
*=          os_std(item_ptr,os_item_id, std_item_id)        (eixfrm.c)
*=          make_addr_vector(rec_ptr,data_ptr)              (* f)
*=
*=  d - bld_corrupt_eis(eis_ptr)                    Sets eis_values to indicate
*=                                                  corrupt record
*=
*=  e - ads_bld(ads_ptr,os_rec_ptr)                 Processes ads_segment
*=          ini_seg(seg_ptr)                                (eixfrm.c)
*=          os_std(item_ptr,os_item_id, std_item_id)        (eixfrm.c)
*=          make_addr_vector(rec_ptr,data_ptr)              (* f)
*=          mscp_ta90(ads_ptr,os_rec_ptr)                   (* g)
*=
*=  f - make_addr_vector(rec_ptr,data_ptr)          Makes addr vector item
*=
*=  g - mscp_ta90(ads_ptr,os_rec_ptr)               Processes ta90 items
*=          ini_seg(seg_ptr)                                (eixfrm.c)
*=          os_std(item_ptr,os_item_id, std_item_id)        (eixfrm.c)
*=          make_addr_vector(rec_ptr,data_ptr)              (* f)
*=
*--
*/


/**************   DEFINITIONS COMMON TO THIS MODULE  **************/

/******************  DEFINE CI MACROS ********************************/

#define	Elcibiregs(p)	   ((struct bi_regs *)p)
#define	Elciciregs(p)	   ((struct ci_regs *)p)
#define	Elcicommon(p)	   (&p->el_body.elci.cicommon)
#define	Elcicpurevlev(p)   ((struct ci_cpurevlev *)p)
#define	Elcidattn(p)	   (&p->el_body.elci.citypes.cidattn)
#define	Elcilcommon(p)	   (&p->el_body.elci.citypes.cilpkt.cilcommon)
#define	Elcipacket(p)	   (&p->el_body.elci.citypes.cilpkt.cilpktopt.cipacket)
#define	Elcirevlev(p)	   ((struct ci_revlev *)p)
#define	Elciucode(p)	   ((struct ci_ucode *)p)
#define	Elcixmiregs(p)	   ((struct cixmi_regs *)p)

#define	Elcippdcommon(p)   (&p->el_body.elcippd.cippdcommon)
#define	Elcippddbcoll(p)   (&p->el_body.elcippd.cippdtypes.cippdpath.cippdpathopt.cippddbcoll)
#define	Elcippdnewpath(p)  (&p->el_body.elcippd.cippdtypes.cippdpath.cippdpathopt.cippdnewpath)
#define	Elcippdpcommon(p)  (&p->el_body.elcippd.cippdtypes.cippdpath.cippdpcommon)
#define	Elcippdppacket(p)  (&p->el_body.elcippd.cippdtypes.cippdpath.cippdpathopt.cippdppacket)
#define	Elcippdprotocol(p) (&p->el_body.elcippd.cippdtypes.cippdsystem.cippdsystemopt.cippdprotocol)
#define	Elcippdscommon(p)  (&p->el_body.elcippd.cippdtypes.cippdsystem.cippdscommon)
#define	Elcippdspacket(p)  (&p->el_body.elcippd.cippdtypes.cippdsystem.cippdsystemopt.cippdspacket)
#define	Elcippdsysapnam(p) (&p->el_body.elcippd.cippdtypes.cippdpath.cippdpathopt.cippd_sysap[0])

#define U_MSCP      el_body.elbdev.eldevdata.elmslg.mscp_mslg

#define U_SCSI 	    el_body.elbdev.eldevdata.elscsi

#define Elscscommon(p)	   (&p->el_body.elscs.scscommon)
#define Elscsopt(p)	   (&p->el_body.elscs.scsopt)

#define Elmsicommon(p)	   (&p->el_body.elmsi.msicommon)
#define Elmsiregs(p)	   (&p->el_body.elmsi.msitypes.msidattn.msiregs)
#define Elmsilcommon(p)    (&p->el_body.elmsi.msitypes.msilpkt.msilcommon)
#define Elmsicmdblk(p)	   ((struct msi_cmdblk *)p)
#define Elmsipacket(p)	   ((struct msi_packet *)p)


#ifndef ST_VAXSTAR
#define ST_VAXSTAR 4
#endif ST_VAXSTAR

/****************************  STORAGE AREAS **************************/

static  short  unk_rec;
static  short  ads_next;
static  short  ads_num;
static  long   *ads_item_ptr;
int scan_ads_num;
int spu_crd_cnt;

/*
*	.SBTTL	EI$BLD - Builds the standard record
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine sets up the buffers for building a standard event
*	record, gets the definitions for each record from the
*	corresponding tables, and passes each segment to the transformation
*	engine to build a standard EIMS record
*	
* CALLING SEQUENCE:		CALL EI$BLD (..See Below..)
*					Called from EI$GET with address
*					of the elrp event buffer and pointer
*					to a structure
*					containing segment ptrs
* FORMAL PARAMETERS:		
*
*	ERMS CTX		Address of context block for erms
*	buf_addr		Address of the elrp event buffer
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		NONE
*
* ROUTINE VALUE:		NONE
*
*
* SIDE EFFECTS:			
*
*--
*/

/*...	ROUTINE EI$BLD (eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr, elrp)	*/
int  ei$bld(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr, elrp)

EIS  *eis_ptr;
DIS  *dis_ptr;
CDS  *cds_ptr;
SDS  *sds_ptr;
ADS  *ads_ptr;
SIS  *sis_ptr;

struct el_rec *elrp;

{

u_char  *var_elrp;
long    temp_long;
long    temp_long1;
long    temp_long2;
long    tmp_long;
long    tmp1_long;
long    status;
long	sysktype;
short   ostype = es$ultrix32;

long    *make_addr_vector();

eis_ptr->type = ES$EIS;
eis_ptr->subtype = 1;	/* Always 1 because there is no subtype */
eis_ptr->version = 1;

dis_ptr->type = ES$DIS;
dis_ptr->subtype = 0;	/* 0 means this segment is not used */
dis_ptr->version = 1;

cds_ptr->type = ES$CDS;
cds_ptr->subtype = 0;	/* 0 means this segment is not used */
cds_ptr->version = 1;

sds_ptr->type = ES$SDS;
sds_ptr->subtype = 0;	/* 0 means this segment is not used */
sds_ptr->version = 1;

ads_ptr->type = ES$ADS;
ads_ptr->subtype = 0;	/* 0 means this segment is not used */
ads_ptr->version = 1;

sis_ptr->type = ES$SIS;
sis_ptr->subtype = 0;	/* 0 means this segment is not used */
sis_ptr->version = 1;

				/* Initialize segments - build    */
				/* item list & setup string area  */

status  = EI$SUCC;
unk_rec = FALSE;

if (ini_seg(eis_ptr) == EI$FAIL)
    status = EI$FAIL;





/******************* MOVE EIS  ****************************/

os_std(&elrp->elsubid.subid_class,
		OS$evclass,		DD$eventclass);
os_std(&elrp->elsubid.subid_class,
		OS$gen_short,		DD$eventtype);
os_std(&elrp->elrhdr.rhdr_seqnum,
		OS$gen_short,		DD$recordnumber);
os_std(&ostype,	OS$gen_short,		DD$ostype);
os_std(&elrp->elrhdr.rhdr_time,
		OS$gen_long,		DD$datetime);
os_std(elrp->elrhdr.rhdr_hname,
		OS$rhdr_hname,		DD$hostname);

if (elrp->elrhdr.rhdr_systype != 0)
  os_std(&elrp->elrhdr.rhdr_systype,
		OS$gen_long,		DD$systype);

switch (elrp->elrhdr.rhdr_sid >> 24)	/* only cpu ident */
  {
  case 0x01:
  case 0x02:
  case 0x03:
  case 0x06:
  case 0x07:
  case 0x11:
    os_std(&elrp->elrhdr.rhdr_sid, OS$gen_long,	DD$sysid);
    sysktype = (elrp->elrhdr.rhdr_sid & 0xff000000);
    os_std(&sysktype, OS$sysktyp, DD$sysktype);
  break;

  case 0x04:
    os_std(&elrp->elrhdr.rhdr_sid, OS$gen_long,	DD$sysid);
    sysktype = (elrp->elrhdr.rhdr_sid & 0xffc00000);
    os_std(&sysktype, OS$sysktyp, DD$sysktype);
  break;    

  case 0x05:
    os_std(&elrp->elrhdr.rhdr_sid, OS$gen_long,	DD$sysid);
    sysktype = (elrp->elrhdr.rhdr_sid & 0xff800000);
    os_std(&sysktype, OS$sysktyp, DD$sysktype);
  break;    

  case 0x12:
    os_std(&elrp->elrhdr.rhdr_sid, OS$gen_long,	DD$sysid);
    sysktype  = (elrp->elrhdr.rhdr_sid & 0xff000000);
    sysktype |= (elrp->elrhdr.rhdr_systype >> 8) & 0x00ff0000;
    os_std(&sysktype, OS$sysktyp, DD$sysktype);
  break;

  case 0x08:
    os_std(&elrp->elrhdr.rhdr_sid, OS$gen_long,	DD$sysid);
    sysktype  = (elrp->elrhdr.rhdr_sid & 0xff000000);
    sysktype |= (elrp->elrhdr.rhdr_systype >> 8) & 0x00ff0000;
    if ((elrp->elrhdr.rhdr_systype == 0)  &&	/* old entries */
        (elrp->elsubid.subid_ctldevtyp == ST_VAXSTAR))
            sysktype |= 0x00040000;
    os_std(&sysktype, OS$sysktyp, DD$sysktype);
  break;

  case 0x0A:
  case 0x0B:
    os_std(&elrp->elrhdr.rhdr_sid, OS$gen_long,	DD$sysid2);
    sysktype  = (elrp->elrhdr.rhdr_sid & 0xff000000);
    sysktype |= (elrp->elrhdr.rhdr_systype >> 8) & 0x00ff0000;
    if (((elrp->elrhdr.rhdr_systype >> 24) == 0x01) ||
        ((elrp->elrhdr.rhdr_systype >> 24) == 0x04))
            sysktype |= (elrp->elrhdr.rhdr_systype & 0x0000ff00);
    os_std(&sysktype, OS$sysktyp, DD$sysktype);
  break;

  case 0x0E:
    os_std(&elrp->elrhdr.rhdr_sid, OS$gen_long,	DD$sysid);
    sysktype  = (elrp->elrhdr.rhdr_sid & 0xff000000);
		/* SID<23:22>System Type Identifier is not yet defined */
    os_std(&sysktype, OS$sysktyp, DD$sysktype);
  break;

  case 0x82:
  case 0x83:
    os_std(&elrp->elrhdr.rhdr_sid, OS$gen_long, DD$pmax_prid);
    sysktype  = (elrp->elrhdr.rhdr_sid & 0xffff0000);
    os_std(&sysktype, OS$sysktyp, DD$sysktype);
  break;
  }

if (elrp->elrhdr.rhdr_mpnum > 1)
  {
  os_std(&elrp->elrhdr.rhdr_mpnum,
		OS$gen_long,		DD$cpucnt);
  os_std(&elrp->elrhdr.rhdr_mperr,
		OS$gen_long,		DD$cpulog);
  }

dis_ptr->subtype = 1;			/* give dis subtype	*/
if (ini_seg(dis_ptr) == EI$FAIL)	/* get dis items	*/
    status = EI$FAIL;

switch(elrp->elsubid.subid_class)
  {
/*************** 100 ELCT_MCK  (Machine Check Errors) ***********/

  case ELCT_MCK :
    os_std(&elrp->elsubid.subid_class,
		OS$cpuerr,		DD$devclass);
    os_std(&elrp->elsubid.subid_class,
		OS$mckstk,		DD$coarsesyndrome);
    switch(elrp->elsubid.subid_type)
      {

      case ELMCKT_780 :
	cds_ptr->subtype = DD$MC780CDS;
	ads_ptr->subtype = DD$MC78_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmck.elmck_frame.el780mcf.mc8_bcnt,
		OS$gen_long,		DD$mcbcnt);
        os_std(&elrp->el_body.elmck.elmck_frame.el780mcf.mc8_summary,
		OS$gen_long,		DD$mcksumm);
        os_std(&elrp->el_body.elmck.elmck_frame.el780mcf.mc8_cpues,
		OS$gen_long,		DD$mc8cpues);
        os_std(&elrp->el_body.elmck.elmck_frame.el780mcf.mc8_upc,
		OS$gen_long,		DD$mc8upc);
        os_std(&elrp->el_body.elmck.elmck_frame.el780mcf.mc8_vaviba,
		OS$gen_long,		DD$mc8vaviba);
        os_std(&elrp->el_body.elmck.elmck_frame.el780mcf.mc8_dreg,
		OS$gen_long,		DD$mc8dreg);
        os_std(&elrp->el_body.elmck.elmck_frame.el780mcf.mc8_tber0,
		OS$gen_long,		DD$mc8tb0);
        os_std(&elrp->el_body.elmck.elmck_frame.el780mcf.mc8_tber1,
		OS$gen_long,		DD$mc8tb1);
        os_std(&elrp->el_body.elmck.elmck_frame.el780mcf.mc8_timo,
		OS$gen_long,		DD$mc8timo);
        os_std(&elrp->el_body.elmck.elmck_frame.el780mcf.mc8_parity,
		OS$gen_long,		DD$mc8par);
        os_std(&elrp->el_body.elmck.elmck_frame.el780mcf.mc8_sbier,
		OS$gen_long,		DD$awer);
        os_std(&elrp->el_body.elmck.elmck_frame.el780mcf.mc8_pc,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.elmck.elmck_frame.el780mcf.mc8_psl,
		OS$gen_long,		DD$psl);
      break;
      case ELMCKT_750 :
	cds_ptr->subtype = DD$MC750_CDS;
	ads_ptr->subtype = DD$MC75_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmck.elmck_frame.el750mcf.mc5_bcnt,
		OS$gen_long,		DD$mcbcnt);
        os_std(&elrp->el_body.elmck.elmck_frame.el750mcf.mc5_summary,
		OS$gen_long,		DD$mcksumm);
        os_std(&elrp->el_body.elmck.elmck_frame.el750mcf.mc5_va,
		OS$gen_long,		DD$mc5va);
        os_std(&elrp->el_body.elmck.elmck_frame.el750mcf.mc5_errpc,
		OS$gen_long,		DD$mc5erpc);
        os_std(&elrp->el_body.elmck.elmck_frame.el750mcf.mc5_mdr,
		OS$gen_long,		DD$mc5mdr);
        os_std(&elrp->el_body.elmck.elmck_frame.el750mcf.mc5_svmode,
		OS$gen_long,		DD$mc5sav);
        os_std(&elrp->el_body.elmck.elmck_frame.el750mcf.mc5_rdtimo,
		OS$gen_long,		DD$mc5rdt);
        os_std(&elrp->el_body.elmck.elmck_frame.el750mcf.mc5_tbgpar,
		OS$gen_long,		DD$mc5tbg);
        os_std(&elrp->el_body.elmck.elmck_frame.el750mcf.mc5_cacherr,
		OS$gen_long,		DD$mc5cache);
        os_std(&elrp->el_body.elmck.elmck_frame.el750mcf.mc5_buserr,
		OS$gen_long,		DD$mc5bus);
        os_std(&elrp->el_body.elmck.elmck_frame.el750mcf.mc5_mcesr,
		OS$gen_long,		DD$mc5mce);
        os_std(&elrp->el_body.elmck.elmck_frame.el750mcf.mc5_pc,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.elmck.elmck_frame.el750mcf.mc5_psl,
		OS$gen_long,		DD$psl);
      break;
      case ELMCKT_730 :
	cds_ptr->subtype = DD$MC730_CDS;
	ads_ptr->subtype = DD$MCK_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmck.elmck_frame.el730mcf.mc3_bcnt,
		OS$gen_long,		DD$mcbcnt);
        os_std(&elrp->el_body.elmck.elmck_frame.el730mcf.mc3_summary,
		OS$gen_long,		DD$mc73summ);
        os_std(&elrp->el_body.elmck.elmck_frame.el730mcf.mc3_parm[0],
		OS$gen_long,		DD$mc1pm1);
        os_std(&elrp->el_body.elmck.elmck_frame.el730mcf.mc3_parm[1],
		OS$gen_long,		DD$mc1pm2);
        os_std(&elrp->el_body.elmck.elmck_frame.el730mcf.mc3_pc,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.elmck.elmck_frame.el730mcf.mc3_psl,
		OS$gen_long,		DD$psl);
      break;
      case ELMCKT_8600 :
	cds_ptr->subtype = DD$MC8600_CDS;
	ads_ptr->subtype = DD$MC86_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_bytcnt,
		OS$gen_long,		DD$mcbcnt);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_ehm_sts,
		OS$gen_long,		DD$m86ehm);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_evmqsav,
		OS$gen_long,		DD$m86evmqsav);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_ebcs,
		OS$gen_long,		DD$m86ebcs);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_edpsr,
		OS$gen_long,		DD$m86edpsr);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_cslint,
		OS$gen_long,		DD$m86cslint);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_ibesr,
		OS$gen_long,		DD$m86ibesr);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_ebxwd1,
		OS$gen_long,		DD$m86ew1);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_ebxwd2,
		OS$gen_long,		DD$m86ew2);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_ivasav,
		OS$gen_long,		DD$m86ivasav);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_vibasav,
		OS$gen_long,		DD$m86vibasav);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_esasav,
		OS$gen_long,		DD$m86esasav);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_isasav,
		OS$gen_long,		DD$m86isasav);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_cpc,
		OS$gen_long,		DD$m86cpc);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_mstat1,
		OS$gen_long,		DD$m86ms1);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_mstat2,
		OS$gen_long,		DD$m86ms2);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_mdecc,
		OS$gen_long,		DD$m86mdecc);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_merg,
		OS$gen_long,		DD$m86merg);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_cshctl,
		OS$gen_long,		DD$m86cshctl);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_mear,
		OS$gen_long,		DD$m86mear);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_medr,
		OS$gen_long,		DD$m86medr);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_accs,
		OS$gen_long,		DD$m86fber);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_cses,
		OS$gen_long,		DD$m86cses);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_pc,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.elmck.elmck_frame.el8600mcf.mc8600_psl,
		OS$gen_long,		DD$psl);
      break;
      case ELMCKT_8200 :
	cds_ptr->subtype = DD$MC8200_CDS;
	ads_ptr->subtype = DD$MC82_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmck.elmck_frame.el8200mcf.mc8200_bcnt,
		OS$gen_long,		DD$mcbcnt);
        os_std(&elrp->el_body.elmck.elmck_frame.el8200mcf.mc8200_summary,
		OS$gen_long,		DD$m82summ);
        os_std(&elrp->el_body.elmck.elmck_frame.el8200mcf.mc8200_parm1,
		OS$gen_long,		DD$m82pm1);
        os_std(&elrp->el_body.elmck.elmck_frame.el8200mcf.mc8200_va,
		OS$gen_long,		DD$m82va);
        os_std(&elrp->el_body.elmck.elmck_frame.el8200mcf.mc8200_vap,
		OS$gen_long,		DD$m82vap);
        os_std(&elrp->el_body.elmck.elmck_frame.el8200mcf.mc8200_mar,
		OS$gen_long,		DD$m82mar);
        os_std(&elrp->el_body.elmck.elmck_frame.el8200mcf.mc8200_stat,
		OS$gen_long,		DD$m82stat);
        os_std(&elrp->el_body.elmck.elmck_frame.el8200mcf.mc8200_pcfail,
		OS$gen_long,		DD$m82pcf);
        os_std(&elrp->el_body.elmck.elmck_frame.el8200mcf.mc8200_upcfail,
		OS$gen_long,		DD$m82upc);
        os_std(&elrp->el_body.elmck.elmck_frame.el8200mcf.mc8200_pc,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.elmck.elmck_frame.el8200mcf.mc8200_psl,
		OS$gen_long,		DD$psl);
      break;
      case ELMCKT_8800 :
	cds_ptr->subtype = DD$MC8800_CDS;
	ads_ptr->subtype = DD$MC88_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmck.elmck_frame.el8800mcf.mc8800_bcnt,
		OS$gen_long,		DD$mcbcnt);
        os_std(&elrp->el_body.elmck.elmck_frame.el8800mcf.mc8800_mcsts,
		OS$gen_long,		DD$m88mcs);
        os_std(&elrp->el_body.elmck.elmck_frame.el8800mcf.mc8800_ipc,
		OS$gen_long,		DD$m88ipc);
        os_std(&elrp->el_body.elmck.elmck_frame.el8800mcf.mc8800_vaviba,
		OS$gen_long,		DD$m88vaviba);
        os_std(&elrp->el_body.elmck.elmck_frame.el8800mcf.mc8800_iber,
		OS$gen_long,		DD$m88iber);
        os_std(&elrp->el_body.elmck.elmck_frame.el8800mcf.mc8800_cber,
		OS$gen_long,		DD$m88cber);
        os_std(&elrp->el_body.elmck.elmck_frame.el8800mcf.mc8800_eber,
		OS$gen_long,		DD$m88eber);
        os_std(&elrp->el_body.elmck.elmck_frame.el8800mcf.mc8800_nmifsr,
		OS$gen_long,		DD$m88nmf);
        os_std(&elrp->el_body.elmck.elmck_frame.el8800mcf.mc8800_nmiear,
		OS$gen_long,		DD$m88nme);
        os_std(&elrp->el_body.elmck.elmck_frame.el8800mcf.mc8800_pc,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.elmck.elmck_frame.el8800mcf.mc8800_psl,
		OS$gen_long,		DD$psl);
      break;
      case ELMCKT_UVI :
	cds_ptr->subtype = DD$MCUVAX1_CDS;
	ads_ptr->subtype = DD$MCK_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmck.elmck_frame.elUVImcf.mc1_bcnt,
		OS$gen_long,		DD$mcbcnt);
        os_std(&elrp->el_body.elmck.elmck_frame.elUVImcf.mc1_summary,
		OS$gen_long,		DD$mcuv1summ);
        os_std(&elrp->el_body.elmck.elmck_frame.elUVImcf.mc1_parm[0],
		OS$gen_long,		DD$mc1pm1);
        os_std(&elrp->el_body.elmck.elmck_frame.elUVImcf.mc1_parm[1],
		OS$gen_long,		DD$mc1pm2);
        os_std(&elrp->el_body.elmck.elmck_frame.elUVImcf.mc1_pc,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.elmck.elmck_frame.elUVImcf.mc1_psl,
		OS$gen_long,		DD$psl);
      break;
      case ELMCKT_UVII :
	cds_ptr->subtype = DD$MCUVAX2_CDS;
	ads_ptr->subtype = DD$MCUVII_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmck.elmck_frame.elUVIImcf.mc1_bcnt,
		OS$gen_long,		DD$mcbcnt);
        os_std(&elrp->el_body.elmck.elmck_frame.elUVIImcf.mc1_summary,
		OS$gen_long,		DD$mcuv2summ);
        if (((elrp->el_body.elmck.elmck_frame.elUVIImcf.mc1_summary & 
		0x000000ff) == 0x81) ||
            ((elrp->el_body.elmck.elmck_frame.elUVIImcf.mc1_summary & 
		0x000000ff) == 0x83))
            os_std(&elrp->el_body.elmck.elmck_frame.elUVIImcf.mc1_vap,
		OS$gen_long,		DD$mc1pap);
	else
            os_std(&elrp->el_body.elmck.elmck_frame.elUVIImcf.mc1_vap,
		OS$gen_long,		DD$mc1vap);
        os_std(
	  &elrp->el_body.elmck.elmck_frame.elUVIImcf.mc1_internal_state,
		OS$gen_long,		DD$mc1int);
        os_std(&elrp->el_body.elmck.elmck_frame.elUVIImcf.mc1_pc,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.elmck.elmck_frame.elUVIImcf.mc1_psl,
		OS$gen_long,		DD$psl);
      break;
      case ELMCKT_CVAX :
      case ELMCKT_PVAX :
      case ELMCKT_6200 :
	cds_ptr->subtype = DD$MCCVAX_CDS;
	ads_ptr->subtype = DD$MCCVAX_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmck.elmck_frame.elCVAXmcf.mc1_bcnt,
		OS$gen_long,		DD$mcbcnt);
        os_std(&elrp->el_body.elmck.elmck_frame.elCVAXmcf.mc1_summary,
		OS$gen_long,		DD$mccvaxsum);
        if (((elrp->el_body.elmck.elmck_frame.elCVAXmcf.mc1_summary & 
		0x000000ff) == 0x81) ||
            ((elrp->el_body.elmck.elmck_frame.elCVAXmcf.mc1_summary & 
		0x000000ff) == 0x83))
            os_std(&elrp->el_body.elmck.elmck_frame.elCVAXmcf.mc1_vap,
		OS$gen_long,		DD$mc1pap);
	else
            os_std(&elrp->el_body.elmck.elmck_frame.elCVAXmcf.mc1_vap,
		OS$gen_long,		DD$mc1vap);
        os_std(
	  &elrp->el_body.elmck.elmck_frame.elCVAXmcf.mc1_internal_state1,
		OS$gen_long,		DD$mccvaxint1);
        os_std(
	  &elrp->el_body.elmck.elmck_frame.elCVAXmcf.mc1_internal_state2,
		OS$gen_long,		DD$mccvaxint2);
        os_std(&elrp->el_body.elmck.elmck_frame.elCVAXmcf.mc1_pc,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.elmck.elmck_frame.elCVAXmcf.mc1_psl,
		OS$gen_long,		DD$psl);
	if(elrp->elsubid.subid_type == ELMCKT_6200)
	  {
	  sds_ptr->subtype = DD$XCP_6200_SDS;
	  if (ini_seg(sds_ptr) == EI$FAIL)
	      status = EI$FAIL;
          os_std(&elrp->el_body.elmck.elmck_frame.el6200mcf.xcp_dtype,
		OS$gen_long,		DD$xdev);
          os_std(&elrp->el_body.elmck.elmck_frame.el6200mcf.xcp_xbe,
		OS$gen_long,		DD$xbe);
          os_std(&elrp->el_body.elmck.elmck_frame.el6200mcf.xcp_csr1,
		OS$gen_long,		DD$csr1);
          os_std(&elrp->el_body.elmck.elmck_frame.el6200mcf.xcp_csr2,
		OS$gen_long,		DD$csr2);
          os_std(&elrp->el_body.elmck.elmck_frame.el6200mcf.xcp_mser,
		OS$gen_long,		DD$esr650_mser);
	  }
      break;
      case ELMCKT_6400: 
	cds_ptr->subtype = DD$XRP_CDS;
	sds_ptr->subtype = DD$XRP_SDS;
	ads_ptr->subtype = DD$XRP_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(sds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.mcode,
		OS$gen_long,		DD$mcode);
        os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.vaddr,
		OS$gen_long,		DD$vaddr);
        os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.viba,
		OS$gen_long,		DD$viba);
        os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.s_rcsr,
		OS$gen_long,		DD$rcsr);
        if (((elrp->el_body.elmck.elmck_frame.el6400mcf.s_xber & 
	  	    0x00100000) == 0x01) ||
            ((elrp->el_body.elmck.elmck_frame.el6400mcf.s_xber & 
	  	    0x00040000) == 0x01) ||
            ((elrp->el_body.elmck.elmck_frame.el6400mcf.s_xber & 
	  	    0x00020000) == 0x01) ||
            ((elrp->el_body.elmck.elmck_frame.el6400mcf.s_xber & 
	  	    0x00010000) == 0x01) ||
            ((elrp->el_body.elmck.elmck_frame.el6400mcf.s_xber & 
	  	    0x00008000) == 0x01) ||
            ((elrp->el_body.elmck.elmck_frame.el6400mcf.s_xber & 
	  	    0x00004000) == 0x01) ||
            ((elrp->el_body.elmck.elmck_frame.el6400mcf.s_xber & 
	  	    0x00002000) == 0x01))
            os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.s_xber,
		OS$gen_long,		DD$xber);
        if ((elrp->el_body.elmck.elmck_frame.el6400mcf.s_xber & 
	  	    0x0000000f) == 0x09)
            os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.s_xfadr,
		OS$gen_long,		DD$xfadrid);
        else if ((elrp->el_body.elmck.elmck_frame.el6400mcf.s_xber & 
	  	    0x0000000f) == 0x0f)
            os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.s_xfadr,
		OS$gen_long,		DD$xfadriv);
	else
            os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.s_xfadr,
		OS$gen_long,		DD$xfadr);
        os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.s_sscbtr,
		OS$gen_long,		DD$sscbtr);
        os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.iccs_sisr,
		OS$gen_long,		DD$iccs_sisr);
        os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.istate,
		OS$gen_long,		DD$istate);
        os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.sc,
		OS$gen_long,		DD$sc);
        os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.pc,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.psl,
		OS$gen_long,		DD$psl);
        os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.s_bcctl,
		OS$gen_long,		DD$bcctl);
        if ((elrp->el_body.elmck.elmck_frame.el6400mcf.s_bcsts & 
		        0x02000000) == 0x01)
            os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.s_bcsts,
		OS$gen_long,		DD$bcsts1);
        else
	    {
            if (((elrp->el_body.elmck.elmck_frame.el6400mcf.s_bcsts & 
	  	        0x01E00000) == 0x02) ||
                ((elrp->el_body.elmck.elmck_frame.el6400mcf.s_bcsts & 
		        0x01E00000) == 0x0A))
              os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.s_bcsts,
		OS$gen_long,		DD$bcsts0a);
	    else
              os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.s_bcsts,
		OS$gen_long,		DD$bcsts0b);
	    }
        if ((elrp->el_body.elmck.elmck_frame.el6400mcf.s_bcsts & 
		        0x00000001) == 0x01)
            os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.s_bcerr,
		OS$gen_long,		DD$bcerr);
        os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.s_pcsts,
		OS$gen_long,		DD$pcsts);
        if ((elrp->el_body.elmck.elmck_frame.el6400mcf.s_pcsts & 
		        0x00000080) == 0x01)
            os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.s_pcerr,
		OS$gen_long,		DD$pcerr1);
        else
            os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.s_pcerr,
		OS$gen_long,		DD$pcerr0);
        os_std(&elrp->el_body.elmck.elmck_frame.el6400mcf.s_vintsr,
		OS$gen_long,		DD$vintsr);
      break;

      case ELMCKT_6500: 
	cds_ptr->subtype = DD$6500_MCHECK_CDS;
	ads_ptr->subtype = DD$6500_INT_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
	
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.bcnt,
	       OS$gen_long,		DD$mcbcnt);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.mcode,
	       OS$gen_long,		DD$mc_code);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.vaddr,
	       OS$gen_long,		DD$vaddr);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.viba,
	       OS$gen_long,		DD$viba);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.iccs_sisr,
	       OS$gen_long,		DD$6500_iccs_sisr);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.istate,
	       OS$gen_long,		DD$6500_istate);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.sc,
	       OS$gen_long,		DD$sc);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.pc,
	       OS$gen_long,		DD$pc);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.psl,
	       OS$gen_long,		DD$6500_psl);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_xbe0,
	       OS$gen_long,		DD$xbe0);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_xfadr0,
	       OS$gen_long,		DD$xfadr0);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_xfaer0,
	       OS$gen_long,		DD$xfaer);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_xbeer0,
	       OS$gen_long,		DD$xbeer);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_wfadr0,
	       OS$gen_long,		DD$wfadr0);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_wfadr1,
	       OS$gen_long,		DD$wfadr1);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_fdal0,
	       OS$gen_long,		DD$fdal0);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_fdal1,
	       OS$gen_long,		DD$fdal1);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_fdal2,
	       OS$gen_long,		DD$fdal2);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_fdal3,
	       OS$gen_long,		DD$fdal3);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_sscbtr,
	       OS$gen_long,		DD$6500_sscbtr);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_bcsts,
	       OS$gen_long,		DD$bcsts);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_bcera,
	       OS$gen_long,		DD$bcera);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_bcert,
	       OS$gen_long,		DD$bcert);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_pcsts,
	       OS$gen_long,		DD$6500_pcsts);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_pcerr,
	       OS$gen_long,		DD$pcerr);
	os_std(&elrp->el_body.elmck.elmck_frame.el6500mcf.s_vintsr,
	       OS$gen_long,		DD$6500_vintsr);
      break;

      case ELMCKT_9000:
	cds_ptr->subtype = DD$MCK9000_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
#define AQMCK &elrp->el_body.elmck.elmck_frame.el9000mcf
#define AQEBOXMCK &elrp->el_body.elmck.elmck_frame.el9000eboxmcf
#define MCK_EBOX 0
	if(elrp->el_body.elmck.elmck_frame.el9000mcf.mc_type == MCK_EBOX) {
		/* MCK from EBOX */
		os_std(AQEBOXMCK.mc_length, OS$gen_long, DD$mck9000_length);
		os_std(AQEBOXMCK.mc_type, OS$gen_long, DD$mck9000_type);
		os_std(AQEBOXMCK.mc_pc, OS$gen_long, DD$mck9000_pc);
		os_std(AQEBOXMCK.mc_psl, OS$gen_long, DD$psl);
	} else {
		/* MCK from SPU */
		os_std(AQMCK.mc_length, OS$gen_long, DD$mck9000_length);
		os_std(AQMCK.mc_type, OS$gen_long, DD$mck9000_type);
		os_std(AQMCK.mc_pc, OS$gen_long, DD$mck9000_pc);
		os_std(AQMCK.mc_psl, OS$gen_long, DD$psl);
		os_std(AQMCK.mc_id, OS$gen_long, DD$mck9000_id);
		os_std(AQMCK.mc_err_summ, OS$gen_long, DD$mck9000_err_summ);
		os_std(AQMCK.mc_sys_summ, OS$gen_long, DD$mck9000_sys_summ);
		os_std(AQMCK.mc_vaddr, OS$gen_long, DD$mck9000_vaddr);
		os_std(AQMCK.mc_paddr, OS$gen_long, DD$mck9000_paddr);
		os_std(AQMCK.mc_misc_info, OS$gen_long, DD$mck9000_misc_info);
		os_std(AQMCK.mc_vpsr, OS$gen_long, DD$vec_vpsr);
	}
	break;

      case ELMCKT_9000SPU:
	status = v9000_bld_spumck(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp);
	break;

      default:
        unk_rec = TRUE;
      break;
      }
  break;

/*************** 101 ELCT_MEM  (Memory Errors) ******************/

  case ELCT_MEM :
    ads_ptr->subtype = DD$MEM_ADS;
    if (ini_seg(ads_ptr) == EI$FAIL)
	status = EI$FAIL;
    os_std(&elrp->elsubid.subid_class,
		OS$memerr,		DD$devclass);
    os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$memory_type,		DD$devtype);
    if( elrp->elsubid.subid_ctldevtyp != ELMCNTR_9000_SE &&
	elrp->elsubid.subid_ctldevtyp != ELMCNTR_9000_HE &&
        elrp->elsubid.subid_ctldevtyp != ELMCNTR_XMA2 &&
	elrp->elsubid.subid_ctldevtyp != ELMCNTR_6200
      )
    {
        os_std(&elrp->el_body.elmem.elmemerr.type,
		OS$memsyn,		DD$coarsesyndrome);
        os_std(&elrp->el_body.elmem.elmemerr.cntl,
		OS$gen_short,		DD$ctrlr);
        os_std(&elrp->el_body.elmem.elmemerr.numerr,
		OS$gen_tiny,		DD$errcnt);
    }

    switch(elrp->elsubid.subid_ctldevtyp)
      {

      case ELMCNTR_780C:
	cds_ptr->subtype = DD$MEMC78_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmem.elmemerr.regs[0],
		OS$gen_long,		DD$m8cmera);
        os_std(&elrp->el_body.elmem.elmemerr.regs[1],
		OS$gen_long,		DD$m8cmerb);
        os_std(&elrp->el_body.elmem.elmemerr.regs[2],
		OS$gen_long,		DD$m8cmerc);
      break;
      case ELMCNTR_780E:
	cds_ptr->subtype = DD$MEME78_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmem.elmemerr.regs[0],
		OS$gen_long,		DD$memer0);
        os_std(&elrp->el_body.elmem.elmemerr.regs[1],
		OS$gen_long,		DD$memer1);
        os_std(&elrp->el_body.elmem.elmemerr.regs[2],
		OS$gen_long,		DD$memer2);
        os_std(&elrp->el_body.elmem.elmemerr.regs[3],
		OS$gen_long,		DD$memer3);
      break;
      case ELMCNTR_750:
      case ELMCNTR_730:
	cds_ptr->subtype = DD$MEM750_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmem.elmemerr.regs[0],
		OS$gen_long,		DD$m5mer0);
        os_std(&elrp->el_body.elmem.elmemerr.regs[1],
		OS$gen_long,		DD$m5mer1);
        os_std(&elrp->el_body.elmem.elmemerr.regs[2],
		OS$gen_long,		DD$m5mer2);
      break;
      case ELMCNTR_8600:
	cds_ptr->subtype = DD$MEM860_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmem.elmemerr.regs[0],
		OS$gen_long,		DD$m86mdecc);
        os_std(&elrp->el_body.elmem.elmemerr.regs[1],
		OS$gen_long,		DD$m86mear);
        os_std(&elrp->el_body.elmem.elmemerr.regs[2],
		OS$gen_long,		DD$m86ms1);
        os_std(&elrp->el_body.elmem.elmemerr.regs[3],
		OS$gen_long,		DD$m86ms2);
      break;
      case ELMCNTR_BI:		/* 8200 memory controller */
	cds_ptr->subtype = DD$MEM820_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmem.elmemerr.regs[0],
		OS$gen_long,		DD$m82csr0);
        os_std(&elrp->el_body.elmem.elmemerr.regs[1],
		OS$gen_long,		DD$m82csr1);
      break;
      case ELMCNTR_NMI:		/* 8800 memory controller */
	cds_ptr->subtype = DD$MEM880_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmem.elmemerr.regs[0],
		OS$gen_long,		DD$m88csr0);
        os_std(&elrp->el_body.elmem.elmemerr.regs[1],
		OS$gen_long,		DD$m88csr1);
        os_std(&elrp->el_body.elmem.elmemerr.regs[2],
		OS$gen_long,		DD$m88csr2);
        os_std(&elrp->el_body.elmem.elmemerr.regs[3],
		OS$gen_long,		DD$m88csr3);
      break;
      case ELMCNTR_630:
      case ELMCNTR_VAXSTAR:
	cds_ptr->subtype = DD$MEMUVAX2_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmem.elmemerr.regs[0],
		OS$gen_long,		DD$mvaxmser);
        os_std(&elrp->el_body.elmem.elmemerr.regs[1],
		OS$gen_long,		DD$mvaxcaer);
        os_std(&elrp->el_body.elmem.elmemerr.regs[2],
		OS$gen_long,		DD$mvaxdaer);
      break;
      case ELMCNTR_420 : 
	cds_ptr->subtype = DD$650ESR_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmem.elmemerr.regs[0],
		OS$gen_long,		DD$esr650_cacr);
        os_std(&elrp->el_body.elmem.elmemerr.regs[1],
		OS$gen_long,		DD$esr650_cadr);
        os_std(&elrp->el_body.elmem.elmemerr.regs[2],
		OS$gen_long,		DD$esr650_mser);
        os_std(&elrp->el_body.elmem.elmemerr.regs[3],
		OS$gen_long,		DD$mc1pap);
      break;
      case ELMCNTR_650 :
      case ELMCNTR_5400:
	cds_ptr->subtype = DD$MEMCVAX_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        if ((elrp->el_body.elmem.elmemerr.regs[0] & 0x20000000) != 0)
            os_std(&elrp->el_body.elmem.elmemerr.regs[0], /* bit 29 */
		OS$gen_long,		DD$memcsr16a);
        else if ((elrp->el_body.elmem.elmemerr.regs[0] & 0xC0000000) != 0)
            os_std(&elrp->el_body.elmem.elmemerr.regs[0], /* bit 31:30 */
		OS$gen_long,		DD$memcsr16b);
        else if ((elrp->el_body.elmem.elmemerr.regs[0] & 0x00000080) != 0)
            os_std(&elrp->el_body.elmem.elmemerr.regs[0], /* bit 7 */
		OS$gen_long,		DD$memcsr16c);
        else if ((elrp->el_body.elmem.elmemerr.regs[0] & 0x20000000) != 0)
            os_std(&elrp->el_body.elmem.elmemerr.regs[0], /* others */
		OS$gen_long,		DD$memcsr16d);
        os_std(&elrp->el_body.elmem.elmemerr.regs[1],
		OS$gen_long,		DD$memcsr17);
        if ((elrp->el_body.elmem.elmemerr.regs[0] & 0xE0000080) != 0)
 	    {				/* bits 31:29, 7 */
            os_std(&elrp->el_body.elmem.elmemerr.regs[2],
		OS$gen_long,		DD$memcsrn);
            os_std(&elrp->el_body.elmem.elmemerr.regs[3],
		OS$gen_long,		DD$memcon);
 	    }
      break;
      case ELMCNTR_60 :
	cds_ptr->subtype = DD$60_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elmem.elmemerr.regs[0],
		OS$gen_long,		DD$ff_eccaddr0);
        os_std(&elrp->el_body.elmem.elmemerr.regs[1],
		OS$gen_long,		DD$ff_eccaddr1);
        os_std(&elrp->el_body.elmem.elmemerr.regs[2],
		OS$gen_long,		DD$ff_eccsynd0);
        os_std(&elrp->el_body.elmem.elmemerr.regs[3],
		OS$gen_long,		DD$ff_eccsynd1);
      break;
      case ELMCNTR_6200:
      case ELMCNTR_5800:
	cds_ptr->subtype = DD$XBI_XMA_CDS;
	sds_ptr->subtype = DD$XMA_SDS;
	ads_ptr->subtype = DD$XMA_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(sds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.el_xma.xma_node,
		OS$gen_long,		DD$xnode);
        os_std(&elrp->el_body.el_xma.xma_dtype,
		OS$gen_long,		DD$xdev);
        os_std(&elrp->el_body.el_xma.xma_xbe,
		OS$gen_long,		DD$xbe);
        os_std(&elrp->el_body.el_xma.xma_seadr,
		OS$gen_long,		DD$seadr);
        os_std(&elrp->el_body.el_xma.xma_mctl1,
		OS$gen_long,		DD$mctl1);
        os_std(&elrp->el_body.el_xma.xma_mecer,
		OS$gen_long,		DD$mecer);
        os_std(&elrp->el_body.el_xma.xma_mecea,
		OS$gen_long,		DD$mecea);
        os_std(&elrp->el_body.el_xma.xma_mctl2,
		OS$gen_long,		DD$mctl2);
      break;

      case ELMCNTR_KN02BA:
        cds_ptr->subtype = DD$MEM_KN02BA_CDS;
        if (ini_seg(cds_ptr) == EI$FAIL)
            status = EI$FAIL;
	if (elrp->el_body.elmem.elmemerr.regs[0]>>28 & 0x1)
           os_std(&elrp->el_body.elmem.elmemerr.regs[0],
                OS$gen_long,            DD$kn02ba_mreg2);
        else
	   os_std(&elrp->el_body.elmem.elmemerr.regs[0],
                OS$gen_long,            DD$kn02ba_mreg1);

        os_std(&elrp->el_body.elmem.elmemerr.regs[1],
                OS$gen_long,            DD$pmax_paddr);
        os_std(&elrp->el_body.elmem.elmemerr.regs[2],
                OS$gen_long,            DD$pmax_epc);
        os_std(&elrp->el_body.elmem.elmemerr.regs[3],
                OS$gen_long,            DD$badva);
      break;
	
      case ELMCNTR_5100:
      case ELMCNTR_PMAX:
        cds_ptr->subtype = DD$PMAX_MEM_CDS;
        if (ini_seg(cds_ptr) == EI$FAIL)
            status = EI$FAIL;
        os_std(&elrp->el_body.elmem.elmemerr.regs[0],
                OS$gen_long,            DD$pmax_mreg);
        os_std(&elrp->el_body.elmem.elmemerr.regs[1],
                OS$gen_long,            DD$pmax_paddr);
        os_std(&elrp->el_body.elmem.elmemerr.regs[2],
                OS$gen_long,            DD$pmax_epc);
        os_std(&elrp->el_body.elmem.elmemerr.regs[3],
                OS$gen_long,            DD$badva);
      break;

      case ELMCNTR_kn02:
        cds_ptr->subtype = DD$MAX_ESR_CDS;
        if (ini_seg(cds_ptr) == EI$FAIL)
            status = EI$FAIL;
        os_std(&elrp->el_body.elmem.elmemerr.regs[1],
                OS$gen_long,            DD$kn02csr);
	tmp1_long = elrp->el_body.elmem.elmemerr.regs[2];
	tmp1_long &= 0x07ffffff;
	tmp1_long = (tmp1_long << 2);
	temp_long = elrp->el_body.elmem.elmemerr.regs[2];
        temp_long &= 0x80000000;
	if (temp_long == 0x80000000)
	    {
            os_std(&tmp1_long,
                    OS$gen_long,            DD$erradr);
            os_std(&elrp->el_body.elmem.elmemerr.regs[2],
                    OS$gen_long,            DD$error_adr);
	    }
        os_std(&elrp->el_body.elmem.elmemerr.regs[3],
                OS$gen_long,            DD$chksyn);
/*	temp_long = elrp->el_body.elesr.elesr.el_esrkn02.esr_epc;*/
	temp_long = elrp->el_body.elmem.elmemerr.regs[0];
	temp_long &= 0x80000000;
	if (temp_long == 0x80000000)
            os_std(&elrp->el_body.elmem.elmemerr.regs[0],
                OS$gen_long,            DD$pmax_epc);
      break;

      case ELMCNTR_5500:
        cds_ptr->subtype = DD$MIPSFAIR2_MEM_CDS;
        if (ini_seg(cds_ptr) == EI$FAIL)
            status = EI$FAIL;
        os_std(&elrp->el_body.elmem.elmemerr.regs[0],
                OS$gen_long,            DD$mf2_mesr);
        os_std(&elrp->el_body.elmem.elmemerr.regs[1],
                OS$gen_long,            DD$pmax_paddr);
        os_std(&elrp->el_body.elmem.elmemerr.regs[2],
                OS$gen_long,            DD$pmax_epc);
        os_std(&elrp->el_body.elmem.elmemerr.regs[3],
                OS$gen_long,            DD$badva);
	if (elrp->el_body.elmem.elmemerr.regs[4] > 0 &
	    elrp->el_body.elmem.elmemerr.regs[4] < 5)
            os_std(&elrp->el_body.elmem.elmemerr.regs[4],
                OS$gen_long,            DD$board_num);
      break;

      case ELMCNTR_XMA2:
        cds_ptr->subtype = DD$XMA2_CDS;
        if (ini_seg(cds_ptr) == EI$FAIL)
            status = EI$FAIL;
        os_std(&elrp->el_body.el_xma2.xma_node,
		OS$gen_long,		DD$xnode);
        os_std(&elrp->el_body.el_xma2.xma_dtype,
		OS$gen_long,		DD$dtype);
        os_std(&elrp->el_body.el_xma2.xma_xbe,
		OS$gen_long,		DD$xbe0);
        os_std(&elrp->el_body.el_xma2.xma_seadr,
		OS$gen_long,		DD$seadr);
        os_std(&elrp->el_body.el_xma2.xma_mctl1,
		OS$gen_long,		DD$6500_mctl1);
        os_std(&elrp->el_body.el_xma2.xma_mecer,
		OS$gen_long,		DD$mecer);
        os_std(&elrp->el_body.el_xma2.xma_mecea,
		OS$gen_long,		DD$mecea);
        os_std(&elrp->el_body.el_xma2.xma_mctl2,
		OS$gen_long,		DD$mctl2);
	os_std(&elrp->el_body.el_xma2.xma_becer,
		OS$gen_long,		DD$becer);
	os_std(&elrp->el_body.el_xma2.xma_becea,
		OS$gen_long,		DD$becea);
	os_std(&elrp->el_body.el_xma2.xma_stadr,
		OS$gen_long,		DD$stadr);
	os_std(&elrp->el_body.el_xma2.xma_enadr,
		OS$gen_long,		DD$enadr);
	os_std(&elrp->el_body.el_xma2.xma_intlv,
		OS$gen_long,		DD$intlv);
	os_std(&elrp->el_body.el_xma2.xma_mctl3,
		OS$gen_long,		DD$mctl3);
	os_std(&elrp->el_body.el_xma2.xma_mctl4,
		OS$gen_long,		DD$6500_mctl4);
	os_std(&elrp->el_body.el_xma2.xma_bsctl,
		OS$gen_long,		DD$bsctl);
	os_std(&elrp->el_body.el_xma2.xma_bsadr,
		OS$gen_long,		DD$bsadr);
	os_std(&elrp->el_body.el_xma2.xma_eectl,
		OS$gen_long,		DD$eectl);
	os_std(&elrp->el_body.el_xma2.xma_tmoer,
		OS$gen_long,		DD$tmoer);
      break;

      case ELMCNTR_9000_SE:
      case ELMCNTR_9000_HE:
		status = v9000_bld_mem(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp);
      break;

      default:
        unk_rec = TRUE;
      break;
      }
  break; 

/*************** 102 ELCT_DISK  (Disk Errors) *******************/

  case ELCT_DISK:
    sis_ptr->subtype = DD$SUM_DISK_SIS;
    if (ini_seg(sis_ptr) == EI$FAIL)
        status = EI$FAIL;
    os_std(&elrp->elsubid.subid_num,
		OS$gen_short,		DD$controller);
    os_std(&elrp->elsubid.subid_unitnum,
		OS$gen_short,		DD$unitnumber);
    os_std(&elrp->elsubid.subid_unitnum,
		OS$gen_short,		DD$sum_unum);

    switch(elrp->elsubid.subid_type)
      {
      case ELDEV_MSCP:
        sds_ptr->subtype = DD$DSK_COM_SDS;
        if (ini_seg(sds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$dsdisk,		DD$devclass);
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$dsdtyp,		DD$devtype);
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$sum_dev_d,		DD$sum_dev);
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$sum_dev_ddpre,	DD$sum_dev_pre);
        if (mscp_bld(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr, elrp) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->U_MSCP.mslg_unit,
		OS$gen_short,		DD$dsaunit);
        os_std(&elrp->U_MSCP.mslg_event,
		OS$gen_short,		DD$dsastat);
        os_std(&elrp->U_MSCP.mslg_event,
		OS$gen_short,		DD$sum_event);
        os_std(((DD$BYTE *) &elrp->U_MSCP.mslg_unit_id[1])+2,
		OS$gen_short,		DD$unitid2);
        break;

      case ELDEV_SCSI:
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$scdtyp,		DD$devclass);
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$scdisk,		DD$devtype);
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$sum_dev_dz,		DD$sum_dev);
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$sum_dev_dzpre,	DD$sum_dev_pre);

	if (scsi_bld(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,elrp) ==
		EI$FAIL)
	status = EI$FAIL;
	break;

      default:
        unk_rec = TRUE;
	break;
      }
  break; 


/*************** 103 ELCT_TAPE  (Tape Errors) *******************/

  case ELCT_TAPE:
    sis_ptr->subtype = DD$SUM_DISK_SIS;
    if (ini_seg(sis_ptr) == EI$FAIL)
        status = EI$FAIL;
    os_std(&elrp->elsubid.subid_num,
		OS$gen_short,		DD$controller);
    os_std(&elrp->elsubid.subid_unitnum,
		OS$gen_short,		DD$unitnumber);
    os_std(&elrp->elsubid.subid_unitnum,
		OS$gen_short,		DD$sum_unum);
    switch(elrp->elsubid.subid_type)
      {
      case ELDEV_MSCP: 
	sds_ptr->subtype = DD$STI_COM_SDS;
        if (ini_seg(sds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$dstape,		DD$devclass);
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$dsttyp,		DD$devtype);
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$sum_dev_t,		DD$sum_dev);
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$sum_dev_tpre,	DD$sum_dev_pre);
        if (mscp_bld(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr, elrp) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->U_MSCP.mslg_event,
		OS$gen_short,		DD$sum_event);
        switch (elrp->elsubid.subid_ctldevtyp)
          {
          case ELTT_TA78:
          case ELTT_TA81:
          case ELTT_TA90:
          case ELTT_TA91:
	    if (elrp->U_MSCP.mslg_format == 0x0b)
              os_std(&elrp->U_MSCP.mslg_event,
		OS$gen_short,		DD$fm_b_stat);
	    else
              os_std(&elrp->U_MSCP.mslg_event,
		OS$gen_short,		DD$ta_dsastat);
          break;
          case ELTT_TK50:
          case ELTT_TK70:
            os_std(&elrp->U_MSCP.mslg_event,
		OS$gen_short,		DD$tk_dsastat);
          break;
          case ELTT_TU81:
            os_std(&elrp->U_MSCP.mslg_event,
		OS$gen_short,		DD$tu_dsastat);
          break;
          case ELTT_TRV20:
            os_std(&elrp->U_MSCP.mslg_event,
		OS$gen_short,		DD$rv_dsastat);
          break;
          }
        os_std(&elrp->U_MSCP.mslg_unit,
		OS$gen_short,		DD$dsaunit);
        os_std(((DD$BYTE *) &elrp->U_MSCP.mslg_unit_id[1])+2,
		OS$gen_short,		DD$tp_unitid2);
        break;

     case ELDEV_SCSI:
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$scttyp,		DD$devclass);
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$sctape,		DD$devtype);
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$sum_dev_tz,		DD$sum_dev);
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$sum_dev_tpre,	DD$sum_dev_pre);

        if (scsi_bld(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,elrp) ==
                EI$FAIL)
        status = EI$FAIL;
        break;

      default:
        unk_rec = TRUE;
	break;
    }
  break; 



/*************** 104 ELCT_DCNTL  (Controller Errors) ************/

  case ELCT_DCNTL:
    os_std(&elrp->elsubid.subid_class,
		OS$adaperr,		DD$devclass);
    os_std(&elrp->elsubid.subid_type,
		OS$ctlsyn,		DD$coarsesyndrome);
    switch (elrp->elsubid.subid_type)
      {
      case ELMSCP_CNTRL:
      case ELTMSCP_CNTRL:
        os_std(&elrp->elsubid.subid_num,
		OS$gen_short,		DD$controller);
        os_std(&elrp->elsubid.subid_unitnum,
		OS$gen_short,		DD$unitnumber);
        os_std(&elrp->elsubid.subid_ctldevtyp,
 		OS$mscp_ctltyp,		DD$devtype);
	sds_ptr->subtype = DD$DSK_COM_SDS;
	if (ini_seg(sds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        if (mscp_bld(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr, elrp) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->U_MSCP.mslg_event,
		OS$gen_short,		DD$dsastat);
      break;

/*********************** SCSI CONTROLLER ERRORS *******************/
      case ELSCSI_CNTRL:

        if (scsi_bld(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,elrp) ==
                EI$FAIL)
        status = EI$FAIL;
      break;
/*********************** BVP CONTROLLER ERRORS  *******************/

      case ELBI_BVP:
        os_std(&elrp->elsubid.subid_num,
		OS$gen_short,		DD$controller);
        os_std(&elrp->elsubid.subid_unitnum,
		OS$gen_short,		DD$unitnumber);
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$bvptyp,		DD$devtype);
	switch (elrp->elsubid.subid_ctldevtyp)
          {
          case ELBVP_ACP:
          case ELBVP_SHDWFAX:
	    cds_ptr->subtype = DD$LYNX_ACP_CDS;
	    sds_ptr->subtype = DD$LYNX_ACP_SDS;
	    ads_ptr->subtype = DD$LYNX_ACP_ADS;
	    if (ini_seg(cds_ptr) == EI$FAIL)
	        status = EI$FAIL;
	    if (ini_seg(sds_ptr) == EI$FAIL)
	        status = EI$FAIL;
	    if (ini_seg(ads_ptr) == EI$FAIL)
	        status = EI$FAIL;
            os_std(&elrp->el_body.ellxerr.bi_csr,
		OS$gen_long,		DD$bicsr);
            os_std(&elrp->el_body.ellxerr.bi_buserr,
		OS$gen_long,		DD$bibuserreg);
            os_std(&elrp->el_body.ellxerr.port_error,
		OS$gen_long,		DD$bvperr);
            os_std(&elrp->el_body.ellxerr.port_data,
		OS$gen_long,		DD$port_data);
            os_std(&elrp->el_body.ellxerr.ACP_status,
		OS$gen_long,		DD$lx_acp_stat);
            os_std(&elrp->el_body.ellxerr.test_num,
		OS$gen_long,		DD$lx_errtst);
            os_std(&elrp->el_body.ellxerr.subtest_num,
		OS$gen_long,		DD$lx_errsubtst);
            os_std(elrp->el_body.ellxerr.error_name,
		OS$gen_long,		DD$lx_errname);
            os_std(&elrp->el_body.ellxerr.PR_byte1,
		OS$gen_long,		DD$lx_pr1);
            os_std(&elrp->el_body.ellxerr.PR_byte2,
		OS$gen_long,		DD$lx_pr2);
            os_std(&elrp->el_body.ellxerr.PR_config,
		OS$gen_long,		DD$lx_prconf);
            os_std(&elrp->el_body.ellxerr.PR_rev,
		OS$gen_long,		DD$lx_prrev);

          break;
          default:
	    sds_ptr->subtype = DD$BVP_SDS;
	    if (ini_seg(sds_ptr) == EI$FAIL)
	        status = EI$FAIL;
            os_std(&elrp->el_body.elbvp.bvp_biic_typ,
		OS$gen_long,		DD$bidevreg);
            os_std(&elrp->el_body.elbvp.bvp_biic_csr,
		OS$gen_long,		DD$bicsr);
            os_std(&elrp->el_body.elbvp.bvp_pcntl,
		OS$gen_long,		DD$bvpcntl);
	    switch (elrp->el_body.elbvp.bvp_pstatus)
	      {
              case 1:
              case 3:
              case 4:
	        cds_ptr->subtype = DD$BVP_BIER_CDS;
	        if (ini_seg(cds_ptr) == EI$FAIL)
	            status = EI$FAIL;
                os_std(&elrp->el_body.elbvp.bvp_pstatus,
		  OS$gen_long,		DD$bvpsts);
                os_std(&elrp->el_body.elbvp.bvp_perr,
		  OS$gen_long,		DD$bibuserreg);
                os_std(&elrp->el_body.elbvp.bvp_pdata,
		  OS$gen_long,		DD$biaddr);
              break;
              default:
	        cds_ptr->subtype = DD$BVP_GEN_CDS;
	        if (ini_seg(cds_ptr) == EI$FAIL)
	            status = EI$FAIL;
                os_std(&elrp->el_body.elbvp.bvp_pstatus,
		  OS$gen_long,		DD$bvpsts);
                os_std(&elrp->el_body.elbvp.bvp_perr,
		  OS$gen_long,		DD$bvperr);
                os_std(&elrp->el_body.elbvp.bvp_pdata,
		  OS$gen_long,		DD$port_data);
              break;
              }
          break;
          }
      break;

/*************************  CI CONTROLLER ERRORS  ******************/

      case ELCI_ATTN:
        cds_ptr->subtype = DD$CI_GEN_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	  status = EI$FAIL;
        sds_ptr->subtype = DD$CI_DATTN_SDS;
	if (ini_seg(sds_ptr) == EI$FAIL)
	  status = EI$FAIL;
        ads_ptr->subtype = DD$CI_DATTN_ADS;
	if (ini_seg(ads_ptr) == EI$FAIL)
	  status = EI$FAIL;

	var_elrp = (u_char *)Elcidattn(elrp);

        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$cityp,		DD$devtype);
        os_std(&elrp->elsubid.subid_errcode,
		OS$gen_long,		DD$cierrcode);
        temp_long = (elrp->elsubid.subid_errcode & 0xe7ffffff);
	os_std(&temp_long,
		OS$cierrcod,		DD$cierrcod);
        os_std(Elcicommon(elrp)->ci_lpname,
		OS$ci_lpname,		DD$cilpname);
        os_std(Elcicommon(elrp)->ci_lname,
		OS$ci_lname,		DD$cilname);
        os_std(Elcicommon(elrp)->ci_lsysid,
		OS$gen_bv_6,		DD$cilsysid);
        os_std(Elcicommon(elrp)->ci_lsaddr,
		OS$gen_bv_6,		DD$cilsaddr);
        os_std(&Elcicommon(elrp)->ci_nerrs,
		OS$gen_short,		DD$cierrs);
        os_std(&Elcicommon(elrp)->ci_nreinits,
		OS$gen_short,		DD$cireinits);

/**************************** CI REGS ********************************/

        if ((Elcicommon(elrp)->ci_optfmask1 & CI_REGS) == CI_REGS)
	  {
          os_std(&Elciciregs(var_elrp)->ci_pfaddr,
		OS$gen_long,		DD$cipfaddr);
          os_std(&Elciciregs(var_elrp)->ci_pesr,
		OS$gen_long,		DD$ciper);

	  if ((elrp->elsubid.subid_ctldevtyp == ELCIHPT_CI750) ||
	      (elrp->elsubid.subid_ctldevtyp == ELCIHPT_CI780))
	    {
            os_std(&Elciciregs(var_elrp)->ci_cnfr,
		OS$gen_long,		DD$cipcnf);
            os_std(&Elciciregs(var_elrp)->ci_pmcsr,
		OS$gen_long,		DD$cipmcsr);
            os_std(&Elciciregs(var_elrp)->ci_psr,
		OS$gen_long,		DD$cipsr);
            os_std(&Elciciregs(var_elrp)->ci_ppr,
		OS$gen_long,		DD$cippr);
	    }
	  else if ((elrp->elsubid.subid_ctldevtyp == ELCIHPT_CIBCAAA) ||
	           (elrp->elsubid.subid_ctldevtyp == ELCIHPT_CIBCABA))
	    {
            os_std(&Elciciregs(var_elrp)->ci_cnfr,
		OS$gen_long,		DD$cibcipcnf);
            os_std(&Elciciregs(var_elrp)->ci_pmcsr,
		OS$gen_long,		DD$cibcapmcsr);
            os_std(&Elciciregs(var_elrp)->ci_psr,
		OS$gen_long,		DD$cibcapsr);
            os_std(&Elciciregs(var_elrp)->ci_ppr,
		OS$gen_long,		DD$cibcappr);
	    }
	  else if (elrp->elsubid.subid_ctldevtyp == ELCIHPT_CIBCI)
	    {
            os_std(&Elciciregs(var_elrp)->ci_cnfr,
		OS$gen_long,		DD$cibcipcnf);
            os_std(&Elciciregs(var_elrp)->ci_pmcsr,
		OS$gen_long,		DD$cipmcsr);
            os_std(&Elciciregs(var_elrp)->ci_psr,
		OS$gen_long,		DD$cipsr);
            os_std(&Elciciregs(var_elrp)->ci_ppr,
		OS$gen_long,		DD$cippr);
	    }
	  else if (elrp->elsubid.subid_ctldevtyp == ELCIHPT_CIXCD)
	    {
            os_std(&Elciciregs(var_elrp)->ci_cnfr,
		OS$gen_long,		DD$cipcnf);
            os_std(&Elciciregs(var_elrp)->ci_pmcsr,
		OS$gen_long,		DD$cixcdpmcsr);
            os_std(&Elciciregs(var_elrp)->ci_psr,
		OS$gen_long,		DD$cixcdpsr);
            os_std(&Elciciregs(var_elrp)->ci_ppr,
		OS$gen_long,		DD$cixcdppr);
	    }
	  var_elrp += sizeof(struct ci_regs);
	  }

/**************************** BI REGS ********************************/

        if ((Elcicommon(elrp)->ci_optfmask1 & CI_BIREGS) == CI_BIREGS)
	  {
          os_std(&Elcibiregs(var_elrp)->bi_typ,
		OS$gen_long,		DD$bidevreg);
          os_std(&Elcibiregs(var_elrp)->bi_ctrl,
		OS$gen_long,		DD$bicsr);
          os_std(&Elcibiregs(var_elrp)->bi_err,
		OS$gen_long,		DD$bibuserreg);
          os_std(&Elcibiregs(var_elrp)->bi_err_int,
		OS$gen_long,		DD$bierint);
          os_std(&Elcibiregs(var_elrp)->bi_int_dst,
		OS$gen_long,		DD$biintdst);
	  var_elrp += sizeof(struct bi_regs);
	  }

/*************************** XMI REGS ********************************/

        if ((Elcicommon(elrp)->ci_optfmask1 & CI_XMIREGS) == CI_XMIREGS)
          {
          os_std(&Elcixmiregs(var_elrp)->xdev,
		OS$gen_long,		DD$xdev);
          os_std(&Elcixmiregs(var_elrp)->xbe,
		OS$gen_long,		DD$xbe);
          os_std(&Elcixmiregs(var_elrp)->xfadrl,
		OS$gen_long,		DD$xfadrl);
          os_std(&Elcixmiregs(var_elrp)->xfadrh,
		OS$gen_long,		DD$xfadrh);
          os_std(&Elcixmiregs(var_elrp)->pidr,
		OS$gen_long,		DD$pidr);
          os_std(&Elcixmiregs(var_elrp)->pvr,
		OS$gen_long,		DD$pvr);
	  var_elrp += sizeof(struct cixmi_regs);
	  }

/***************************  UCODE  *********************************/

        if ((Elcicommon(elrp)->ci_optfmask1 & CI_UCODE) == CI_UCODE)
	  {
          os_std(&Elciucode(var_elrp)->ci_addr,
		OS$gen_long,		DD$ciaddr);
          os_std(&Elciucode(var_elrp)->ci_bvalue,
		OS$gen_long,		DD$cibvalue);
          os_std(&Elciucode(var_elrp)->ci_gvalue,
		OS$gen_long,		DD$cigvalue);
	  }

/***************************  REVLEV  ********************************/

        if ((Elcicommon(elrp)->ci_optfmask1 & CI_REVLEV) == CI_REVLEV)
	  {
          os_std(&Elcirevlev(var_elrp)->ci_romlev,
		OS$gen_long,		DD$cirom);
          os_std(&Elcirevlev(var_elrp)->ci_ramlev,
		OS$gen_long,		DD$ciram);
	  }

/************************** CPU REVLEV *******************************/

        if ((Elcicommon(elrp)->ci_optfmask1 & CI_CPUREVLEV) == CI_CPUREVLEV)
	  {
          os_std(&Elcicpurevlev(var_elrp)->ci_hwtype,
		OS$gen_long,		DD$cihwtype);
          os_std(&Elcicpurevlev(var_elrp)->ci_mincpurev,
		OS$gen_long,		DD$cimincpurev);
          os_std(&Elcicpurevlev(var_elrp)->ci_currevlev,
		OS$gen_long,		DD$cicurrevlev);
	  }

      break;

      case ELCI_LPKT:
        cds_ptr->subtype = DD$CI_GEN_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	  status = EI$FAIL;
        sds_ptr->subtype = DD$CI_COMMON_SDS;
	if (ini_seg(sds_ptr) == EI$FAIL)
	  status = EI$FAIL;
        ads_ptr->subtype = DD$CI_LPKT_ADS;
	if (ini_seg(ads_ptr) == EI$FAIL)
	  status = EI$FAIL;
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$cityp,		DD$devtype);
        os_std(&elrp->elsubid.subid_errcode,
		OS$gen_long,		DD$cierrcode);
        temp_long = (elrp->elsubid.subid_errcode & 0xe7ffffff);
	os_std(&temp_long,
		OS$cierrcod,		DD$cierrcod);
        os_std(Elcicommon(elrp)->ci_lpname,
		OS$ci_lpname,		DD$cilpname);
        os_std(Elcicommon(elrp)->ci_lname,
		OS$ci_lname,		DD$cilname);
        os_std(Elcicommon(elrp)->ci_lsysid,
		OS$gen_bv_6,		DD$cilsysid);
        os_std(Elcicommon(elrp)->ci_lsaddr,
		OS$gen_bv_6,		DD$cilsaddr);
        os_std(&Elcicommon(elrp)->ci_nerrs,
		OS$gen_short,		DD$cierrs);
        os_std(&Elcicommon(elrp)->ci_nreinits,
		OS$gen_short,		DD$cireinits);

/*************************** LCOMMON *********************************/

        if ((Elcicommon(elrp)->ci_optfmask1 & CI_LCOMMON) == CI_LCOMMON)
	  {
          os_std(Elcilcommon(elrp)->ci_rsaddr,
		OS$gen_bv_6,		DD$cirsaddr);
          os_std(Elcilcommon(elrp)->ci_rsysid,
		OS$gen_bv_6,		DD$cirsysid);
          os_std(Elcilcommon(elrp)->ci_rname,
		OS$ci_rname,		DD$cirname);
	  }

/**************************** PACKET *********************************/

        if ((Elcicommon(elrp)->ci_optfmask1 & CI_PACKET) == CI_PACKET)
	  {
          os_std(&Elcipacket(elrp)->ci_port,
		  OS$gen_tiny,		DD$ciport);
          os_std(&Elcipacket(elrp)->ci_status,
		  OS$gen_tiny,		DD$cistatus);
          os_std(&Elcipacket(elrp)->ci_opcode,
 		  OS$gen_tiny,		DD$ciopcode);
          os_std(&Elcipacket(elrp)->ci_flags,
		  OS$gen_tiny,		DD$ciflags);
	  }
      break;

/******************  OTHER CONTROLLER ERRORS *********************/

      case ELMSI_ATTN:
      case ELMSI_LPKT:
        cds_ptr->subtype = DD$MSI_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	  status = EI$FAIL;
    
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$cityp,		DD$devtype);
        os_std(&elrp->elsubid.subid_errcode,
		OS$gen_long,		DD$cierrcode);
        temp_long = (elrp->elsubid.subid_errcode & 0xe7ffffff);
	os_std(&temp_long,
		OS$msi_errcode,		DD$msi_errcode);

	os_std(Elmsicommon(elrp)->msi_lpname,
		OS$ci_lpname,		DD$cilpname);
  	os_std(Elmsicommon(elrp)->msi_lname,
		OS$ci_lname,		DD$cilname);
	os_std(Elmsicommon(elrp)->msi_lsysid,
		OS$gen_bv_6,		DD$cilsysid);
	os_std(Elmsicommon(elrp)->msi_lsaddr,
		OS$gen_bv_6,		DD$cilsaddr);
	os_std(&Elmsicommon(elrp)->msi_nerrs,
		OS$gen_short,		DD$cierrs);
	os_std(&Elmsicommon(elrp)->msi_nreinits,
  		OS$gen_short,		DD$cireinits);

	if (elrp->elsubid.subid_type == ELMSI_ATTN)
	  {
          if ((Elmsicommon(elrp)->msi_optfmask1 & MSI_REGS) != MSI_REGS)
  	    break;

          ads_ptr->subtype = DD$MSI_DATTN_ADS;
	  if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
          os_std(&Elmsiregs(elrp)->msi_csr,
		OS$gen_short,		DD$msi_csr);
          os_std(&Elmsiregs(elrp)->msi_dssr,
		OS$gen_short,		DD$msi_dssr);
          os_std(&Elmsiregs(elrp)->msi_idr,
		OS$gen_short,		DD$msiidr);
          os_std(&Elmsiregs(elrp)->msi_slcs,
		OS$gen_short,		DD$msislcs);
          os_std(&Elmsiregs(elrp)->msi_destat,
		OS$gen_short,		DD$msidestat);
          os_std(&Elmsiregs(elrp)->msi_tr,
		OS$gen_short,		DD$msitr);
          os_std(&Elmsiregs(elrp)->msi_dmctlr,
		OS$gen_short,		DD$msidmctlr);
          os_std(&Elmsiregs(elrp)->msi_dmlotc,
		OS$gen_short,		DD$msidmlotc);
          os_std(&Elmsiregs(elrp)->msi_dmaaddrl,
		OS$gen_short,		DD$msidmaaddrl);
          os_std(&Elmsiregs(elrp)->msi_dmaaddrh,
		OS$gen_short,		DD$msidmaaddrh);
          os_std(&Elmsiregs(elrp)->msi_stlp,
		OS$gen_short,		DD$msistlp);
          os_std(&Elmsiregs(elrp)->msi_tlp,
		OS$gen_short,		DD$msitlp);
          os_std(&Elmsiregs(elrp)->msi_ilp,
		OS$gen_short,		DD$msiilp);
          os_std(&Elmsiregs(elrp)->msi_dscr,
		OS$gen_short,		DD$msidscr);
          os_std(&Elmsiregs(elrp)->msi_dstat,
		OS$gen_short,		DD$msidstat);
          os_std(&Elmsiregs(elrp)->msi_dcr,
		OS$gen_short,		DD$msidcr);
          os_std(&Elmsiregs(elrp)->msi_save_dssr,
		OS$gen_short,		DD$msisavedssr);
          os_std(&Elmsiregs(elrp)->msi_save_dstat,
		OS$gen_short,		DD$msisavedstat);
	  break;
	  }

	if (elrp->elsubid.subid_type == ELMSI_LPKT)
	  {
	  var_elrp = (u_char *)Elmsilcommon(elrp);

          ads_ptr->subtype = DD$MSI_LPKT_ADS;
	  if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;

	  if ((Elmsicommon(elrp)->msi_optfmask1 & MSI_LCOMMON) == MSI_LCOMMON)
	    {
            os_std(Elmsilcommon(elrp)->msi_rsaddr,
		OS$gen_bv_6,		DD$cirsaddr);
	    os_std(Elmsilcommon(elrp)->msi_rsysid,
		OS$gen_bv_6,		DD$cirsysid);
	    os_std(Elmsilcommon(elrp)->msi_rname,
		OS$ci_rname,		DD$cirname);
	    var_elrp += sizeof(struct msi_lcommon);
	    }
          if ((Elmsicommon(elrp)->msi_optfmask1 & MSI_CMDBLK) == MSI_CMDBLK)
	    {
	    os_std(&Elmsicmdblk(var_elrp)->msi_thread,
		  OS$gen_short,		DD$msithread);
            os_std(&Elmsicmdblk(var_elrp)->msi_status,
		  OS$gen_short,		DD$cistatus);
	    os_std(&Elmsicmdblk(var_elrp)->msi_command,
		  OS$gen_short,		DD$msicommand);
            os_std(&Elmsicmdblk(var_elrp)->msi_opcode,
 		  OS$gen_short,		DD$ciopcode);
	    os_std(&Elmsicmdblk(var_elrp)->msi_dst,
		  OS$gen_tiny,		DD$msidst);
	    os_std(&Elmsicmdblk(var_elrp)->msi_src,
		  OS$gen_tiny,		DD$msisrc);
	    os_std(&Elmsicmdblk(var_elrp)->msi_length,
		  OS$gen_short,		DD$msilength);
	    var_elrp += sizeof(struct msi_cmdblk);
	    }
          if ((Elmsicommon(elrp)->msi_optfmask1 & MSI_PACKET) == MSI_PACKET)
	    {
            os_std(&Elmsipacket(var_elrp)->msi_opcode,
 		  OS$gen_tiny,		DD$pkt_opcode);
	    os_std(&Elmsipacket(var_elrp)->msi_flags,
		  OS$gen_tiny,		DD$msiflags);
	    }
         }
       break;

/*********************************************************************/

      case ELUQ_ATTN:
        os_std(&elrp->elsubid.subid_num,
		OS$gen_short,		DD$controller);
        os_std(&elrp->elsubid.subid_unitnum,
		OS$gen_short,		DD$unitnumber);
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$uqtyp,		DD$devtype);
	switch (elrp->elsubid.subid_ctldevtyp)
	  {
	  case ELUQHPT_UDA50:
          case ELUQHPT_UDA50A:
	    cds_ptr->subtype = DD$UDA5X_ATTN_CDS;
            if (ini_seg(cds_ptr) == EI$FAIL)
	        status = EI$FAIL;
            os_std(&elrp->el_body.eluq.sa_contents,
		OS$gen_long,		DD$uda5sa);
	  break;
	  case ELUQHPT_TK50:
          case ELUQHPT_TK70:
	    cds_ptr->subtype = DD$TK57_ATTN_CDS;
            if (ini_seg(cds_ptr) == EI$FAIL)
	        status = EI$FAIL;
            os_std(&elrp->el_body.eluq.sa_contents,
		OS$gen_long,		DD$tk57sa);
	  break;
	  case ELUQHPT_KDB50:
	    cds_ptr->subtype = DD$KDB50_ATTN_CDS;
            if (ini_seg(cds_ptr) == EI$FAIL)
	        status = EI$FAIL;
            os_std(&elrp->el_body.eluq.sa_contents,
		OS$gen_long,		DD$kdb5sa);
	  break;
	  case ELUQHPT_RC25:
	    cds_ptr->subtype = DD$RC25_ATTN_CDS;
            if (ini_seg(cds_ptr) == EI$FAIL)
	        status = EI$FAIL;
            os_std(&elrp->el_body.eluq.sa_contents,
		OS$gen_long,		DD$rc25sa);
	  break;
	  default:
	    cds_ptr->subtype = DD$UQ_PORT_CDS;
	    if (ini_seg(cds_ptr) == EI$FAIL)
	        status = EI$FAIL;
            os_std(&elrp->elsubid.subid_num,
		OS$gen_short,		DD$busnum);
            os_std(&elrp->el_body.eluq.sa_contents,
		OS$gen_long,		DD$sareg);
	  break;
	  }
      break;

/*********************************************************************/

            /* TEMP until it is defined in all pools */ 
#ifdef ELXMI_XNA

      case ELXMI_XNA:
      case ELBI_XNA:
	cds_ptr->subtype=DD$XNA_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	ads_ptr->subtype=DD$XNA_ADS;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
	switch (elrp->elsubid.subid_errcode)
	{
	case XNA_FATAL: 
	/* Decode the fatal common registers for XMI and BI. */
	os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_fatal.xna_type,
		OS$gen_long,		DD$xna_type);
	os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_fatal.xna_date_lo,
		OS$gen_long,		DD$xna_date_lo);
	os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_fatal.xna_date_hi,
		OS$gen_long,		DD$xna_date_hi);
	os_std(make_addr_vector(elrp, &elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_fatal.xna_r0)+1,
		OS$gen_cntvecl,		DD$r0_r12);
	  if (elrp->elsubid.subid_type == ELXMI_XNA)
	  /* decode the XMI specific fatal registers. */
	  {
	  os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_fatal.xna_xbe,
		OS$gen_long,		DD$xbe);
	  os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_fatal.xna_xfadr,
		OS$gen_long,		DD$xfadr);
	  os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_fatal.xna_xfaer,
		OS$gen_long,		DD$xfaer);
	  os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_fatal.xna_gacsr,
		OS$gen_long,		DD$gacsr);
	  os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_fatal.xna_diag,
		OS$gen_long,		DD$diag);
	  os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_fatal.xna_xpst_init,
		OS$gen_long,		DD$xpst_init);
	  os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_fatal.xna_xpd1_init,
		OS$gen_long,		DD$xpd1_init);
	  os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_fatal.xna_xpd2_init,
		OS$gen_long,		DD$xpd2_init);
	  os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_fatal.xna_xpst_final,
		OS$gen_long,		DD$xpst_final);
	  os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_fatal.xna_xpd1_final,
		OS$gen_long,		DD$xpd1_final);
	  os_std(make_addr_vector(elrp, elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_fatal.xnastack)+1,
		OS$gen_cntvecl,		DD$stack);
	  }
	  if (elrp->elsubid.subid_type == ELBI_XNA)
	  /* decode the BI specific fatal registers. */
	  {
	  os_std(&elrp->el_body.elxna.xnatype.xnabi.xnabi_fatal.xna_ber,
		OS$gen_long,		DD$ber);
	  os_std(&elrp->el_body.elxna.xnatype.xnabi.xnabi_fatal.xna_bicsr,
		OS$gen_long,		DD$bicsr);
	  os_std(&elrp->el_body.elxna.xnatype.xnabi.xnabi_fatal.xna_bci3_csr,
		OS$gen_long,		DD$bci3_csr);
	  os_std(&elrp->el_body.elxna.xnatype.xnabi.xnabi_fatal.xna_xpst_init,
		OS$gen_long,		DD$xpst_init);
	  os_std(&elrp->el_body.elxna.xnatype.xnabi.xnabi_fatal.xna_xpd1_init,
		OS$gen_long,		DD$xpd1_init);
	  os_std(&elrp->el_body.elxna.xnatype.xnabi.xnabi_fatal.xna_xpd2_init,
		OS$gen_long,		DD$xpd2_init);
	  os_std(&elrp->el_body.elxna.xnatype.xnabi.xnabi_fatal.xna_xpst_final,
		OS$gen_long,		DD$xpst_final);
	  os_std(&elrp->el_body.elxna.xnatype.xnabi.xnabi_fatal.xna_xpd1_final,
		OS$gen_long,		DD$xpd1_final);
	  os_std(make_addr_vector(elrp, elrp->el_body.elxna.xnatype.xnabi.xnabi_fatal.xnastack)+1,
		OS$gen_cntvecl,		DD$stack);
	  }
   	break;

	case XNA_NONFATAL:
	/* Decode the nonfatal common registers for XMI and BI  */
	os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_nonfatal.xna_date_lo,
		OS$gen_long,		DD$xna_date_lo);
	os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_nonfatal.xna_date_hi,
		OS$gen_long,		DD$xna_date_hi);
	os_std(make_addr_vector(elrp, &elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_nonfatal.xna_r0)+1,
		OS$gen_cntvecl,		DD$r0_r3);
	if (elrp->elsubid.subid_type == ELXMI_XNA)
	/* decode the XMI specific non-fatal registers.  */
	  {
	  os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_nonfatal.xna_xbe,
		OS$gen_long,		DD$xbe);
	  os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_nonfatal.xna_xfadr,
		OS$gen_long,		DD$xfadr);
	  os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_nonfatal.xna_xfaer,
		OS$gen_long,		DD$xfaer);
	  os_std(&elrp->el_body.elxna.xnatype.xnaxmi.xnaxmi_nonfatal.xna_gacsr,
		OS$gen_long,		DD$gacsr);
	  }

 	if (elrp->elsubid.subid_type == ELBI_XNA)
	  /* decode the BI specific non-fatal registers. */
	  {
	  os_std(&elrp->el_body.elxna.xnatype.xnabi.xnabi_nonfatal.xna_ber,
		OS$gen_long,		DD$ber);
	  }
	break;
	}
            /* TEMP until it is defined in all pools */ 
      break; 
#endif ELXMI_XNA
/*********************************************************************/

      case ELBI_BLA:
        os_std(&elrp->elsubid.subid_num,
		OS$gen_short,		DD$controller);
        os_std(&elrp->elsubid.subid_unitnum,
		OS$gen_short,		DD$unitnumber);
        os_std(&elrp->elsubid.subid_type,
		OS$blatyp,		DD$devtype);
	cds_ptr->subtype = DD$BLA_ERR_CDS;
	sds_ptr->subtype = DD$BLA_ERR_SDS;
	ads_ptr->subtype = DD$BLA_ERR_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(sds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->elsubid.subid_num,
		OS$gen_short,		DD$bibsnum);
        os_std(&elrp->el_body.elbigen.bigen_dev,
		OS$gen_long,		DD$bidevreg);
        os_std(&elrp->el_body.elbigen.bigen_bicsr,
		OS$gen_long,		DD$bicsr);
        os_std(&elrp->el_body.elbigen.bigen_ber,
		OS$gen_long,		DD$bibuserreg);
        os_std(&elrp->el_body.elbigen.bigen_csr,
		OS$gen_long,		DD$blacsr);
        os_std(&elrp->el_body.elbigen.bigen_pc,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.elbigen.bigen_psl,
		OS$gen_long,		DD$psl);
      break;

/*********************************************************************/

      case ELFZA:
        os_std(&elrp->elsubid.subid_num,
                OS$gen_short,           DD$controller);
        os_std(&elrp->elsubid.subid_unitnum,
                OS$gen_short,           DD$unitnumber);
/*
        os_std(&elrp->elsubid.subid_type,
                OS$blatyp,              DD$devtype);
*/
        cds_ptr->subtype = DD$FDDI_CDS;
        if (ini_seg(cds_ptr) == EI$FAIL)
            status = EI$FAIL;
        os_std(&elrp->el_body.el_fza.fza_id,
                OS$gen_short,           DD$fza_id);
        os_std(&elrp->el_body.el_fza.reset_count,
                OS$gen_short,           DD$fza_rcnt);
        os_std(&elrp->el_body.el_fza.timestamp_hi,
                OS$gen_long,            DD$fza_tsmp_hi);
        os_std(&elrp->el_body.el_fza.timestamp_lo,
                OS$gen_short,           DD$fza_tsmp_lo);
        os_std(&elrp->el_body.el_fza.write_count,
                OS$gen_short,           DD$fza_w_cnt);
        os_std(&elrp->el_body.el_fza.int_reason,
                OS$gen_short,           DD$fza_int);
        os_std(&elrp->el_body.el_fza.ext_reason,
                OS$gen_short,           DD$fza_ext);
        os_std(&elrp->el_body.el_fza.cmd_next_ptr,
                OS$gen_long,            DD$fza_cmd_np);
        os_std(&elrp->el_body.el_fza.cmd_next_cmd,
                OS$gen_long,            DD$fza_cmd_nc);
        os_std(&elrp->el_body.el_fza.dma_next_rmc_ptr,
                OS$gen_long,            DD$fza_dma_nrp);
        os_std(&elrp->el_body.el_fza.dma_next_rmc_descr,
                OS$gen_long,            DD$fza_dma_nrd);
        os_std(&elrp->el_body.el_fza.dma_next_rmc_own,
                OS$gen_long,            DD$fza_dma_nro);
        os_std(&elrp->el_body.el_fza.dma_next_host_ptr,
                OS$gen_long,            DD$fza_dma_nhp);
        os_std(&elrp->el_body.el_fza.dma_next_host_descr,
                OS$gen_long,            DD$fza_dma_nhd);
        os_std(&elrp->el_body.el_fza.lmgr_next_ptr,
                OS$gen_long,            DD$fza_lmgr_np);
        os_std(&elrp->el_body.el_fza.lmgr_next_descr,
                OS$gen_long,            DD$fza_lmgr_nd);
        os_std(&elrp->el_body.el_fza.smt_next_put_ptr,
                OS$gen_long,            DD$fza_smt_npp);
        os_std(&elrp->el_body.el_fza.smt_next_put_descr,
                OS$gen_long,            DD$fza_smt_npd);
        os_std(&elrp->el_body.el_fza.smt_next_take_ptr,
                OS$gen_long,            DD$fza_smt_ntp);
        os_std(&elrp->el_body.el_fza.smt_next_take_descr,
                OS$gen_long,            DD$fza_smt_ntd);
        os_std(&elrp->el_body.el_fza.pm_csr,
                OS$gen_short,           DD$fza_pm_csr);
        os_std(&elrp->el_body.el_fza.int_68k_present,
                OS$gen_short,           DD$fza_int_68k);
        os_std(&elrp->el_body.el_fza.int_68k_mask,
                OS$gen_short,           DD$fza_int_68k_msk);
        os_std(&elrp->el_body.el_fza.pint_event,
                OS$gen_short,           DD$fza_port_int);
        os_std(&elrp->el_body.el_fza.port_ctrl_a,
                OS$gen_short,           DD$fza_port_cntl_a);
        os_std(&elrp->el_body.el_fza.port_ctrl_a_mask,
                OS$gen_short,           DD$fza_port_cntl_a_msk);
        os_std(&elrp->el_body.el_fza.port_ctrl_b,
                OS$gen_short,           DD$fza_port_cntl_b);
        os_std(&elrp->el_body.el_fza.port_status,
                OS$gen_short,           DD$fza_port_st);
        os_std(&elrp->el_body.el_fza.ram_rom_map,
                OS$gen_short,           DD$fza_ram_rom);
        os_std(&elrp->el_body.el_fza.phy_csr,
                OS$gen_short,           DD$fza_phy_csr);
        os_std(&elrp->el_body.el_fza.dma_done,
                OS$gen_short,           DD$fza_dma_done);
        os_std(&elrp->el_body.el_fza.dma_err,
                OS$gen_short,           DD$fza_dma_err);
        os_std(&elrp->el_body.el_fza.dma_start_lo,
                OS$gen_short,           DD$fza_dma_s_lo);
        os_std(&elrp->el_body.el_fza.dma_start_hi,
                OS$gen_short,           DD$fza_dma_s_hi);
        os_std(&elrp->el_body.el_fza.rmc_cmd,
                OS$gen_short,           DD$fza_rmc_cmd);
        os_std(&elrp->el_body.el_fza.rmc_mode,
                OS$gen_short,           DD$fza_rmc_mode);
        os_std(&elrp->el_body.el_fza.rmc_rcv_page,
                OS$gen_short,           DD$fza_rmc_rcv_pg);
        os_std(&elrp->el_body.el_fza.rmc_rcv_params,
                OS$gen_short,           DD$fza_rmc_rcv_par);
        os_std(&elrp->el_body.el_fza.rmc_xmt_page,
                OS$gen_short,           DD$fza_rmc_xmt_pg);
        os_std(&elrp->el_body.el_fza.rmc_xmt_params,
                OS$gen_short,           DD$fza_rmc_xmt_par);
        os_std(&elrp->el_body.el_fza.rmc_interrupts,
                OS$gen_short,           DD$fza_rmc_intr);
        os_std(&elrp->el_body.el_fza.rmc_int_mask,
                OS$gen_short,           DD$fza_rmc_int_m);
        os_std(&elrp->el_body.el_fza.rmc_chan_status,
                OS$gen_short,           DD$fza_rmc_chan);
        os_std(&elrp->el_body.el_fza.mac_rcv_cntrl,
                OS$gen_short,           DD$fza_mac_rcv_ctl);
        os_std(&elrp->el_body.el_fza.mac_xmt_cntrl,
                OS$gen_short,           DD$fza_mac_xmt_ctl);
        os_std(&elrp->el_body.el_fza.mac_int_mask_a,
                OS$gen_short,           DD$fza_mac_int_a);
        os_std(&elrp->el_body.el_fza.mac_int_mask_b,
                OS$gen_short,           DD$fza_mac_int_b);
        os_std(&elrp->el_body.el_fza.mac_rcv_status,
                OS$gen_short,           DD$fza_mac_rcv);
        os_std(&elrp->el_body.el_fza.mac_xmt_status,
                OS$gen_short,           DD$fza_mac_xmt);
        os_std(&elrp->el_body.el_fza.mac_mla_a,
                OS$gen_short,           DD$fza_mac_mla_a);
        os_std(&elrp->el_body.el_fza.mac_mla_b,
                OS$gen_short,           DD$fza_mac_mla_b);
        os_std(&elrp->el_body.el_fza.mac_mla_c,
                OS$gen_short,           DD$fza_mac_mla_c);
        os_std(&elrp->el_body.el_fza.mac_t_req,
                OS$gen_short,           DD$fza_mac_t_req);
        os_std(&elrp->el_body.el_fza.mac_tvx_value,
                OS$gen_short,           DD$fza_mac_tvx);
        os_std(&elrp->el_body.el_fza.crc,
                OS$gen_long,            DD$fza_crc);
      break;

/*************************  VME CONTROLLER ERRORS  ******************/

      case ELVME_DEV_CNTL:
	cds_ptr->subtype = DD$MVIB_CDS;	
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(elrp->el_body.elvme_devcntl.module,
		OS$pnc_asc,		DD$uba_name);
        os_std(&elrp->el_body.elvme_devcntl.num,
		OS$gen_short,		DD$vme_num);
        os_std(&elrp->el_body.elvme_devcntl.csr1,
		OS$gen_long,		DD$vme_csr1);
        os_std(&elrp->el_body.elvme_devcntl.csr2,
		OS$gen_long,		DD$vme_csr2);
        os_std(elrp->el_body.elvme_devcntl.el_vme_error_msg.msg_asc,
		OS$pnc_asc,		DD$message);

/* el_vba structure */
        os_std(&elrp->elsubid.subid_errcode,
		OS$gen_long,		DD$vme_errcode);
        os_std(&elrp->el_body.elvme_devcntl.elvba.elvba_reg.elmvib.mvib_viacsr,
		OS$gen_long,		DD$viacsr);
        os_std(&elrp->el_body.elvme_devcntl.elvba.elvba_reg.elmvib.mvib_csr,
		OS$gen_long,		DD$mvib_csr);
        os_std(&elrp->el_body.elvme_devcntl.elvba.elvba_reg.elmvib.mvib_vfadr,
		OS$gen_long,		DD$vfadr1);
	temp_long= (elrp->el_body.elvme_devcntl.elvba.elvba_reg.elmvib.mvib_vfadr >> 2)<<2 ;
	os_std(&temp_long,
		OS$gen_long,		DD$vfadr);
        os_std(&elrp->el_body.elvme_devcntl.elvba.elvba_reg.elmvib.mvib_cfadr,
		OS$gen_long,		DD$cfadr);
        os_std(&elrp->el_body.elvme_devcntl.elvba.elvba_reg.elmvib.mvib_ivs,
		OS$gen_long,		DD$ivs);
        os_std(&elrp->el_body.elvme_devcntl.elvba.elvba_reg.elmvib.mvib_besr,
		OS$gen_long,		DD$mvib_besr);
        os_std(&elrp->el_body.elvme_devcntl.elvba.elvba_reg.elmvib.mvib_errgi,
		OS$gen_long,		DD$errgi);
        os_std(&elrp->el_body.elvme_devcntl.elvba.elvba_reg.elmvib.mvib_lvb,
		OS$gen_long,		DD$lvb);
        os_std(&elrp->el_body.elvme_devcntl.elvba.elvba_reg.elmvib.mvib_err,
		OS$gen_long,		DD$mvib_err);


      default:
        unk_rec = TRUE;
      break;
      }
  break;

/*************** 105 ELCT_ADPTR  (Adapter Errors) ************/

  case ELCT_ADPTR:
    os_std(&elrp->elsubid.subid_class,
		OS$adaperr,		DD$devclass);
    os_std(&elrp->elsubid.subid_type,
		OS$adptyp,		DD$devtype);
	if( elrp->elsubid.subid_type != ELADP_SJASCM ) {
    	os_std(&elrp->elsubid.subid_unitnum,
			OS$gen_short,		DD$controller);
    	os_std(&elrp->elsubid.subid_num,
			OS$gen_short,		DD$unitnumber);
    	os_std(&elrp->elsubid.subid_type,
			OS$adpsyn,		DD$coarsesyndrome);
	}

    switch(elrp->elsubid.subid_type)
      {
      case ELADP_BUA:
	cds_ptr->subtype = DD$BUA_ERR_CDS;
	ads_ptr->subtype = DD$BUA_ERR_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elbigen.bigen_dev,
		OS$gen_long,		DD$bidevreg);
        os_std(&elrp->el_body.elbigen.bigen_bicsr,
		OS$gen_long,		DD$bicsr);
        os_std(&elrp->el_body.elbigen.bigen_ber,
		OS$gen_long,		DD$bibuserreg);
        os_std(&elrp->el_body.elbigen.bigen_csr,
		OS$gen_long,		DD$buacsr);
        os_std(&elrp->el_body.elbigen.bigen_fubar,
		OS$gen_long,		DD$buafub);
        os_std(&elrp->el_body.elbigen.bigen_pc,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.elbigen.bigen_psl,
		OS$gen_long,		DD$psl);
      break;
      case ELADP_UBA:
	cds_ptr->subtype = DD$UBA780_CDS;
	ads_ptr->subtype = DD$UBA78_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.eluba780.uba_cf,
		OS$gen_long,		DD$ubacf);
        os_std(&elrp->el_body.eluba780.uba_cr,
		OS$gen_long,		DD$ubacr);
        os_std(&elrp->el_body.eluba780.uba_sr,
		OS$gen_long,		DD$ubasr);
        os_std(&elrp->el_body.eluba780.uba_dcr,
		OS$gen_long,		DD$ubadcr);
        os_std(&elrp->el_body.eluba780.uba_fmer,
		OS$gen_long,		DD$ubafme);
        os_std(&elrp->el_body.eluba780.uba_fubar,
		OS$gen_long,		DD$ubafub);
        os_std(&elrp->el_body.eluba780.uba_pc,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.eluba780.uba_psl,
		OS$gen_long,		DD$psl);
      break;
      case ELADP_NBI:
	cds_ptr->subtype = DD$NMIADP_CDS;
	sds_ptr->subtype = DD$NMIADP_SDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(sds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elnmiadp.nmiadp_nbiacsr0,
		OS$gen_long,		DD$nbiacsr0);
        os_std(&elrp->el_body.elnmiadp.nmiadp_nbiacsr1,
		OS$gen_long,		DD$nbiacsr1);
        os_std(&elrp->el_body.elnmiadp.nmiadp_nbib0err,
		OS$gen_long,		DD$bibuserreg);
        os_std(&elrp->el_body.elnmiadp.nmiadp_nbib1err,
		OS$gen_long,		DD$bi1buserreg);
      break;

      case ELADP_XBI:
	cds_ptr->subtype = DD$XBI_XMA_CDS;
	sds_ptr->subtype = DD$XMI_BI_SDS;
	ads_ptr->subtype = DD$XMI_BI_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(sds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.el_xbi.xbi_node,
		OS$gen_long,		DD$xnode);
        os_std(&elrp->el_body.el_xbi.xbi_dtype,
		OS$gen_long,		DD$xdev);
        os_std(&elrp->el_body.el_xbi.xbi_fadr,
		OS$gen_long,		DD$xfadr);
        os_std(&elrp->el_body.el_xbi.xbi_arear,
		OS$gen_long,		DD$arear);
        os_std(&elrp->el_body.el_xbi.xbi_aesr,
		OS$gen_long,		DD$aesr);
        os_std(&elrp->el_body.el_xbi.xbi_aimr,
		OS$gen_long,		DD$aimr);
        os_std(&elrp->el_body.el_xbi.xbi_node,
		OS$gen_long,		DD$xnode);
        os_std(&elrp->el_body.el_xbi.xbi_aivintr,
		OS$gen_long,		DD$aivintr);
        os_std(&elrp->el_body.el_xbi.xbi_adg1,
		OS$gen_long,		DD$adg1);
        os_std(&elrp->el_body.el_xbi.xbi_bcsr,
		OS$gen_long,		DD$bcsr);
        os_std(&elrp->el_body.el_xbi.xbi_besr,
		OS$gen_long,		DD$besr);
        os_std(&elrp->el_body.el_xbi.xbi_bidr,
		OS$gen_long,		DD$bidr);
        os_std(&elrp->el_body.el_xbi.xbi_btim,
		OS$gen_long,		DD$btim);
        os_std(&elrp->el_body.el_xbi.xbi_bvor,
		OS$gen_long,		DD$bvor);
        os_std(&elrp->el_body.el_xbi.xbi_bvr,
		OS$gen_long,		DD$bvr);
        os_std(&elrp->el_body.el_xbi.xbi_bdcr1,
		OS$gen_long,		DD$bdcr1);
      break;

      case ELADP_NBW:
	cds_ptr->subtype = DD$NBWADP_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.el_nbwadp.star_csr0,
		OS$gen_long,		DD$star_csr0);
        os_std(&elrp->el_body.el_nbwadp.star_csr1,
		OS$gen_long,		DD$star_csr1);
        os_std(&elrp->el_body.el_nbwadp.nemo_csr0,
		OS$gen_long,		DD$nemo_csr0);
        os_std(&elrp->el_body.el_nbwadp.nemo_csr1,
		OS$gen_long,		DD$nemo_csr1);
        os_std(&elrp->el_body.el_nbwadp.nemo_csr6,
		OS$gen_long,		DD$nemo_csr6);
      break;

      case ELADP_XBIPLUS:
	cds_ptr->subtype = DD$XBI_PLUS_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.el_xbiplus.xbi_node,
		OS$gen_long,		DD$xnode);
        os_std(&elrp->el_body.el_xbiplus.xbi_dtype,
		OS$gen_long,		DD$dtype);
        os_std(&elrp->el_body.el_xbiplus.xbi_fadr,
		OS$gen_long,		DD$xfadr0);
        os_std(&elrp->el_body.el_xbiplus.xbi_arear,
		OS$gen_long,		DD$arear);
        os_std(&elrp->el_body.el_xbiplus.xbi_aesr,
		OS$gen_long,		DD$aesr);
        os_std(&elrp->el_body.el_xbiplus.xbi_aimr,
		OS$gen_long,		DD$aimr);
        os_std(&elrp->el_body.el_xbiplus.xbi_aivintr,
		OS$gen_long,		DD$aivintr);
        os_std(&elrp->el_body.el_xbiplus.xbi_adg1,
		OS$gen_long,		DD$adg1);
	os_std(&elrp->el_body.el_xbiplus.xbi_bcsr,
		OS$gen_long,		DD$bcsr);
	os_std(&elrp->el_body.el_xbiplus.xbi_besr,
		OS$gen_long,		DD$besr);
	os_std(&elrp->el_body.el_xbiplus.xbi_bidr,
		OS$gen_long,		DD$xbi_bidr);
	os_std(&elrp->el_body.el_xbiplus.xbi_btim,
		OS$gen_long,		DD$btim);
        os_std(&elrp->el_body.el_xbiplus.xbi_bvor,
		OS$gen_long,		DD$bvor);
        os_std(&elrp->el_body.el_xbiplus.xbi_bvr,
		OS$gen_long,		DD$bvr);
        os_std(&elrp->el_body.el_xbiplus.xbi_bdcr1,
		OS$gen_long,		DD$bdcr1);
        os_std(&elrp->el_body.el_xbiplus.xbi_autlr,
		OS$gen_long,		DD$autlr);
        os_std(&elrp->el_body.el_xbiplus.xbi_acsr,
		OS$gen_long,		DD$acsr);
        os_std(&elrp->el_body.el_xbiplus.xbi_arvr,
		OS$gen_long,		DD$arvr);
        os_std(&elrp->el_body.el_xbiplus.xbi_abear,
		OS$gen_long,		DD$abear);
        os_std(&elrp->el_body.el_xbiplus.xbi_xbe,
		OS$gen_long,		DD$xbe0);
        os_std(&elrp->el_body.el_xbiplus.xbi_xfaer,
		OS$gen_long,		DD$xfaer);
      break;

      case ELADP_SJASCM:
		status = v9000_bld_bi(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp);
      break;

      case ELADP_XJA:
	cds_ptr->subtype = DD$XJA_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	os_std(&elrp->el_body.el_xja.el_xja_xdev, OS$gen_long, DD$xja_xdev);
	os_std(&elrp->el_body.el_xja.el_xja_xber, OS$gen_long, DD$xja_xber);
	os_std(&elrp->el_body.el_xja.el_xja_xfadra, OS$gen_long, DD$xja_xfadra);
	os_std(&elrp->el_body.el_xja.el_xja_xfadrb, OS$gen_long, DD$xja_xfadrb);
	os_std(&elrp->el_body.el_xja.el_xja_aosts, OS$gen_long, DD$xja_aosts);
	os_std(&elrp->el_body.el_xja.el_xja_sernum, OS$gen_long, DD$xja_sernum);
	os_std(&elrp->el_body.el_xja.el_xja_errs, OS$gen_long, DD$xja_errs);
	os_std(&elrp->el_body.el_xja.el_xja_fcmd, OS$gen_long, DD$xja_fcmd);
	os_std(&elrp->el_body.el_xja.el_xja_ipintrsrc, OS$gen_long, DD$xja_ipintrsrc);
	os_std(&elrp->el_body.el_xja.el_xja_diag, OS$gen_long, DD$xja_diag);
	os_std(&elrp->el_body.el_xja.el_xja_dmafaddr, OS$gen_long, DD$xja_dmafaddr);
	os_std(&elrp->el_body.el_xja.el_xja_dmafcmd, OS$gen_long, DD$xja_dmafcmd);
	os_std(&elrp->el_body.el_xja.el_xja_errintr, OS$gen_long, DD$xja_errintr);
	os_std(&elrp->el_body.el_xja.el_xja_cnf, OS$gen_long, DD$xja_cnf);
	os_std(&elrp->el_body.el_xja.el_xja_xbiida, OS$gen_long, DD$xja_xbiida);
	os_std(&elrp->el_body.el_xja.el_xja_xbiidb, OS$gen_long, DD$xja_xbiidb);
	os_std(&elrp->el_body.el_xja.el_xja_errscb, OS$gen_long, DD$xja_errscb);
      break;

      case ELADP_VBA:
	cds_ptr->subtype = DD$MVIB_CDS;	
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->elsubid.subid_errcode,
		OS$gen_long,		DD$vme_errcode);
        os_std(&elrp->el_body.el_vba.elvba_reg.elmvib.mvib_viacsr,
		OS$gen_long,		DD$viacsr);
        os_std(&elrp->el_body.el_vba.elvba_reg.elmvib.mvib_csr,
		OS$gen_long,		DD$mvib_csr);
        os_std(&elrp->el_body.el_vba.elvba_reg.elmvib.mvib_vfadr,
		OS$gen_long,		DD$vfadr1);
        temp_long = ((elrp->el_body.el_vba.elvba_reg.elmvib.mvib_vfadr >>2)<<2);
        os_std(&temp_long,
		OS$gen_long,		DD$vfadr);  
        os_std(&elrp->el_body.el_vba.elvba_reg.elmvib.mvib_cfadr,
		OS$gen_long,		DD$cfadr);
        os_std(&elrp->el_body.el_vba.elvba_reg.elmvib.mvib_ivs,
		OS$gen_long,		DD$ivs);
        os_std(&elrp->el_body.el_vba.elvba_reg.elmvib.mvib_besr,
		OS$gen_long,		DD$mvib_besr);
        os_std(&elrp->el_body.el_vba.elvba_reg.elmvib.mvib_errgi,
		OS$gen_long,		DD$errgi);
        os_std(&elrp->el_body.el_vba.elvba_reg.elmvib.mvib_lvb,
		OS$gen_long,		DD$lvb);
        os_std(&elrp->el_body.el_vba.elvba_reg.elmvib.mvib_err,
		OS$gen_long,		DD$mvib_err);
      default:
        unk_rec = TRUE;
      break;
      }
  break;


/*************** 106 ELCT_BUS  (Bus Errors) ***************/

  case ELCT_BUS:
    os_std(&elrp->elsubid.subid_class,
		OS$buserr,		DD$devclass);
    os_std(&elrp->elsubid.subid_type,
		OS$bustyp,		DD$devtype);
    os_std(&elrp->elsubid.subid_num,
		OS$gen_short,		DD$unitnumber);

    switch(elrp->elsubid.subid_type)
      {
      case ELBUS_SBI780:
	cds_ptr->subtype = DD$SBI_CDS;
	ads_ptr->subtype = DD$SBI78_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->elsubid.subid_errcode,
		OS$sbiflt,		DD$coarsesyndrome);
        os_std(&elrp->el_body.elsbiaw780.sbiaw_er,
		OS$gen_long,		DD$awer);
        os_std(&elrp->el_body.elsbiaw780.sbiaw_toa,
		OS$gen_long,		DD$awtoa);
        os_std(&elrp->el_body.elsbiaw780.sbiaw_fs,
		OS$gen_long,		DD$awfs);
        os_std(&elrp->el_body.elsbiaw780.sbiaw_sc,
		OS$gen_long,		DD$awsc);
        os_std(&elrp->el_body.elsbiaw780.sbiaw_mt,
		OS$gen_long,		DD$awmt);
        os_std(&elrp->el_body.elsbiaw780.sbiaw_silo[0],
		OS$sbiawsilo,		DD$sbiawsilo);
        os_std(&elrp->el_body.elsbiaw780.sbiaw_csr[0],
		OS$sbiawcsr,		DD$sbiawcsr);
        os_std(&elrp->el_body.elsbiaw780.sbiaw_pc,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.elsbiaw780.sbiaw_psl,
		OS$gen_long,		DD$psl);
      break;
      case ELBUS_SBI8600:
	cds_ptr->subtype = DD$SBIA8600_CDS;
	ads_ptr->subtype = DD$SBI86_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->elsubid.subid_errcode,
		OS$sbiflt,		DD$coarsesyndrome);
        os_std(&elrp->el_body.elsbia8600.sbia_ioaba,
		OS$gen_long,		DD$ioaba);
        os_std(&elrp->el_body.elsbia8600.sbia_dmacid,
		OS$gen_long,		DD$dmacid);
        os_std(&elrp->el_body.elsbia8600.sbia_dmacca,
		OS$gen_long,		DD$dmacca);
        os_std(&elrp->el_body.elsbia8600.sbia_dmabid,
		OS$gen_long,		DD$dmabid);
        os_std(&elrp->el_body.elsbia8600.sbia_dmabca,
		OS$gen_long,		DD$dmabca);
        os_std(&elrp->el_body.elsbia8600.sbia_dmaaid,
		OS$gen_long,		DD$dmaaid);
        os_std(&elrp->el_body.elsbia8600.sbia_dmaaca,
		OS$gen_long,		DD$dmaaca);
        os_std(&elrp->el_body.elsbia8600.sbia_dmaiid,
		OS$gen_long,		DD$dmaiid);
        os_std(&elrp->el_body.elsbia8600.sbia_dmaica,
		OS$gen_long,		DD$dmaica);
        os_std(&elrp->el_body.elsbia8600.sbia_ioadc,
		OS$gen_long,		DD$ioadc);
        os_std(&elrp->el_body.elsbia8600.sbia_ioaes,
		OS$gen_long,		DD$ioaes);
        os_std(&elrp->el_body.elsbia8600.sbia_ioacs,
		OS$gen_long,		DD$ioacs);
        os_std(&elrp->el_body.elsbia8600.sbia_ioacf,
		OS$gen_long,		DD$ioacf);
        os_std(&elrp->el_body.elsbia8600.sbia_er,
		OS$gen_long,		DD$sbiaer);
        os_std(&elrp->el_body.elsbia8600.sbia_to,
		OS$gen_long,		DD$sbiato);
        os_std(&elrp->el_body.elsbia8600.sbia_fs,
		OS$gen_long,		DD$sbiafs);
        os_std(&elrp->el_body.elsbia8600.sbia_sc,
		OS$gen_long,		DD$sbiasc);
        os_std(&elrp->el_body.elsbia8600.sbia_mr,
		OS$gen_long,		DD$sbiamr);
        os_std(&elrp->el_body.elsbia8600.sbia_silo[0],
		OS$sbiawsilo,		DD$sbiawsilo);
        os_std(&elrp->el_body.elsbia8600.sbia_csr[0],
		OS$sbiawcsr,		DD$sbiawcsr);
        os_std(&elrp->el_body.elsbia8600.sbia_pc,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.elsbia8600.sbia_psl,
		OS$gen_long,		DD$psl);
      break;
      case ELBUS_BIER:
	cds_ptr->subtype = DD$BI_BUS_ERR_CDS;
	ads_ptr->subtype = DD$BI_BUS_ERR_ADS;
	ads_num = elrp->el_body.elbier.bier_nument;
	ads_next = 0;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->elsubid.subid_type,
		OS$bussyn,		DD$coarsesyndrome);
        os_std(&elrp->el_body.elbier.biregs[ads_next].bi_typ,
		OS$gen_long,		DD$bidevreg);
        os_std(&elrp->el_body.elbier.biregs[ads_next].bi_ctrl,
		OS$gen_long,		DD$bicsr);
        os_std(&elrp->el_body.elbier.biregs[ads_next].bi_err,
		OS$gen_long,		DD$bibuserreg);
        os_std(&elrp->el_body.elbier.biregs[ads_next].bi_err_int,
		OS$gen_long,		DD$bierint);
        os_std(&elrp->el_body.elbier.biregs[ads_next].bi_int_dst,
		OS$gen_long,		DD$biintdst);
        os_std(&elrp->el_body.elbier.biregs[ads_num].bi_typ,
		OS$gen_long,		DD$pc);
        os_std(&elrp->el_body.elbier.biregs[ads_num].bi_ctrl,
		OS$gen_long,		DD$psl);
      break;
      case ELBUS_NMIFLT:
	cds_ptr->subtype = DD$NMIFLT_CDS;
	sds_ptr->subtype = DD$NMIFLT_SDS;
	ads_ptr->subtype = DD$NMIFLT_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(sds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->elsubid.subid_type,
		OS$bussyn,		DD$coarsesyndrome);
        os_std(&elrp->el_body.elnmiflt.nmiflt_nmifsr,
		OS$gen_long,		DD$m88nmf);
        os_std(&elrp->el_body.elnmiflt.nmiflt_nmiear,
		OS$gen_long,		DD$m88nme);
        os_std(&elrp->el_body.elnmiflt.nmiflt_memcsr0,
		OS$gen_long,		DD$m88csr0);
        os_std(&elrp->el_body.elnmiflt.nmiflt_nbia0,
		OS$gen_long,		DD$nbiacsr0);
        os_std(&elrp->el_body.elnmiflt.nmiflt_nbia1,
		OS$gen_long,		DD$nbia1csr0);
        os_std(&elrp->el_body.elnmiflt.nmiflt_nmisilo[0],
		OS$nmiflt_silo,		DD$nmifltsilo);
      break;
      default:
        unk_rec = TRUE;
      break;
      }
  break;

/*************** 107 ELCT_SINT   (Stray Interupt Errors) ************/

  case ELCT_SINT:
    os_std(&elrp->elsubid.subid_type,
		OS$sisyn,		DD$coarsesyndrome);
    ads_ptr->subtype = DD$STRAY_ADS;
    if (ini_seg(ads_ptr) == EI$FAIL)
	status = EI$FAIL;
    os_std(&elrp->el_body.elstrayintr.stray_ipl,
		OS$gen_tiny,		DD$stripl);
    os_std(&elrp->el_body.elstrayintr.stray_vec,
		OS$gen_short,		DD$strvec);
  break;


/*************** 108 ELCT_AWE  (Async Write Errors) ************/

  case ELCT_AWE:
    os_std(&elrp->elsubid.subid_class,
		OS$buserr,		DD$devclass);
    os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$bustyp,		DD$devtype);
    cds_ptr->subtype = DD$SBI_CDS;
    ads_ptr->subtype = DD$SBI78_ADS;
    if (ini_seg(cds_ptr) == EI$FAIL)
	status = EI$FAIL;
    if (ini_seg(ads_ptr) == EI$FAIL)
	status = EI$FAIL;
    os_std(&elrp->el_body.elsbiaw780.sbiaw_er,
		OS$gen_long,		DD$awer);
    os_std(&elrp->el_body.elsbiaw780.sbiaw_toa,
		OS$gen_long,		DD$awtoa);
    os_std(&elrp->el_body.elsbiaw780.sbiaw_fs,
		OS$gen_long,		DD$awfs);
    os_std(&elrp->el_body.elsbiaw780.sbiaw_sc,
		OS$gen_long,		DD$awsc);
    os_std(&elrp->el_body.elsbiaw780.sbiaw_mt,
		OS$gen_long,		DD$awmt);
    os_std(elrp->el_body.elsbiaw780.sbiaw_silo,
		OS$sbiawsilo,		DD$sbiawsilo);
    os_std(elrp->el_body.elsbiaw780.sbiaw_csr,
		OS$sbiawcsr,		DD$sbiawcsr);
    os_std(&elrp->el_body.elsbiaw780.sbiaw_pc,
		OS$gen_long,		DD$pc);
    os_std(&elrp->el_body.elsbiaw780.sbiaw_psl,
		OS$gen_long,		DD$psl);
  break;

/*************** 109 ELCT_EXPTFLT  (Exception Fault Errors) *****/

  case ELCT_EXPTFLT:
    os_std(&elrp->elsubid.subid_type,
		OS$expflt,		DD$coarsesyndrome);
    ads_ptr->subtype = DD$EXPTFLT_ADS;
    if (ini_seg(ads_ptr) == EI$FAIL)
	status = EI$FAIL;
    os_std(&elrp->el_body.elexptflt.exptflt_va,
		OS$gen_long,		DD$fltva);
    os_std(&elrp->el_body.elexptflt.exptflt_pc,
		OS$gen_long,		DD$pc);
    os_std(&elrp->el_body.elexptflt.exptflt_psl,
		OS$gen_long,		DD$psl);
  break;



/*************** 110 ELCT_NMIEMM  (8800 emm Exception ) *********/
/*************** 111 ELCT_CTE     (Console Timeout Entry ) ******/

#define SID_AQ 0xe
  case ELCT_NMIEMM:
			/* vax9000 does other than merely stack dump on this */
	if((elrp->elrhdr.rhdr_sid>>24) == SID_AQ) {
		status = v9000_bld_emm(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp);
		break;
	}
  case ELCT_CTE:
			/* vax9000 does other than merely stack dump on this */
	if((elrp->elrhdr.rhdr_sid>>24) == SID_AQ) {
		status = v9000_bld_cte(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp);
		break;
	}
/*************** 112 ELCT_STKDMP  (Stack Dump Entry ) ***********/

  case ELCT_STKDMP:
    cds_ptr->subtype = DD$STKDMP_CDS;
    if (ini_seg(cds_ptr) == EI$FAIL)
	status = EI$FAIL;
    os_std(&elrp->el_body.elstkdmp.addr,
		OS$gen_acv,		DD$stkdmp);
  break;

/*************** 113 ELCT_ESR650  (KA650 Error & Status Regs) ***/
/*************** 116 ELCT_ESR420  (KA420 Error & Status Regs) ***/

  case ELCT_ESR650:
  case ELCT_ESR420:
    cds_ptr->subtype = DD$650ESR_CDS;
    if (ini_seg(cds_ptr) == EI$FAIL)
	status = EI$FAIL;
    os_std(&elrp->el_body.elesr650.esr_cacr,
		OS$gen_long,		DD$esr650_cacr);
    os_std(&elrp->el_body.elesr650.esr_cadr,
		OS$gen_long,		DD$esr650_cadr);
    os_std(&elrp->el_body.elesr650.esr_mser,
		OS$gen_long,		DD$esr650_mser);
    if(elrp->elsubid.subid_class == ELCT_ESR420)
	break;
    os_std(&elrp->el_body.elesr650.esr_dser,
		OS$gen_long,		DD$esr650_dser);
    if ((elrp->el_body.elesr650.esr_dser & 0x000000A0) != 0) /* bit 5,7 */
        os_std(&elrp->el_body.elesr650.esr_qbear,
		OS$gen_long,		DD$esr650_qbear);
    if ((elrp->el_body.elesr650.esr_dser & 0x00000011) != 0) /* bit 4,0 */
        os_std(&elrp->el_body.elesr650.esr_dear,
		OS$gen_long,		DD$esr650_dear);
    os_std(&elrp->el_body.elesr650.esr_cbtcr,
		OS$gen_long,		DD$esr650_cbtcr);
    os_std(&elrp->el_body.elesr650.esr_ipcr0,
		OS$gen_short,		DD$esr650_ipcr0);
  break;


/********************* 114 ELCT_6200_INT60 **********************/

  case ELCT_6200_INT60:
    cds_ptr->subtype = DD$XBI_XMA_CDS;
    sds_ptr->subtype = DD$XCP_6200_SDS;
    ads_ptr->subtype = DD$XCP_6200_ADS;
    if (ini_seg(cds_ptr) == EI$FAIL)
	status = EI$FAIL;
    if (ini_seg(sds_ptr) == EI$FAIL)
	status = EI$FAIL;
    if (ini_seg(ads_ptr) == EI$FAIL)
	status = EI$FAIL;
    os_std(&elrp->el_body.el_xcp60.xcp_dtype,
		OS$gen_long,		DD$xdev);
    os_std(&elrp->el_body.el_xcp60.xcp_xbe,
		OS$gen_long,		DD$xbe);
    os_std(&elrp->el_body.el_xcp60.xcp_csr1,
		OS$gen_long,		DD$csr1);
    os_std(&elrp->el_body.el_xcp60.xcp_csr2,
		OS$gen_long,		DD$csr2);
    os_std(&elrp->el_body.el_xcp60.xcp_mser,
		OS$gen_long,		DD$esr650_mser);
    os_std(&elrp->el_body.el_xcp60.xcp_cadr,
		OS$gen_long,		DD$esr650_cadr);
    os_std(&elrp->el_body.el_xcp60.xcp_fadr,
		OS$gen_long,		DD$xfadr);
    os_std(&elrp->el_body.el_xcp60.xcp_gpr,
		OS$gen_long,		DD$xgpr);
  break;

/********************* 115 ELCT_6200_INT54 **********************/

  case ELCT_6200_INT54:
    cds_ptr->subtype = DD$XBI_XMA_CDS;
    sds_ptr->subtype = DD$XCP_6200_SDS;
    ads_ptr->subtype = DD$XCP_6200_ADS;
    if (ini_seg(cds_ptr) == EI$FAIL)
	status = EI$FAIL;
    if (ini_seg(sds_ptr) == EI$FAIL)
	status = EI$FAIL;
    if (ini_seg(ads_ptr) == EI$FAIL)
	status = EI$FAIL;
    os_std(&elrp->el_body.el_xcp54.xcp_dtype,
		OS$gen_long,		DD$xdev);
    os_std(&elrp->el_body.el_xcp54.xcp_xbe,
		OS$gen_long,		DD$xbe);
    os_std(&elrp->el_body.el_xcp54.xcp_csr1,
		OS$gen_long,		DD$csr1);
    os_std(&elrp->el_body.el_xcp54.xcp_csr2,
		OS$gen_long,		DD$csr2);
    os_std(&elrp->el_body.el_xcp54.xcp_mser,
		OS$gen_long,		DD$esr650_mser);
    os_std(&elrp->el_body.el_xcp54.xcp_cadr,
		OS$gen_long,		DD$esr650_cadr);
    os_std(&elrp->el_body.el_xcp54.xcp_fadr,
		OS$gen_long,		DD$xfadr);
    os_std(&elrp->el_body.el_xcp54.xcp_gpr,
		OS$gen_long,		DD$xgpr);
    os_std(&elrp->el_body.el_xcp54.xcp_soft.xcp_iqo,
		OS$gen_long,		DD$xiqo);
    os_std(&elrp->el_body.el_xcp54.xcp_soft.xcp_dtpe,
		OS$gen_long,		DD$bidr);
    os_std(&elrp->el_body.el_xcp54.xcp_soft.xcp_cfe,
		OS$gen_long,		DD$xcfe);
    os_std(&elrp->el_body.el_xcp54.xcp_soft.xcp_cc,
		OS$gen_long,		DD$xcc);
    os_std(&elrp->el_body.el_xcp54.xcp_soft.xcp_ipe,
		OS$gen_long,		DD$xipe);
    os_std(&elrp->el_body.el_xcp54.xcp_soft.xcp_pe,
		OS$gen_long,		DD$xpe);
    os_std(&elrp->el_body.el_xcp54.xcp_soft.xcp_vbpe,
		OS$gen_long,		DD$xvbpe);
    os_std(&elrp->el_body.el_xcp54.xcp_soft.xcp_tpe,
		OS$gen_long,		DD$xtpe);
  break;

/*************** 117 ELCT_ESRPMAX  (PMAX Error & Status Regs) ***/

  case ELCT_ESRPMAX:
    cds_ptr->subtype = DD$PMAX_ESR_CDS;
    if (ini_seg(cds_ptr) == EI$FAIL)
        status = EI$FAIL;
    os_std(&elrp->el_body.elesrpmax.esr_cause,
                OS$gen_long,            DD$pmax_caus);
    os_std(&elrp->el_body.elesrpmax.esr_epc,
                OS$gen_long,            DD$pmax_epc);
    os_std(&elrp->el_body.elesrpmax.esr_badva,
                OS$gen_long,            DD$pmax_vaddr);
    os_std(&elrp->el_body.elesrpmax.esr_sp,
                OS$gen_long,            DD$sp);
    if ((elrp->el_body.elesrpmax.esr_cause & 0x0000002c) == 0x0000002c)
        {
        temp_long = ((elrp->el_body.elesrpmax.esr_cause << 2) >> 30);
        os_std(&temp_long,
                OS$gen_long,            DD$pmax_copro);
        os_std(&elrp->el_body.elesrpmax.esr_status,
                OS$gen_long,            DD$pmax_stat1);
        }
    else
        {
        os_std(&elrp->el_body.elesrpmax.esr_status,
                OS$gen_long,            DD$pmax_stat0);
        }
  break;

/********************* 118 ELCT_6400_INT60 **********************/
/********************* 119 ELCT_6400_INT54 **********************/


  case ELCT_6400_INT54:
  case ELCT_6400_INT60:
    cds_ptr->subtype = DD$XRP_CDS;
    ads_ptr->subtype = DD$XRP_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
    os_std(&elrp->el_body.el_xrp.s_rcsr,
		OS$gen_long,		DD$rcsr);
    if (((elrp->el_body.el_xrp.s_xber & 0x00100000) == 0x01) ||
        ((elrp->el_body.el_xrp.s_xber & 0x00040000) == 0x01) ||
        ((elrp->el_body.el_xrp.s_xber & 0x00020000) == 0x01) ||
        ((elrp->el_body.el_xrp.s_xber & 0x00010000) == 0x01) ||
        ((elrp->el_body.el_xrp.s_xber & 0x00008000) == 0x01) ||
        ((elrp->el_body.el_xrp.s_xber & 0x00004000) == 0x01) ||
        ((elrp->el_body.el_xrp.s_xber & 0x00002000) == 0x01))
      os_std(&elrp->el_body.el_xrp.s_xber,
		OS$gen_long,		DD$xber);
    if ((elrp->el_body.el_xrp.s_xber & 0x0000000f) == 0x09)
      os_std(&elrp->el_body.el_xrp.s_xfadr,
		OS$gen_long,		DD$xfadrid);
    else if ((elrp->el_body.el_xrp.s_xber & 0x0000000f) == 0x0f)
      os_std(&elrp->el_body.el_xrp.s_xfadr,
		OS$gen_long,		DD$xfadriv);
    else
      os_std(&elrp->el_body.el_xrp.s_xfadr,
		OS$gen_long,		DD$xfadr);
    os_std(&elrp->el_body.el_xrp.s_sscbtr,
		OS$gen_long,		DD$sscbtr);
    os_std(&elrp->el_body.el_xrp.s_bcctl,
		OS$gen_long,		DD$bcctl);
    if ((elrp->el_body.el_xrp.s_bcsts & 0x02000000) == 0x01)
      os_std(&elrp->el_body.el_xrp.s_bcsts,
		OS$gen_long,		DD$bcsts1);
    else
      {
      if (((elrp->el_body.el_xrp.s_bcsts & 0x01E00000) == 0x02) ||
          ((elrp->el_body.el_xrp.s_bcsts & 0x01E00000) == 0x0A))
        os_std(&elrp->el_body.el_xrp.s_bcsts,
		OS$gen_long,		DD$bcsts0a);
      else
        os_std(&elrp->el_body.el_xrp.s_bcsts,
		OS$gen_long,		DD$bcsts0b);
      }
    if ((elrp->el_body.el_xrp.s_bcsts & 0x00000001) == 0x01)
      os_std(&elrp->el_body.el_xrp.s_bcerr,
		OS$gen_long,		DD$bcerr);
    os_std(&elrp->el_body.el_xrp.s_pcsts,
		OS$gen_long,		DD$pcsts);
    if ((elrp->el_body.el_xrp.s_pcsts & 0x00000080) == 0x01)
      os_std(&elrp->el_body.el_xrp.s_pcerr,
		OS$gen_long,		DD$pcerr1);
    else
      os_std(&elrp->el_body.el_xrp.s_pcerr,
		OS$gen_long,		DD$pcerr0);
    os_std(&elrp->el_body.el_xrp.s_vintsr,
		OS$gen_long,		DD$vintsr);
  break;

/********************* 120 ELCT_MBUS ****************************/

  case ELCT_MBUS:
    cds_ptr->subtype = DD$MBUS_CDS;
    ads_ptr->subtype = DD$FF_ADS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
    os_std(&elrp->el_body.elmbus.elmb_dominant,
		OS$gen_tiny,		DD$ff_dom_err);
    os_std(&elrp->el_body.elmbus.elmb_flags,
		OS$gen_short,		DD$ff_flags);
    os_std(&elrp->el_body.elmbus.elmb_mod_err,
		OS$gen_long,		DD$ff_mod_err);
    ads_num  = elrp->el_body.elmbus.elmb_count,
    ads_next = 0;
  break;

/*************** 130 ELCT_ESR (Generic ESR )  *******************/

#ifdef ELCT_ESR

  case ELCT_ESR:
    switch (elrp->elsubid.subid_type)
      {
      case ELESR_5400:
        cds_ptr->subtype = DD$ESR5400_CDS;
        if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elesr.elesr.el_esr5400.esr_cause,
		OS$gen_long,		DD$pmax_caus);
        os_std(&elrp->el_body.elesr.elesr.el_esr5400.esr_epc,
		OS$gen_long,		DD$pmax_epc);
        if ((elrp->el_body.elesr.elesr.el_esr5400.esr_cause & 0x0000002c) ==
                                                              0x0000002c)
            {
            temp_long = ((elrp->el_body.elesr.elesr.el_esr5400.esr_cause << 2)
                                                                         >> 30);
            os_std(&temp_long,
                OS$gen_long,            DD$pmax_copro);
            os_std(&elrp->el_body.elesr.elesr.el_esr5400.esr_status,
                OS$gen_long,            DD$pmax_stat1);
            }
        else
            {
            os_std(&elrp->el_body.elesr.elesr.el_esr5400.esr_status,
                OS$gen_long,            DD$pmax_stat0);
            }
        os_std(&elrp->el_body.elesr.elesr.el_esr5400.esr_badva,
		OS$gen_long,		DD$badva);
        os_std(&elrp->el_body.elesr.elesr.el_esr5400.esr_sp,
		OS$gen_long,		DD$sp);
        os_std(&elrp->el_body.elesr.elesr.el_esr5400.esr_wear,
		OS$gen_long,		DD$wear);
        os_std(&elrp->el_body.elesr.elesr.el_esr5400.esr_dser,
		OS$gen_long,		DD$esr650_dser);
        if ((elrp->el_body.elesr650.esr_dser & 0x000000A0) != 0) /* bit 5,7 */
            os_std(&elrp->el_body.elesr.elesr.el_esr5400.esr_qbear,
		OS$gen_long,		DD$esr650_qbear);
        if ((elrp->el_body.elesr650.esr_dser & 0x00000011) != 0) /* bit 4,0 */
            os_std(&elrp->el_body.elesr.elesr.el_esr5400.esr_dear,
		OS$gen_long,		DD$esr650_dear);
        os_std(&elrp->el_body.elesr.elesr.el_esr5400.esr_cbtcr,
		OS$gen_long,		DD$esr650_cbtcr);
        os_std(&elrp->el_body.elesr.elesr.el_esr5400.esr_isr,
		OS$gen_long,		DD$isr);
      break;
      
      case ELESR_5500:
        cds_ptr->subtype = DD$MIPSFAIR_CDS;
        if (ini_seg(cds_ptr) == EI$FAIL)
            status = EI$FAIL;
        os_std(&elrp->el_body.elesr.elesr.el_esr5500.esr_cause,
                OS$gen_long,            DD$pmax_caus);
        os_std(&elrp->el_body.elesr.elesr.el_esr5500.esr_epc,
                OS$gen_long,            DD$pmax_epc);

        if ((elrp->el_body.elesr.elesr.el_esr5500.esr_cause & 0x0000002c) ==
                                                              0x0000002c)
            {
            temp_long = ((elrp->el_body.elesr.elesr.el_esr5500.esr_cause << 2)
                                                                         >> 30);
            os_std(&temp_long,
                OS$gen_long,            DD$pmax_copro);
            os_std(&elrp->el_body.elesr.elesr.el_esr5500.esr_status,
                OS$gen_long,            DD$pmax_stat1);
            }
        else
            {
            os_std(&elrp->el_body.elesr.elesr.el_esr5500.esr_status,
                OS$gen_long,            DD$pmax_stat0);
            }
        os_std(&elrp->el_body.elesr.elesr.el_esr5500.esr_badva,
		OS$gen_long,		DD$badva);
	os_std(&elrp->el_body.elesr.elesr.el_esr5500.esr_sp,
		OS$gen_long,		DD$sp);
	temp_long1 =  (elrp->el_body.elesr.elesr.el_esr5500.esr_mear)>>28;
	temp_long2 =  (elrp->el_body.elesr.elesr.el_esr5500.esr_mear)>>29;
	if (!(((temp_long1 ) || (temp_long2)) && (temp_long1 != temp_long2))) 
        	os_std(&elrp->el_body.elesr.elesr.el_esr5500.esr_dser,
		OS$gen_long,		DD$esr650_dser);
    	if ((elrp->el_body.elesr.elesr.el_esr5500.esr_dser & 0x000000A0) != 0) /* bit 5,7 */
        	os_std(&elrp->el_body.elesr.elesr.el_esr5500.esr_qbear,
			OS$gen_long,		DD$esr650_qbear);
    	if ((elrp->el_body.elesr.elesr.el_esr5500.esr_dser & 0x00000011) != 0) /* bit 4,0 */
        	os_std(&elrp->el_body.elesr.elesr.el_esr5500.esr_dear,
			OS$gen_long,		DD$esr650_dear);
    	os_std(&elrp->el_body.elesr.elesr.el_esr5500.esr_cbtcr,
		OS$gen_long,		DD$esr650_cbtcr);
        os_std(&elrp->el_body.elesr.elesr.el_esr5500.esr_isr,
		OS$gen_long,		DD$mf2_isr);
	os_std(&elrp->el_body.elesr.elesr.el_esr5500.esr_mser,
		OS$gen_long,		DD$mf2_mesr);
	os_std(&elrp->el_body.elesr.elesr.el_esr5500.esr_mear,
		OS$gen_long,		DD$mear);
	os_std(&elrp->el_body.elesr.elesr.el_esr5500.esr_ipcr,
		OS$gen_long,		DD$ipcr);
      break;
      
      case ELESR_5800:
        cds_ptr->subtype = DD$ESR5800_CDS;
        if (ini_seg(cds_ptr) == EI$FAIL)
            status = EI$FAIL;
        os_std(&elrp->el_body.elesr.elesr.el_esr5800.esr_cause,
                OS$gen_long,            DD$pmax_caus);
        os_std(&elrp->el_body.elesr.elesr.el_esr5800.esr_epc,
                OS$gen_long,            DD$pmax_epc);

        if ((elrp->el_body.elesr.elesr.el_esr5400.esr_cause & 0x0000002c) ==
                                                              0x0000002c)
            {
            temp_long = ((elrp->el_body.elesr.elesr.el_esr5800.esr_cause << 2)
                                                                         >> 30);
            os_std(&temp_long,
                OS$gen_long,            DD$pmax_copro);
            os_std(&elrp->el_body.elesr.elesr.el_esr5800.esr_status,
                OS$gen_long,            DD$pmax_stat1);
            }
        else
            {
            os_std(&elrp->el_body.elesr.elesr.el_esr5800.esr_status,
                OS$gen_long,            DD$pmax_stat0);
            }
        os_std(&elrp->el_body.elesr.elesr.el_esr5800.esr_badva,
		OS$gen_long,		DD$badva);
	os_std(&elrp->el_body.elesr.elesr.el_esr5800.esr_sp,
		OS$gen_long,		DD$sp);
	os_std(&elrp->el_body.elesr.elesr.el_esr5800.x3p_csr1,
		OS$gen_long,		DD$isis_csr1);
	os_std(&elrp->el_body.elesr.elesr.el_esr5800.x3p_dtype,
		OS$gen_long,		DD$xdev);
	os_std(&elrp->el_body.elesr.elesr.el_esr5800.x3p_xbe,
		OS$gen_long,		DD$xbe);
	os_std(&elrp->el_body.elesr.elesr.el_esr5800.x3p_fadr,
		OS$gen_long,		DD$xfadr);
	os_std(&elrp->el_body.elesr.elesr.el_esr5800.x3p_gpr,
		OS$gen_long,		DD$xgpr);
	os_std(&elrp->el_body.elesr.elesr.el_esr5800.x3p_csr2,
		OS$gen_long,		DD$csr2);
      break;

      case ELESR_kn02:
        cds_ptr->subtype = DD$MAX_ESR_CDS;
        if (ini_seg(cds_ptr) == EI$FAIL)
            status = EI$FAIL;
        os_std(&elrp->el_body.elesr.elesr.el_esrkn02.esr_cause,
                OS$gen_long,            DD$pmax_caus);
        if ((elrp->el_body.elesr.elesr.el_esrkn02.esr_cause & 0x0000002c) == 0x0000002c)
            {
            temp_long = ((elrp->el_body.elesr.elesr.el_esrkn02.esr_cause << 2) >> 30);
            os_std(&temp_long,
                OS$gen_long,            DD$pmax_copro);
            os_std(&elrp->el_body.elesr.elesr.el_esrkn02.esr_status,
                OS$gen_long,            DD$pmax_stat1);
            }
        else
            {
            os_std(&elrp->el_body.elesr.elesr.el_esrkn02.esr_status,
                OS$gen_long,            DD$pmax_stat0);
            }
	os_std(&elrp->el_body.elesr.elesr.el_esrkn02.esr_sp,
		OS$gen_long,		DD$sp);
	os_std(&elrp->el_body.elesr.elesr.el_esrkn02.esr_csr,
		OS$gen_long,		DD$kn02csr);
        tmp_long  = elrp->el_body.elesr.elesr.el_esrkn02.esr_erradr;
        tmp_long &= 0x80000000;
	if (tmp_long == 0x80000000)
	    {
            tmp1_long  = elrp->el_body.elesr.elesr.el_esrkn02.esr_erradr;
	    tmp1_long &= 0x07ffffff;
	    tmp1_long = (tmp1_long << 2);
            os_std(&tmp1_long,
                    OS$gen_long,            DD$erradr);
            os_std(&elrp->el_body.elesr.elesr.el_esrkn02.esr_erradr,
                    OS$gen_long,            DD$error_adr);
	    }
        temp_long  = elrp->el_body.elesr.elesr.el_esrkn02.esr_erradr;
        temp_long &= 0x70000000;
        if ((!((temp_long == 0x00000000) ||
             (temp_long == 0x20000000) ||
             (temp_long == 0x60000000))) || (tmp_long != 0x80000000))
	    {
            os_std(&elrp->el_body.elesr.elesr.el_esrkn02.esr_epc,
                OS$gen_long,            DD$pmax_epc);
            os_std(&elrp->el_body.elesr.elesr.el_esrkn02.esr_badva,
		OS$gen_long,		DD$badva);
            }
      break;

      case ELESR_5100:
        cds_ptr->subtype = DD$PMAX_ESR_CDS;
        if (ini_seg(cds_ptr) == EI$FAIL)
            status = EI$FAIL;
        ads_ptr->subtype = DD$5100_ADS;
        if (ini_seg(ads_ptr) == EI$FAIL)
            status = EI$FAIL;
        os_std(&elrp->el_body.elesr.elesr.el_esr5100.esr_cause,
                OS$gen_long,            DD$pmax_caus);
        os_std(&elrp->el_body.elesr.elesr.el_esr5100.esr_epc,
                OS$gen_long,            DD$pmax_epc);
        os_std(&elrp->el_body.elesr.elesr.el_esr5100.esr_badva,
                OS$gen_long,            DD$pmax_vaddr);
        os_std(&elrp->el_body.elesr.elesr.el_esr5100.esr_sp,
                OS$gen_long,            DD$sp);
        if ((elrp->el_body.elesr.elesr.el_esr5100.esr_cause & 0x0000002c) == 0x0000002c)
            {
            temp_long = ((elrp->el_body.elesr.elesr.el_esr5100.esr_cause << 2) >> 30);
            os_std(&temp_long,
                OS$gen_long,            DD$pmax_copro);
            os_std(&elrp->el_body.elesr.elesr.el_esr5100.esr_status,
                OS$gen_long,            DD$pmax_stat1);
            }
        else
            {
            os_std(&elrp->el_body.elesr.elesr.el_esr5100.esr_status,
                OS$gen_long,            DD$pmax_stat0);
            }
        os_std(&elrp->el_body.elesr.elesr.el_esr5100.esr_icsr,
                OS$gen_long,            DD$icsr);
        os_std(&elrp->el_body.elesr.elesr.el_esr5100.esr_leds,
                OS$gen_long,            DD$leds);
        os_std(&elrp->el_body.elesr.elesr.el_esr5100.esr_wear,
                OS$gen_long,            DD$5100_wear);
        os_std(&elrp->el_body.elesr.elesr.el_esr5100.esr_oid,
                OS$gen_long,            DD$oid);
      break;
      case ELESR_KN02BA:
        cds_ptr->subtype = DD$ESRKN02BA_CDS;
        if (ini_seg(cds_ptr) == EI$FAIL)
            status = EI$FAIL;
        os_std(&elrp->el_body.elesr.elesr.el_esrkn02ba.esr_cause,
                OS$gen_long,            DD$pmax_caus);
        os_std(&elrp->el_body.elesr.elesr.el_esrkn02ba.esr_epc,
                OS$gen_long,            DD$pmax_epc);

        if ((elrp->el_body.elesr.elesr.el_esrkn02ba.esr_cause & 0x0000002c) ==
                                                              0x0000002c)
            {
            temp_long = ((elrp->el_body.elesr.elesr.el_esrkn02ba.esr_cause << 2)
                                                                         >> 30);
            os_std(&temp_long,
                OS$gen_long,            DD$pmax_copro);
            os_std(&elrp->el_body.elesr.elesr.el_esrkn02ba.esr_status,
                OS$gen_long,            DD$pmax_stat1);
            }
        else
            {
            os_std(&elrp->el_body.elesr.elesr.el_esrkn02ba.esr_status,
                OS$gen_long,            DD$pmax_stat0);
            }
        os_std(&elrp->el_body.elesr.elesr.el_esrkn02ba.esr_badva,
		OS$gen_long,		DD$badva);
	os_std(&elrp->el_body.elesr.elesr.el_esrkn02ba.esr_sp,
		OS$gen_long,		DD$sp);
	os_std(&elrp->el_body.elesr.elesr.el_esrkn02ba.esr_ssr,
		OS$gen_long,		DD$ssr);
	os_std(&elrp->el_body.elesr.elesr.el_esrkn02ba.esr_sir,
		OS$gen_long,		DD$sir);
	os_std(&elrp->el_body.elesr.elesr.el_esrkn02ba.esr_sirm,
		OS$gen_long,		DD$sirm);
	break;
      }   
  break;

#endif ELCT_ESR
/*************** 131 ELCT_INT60 (Generic Interrupt 60 )  *******************/
/*************** 132 ELCT_INT54 (Generic Interrupt 54 )  *******************/

  case ELCT_INT60:
  case ELCT_INT54:
    switch (elrp->elsubid.subid_type)
      {
      case ELINT_6500:
	ads_ptr->subtype = DD$6500_INT_ADS;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;

	os_std(&elrp->el_body.el_xmp.s_xbe0,
	       OS$gen_long,		DD$xbe0);
	os_std(&elrp->el_body.el_xmp.s_xfadr0,
	       OS$gen_long,		DD$xfadr0);
	os_std(&elrp->el_body.el_xmp.s_xfaer0,
	       OS$gen_long,		DD$xfaer);
	os_std(&elrp->el_body.el_xmp.s_xbeer0,
	       OS$gen_long,		DD$xbeer);
	os_std(&elrp->el_body.el_xmp.s_wfadr0,
	       OS$gen_long,		DD$wfadr0);
	os_std(&elrp->el_body.el_xmp.s_wfadr1,
	       OS$gen_long,		DD$wfadr1);
	os_std(&elrp->el_body.el_xmp.s_fdal0,
	       OS$gen_long,		DD$fdal0);
	os_std(&elrp->el_body.el_xmp.s_fdal1,
	       OS$gen_long,		DD$fdal1);
	os_std(&elrp->el_body.el_xmp.s_fdal2,
	       OS$gen_long,		DD$fdal2);
	os_std(&elrp->el_body.el_xmp.s_fdal3,
	       OS$gen_long,		DD$fdal3);
	os_std(&elrp->el_body.el_xmp.s_sscbtr,
	       OS$gen_long,		DD$6500_sscbtr);
	os_std(&elrp->el_body.el_xmp.s_bcsts,
	       OS$gen_long,		DD$bcsts);
	os_std(&elrp->el_body.el_xmp.s_bcera,
	       OS$gen_long,		DD$bcera);
	os_std(&elrp->el_body.el_xmp.s_bcert,
	       OS$gen_long,		DD$bcert);
	os_std(&elrp->el_body.el_xmp.s_pcsts,
	       OS$gen_long,		DD$6500_pcsts);
	os_std(&elrp->el_body.el_xmp.s_pcerr,
	       OS$gen_long,		DD$pcerr);
	os_std(&elrp->el_body.el_xmp.s_vintsr,
	       OS$gen_long,		DD$6500_vintsr);
      break;

/*      default:
        unk_rec = TRUE;
      break; */
      }
  break;

/*************** 133 ELCT_9000_SYNDROME  (Aquarius syndrome)  ***************/
  case ELCT_9000_SYNDROME:
	status = v9000_bld_syndrome(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp);
	break;

/*************** 134 ELCT_9000_KAF  (Aquarius SPU KEEP ALIVE FAILURE)  *****/
  case ELCT_9000_KAF:
	status = v9000_bld_kaf(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp);
	break;

/*************** 135 ELCT_9000_CLK  (Aquarius clock system error)  *****/
  case ELCT_9000_CLK:
	status = v9000_bld_clk(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp);
	break;

/*************** 136 ELCT_9000_SCAN  (Aquarius scan message)  *****/
  case ELCT_9000_SCAN:
	status = v9000_bld_scan(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp);
	break;

/*************** 137 ELCT_9000_CONFIG  (Aquarius SPU configuration msg)  *****/
  case ELCT_9000_CONFIG:
	status = v9000_bld_config(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp);
	break;

/*************** 138 ELCT_VECTOR  *****/
  case ELCT_VECTOR:
        cds_ptr->subtype = DD$VEC6400_CDS;
        if (ini_seg(cds_ptr) == EI$FAIL)
            status = EI$FAIL;
    os_std(&elrp->el_body.el_vec6400.vec6400_vintsr,
		OS$gen_long,		DD$vec6400_vintsr);
    os_std(&elrp->el_body.el_vec6400.vec6400_vpsr,
		OS$gen_long,		DD$vec_vpsr);
    os_std(&elrp->el_body.el_vec6400.vec6400_vctl_csr,
		OS$gen_long,		DD$vec6400_vctl_csr);
    os_std(&elrp->el_body.el_vec6400.vec6400_lsx_ccsr,
		OS$gen_long,		DD$vec6400_lsx_ccsr);
	break;

/*************** 200 ELSW_PNC  (Panic Bug Check)  ***************/

  case ELSW_PNC:
    cds_ptr->subtype = DD$PANIC_CDS;
    ads_ptr->subtype = DD$PANIC_ADS;
    if (ini_seg(cds_ptr) == EI$FAIL)
	status = EI$FAIL;
    if (ini_seg(ads_ptr) == EI$FAIL)
	status = EI$FAIL;
if (((elrp->elrhdr.rhdr_sid >> 24) == 0x00) ||	/* TEMP */
    ((elrp->elrhdr.rhdr_sid >> 24) == 0x82))    /* MIPS PROCESSOR */
    {
    os_std(elrp->el_body.elpnc.pnc_asc,
		OS$pnc_asc,		DD$pncmes);
    break;
    }

    os_std(elrp->el_body.elpnc.pnc_asc,
		OS$pnc_asc,		DD$pncmes);
    os_std(&elrp->el_body.elpnc.pnc_sp,
		OS$gen_long,		DD$sp);
    os_std(&elrp->el_body.elpnc.pnc_ap,
		OS$gen_long,		DD$pncap);
    os_std(&elrp->el_body.elpnc.pnc_fp,
		OS$gen_long,		DD$pncfp);
    os_std(&elrp->el_body.elpnc.pnc_pc,
		OS$gen_long,		DD$pc);
    os_std(&elrp->el_body.elpnc.pncregs.pnc_ksp,
		OS$gen_long,		DD$pncksp);
    os_std(&elrp->el_body.elpnc.pncregs.pnc_usp,
		OS$gen_long,		DD$pncusp);
    os_std(&elrp->el_body.elpnc.pncregs.pnc_isp,
		OS$gen_long,		DD$pncisp);
    os_std(&elrp->el_body.elpnc.pncregs.pnc_p0br,
		OS$gen_long,		DD$pncp0b);
    os_std(&elrp->el_body.elpnc.pncregs.pnc_p0lr,
		OS$gen_long,		DD$pncp0l);
    os_std(&elrp->el_body.elpnc.pncregs.pnc_p1br,
		OS$gen_long,		DD$pncp1b);
    os_std(&elrp->el_body.elpnc.pncregs.pnc_p1lr,
		OS$gen_long,		DD$p1lr);
    os_std(&elrp->el_body.elpnc.pncregs.pnc_sbr,
		OS$gen_long,		DD$pncsbr);
    os_std(&elrp->el_body.elpnc.pncregs.pnc_slr,
		OS$gen_long,		DD$pncslr);
    os_std(&elrp->el_body.elpnc.pncregs.pnc_pcbb,
		OS$gen_long,		DD$pncpcb);
    os_std(&elrp->el_body.elpnc.pncregs.pnc_scbb,
		OS$gen_long,		DD$pncscb);
    os_std(&elrp->el_body.elpnc.pncregs.pnc_ipl,
		OS$gen_long,		DD$pncipl);
    os_std(&elrp->el_body.elpnc.pncregs.pnc_astlvl,
		OS$gen_long,		DD$pncast);
    os_std(&elrp->el_body.elpnc.pncregs.pnc_sisr,
		OS$gen_long,		DD$pncsis);
    os_std(&elrp->el_body.elpnc.pncregs.pnc_iccs,
		OS$gen_long,		DD$pncicc);


  break;

/*************** 201 ELSW_CIPPD  (ci ppd info errors) ************/

  case ELSW_CIPPD: 
    cds_ptr->subtype = DD$CI_GEN_CDS;
    if (ini_seg(cds_ptr) == EI$FAIL)
        status = EI$FAIL;
    sds_ptr->subtype = DD$CI_COMMON_SDS;
    if (ini_seg(sds_ptr) == EI$FAIL)
        status = EI$FAIL;
    ads_ptr->subtype = DD$CI_PPD_ADS;
    if (ini_seg(ads_ptr) == EI$FAIL)
        status = EI$FAIL;
    if ((Elcippdcommon(elrp)->cippd_optfmask1 & CIPPD_CLTDEVNUM) ==
						CIPPD_CLTDEVNUM)
        os_std(&elrp->elsubid.subid_num,
		OS$gen_short,		DD$controller);
    if ((Elcippdcommon(elrp)->cippd_optfmask1 & CIPPD_CLTDEVTYP) ==
						CIPPD_CLTDEVTYP)
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$cityp,		DD$devtype);
    os_std(&elrp->elsubid.subid_errcode,
		OS$gen_long,		DD$cierrcode);
    temp_long = (elrp->elsubid.subid_errcode & 0xe7ffffff);
    os_std(&temp_long,
		OS$cierrcod,		DD$cierrcod);
    os_std(Elcippdcommon(elrp)->cippd_lname,
		OS$ci_lname,		DD$cilname);
    os_std(Elcippdcommon(elrp)->cippd_rname,
		OS$ci_rname,		DD$cirname);
    os_std(Elcippdcommon(elrp)->cippd_lsysid,
		OS$gen_bv_6,		DD$cilsysid);
    os_std(Elcippdcommon(elrp)->cippd_rsysid,
		OS$gen_bv_6,		DD$cirsysid);
    os_std(&Elcippdcommon(elrp)->cippd_npaths,
		OS$gen_short,		DD$cinpaths);
    os_std(&Elcippdcommon(elrp)->cippd_nerrs,
		OS$gen_short,		DD$cierrs);


/**************************** PACKET *********************************/
/**************************** COMMON *********************************/

    if ((Elcippdcommon(elrp)->cippd_optfmask1 & CIPPD_PCOMMON) ==
						CIPPD_PCOMMON)
      {
      os_std(Elcippdpcommon(elrp)->cippd_lpname,
		OS$ci_lpname,		DD$cilpname);
      os_std(Elcippdpcommon(elrp)->cippd_lsaddr,
		OS$gen_bv_6,		DD$cilsaddr);
      os_std(Elcippdpcommon(elrp)->cippd_rsaddr,
		OS$gen_bv_6,		DD$cirsaddr);
      os_std(&Elcippdpcommon(elrp)->cippd_pstate,
		OS$gen_long,		DD$cipstate);
      }

/**************************** SYSAPP *********************************/

    if ((Elcippdcommon(elrp)->cippd_optfmask1 & CIPPD_SYSAP) ==
						CIPPD_SYSAP)
      {
      os_std(Elcippdsysapnam(elrp),
 	      	  OS$gen_asc_16,		DD$cisysap);
      }

/**************************** DBCOLL *********************************/

    if ((Elcippdcommon(elrp)->cippd_optfmask1 & CIPPD_DBCOLL) ==
						CIPPD_DBCOLL)
      {
      os_std(Elcippddbcoll(elrp)->cippd_rswincrn,
 	      	  OS$gen_bv_8,		DD$cirswincrn);
      os_std(Elcippddbcoll(elrp)->cippd_kswincrn,
 	      	  OS$gen_bv_8,		DD$cikswincrn);
      os_std(Elcippddbcoll(elrp)->cippd_kname,
 	      	  OS$ci_kname,		DD$cikname);
      os_std(Elcippddbcoll(elrp)->cippd_ksysid,
 	      	  OS$gen_bv_6,		DD$ciksysid);
      os_std(Elcippddbcoll(elrp)->cippd_klsaddr,
 	      	  OS$gen_bv_6,		DD$ciklsaddr);
      os_std(Elcippddbcoll(elrp)->cippd_krsaddr,
 	      	  OS$gen_bv_6,		DD$cikrsaddr);
      }

/**************************** NEW PATH *******************************/

    if ((Elcippdcommon(elrp)->cippd_optfmask1 & CIPPD_NEWPATH) ==
						CIPPD_NEWPATH)
      {
      os_std(&Elcippdnewpath(elrp)->cippd_max_dg,
		  OS$gen_short,		DD$cimaxdg);
      os_std(&Elcippdnewpath(elrp)->cippd_max_msg,
		  OS$gen_short,		DD$cimaxmsg);
      os_std(Elcippdnewpath(elrp)->cippd_swtype,
		  OS$gen_asc_4,		DD$ciswtype);
      os_std(Elcippdnewpath(elrp)->cippd_swver,
		  OS$gen_asc_4,		DD$ciswver);
      os_std(Elcippdnewpath(elrp)->cippd_swincrn,
		  OS$gen_bv_8,		DD$ciswincrn);
      os_std(Elcippdnewpath(elrp)->cippd_hwtype,
		  OS$gen_asc_4,		DD$cihwtype);
      os_std(Elcippdnewpath(elrp)->cippd_hwver,
		  OS$gen_bv_12,		DD$cihwver);
      }


/**************************** PPACKET ********************************/

    if ((Elcippdcommon(elrp)->cippd_optfmask1 & CIPPD_PPACKET) ==
						CIPPD_PPACKET)
      {
      os_std(&Elcippdppacket(elrp)->cippd_mtype,
		  OS$gen_short,		DD$cimtype);
      }

/**************************** SCOMMON ********************************/

    if ((Elcippdcommon(elrp)->cippd_optfmask1 & CIPPD_SCOMMON) ==
						CIPPD_SCOMMON)
      {
      os_std(Elcippdscommon(elrp)->cippd_rswtype,
	       	  OS$gen_asc_4,		DD$cirswtype);
      os_std(Elcippdscommon(elrp)->cippd_rswver,
	       	  OS$gen_asc_4,		DD$cirswver);
      os_std(Elcippdscommon(elrp)->cippd_rswincrn,
	       	  OS$gen_bv_8,		DD$cirswincrn);
      os_std(Elcippdscommon(elrp)->cippd_rhwtype,
	       	  OS$gen_asc_4,		DD$cirhwtype);
      os_std(Elcippdscommon(elrp)->cippd_rhwver,
	       	  OS$gen_bv_12,		DD$cirhwver);
      }

/**************************** PROTOCOL *******************************/

    if ((Elcippdcommon(elrp)->cippd_optfmask1 & CIPPD_PROTOCOL) ==
						CIPPD_PROTOCOL)
      {
      os_std(&Elcippdprotocol(elrp)->cippd_local,
	       	  OS$gen_tiny,		DD$cilocal);
      os_std(&Elcippdprotocol(elrp)->cippd_remote,
 	      	  OS$gen_tiny,		DD$ciremote);
      }

/**************************** SPACKET ********************************/

    if ((Elcippdcommon(elrp)->cippd_optfmask1 & CIPPD_SPACKET) ==
						CIPPD_SPACKET)
      {
      os_std(&Elcippdspacket(elrp)->cippd_mtype,
		  OS$gen_short,		DD$cimtype);
      os_std(&Elcippdspacket(elrp)->cippd_mtype+10,
		  OS$msg_asc,		DD$message);
      }
  break;

/*************** 202 ELSW_SCS  (scs errors) ************/

  case ELSW_SCS: 
    cds_ptr->subtype = DD$SCS_CDS;
    if (ini_seg(cds_ptr) == EI$FAIL)
        status = EI$FAIL;
    ads_ptr->subtype = DD$SCS_ADS;
    if (ini_seg(ads_ptr) == EI$FAIL)
        status = EI$FAIL;

    if ((Elscscommon(elrp)->scs_optfmask1 & SCS_CLTDEVNUM) ==
						SCS_CLTDEVNUM)
        os_std(&elrp->elsubid.subid_num,
		OS$gen_short,		DD$controller);
    if ((Elscscommon(elrp)->scs_optfmask1 & SCS_CLTDEVTYP) ==
						SCS_CLTDEVTYP)
        os_std(&elrp->elsubid.subid_ctldevtyp,
		OS$cityp,		DD$devtype);

    os_std(&elrp->elsubid.subid_errcode,
		OS$gen_long,		DD$cierrcode);
    temp_long = (elrp->elsubid.subid_errcode & 0xe7ffffff);
    os_std(&temp_long,
		OS$scs_errcode,		DD$scs_errcode);

    os_std(Elscscommon(elrp)->scs_lsysap,
		OS$gen_asc_16,		DD$scslsysap);
    os_std(Elscscommon(elrp)->scs_lconndata,
		OS$gen_asc_16,		DD$scslconndata);
    os_std(&Elscscommon(elrp)->scs_lconnid,
		OS$gen_long,		DD$scslconnid);
    os_std(Elscscommon(elrp)->scs_lname,
		OS$gen_bv_8,		DD$cilname);
    os_std(Elscscommon(elrp)->scs_lsysid,
		OS$gen_bv_6,		DD$cilsysid);
    os_std(&Elscscommon(elrp)->scs_cstate,
		OS$gen_short,		DD$scscstate);

    if (Elscscommon(elrp)->scs_optfmask1 & SCS_CONN == SCS_CONN)
	{
	os_std(Elscsopt(elrp)->scsconn.scs_rsysap,
		OS$gen_asc_16,		DD$scsrsysap);
        os_std(Elscsopt(elrp)->scsconn.scs_rconndata,
		OS$gen_asc_16,		DD$scsrconndata);
	os_std(&Elscsopt(elrp)->scsconn.scs_rconnid,
		OS$gen_long,		DD$scsrconnid);
	os_std(Elscsopt(elrp)->scsconn.scs_rname,
		OS$gen_bv_8,		DD$cirname);
	os_std(Elscsopt(elrp)->scsconn.scs_rsysid,
		OS$gen_bv_6,		DD$cirsysid);
	os_std(Elscsopt(elrp)->scsconn.scs_rsaddr,
		OS$gen_bv_6,		DD$cirsaddr);
	os_std(Elscsopt(elrp)->scsconn.scs_lpname,
		OS$gen_asc_4,		DD$cilpname);
	os_std(Elscsopt(elrp)->scsconn.scs_lsaddr,
		OS$gen_bv_6,		DD$cilsaddr);
	os_std(&Elscsopt(elrp)->scsconn.scs_nconns,
		OS$gen_short,		DD$scsnconns);
	break;
	}

    if (Elscscommon(elrp)->scs_optfmask1 & SCS_LDIRID == SCS_LDIRID)
	{
	os_std(&Elscsopt(elrp)->scs_ldirid,
		OS$gen_short,		DD$scsldirid);
	break;
	}

    if (Elscscommon(elrp)->scs_optfmask1 & SCS_RREASON == SCS_RREASON)
	os_std(&Elscsopt(elrp)->scs_rreason,
		OS$gen_long,		DD$scsrreason);
	
  break;

/*************** 300 ELMSGT_SU  (Startup Message)  ***************/
/*************** 301 ELMSGT_SD  (Shutdown Message)  **************/

  case ELMSGT_SU:
  case ELMSGT_SD:
    cds_ptr->subtype = DD$INFO_MSG_CDS;
    if (ini_seg(cds_ptr) == EI$FAIL)
	status = EI$FAIL;
    os_std(elrp->el_body.elmsg.msg_asc,
		OS$startup_msg,		DD$message);
  break;

/*************** 250 ELMSGT_INFO  (Informational Message ) *********/
/*************** 350 ELMSGT_DIAG  (Diagnostic Message ) ************/
/*************** 351 ELMSGT_REPAIR  (Repair Message )  ***************/

  case ELMSGT_INFO:
  case ELMSGT_DIAG:
  case ELMSGT_REPAIR:
    cds_ptr->subtype = DD$INFO_MSG_CDS;
    if (ini_seg(cds_ptr) == EI$FAIL)
	status = EI$FAIL;
    os_std(elrp->el_body.elmsg.msg_asc,
		OS$msg_asc,		DD$message);
  break;

/*************** 251 ELMSGT_SNAP8600  (8600 Snapshot )  ************/
/*************** 310 ELMSGT_TIM  (Time Stamp )  ********************/

  case ELMSGT_SNAP8600:
  case ELMSGT_TIM:
  break;

/****************** NONE OF THE ABOVE ***************************/

  default :
    unk_rec = TRUE;
    if (get_tree(UE$FLG_Z) == UE$NULL)
        return EI$FAIL;
  break;
  }
if (status == EI$FAIL)
    unk_rec = TRUE;
return EI$SUCC;
}
/*...	ENDROUTINE EI$BLD          */



/*
*	.SBTTL	MSCP_BLD    -  Builds the MSCP segments
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine 
*	
* CALLING SEQUENCE:		CALL MSCP_BLD   (..See Below..)
*
*					Called from EI$BLD with address
*					with a error log entry
*
* FORMAL PARAMETERS:		
*
*	xxx_ptr			Pointers to all segments
*	elrp			Pointer to errlog entry
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		STD segments filled
*
* ROUTINE VALUE:		EI$SUCC 
*				EI$FAIL
*
* SIDE EFFECTS:			
*
*--
*/
/*...	ROUTINE MSCP_BLD(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,elrp) */

int  mscp_bld(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr, elrp)

EIS  *eis_ptr;
DIS  *dis_ptr;
CDS  *cds_ptr;
SDS  *sds_ptr;
ADS  *ads_ptr;

struct el_rec *elrp;

{
long    status;
long 	id;
char    temp[4];

short   temp_short;

status = EI$SUCC;

os_std(&elrp->U_MSCP.mslg_format,
		OS$msg_fmt,		DD$coarsesyndrome);

ads_ptr->subtype = DD$DSA_GENERIC_ADS;
if (ini_seg(ads_ptr) == EI$FAIL)
    status = EI$FAIL;


switch(elrp->U_MSCP.mslg_format)
    {
    case MSLG_FM_CNT_ER:  				/* format 0 */
	switch(elrp->U_MSCP.mslg_cnt_id[6])
	  {
	  case ELMPCT_HSC50:
	  case ELMPCT_HSC70:
              os_std(elrp->U_MSCP.mslg_unit_id,
		OS$gen_bv_6,		DD$unitid1);
              os_std(&elrp->U_MSCP.mslg_unit_svr,
		OS$gen_tiny,		DD$usvrsn);
              elrp->U_MSCP.mslg_unit_hvr &= 0x7f;
              os_std(&elrp->U_MSCP.mslg_unit_hvr,
		OS$gen_tiny,		DD$ushvrsn);
              os_std(&elrp->U_MSCP.mslg_mult_unt,
		OS$gen_short,		DD$req_port);
	  break;

	  case ELMPCT_RV20:
	  break;		/* do more in ads_bld */

	  default:
              os_std(make_addr_vector(elrp, &elrp->U_MSCP.mslg_mult_unt),
                  OS$gen_acv,           DD$ctl_addl_info);
	  break;
	  };
    break;


    case MSLG_FM_BUS_ADDR: 				/* format 1 */
	cds_ptr->subtype = DD$DSK_CTL_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->U_MSCP.mslg_bus_addr,
		  OS$gen_long,		DD$hmemaddr);
	if (elrp->U_MSCP.mslg_cnt_id[6] == ELMPCT_RV20)
	    break;		/* do more in ads_bld */
        os_std(make_addr_vector(elrp, ((DD$BYTE *) 
            &elrp->U_MSCP.mslg_bus_addr)+4),
                  OS$gen_acv,          	DD$ctl_addl_info);
    break;


    case MSLG_FM_DISK_TRN:               		/* format 2 */
        os_std(&elrp->U_MSCP.mslg_hdr_code,
		OS$gen_long,		DD$hdrcod);
    break;


    case MSLG_FM_SDI:                   		/* format 3 */
	switch (elrp->elsubid.subid_ctldevtyp)
	  {
 	  case ELDT_RA60:
	      cds_ptr->subtype = DD$RA60_SDI_CDS;
	      if (ini_seg(cds_ptr) == EI$FAIL)
	            status = EI$FAIL;
              os_std(&elrp->U_MSCP.mslg_hdr_code,
		    OS$gen_long,		DD$hdrcod);
              os_std(elrp->U_MSCP.mslg_sdi+11,
		    OS$gen_tiny,		DD$fpcod);
              os_std(elrp->U_MSCP.mslg_sdi+5,
		    OS$gen_short,   		DD$ra6prvcyl);
              os_std(elrp->U_MSCP.mslg_sdi+7,
		    OS$gen_tiny,		DD$ra6prvhd);
              os_std(elrp->U_MSCP.mslg_sdi+8,
		    OS$gen_short,		DD$curcyl);
              os_std(elrp->U_MSCP.mslg_sdi+10,
		    OS$gen_tiny,		DD$curhd);
              os_std(elrp->U_MSCP.mslg_sdi+8,
		    OS$gen_short,		DD$sum_cyl);
              os_std(elrp->U_MSCP.mslg_sdi+10,
		    OS$gen_tiny,		DD$sum_trk);
	  break;
	  case ELDT_RA70:
	  case ELDT_RA72:
	  case ELDT_RA71:
	      cds_ptr->subtype = DD$RA7_SDI_CDS;
	      if (ini_seg(cds_ptr) == EI$FAIL)
	          status = EI$FAIL;
              os_std(&elrp->U_MSCP.mslg_hdr_code,
		    OS$gen_long,		DD$hdrcod);
              os_std(elrp->U_MSCP.mslg_sdi+11,
		    OS$gen_tiny,		DD$fpcod);
              os_std(elrp->U_MSCP.mslg_sdi+5,
		    OS$gen_tiny,		DD$sdilstop);
              os_std(elrp->U_MSCP.mslg_sdi+6,
		    OS$gen_tiny,		DD$sdidrvsta);
              os_std(elrp->U_MSCP.mslg_sdi+7,
		    OS$gen_short,		DD$curcyl);
              os_std(elrp->U_MSCP.mslg_sdi+9,
		    OS$gen_tiny,		DD$curhd);
              os_std(elrp->U_MSCP.mslg_sdi+10,
		    OS$gen_tiny,		DD$led);
              os_std(elrp->U_MSCP.mslg_sdi+7,
		    OS$gen_short,		DD$sum_cyl);
              os_std(elrp->U_MSCP.mslg_sdi+9,
		    OS$gen_tiny,		DD$sum_trk);
              os_std(elrp->U_MSCP.mslg_sdi+10,
		    OS$gen_tiny,		DD$sum_led);
	  break;
	  case ELDT_RA80:
	  case ELDT_RA81:
	  case ELDT_RA82:
	      cds_ptr->subtype = DD$RA8_SDI_CDS;
	      if (ini_seg(cds_ptr) == EI$FAIL)
	          status = EI$FAIL;
              os_std(&elrp->U_MSCP.mslg_hdr_code,
		    OS$gen_long,		DD$hdrcod);
              os_std(elrp->U_MSCP.mslg_sdi+11,
		    OS$gen_tiny,		DD$fpcod);
              os_std(elrp->U_MSCP.mslg_sdi+5,
		    OS$gen_tiny,		DD$sdilstop);
              os_std(elrp->U_MSCP.mslg_sdi+6,
		    OS$gen_tiny,		DD$sdidrvdet);
              os_std(elrp->U_MSCP.mslg_sdi+7,
		    OS$gen_short,		DD$curcyl);
              os_std(elrp->U_MSCP.mslg_sdi+9,
		    OS$gen_tiny,		DD$curhd);
              os_std(elrp->U_MSCP.mslg_sdi+10,
		    OS$gen_tiny,		DD$led);
              os_std(elrp->U_MSCP.mslg_sdi+7,
		    OS$gen_short,		DD$sum_cyl);
              os_std(elrp->U_MSCP.mslg_sdi+9,
		    OS$gen_tiny,		DD$sum_trk);
              os_std(elrp->U_MSCP.mslg_sdi+10,
		    OS$gen_tiny,		DD$sum_led);
	  break;
	  case ELDT_RA90:
	      cds_ptr->subtype = DD$RA9_SDI_CDS;
	      if (ini_seg(cds_ptr) == EI$FAIL)
	          status = EI$FAIL;
              os_std(&elrp->U_MSCP.mslg_hdr_code,
		    OS$gen_long,		DD$hdrcod);
              os_std(elrp->U_MSCP.mslg_sdi+11,
		    OS$gen_tiny,		DD$fpcod);
              os_std(elrp->U_MSCP.mslg_sdi+5,
		    OS$gen_tiny,		DD$sdilstop);
              os_std(elrp->U_MSCP.mslg_sdi+6,
		    OS$gen_tiny,		DD$curhd);
              os_std(elrp->U_MSCP.mslg_sdi+7,
		    OS$gen_short,		DD$curcyl);
              os_std(elrp->U_MSCP.mslg_sdi+9,
		    OS$gen_short,		DD$errecov);
              os_std(elrp->U_MSCP.mslg_sdi+10,
		    OS$gen_tiny,		DD$led);
              os_std(elrp->U_MSCP.mslg_sdi+7,
		    OS$gen_short,		DD$sum_cyl);
              os_std(elrp->U_MSCP.mslg_sdi+6,
		    OS$gen_tiny,		DD$sum_trk);
              os_std(elrp->U_MSCP.mslg_sdi+10,
		    OS$gen_tiny,		DD$sum_led);
	  break;
	  default:
              os_std(make_addr_vector(elrp, &elrp->U_MSCP.mslg_level),
                  OS$gen_acv,           DD$ctl_addl_info);
	  break;
	  }
    break;
    case MSLG_FM_SML_DSK:               		/* format 4 */
	cds_ptr->subtype = DD$SMALL_DISK_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->U_MSCP.mslg_sde_cyl,
		OS$gen_short,		DD$cylinder);
        os_std(&elrp->U_MSCP.mslg_sde_cyl,
		OS$gen_short,		DD$sum_cyl);
    break;
    case MSLG_FM_TAPE_TRN:            			/* format 5 */
	cds_ptr->subtype = DD$STI_COM_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->U_MSCP.mslg_gap_cnt,
		OS$gen_long,		DD$position);
    break;
    case MSLG_FM_STI_ERR:             			/* format 6 */
	cds_ptr->subtype = DD$STI_COM_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->U_MSCP.mslg_gap_cnt,
		OS$gen_long,		DD$position);
        os_std(elrp->U_MSCP.mslg_sti+0,
		OS$gen_long,		DD$stisumm);
        os_std(elrp->U_MSCP.mslg_sti+4,
		OS$gen_short,		DD$stidrv0);
        os_std(elrp->U_MSCP.mslg_sti+6,
		OS$gen_short,		DD$stidrv1);
        os_std(elrp->U_MSCP.mslg_sti+8,
		OS$gen_short,		DD$stidrv2);
        os_std(elrp->U_MSCP.mslg_sti+10,
		OS$gen_short,		DD$stidrv3);
    break;
    case MSLG_FM_STI_DEL:             			/* format 7 */
	cds_ptr->subtype = DD$STI_COM_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->U_MSCP.mslg_gap_cnt,
		OS$gen_long,		DD$position);
    break;
    case MSLG_FM_STI_FEL:             			/* format 8 */
	cds_ptr->subtype = DD$STI_COM_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->U_MSCP.mslg_gap_cnt,
		OS$gen_long,		DD$position);
	if (elrp->elsubid.subid_ctldevtyp == ELTT_TA81)
	    {
            os_std(elrp->U_MSCP.mslg_sti+0,
		OS$gen_tiny,		DD$sti_summ);
	    }
	else
	    {
            os_std(elrp->U_MSCP.mslg_sti+4,
		OS$gen_long,		DD$stisumm);
	    }
    break;
    case MSLG_FM_REPLACE:             			/* format 9 */
	cds_ptr->subtype = DD$BAD_BLK_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->U_MSCP.mslg_rpl_flgs,
		OS$gen_short,		DD$replflg);
        os_std(&elrp->U_MSCP.mslg_rpl_flgs,
		OS$gen_short,		DD$sum_rp);
        os_std(&elrp->U_MSCP.mslg_hdr_code,
		OS$gen_long,		DD$badlbn);
        os_std(&elrp->U_MSCP.mslg_hdr_code,
		OS$gen_long,		DD$sum_lbn);
        os_std(elrp->U_MSCP.mslg_sdi+0,
		OS$gen_long,		DD$oldrbn);
        os_std(elrp->U_MSCP.mslg_sdi+4,
		OS$gen_long,		DD$newrbn);
        os_std(elrp->U_MSCP.mslg_sdi+8,
		OS$gen_short,		DD$cause);
    break;
    case MSLG_FM_IBMSENSE:             			/* format B */
	cds_ptr->subtype = DD$STI_COM_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->U_MSCP.mslg_gap_cnt,
		OS$gen_long,		DD$position);
    break;
    default:
        unk_rec = TRUE;
    break;
    }



/************** COMMON MSCP INFORMATION ***********************/

os_std(&elrp->U_MSCP.mslg_format,
		OS$gen_tiny,		DD$dsaformat);
os_std(&elrp->U_MSCP.mslg_flags,
		OS$gen_tiny,		DD$dsaflags);
os_std(&elrp->U_MSCP.mslg_cmd_ref,
		OS$gen_long,		DD$cmdref);
os_std(&elrp->U_MSCP.mslg_seq_num,
		OS$gen_short,		DD$dsaseq);
os_std(elrp->U_MSCP.mslg_cnt_id,
		OS$gen_bv_6,		DD$cntrlidnum);
os_std(elrp->U_MSCP.mslg_cnt_id+6,
		OS$gen_short,		DD$cntrlid);
os_std(&elrp->U_MSCP.mslg_cnt_svr,
		OS$gen_tiny,		DD$csvrsn);
os_std(&elrp->U_MSCP.mslg_cnt_hvr,
		OS$gen_tiny,		DD$chvrsn);
/*
temp[0] = ((char)elrp->U_MSCP.mslg_event)+0;
temp[1] = ((char)elrp->U_MSCP.mslg_event)+1;
*((short *)temp) &= 0x001f;
*/

temp_short = elrp->U_MSCP.mslg_event
		   & 0x01f;

if (elrp->U_MSCP.mslg_format == 0x0b)
os_std(&temp_short,
		OS$gen_short,		DD$fm_b_event_code);
os_std(&temp_short,
		OS$gen_short,		DD$event_code);

switch (elrp->U_MSCP.mslg_format)
    {
    case MSLG_FM_CNT_ER:			/* format 0 */
    case MSLG_FM_BUS_ADDR:			/* format 1 */
    break;

    default:
    os_std(elrp->U_MSCP.mslg_unit_id,
		OS$gen_bv_6,		DD$unitid1);
    os_std(&elrp->U_MSCP.mslg_unit_svr,
		OS$gen_tiny,		DD$usvrsn);
    elrp->U_MSCP.mslg_unit_hvr &= 0x7f;
    os_std(&elrp->U_MSCP.mslg_unit_hvr,
		OS$gen_tiny,		DD$ushvrsn);
    switch(elrp->U_MSCP.mslg_cnt_id[6])
        {
        case ELMPCT_HSC50:
        case ELMPCT_HSC70:
            os_std(&elrp->U_MSCP.mslg_mult_unt,
		OS$gen_short,		DD$req_port);
	break;

	default:
            os_std(&elrp->U_MSCP.mslg_mult_unt,
		OS$gen_short,		DD$multi_unit);
	break;
	}
    break;
    }

return (status);
}

/*...	ENDROUTINE MSCP_BLD				*/



/*
*	.SBTTL	SCSI_BLD    -  Builds the SCSI segments
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine 
*	
* CALLING SEQUENCE:		CALL SCSI_BLD   (..See Below..)
*
*					Called from EI$BLD with address
*					with a error log entry
*
* FORMAL PARAMETERS:		
*
*	xxx_ptr			Pointers to all segments
*	elrp			Pointer to errlog entry
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		STD segments filled
*
* ROUTINE VALUE:		EI$SUCC 
*				EI$FAIL
*
* SIDE EFFECTS:			
*
*--
*/
/*...	ROUTINE SCSI_BLD(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,elrp) */

int  scsi_bld(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr, elrp)

EIS  *eis_ptr;
DIS  *dis_ptr;
CDS  *cds_ptr;
SDS  *sds_ptr;
ADS  *ads_ptr;

struct el_rec *elrp;

{
long    status;
long    *vector;
char    temp_char;
long    temp_long;
long	tmp_long;
char	*tmp_lptr;
char    part_num[8];
char    unit_num[4];

tmp_lptr = (char *)&tmp_long;

status = EI$SUCC;

ads_ptr->subtype = DD$SCSI_ADS;
if (ini_seg(ads_ptr) == EI$FAIL)
    status = EI$FAIL;

cds_ptr->subtype = DD$SCSI_CDS;
if (ini_seg(cds_ptr) == EI$FAIL)
    status = EI$FAIL;

os_std(&elrp->U_SCSI.scsi_elvers,
      OS$gen_tiny,		DD$revision);

os_std(&elrp->U_SCSI.error_typ,
      OS$gen_tiny,		DD$scsi_errtyp);

if ((elrp->U_SCSI.error_typ == SZ_ET_BUSERR) &&
    ((elrp->U_SCSI.info_flags & SZ_NCR5380) == SZ_NCR5380))
    {
    os_std(&elrp->U_SCSI.suberr_typ,
	OS$gen_tiny,		DD$scsi_suberr);
    }
if ((elrp->U_SCSI.error_typ == SZ_ET_BUSERR) &&
    ((elrp->U_SCSI.info_flags & SZ_DECSII) == SZ_DECSII))
    {
    os_std( &elrp->U_SCSI.suberr_typ,
       	    OS$gen_tiny, 	DD$sii_suberr);
    } 
    else if (elrp->U_SCSI.error_typ == SZ_ET_DBBR)
        {
        os_std( &elrp->U_SCSI.suberr_typ,
  	       	OS$gen_tiny,	DD$dbbr_suberr);
	os_std( &elrp->el_body.elbdev.eldevhdr.devhdr_blkno,
		OS$gen_long,	DD$devhdr_blkno);
	}
        else os_std(&elrp->U_SCSI.suberr_typ,
                    OS$gen_tiny,		DD$suberr_type);

if (elrp->elsubid.subid_class == ELSCSI_CNTRL)
    os_std(&elrp->elsubid.subid_unitnum,
           OS$gen_short,           DD$unitnumber);
 
if (elrp->U_SCSI.scsi_id != 0xff)
    os_std(&elrp->U_SCSI.scsi_id,
           OS$gen_tiny,		DD$scsi_id);

os_std(&elrp->U_SCSI.info_flags,
      OS$gen_long,		DD$info_flags);

temp_long = elrp->U_SCSI.info_flags;
if ((temp_long & SZ_LOGCMD) == SZ_LOGCMD)
  {
  os_std(elrp->U_SCSI.scsi_cmd,
	  OS$gen_bv_12,		DD$cmd_blk);

  if (elrp->elsubid.subid_class == ELCT_DISK)
    os_std(elrp->U_SCSI.scsi_cmd,
	  OS$gen_tiny,		DD$disk_cmd);
  else
    os_std(elrp->U_SCSI.scsi_cmd,
          OS$gen_tiny,          DD$tape_cmd);
  }

temp_long = elrp->U_SCSI.info_flags;
if ((temp_long & SZ_LOGSTAT) == SZ_LOGSTAT)
  os_std(&elrp->U_SCSI.scsi_status,
	  OS$gen_tiny,		DD$scsi_sts);

temp_long = elrp->U_SCSI.info_flags;
if ((temp_long & SZ_LOGMSG) == SZ_LOGMSG)
  os_std(&elrp->U_SCSI.scsi_msgin,
	  OS$gen_tiny,		DD$scsi_msg);
  
temp_long = elrp->U_SCSI.info_flags;
if ((temp_long & SZ_LOGSNS) == SZ_LOGSNS)
  {
  temp_char = elrp->U_SCSI.scsi_esd.snskey;
  os_std(&temp_char,
	 OS$gen_tiny,		DD$scsi_sense_key);


  tmp_lptr[0] = elrp->U_SCSI.scsi_esd.infobyte0;
  tmp_lptr[1] = elrp->U_SCSI.scsi_esd.infobyte1;
  tmp_lptr[2] = elrp->U_SCSI.scsi_esd.infobyte2;
  tmp_lptr[3] = elrp->U_SCSI.scsi_esd.infobyte3;

  if (elrp->elsubid.subid_class == ELCT_DISK)
	{
  	os_std( &elrp->U_SCSI.scsi_esd.asb.rz_asb.asc,
			OS$gen_tiny,		DD$scsi_asc);

	os_std( &tmp_long,
		OS$gen_long,		DD$scsi_lbn);
	os_std( &tmp_long,
		OS$gen_long,		DD$sum_lbn);

	if ( elrp->U_SCSI.scsi_elvers != 1)
                { 
		os_std( &elrp->U_SCSI.sect_num,
		 	OS$gen_long,		DD$sector_num); 
                os_std( &elrp->U_SCSI.sect_num,
                        OS$gen_long,            DD$sum_sec);
                }
        strcpy(part_num, "RZ");
        temp_long = ((elrp->el_body.elbdev.eldevhdr.devhdr_dev >> 3) & 0x1f); 
        sprintf(unit_num,"%d",temp_long);
        strcat(part_num, unit_num);

        temp_long = (((elrp->el_body.elbdev.eldevhdr.devhdr_dev & 7) 
                        + 0x41) );
        strcat(part_num, (char *) &temp_long);
	os_std( part_num,
		OS$gen_asc_12,		DD$partition);
	}

  if (elrp->elsubid.subid_class == ELCT_TAPE)
	{
	os_std( &tmp_long,
		OS$gen_long,		DD$xfer_len);
	os_std( &elrp->U_SCSI.scsi_esd.asb.tz_asb.ctlr,
		OS$gen_tiny,		DD$tape_ctlr);
	os_std( &elrp->U_SCSI.scsi_esd.asb.tz_asb.drv0,
		OS$gen_tiny,		DD$tape_drv0);
	os_std( &elrp->U_SCSI.scsi_esd.asb.tz_asb.drv1,
		OS$gen_tiny,		DD$tape_drv1);
	}
  vector = make_addr_vector(elrp, &elrp->U_SCSI.scsi_esd);
  vector[1] = sizeof(struct sz_exsns_dt);
  os_std(++vector, OS$gen_cntvecl, DD$xsns_dt);
  }

temp_long = elrp->U_SCSI.info_flags;
if ((temp_long & SZ_LOGREGS) == SZ_LOGREGS)
  {
  temp_long = elrp->U_SCSI.info_flags;
  if ((temp_long & SZ_NCR5380) == SZ_NCR5380)
    {
      os_std(&elrp->U_SCSI.scsi_regs.ncrregs.ini_cmd,
	      OS$gen_tiny,		DD$ini_cmd);
      os_std(&elrp->U_SCSI.scsi_regs.ncrregs.mode,
	      OS$gen_tiny,		DD$mode);
      os_std(&elrp->U_SCSI.scsi_regs.ncrregs.tar_cmd,
	      OS$gen_tiny,		DD$tar_cmd);
      os_std(&elrp->U_SCSI.scsi_regs.ncrregs.cur_stat,
	      OS$gen_tiny,		DD$cur_sts);
      os_std(&elrp->U_SCSI.scsi_regs.ncrregs.sel_ena,
	      OS$gen_tiny,		DD$sel_enable);
      os_std(&elrp->U_SCSI.scsi_regs.ncrregs.status,
	      OS$gen_tiny,		DD$ctlr_sts);
      os_std(&elrp->U_SCSI.scsi_regs.ncrregs.adr,
	      OS$gen_long,		DD$dma_adr);
      os_std(&elrp->U_SCSI.scsi_regs.ncrregs.cnt,
	      OS$gen_long,		DD$dma_cnt);
      os_std(&elrp->U_SCSI.scsi_regs.ncrregs.dir,
	      OS$gen_long,		DD$dma_dir);
    }

  temp_long = elrp->U_SCSI.info_flags;
  if ((temp_long & SZ_DECSII) == SZ_DECSII)      
    {
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_sdb,
	      OS$gen_short,		DD$scsi_dbp);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_sc1,
	      OS$gen_short,		DD$scsi_cs1);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_sc2,
	      OS$gen_short,		DD$scsi_cs2);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_csr,
	      OS$gen_short,		DD$csr);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_id,
	      OS$gen_short,		DD$msiidr);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_slcsr,
	      OS$gen_short,		DD$msislcs);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_destat,
	      OS$gen_short,		DD$msidestat);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_dstmo,
	      OS$gen_short,		DD$msitr);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_data,
	      OS$gen_short,		DD$data);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_dmctrl,
	      OS$gen_short,		DD$msidmctlr);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_dmlotc,
	      OS$gen_short,		DD$msidmlotc);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_dmaddrl,
	      OS$gen_short,		DD$msidmaaddrl);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_dmaddrh,
	      OS$gen_short,		DD$msidmaaddrh);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_dmabyte,
	      OS$gen_short,		DD$dmabyte);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_stlp,
	      OS$gen_short,		DD$msistlp);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_ltlp,
	      OS$gen_short,		DD$msitlp);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_ilp,
	      OS$gen_short,		DD$msiilp);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_dsctrl,
	      OS$gen_short,		DD$msidscr);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_cstat,
	      OS$gen_short,		DD$scscstate);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_dstat,
	      OS$gen_short,		DD$msidstat);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_comm,
	      OS$gen_short,		DD$msicommand);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_dictrl,
	      OS$gen_short,		DD$msidcr);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_clock,
	      OS$gen_short,		DD$diag_clk);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_bhdiag,
	      OS$gen_short,		DD$bh_diag);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_sidiag,
	      OS$gen_short,		DD$io_diag);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_dmdiag,
	      OS$gen_short,		DD$dm_diag);
      os_std(&elrp->U_SCSI.scsi_regs.siiregs.sii_mcdiag,
	      OS$gen_short,		DD$mc_diag);
    }		
  temp_long = elrp->U_SCSI.info_flags;
  if ((temp_long & SZ_NCRASC) == SZ_NCRASC)      
    {
      os_std(&elrp->U_SCSI.scsi_regs.ascregs.tclsb,
	      OS$gen_short,		DD$asc_tc);
      os_std(&elrp->U_SCSI.scsi_regs.ascregs.cmd,
	      OS$gen_tiny,		DD$asc_cmd);
      os_std(&elrp->U_SCSI.scsi_regs.ascregs.stat,
	      OS$gen_tiny,		DD$asc_stat);
      os_std(&elrp->U_SCSI.scsi_regs.ascregs.ss,
	      OS$gen_tiny,		DD$asc_ss);
      os_std(&elrp->U_SCSI.scsi_regs.ascregs.intr,
	      OS$gen_tiny,		DD$asc_intr);
      os_std(&elrp->U_SCSI.scsi_regs.ascregs.ffr,
	      OS$gen_tiny,		DD$asc_ffr);
      temp_long = (long)(elrp->U_SCSI.scsi_regs.ascregs.cnf1),
      temp_long >> 8;
      temp_long &= 0x00ffffff;
      os_std(&temp_long,
	      OS$gen_long,		DD$asc_cnf);
    }
  }		/* if sz_logregs */

temp_long = elrp->U_SCSI.info_flags;
if ((temp_long & SZ_LOGBUS) == SZ_LOGBUS)
      os_std(&elrp->U_SCSI.bus_data,
	      OS$gen_tiny,		DD$bus_data);

temp_long = elrp->U_SCSI.info_flags;
if ((temp_long & SZ_LOGSELST) == SZ_LOGSELST)
      os_std(elrp->U_SCSI.scsi_selst,
	      OS$gen_tiny,		DD$scsi_selsts);

return(status);

}

/*
*	.SBTTL	BLD_CORRUPT_EIS - Builds an eis for a corrupted elrp event
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine 
*	
* CALLING SEQUENCE:		CALL BLD_CORRUPT_EIS(..See Below..)
*					Called from EI$GET with address
*					if a corrupt elrp system event is
*					found.
* FORMAL PARAMETERS:		
*
*	eis			Pointer to eis
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		NONE
*
* ROUTINE VALUE:		EI$SUCC (success always returned)
*
*
* SIDE EFFECTS:			
*
*--
*/
/*...	ROUTINE BLD_CORRUPT_EIS(eis)				*/
void bld_corrupt_eis(eis)
   EIS *eis;
{
					/* Assign EI$CORRUPT to 
					   eventclass, and 0 out 
					   everything else	*/
eis->eventclass = EI$CORRUPT;
eis->eventtype = 0;
eis->recordnumber = 0;
eis->ostype = 0;
eis->datetime = 0;
eis->uptime = 0;
eis->serialnumber = 0;
eis->hostname = 0;

}

/*...	ENDROUTINE BLD_CORRUPT_EIS					    */



/*
*	.SBTTL	ADS_BLD - Builds an ads
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine 
*	
* CALLING SEQUENCE:		CALL ADS_BLD(..See Below..)
*					Called from EI$ADS_GET
*
* FORMAL PARAMETERS:		
*
*	ads_ptr			Pointer to ads
*	elrp			Pointer to data
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		NONE
*
* ROUTINE VALUE:		NONE
*
*
* SIDE EFFECTS:			Completes a standard EIMS record
*
*--
*/

/*...	ROUTINE ADS_BLD(ads_ptr, elrp)			*/
long ads_bld(ads_ptr, elrp)

ADS *ads_ptr;
struct el_rec *elrp;

{
long	status;
long	temp_long;
short	temp_short;
static	short pkt_len;		/* Length of pkt */

if (unk_rec == TRUE)
    {
    ads_ptr->subtype = DD$UNKNOWN_REC_ADS;
    return(EI$SUCC);
    }

if (ads_ptr->subtype == 0)	/* No previous ads */
    return(EI$FAIL);		/* All done        */

switch(elrp->elsubid.subid_class)
  {
/*************** ELCT_MEM  (Memory Errors) ******************/

			/* sid == 0e, aquarius host,
			   sid == 08, type == 6., aquarius SPU */
			/* the mem ads seg is only for spu memory errs */
  case ELCT_MEM :
	if(elrp->elsubid.subid_ctldevtyp == ELMCNTR_9000_SE &&
	   ((elrp->elrhdr.rhdr_sid>>24) == 0x08 && 
		(elrp->elrhdr.rhdr_systype>>24) == 6) ) {
				/* aquarius SPU */
		if(++spu_crd_cnt > elrp->el_body.el_spuse.n_entries) {
            ads_ptr->subtype = 0;
		} else {
            ads_ptr->subtype = DD$AQSPUSE_ADS;
			if (ini_seg (ads_ptr) == EI$FAIL)
				status = EI$FAIL;
			os_std(&elrp->el_body.el_spuse.smcs[spu_crd_cnt-1].smc0,
					OS$gen_long, DD$mck9000spu_smc0);
			os_std(&elrp->el_body.el_spuse.smcs[spu_crd_cnt-1].smc1,
					OS$gen_long, DD$mck9000spu_smc1);
			os_std(&elrp->el_body.el_spuse.smcs[spu_crd_cnt-1].smc2,
					OS$gen_long, DD$mck9000spu_smc2);
			os_std(&elrp->el_body.el_spuse.smcs[spu_crd_cnt-1].smc3,
					OS$gen_long, DD$mck9000spu_smc3);
		}
	} else {
    	ads_ptr->subtype = 0;
	}
    break;

/*********************** MSCP ERRORS *********************/ 

  case ELCT_DISK:
  case ELCT_TAPE:
  case ELCT_DCNTL:
    if (ads_ptr->subtype != DD$DSA_GENERIC_ADS)
        {
        ads_ptr->subtype = 0;
        return(EI$FAIL);
        }
/*
    ini_seg (0);
*/
    ads_ptr->subtype = 0;

    switch(elrp->U_MSCP.mslg_format)
      {


      case MSLG_FM_CNT_ER:				/* format 0 */
	switch(elrp->U_MSCP.mslg_cnt_id[6])
	  {
	  case ELMPCT_HSC50:
	  case ELMPCT_HSC70:
	      ads_ptr->subtype = DD$DSA_GEN2_ADS;
	      if (ini_seg(ads_ptr) == EI$FAIL)
	          status = EI$FAIL;
              os_std(&elrp->U_MSCP.mslg_level,
		OS$gen_short,		DD$std_retlvl);
              os_std(&elrp->U_MSCP.mslg_vol_ser,
		OS$gen_long,		DD$volsernum);
              os_std(&elrp->U_MSCP.mslg_vol_ser,
		OS$gen_long,		DD$sum_serial);
              os_std(elrp->U_MSCP.mslg_sdi+0,
		OS$gen_short,		DD$hsc_orig_err);
              os_std(elrp->U_MSCP.mslg_sdi+2,
		OS$gen_short,		DD$err_rec_flags);
              os_std(elrp->U_MSCP.mslg_sdi+4,
		OS$gen_tiny,		DD$lvl_a_retry);
              os_std(elrp->U_MSCP.mslg_sdi+5,
		OS$gen_tiny,		DD$lvl_b_retry);
              os_std(elrp->U_MSCP.mslg_sdi+6,
		OS$gen_short,		DD$buf_data_mem_addr);
              os_std(elrp->U_MSCP.mslg_sdi+8,
		OS$gen_short,		DD$src_det_req);
	  break;
	  case ELMPCT_RV20:
	      ads_ptr->subtype = DD$RV_CNTL_ADS;
	      if (ini_seg(ads_ptr) == EI$FAIL)
	          status = EI$FAIL;
              if (elrp->U_MSCP.mslg_event == 0x006A)
		{
                os_std(elrp->U_MSCP.mslg_unit_id,
	  	  OS$gen_long,		DD$rv_inc);
		}
              else if ((elrp->U_MSCP.mslg_event == 0x00AA) ||
              	       (elrp->U_MSCP.mslg_event == 0x00CA))
		{
                os_std(elrp->U_MSCP.mslg_unit_id,
	  	  OS$gen_long,		DD$rv_lesi);
		}
              else if (elrp->U_MSCP.mslg_event == 0x012A)
		{
                os_std(elrp->U_MSCP.mslg_unit_id,
	  	  OS$gen_long,		DD$rv_mem);
		}
	      else
                os_std(make_addr_vector(elrp, 
                  &elrp->U_MSCP.mslg_mult_unt),
                    OS$gen_acv,		DD$ctl_addl_info);
	  break;
	  default:
	  break;
	  };
      break;

      case MSLG_FM_BUS_ADDR:           			/* format 1 */
	  if (elrp->U_MSCP.mslg_cnt_id[6] == ELMPCT_RV20)
	      {
	      ads_ptr->subtype = DD$RV_CNTL_ADS;
	      if (ini_seg(ads_ptr) == EI$FAIL)
	          status = EI$FAIL;
              if (elrp->U_MSCP.mslg_event == 0x0007)
		{
                os_std(elrp->U_MSCP.mslg_unit_id[1],
	  	  OS$gen_long,		DD$rv_cmrw);
		}
	      }
      break;

      case MSLG_FM_DISK_TRN:           			/* format 2 */
	ads_ptr->subtype = DD$DSA_GEN2_ADS;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->U_MSCP.mslg_level,
		OS$gen_short,		DD$std_retlvl);
        os_std(&elrp->U_MSCP.mslg_vol_ser,
		OS$gen_long,		DD$volsernum);
        os_std(&elrp->U_MSCP.mslg_vol_ser,
		OS$gen_long,		DD$sum_serial);
        os_std(elrp->U_MSCP.mslg_sdi+2,
		OS$gen_short,		DD$err_rec_flags);
        os_std(elrp->U_MSCP.mslg_sdi+4,
		OS$gen_tiny,		DD$lvl_a_retry);
        os_std(elrp->U_MSCP.mslg_sdi+5,
		OS$gen_tiny,		DD$lvl_b_retry);
        os_std(elrp->U_MSCP.mslg_sdi+6,
		OS$gen_short,		DD$buf_data_mem_addr);
        os_std(elrp->U_MSCP.mslg_sdi+8,
		OS$gen_short,		DD$src_det_req);
	switch(elrp->U_MSCP.mslg_cnt_id[6])
	  {
	  case ELMPCT_HSC50:
	  case ELMPCT_HSC70:
              os_std(elrp->U_MSCP.mslg_sdi+0,
		OS$gen_short,		DD$hsc_orig_err);
	  break;
	  default:
              os_std(elrp->U_MSCP.mslg_sdi+0,
		OS$gen_short,		DD$orig_err);
	  break;
	  };
      break;


      case MSLG_FM_SDI:               			/* format 3 */
	ads_ptr->subtype = DD$SDI_GENERIC_ADS;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->U_MSCP.mslg_vol_ser,
		OS$gen_long,		DD$volsernum);
        os_std(&elrp->U_MSCP.mslg_vol_ser,
		OS$gen_long,		DD$sum_serial);
        os_std(elrp->U_MSCP.mslg_sdi,
		OS$gen_tiny,		DD$sdireqbyte);
        os_std(elrp->U_MSCP.mslg_sdi+1,
		OS$gen_tiny,		DD$sdimodbyte);
        os_std(elrp->U_MSCP.mslg_sdi+2,
		OS$gen_tiny,		DD$sdierrbyte);
        os_std(elrp->U_MSCP.mslg_sdi+3,
		OS$gen_tiny,		DD$sdictlbyte);
        os_std(elrp->U_MSCP.mslg_sdi+4,
		OS$gen_tiny,		DD$sdiret);
      break;


      case MSLG_FM_SML_DSK:           			/* format 4 */
	switch (elrp->elsubid.subid_ctldevtyp)
	  {
 	  case ELDT_RF30:
	    if (elrp->U_MSCP.mslg_event == 0x06b)
		{
	        ads_ptr->subtype = DD$RF3_FM4_POS_ADS;
	        if (ini_seg(ads_ptr) == EI$FAIL)
	            status = EI$FAIL;
        	os_std(elrp->U_MSCP.mslg_sdi+0,
			OS$gen_long,		DD$rf3_seek_ct);
        	os_std(elrp->U_MSCP.mslg_sdi+4,
			OS$gen_long,		DD$rf3_perf_ct);
        	os_std(elrp->U_MSCP.mslg_sdi+8,
			OS$gen_long, 		DD$rf3_event_ct);
        	os_std(elrp->U_MSCP.mslg_sdi+12,
			OS$gen_short,		DD$rf3_perf_dat);
		}
	    else
		{
	        ads_ptr->subtype = DD$RF3_FM4_NONPOS_ADS;
	        if (ini_seg(ads_ptr) == EI$FAIL)
	            status = EI$FAIL;
                os_std(&elrp->U_MSCP.mslg_hdr_code,
			OS$gen_long,		DD$rf3_error);
        	os_std(elrp->U_MSCP.mslg_sdi+0,
			OS$gen_long,		DD$rf3_lbn);
        	os_std(elrp->U_MSCP.mslg_sdi+0,
			OS$gen_long,		DD$sum_lbn);
        	os_std(elrp->U_MSCP.mslg_sdi+4,
			OS$gen_tiny,		DD$rf3_bbspace);
			/* pick up mslg_sid+(4-5) shift 4 off */
		temp_short = (*((short *) (elrp->U_MSCP.mslg_sdi+4)) >> 8);
			/* remove sign propagation and check for 00ff */
		if ((temp_short & 0xff00) == 0x00ff)
		    temp_short << 8;	/* special case servo off trk */
        	os_std(&temp_short,
			OS$gen_tiny,		DD$rf3_wrtflt);
        	os_std(elrp->U_MSCP.mslg_sdi+6,
			OS$gen_short,		DD$rf3_servo_stat);
		temp_short = *((short *) (elrp->U_MSCP.mslg_sdi+10));
		if (temp_short == 0x4000)
			/* special case DSR value not available */
		    temp_short = 1;		/* set bit 0 only */
        	os_std(&temp_short,
			OS$gen_short,		DD$rf3_phx_dsr);
		}
	  break;
	  default:
	    ads_ptr->subtype = DD$SMALL_DSK_ADS;
	    if (ini_seg(ads_ptr) == EI$FAIL)
	        status = EI$FAIL;
            os_std(make_addr_vector(elrp, 
              &elrp->U_MSCP.mslg_hdr_code),
                OS$gen_acv,		DD$ctl_addl_info);
	  break;
	  }
        os_std(&elrp->U_MSCP.mslg_vol_ser,
		OS$gen_long,		DD$volsernum);
        os_std(&elrp->U_MSCP.mslg_vol_ser,
		OS$gen_long,		DD$sum_serial);
      break;


      case MSLG_FM_TAPE_TRN:          			/* format 5 */
	switch (elrp->elsubid.subid_ctldevtyp)
	  {
 	  case ELTT_TK50:
	    ads_ptr->subtype = DD$F5_TK50_ADS;
	    if (ini_seg(ads_ptr) == EI$FAIL)
	        status = EI$FAIL;
            os_std(elrp->U_MSCP.mslg_sti+0,
		OS$gen_tiny,   		DD$sti_drv_flgs);
            os_std(elrp->U_MSCP.mslg_sti+1,
		OS$gen_long,   		DD$sti_loc);
	    temp_long = (*((long *) (elrp->U_MSCP.mslg_sti+4)) >> 8);
            os_std(&temp_long,
		OS$gen_long,  		DD$sti_tpcnt);
            os_std(elrp->U_MSCP.mslg_sti+8,
		OS$gen_long,   		DD$sti_state);
            os_std(elrp->U_MSCP.mslg_sti+12,
		OS$gen_short,  		DD$sti_op_flgs);
            os_std(elrp->U_MSCP.mslg_sti+14,
		OS$gen_tiny,  		DD$sti_status);
            os_std(elrp->U_MSCP.mslg_sti+15,
		OS$gen_tiny,  		DD$sti_drv_cod);
	  break;
 	  case ELTT_RV60:
 	  case ELTT_TRV20:
	    ads_ptr->subtype = DD$RV_F5_ADS;
	    if (ini_seg(ads_ptr) == EI$FAIL)
	        status = EI$FAIL;
            os_std(elrp->U_MSCP.mslg_sti+0,
		OS$gen_long,  		DD$sec_add);
            os_std(elrp->U_MSCP.mslg_sti+0,
		OS$gen_long,  		DD$sum_sec);
            os_std(elrp->U_MSCP.mslg_sti+4,
		OS$gen_long,  		DD$rv_flaw);
            os_std(elrp->U_MSCP.mslg_sti+8,
		OS$rv_svol_ser,		DD$rv_svol);
            os_std(elrp->U_MSCP.mslg_sti+8,
		OS$gen_long,		DD$sum_serial);
	  break;

          case ELTT_TA90:
          case ELTT_TA91:
            status = (mscp_ta90(ads_ptr, elrp));
	  break;

	  default:
	    ads_ptr->subtype = DD$TAP_XFER_ADS;
	    if (ini_seg(ads_ptr) == EI$FAIL)
	        status = EI$FAIL;
            os_std(make_addr_vector(elrp, elrp->U_MSCP.mslg_sti),
                OS$gen_acv,		DD$ctl_addl_info);
	  break;
	  }
        os_std(&elrp->U_MSCP.mslg_level,
		OS$gen_short,		DD$std_retlvl);
        os_std(&elrp->U_MSCP.mslg_fmtr_svr,
		OS$gen_tiny,		DD$fmt_svr);
        os_std(&elrp->U_MSCP.mslg_fmtr_hvr,
		OS$gen_tiny,		DD$fmt_hvr);
      break;

      case MSLG_FM_STI_ERR:           			/* format 6 */
	ads_ptr->subtype = DD$STI_COMM_ADS;
	if (ini_seg(ads_ptr) == EI$FAIL)
	      status = EI$FAIL;
        os_std(&elrp->U_MSCP.mslg_fmtr_svr,
		OS$gen_tiny,		DD$fmt_svr);
        os_std(&elrp->U_MSCP.mslg_fmtr_hvr,
		OS$gen_tiny,		DD$fmt_hvr);
      break;

      case MSLG_FM_STI_DEL:           			/* format 7 */
	switch (elrp->elsubid.subid_ctldevtyp)
	  {
	  case ELTT_TA81:
	    ads_ptr->subtype = DD$F7_TA81_ADS;
	    if (ini_seg(ads_ptr) == EI$FAIL)
	        status = EI$FAIL;
            os_std(elrp->U_MSCP.mslg_sti,
                OS$gen_long,            DD$st1drvst);
            os_std(elrp->U_MSCP.mslg_sti+4,
                OS$gen_long,            DD$gap_count);
            os_std(elrp->U_MSCP.mslg_sti+8,
                OS$gen_long,            DD$sti_diagsts);
            os_std(elrp->U_MSCP.mslg_sti+12,
                OS$gen_long,            DD$sti_sense0);
            os_std(elrp->U_MSCP.mslg_sti+16,
                OS$gen_long,            DD$sti_sense4);
            os_std(elrp->U_MSCP.mslg_sti+20,
                OS$gen_long,            DD$sti_sense8);
            os_std(elrp->U_MSCP.mslg_sti+24,
                OS$gen_long,            DD$sti_sense12);
            os_std(elrp->U_MSCP.mslg_sti+28,
                OS$gen_long,            DD$sti_sense16);
	  break;
	  case ELTT_TA78:
	    ads_ptr->subtype = DD$STI_DRV_ADS;
	    if (ini_seg(ads_ptr) == EI$FAIL)
	        status = EI$FAIL;
            os_std(elrp->U_MSCP.mslg_sti,
                OS$gen_long,            DD$st1drvst);
            os_std(elrp->U_MSCP.mslg_sti+4,
                OS$gen_long,            DD$gap_count);
            os_std(elrp->U_MSCP.mslg_sti+8,
                OS$gen_long,            DD$sti_diagsts);
            os_std(elrp->U_MSCP.mslg_sti+12,
                OS$gen_tiny,            DD$sti_opsav);
            os_std(elrp->U_MSCP.mslg_sti+13,
                OS$gen_short,           DD$sti_errnum);
            os_std(elrp->U_MSCP.mslg_sti+15,
                OS$gen_tiny,            DD$sti_rpfail);
            os_std(elrp->U_MSCP.mslg_sti+15,
                OS$gen_tiny,            DD$sum_rp);
            os_std(elrp->U_MSCP.mslg_sti+16,
                OS$gen_short,           DD$sti_rpath_rstat);
            os_std(elrp->U_MSCP.mslg_sti+18,
                OS$gen_tiny,            DD$sti_rmclp);
            os_std(elrp->U_MSCP.mslg_sti+19,
                OS$gen_long,            DD$sti_rdch_stat);
            os_std(elrp->U_MSCP.mslg_sti+23,
                OS$gen_long,            DD$sti_rdch_stat2);
            os_std(elrp->U_MSCP.mslg_sti+27,
                OS$gen_tiny,            DD$sti_crcwrd);
            os_std(elrp->U_MSCP.mslg_sti+28,
                OS$gen_short,           DD$sti_ecc);
            os_std(elrp->U_MSCP.mslg_sti+30,
                OS$gen_long,            DD$sti_rdnn_tie);
            os_std(elrp->U_MSCP.mslg_sti+34,
                OS$gen_tiny,            DD$sti_rdpb_tie);
            os_std(elrp->U_MSCP.mslg_sti+35,
                OS$gen_short,           DD$sti_pstat_tamt);
            os_std(elrp->U_MSCP.mslg_sti+37,
                OS$gen_short,           DD$sti_peri_prdd);
            os_std(elrp->U_MSCP.mslg_sti+39,
                OS$gen_tiny,            DD$sti_wmcsta);
            os_std(elrp->U_MSCP.mslg_sti+40,
                OS$gen_short,           DD$sti_tusel);
            os_std(elrp->U_MSCP.mslg_sti+42,
                OS$gen_tiny,            DD$sti_wrtdat);
            os_std(elrp->U_MSCP.mslg_sti+43,
                OS$gen_short,           DD$sti_bytcnt);
            os_std(elrp->U_MSCP.mslg_sti+45,
                OS$gen_short,           DD$sti_padcnt);
            os_std(elrp->U_MSCP.mslg_sti+47,
                OS$gen_short,           DD$sti_errcnt);
            os_std(elrp->U_MSCP.mslg_sti+49,
                OS$gen_tiny,            DD$sti_wmcerr);
            os_std(elrp->U_MSCP.mslg_sti+50,
                OS$gen_tiny,            DD$sti_intsta);
            os_std(elrp->U_MSCP.mslg_sti+51,
                OS$gen_tiny,            DD$tu_stat);
            os_std(elrp->U_MSCP.mslg_sti+52,
                OS$gen_short,           DD$tu_stat_ab);
            os_std(elrp->U_MSCP.mslg_sti+54,
                OS$gen_short,           DD$sti_ser_num);
            os_std(elrp->U_MSCP.mslg_sti+54,
                OS$gen_short,           DD$sum_serial);
            os_std(elrp->U_MSCP.mslg_sti+56,
                OS$gen_tiny,            DD$tu_diag);
	  break;
 	  case ELTT_RV60:
 	  case ELTT_TRV20:
	    ads_ptr->subtype = DD$RV_F7_ADS;
	    if (ini_seg(ads_ptr) == EI$FAIL)
	        status = EI$FAIL;
            if (elrp->U_MSCP.mslg_event == 0x0103)
	      {
              os_std(elrp->U_MSCP.mslg_sti+0,
		OS$gen_long,  		DD$rv_cm);
              os_std(elrp->U_MSCP.mslg_sti+4,
		OS$gen_long,  		DD$rv_sd);
              os_std(elrp->U_MSCP.mslg_sti+8,
		OS$gen_short, 		DD$rv_ver);
              os_std(elrp->U_MSCP.mslg_sti+10,
		OS$gen_tiny,  		DD$rv_cmaddr);
              os_std(elrp->U_MSCP.mslg_sti+11,
		OS$gen_tiny,  		DD$rv_drvmod);
	      }
	    else
	      {
              os_std(elrp->U_MSCP.mslg_sti+0,
		OS$gen_short, 		DD$rv_ustate);
              os_std(elrp->U_MSCP.mslg_sti+2,
		OS$gen_short, 		DD$rv_cmstat);
              os_std(elrp->U_MSCP.mslg_sti+4,
		OS$gen_short, 		DD$rv_intstat);
              os_std(elrp->U_MSCP.mslg_sti+6,
		OS$gen_tiny,  		DD$rv_delstat);
              os_std(elrp->U_MSCP.mslg_sti+7,
		OS$gen_tiny,  		DD$rv_curcm);
              os_std(elrp->U_MSCP.mslg_sti+8,
		OS$gen_long,  		DD$rv_slave1);
              os_std(elrp->U_MSCP.mslg_sti+12,
		OS$gen_short, 		DD$rv_slave2);
              os_std(elrp->U_MSCP.mslg_sti+14,
		OS$gen_short, 		DD$rv_func);
	      }
	  break;

          case ELTT_TA90:
          case ELTT_TA91:
            status = (mscp_ta90(ads_ptr, elrp));
	  break;
	  }
        os_std(&elrp->U_MSCP.mslg_fmtr_svr,
		OS$gen_tiny,		DD$fmt_svr);
        os_std(&elrp->U_MSCP.mslg_fmtr_hvr,
		OS$gen_tiny,		DD$fmt_hvr);
      break;


      case MSLG_FM_STI_FEL:           			/* format 8 */
	switch (elrp->elsubid.subid_ctldevtyp)
	  {
	  case ELTT_TA81:
	    ads_ptr->subtype = DD$F8_TA81_ADS;
	    if (ini_seg(ads_ptr) == EI$FAIL)
	        status = EI$FAIL;
            os_std(elrp->U_MSCP.mslg_sti+1,
                OS$gen_long,            DD$sti_errsym);
            os_std(elrp->U_MSCP.mslg_sti+5,
                OS$gen_long,            DD$sti_sense0);
            os_std(elrp->U_MSCP.mslg_sti+9,
                OS$gen_long,            DD$sti_sense4);
            os_std(elrp->U_MSCP.mslg_sti+13,
                OS$gen_long,            DD$sti_sense8);
            os_std(elrp->U_MSCP.mslg_sti+17,
                OS$gen_long,            DD$sti_sense12);
            os_std(elrp->U_MSCP.mslg_sti+21,
                OS$gen_long,            DD$sti_sense16);
 	  break;
	  case ELTT_TA78:
	    ads_ptr->subtype = DD$STI_FMT_ADS;
	    if (ini_seg(ads_ptr) == EI$FAIL)
	        status = EI$FAIL;
            os_std(elrp->U_MSCP.mslg_sti+2,
                OS$gen_tiny,            DD$sti_opsav);
            os_std(elrp->U_MSCP.mslg_sti+3,
                OS$gen_tiny,            DD$sti_connst);
            os_std(elrp->U_MSCP.mslg_sti+8,
                OS$gen_tiny,            DD$sti_prtosv);
            os_std(elrp->U_MSCP.mslg_sti+9,
                OS$gen_tiny,            DD$sti_fstosv);
            os_std(elrp->U_MSCP.mslg_sti+10,
                OS$gen_tiny,            DD$sti_pswisv);
            os_std(elrp->U_MSCP.mslg_sti+11,
                OS$gen_tiny,            DD$sti_lassta);
            os_std(elrp->U_MSCP.mslg_sti+12,
                OS$gen_short,           DD$sti_retry);
            os_std(elrp->U_MSCP.mslg_sti+14,
                OS$gen_tiny,            DD$sti_kicnt);
            os_std(elrp->U_MSCP.mslg_sti+15,
                OS$gen_long,            DD$sti_sp);
	  break;
          case ELTT_TA90:
          case ELTT_TA91:
            status = (mscp_ta90(ads_ptr, elrp));
	  break;
	  }
        os_std(&elrp->U_MSCP.mslg_fmtr_svr,
		OS$gen_tiny,		DD$fmt_svr);
        os_std(&elrp->U_MSCP.mslg_fmtr_hvr,
		OS$gen_tiny,		DD$fmt_hvr);
        os_std(elrp->U_MSCP.mslg_sti,
                OS$gen_short,           DD$sti_errnum);
      break;


      case MSLG_FM_REPLACE:           			/* format 9 */
	ads_ptr->subtype = DD$BAD_BLK_ADS;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->U_MSCP.mslg_vol_ser,
		OS$gen_long,		DD$volsernum);
        os_std(&elrp->U_MSCP.mslg_vol_ser,
		OS$gen_long,		DD$sum_serial);
      break;

      case MSLG_FM_IBMSENSE:           			/* format B */
	switch (elrp->elsubid.subid_ctldevtyp)
	  {
          case ELTT_TA90:
          case ELTT_TA91:
            status = (mscp_ta90(ads_ptr, elrp));
	  break;
	  }
        os_std(&elrp->U_MSCP.mslg_fmtr_svr,
		OS$gen_tiny,		DD$fmt_svr);
        os_std(&elrp->U_MSCP.mslg_fmtr_hvr,
		OS$gen_tiny,		DD$fmt_hvr);
      break;

      }
  break;

/********************* 106 ** BUS ERROR **************************/

  case ELCT_BUS:
    switch (elrp->elsubid.subid_type)
      {
      case ELBUS_BIER:
        if (++ads_next >= ads_num)      /* reuse ads BI_BUS_ERR_ADS */
          {
          ads_ptr->subtype = 0;
          return (EI$FAIL);
          }
        os_std(&elrp->el_body.elbier.biregs[ads_next].bi_typ,
		OS$gen_long,		DD$bidevreg);
        os_std(&elrp->el_body.elbier.biregs[ads_next].bi_ctrl,
		OS$gen_long,		DD$bicsr);
        os_std(&elrp->el_body.elbier.biregs[ads_next].bi_err,
		OS$gen_long,		DD$bibuserreg);
        os_std(&elrp->el_body.elbier.biregs[ads_next].bi_err_int,
		OS$gen_long,		DD$bierint);
        os_std(&elrp->el_body.elbier.biregs[ads_next].bi_int_dst,
		OS$gen_long,		DD$biintdst);
      break;
      default:
        ads_ptr->subtype = 0;
      break;
      }
  break;
/********************* 120 ELCT_MBUS ****************************/

  case ELCT_MBUS:
    if (ads_next >= ads_num)      /* reuse ads FF_ADS */
        {
        ads_ptr->subtype = 0;
        return (EI$FAIL);
        }
    temp_long = elrp->el_body.elmbus.elmb_module_log[ads_next][0];
    temp_long = temp_long >> 16;
    os_std(&temp_long,
		OS$gen_short,		DD$ff_valid);
    temp_long = elrp->el_body.elmbus.elmb_module_log[ads_next][0];
    temp_long = (temp_long << 16) >> 24;
    if(((elrp->el_body.elmbus.elmb_module_log[ads_next][1] << 24) >> 24) == 0x08)
        os_std(&temp_long,
		OS$gen_short,		DD$ff_cpuid);
    else
        os_std(&temp_long,
		OS$gen_short,		DD$ff_cpuidx);

    os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][1],
		OS$gen_long,		DD$ff_modtype);
    os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][2],
		OS$gen_long,		DD$ff_buscsr);
    os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][3],
		OS$gen_long,		DD$ff_busctl);

    switch ((elrp->el_body.elmbus.elmb_module_log[ads_next][1] <<8)>>24)
        {
        case 0x01:		/* FBIC  */
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][4],
		OS$gen_long,		DD$ff_busaddr);
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][5],
		OS$gen_long,		DD$ff_busdat);
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][6],
		OS$gen_long,		DD$ff_fbicsr);
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][7],
		OS$gen_long,		DD$ff_range);
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][8],
		OS$gen_long,		DD$ff_ipdvint);
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][9],
		OS$gen_long,		DD$ff_iadr1);
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][10],
		OS$gen_long,		DD$ff_iadr2);
        break;
        case 0x02:		/* FMDC  */
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][4],
		OS$gen_long,		DD$ff_busaddr);
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][5],
		OS$gen_long,		DD$ff_busdat);
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][6],
		OS$gen_long,		DD$ff_fmdcsr);
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][7],
		OS$gen_long,		DD$ff_baseaddr);
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][8],
		OS$gen_long,		DD$ff_eccaddr0);
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][9],
		OS$gen_long,		DD$ff_eccaddr1);
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][10],
		OS$gen_long,		DD$ff_eccsynd0);
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][11],
		OS$gen_long,		DD$ff_eccsynd1);
        break;
        case 0xfe:		/* FMCM  */
            os_std(&elrp->el_body.elmbus.elmb_module_log[ads_next][4],
		OS$gen_long,		DD$ff_baseaddr);
        break;
        }
  ads_next++;
  break;

/**************************** PANICS ***************************/

  case ELSW_PNC:
    switch (ads_ptr->subtype)
      {
      case DD$PANIC_ADS:
        ads_ptr->subtype = DD$PANIC_KERNEL1_ADS;
        if (ini_seg (ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
        os_std(&elrp->el_body.elpnc.kernstk.addr,
		OS$gen_acv,     	DD$pnckrnstk);
      break;
      case DD$PANIC_KERNEL1_ADS:
	ads_ptr->subtype = DD$PANIC_INTSTK1_ADS;
        if (ini_seg (ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
	os_std(&elrp->el_body.elpnc.intstk.addr,
		OS$gen_acv,		DD$pncintstk);
      break;
      default:
        ads_ptr->subtype = 0;
      break;
      }
  break;

/**************************** SCAN ***************************/

  case ELCT_9000_SCAN:
	switch(scan_ads_num++) {
		case 1:
			ads_ptr->subtype = DD$AQSCAN_IBOX_ADS;
			if (ini_seg (ads_ptr) == EI$FAIL)
				status = EI$FAIL;
#define SCANC &elrp->el_body.el_aqscan.cpuscu.cpu
			os_std(SCANC.i_fetch_err1, OS$gen_long, DD$aqscan_i_fetch_err1 );
			os_std(SCANC.i_fetch_err2, OS$gen_long, DD$aqscan_i_fetch_err2 );
			os_std(SCANC.i_decode_err1, OS$gen_long, DD$aqscan_i_decode_err1 );
			os_std(SCANC.i_xbr_decode, OS$gen_long, DD$aqscan_i_xbr_decode );
			os_std(SCANC.i_spec_err1, OS$gen_long, DD$aqscan_i_spec_err1 );
			os_std(SCANC.i_spec_err2, OS$gen_long, DD$aqscan_i_spec_err2 );
			os_std(SCANC.i_fetch_dat1, OS$gen_long, DD$aqscan_i_fetch_dat1 );
			os_std(SCANC.i_fetch_dat2, OS$gen_long, DD$aqscan_i_fetch_dat2 );
			os_std(SCANC.i_fetch_dat3, OS$gen_long, DD$aqscan_i_fetch_dat3 );
			os_std(SCANC.i_fetch_dat4, OS$gen_long, DD$aqscan_i_fetch_dat4 );
			os_std(SCANC.i_fetch_dat5, OS$gen_long, DD$aqscan_i_fetch_dat5 );
			os_std(SCANC.i_fetch_dat6, OS$gen_long, DD$aqscan_i_fetch_dat6 );
			os_std(SCANC.i_fetch_dat7, OS$gen_long, DD$aqscan_i_fetch_dat7 );
			os_std(SCANC.i_fetch_dat8, OS$gen_long, DD$aqscan_i_fetch_dat8 );
			os_std(SCANC.i_fetch_dat9, OS$gen_long, DD$aqscan_i_fetch_dat9 );
			os_std(SCANC.i_decode_data1,OS$gen_long, DD$aqscan_i_decode_data1);
			os_std(SCANC.i_decode_data2,OS$gen_long, DD$aqscan_i_decode_data2);
			os_std(SCANC.i_decode_data3,OS$gen_long, DD$aqscan_i_decode_data3);
			os_std(SCANC.i_decode_data4,OS$gen_long, DD$aqscan_i_decode_data4);
			os_std(SCANC.i_xdata1, OS$gen_long, DD$aqscan_i_xdata1 );
			os_std(SCANC.i_xdata2, OS$gen_long, DD$aqscan_i_xdata2 );
			os_std(SCANC.i_xdata3, OS$gen_long, DD$aqscan_i_xdata3 );
			os_std(SCANC.i_xdata4, OS$gen_long, DD$aqscan_i_xdata4 );
			os_std(SCANC.i_spec_data0, OS$gen_long, DD$aqscan_i_spec_data0 );
			os_std(SCANC.i_spec_data1, OS$gen_long, DD$aqscan_i_spec_data1 );
			os_std(SCANC.i_spec_data2, OS$gen_long, DD$aqscan_i_spec_data2 );
			os_std(SCANC.i_spec_data3, OS$gen_long, DD$aqscan_i_spec_data3 );
			os_std(SCANC.i_spec_data4, OS$gen_long, DD$aqscan_i_spec_data4 );
			os_std(SCANC.i_spec_data5, OS$gen_long, DD$aqscan_i_spec_data5 );
			os_std(SCANC.i_spec_data6, OS$gen_long, DD$aqscan_i_spec_data6 );
			os_std(SCANC.i_spec_data7, OS$gen_long, DD$aqscan_i_spec_data7 );
			os_std(SCANC.i_spec_data8, OS$gen_long, DD$aqscan_i_spec_data8 );
			os_std(SCANC.i_spec_data9, OS$gen_long, DD$aqscan_i_spec_data9 );
			os_std(SCANC.i_spec_data10, OS$gen_long, DD$aqscan_i_spec_data10 );
			os_std(SCANC.i_spec_data11, OS$gen_long, DD$aqscan_i_spec_data11 );
			os_std(SCANC.i_spec_data12, OS$gen_long, DD$aqscan_i_spec_data12 );
			os_std(SCANC.i_spec_data13, OS$gen_long, DD$aqscan_i_spec_data13 );
			os_std(SCANC.i_spec_data14, OS$gen_long, DD$aqscan_i_spec_data14 );
			os_std(SCANC.i_spec_data15, OS$gen_long, DD$aqscan_i_spec_data15 );
			os_std(SCANC.i_fetch_sts1, OS$gen_long, DD$aqscan_i_fetch_sts1 );
			os_std(SCANC.i_fetch_sts2, OS$gen_long, DD$aqscan_i_fetch_sts2 );
			os_std(SCANC.i_fetch_sts3, OS$gen_long, DD$aqscan_i_fetch_sts3 );
			break;
		case 2:
			ads_ptr->subtype = DD$AQSCAN_MBOX_ADS;
			if (ini_seg (ads_ptr) == EI$FAIL)
				status = EI$FAIL;
			os_std(SCANC.m_err_sum, OS$gen_long, DD$aqscan_m_err_sum );
			os_std(SCANC.m_tb_error, OS$gen_long, DD$aqscan_m_tb_error );
			os_std(SCANC.m_cache_error0, OS$gen_long, DD$aqscan_m_cache_error0);
			os_std(SCANC.m_cache_error1,OS$gen_long,DD$aqscan_m_cache_error1);
			os_std(SCANC.m_cache_error2, OS$gen_long, DD$aqscan_m_cache_error2);
			os_std(SCANC.m_err_data, OS$gen_long, DD$aqscan_m_err_data );
			os_std(SCANC.m_status, OS$gen_long, DD$aqscan_m_status );
			break;

		case 3:
			ads_ptr->subtype = DD$AQSCAN_VBOX_ADS;
			if (ini_seg (ads_ptr) == EI$FAIL)
					status = EI$FAIL;
			os_std(SCANC.v_errsum, OS$gen_long, DD$aqscan_v_errsum );
			os_std(SCANC.v_err0, OS$gen_long, DD$aqscan_v_err0 );
			os_std(SCANC.v_err1, OS$gen_long, DD$aqscan_v_err1 );
			os_std(SCANC.v_err2, OS$gen_long, DD$aqscan_v_err2 );
			os_std(SCANC.v_err3, OS$gen_long, DD$aqscan_v_err3 );
			os_std(SCANC.v_vcta_sts, OS$gen_long, DD$aqscan_v_vcta_sts );
			os_std(SCANC.v_vctb_sts, OS$gen_long, DD$aqscan_v_vctb_sts );
			os_std(SCANC.v_vctc_sts, OS$gen_long, DD$aqscan_v_vctc_sts );
			break;

		default:
			ads_ptr->subtype = 0;
			scan_ads_num =0;
	}
	break;

/******************** NO MORE ADS FOR ALL OTHERS ********************/
	
  default:
    ads_ptr->subtype = 0;
  break;
  }

/********************************************************************/


if (ads_ptr->subtype == 0)
  return (EI$FAIL);
if (status == EI$FAIL)
    unk_rec = TRUE;
return EI$SUCC;
}



/*
*	.SBTTL	MAKE_ADDR_VECTOR - Creates an addressed vector data type
*                                  to be moved to the std segment.
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine creates a elrp data type of DT_ADDT_CNT_VECTOR
*       format is - data addr, count of longs, data.
*	
* CALLING SEQUENCE:		CALL MAKE_ADDR_VECTOR (..See Below..)
*
* FORMAL PARAMETERS:		
*
*	elrp                    Errlog entry address
*       data                    Pointer to the data in elrp
*
* IMPLICIT INPUTS:		NONE
*
*
* ROUTINE VALUE:		Pointer to the vector
*
*
* SIDE EFFECTS:			
*
*--
*/

/*...	ROUTINE MAKE_ADDR_VECTOR (elrp,data) */
long 	*make_addr_vector (elrp,data)
struct	el_rec *elrp;
char    *data;
{
#define vec_size  64			/* max long words */

long    i;
long    len;
static  long    vector[vec_size+1];	/* add zero fill long */

for (i = 0; i < vec_size+1; i++)
    vector[i] = 0;

len       = (elrp->elrhdr.rhdr_reclen - 4) - ((long)data - (long)elrp);

vector[0] = (long)data - (long)&elrp->U_MSCP.mslg_cmd_ref;

i = (vec_size - 2) * 4;
if (len > i)			/* Actual longs in data part*/
    len = i;

for(i = 0; i < len; i++)
    {
    ((char *)vector)[8+i] = *(data+i); 
    }
vector[1] = (len+3) & 0xfffffffc;         /* vector len = actual bytes */
return (vector);
} 


/*
*	.SBTTL	MSCP_ROUTINES - Builds additional device specific
*				ADSs for MSCP devices
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine 
*	
* CALLING SEQUENCE:		CALL MSCP_....(..See Below..)
*					Called from ADS_BLD
*
* FORMAL PARAMETERS:		
*
*	ads_ptr			Pointer to ads
*	elrp			Pointer to data
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		NONE
*
* ROUTINE VALUE:		NONE
*
*
* SIDE EFFECTS:			Completes a standard EIMS record
*
***********************************************************************
*/

/*...	ROUTINE MSCP_TA90(ads_ptr, elrp)			*/

long mscp_ta90(ads_ptr, elrp)

ADS  *ads_ptr;

struct el_rec *elrp;

{
long	status;
long	temp_long;
short	temp_short;
char    *temp_lptr;
char    *temp_sptr;

u_char  *dev_dep_info;
long    *make_addr_vector();

temp_lptr = (char *)&temp_long;
temp_sptr = (char *)&temp_short;

switch(elrp->U_MSCP.mslg_format)
    {
    case MSLG_FM_TAPE_TRN:            			/* format 5 */
    case MSLG_FM_IBMSENSE:           			/* format B */
    ads_ptr->subtype = DD$TA9_F5_7_B_ADS;
    if (ini_seg(ads_ptr) == EI$FAIL)
        status = EI$FAIL;
    dev_dep_info = (u_char *)elrp->U_MSCP.mslg_sti;
    break;

    case MSLG_FM_STI_DEL:           			/* format 7 */
    ads_ptr->subtype = DD$TA9_F5_7_B_ADS;
    if (ini_seg(ads_ptr) == EI$FAIL)
        status = EI$FAIL;
    dev_dep_info = (u_char *)elrp->U_MSCP.mslg_sti;
    os_std(dev_dep_info+0,
		OS$gen_long,		DD$ta9_chr);
    os_std(dev_dep_info+4,
		OS$gen_long,		DD$gap_count);
    dev_dep_info = (u_char *)elrp->U_MSCP.mslg_sti+8;
    break;

    case MSLG_FM_STI_FEL:           			/* format 8 */
    ads_ptr->subtype = DD$TA9_F8_ADS;
    if (ini_seg(ads_ptr) == EI$FAIL)
        return(EI$FAIL);
    dev_dep_info = (u_char *)elrp->U_MSCP.mslg_sti;
    os_std(dev_dep_info+0,
		OS$gen_tiny,		DD$ta9_eid);
    os_std(dev_dep_info+1,
		OS$gen_tiny,		DD$ta9_mmcd);
    os_std(make_addr_vector(elrp, dev_dep_info+2),
                OS$gen_acv,             DD$ctl_addl_info);
    return(EI$SUCC);
    break;
    default:
        return(EI$FAIL);
    break;
    }

os_std(dev_dep_info+0,
		OS$gen_short,		DD$ta9_0_1);
os_std(dev_dep_info+3,
		OS$gen_tiny,		DD$ta9_3);
temp_lptr[2] = dev_dep_info[4];
temp_lptr[1] = dev_dep_info[5];
temp_lptr[0] = dev_dep_info[6];
temp_long   &= 0x000fffff;
os_std(&temp_long,
		OS$gen_long,		DD$ta9_4_6);
os_std(dev_dep_info+7,
		OS$gen_tiny,		DD$ta9_7);

switch (dev_dep_info[7])
    {
    case 0:
        return(EI$SUCC);
    break;

    case 0x19:
    case 0x20:
        os_std(dev_dep_info+2,
		OS$gen_tiny,		DD$ta9_2a);
        os_std(dev_dep_info+8,
		OS$gen_tiny,		DD$ta9_8a);
        if ((dev_dep_info+9  != 0) ||
            (dev_dep_info+10 != 0) ||
            (dev_dep_info+11 != 0))
            os_std(dev_dep_info+9,
		OS$gen_tiny,		DD$ta9_9a);
        temp_lptr[3] = dev_dep_info[12];
        temp_lptr[2] = dev_dep_info[13];
        temp_lptr[1] = dev_dep_info[10];
        temp_lptr[0] = dev_dep_info[11];
        os_std(&temp_long,
		OS$gen_long,		DD$ta9_10_13a);
        temp_lptr[3] = dev_dep_info[16];
        temp_lptr[2] = dev_dep_info[17];
        temp_lptr[1] = dev_dep_info[14];
        temp_lptr[0] = dev_dep_info[15];
        os_std(&temp_long,
		OS$gen_long,		DD$ta9_14_17a);
        os_std(dev_dep_info+18,
		OS$gen_short,		DD$ta9_18_19a);
        os_std(dev_dep_info+20,
		OS$gen_long,		DD$ta9_20_23a);
        os_std(dev_dep_info+24,
		OS$gen_long,		DD$ta9_24_27a);
        temp_lptr[2] = dev_dep_info[27];
        temp_lptr[1] = dev_dep_info[28];
        temp_lptr[0] = dev_dep_info[29];
        temp_long   &= 0x000fffff;
        os_std(&temp_long,
		OS$gen_long,		DD$ta9_27_29);
        os_std(dev_dep_info+30,
		OS$gen_short,		DD$ta9_30_31a);
    break;

    case 0x21:
        os_std(dev_dep_info+2,
		OS$gen_tiny,		DD$ta9_2b);
        os_std(dev_dep_info+8,
		OS$gen_tiny,		DD$ta9_8b);
        os_std(dev_dep_info+9,
		OS$gen_tiny,		DD$ta9_9b);
        os_std(dev_dep_info+10,
		OS$gen_long,		DD$ta9_10_13b);
        temp_lptr[3] = dev_dep_info[16];
        temp_lptr[2] = dev_dep_info[17];
        temp_lptr[1] = dev_dep_info[14];
        temp_lptr[0] = dev_dep_info[15];
        os_std(&temp_long,
		OS$gen_long,		DD$ta9_14_17b);
        temp_lptr[2] = dev_dep_info[26];
        temp_long <<= 4;
        temp_lptr[3] = dev_dep_info[19];
        temp_lptr[1] = dev_dep_info[18];
        temp_lptr[0] = dev_dep_info[26];
        temp_long >>= 4;
        temp_long   &= 0x0fff0fff;
        os_std(&temp_long,
		OS$gen_long,		DD$ta9_18_19b);
        os_std(dev_dep_info+20,
		OS$gen_long,		DD$ta9_20_23b);
        os_std(dev_dep_info+24,
		OS$gen_long,		DD$ta9_24_27b);
        temp_lptr[2] = dev_dep_info[27];
        temp_lptr[1] = dev_dep_info[28];
        temp_lptr[0] = dev_dep_info[29];
        temp_long   &= 0x000fffff;
        os_std(&temp_long,
		OS$gen_long,		DD$ta9_27_29);
        os_std(dev_dep_info+30,
		OS$gen_short,		DD$ta9_30_31b);
    break;

    case 0x92:
        os_std(dev_dep_info+2,
		OS$gen_tiny,		DD$ta9_2a);
        temp_sptr[1] = dev_dep_info[8];
        temp_sptr[0] = dev_dep_info[9];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_cc);
        os_std(dev_dep_info+10,
		OS$gen_tiny,		DD$ta9_par);
        temp_sptr[1] = dev_dep_info[11];
        temp_sptr[0] = dev_dep_info[12];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct1);
        temp_sptr[1] = dev_dep_info[13];
        temp_sptr[0] = dev_dep_info[14];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct2);
        temp_sptr[1] = dev_dep_info[15];
        temp_sptr[0] = dev_dep_info[16];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct3);
        temp_sptr[1] = dev_dep_info[17];
        temp_sptr[0] = dev_dep_info[18];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct4);
        temp_sptr[1] = dev_dep_info[19];
        temp_sptr[0] = dev_dep_info[20];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct5);
        temp_sptr[1] = dev_dep_info[21];
        temp_sptr[0] = dev_dep_info[22];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct6);
        temp_sptr[1] = dev_dep_info[23];
        temp_sptr[0] = dev_dep_info[24];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct7);
        temp_sptr[1] = dev_dep_info[25];
        temp_sptr[0] = dev_dep_info[26];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct8);
        temp_sptr[1] = dev_dep_info[27];
        temp_sptr[0] = dev_dep_info[28];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct9);
        temp_sptr[1] = dev_dep_info[29];
        temp_sptr[0] = dev_dep_info[30];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct10);
        temp_sptr[1] = dev_dep_info[31];
        temp_sptr[0] = dev_dep_info[32];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct11);
        temp_sptr[1] = dev_dep_info[33];
        temp_sptr[0] = dev_dep_info[34];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct12);
        temp_sptr[1] = dev_dep_info[35];
        temp_sptr[0] = dev_dep_info[36];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct13);
        temp_sptr[1] = dev_dep_info[37];
        temp_sptr[0] = dev_dep_info[38];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct14);
        temp_sptr[1] = dev_dep_info[39];
        temp_sptr[0] = dev_dep_info[40];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct15);
        temp_sptr[1] = dev_dep_info[41];
        temp_sptr[0] = dev_dep_info[42];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct16);
        temp_sptr[1] = dev_dep_info[43];
        temp_sptr[0] = dev_dep_info[44];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct17);
        temp_sptr[1] = dev_dep_info[45];
        temp_sptr[0] = dev_dep_info[46];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct18);
        temp_sptr[1] = dev_dep_info[47];
        temp_sptr[0] = dev_dep_info[48];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct19);
        temp_sptr[1] = dev_dep_info[49];
        temp_sptr[0] = dev_dep_info[50];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct20);
        temp_sptr[1] = dev_dep_info[51];
        temp_sptr[0] = dev_dep_info[52];
        os_std(&temp_short,
		OS$gen_short,		DD$ta9_ct21);
    break;
    }
return (status);
}


