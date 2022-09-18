#ifndef lint
static char sccsid[]  =  "@(#)eribld_aqpcs.c	4.3   (ULTRIX)   12/20/90"; 
#endif  lint

/*
*
* COPYRIGHT (C) 1989, 1990 DIGITAL EQUIPMENT CORP.,
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
*/

/* vax 9000 pcs eribld */

/* #include <stdio.h> */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/errlog.h>
#include <machine/cpu.h>
/* #include "eims.h" */
#include "erms.h"
/* #include "select.h" */
#include "generic_dsd.h"
#include "std_dsd.h"
#include "os_dsd.h"
#include "eiliterals.h"
#include "qtrans.h"

/* These structures replace the ones in errlog_aq.h
   Errlog structures normally belong in errlog_aq.h
   because both kernel code and uerf refer to the same data.
   But, these messages are passed from the SPU(console) to
   the host already packaged, and the kernel code just dumps
   n-bytes into the error log; it doesn't need to know the
   real structure.  We originally placed the structure
   in the kernel-accessible header file errlog_aq.h.
   But then we needed to change the structures after all the
   header files and kernel code was frozen.  So, we simply
   add the revised structures here.  This source file is the
   only source file that actually refers to the structure members.
*/

struct new_pcs {
	u_char format_byte;
	u_char idreg;
	u_char xcpid;
	u_char severity;
	union {
		struct {
			u_short rsreg;
			u_char asdreg;
			u_char hwsrega;
			u_char hwsregb;
			u_char hwsregc;
			u_char hwsregd;
			u_char hwsrege;
			u_char hwsregf;
			u_char pfreg_1; /* this is a short in two parts */
			u_char pfreg_2;
			u_char scsreg;
			u_char rcreg;
			union {
				struct {
					u_char mgnreg;
					u_short measure;
					u_short low;
					u_short high;
					u_short reg0cur;
					u_short reg1cur;
					u_short reg2cur;
					u_short reg3cur;
					u_short reg4cur;
				} fmt1;
				struct {
					/* these shorts are not aligned */
					u_char tempa_1;
					u_char tempa_2;
					u_char tempb_1;
					u_char tempb_2;
					u_char yellowa_1;
					u_char yellowa_2;
					u_char reda_1;
					u_char reda_2;
					u_char yellowb_1;
					u_char yellowb_2;
					u_char redb_1;
					u_char redb_2;
					u_char open_1;
					u_char open_2;
					u_char shortsensor_1;
					u_char shortsensor_2;
				} fmt2;
			} fmt;
		} ric;
		struct {
			u_short psreg;
			u_char pwr1reg;
			u_char pwr2reg;
			u_char pasdrsb[17];
#define pem_stcreg (PCS.ricpem.pem.stcreg_1|(PCS.ricpem.pem.stcreg_2<<8))
			u_char stcreg_1;
			u_char stcreg_2;
			u_char connreg;
			union {
				struct {
					u_char keyabreg;
					u_char keybcreg;
					u_short ricavarsb[18];
				} fmt4;
				struct {
					u_char bbureg;
				} fmt5;
				struct {
					u_short ocpswreg;
					u_char ocpdisreg[3];
				} fmt6;
				struct {
					u_char bscreg;
				} fmt7;
			} fmt;
		} pem;
	} ricpem;
};

struct new_pcsstat {
	u_char format_byte;

#define stat_stcreg (PCST.stcreg_1|(PCST.stcreg_2<<8))
	u_char stcreg_1;
	u_char stcreg_2;
#define stat_ambient (PCST.ambient_1|(PCST.ambient_2<<8))
	u_char ambient_1;
	u_char ambient_2;
#define stat_scufan1 (PCST.scufan1_1|(PCST.scufan1_2<<8))
	u_char scufan1_1;
	u_char scufan1_2;
#define stat_scufan2 (PCST.scufan2_1|(PCST.scufan2_2<<8))
	u_char scufan2_1;
	u_char scufan2_2;
#define stat_scufan3 (PCST.scufan3_1|(PCST.scufan3_2<<8))
	u_char scufan3_1;
	u_char scufan3_2;

	u_char scuasd;

#define stat_scufansts (PCST.scufansts_1|(PCST.scufansts_2<<8))
	u_char scufansts_1;
	u_char scufansts_2;
#define stat_scuairsts (PCST.scuairsts_1|(PCST.scuairsts_2<<8))
	u_char scuairsts_1;
	u_char scuairsts_2;
#define stat_cpafan1 (PCST.cpafan1_1|(PCST.cpafan1_2<<8))
	u_char cpafan1_1;
	u_char cpafan1_2;
#define stat_cpafan2 (PCST.cpafan2_1|(PCST.cpafan2_2<<8))
	u_char cpafan2_1;
	u_char cpafan2_2;
#define stat_cpafan3 (PCST.cpafan3_1|(PCST.cpafan3_2<<8))
	u_char cpafan3_1;
	u_char cpafan3_2;

	u_char cpaasd;

#define stat_cpafansts (PCST.cpafansts_1|(PCST.cpafansts_2<<8))
	u_char cpafansts_1;
	u_char cpafansts_2;
#define stat_cpaairsts (PCST.cpaairsts_1|(PCST.cpaairsts_2<<8))
	u_char cpaairsts_1;
	u_char cpaairsts_2;
#define stat_cpbfan1 (PCST.cpbfan1_1|(PCST.cpbfan1_2<<8))
	u_char cpbfan1_1;
	u_char cpbfan1_2;
#define stat_cpbfan2 (PCST.cpbfan2_1|(PCST.cpbfan2_2<<8))
	u_char cpbfan2_1;
	u_char cpbfan2_2;
#define stat_cpbfan3 (PCST.cpbfan3_1|(PCST.cpbfan3_2<<8))
	u_char cpbfan3_1;
	u_char cpbfan3_2;

	u_char cpbasd;

	u_short cpbfansts;
	u_short cpbairsts;
	u_char ioastatus;
	u_char ioaasd;
	u_char iobstatus;
	u_char iobasd;
	u_short wcuatempo;
	u_short wcuatempi;
	u_short wcuah2oo;
	u_short wcuah2oi;
	u_short wcuassts;
	u_short wcuasts;
	u_short wcubtempo;
	u_short wcubtempi;
	u_short wcubh2oo;
	u_short wcubh2oi;
	u_short wcubssts;
	u_short wcubsts;
	u_short ioacab;
	u_short iobcab;
	u_short ioabi;
	u_short iobbi;
	u_short ioaac;
	u_short iobac;
	u_short ioagc;
	u_short iobgc;
	u_short pcssts;
	u_short busareg;
	u_short busavbus;
	u_short busareg0;
	u_short busareg1;
	u_short busareg2;
	u_short busareg3;
	u_short busareg4;
	u_short busasts;
	u_short busbreg;
	u_short busbvbus;
	u_short busbreg0;
	u_short busbreg1;
	u_short busbreg2;
	u_short busbreg3;
	u_short busbreg4;
	u_short busbsts;
	u_short buscreg;
	u_short buscvbus;
	u_short buscreg0;
	u_short buscreg1;
	u_short buscreg2;
	u_short buscreg3;
	u_short buscreg4;
	u_short buscsts;
	u_short busdreg;
	u_short busdvbus;
	u_short busdreg0;
	u_short busdreg1;
	u_short busdreg2;
	u_short busdreg3;
	u_short busdreg4;
	u_short busdsts;
	union {
	  struct {
		u_short busereg;
		u_short busevbus;
		u_short busereg0;
		u_short busereg1;
		u_short busereg2;
		u_short busereg3;
		u_short busereg4;
		u_short busests;
		u_short busfreg;
		u_short busfvbus;
		u_short busfreg0;
		u_short busfreg1;
		u_short busfreg2;
		u_short busfreg3;
		u_short busfreg4;
		u_short busfsts;
	  } tab2;
	  struct {
		u_short busjreg;
		u_short busjvbus;
		u_short busjreg0;
		u_short busjreg1;
		u_short busjreg2;
		u_short busjreg3;
		u_short busjreg4;
		u_short busjsts;
		u_short buskreg;
		u_short buskvbus;
		u_short buskreg0;
		u_short buskreg1;
		u_short buskreg2;
		u_short buskreg3;
		u_short buskreg4;
		u_short busksts;
		u_short busmreg;
		u_short busmvbus;
		u_short busmreg0;
		u_short busmreg1;
		u_short busmreg2;
		u_short busmreg3;
		u_short busmreg4;
		u_short busmsts;
		u_short busnreg;
		u_short busnvbus;
		u_short busnreg0;
		u_short busnreg1;
		u_short busnreg2;
		u_short busnreg3;
		u_short busnreg4;
		u_short busnsts;
	  } tab4;
	} tab2or4;
};

struct el_rec_new {
	struct el_rhdr elrhdr;			/* record header */
	struct el_sub_id elsubid;		/* subsystem id packet */
	union {
		struct new_pcs el_aqpcs;
		struct new_pcsstat el_aqpcsstat;
	} el_body;
	char eltrailer[EL_TRAILERSIZE];			/* asc trailer code  */
};


