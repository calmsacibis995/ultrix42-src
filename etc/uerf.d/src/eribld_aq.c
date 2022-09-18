#ifndef lint
static char sccsid[]  =  "@(#)eribld_aq.c	4.3   (ULTRIX)   12/20/90"; 
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
#include "btliterals.h"

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

struct new_cpusyn {
    u_short format;
    u_short length;
    u_long end_time [2];
    u_long begin_time [2];
    u_char attn_code;
    u_char proc_id; /* Process id                      */
    u_short proc_rev; /* Process Revision          */
    u_short cpu_num; /* Number of CPUs             */
    u_short event_cnt; /* Event count              */
    u_long sid;
    u_long sys_type;
    u_long cpu_id;
    char node_name [16];
    char bcm_dev_name [24];
    long int bcm_rev [3];
    char synd_code [52];
    char pcs_synd_code [52];
    } ;
struct new_iosyn {
    struct new_cpusyn cpu_synerr;
    u_long dev_type;
    char dev_name [32];
    u_long dev_serial_num [2];
    u_long media_type;
    char media_label [12];
    u_long media_serial_num [2];
    u_long mount_time [2];
    } ;

struct new_scan {
	u_long recovery_status;
	union {
		struct {
			u_long ele_number;
			u_long mchkid;
			u_long rev_reg0;
			u_long rev_reg1;
			u_long cdc_reg0;
			u_long cdc_reg1;
			u_long sync_reg0;
			u_long sync_reg1;
			u_long sync_reg2;

			u_long e_errsum_reg;
			u_long e_ctl_reg1;
			u_long e_ctl_reg2;
			u_long e_distr_reg;
			u_long e_retire;
			u_long e_func_1;
			u_long e_func_2;
			u_long e_errdat_reg1;
			u_long e_errdat_reg2;
			u_long e_sts_reg1;
			u_long e_sts_reg2;

			u_long i_fetch_err1;
			u_long i_fetch_err2;
			u_long i_decode_err1;
			u_long i_xbr_decode;
			u_long i_spec_err1;
			u_long i_spec_err2;
			u_long i_fetch_dat1;
			u_long i_fetch_dat2;
			u_long i_fetch_dat3;
			u_long i_fetch_dat4;
			u_long i_fetch_dat5;
			u_long i_fetch_dat6;
			u_long i_fetch_dat7;
			u_long i_fetch_dat8;
			u_long i_fetch_dat9;
			u_long i_decode_data1;
			u_long i_decode_data2;
			u_long i_decode_data3;
			u_long i_decode_data4;
			u_long i_xdata1;
			u_long i_xdata2;
			u_long i_xdata3;
			u_long i_xdata4;
			u_long i_spec_data0;
			u_long i_spec_data1;
			u_long i_spec_data2;
			u_long i_spec_data3;
			u_long i_spec_data4;
			u_long i_spec_data5;
			u_long i_spec_data6;
			u_long i_spec_data7;
			u_long i_spec_data8;
			u_long i_spec_data9;
			u_long i_spec_data10;
			u_long i_spec_data11;
			u_long i_spec_data12;
			u_long i_spec_data13;
			u_long i_spec_data14;
			u_long i_spec_data15;
			u_long i_fetch_sts1;
			u_long i_fetch_sts2;
			u_long i_fetch_sts3;
			u_long m_err_sum;
			u_long m_tb_error;
			u_long m_cache_error0;
			u_long m_cache_error1;
			u_long m_cache_error2;
			u_long m_err_data;
			u_long m_status;
			u_long v_errsum;
			u_long v_err0;
			u_long v_err1;
			u_long v_err2;
			u_long v_err3;
			u_long v_vcta_sts;
			u_long v_vctb_sts;
			u_long v_vctc_sts;
		} cpu;
		struct {
			u_long ele_number;
			u_long mchkid;
			u_long cpu0_scan_num;
			u_long cpu1_scan_num;
			u_long cpu2_scan_num;
			u_long cpu3_scan_num;
			u_long mcu_reg;
			u_long cdc_reg;
			u_long sync_reg0;
			u_long sync_reg1;
			u_long scu_err_sum;
			u_long cpu_cnf;
			u_long scu_cnfg;
			u_long tag_cnfg0;
			u_long tag_cnfg1;
			u_long jbox_reg0;
			u_long jbox_reg1;
			u_long jbox_reg2;
			u_long jbox_reg3;
			u_long jbox_reg4;
			u_long jbox_reg5;
			u_long jbox_reg6;
			u_long jbox_reg7;
			u_long jbox_reg8;
			u_long jbox_reg9;
			u_long jbox_rega;
			u_long jbox_regb;
			u_long icu0_reg0;
			u_long icu0_reg1;
			u_long icu0_reg2;
			u_long icu1_reg0;
			u_long icu1_reg1;
			u_long icu1_reg2;
			u_long acu0_reg0;
			u_long acu0_reg1;
			u_long acu1_reg0;
			u_long acu1_reg1;
			u_long ser80;
			u_long ser81;
			u_long ser82;
			u_long ser83;
			u_long ser84;
			u_long ser85;
			u_long ser86;
			u_long ser87;
			u_long ser88;
			u_long ser89;
			u_long ser8a;
		} scu;
	} cpuscu;
};

struct new_bi {
	u_long dtype;
	union {
		struct {
			u_long sjcs;
			u_long flag;
			u_long cmnd;
			u_long addr;
			u_long datlo;
			u_long dathi;
			u_long dmask;
			u_long dxcs;
			u_long dxspu;
			u_long dxmem;
			u_long dxcnt;
			u_long rxpar;
		} sja;
		struct {
			u_long last_error;
			u_long dtype;
			u_long bicsr;
			u_long ber;
			u_long gpr0;
			u_long gpr1;
			u_long gpr2;
			u_long gpr3;
			u_long sp[8];
			u_long scccsr;
			u_long clkctl;
			u_long dcsr;
			u_long dmao;
			u_long dmai;
			u_long dmam;
			u_long dmae;
			u_long dyrc;
			u_long bci3csr;
			u_long bci3dm;
			u_long bci3bi;
			u_long bci3ii;
			u_long bci3ev;
			u_long uintcsr;
			u_long intrdes;
		} scm;
	} sjascm;
};

