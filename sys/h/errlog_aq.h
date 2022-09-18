/*
 * @(#)errlog_aq.h	4.3	(ULTRIX)	10/10/90
 */


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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 * Modification History: 	errlog_aq.h
 *
 * 20-Sep-90 -- stuarth (Stuart Hollander)
 *	Moved 9000 mach chk structs from ka9000.h to errlog_aq.h
 *
 ************************************************************************/

/* Aquarius error log structures */

#ifndef __ERRLOG_AQ__
#define __ERRLOG_AQ__
#endif /*	__ERRLOG_AQ__ */

#define ELSPU_MC 2
#define ELSPU_SYNDROME 3
#define ELSPU_SE 6
#define ELSPU_HE 8
#define ELSPU_EMM 15
#define ELSPU_HLT 16
#define ELSPU_BIADPERR 18
#define ELSPU_CLKERR 33
#define ELSPU_SCAN 34
#define ELSPU_CONFIG 43
#define ELSPU_LM 100
#define ELSPU_LOGMSCP 101

/* msg 2, MC, machine check from service processor */
/* SPU Machine Check Error Log Entry */
struct el_mc9000spuframe {
u_long	length;		/* Machine Check Stack Frame Length */
u_long	mchktype;	/* Machine Check Type */
u_long	vap;		/* Virtual Address */
u_long	state;		/* uVAX State Register */
u_long	pc;		/* Current uVAX Program Counter (PC) */
u_long	psl;		/* Current uVAX Processor Status Longword (PSL) */
u_long	smc0;		/* SPM Memory Controller CSR Register 0 */
u_long	smc1;		/* SMC CSR Register 1 */
u_long	smc2;		/* SMC CSR Register 2 */
u_long	smc3;		/* SMC CSR Register 3 */
u_long	sscbto;		/* System Support Chip Bus Time Out Register */
u_long	dtype;		/* BIIC Device Type Register */
u_long	bicsr;		/* BIIC Control Status Register */
u_long	ber;		/* BI Bus Error Register */
u_long	bci3evnt;	/* BCI3 Event Status Register */
};


/* Stack frame for vax9000 host machine checks generated by SPU */
struct el_mc9000frame {
	int	mc_length;	/* Length of stack frame in bytes */
	int	mc_type;	/* Type field (ebox or spu) */ 
	int	mc_id;		/* Unique ID (to correlate w/errlog) */ 
	int	mc_err_summ;	/* Error summary */ 
	int	mc_sys_summ;	/* ID of failing subsystem */
	int	mc_vaddr;	/* Failing virtual address, if applicable */
	int	mc_paddr;	/* Failing physical address, if applicable */
	int	mc_misc_info;	/* Miscellaneous info */
	int	mc_pc;		/* PC where error occurred */
	int	mc_psl;		/* PSL at time error occurred */
	int	mc_vpsr;	/* Vector Processor Status Reg. */
};

/* Stack frame for vax9000 host machine checks generated by EBOX */
struct el_mc9000eboxframe {
	int	mc_length;	/* Length of stack frame in bytes */
	int	mc_type;	/* Type field (ebox or spu) */
	int	mc_pc;		/* PC where error occurred */
	int	mc_psl;		/* PSL at time error occurred */
};

struct el_aq_cpusyn {
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
    u_long rsb_ptr;
    } ;
struct el_aq_iosyn {
	struct el_aq_cpusyn cpu_synerr;
    u_long dev_type;
    char dev_name [32];
    u_long dev_serial_num [2];
    u_long media_type;
    char media_label [12];
    u_long media_serial_num [2];
    u_long mount_time [2];
    } ;

struct el_aq_hese {
    u_long time_stamp[2];/* System time of first CRD */
    u_long err_adr;		/* Phys adr of location in err. */
    u_long ecc_synd;		/* Error syndrome and status. */
    u_long log_errcnt;	/* # of errors logged since */
								/* creation of this CRD entry. */
    u_long cur_errcnt;	/* # of errors with same synd */
								/* that occurred since last CRD. */
    u_long cnt_threshld;	/* # of CRD errors that must */
								/* occur since last errlog */
    u_long valid_cnt;	/* Current number of valid CRDs. */
    u_long buff_sts;		/* status of the EHM and CRD   */
								/* Buffer at time of error */
    u_long size_threshld;/* max # of entries that can */
								/* be tracked by the CRD Buffer */
    u_long init_threshld;/* default cnt threshold for */
								/* a new CRD entry */
    u_long filter_factor;/* used to mult count_threshold */
								/* when reason is cnt_thresh_reach */
    u_long inhibit_per;	/* minutes CRD reporting inhibited */
    u_long defect_threshld;/* max # of defected 64 bblocks */
								/* allowed in DEFECT LIST */
    u_long maint_threshld;/* # of defect 64BBLKs in 1 bank */
								/* first warning of possible  */
								/* impending defective bank. */
    u_long defect_list_sz;/* DEFECT LIST size. */
};