extern struct qt_entry qt_psreg[];
extern struct qt_entry qt_rsreg_old[];
extern struct qt_entry qt_rsreg_new[];
extern struct qt_entry qt_hwsrega[], qt_hwsregb[], qt_hwsregc[], qt_hwsregd[];
extern struct qt_entry qt_pfreg[];

int  v9000_bld_emm(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp_old)
EIS  *eis_ptr;
DIS  *dis_ptr;
CDS  *cds_ptr;
SDS  *sds_ptr;
ADS  *ads_ptr;
SIS  *sis_ptr;
struct el_rec *elrp_old;
{
struct el_rec_new *elrp = (struct el_rec_new *)elrp_old;
	int status;
	unsigned long value;
	unsigned char uc, xcpid;
	int format_type;
#define MAX_TRSTRING_LEN 36
		/* strings area for exception entry */
	static char asdreg_str1[MAX_TRSTRING_LEN+2];
	static char asdreg_str2[MAX_TRSTRING_LEN+2];
	static char asdreg_str3[MAX_TRSTRING_LEN+2];
	static char asdreg_str4[MAX_TRSTRING_LEN+2];
	static char measure_str[MAX_TRSTRING_LEN+2];
	static char low_str[MAX_TRSTRING_LEN+2];
	static char high_str[MAX_TRSTRING_LEN+2];
	static char reg0cur_str[MAX_TRSTRING_LEN+2];
	static char reg1cur_str[MAX_TRSTRING_LEN+2];
	static char reg2cur_str[MAX_TRSTRING_LEN+2];
	static char reg3cur_str[MAX_TRSTRING_LEN+2];
	static char reg4cur_str[MAX_TRSTRING_LEN+2];
	static char tempa_str[MAX_TRSTRING_LEN+2];
	static char tempb_str[MAX_TRSTRING_LEN+2];
	static char yellowa_str[MAX_TRSTRING_LEN+2];
	static char reda_str[MAX_TRSTRING_LEN+2];
	static char yellowb_str[MAX_TRSTRING_LEN+2];
	static char redb_str[MAX_TRSTRING_LEN+2];
	static char open_str[MAX_TRSTRING_LEN+2];
	static char shortsensor_str[MAX_TRSTRING_LEN+2];