struct new_kaf {
	u_short reserved[3];
	u_short create_time[4];
	u_char name[256];
};

struct el_rec_new {
	struct el_rhdr elrhdr;			/* record header */
	struct el_sub_id elsubid;		/* subsystem id packet */
	union {
		struct new_cpusyn el_aqcpusyn;
		struct new_iosyn el_aqiosyn;
		struct new_scan el_aqscan;
		struct new_bi el_aqbi;
		struct new_kaf el_aqkaf;
	} el_body;
	char eltrailer[EL_TRAILERSIZE];			/* asc trailer code  */
};

#define min(a,b) ((b)<(a)?(b):(a))
int scan_ads_num;
int spu_crd_cnt;

int  v9000_bld_spumck(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp)
EIS  *eis_ptr;
DIS  *dis_ptr;
CDS  *cds_ptr;
SDS  *sds_ptr;
ADS  *ads_ptr;
SIS  *sis_ptr;
struct el_rec *elrp;
{
	int status;
		cds_ptr->subtype = DD$MCK9000SPU_CDS;
		if (ini_seg(cds_ptr) == EI$FAIL)
		    status = EI$FAIL;
		sis_ptr->subtype = DD$AQ_SPM_SIS;
		if (ini_seg(sis_ptr) == EI$FAIL)
		    status = EI$FAIL;
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.length,
			OS$gen_long,		DD$mck9000spu_len);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.mchktype,
			OS$gen_long,		DD$mcuv2summ);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.vap,
			OS$gen_long,		DD$mc1vap);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.state,
			OS$gen_long,		DD$mc1int);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.pc,
			OS$gen_long,		DD$pc);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.psl,
			OS$gen_long,		DD$psl);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.smc0,
			OS$gen_long,		DD$mck9000spu_smc0);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.smc1,
			OS$gen_long,		DD$mck9000spu_smc1);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.smc2,
			OS$gen_long,		DD$mck9000spu_smc2);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.smc3,
			OS$gen_long,		DD$mck9000spu_smc3);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.sscbto,
			OS$gen_long,		DD$mck9000spu_sscbto);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.dtype,
			OS$gen_long,		DD$bidevreg);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.bicsr,
			OS$gen_long,		DD$bicsr);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.bicsr,
			OS$gen_long,		DD$sum_bicsr);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.ber,
			OS$gen_long,		DD$ber);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.ber,
			OS$gen_long,		DD$sum_ber);
		os_std(&elrp->el_body.elmck.elmck_frame.el9000spumcf.bci3evnt,
			OS$gen_long,		DD$bci3_csr);
	if( status == EI$FAIL )
		return(EI$FAIL);
	else
		return(EI$SUCC);
}

int  v9000_bld_syndrome(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp_old)
EIS  *eis_ptr;
DIS  *dis_ptr;
CDS  *cds_ptr;
SDS  *sds_ptr;
ADS  *ads_ptr;
SIS  *sis_ptr;
struct el_rec *elrp_old;
{
	int status;
	struct el_rec_new *elrp = (struct el_rec_new *)elrp_old;

		if( elrp->el_body.el_aqcpusyn.format == aqsyn_io )
			cds_ptr->subtype = DD$AQIOSYN_CDS;
		else
			cds_ptr->subtype = DD$AQCPUSYN_CDS;

		if (ini_seg(cds_ptr) == EI$FAIL)
		    status = EI$FAIL;
#define CPUSYN &elrp->el_body.el_aqcpusyn
		os_std(CPUSYN.format, OS$gen_short, DD$aqsyn_format);
		os_std(CPUSYN.attn_code, OS$gen_tiny, DD$aqsyn_attn_code);
		os_std(CPUSYN.end_time[0], OS$gen_long, DD$aqsyn_end_time);
		os_std(CPUSYN.begin_time[0], OS$gen_long, DD$aqsyn_begin_time);
		os_std(CPUSYN.proc_id, OS$gen_tiny, DD$aqsyn_proc_id);
		os_std(CPUSYN.proc_rev, OS$gen_short, DD$aqsyn_proc_rev);
		os_std(CPUSYN.cpu_num, OS$gen_short, DD$aqsyn_cpu_num);
		os_std(CPUSYN.event_cnt, OS$gen_short, DD$aqsyn_event_cnt);
		os_std(CPUSYN.sid, OS$gen_long, DD$aqsyn_sid);
		os_std(CPUSYN.sys_type, OS$gen_long, DD$aqsyn_sys_type);
		os_std(CPUSYN.cpu_id, OS$gen_long, DD$aqsyn_cpu_id);
		os_std(CPUSYN.node_name[0], OS$gen_asc_16, DD$aqsyn_node_name);
		os_std(CPUSYN.bcm_dev_name[0], OS$gen_asc_24, DD$aqsyn_bcm_dev_name);
		os_std(CPUSYN.bcm_rev[0], OS$gen_asc_12, DD$aqsyn_bcm_rev);
		os_std(CPUSYN.synd_code[0], OS$gen_asc_52, DD$aqsyn_synd_code);
		os_std(CPUSYN.pcs_synd_code[0], OS$gen_asc_52, DD$aqsyn_pcs_synd_code);
		if( elrp->el_body.el_aqcpusyn.format == aqsyn_io ) {
#define IOSYN &elrp->el_body.el_aqiosyn
			os_std(IOSYN.dev_type, OS$gen_long, DD$aqsyn_dev_type);
			os_std(IOSYN.dev_name[0], OS$gen_bv_32, DD$aqsyn_dev_name);
			os_std(IOSYN.dev_serial_num[0], OS$gen_bv_8,
											DD$aqsyn_dev_serial_num);
			os_std(IOSYN.media_type, OS$gen_long, DD$aqsyn_media_type);
			os_std(IOSYN.media_label[0], OS$gen_asc_12, DD$aqsyn_media_label);
			os_std(IOSYN.media_serial_num[0], OS$gen_bv_8,
											DD$aqsyn_media_serial_num);
			os_std(IOSYN.mount_time[0], OS$gen_long, DD$aqsyn_mount_time);
		}