struct el_aq_spuhe {
	u_long smc0;
	u_long smc1;
	u_long smc2;
	u_long smc3;
};

struct el_aq_spuse {
	u_long n_entries;
	struct {
		u_long smc0;
		u_long smc1;
		u_long smc2;
		u_long smc3;
	} smcs[16];
};

struct el_aq_pcs {
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
			u_short pfreg;
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
					u_short tempa;
					u_short tempb;
					u_short yellowa;
					u_short reda;
					u_short yellowb;
					u_short redb;
					u_short open;
					u_short shortsensor;
				} fmt2;
			} fmt;
		} ric;
		struct {
			u_short psreg;
			u_char pwr1reg;
			u_char pwr2reg;
			u_char pasdrsb[17];
			u_short stcreg;
			u_char connreg;
			union {
				struct {
					u_char keyareg;
					u_char keybreg;
					u_short ricavarsb[18];
				} fmt4;
				struct {
					u_char bbureg;
				} fmt5;
				struct {
					u_short ocpswreg;
					u_short ocpdisreg;
				} fmt6;
				struct {
					u_char bcsreg;
				} fmt7;
			} fmt;
		} pem;
	} ricpem;
};

struct el_aq_pcsstat {
	u_char format_byte;
	u_short stcreg;
	u_short ambient;
	u_short scufan1;
	u_short scufan2;
	u_short scufan3;
	u_char scuasd;
	u_short scufansts;
	u_short scuairsts;
	u_short cpafan1;
	u_short cpafan2;
	u_short cpafan3;
	u_char cpaasd;
	u_short cpafansts;
	u_short cpaairsts;
	u_short cpbfan1;
	u_short cpbfan2;
	u_short cpbfan3;
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
		u_short ioacab;
		u_short iobcab;
		u_short ioabi;
		u_short iobbi;
		u_short ioaac;
		u_short iobac;
		u_short ioagc;
		u_short iobgc;
		u_short pcssts;
	  } tab1;
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
		u_short ioacab;
		u_short iobcab;
		u_short ioabi;
		u_short iobbi;
		u_short ioaac;
		u_short iobac;
		u_short ioagc;
		u_short iobgc;
		u_short pcssts;
	  } tab2;
	} tab1or2;
};

struct el_aq_scan {
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
			u_long scan_num;
			u_long cpu0_scan_num;
			u_long cpu1_scan_num;
			u_long cpu2_scan_num;
			u_long cpu3_scan_num;
			u_long mchkid;
			u_long cdc_reg;
			u_long sync_reg0;
			u_long sync_reg1;
			u_long jbox_reg0;
			u_long jbox_reg1;
			u_long jbox_reg2;
			u_long jbox_reg3;
			u_long jbox_reg4;
			u_long jbox_reg5;
			u_long icu0_reg0;
			u_long icu0_reg1;
			u_long icu1_reg0;
			u_long icu1_reg1;
			u_long acu0_reg0;
			u_long acu1_reg0;
		} scu;
	} cpuscu;
};

struct el_aq_bi {
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
			u_long last_error;
			u_long uintcsr;
			u_long intrdes;
		} scm;
	} sjascm;
};

struct el_aq_kaf {
	u_long reserved[3];
	u_long create_time[2];
	u_char name_len;
	u_char name[255];
};

struct el_aq_clk {
	u_long ccr0;
	u_long freq;
	u_long burst;
	u_long interval;
	u_long ccr1;
	u_long position;
	u_long divider;
};

struct el_aq_config {
	u_long dtype;
	u_char type;
	u_char status;
	u_short location;
	u_short reserved;
	u_char serialno[10];
	u_char partid[16];
};