		/* strings area for status entry */
	static char ambient[MAX_TRSTRING_LEN+2];
	static char scufan1[MAX_TRSTRING_LEN+2];
	static char scufan2[MAX_TRSTRING_LEN+2];
	static char scufan3[MAX_TRSTRING_LEN+2];
	static char scuasd[MAX_TRSTRING_LEN+2];
	static char cpafan1[MAX_TRSTRING_LEN+2];
	static char cpafan2[MAX_TRSTRING_LEN+2];
	static char cpafan3[MAX_TRSTRING_LEN+2];
	static char cpaasd[MAX_TRSTRING_LEN+2];
	static char cpbfan1[MAX_TRSTRING_LEN+2];
	static char cpbfan2[MAX_TRSTRING_LEN+2];
	static char cpbfan3[MAX_TRSTRING_LEN+2];
	static char cpbasd[MAX_TRSTRING_LEN+2];
	static char ioaasd[MAX_TRSTRING_LEN+2];
	static char iobasd[MAX_TRSTRING_LEN+2];
	static char wcuatempo[MAX_TRSTRING_LEN+2];
	static char wcuatempi[MAX_TRSTRING_LEN+2];
	static char wcubtempo[MAX_TRSTRING_LEN+2];
	static char wcubtempi[MAX_TRSTRING_LEN+2];
	static char busavbus[MAX_TRSTRING_LEN+2];
	static char busbvbus[MAX_TRSTRING_LEN+2];
	static char buscvbus[MAX_TRSTRING_LEN+2];
	static char busdvbus[MAX_TRSTRING_LEN+2];
	static char busevbus[MAX_TRSTRING_LEN+2];
	static char busfvbus[MAX_TRSTRING_LEN+2];
	static char busjvbus[MAX_TRSTRING_LEN+2];
	static char buskvbus[MAX_TRSTRING_LEN+2];
	static char busmvbus[MAX_TRSTRING_LEN+2];
	static char busnvbus[MAX_TRSTRING_LEN+2];

#define PCS elrp->el_body.el_aqpcs
#define PCST elrp->el_body.el_aqpcsstat
 if( PCS.format_byte == 1 ) {
		/* Power/Environmental Entry */
		/* status information */
	cds_ptr->subtype = DD$AQPCST_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	ads_ptr->subtype = DD$AQPCST_ADS;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
	os_std(&PCST.stcreg_1, OS$gen_short, DD$aqpcst_stcreg);
	sprintf(ambient, "AMBIENT TEMP (DEG C.) = %c%3d.%02d",
			stat_ambient&0x8000?'-':'+',
			stat_ambient>>8&0x7f, stat_ambient&0xff);
	os_std(ambient, OS$gen_asc_52, DD$aqpcst_ambient);
	sprintf(scufan1, "SCU FAN 1 TEMP (DEG C.) = %c%3d.%02d",
			stat_scufan1&0x8000?'-':'+',
			stat_scufan1>>8&0x7f, stat_scufan1&0xff);
	sprintf(scufan2, "SCU FAN 2 TEMP (DEG C.) = %c%3d.%02d",
			stat_scufan2&0x8000?'-':'+',
			stat_scufan2>>8&0x7f, stat_scufan2&0xff);
	sprintf(scufan3, "SCU FAN 3 TEMP (DEG C.) = %c%3d.%02d",
			stat_scufan3&0x8000?'-':'+',
			stat_scufan3>>8&0x7f, stat_scufan3&0xff);
	if( (PCST.scuasd&0xff) == 0xff)
		sprintf(scuasd, "NO ASD PENDING FOR SCU CAB");
	else
		sprintf(scuasd, "SCU CAB ASD in %d SEC", PCST.scuasd * 5);
	os_std(scufan1, OS$gen_asc_52, DD$aqpcst_scufan1);
	os_std(scufan2, OS$gen_asc_52, DD$aqpcst_scufan2);
	os_std(scufan3, OS$gen_asc_52, DD$aqpcst_scufan3);
	os_std(scuasd, OS$gen_asc_52, DD$aqpcst_scuasd);
	os_std(&PCST.scufansts_1, OS$gen_short, DD$aqpcst_scufansts);
	os_std(&PCST.scuairsts_1, OS$gen_short, DD$aqpcst_scuairsts);
	sprintf(cpafan1, "CPA FAN 1 TEMP (DEG C.) = %c%3d.%02d",
			stat_cpafan1&0x8000?'-':'+',
			stat_cpafan1>>8&0x7f, stat_cpafan1&0xff);
	sprintf(cpafan2, "CPA FAN 2 TEMP (DEG C.) = %c%3d.%02d",
			stat_cpafan2&0x8000?'-':'+',
			stat_cpafan2>>8&0x7f, stat_cpafan2&0xff);
	sprintf(cpafan3, "CPA FAN 3 TEMP (DEG C.) = %c%3d.%02d",
			stat_cpafan3&0x8000?'-':'+',
			stat_cpafan3>>8&0x7f, stat_cpafan3&0xff);
	if( (PCST.cpaasd&0xff) == 0xff)
		sprintf(cpaasd, "NO ASD PENDING FOR CPA CAB");
	else
		sprintf(cpaasd, "CPA CAB ASD in %d SEC", PCST.cpaasd * 5);
	os_std(cpafan1, OS$gen_asc_52, DD$aqpcst_cpafan1);
	os_std(cpafan2, OS$gen_asc_52, DD$aqpcst_cpafan2);
	os_std(cpafan3, OS$gen_asc_52, DD$aqpcst_cpafan3);
	os_std(cpaasd, OS$gen_asc_52, DD$aqpcst_cpaasd);
	os_std(&PCST.cpafansts_1, OS$gen_short, DD$aqpcst_cpafansts);
	os_std(&PCST.cpaairsts_1, OS$gen_short, DD$aqpcst_cpaairsts);
    if( (PCST.stcreg_1 >> 2 & 0x3) == 3) {
	sprintf(cpbfan1, "CPB FAN 1 TEMP (DEG C.) = %c%3d.%02d",
			stat_cpbfan1&0x8000?'-':'+',
			stat_cpbfan1>>8&0x7f, stat_cpbfan1&0xff);
	sprintf(cpbfan2, "CPB FAN 2 TEMP (DEG C.) = %c%3d.%02d",
			stat_cpbfan2&0x8000?'-':'+',
			stat_cpbfan2>>8&0x7f, stat_cpbfan2&0xff);
	sprintf(cpbfan3, "CPB FAN 3 TEMP (DEG C.) = %c%3d.%02d",
			stat_cpbfan3&0x8000?'-':'+',
			stat_cpbfan3>>8&0x7f, stat_cpbfan3&0xff);
	if( (PCST.cpbasd&0xff) == 0xff)
		sprintf(cpbasd, "NO ASD PENDING FOR CPB CAB");
	else
		sprintf(cpbasd, "CPB CAB ASD in %d SEC", PCST.cpbasd * 5);
	os_std(cpbfan1, OS$gen_asc_52, DD$aqpcst_cpbfan1);
	os_std(cpbfan2, OS$gen_asc_52, DD$aqpcst_cpbfan2);
	os_std(cpbfan3, OS$gen_asc_52, DD$aqpcst_cpbfan3);
	os_std(cpbasd, OS$gen_asc_52, DD$aqpcst_cpbasd);
	os_std(&PCST.cpbfansts, OS$gen_short, DD$aqpcst_cpbfansts);
	os_std(&PCST.cpbairsts, OS$gen_short, DD$aqpcst_cpbairsts);
    }
	os_std(&PCST.ioastatus, OS$gen_tiny, DD$aqpcst_ioastatus);
	if( (PCST.ioaasd&0xff) == 0xff)
		sprintf(ioaasd, "NO ASD PENDING FOR IOA CAB");
	else
		sprintf(ioaasd, "IOA CAB ASD in %d SEC", PCST.cpbasd * 5);
	os_std(ioaasd, OS$gen_asc_52, DD$aqpcst_ioaasd);
    if( (PCST.stcreg_1 >> 2 & 0x3) == 3) {
	os_std(&PCST.iobstatus, OS$gen_tiny, DD$aqpcst_iobstatus);
	if( (PCST.iobasd&0xff) == 0xff)
		sprintf(iobasd, "NO ASD PENDING FOR IOB CAB");
	else
		sprintf(iobasd, "IOB CAB ASD in %d SEC", PCST.iobasd * 5);
	os_std(iobasd, OS$gen_asc_52, DD$aqpcst_iobasd);
    }
	if( PCST.wcuasts&0x80 ) {
	  sprintf(wcuatempo, "WCUA AIR OUT (DEG C.) = %c%3d.%02d",
			PCST.wcuatempo&0x8000?'-':'+',
			PCST.wcuatempo>>8&0x7f, PCST.wcuatempo&0xff);
	  sprintf(wcuatempi, "WCUA AIR IN (DEG C.) = %c%3d.%02d",
			PCST.wcuatempi&0x8000?'-':'+',
			PCST.wcuatempi>>8&0x7f, PCST.wcuatempi&0xff);
	  os_std(wcuatempo, OS$gen_asc_52, DD$aqpcst_wcuatempo);
	  os_std(wcuatempi, OS$gen_asc_52, DD$aqpcst_wcuatempi);
	  os_std(&PCST.wcuassts, OS$gen_short, DD$aqpcst_wcuassts);
	}
	os_std(&PCST.wcuasts, OS$gen_short, DD$aqpcst_wcuasts);
	if( PCST.wcubsts&0x80 ) {
	  sprintf(wcubtempo, "WCUA AIR OUT (DEG C.) = %c%3d.%02d",
			PCST.wcubtempo&0x8000?'-':'+',
			PCST.wcubtempo>>8&0x7f, PCST.wcubtempo&0xff);
	  sprintf(wcubtempi, "WCUA AIR IN (DEG C.) = %c%3d.%02d",
			PCST.wcubtempi&0x8000?'-':'+',
			PCST.wcubtempi>>8&0x7f, PCST.wcubtempi&0xff);
	  os_std(wcubtempo, OS$gen_asc_52, DD$aqpcst_wcubtempo);
	  os_std(wcubtempi, OS$gen_asc_52, DD$aqpcst_wcubtempi);
	  os_std(&PCST.wcubssts, OS$gen_short, DD$aqpcst_wcubssts);
	}

	os_std(&PCST.wcubsts, OS$gen_short, DD$aqpcst_wcubsts);
	os_std(&PCST.ioacab, OS$gen_short, DD$aqpcst_ioacab);
	os_std(&PCST.iobcab, OS$gen_short, DD$aqpcst_iobcab);
	os_std(&PCST.ioabi, OS$gen_short, DD$aqpcst_ioabi);
	os_std(&PCST.iobbi, OS$gen_short, DD$aqpcst_iobbi);
	os_std(&PCST.ioaac, OS$gen_short, DD$aqpcst_ioaac);
	os_std(&PCST.iobac, OS$gen_short, DD$aqpcst_iobac);
	os_std(&PCST.ioagc, OS$gen_short, DD$aqpcst_ioagc);
	os_std(&PCST.iobgc, OS$gen_short, DD$aqpcst_iobgc);
	os_std(&PCST.pcssts, OS$gen_short, DD$aqpcst_pcssts);

	os_std(&PCST.busareg, OS$gen_short, DD$aqpcst_busareg);
	sprintf(busavbus, "BUS A VOLTAGE = %c%3d.%03d",
			PCST.busavbus&0x8000?'-':'+',
			PCST.busavbus>>10&0x1f, PCST.busavbus&0x3ff);
	os_std(busavbus, OS$gen_asc_52, DD$aqpcst_busavbus);
	os_std(&PCST.busareg0, OS$gen_short, DD$aqpcst_busareg0);
	os_std(&PCST.busareg1, OS$gen_short, DD$aqpcst_busareg1);
	os_std(&PCST.busareg2, OS$gen_short, DD$aqpcst_busareg2);
	os_std(&PCST.busareg3, OS$gen_short, DD$aqpcst_busareg3);
	os_std(&PCST.busareg4, OS$gen_short, DD$aqpcst_busareg4);
	os_std(&PCST.busasts, OS$gen_short, DD$aqpcst_busasts);
	os_std(&PCST.busbreg, OS$gen_short, DD$aqpcst_busbreg);
	sprintf(busbvbus, "BUS B VOLTAGE = %c%3d.%03d",
			PCST.busbvbus&0x8000?'-':'+',
			PCST.busbvbus>>10&0x1f, PCST.busbvbus&0x3ff);
	os_std(busbvbus, OS$gen_asc_52, DD$aqpcst_busbvbus);
	os_std(&PCST.busbreg0, OS$gen_short, DD$aqpcst_busbreg0);
	os_std(&PCST.busbreg1, OS$gen_short, DD$aqpcst_busbreg1);
	os_std(&PCST.busbreg2, OS$gen_short, DD$aqpcst_busbreg2);
	os_std(&PCST.busbreg3, OS$gen_short, DD$aqpcst_busbreg3);
	os_std(&PCST.busbreg4, OS$gen_short, DD$aqpcst_busbreg4);
	os_std(&PCST.busbsts, OS$gen_short, DD$aqpcst_busbsts);
	os_std(&PCST.buscreg, OS$gen_short, DD$aqpcst_buscreg);
	sprintf(buscvbus, "BUS C VOLTAGE = %c%3d.%03d",
			PCST.buscvbus&0x8000?'-':'+',
			PCST.buscvbus>>10&0x1f, PCST.buscvbus&0x3ff);
	os_std(buscvbus, OS$gen_asc_52, DD$aqpcst_buscvbus);
	os_std(&PCST.buscreg0, OS$gen_short, DD$aqpcst_buscreg0);
	os_std(&PCST.buscreg1, OS$gen_short, DD$aqpcst_buscreg1);
	os_std(&PCST.buscreg2, OS$gen_short, DD$aqpcst_buscreg2);
	os_std(&PCST.buscreg3, OS$gen_short, DD$aqpcst_buscreg3);
	os_std(&PCST.buscreg4, OS$gen_short, DD$aqpcst_buscreg4);
	os_std(&PCST.buscsts, OS$gen_short, DD$aqpcst_buscsts);
	os_std(&PCST.busdreg, OS$gen_short, DD$aqpcst_busdreg);
	sprintf(busdvbus, "BUS D VOLTAGE = %c%3d.%03d",
			PCST.busdvbus&0x8000?'-':'+',
			PCST.busdvbus>>10&0x1f, PCST.busdvbus&0x3ff);
	os_std(busdvbus, OS$gen_asc_52, DD$aqpcst_busdvbus);
	os_std(&PCST.busdreg0, OS$gen_short, DD$aqpcst_busdreg0);
	os_std(&PCST.busdreg1, OS$gen_short, DD$aqpcst_busdreg1);
	os_std(&PCST.busdreg2, OS$gen_short, DD$aqpcst_busdreg2);
	os_std(&PCST.busdreg3, OS$gen_short, DD$aqpcst_busdreg3);
	os_std(&PCST.busdreg4, OS$gen_short, DD$aqpcst_busdreg4);
	os_std(&PCST.busdsts, OS$gen_short, DD$aqpcst_busdsts);

  if( (stat_stcreg >> 10 & 0xf) == 1) {
	/* aridus, tables 1 and 2 */
    if( (stat_stcreg >> 2 & 0x3) == 3) {
	/* table 2 */
	os_std(&PCST.tab2or4.tab2.busereg, OS$gen_short, DD$aqpcst_busereg);
	sprintf(busevbus, "BUS E VOLTAGE = %c%3d.%03d",
			PCST.tab2or4.tab2.busevbus&0x8000?'-':'+',
			PCST.tab2or4.tab2.busevbus>>10&0x1f, PCST.tab2or4.tab2.busevbus&0x3ff);
	os_std(busevbus, OS$gen_asc_52, DD$aqpcst_busevbus);
	os_std(&PCST.tab2or4.tab2.busereg0, OS$gen_short, DD$aqpcst_busereg0);
	os_std(&PCST.tab2or4.tab2.busereg1, OS$gen_short, DD$aqpcst_busereg1);
	os_std(&PCST.tab2or4.tab2.busereg2, OS$gen_short, DD$aqpcst_busereg2);
	os_std(&PCST.tab2or4.tab2.busereg3, OS$gen_short, DD$aqpcst_busereg3);
	os_std(&PCST.tab2or4.tab2.busereg4, OS$gen_short, DD$aqpcst_busereg4);
	os_std(&PCST.tab2or4.tab2.busests, OS$gen_short, DD$aqpcst_busests);
	os_std(&PCST.tab2or4.tab2.busfreg, OS$gen_short, DD$aqpcst_busfreg);
	sprintf(busfvbus, "BUS F VOLTAGE = %c%3d.%03d",
			PCST.tab2or4.tab2.busfvbus&0x8000?'-':'+',
			PCST.tab2or4.tab2.busfvbus>>10&0x1f, PCST.tab2or4.tab2.busfvbus&0x3ff);
	os_std(busfvbus, OS$gen_asc_52, DD$aqpcst_busfvbus);
	os_std(&PCST.tab2or4.tab2.busfreg0, OS$gen_short, DD$aqpcst_busfreg0);
	os_std(&PCST.tab2or4.tab2.busfreg1, OS$gen_short, DD$aqpcst_busfreg1);
	os_std(&PCST.tab2or4.tab2.busfreg2, OS$gen_short, DD$aqpcst_busfreg2);
	os_std(&PCST.tab2or4.tab2.busfreg3, OS$gen_short, DD$aqpcst_busfreg3);
	os_std(&PCST.tab2or4.tab2.busfreg4, OS$gen_short, DD$aqpcst_busfreg4);
	os_std(&PCST.tab2or4.tab2.busfsts, OS$gen_short, DD$aqpcst_busfsts);
    }
  } else if( (stat_stcreg >> 10 & 0xf) == 0) {
	/* aquarius, tables 3 and 4 */
	/* (table 3 is just regj and regk of table 4) */
	os_std(&PCST.tab2or4.tab4.busjreg, OS$gen_short, DD$aqpcst_busjreg);
	sprintf(busjvbus, "BUS J VOLTAGE = %c%3d.%03d",
			PCST.tab2or4.tab4.busjvbus&0x8000?'-':'+',
			PCST.tab2or4.tab4.busjvbus>>10&0x1f, PCST.tab2or4.tab4.busjvbus&0x3ff);
	os_std(busjvbus, OS$gen_asc_52, DD$aqpcst_busjvbus);
	os_std(&PCST.tab2or4.tab4.busjreg0, OS$gen_short, DD$aqpcst_busjreg0);
	os_std(&PCST.tab2or4.tab4.busjreg1, OS$gen_short, DD$aqpcst_busjreg1);
	os_std(&PCST.tab2or4.tab4.busjreg2, OS$gen_short, DD$aqpcst_busjreg2);
	os_std(&PCST.tab2or4.tab4.busjreg3, OS$gen_short, DD$aqpcst_busjreg3);
	os_std(&PCST.tab2or4.tab4.busjreg4, OS$gen_short, DD$aqpcst_busjreg4);
	os_std(&PCST.tab2or4.tab4.busjsts, OS$gen_short, DD$aqpcst_busjsts);
	os_std(&PCST.tab2or4.tab4.buskreg, OS$gen_short, DD$aqpcst_buskreg);
	sprintf(buskvbus, "BUS K VOLTAGE = %c%3d.%03d",
			PCST.tab2or4.tab4.buskvbus&0x8000?'-':'+',
			PCST.tab2or4.tab4.buskvbus>>10&0x1f, PCST.tab2or4.tab4.buskvbus&0x3ff);
	os_std(buskvbus, OS$gen_asc_52, DD$aqpcst_buskvbus);
	os_std(&PCST.tab2or4.tab4.buskreg0, OS$gen_short, DD$aqpcst_buskreg0);
	os_std(&PCST.tab2or4.tab4.buskreg1, OS$gen_short, DD$aqpcst_buskreg1);
	os_std(&PCST.tab2or4.tab4.buskreg2, OS$gen_short, DD$aqpcst_buskreg2);
	os_std(&PCST.tab2or4.tab4.buskreg3, OS$gen_short, DD$aqpcst_buskreg3);
	os_std(&PCST.tab2or4.tab4.buskreg4, OS$gen_short, DD$aqpcst_buskreg4);
	os_std(&PCST.tab2or4.tab4.busksts, OS$gen_short, DD$aqpcst_busksts);
    if( (stat_stcreg >> 2 & 0x3) == 3) {
	/* table 4 */
	/* The first part of table 4 is done just above this. 
           Here is the rest of it. */
	os_std(&PCST.tab2or4.tab4.busmreg, OS$gen_short, DD$aqpcst_busmreg);
	sprintf(busmvbus, "BUS M VOLTAGE = %c%3d.%03d",
			PCST.tab2or4.tab4.busmvbus&0x8000?'-':'+',
			PCST.tab2or4.tab4.busmvbus>>10&0x1f, PCST.tab2or4.tab4.busmvbus&0x3ff);
	os_std(busmvbus, OS$gen_asc_52, DD$aqpcst_busmvbus);
	os_std(&PCST.tab2or4.tab4.busmreg0, OS$gen_short, DD$aqpcst_busmreg0);
	os_std(&PCST.tab2or4.tab4.busmreg1, OS$gen_short, DD$aqpcst_busmreg1);
	os_std(&PCST.tab2or4.tab4.busmreg2, OS$gen_short, DD$aqpcst_busmreg2);
	os_std(&PCST.tab2or4.tab4.busmreg3, OS$gen_short, DD$aqpcst_busmreg3);
	os_std(&PCST.tab2or4.tab4.busmreg4, OS$gen_short, DD$aqpcst_busmreg4);
	os_std(&PCST.tab2or4.tab4.busmsts, OS$gen_short, DD$aqpcst_busmsts);
	os_std(&PCST.tab2or4.tab4.busnreg, OS$gen_short, DD$aqpcst_busnreg);
	sprintf(busnvbus, "BUS N VOLTAGE = %c%3d.%03d",
			PCST.tab2or4.tab4.busnvbus&0x8000?'-':'+',
			PCST.tab2or4.tab4.busnvbus>>10&0x1f, PCST.tab2or4.tab4.busnvbus&0x3ff);
	os_std(busnvbus, OS$gen_asc_52, DD$aqpcst_busnvbus);
	os_std(&PCST.tab2or4.tab4.busnreg0, OS$gen_short, DD$aqpcst_busnreg0);
	os_std(&PCST.tab2or4.tab4.busnreg1, OS$gen_short, DD$aqpcst_busnreg1);
	os_std(&PCST.tab2or4.tab4.busnreg2, OS$gen_short, DD$aqpcst_busnreg2);
	os_std(&PCST.tab2or4.tab4.busnreg3, OS$gen_short, DD$aqpcst_busnreg3);
	os_std(&PCST.tab2or4.tab4.busnreg4, OS$gen_short, DD$aqpcst_busnreg4);
	os_std(&PCST.tab2or4.tab4.busnsts, OS$gen_short, DD$aqpcst_busnsts);
    }
  }
 } else if( PCS.format_byte == 0 ) {
		/* Power System */
		/* exception information */
	cds_ptr->subtype = DD$AQPCS_CDS;
	if (ini_seg(cds_ptr) == EI$FAIL)
	    status = EI$FAIL;
	ads_ptr->subtype = DD$AQPCS_ADS;
	if (ini_seg(ads_ptr) == EI$FAIL)
	    status = EI$FAIL;
	sis_ptr->subtype = DD$AQ_PCS_SIS;
	if (ini_seg(sis_ptr) == EI$FAIL)
	    status = EI$FAIL;
		/* This format byte differentiates between pcs exception
			and the pcs status information.  We could output this value,
			but it might be confused with the 7 'format types' of
			the pcs exception entry (which we also do not output.) */
	/* os_std(&PCS.format_byte, OS$gen_tiny,		DD$aqpcs_format_byte); */
	os_std(&PCS.idreg, OS$gen_tiny,		DD$aqpcs_idreg);
	os_std(&PCS.idreg, OS$gen_tiny,		DD$sum_idreg);
	os_std(&PCS.xcpid, OS$gen_tiny,		DD$aqpcs_xcpid_ric);
	os_std(&PCS.xcpid, OS$gen_tiny,		DD$sum_xcpid);
	os_std(&PCS.severity, OS$gen_tiny,		DD$aqpcs_severity);

		/* determine format type */
	format_type = 0;
	xcpid = elrp->el_body.el_aqpcs.xcpid;
	if( elrp->el_body.el_aqpcs.idreg ) {
		if( xcpid >= 0 && xcpid <= 2 )
			format_type = 1;
		if( xcpid >= 4 && xcpid <= 5 )
			format_type = 2;
		if( xcpid >= 0xc && xcpid <= 0x1a )
			format_type = 3;
		if( xcpid == 3 || xcpid == 6 )
			format_type = 3;
	} else {
		if( xcpid >= 0 && xcpid <= 5 )
			format_type = 4;
		if( xcpid >= 6 && xcpid <= 7 )
			format_type = 5;
		if( xcpid >= 8 )
			format_type = 6;
		if( xcpid >= 9 )
			format_type = 7;
	}

  if( format_type == 1 || format_type == 2 || format_type == 3 ) {
    /* rsreg */
	os_std(&PCS.ricpem.ric.rsreg, OS$gen_short,		DD$aqpcs_rsreg);
	clear_conditions();
	cnd[1] = elrp->el_body.el_aqpcs.idreg > 0;
	cnd[2] = elrp->el_body.el_aqpcs.xcpid == 0;
	cnd[3] = elrp->el_body.el_aqpcs.xcpid == 1;
	cnd[4] = elrp->el_body.el_aqpcs.xcpid == 2;
	cnd[5] = elrp->el_body.el_aqpcs.xcpid == 3;
	cnd[6] = elrp->el_body.el_aqpcs.xcpid == 4;
	cnd[7] = elrp->el_body.el_aqpcs.xcpid == 5;
	cnd[8] = elrp->el_body.el_aqpcs.xcpid == 6;
	cnd[9] = elrp->el_body.el_aqpcs.xcpid == 0x0c;
	cnd[10] = elrp->el_body.el_aqpcs.xcpid == 0x0d;
	cnd[11] = elrp->el_body.el_aqpcs.xcpid == 0x0e;
	cnd[12] = elrp->el_body.el_aqpcs.xcpid == 0x0f;



	cnd[16] = elrp->el_body.el_aqpcs.xcpid == 0x13;
	cnd[17] = elrp->el_body.el_aqpcs.xcpid == 0x14;
	cnd[18] = elrp->el_body.el_aqpcs.xcpid == 0x15;

	cnd[20] = elrp->el_body.el_aqpcs.xcpid == 0x17;
	cnd[21] = elrp->el_body.el_aqpcs.xcpid == 0x18;
	cnd[22] = elrp->el_body.el_aqpcs.xcpid == 0x19;
	cnd[23] = elrp->el_body.el_aqpcs.xcpid == 0x1a;

	cnd[24] = elrp->el_body.el_aqpcs.ricpem.ric.rcreg & 0x01;
	cnd[25] = elrp->el_body.el_aqpcs.ricpem.ric.rcreg & 0x02;
	cnd[26] = elrp->el_body.el_aqpcs.ricpem.ric.rcreg & 0x04;
	cnd[27] = elrp->el_body.el_aqpcs.ricpem.ric.rcreg & 0x08;
	cnd[28] = elrp->el_body.el_aqpcs.ricpem.ric.rcreg & 0x10;

	cnd[29] = elrp->el_body.el_aqpcs.idreg == 0x11;
	cnd[30] = elrp->el_body.el_aqpcs.idreg == 0x12;
	cnd[31] = elrp->el_body.el_aqpcs.idreg == 0x13;
	cnd[32] = elrp->el_body.el_aqpcs.idreg == 0x14;
	cnd[33] = elrp->el_body.el_aqpcs.idreg == 0x16;
	cnd[34] = elrp->el_body.el_aqpcs.idreg == 0x21;
	cnd[35] = elrp->el_body.el_aqpcs.idreg == 0x22;
	cnd[36] = elrp->el_body.el_aqpcs.idreg == 0x23;
	cnd[37] = elrp->el_body.el_aqpcs.idreg == 0x24;
	cnd[38] = elrp->el_body.el_aqpcs.idreg == 0x25;
	cnd[39] = elrp->el_body.el_aqpcs.idreg == 0x31;
	cnd[40] = elrp->el_body.el_aqpcs.idreg == 0x32;
	cnd[41] = elrp->el_body.el_aqpcs.idreg == 0x41;
	cnd[42] = elrp->el_body.el_aqpcs.idreg == 0x42;
	cnd[43] = elrp->el_body.el_aqpcs.idreg == 0x51;
	cnd[44] = elrp->el_body.el_aqpcs.idreg == 0x52;
	cnd[45] = elrp->el_body.el_aqpcs.idreg == 0x53;
	cnd[46] = elrp->el_body.el_aqpcs.idreg == 0x54;
	cnd[48]  = elrp->el_body.el_aqpcs.idreg == 0x14;
	cnd[48] |= elrp->el_body.el_aqpcs.idreg == 0x15;
	cnd[48] |= elrp->el_body.el_aqpcs.idreg == 0x24;
	cnd[48] |= elrp->el_body.el_aqpcs.idreg == 0x25;
	cnd[48] |= elrp->el_body.el_aqpcs.idreg == 0x32;
	cnd[48] |= elrp->el_body.el_aqpcs.idreg == 0x43;

	clear_sips();
	sip[0] = DD$aqpcs_rsreg_o1;
	sip[1] = DD$aqpcs_rsreg_o2;
	sip[2] = DD$aqpcs_rsreg_o3;
	sip[3] = DD$aqpcs_rsreg_o4;
	sip[4] = DD$aqpcs_rsreg_o5;
	sip[5] = DD$aqpcs_rsreg_o6;
	sip[6] = DD$aqpcs_rsreg_o7;
	sip[7] = DD$aqpcs_rsreg_o8;
	sip[8] = 0;

	value = elrp->el_body.el_aqpcs.ricpem.ric.rsreg;
	qualified_translate(value & 0xf, qt_rsreg_old);

	clear_sips();
	sip[0] = DD$aqpcs_rsreg_n1;
	sip[1] = DD$aqpcs_rsreg_n2;
	sip[2] = DD$aqpcs_rsreg_n3;
	sip[3] = DD$aqpcs_rsreg_n4;
	sip[4] = DD$aqpcs_rsreg_n5;
	sip[5] = DD$aqpcs_rsreg_n6;
	sip[6] = DD$aqpcs_rsreg_n7;
	sip[7] = DD$aqpcs_rsreg_n8;
	sip[8] = 0;
	qualified_translate(value>>8 & 0xf, qt_rsreg_new);

#define PCS elrp->el_body.el_aqpcs
  /* asdreg - automatic shutdown register */
	if( PCS.idreg > 0 ) {
		uc = PCS.ricpem.ric.asdreg;
		if( uc == 0xff )
			sprintf(asdreg_str1, "NO AUTOMATIC SHUTDOWN PENDING" );
		else
			sprintf(asdreg_str1, "AUTOMATIC SHUTDOWN IN %4d SEC", 5*uc );
		os_std( &uc, OS$gen_tiny, DD$aqpcs_asdreg );
		os_std( asdreg_str1, OS$gen_asc_52, DD$aqpcs_asdreg_1 );
	} else {
							/* can use 32 for the 17 bytes we need to move. */
							/* It moves the number required by pasdrsb. */
	  os_std( &PCS.ricpem.pem.pasdrsb[0], OS$gen_bv_32, DD$aqpcs_pasdrsb );
	  uc = PCS.ricpem.pem.pasdrsb[0];
	  if( uc == 0xff )
			sprintf(asdreg_str1, "NO AUTOMATIC SHUTDOWNS PENDING" );
	  else
			sprintf(asdreg_str1, "SOONEST ASD PENDING  %4d SEC", 5*uc );
	  os_std( asdreg_str1, OS$gen_asc_52, DD$aqpcs_asdreg_1 );
	  if( (pem_stcreg>>10&0xf) == 0 ) { /* aquarius system type */
		uc = PCS.ricpem.pem.pasdrsb[1];
		if( uc == 0xff )
			sprintf(asdreg_str2, "94XX RICBUS A - NO ASD PENDING" );
		else
			sprintf(asdreg_str2, "94XX RICBUS A ASD in %4d SEC", 5*uc );
		os_std( asdreg_str2, OS$gen_asc_52, DD$aqpcs_asdreg_2 );
		uc = PCS.ricpem.pem.pasdrsb[2];
		if( uc == 0xff )
			sprintf(asdreg_str3, "94XX RICBUS B - NO ASD PENDING" );
		else
			sprintf(asdreg_str3, "94XX RICBUS B ASD in %4d SEC", 5*uc );
		os_std( asdreg_str3, OS$gen_asc_52, DD$aqpcs_asdreg_3 );
		uc = PCS.ricpem.pem.pasdrsb[3];
		if( uc == 0xff )
			sprintf(asdreg_str4, "94XX RICBUS C - NO ASD PENDING" );
		else
			sprintf(asdreg_str4, "94XX RICBUS C ASD in %4d SEC", 5*uc );
		os_std( asdreg_str4, OS$gen_asc_52, DD$aqpcs_asdreg_4 );
	  } else if( (pem_stcreg>>10&0xf) == 1 ) { /* aridus system type */
		uc = PCS.ricpem.pem.pasdrsb[4];
		if( uc == 0xff )
			sprintf(asdreg_str2, "92XX RICBUS B - NO ASD PENDING" );
		else
			sprintf(asdreg_str2, "92XX RICBUS B ASD in %4d SEC", 5*uc );
		os_std( asdreg_str2, OS$gen_asc_52, DD$aqpcs_asdreg_2 );
		uc = PCS.ricpem.pem.pasdrsb[5];
		if( uc == 0xff )
			sprintf(asdreg_str3, "92XX RICBUS B2 - NO ASD PENDING" );
		else
			sprintf(asdreg_str3, "92XX RICBUS B2 ASD in %4d SEC", 5*uc );
		os_std( asdreg_str3, OS$gen_asc_52, DD$aqpcs_asdreg_3 );
		uc = PCS.ricpem.pem.pasdrsb[6];
		if( uc == 0xff )
			sprintf(asdreg_str4, "92XX RICBUS A - NO ASD PENDING" );
		else
			sprintf(asdreg_str4, "92XX RICBUS A ASD in %4d SEC", 5*uc );
		os_std( asdreg_str4, OS$gen_asc_52, DD$aqpcs_asdreg_4 );
	  }
	}

  /* Hardware Status Register A */
	os_std(&PCS.ricpem.ric.hwsrega, OS$gen_tiny, DD$aqpcs_hwsrega);

	clear_conditions();
	cnd[1] = elrp->el_body.el_aqpcs.idreg == 0x11;
	cnd[2] = elrp->el_body.el_aqpcs.idreg == 0x12;
	cnd[3] = elrp->el_body.el_aqpcs.idreg == 0x13;
	cnd[4] = elrp->el_body.el_aqpcs.idreg == 0x14;
	cnd[5] = elrp->el_body.el_aqpcs.idreg == 0x15;
	cnd[6] = elrp->el_body.el_aqpcs.idreg == 0x21;
	cnd[7] = elrp->el_body.el_aqpcs.idreg == 0x22;
	cnd[8] = elrp->el_body.el_aqpcs.idreg == 0x23;
	cnd[9] = elrp->el_body.el_aqpcs.idreg == 0x24;
	cnd[10] = elrp->el_body.el_aqpcs.idreg == 0x25;
	cnd[11] = elrp->el_body.el_aqpcs.idreg == 0x31;
	cnd[12] = elrp->el_body.el_aqpcs.idreg == 0x32;
	cnd[13] = elrp->el_body.el_aqpcs.idreg == 0x41;
	cnd[14] = elrp->el_body.el_aqpcs.idreg == 0x42;
	cnd[15] = elrp->el_body.el_aqpcs.idreg == 0x51;
	cnd[16] = elrp->el_body.el_aqpcs.idreg == 0x52;
	cnd[17] = elrp->el_body.el_aqpcs.idreg == 0x53;
	cnd[18] = elrp->el_body.el_aqpcs.idreg == 0x54;

	cnd[19] = elrp->el_body.el_aqpcs.ricpem.ric.rcreg & 0x01;
	cnd[20] = elrp->el_body.el_aqpcs.ricpem.ric.rcreg & 0x02;
	cnd[21] = elrp->el_body.el_aqpcs.ricpem.ric.rcreg & 0x04;
	cnd[22] = elrp->el_body.el_aqpcs.ricpem.ric.rcreg & 0x08;
	cnd[23] = elrp->el_body.el_aqpcs.ricpem.ric.rcreg & 0x10;

	clear_sips();
	sip[0] = DD$aqpcs_hwsrega_1;
	sip[1] = DD$aqpcs_hwsrega_2;
	sip[2] = DD$aqpcs_hwsrega_3;
	sip[3] = DD$aqpcs_hwsrega_4;
	sip[4] = DD$aqpcs_hwsrega_5;
	sip[5] = DD$aqpcs_hwsrega_6;
	sip[6] = DD$aqpcs_hwsrega_7;
	sip[7] = DD$aqpcs_hwsrega_8;
	sip[8] = 0;

	qualified_translate(PCS.ricpem.ric.hwsrega, qt_hwsrega);


  /* Hardware Status Register B */
	os_std(&PCS.ricpem.ric.hwsregb, OS$gen_tiny, DD$aqpcs_hwsregb);

	clear_conditions();
	cnd[1] = elrp->el_body.el_aqpcs.idreg == 0x11;
	cnd[2] = elrp->el_body.el_aqpcs.idreg == 0x12;
	cnd[3] = elrp->el_body.el_aqpcs.idreg == 0x13;
	cnd[4] = elrp->el_body.el_aqpcs.idreg == 0x14;
	cnd[5] = elrp->el_body.el_aqpcs.idreg == 0x15;
	cnd[6] = elrp->el_body.el_aqpcs.idreg == 0x21;
	cnd[7] = elrp->el_body.el_aqpcs.idreg == 0x22;
	cnd[8] = elrp->el_body.el_aqpcs.idreg == 0x23;
	cnd[9] = elrp->el_body.el_aqpcs.idreg == 0x24;
	cnd[10] = elrp->el_body.el_aqpcs.idreg == 0x25;
	cnd[11] = elrp->el_body.el_aqpcs.idreg == 0x31;
	cnd[12] = elrp->el_body.el_aqpcs.idreg == 0x32;
	cnd[13] = elrp->el_body.el_aqpcs.idreg == 0x41;
	cnd[14] = elrp->el_body.el_aqpcs.idreg == 0x42;
	cnd[15] = elrp->el_body.el_aqpcs.idreg == 0x51;
	cnd[16] = elrp->el_body.el_aqpcs.idreg == 0x52;
	cnd[17] = elrp->el_body.el_aqpcs.idreg == 0x53;
	cnd[18] = elrp->el_body.el_aqpcs.idreg == 0x54;

	clear_sips();
	sip[0] = DD$aqpcs_hwsregb_1;
	sip[1] = DD$aqpcs_hwsregb_2;
	sip[2] = DD$aqpcs_hwsregb_3;
	sip[3] = DD$aqpcs_hwsregb_4;
	sip[4] = DD$aqpcs_hwsregb_5;
	sip[5] = DD$aqpcs_hwsregb_6;
	sip[6] = DD$aqpcs_hwsregb_7;
	sip[7] = DD$aqpcs_hwsregb_8;
	sip[8] = 0;

	qualified_translate(PCS.ricpem.ric.hwsregb, qt_hwsregb);


  /* Hardware Status Register C */
	os_std(&PCS.ricpem.ric.hwsregc, OS$gen_tiny, DD$aqpcs_hwsregc);

	clear_conditions();
	cnd[1] = elrp->el_body.el_aqpcs.idreg == 0x51;
	cnd[2] = elrp->el_body.el_aqpcs.idreg == 0x52;
	cnd[3] = elrp->el_body.el_aqpcs.idreg == 0x53;
	cnd[4] = elrp->el_body.el_aqpcs.idreg == 0x54;

	clear_sips();
	sip[0] = DD$aqpcs_hwsregc_1;
	sip[1] = DD$aqpcs_hwsregc_2;
	sip[2] = DD$aqpcs_hwsregc_3;
	sip[3] = DD$aqpcs_hwsregc_4;
	sip[4] = DD$aqpcs_hwsregc_5;
	sip[5] = DD$aqpcs_hwsregc_6;
	sip[6] = DD$aqpcs_hwsregc_7;
	sip[7] = DD$aqpcs_hwsregc_8;
	sip[8] = 0;

	qualified_translate(PCS.ricpem.ric.hwsregc, qt_hwsregc);


  /* Hardware Status Register D */
	os_std(&PCS.ricpem.ric.hwsregd, OS$gen_tiny, DD$aqpcs_hwsregd);

	clear_conditions();
	cnd[1] = elrp->el_body.el_aqpcs.idreg == 0x51;
	cnd[2] = elrp->el_body.el_aqpcs.idreg == 0x52;
	cnd[3] = elrp->el_body.el_aqpcs.idreg == 0x53;
	cnd[4] = elrp->el_body.el_aqpcs.idreg == 0x54;

	clear_sips();
	sip[0] = DD$aqpcs_hwsregd_1;
	sip[1] = DD$aqpcs_hwsregd_2;
	sip[2] = DD$aqpcs_hwsregd_3;
	sip[3] = DD$aqpcs_hwsregd_4;
	sip[4] = DD$aqpcs_hwsregd_5;
	sip[5] = DD$aqpcs_hwsregd_6;
	sip[6] = DD$aqpcs_hwsregd_7;
	sip[7] = DD$aqpcs_hwsregd_8;
	sip[8] = 0;

	qualified_translate(PCS.ricpem.ric.hwsregd, qt_hwsregd);

	os_std(&PCS.ricpem.ric.hwsrege, OS$gen_tiny, DD$aqpcs_hwsrege);
	os_std(&PCS.ricpem.ric.hwsregf, OS$gen_tiny, DD$aqpcs_hwsregf);

  /* Power Fail Register */
	os_std(&PCS.ricpem.ric.pfreg_1, OS$gen_short, DD$aqpcs_pfreg);

	clear_conditions();
	/* cnd[1] follows cnd[20] */
	cnd[2] = (elrp->el_body.el_aqpcs.ricpem.ric.pfreg_1&0x8) == 0x0;
	cnd[3] = elrp->el_body.el_aqpcs.idreg == 0x11;
	cnd[4] = elrp->el_body.el_aqpcs.idreg == 0x12;
	cnd[5] = elrp->el_body.el_aqpcs.idreg == 0x13;
	cnd[6] = elrp->el_body.el_aqpcs.idreg == 0x14;
	cnd[7] = elrp->el_body.el_aqpcs.idreg == 0x15;
	cnd[8] = elrp->el_body.el_aqpcs.idreg == 0x21;
	cnd[9] = elrp->el_body.el_aqpcs.idreg == 0x22;
	cnd[10] = elrp->el_body.el_aqpcs.idreg == 0x23;
	cnd[11] = elrp->el_body.el_aqpcs.idreg == 0x24;
	cnd[12] = elrp->el_body.el_aqpcs.idreg == 0x25;
	cnd[13] = elrp->el_body.el_aqpcs.idreg == 0x31;
	cnd[14] = elrp->el_body.el_aqpcs.idreg == 0x32;
	cnd[15] = elrp->el_body.el_aqpcs.idreg == 0x41;
	cnd[16] = elrp->el_body.el_aqpcs.idreg == 0x42;
	cnd[17] = elrp->el_body.el_aqpcs.idreg == 0x51;
	cnd[18] = elrp->el_body.el_aqpcs.idreg == 0x52;
	cnd[19] = elrp->el_body.el_aqpcs.idreg == 0x53;
	cnd[20] = elrp->el_body.el_aqpcs.idreg == 0x54;
	cnd[1] = (cnd[3] || cnd[4] || cnd[5] || cnd[6] ||
			cnd[7] || cnd[8] || cnd[9] || cnd[10] || cnd[11] ||
			cnd[12] || cnd[13] || cnd[14] || cnd[15] || cnd[16] ) ;

	clear_sips();
	sip[0] = DD$aqpcs_pfreg_1;
	sip[1] = DD$aqpcs_pfreg_2;
	sip[2] = DD$aqpcs_pfreg_3;
	sip[3] = DD$aqpcs_pfreg_4;
	sip[4] = DD$aqpcs_pfreg_5;
	sip[5] = DD$aqpcs_pfreg_6;
	sip[6] = DD$aqpcs_pfreg_7;
	sip[7] = DD$aqpcs_pfreg_8;
	sip[8] = 0;

	qualified_translate(PCS.ricpem.ric.pfreg_1|(PCS.ricpem.ric.pfreg_2<<8),
				qt_pfreg);

		/* scs register */
	os_std(&PCS.ricpem.ric.scsreg, OS$gen_tiny, DD$aqpcs_scsreg);
	if(
		(PCS.idreg >= 0x11 && PCS.idreg <= 0x15) ||
		(PCS.idreg >= 0x21 && PCS.idreg <= 0x25) ||
		PCS.idreg == 0x31 ||
		PCS.idreg == 0x32 ||
		PCS.idreg == 0x41 ||
		PCS.idreg == 0x42 
	)
		os_std(&PCS.ricpem.ric.rcreg, OS$gen_tiny, DD$aqpcs_rcreg);

	if(format_type == 1){
#define PF1 elrp->el_body.el_aqpcs.ricpem.ric.fmt.fmt1
		os_std(&PF1.mgnreg, OS$gen_tiny, DD$aqpcs_mgnreg);

		os_std(&PF1.measure, OS$gen_short, DD$aqpcs_measure);
		os_std(&PF1.low, OS$gen_short, DD$aqpcs_low);
		os_std(&PF1.high, OS$gen_short, DD$aqpcs_high);
		os_std(&PF1.reg0cur, OS$gen_short, DD$aqpcs_reg0cur);
		os_std(&PF1.reg1cur, OS$gen_short, DD$aqpcs_reg1cur);
		os_std(&PF1.reg2cur, OS$gen_short, DD$aqpcs_reg2cur);
		os_std(&PF1.reg3cur, OS$gen_short, DD$aqpcs_reg3cur);
		os_std(&PF1.reg4cur, OS$gen_short, DD$aqpcs_reg4cur);
		unpackval("MEASURED VOLTAGE = %c%3d.%03d",
			PF1.measure, measure_str, DD$aqpcs_measure_1);
		unpackval("LOW LIMIT VOLTAGE = %c%3d.%03d",
			PF1.low, low_str, DD$aqpcs_low_1);
		unpackval("HIGH LIMIT VOLTAGE = %c%3d.%03d",
			PF1.high, high_str, DD$aqpcs_high_1);
		sprintf(reg0cur_str, "REG 0 CURRENT = %4d AMPS", PF1.reg0cur&0x0fff);
		os_std(reg0cur_str, OS$gen_asc_52, DD$aqpcs_reg0cur_1);
		sprintf(reg1cur_str, "REG 1 CURRENT = %4d AMPS", PF1.reg1cur&0x0fff);
		os_std(reg1cur_str, OS$gen_asc_52, DD$aqpcs_reg1cur_1);
		sprintf(reg2cur_str, "REG 2 CURRENT = %4d AMPS", PF1.reg2cur&0x0fff);
		os_std(reg2cur_str, OS$gen_asc_52, DD$aqpcs_reg2cur_1);
		sprintf(reg3cur_str, "REG 3 CURRENT = %4d AMPS", PF1.reg3cur&0x0fff);
		os_std(reg3cur_str, OS$gen_asc_52, DD$aqpcs_reg3cur_1);
		sprintf(reg4cur_str, "REG 4 CURRENT = %4d AMPS", PF1.reg4cur&0x0fff);
		os_std(reg4cur_str, OS$gen_asc_52, DD$aqpcs_reg4cur_1);

	}
	if(format_type == 2){
#define PF2 elrp->el_body.el_aqpcs.ricpem.ric.fmt.fmt2
		os_std(&PF2.tempa_1, OS$gen_short, DD$aqpcs_tempa);
		os_std(&PF2.tempb_1, OS$gen_short, DD$aqpcs_tempb);
		os_std(&PF2.yellowa_1, OS$gen_short, DD$aqpcs_yellowa);
		os_std(&PF2.reda_1, OS$gen_short, DD$aqpcs_reda);
		os_std(&PF2.yellowb_1, OS$gen_short, DD$aqpcs_yellowb);
		os_std(&PF2.redb_1, OS$gen_short, DD$aqpcs_redb);
		os_std(&PF2.open_1, OS$gen_short, DD$aqpcs_open);
		os_std(&PF2.shortsensor_1, OS$gen_short, DD$aqpcs_shortsensor);
		unpackval_b("TEMPERATURE A = %c%3d.%02d DEG C",
			PF2.tempa_1|(PF2.tempa_2<<8),
			tempa_str, DD$aqpcs_tempa_1);
		unpackval_b("TEMPERATURE B = %c%3d.%02d DEG C",
			PF2.tempb_1|(PF2.tempb_2<<8),
			tempb_str, DD$aqpcs_tempb_1);
		unpackval_b("YELLOW ZONE A = %c%3d.%02d DEG C",
			PF2.yellowa_1|(PF2.yellowa_2<<8),
			yellowa_str, DD$aqpcs_yellowa_1);
		unpackval_b("RED ZONE A = %c%3d.%02d DEG C",
			PF2.reda_1|(PF2.reda_2<<8),
			reda_str, DD$aqpcs_reda_1);
		unpackval_b("YELLOW ZONE B = %c%3d.%02d DEG C",
			PF2.yellowb_1|(PF2.yellowb_2<<8),
			yellowb_str, DD$aqpcs_yellowb_1);
		unpackval_b("RED ZONE B = %c%3d.%02d DEG C",
			PF2.redb_1|(PF2.redb_2<<8),
			redb_str, DD$aqpcs_redb_1);
		unpackval_b("OPEN VALUE = %c%3d.%02d DEG C",
			PF2.open_1|(PF2.open_2<<8),
			open_str, DD$aqpcs_open_1);
		unpackval_b("SHORTED VALUE = %c%3d.%02d DEG C",
			PF2.shortsensor_1|(PF2.shortsensor_2<<8),
			shortsensor_str, DD$aqpcs_shortsensor_1);
	}
  } else if( format_type >= 4 && format_type <= 7 ) {
    /* psreg */
	os_std(&PCS.ricpem.pem.psreg, OS$gen_short,		DD$aqpcs_psreg);
	clear_conditions();
	cnd[1] = PCS.idreg == 0;
	cnd[2] = PCS.xcpid == 0;
	cnd[3] = PCS.xcpid == 1;
	cnd[4] = PCS.xcpid == 2;
	cnd[5] = PCS.xcpid == 3;
	cnd[6] = PCS.xcpid == 4;
	cnd[7] = PCS.xcpid == 5;
	cnd[8] = PCS.xcpid == 6;
	cnd[9] = PCS.xcpid == 7;
	cnd[10] = PCS.xcpid == 8;
	cnd[11] = PCS.xcpid == 9;
	cnd[12] = (pem_stcreg>>10&0xf) == 0;
	cnd[13] = (pem_stcreg>>10&0xf) == 1;

	clear_sips();
	sip[0] = DD$aqpcs_psreg_1;
	sip[1] = DD$aqpcs_psreg_2;
	sip[2] = DD$aqpcs_psreg_3;
	sip[3] = DD$aqpcs_psreg_4;
	sip[4] = DD$aqpcs_psreg_5;
	sip[5] = DD$aqpcs_psreg_6;
	sip[6] = DD$aqpcs_psreg_7;
	sip[7] = DD$aqpcs_psreg_8;
	sip[8] = DD$aqpcs_psreg_9;
	sip[9] = DD$aqpcs_psreg_10;
	sip[10] = DD$aqpcs_psreg_11;
	sip[11] = DD$aqpcs_psreg_12;
	sip[12] = DD$aqpcs_psreg_13;
	sip[13] = DD$aqpcs_psreg_14;
	sip[14] = DD$aqpcs_psreg_15;
	sip[15] = DD$aqpcs_psreg_16;
	sip[16] = 0;

	value = elrp->el_body.el_aqpcs.ricpem.pem.psreg;
	qualified_translate(value, qt_psreg);

	if((pem_stcreg>>10&0xf) == 0) {
	  os_std(&PCS.ricpem.pem.pwr1reg, OS$gen_tiny, DD$aqpcs_pwr1reg_a);
	  os_std(&PCS.ricpem.pem.pwr2reg, OS$gen_tiny, DD$aqpcs_pwr2reg_a);
	} else {
	  os_std(&PCS.ricpem.pem.pwr1reg, OS$gen_tiny, DD$aqpcs_pwr1reg_b);
	  os_std(&PCS.ricpem.pem.pwr2reg, OS$gen_tiny, DD$aqpcs_pwr2reg_b);
	}
	os_std(&PCS.ricpem.pem.stcreg_1, OS$gen_short, DD$aqpcs_stcreg);
	os_std(&PCS.ricpem.pem.connreg, OS$gen_tiny, DD$aqpcs_connreg);
#define PF4 elrp->el_body.el_aqpcs.ricpem.pem.fmt.fmt4
#define PF5 elrp->el_body.el_aqpcs.ricpem.pem.fmt.fmt5
#define PF6 elrp->el_body.el_aqpcs.ricpem.pem.fmt.fmt6
#define PF7 elrp->el_body.el_aqpcs.ricpem.pem.fmt.fmt7
	if( format_type == 4 ) {
	  if((pem_stcreg>>10&0xf) == 0) {
		os_std(&PF4.keyabreg, OS$gen_tiny, DD$aqpcs_keyabreg_aq);
		os_std(&PF4.keybcreg, OS$gen_tiny, DD$aqpcs_keybcreg_aq);
	  }
	  if((pem_stcreg>>10&0xf) == 1) {
		os_std(&PF4.keyabreg, OS$gen_tiny, DD$aqpcs_keyabreg_ar);
		os_std(&PF4.keybcreg, OS$gen_tiny, DD$aqpcs_keybcreg_ar);
	  }
	  os_std(&PF4.ricavarsb[0], OS$gen_short, DD$aqpcs_ricavarsb_1);
	  os_std(&PF4.ricavarsb[1], OS$gen_short, DD$aqpcs_ricavarsb_2);
	  os_std(&PF4.ricavarsb[2], OS$gen_short, DD$aqpcs_ricavarsb_3);
	  os_std(&PF4.ricavarsb[3], OS$gen_short, DD$aqpcs_ricavarsb_4);
	  os_std(&PF4.ricavarsb[4], OS$gen_short, DD$aqpcs_ricavarsb_5);
	  os_std(&PF4.ricavarsb[5], OS$gen_short, DD$aqpcs_ricavarsb_6);
	  os_std(&PF4.ricavarsb[6], OS$gen_short, DD$aqpcs_ricavarsb_7);
	  os_std(&PF4.ricavarsb[7], OS$gen_short, DD$aqpcs_ricavarsb_8);
	  os_std(&PF4.ricavarsb[8], OS$gen_short, DD$aqpcs_ricavarsb_9);
	  os_std(&PF4.ricavarsb[9], OS$gen_short, DD$aqpcs_ricavarsb_10);
	  os_std(&PF4.ricavarsb[10], OS$gen_short,DD$aqpcs_ricavarsb_11);
	  os_std(&PF4.ricavarsb[11], OS$gen_short,DD$aqpcs_ricavarsb_12);
	  os_std(&PF4.ricavarsb[12], OS$gen_short,DD$aqpcs_ricavarsb_13);
	  os_std(&PF4.ricavarsb[13], OS$gen_short,DD$aqpcs_ricavarsb_14);
	  os_std(&PF4.ricavarsb[14], OS$gen_short,DD$aqpcs_ricavarsb_15);
	  os_std(&PF4.ricavarsb[15], OS$gen_short,DD$aqpcs_ricavarsb_16);
	  os_std(&PF4.ricavarsb[16], OS$gen_short,DD$aqpcs_ricavarsb_17);
	  os_std(&PF4.ricavarsb[17], OS$gen_short,DD$aqpcs_ricavarsb_18);
	} else if( format_type == 5 ) {
		os_std(&PF5.bbureg, OS$gen_tiny, DD$aqpcs_bbureg);
	} else if( format_type == 6 ) {
		os_std(&PF6.ocpswreg, OS$gen_short, DD$aqpcs_ocpswreg);
		if(PF6.ocpdisreg[0] || PF6.ocpdisreg[1] || PF6.ocpdisreg[2])
		  os_std(&PF6.ocpdisreg[0], OS$gen_long, DD$aqpcs_ocpdisreg);
	} else if( format_type == 7 ) {
	  if((pem_stcreg>>10&0xf) == 0)
		os_std(&PF7.bscreg, OS$gen_tiny, DD$aqpcs_bscreg_aq);
	  else
		os_std(&PF7.bscreg, OS$gen_tiny, DD$aqpcs_bscreg_ar);
	}
  }
 }