	if( status == EI$FAIL )
		return(EI$FAIL);
	else
		return(EI$SUCC);
}

int  v9000_bld_mem(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp)
EIS  *eis_ptr;
DIS  *dis_ptr;
CDS  *cds_ptr;
SDS  *sds_ptr;
ADS  *ads_ptr;
SIS  *sis_ptr;
struct el_rec *elrp;
{
	int status, i, j, k;
	unsigned long ecc, internal1, internal2, internal3;
	unsigned long ext, phys, longw, quadw, mpam, rowcol, row, col;

			/* check rhdr_sid, rhdr_systype to determine if
				spu or host error packet */
			/* sid == 0e, aquarius host,
			   sid == 08, type == 6., aquarius SPU */
	  if((elrp->elrhdr.rhdr_sid>>24) == 0x0e) {
				/* aquarius host */
    	if(elrp->elsubid.subid_ctldevtyp == ELMCNTR_9000_SE)
			cds_ptr->subtype = DD$AQSE_CDS;
		else
			cds_ptr->subtype = DD$AQHE_CDS;
		if (ini_seg(cds_ptr) == EI$FAIL)
		    status = EI$FAIL;
#define HESE &elrp->el_body.el_aqhese
		os_std(HESE.time_stamp[0], OS$gen_long, DD$aqhese_time_stamp);
		os_std(HESE.err_adr, OS$gen_long, DD$aqhese_err_adr);
				/* do address decoding */
		ext = elrp->el_body.el_aqhese.err_adr >> 30;
		phys = elrp->el_body.el_aqhese.err_adr << 2;
		longw = elrp->el_body.el_aqhese.err_adr & 0x1;
		quadw = elrp->el_body.el_aqhese.err_adr & 0xe;
		mpam = (elrp->el_body.el_aqhese.err_adr & 0x30) >> 4 |
				(elrp->el_body.el_aqhese.err_adr & 0xff000000) >> 22;
		rowcol = (elrp->el_body.el_aqhese.err_adr & 0x00ffffc0) >> 6 |
				((elrp->el_body.el_aqhese.ecc_synd & 0x1f800000) >> 5);
		os_std(&ext, OS$gen_long, DD$aqhese_err_adr_ext);
		os_std(&phys, OS$gen_long, DD$aqhese_err_adr_phys);
		os_std(&longw, OS$gen_long, DD$aqhese_err_adr_long);
		os_std(&quadw, OS$gen_long, DD$aqhese_err_adr_quad);
		os_std(&mpam, OS$gen_long, DD$aqhese_err_adr_mpam);
		os_std(&rowcol, OS$gen_long, DD$aqhese_err_adr_rowcol);
		for(row=0,col=0,j=0,k=1,i=0; i<=11; i++) {
			row |= ((rowcol>>j) & 0x1) << i;
			col |= ((rowcol>>k) & 0x1) << i;
			j += 2;
			k += 2;
		}
		os_std(&row, OS$gen_long, DD$aqhese_err_adr_row);
		os_std(&col, OS$gen_long, DD$aqhese_err_adr_col);
		

		os_std(HESE.ecc_synd, OS$gen_long, DD$aqhese_ecc_synd);
				/* interpret ecc syndrome */
#define AQHESE_INT3_SBE 0x1
#define AQHESE_INT3_DBE 0x2
#define AQHESE_INT3_FORCED 0x4
#define AQHESE_ECC_FLAG 0x8000
#define AQHESE_ECC_TYPE 0x4000
#define AQHESE_ECC_BADDATA 0x0080
		ecc = elrp->el_body.el_aqhese.ecc_synd;
		if( ecc & AQHESE_ECC_FLAG ) {
			if( ecc & AQHESE_ECC_TYPE ) {
				if( ecc & AQHESE_ECC_BADDATA ) {
					/* ERROR FORCED DURING MEM WRITE */
					internal3 = AQHESE_INT3_FORCED;
				} else {
					/* DBE ERROR DETECTED */
					internal3 = AQHESE_INT3_DBE;
				}
			} else {
					/* SBE ERROR DETECTED */
					internal3 = AQHESE_INT3_SBE;
			}
			os_std(&internal3, OS$gen_long, DD$aqhese_internal_3);
		}

		os_std(HESE.log_errcnt, OS$gen_long, DD$aqhese_log_errcnt);
		os_std(HESE.cur_errcnt, OS$gen_long, DD$aqhese_cur_errcnt);
		os_std(HESE.cnt_threshld, OS$gen_long, DD$aqhese_cnt_threshld);
		os_std(HESE.valid_cnt, OS$gen_long, DD$aqhese_valid_cnt);
		os_std(HESE.buff_sts, OS$gen_long, DD$aqhese_buff_sts);
		os_std(HESE.size_threshld, OS$gen_long, DD$aqhese_size_threshld);
		os_std(HESE.init_threshld, OS$gen_long, DD$aqhese_init_threshld);
		os_std(HESE.filter_factor, OS$gen_long, DD$aqhese_filter_factor);
		os_std(HESE.inhibit_per, OS$gen_long, DD$aqhese_inhibit_per);
		os_std(HESE.defect_threshld, OS$gen_long, DD$aqhese_defect_threshld);
		os_std(HESE.maint_threshld, OS$gen_long, DD$aqhese_maint_threshld);
		os_std(HESE.defect_list_sz, OS$gen_long, DD$aqhese_defect_list_sz);
	  } else if((elrp->elrhdr.rhdr_sid>>24) == 0x08 && 
				(elrp->elrhdr.rhdr_systype>>24) == 6) {
				/* aquarius SPU */
    	if(elrp->elsubid.subid_ctldevtyp == ELMCNTR_9000_SE) {
#define SPUSE elrp->el_body.el_spuse
			cds_ptr->subtype = DD$AQSPUSE_CDS;
			if (ini_seg(cds_ptr) == EI$FAIL)
	    		status = EI$FAIL;
			ads_ptr->subtype = DD$AQSPUSE_ADS;
			if (ini_seg(ads_ptr) == EI$FAIL)
	    		status = EI$FAIL;
    		        sis_ptr->subtype = DD$AQ_CRD_SIS;
		        if (ini_seg(sis_ptr) == EI$FAIL)
		            status = EI$FAIL;
			os_std(&SPUSE.n_entries, OS$gen_long, DD$aqspuse_nentries);
			if(SPUSE.n_entries) {
				os_std(&SPUSE.smcs[0].smc0, OS$gen_long, DD$mck9000spu_smc0);
				os_std(&SPUSE.smcs[0].smc1, OS$gen_long, DD$mck9000spu_smc1);
				os_std(&SPUSE.smcs[0].smc1, OS$gen_long, DD$sum_smc1);
				os_std(&SPUSE.smcs[0].smc2, OS$gen_long, DD$mck9000spu_smc2);
				os_std(&SPUSE.smcs[0].smc3, OS$gen_long, DD$mck9000spu_smc3);
			}
			spu_crd_cnt = 1;
		} else {
#define SPUHE elrp->el_body.el_spuhe
			cds_ptr->subtype = DD$AQSPUHE_CDS;
			if (ini_seg(cds_ptr) == EI$FAIL)
	    		status = EI$FAIL;
		        sis_ptr->subtype = DD$AQ_CRD_SIS;
		        if (ini_seg(sis_ptr) == EI$FAIL)
		            status = EI$FAIL;
			os_std(&SPUHE.smc0, OS$gen_long, DD$mck9000spu_smc0);
			os_std(&SPUHE.smc1, OS$gen_long, DD$mck9000spu_smc1);
			os_std(&SPUHE.smc1, OS$gen_long, DD$sum_smc1);
			os_std(&SPUHE.smc2, OS$gen_long, DD$mck9000spu_smc2);
			os_std(&SPUHE.smc3, OS$gen_long, DD$mck9000spu_smc3);
		}
	  }
	if( status == EI$FAIL )
		return(EI$FAIL);
	else
		return(EI$SUCC);
}

int  v9000_bld_kaf(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp_old)
EIS  *eis_ptr;
DIS  *dis_ptr;
CDS  *cds_ptr;
SDS  *sds_ptr;
ADS  *ads_ptr;
SIS  *sis_ptr;
struct el_rec *elrp_old;
{
	int status, i;
	int short_i, len;
		/* 32 chars output per line */
#define AQSTRN 32
	static char kaf_name1[AQSTRN+2], kaf_name2[AQSTRN+1], kaf_name3[AQSTRN+1];
	static char kaf_name4[AQSTRN+2];
	static char kaf_name5[AQSTRN+2], kaf_name6[AQSTRN+1], kaf_name7[AQSTRN+1];
	static char kaf_name8[AQSTRN+2];
	struct el_rec_new *elrp = (struct el_rec_new *)elrp_old;

		cds_ptr->subtype = DD$AQKAF_CDS;
		if (ini_seg(cds_ptr) == EI$FAIL)
		    status = EI$FAIL;
		  strncpy(kaf_name1, &elrp->el_body.el_aqkaf.name[0],
					AQSTRN);
		  os_std(kaf_name1, OS$gen_asc_52, DD$aqkaf_1);
		  strncpy(kaf_name2, &elrp->el_body.el_aqkaf.name[AQSTRN],
					AQSTRN);
		  os_std(kaf_name2, OS$gen_asc_52, DD$aqkaf_2);
		  strncpy(kaf_name3, &elrp->el_body.el_aqkaf.name[AQSTRN*2],
					AQSTRN);
		  os_std(kaf_name3, OS$gen_asc_52, DD$aqkaf_3);
		  strncpy(kaf_name4, &elrp->el_body.el_aqkaf.name[AQSTRN*3],
					AQSTRN);
		  os_std(kaf_name4, OS$gen_asc_52, DD$aqkaf_4);
		  strncpy(kaf_name5, &elrp->el_body.el_aqkaf.name[AQSTRN*4],
					AQSTRN);
		  os_std(kaf_name5, OS$gen_asc_52, DD$aqkaf_5);
		  strncpy(kaf_name6, &elrp->el_body.el_aqkaf.name[AQSTRN*5],
					AQSTRN);
		  os_std(kaf_name6, OS$gen_asc_52, DD$aqkaf_6);
		  strncpy(kaf_name7, &elrp->el_body.el_aqkaf.name[AQSTRN*6],
					AQSTRN);
		  os_std(kaf_name7, OS$gen_asc_52, DD$aqkaf_7);
		  strncpy(kaf_name8, &elrp->el_body.el_aqkaf.name[AQSTRN*7],
					AQSTRN);
		  os_std(kaf_name8, OS$gen_asc_52, DD$aqkaf_8);

	if( status == EI$FAIL )
		return(EI$FAIL);
	else
		return(EI$SUCC);
}