  /*   */

	if( status == EI$FAIL )
		return(EI$FAIL);
	else
		return(EI$SUCC);
}

clear_conditions()
{
	int i;
	cnd[0] = 1; /* condition 0 is always true */
	for(i=1; i<=MAX_CONDS; i++)
		cnd[i] = 0;
}

clear_sips()
{
	int i;
	for(i=0; i<=MAX_SIPS; i++)
		sip[i] = 0;
}

qualified_translate(value, qt_table)
unsigned long value;
char *qt_table;
{
	unsigned long v;
	struct qt_entry *qt;
	int next_si = 0; /* index into sip of next std item to use */

	cnd[0] = 1; /* condition 0 is always true */
	for(qt=(struct qt_entry *)qt_table; qt->qcond1 != -1; qt++) {

			/* check qualifying conditions */
		if( !cnd[qt->qcond1] || !cnd[qt->qcond2] || !cnd[qt->qcond3] )
			continue;

			/* get the field */
		v = value;
				/* knock off left bits */
#define BITS_IN_LONG 32
		v = v << (BITS_IN_LONG-1) - qt->field_leftbit;
		v = v >> (BITS_IN_LONG-1) - qt->field_leftbit + qt->field_rightbit;
			/* test field against test value. */
			/* -1 for test_value means any non-zero v matches */
			/* -2 for test_value means any non-0xf v matches */
		if( (v != qt->test_value) &&
		    !(qt->test_value == -1 && v ) &&
		    !(qt->test_value == -2 && v != 0xf ) )
			continue;

			/* use the string */
		if( !sip[next_si] )	/* no standard item pointer? */
			break;
		os_std(qt->translate_string, OS$gen_asc_52, sip[next_si++]);
		if( next_si == MAX_SIPS )
			break;
	}
}

/* unpack a special numeric format using given format
   and given buffer and std_item */
unpackval(fmt, val, buf, std_item)
char *fmt;
unsigned short val;
char *buf;
int std_item;
{
	sprintf(buf, fmt,
		(val&0x8000?'-':'+'), val>>10&0x1f, val&0x3ff);
	os_std(buf, OS$gen_asc_52, std_item);
}

/* unpack, but use slightly different pack format than unpackval */
unpackval_b(fmt, val, buf, std_item)
char *fmt;
unsigned short val;
char *buf;
int std_item;
{
	sprintf(buf, fmt,
		(val&0x8000?'-':'+'), val>>8&0x7f, val&0xff);
	os_std(buf, OS$gen_asc_52, std_item);
}