int  v9000_bld_bi(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp_old)
EIS  *eis_ptr;
DIS  *dis_ptr;
CDS  *cds_ptr;
SDS  *sds_ptr;
ADS  *ads_ptr;
SIS  *sis_ptr;
struct el_rec *elrp_old;
{
	int status;
	struct el_rec_new *elrp = (struct el_rec_new *)elrp_old;

	  if( elrp->el_body.el_aqbi.dtype == 0x120 ) {
			/* SJA */
#define SJA &elrp->el_body.el_aqbi.sjascm.sja
		cds_ptr->subtype = DD$AQBI_SJA_CDS;
		if (ini_seg(cds_ptr) == EI$FAIL)
		    status = EI$FAIL;
		sis_ptr->subtype = DD$AQ_SJA_SIS;
		if (ini_seg(sis_ptr) == EI$FAIL)
		    status = EI$FAIL;
		os_std(&elrp->el_body.el_aqbi.dtype, OS$gen_long, DD$aqbi_dtype);
		os_std(SJA.sjcs, OS$gen_long, DD$aqbi_sjcs);
		os_std(SJA.sjcs, OS$gen_long, DD$sum_sjcs);
		os_std(SJA.flag, OS$gen_long, DD$aqbi_flag);
		os_std(SJA.flag, OS$gen_long, DD$sum_flag);
		os_std(SJA.cmnd, OS$gen_long, DD$aqbi_cmnd);
		os_std(SJA.addr, OS$gen_long, DD$aqbi_addr);
		os_std(SJA.datlo, OS$gen_long, DD$aqbi_datlo);
		os_std(SJA.dathi, OS$gen_long, DD$aqbi_dathi);
		os_std(SJA.dmask, OS$gen_long, DD$aqbi_dmask);
		os_std(SJA.dxcs, OS$gen_long, DD$aqbi_dxcs);
		os_std(SJA.dxcs, OS$gen_long, DD$sum_dxcs);
		os_std(SJA.dxspu, OS$gen_long, DD$aqbi_dxspu);
		os_std(SJA.dxmem, OS$gen_long, DD$aqbi_dxmem);
		os_std(SJA.dxcnt, OS$gen_long, DD$aqbi_dxcnt);
		os_std(SJA.rxpar, OS$gen_long, DD$aqbi_rxpar);
	  } else if( elrp->el_body.el_aqbi.dtype == 0x121 ) {
			/* SCM */
#define SCM &elrp->el_body.el_aqbi.sjascm.scm
		cds_ptr->subtype = DD$AQBI_SCM_CDS;
		if (ini_seg(cds_ptr) == EI$FAIL)
		    status = EI$FAIL;
		sis_ptr->subtype = DD$AQ_SCM_SIS;
		if (ini_seg(sis_ptr) == EI$FAIL)
		    status = EI$FAIL;
		os_std(&elrp->el_body.el_aqbi.dtype, OS$gen_long, DD$aqbi_dtype);
		os_std(SCM.bicsr, OS$gen_long, DD$bicsr);
		os_std(SCM.bicsr, OS$gen_long, DD$sum_bicsr);
		os_std(SCM.ber, OS$gen_long, DD$ber);
		os_std(SCM.ber, OS$gen_long, DD$sum_ber);
		os_std(SCM.gpr0, OS$gen_long, DD$aqbi_gpr0);
		os_std(SCM.gpr1, OS$gen_long, DD$aqbi_gpr1);
		os_std(SCM.gpr1, OS$gen_long, DD$sum_gpr1);
		os_std(SCM.gpr2, OS$gen_long, DD$aqbi_gpr2);
		os_std(SCM.gpr2, OS$gen_long, DD$sum_gpr2);
		switch( elrp->el_body.el_aqbi.sjascm.scm.gpr2 ) {
			case 0x01:
			case 0x03:
			case 0x04:
				os_std(SCM.gpr3, OS$gen_long, DD$aqbi_gpr3_1);

			case 0x02:
				os_std(SCM.gpr3, OS$gen_long, DD$aqbi_gpr3_2);

			case 0x08:
				os_std(SCM.gpr3, OS$gen_long, DD$aqbi_gpr3_3);

			case 0x09:
			case 0x0a:
			case 0x1a:
				os_std(SCM.gpr3, OS$gen_long, DD$aqbi_gpr3_4);

			case 0x0b:
				os_std(SCM.gpr3, OS$gen_long, DD$aqbi_gpr3_5);

			case 0x0c:
				os_std(SCM.gpr3, OS$gen_long, DD$aqbi_gpr3_6);

			case 0x13:
				os_std(SCM.gpr3, OS$gen_long, DD$aqbi_gpr3_7);

			case 0x16:
				os_std(SCM.gpr3, OS$gen_long, DD$aqbi_gpr3_8);

			case 0x17:
				os_std(SCM.gpr3, OS$gen_long, DD$aqbi_gpr3_9);

			case 0x19:
				os_std(SCM.gpr3, OS$gen_long, DD$aqbi_gpr3_10);

			case 0x1b:
			case 0x1c:
			case 0x1d:
				os_std(SCM.gpr3, OS$gen_long, DD$aqbi_gpr3_11);
		}
		os_std(SCM.sp[8], OS$gen_cntvecl, DD$aqbi_sp);
		os_std(SCM.scccsr, OS$gen_long, DD$aqbi_scccsr);
		os_std(SCM.scccsr, OS$gen_long, DD$sum_scccsr);
		os_std(SCM.clkctl, OS$gen_long, DD$aqbi_clkctl);
		os_std(SCM.clkctl, OS$gen_long, DD$sum_clkctl);
		os_std(SCM.dcsr, OS$gen_long, DD$aqbi_dcsr);
		os_std(SCM.dmao, OS$gen_long, DD$aqbi_dmao);
		os_std(SCM.dmai, OS$gen_long, DD$aqbi_dmai);
		os_std(SCM.dmam, OS$gen_long, DD$aqbi_dmam);
		os_std(SCM.dmae, OS$gen_long, DD$aqbi_dmae);
		os_std(SCM.dyrc, OS$gen_long, DD$aqbi_dyrc);
		os_std(SCM.dyrc, OS$gen_long, DD$sum_dyrc);
		os_std(SCM.bci3csr, OS$gen_long, DD$bci3_csr);
		os_std(SCM.bci3dm, OS$gen_long, DD$aqbi_bci3dm);
		os_std(SCM.bci3bi, OS$gen_long, DD$aqbi_bci3bi);
		os_std(SCM.bci3ii, OS$gen_long, DD$aqbi_bci3ii);
		os_std(SCM.bci3ev, OS$gen_long, DD$aqbi_bci3ev);
		os_std(SCM.last_error, OS$gen_long, DD$aqbi_lasterror);
		os_std(SCM.uintcsr, OS$gen_long, DD$aqbi_uintcsr);
		os_std(SCM.intrdes, OS$gen_long, DD$aqbi_intrdes);
	  }

	if( status == EI$FAIL )
		return(EI$FAIL);
	else
		return(EI$SUCC);
}

int  v9000_bld_clk(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp)
EIS  *eis_ptr;
DIS  *dis_ptr;
CDS  *cds_ptr;
SDS  *sds_ptr;
ADS  *ads_ptr;
SIS  *sis_ptr;
struct el_rec *elrp;
{
	int status;
		cds_ptr->subtype = DD$AQCLK_CDS;
		if (ini_seg(cds_ptr) == EI$FAIL)
		    status = EI$FAIL;
		sis_ptr->subtype = DD$AQ_CLK_SIS;
		if (ini_seg(sis_ptr) == EI$FAIL)
		    status = EI$FAIL;
#define CLK &elrp->el_body.el_aqclk
		os_std(CLK.ccr0, OS$gen_long, DD$aqclk_ccr0);
		os_std(CLK.freq, OS$gen_long, DD$aqclk_freq);
		os_std(CLK.burst, OS$gen_long, DD$aqclk_burst);
		os_std(CLK.interval, OS$gen_long, DD$aqclk_interval);
		os_std(CLK.ccr1, OS$gen_long, DD$aqclk_ccr1);
		os_std(CLK.ccr1, OS$gen_long, DD$sum_ccr1);
		os_std(CLK.position, OS$gen_long, DD$aqclk_position);
		os_std(CLK.divider, OS$gen_long, DD$aqclk_divider);

	if( status == EI$FAIL )
		return(EI$FAIL);
	else
		return(EI$SUCC);
}

int  v9000_bld_scan(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp_old)
EIS  *eis_ptr;
DIS  *dis_ptr;
CDS  *cds_ptr;
SDS  *sds_ptr;
ADS  *ads_ptr;
SIS  *sis_ptr;
struct el_rec *elrp_old;
{
	int status;
	struct el_rec_new *elrp = (struct el_rec_new *)elrp_old;

#define SCANS &elrp->el_body.el_aqscan.cpuscu.scu
	  if( elrp->el_body.el_aqscan.recovery_status&1 ) {
			/* scan scu entry */
		cds_ptr->subtype = DD$AQSCAN_SCU_CDS;
		if (ini_seg(cds_ptr) == EI$FAIL)
		    status = EI$FAIL;
		os_std(&elrp->el_body.el_aqscan.recovery_status, OS$gen_long,
							DD$aqscan_recovery_sts_scu );
		os_std(SCANS.ele_number, OS$gen_long, DD$aqscan_ele_number );
		os_std(SCANS.mchkid, OS$gen_long, DD$aqscan_mchkid );
		os_std(SCANS.cpu0_scan_num, OS$gen_long, DD$aqscan_cpu0_scan_num );
		os_std(SCANS.cpu1_scan_num, OS$gen_long, DD$aqscan_cpu1_scan_num );
		os_std(SCANS.cpu2_scan_num, OS$gen_long, DD$aqscan_cpu2_scan_num );
		os_std(SCANS.cpu3_scan_num, OS$gen_long, DD$aqscan_cpu3_scan_num );
		os_std(SCANS.mcu_reg, OS$gen_long, DD$aqscan_mcu_reg );
		os_std(SCANS.cdc_reg, OS$gen_long, DD$aqscan_cdc_reg );
		os_std(SCANS.sync_reg0, OS$gen_long, DD$aqscan_sync_reg0 );
		os_std(SCANS.sync_reg1, OS$gen_long, DD$aqscan_sync_reg1 );
		os_std(SCANS.scu_err_sum, OS$gen_long, DD$aqscan_scu_err_sum );
		os_std(SCANS.cpu_cnf, OS$gen_long, DD$aqscan_cpu_cnf );
		os_std(SCANS.scu_cnfg, OS$gen_long, DD$aqscan_scu_cnfg );
		os_std(SCANS.tag_cnfg0, OS$gen_long, DD$aqscan_tag_cnfg0 );
		os_std(SCANS.tag_cnfg1, OS$gen_long, DD$aqscan_tag_cnfg1 );
		os_std(SCANS.jbox_reg0, OS$gen_long, DD$aqscan_j_reg0 );
		os_std(SCANS.jbox_reg1, OS$gen_long, DD$aqscan_j_reg1 );
		os_std(SCANS.jbox_reg2, OS$gen_long, DD$aqscan_j_reg2 );
		os_std(SCANS.jbox_reg3, OS$gen_long, DD$aqscan_j_reg3 );
		os_std(SCANS.jbox_reg4, OS$gen_long, DD$aqscan_j_reg4 );
		os_std(SCANS.jbox_reg5, OS$gen_long, DD$aqscan_j_reg5 );
		os_std(SCANS.jbox_reg6, OS$gen_long, DD$aqscan_j_reg6 );
		os_std(SCANS.jbox_reg7, OS$gen_long, DD$aqscan_j_reg7 );
		os_std(SCANS.jbox_reg8, OS$gen_long, DD$aqscan_j_reg8 );
		os_std(SCANS.jbox_reg9, OS$gen_long, DD$aqscan_j_reg9 );
		os_std(SCANS.jbox_rega, OS$gen_long, DD$aqscan_j_rega );
		os_std(SCANS.jbox_regb, OS$gen_long, DD$aqscan_j_regb );
		os_std(SCANS.icu0_reg0, OS$gen_long, DD$aqscan_icu0_reg0 );
		os_std(SCANS.icu0_reg1, OS$gen_long, DD$aqscan_icu0_reg1 );
		os_std(SCANS.icu0_reg2, OS$gen_long, DD$aqscan_icu0_reg2 );
		os_std(SCANS.icu1_reg0, OS$gen_long, DD$aqscan_icu1_reg0 );
		os_std(SCANS.icu1_reg1, OS$gen_long, DD$aqscan_icu1_reg1 );
		os_std(SCANS.icu1_reg2, OS$gen_long, DD$aqscan_icu1_reg2 );
		os_std(SCANS.acu0_reg0, OS$gen_long, DD$aqscan_acu0_reg0 );
		os_std(SCANS.acu0_reg1, OS$gen_long, DD$aqscan_acu0_reg1 );
		os_std(SCANS.acu1_reg0, OS$gen_long, DD$aqscan_acu1_reg0 );
		os_std(SCANS.acu1_reg1, OS$gen_long, DD$aqscan_acu1_reg1 );
		os_std(SCANS.ser80, OS$gen_long, DD$aqscan_ser80 );
		os_std(SCANS.ser81, OS$gen_long, DD$aqscan_ser81 );
		os_std(SCANS.ser82, OS$gen_long, DD$aqscan_ser82 );
		os_std(SCANS.ser83, OS$gen_long, DD$aqscan_ser83 );
		os_std(SCANS.ser84, OS$gen_long, DD$aqscan_ser84 );
		os_std(SCANS.ser85, OS$gen_long, DD$aqscan_ser85 );
		os_std(SCANS.ser86, OS$gen_long, DD$aqscan_ser86 );
		os_std(SCANS.ser87, OS$gen_long, DD$aqscan_ser87 );
		os_std(SCANS.ser88, OS$gen_long, DD$aqscan_ser88 );
		os_std(SCANS.ser89, OS$gen_long, DD$aqscan_ser89 );
		os_std(SCANS.ser8a, OS$gen_long, DD$aqscan_ser8a );
	  } else {
			/* scan cpu entry */
		cds_ptr->subtype = DD$AQSCAN_CPU_CDS;
		if (ini_seg(cds_ptr) == EI$FAIL)
		    status = EI$FAIL;
#define SCANC &elrp->el_body.el_aqscan.cpuscu.cpu
		ads_ptr->subtype = DD$AQSCAN_EBOX_ADS;
		if (ini_seg(ads_ptr) == EI$FAIL)
		    status = EI$FAIL;
		os_std(&elrp->el_body.el_aqscan.recovery_status, OS$gen_long,
							DD$aqscan_recovery_sts_cpu );
		os_std(SCANC.ele_number, OS$gen_long, DD$aqscan_ele_number);
		os_std(SCANC.mchkid, OS$gen_long, DD$aqscan_mchkid);
		os_std(SCANC.rev_reg0, OS$gen_long, DD$aqscan_rev_reg0);
		os_std(SCANC.rev_reg1, OS$gen_long, DD$aqscan_rev_reg1);
		os_std(SCANC.cdc_reg0, OS$gen_long, DD$aqscan_cdc_reg0);
		os_std(SCANC.cdc_reg1, OS$gen_long, DD$aqscan_cdc_reg1);
		os_std(SCANC.sync_reg0, OS$gen_long, DD$aqscan_sync_reg0);
		os_std(SCANC.sync_reg1, OS$gen_long, DD$aqscan_sync_reg1);
		os_std(SCANC.sync_reg2, OS$gen_long, DD$aqscan_sync_reg2);

		os_std(SCANC.e_errsum_reg, OS$gen_long, DD$aqscan_e_errsum_reg);
		os_std(SCANC.e_ctl_reg1, OS$gen_long, DD$aqscan_e_ctl_reg1);
		os_std(SCANC.e_ctl_reg2, OS$gen_long, DD$aqscan_e_ctl_reg2);
		os_std(SCANC.e_distr_reg, OS$gen_long, DD$aqscan_e_distr_reg);
		os_std(SCANC.e_retire, OS$gen_long, DD$aqscan_e_retire);
		os_std(SCANC.e_func_1, OS$gen_long, DD$aqscan_e_func_1);
		os_std(SCANC.e_func_2, OS$gen_long, DD$aqscan_e_func_2);
		os_std(SCANC.e_errdat_reg1, OS$gen_long, DD$aqscan_e_errdat_reg1);
		os_std(SCANC.e_errdat_reg2, OS$gen_long, DD$aqscan_e_errdat_reg2);
		os_std(SCANC.e_sts_reg1, OS$gen_long, DD$aqscan_e_sts_reg1);
		os_std(SCANC.e_sts_reg2, OS$gen_long, DD$aqscan_e_sts_reg2);
		scan_ads_num = 1;	/* start cycling through the scan ads segs */
	}

	if( status == EI$FAIL )
		return(EI$FAIL);
	else
		return(EI$SUCC);
}

int  v9000_bld_config(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp)
EIS  *eis_ptr;
DIS  *dis_ptr;
CDS  *cds_ptr;
SDS  *sds_ptr;
ADS  *ads_ptr;
SIS  *sis_ptr;
struct el_rec *elrp;
{
	int status;
	int component_type, loc;
#define AQL 35
	static char location_str[AQL+2];
	static char type_str[AQL+2];

#define CONFIG elrp->el_body.el_aqconfig
		cds_ptr->subtype = DD$AQCONFIG_CDS;
		if (ini_seg(cds_ptr) == EI$FAIL)
		    status = EI$FAIL;
		component_type = CONFIG.type;
			/* Re: Rod Payne, informs me the following */
			/*     are bi adapter types. */
			/*  SPM==20, SCM==21, AIO==22, AIE==23 */
		if(component_type >= 20 && component_type <=23)
			os_std(&CONFIG.dtype, OS$gen_long, DD$bidevreg);
			/*  The follwing are XMI types */
			/*  XJA==30, XCD==31, XNA==32, XBI==33, KDM==34 */
		else if(component_type >= 30 && component_type <=34)
			os_std(&CONFIG.dtype, OS$gen_long, DD$xdev);
		os_std(&CONFIG.type, OS$gen_tiny, DD$aqconfig_type);
		os_std(&CONFIG.location, OS$gen_short, DD$aqconfig_location);
		sprintf(type_str, "UNKNOWN");
		sprintf(location_str, "");
		loc = CONFIG.location;
		switch(CONFIG.type){
			case 0:
			case 2: case 3: case 4: case 5:
			case 6: case 7: case 8: case 9:
			case 12: case 13: case 14: case 15:
				sprintf(type_str, "RESERVED");
				break;
			case 1:
				sprintf(type_str, "OS REVISION");
				break;
			case 10:
				sprintf(type_str, "UNKNOWN SPU-BI DEVICE");
				sprintf(location_str, "NODE = 0x%x", loc);
				break;
			case 11:
				sprintf(type_str, "UNKNOWN XMI DEVICE");
				sprintf(location_str, "XMI = 0x%x, SLOT = 0x%x",
							loc/16, loc%16);
				break;
			case 16:
				sprintf(type_str, "KERNEL (SID REGISTER)");
				break;
			case 17:
				sprintf(type_str, "SPU MEDIA");
				break;
			case 18:
				sprintf(type_str, "MCU");
				sprintf(location_str, "MCU SEL = 0x%x, UNIT = 0x%x",
							loc>>8&0xff, loc&0xff);
				break;
			case 19:
				sprintf(type_str, "PLANAR MODULE");
				sprintf(location_str, "UNIT = 0x%x", loc);
				break;
			case 20:
				sprintf(type_str, "SPM (T2051)");
				sprintf(location_str, "NODE = 0x%x", loc);
				break;
			case 21:
				sprintf(type_str, "SCM (T2050)");
				sprintf(location_str, "NODE = 0x%x", loc);
				break;
			case 22:
				sprintf(type_str, "AIO (KFBTA)");
				sprintf(location_str, "NODE = 0x%x", loc);
				break;
			case 23:
				sprintf(type_str, "AIE (DEBNT)");
				sprintf(location_str, "NODE = 0x%x", loc);
				break;
			case 24:
				sprintf(type_str, "PEM (T1060)");
				break;
			case 25:
				sprintf(type_str, "CPU RIC (H7388)");
				sprintf(location_str, "RIC = 0x%x", loc);
				break;
			case 26:
				sprintf(type_str, "IO RIC (H7389)");
				sprintf(location_str, "RIC = 0x%x", loc);
				break;
			case 27:
				sprintf(type_str, "MASTER CLOCK MODULE");
				break;
			case 28:
				sprintf(type_str, "MEMORY ARRAY CARDS");
				sprintf(location_str, "MAC = 0x%x", loc);
				break;
			case 29:
				sprintf(type_str, "DAUGHTER ARRAY CARDS");
				sprintf(location_str, "DAC = 0x%x", loc);
				break;
			case 30:
				sprintf(type_str, "XJA ADAPTER (T1061)");
				sprintf(location_str, "XMI = 0x%x, SLOT = 0x%x",
							loc/16, loc%16);
				break;
			case 31:
				sprintf(type_str, "XCD ADAPTER (CIXCD)");
				sprintf(location_str, "XMI = 0x%x, SLOT = 0x%x",
							loc/16, loc%16);
				break;
			case 32:
				sprintf(type_str, "XNA ADAPTER (DEMNA)");
				sprintf(location_str, "XMI = 0x%x, SLOT = 0x%x",
							loc/16, loc%16);
				break;
			case 33:
				sprintf(type_str, "XBI ADAPTER (DWMBB)");
				sprintf(location_str, "XMI = 0x%x, SLOT = 0x%x",
							loc/16, loc%16);
				break;
			case 34:
				sprintf(type_str, "KDM ADAPTER (KDM70)");
				sprintf(location_str, "XMI = 0x%x, SLOT = 0x%x",
							loc/16, loc%16);
				break;
		}
		os_std(type_str, OS$gen_asc_52, DD$aqconfig_type_str);
		os_std(location_str, OS$gen_asc_52, DD$aqconfig_location_str);
		os_std(&CONFIG.status, OS$gen_short, DD$aqconfig_status);
		os_std(&CONFIG.serialno[0], OS$gen_bv_16, DD$aqconfig_serialno);
		os_std(&CONFIG.partid[0], OS$gen_bv_16, DD$aqconfig_partid);

	if( status == EI$FAIL )
		return(EI$FAIL);
	else
		return(EI$SUCC);
}

int  v9000_bld_cte(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr,elrp)
EIS  *eis_ptr;
DIS  *dis_ptr;
CDS  *cds_ptr;
SDS  *sds_ptr;
ADS  *ads_ptr;
SIS  *sis_ptr;
struct el_rec *elrp;
{
	int status;

	if( status == EI$FAIL )
		return(EI$FAIL);
	else
		return(EI$SUCC);
}

