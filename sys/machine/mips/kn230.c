#ifndef lint
static char *sccsid = "@(#)kn230.c	4.6       (ULTRIX)  4/11/91";
#endif  lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
 *    	      Digital Equipment Corporation, Maynard, MA		*
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

/*
 * Modification History: kn230.c
 *
 * 11-Apr-91 -- Paul Grist
 * 	Modified trap and get-SIMM-sizes routines to account for
 *	NVRAM when handling errors.
 *
 * 09 Oct 90 -- Paul Grist
 *      Merge for MM to TI sync-ing of pools.
 *
 * 22 Aug 90 -- Paul Grist
 *      Added support to print out name string of option card
 *      at boot time.
 *
 * 19 Aug 90 -- chet
 *	Changes for presto interface.
 *
 * 05-Aug-90    Paul Grist
 *      Addded support for NVRAM/Prestoserve. Removed most of the debug
 *      code, except for option card debug support, this may prove useful
 *      later.
 *
 * 04-Aug-90	Randall Brown
 *	Added call to spl_init() to intialize spl function pointers and
 *      the intr() function pointer.
 *
 * 16-July-90  	Paul Grist
 *	minor FT2 cleanup/fixes and added include for presto.h for
 *      future NVRAM/presto support.
 *
 * 06-Jun-90    Paul Grist
 *      Complete option card support, add various fixes and cleanup
 *      before FT build. Added reporting of console state. 
 *
 * 06-May-90    Paul Grist
 *      Continue clean up, remove debug stuff, add routine to get the
 *      SIMM memory bank sizes for kn230_isolatepar, and add support to
 *      configure the option card.
 *
 * 24-Apr-90    Paul Grist
 *      Make initial modifications for pass 2
 *
 * 23-Apr-90    Paul Grist
 *      Name change everything from ds5100 to kn230.
 *
 * 13-Apr-90    Paul Grist
 *      Cleanup after first boot, added debug support.
 *
 * 09-Mar-90    Paul Grist
 *      Completed first pass code for MIPSMATE.
 */

#include "../machine/pte.h"
#include "../machine/cpu.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/user.h"	       /* gets time.h and debug.h too */
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/buf.h"
#include "../h/dk.h"
#include "../h/conf.h"
#include "../h/errlog.h"
#include "../h/cmap.h"
#include "../h/cpudata.h"
#include "../h/presto.h"
#include "../machine/fpu.h"
#include "../machine/hwconf.h"
#include "../machine/reg.h"
#include "../machine/mc146818clock.h"
#include "../machine/entrypt.h"
#include "../../machine/common/cpuconf.h"

#define	KN230_LANCE 	0x18000000	/* physical addr of lance registers */
#define KN230_SII   	0x1a000000      /* physical addr of sii registers */
#define	KN230_CLOCK 	0x1d000000	/* phys addr of clock chip */

#define KN230_ICSR	0x1e000000	/* Interrupt control & status reg */
#define	KN230_HALT	0x00004000	/* Set if halt switch is pushed */
#define KN230_WMERR	0x00002000	/* Set on memory write NXM	*/
#define KN230_NIIRQ	0x00001000	/* "1" is lance posting interrupt*/
#define KN230_SCSIRQ	0x00000800	/* "1" is SII posting interrupt*/
#define KN230_EXPIRQ1	0x00000400	/* "1" is expan opt posting interrupt*/
#define KN230_EXPIRQ0	0x00000200	/* "1" is expan opt posting interrupt*/
#define KN230_DZIRQ     0x00000100      /* "1" is console posting interrupt */

#define KN230_LED	0x14000000	/* LED register	*/
#define KN230_LED7	0x00008000	/* "1" turns off led 7	*/
#define KN230_LED6	0x00004000	/* "1" turns off led 6	*/
#define KN230_LED5	0x00002000	/* "1" turns off led 5	*/
#define KN230_LED4	0x00001000	/* "1" turns off led 4	*/
#define KN230_LED3	0x00000800	/* "1" Turns off led 3 and disables
                                         main memory, enables wr to EEPROM */
#define KN230_LED2	0x00000400	/* "1" Turns off led 2 and selects 38.4
                                         baud, "0" selects default 19.2k  */
#define KN230_LED1	0x00000200	/* "1" (read only) means 32M in bank1
                                           "0" means 8M in memory bank 1 */
#define KN230_LED0	0x00000100	/* "1" enables halts-disables reset */

#define KN230_WEAR	0x17000000	/* Write error address register */
#define WEAR_IOPRESENT  0x00000001      /* "1" means no option card present */
#define WEAR_SECURE     0x00000002      /* "1" means system is insecure */

#define KN230_PASSWD    0x1f000244      /* password location */
#define PASSWD_FIELD    0xffffffc0      /* mask out passwd field */
#define PASSWD_FAILS    0x0000000f      /* mask out #of failures */

#define KN230_OID       0x1f00020c      /* option slot ID register */
#define OID_NUMBER_MSK  0x000000ff      /* mask out the ID number */
#define OID_ERR_COUNT   0x0000ff00      /* mask out diag failure count */

#define KN230_DZ0_CSR  0x1c000000      /* DZ0 - Control and status register */
#define KN230_DZ1_CSR  0x15000000      /* DZ1 Control and status */
#define KN230_DZ2_CSR  0x15200000      /* DZ2 Control and status */

#define ESRPKT 1
#define MEMPKT 2

#define ICSR_ADDR    PHYS_TO_K1(KN230_ICSR)   /* virt address of ICSR */
#define LANCE_ADDR   PHYS_TO_K1(KN230_LANCE)  /* virt address of LANCE */
#define LED_ADDR     PHYS_TO_K1(KN230_LED)    /* virt address of LED */
#define WEAR_ADDR    PHYS_TO_K1(KN230_WEAR)   /* virt address of WEAR */
#define OID_ADDR     PHYS_TO_K1(KN230_OID)    /* virt address of OID */
#define PASSWD_ADDR  PHYS_TO_K1(KN230_PASSWD) /* virt address of PASSWD */

/* used to save state of registers for logging */
u_long kn230_esr_icsr;
u_long kn230_esr_leds;
u_long kn230_esr_wear;
u_long kn230_esr_oid;


/*
 * Since there is no vector register and we want non-configured
 * devices and controllers to point to stray(), we will use the
 * general purpose scb to dispatch are interrupts.
 *
 * assign indexes into the scb for the five hardware
 * interrrupts. Use scb locations 100-104.
 *
 */

#define DZ_UART_INDEX 400         /* DZ UART - DZIRQ */
#define EXP0_INDEX    404         /* EXPANSION OPTION - EXPIRQ0 */
#define EXP1_INDEX    408         /* EXPANSION OPTION - EXPIRQ1 */
#define SII_INDEX     412         /* SII (scsi controller) - SCSI IRQ */
#define LANCE_INDEX   416         /* NI (lance) - NIIRQ */

/*
 * These defines, macros, and variables are for memory parity errors.
 * Format of "memreg" for logging memory parity errors.
 *
 */

#define SIMMOFF 28
#define TYPEOFF 26
#define BYTEOFF 25
#define DPOFF 24
#define TCOUNTOFF 16
#define SCOUNTOFF 8
#define HCOUNTOFF 0
#define MEMREGFMT(simm, type, byte, dp, tcount, scount, hcount) \
(simm << SIMMOFF | type << TYPEOFF | byte << BYTEOFF | dp << DPOFF | \
tcount << TCOUNTOFF | scount << SCOUNTOFF | hcount << HCOUNTOFF)

/* 
 * MEMREG is a hand crafted info register (sw) used for error logging 
 *        and handling memory parity errors.
 *  
 *   31   28     26   25    24             16            8            0 
 *   +------+------+------+----+-------------+------------+------------+
 *   | SIMM | TYPE | BYTE | TP | TRANS COUNT | SOFT COUNT | HARD COUNT |
 *   +------+------+------+----+-------------+------------+------------+
 *
 */

#define TRANSINTVL (60*15)	    /* time delta to enable parity logging
                                       is 15 minutes */
int kn230transintvl = TRANSINTVL;   /* global var so we can change it */
int kn230translog = 1;		    /* is trans logging currently enabled */

struct kn230_trans_errcnt {       /* trans parity errors */
        long   trans_last;  	  /* time of most recent trans err */
        long   trans_prev;	  /* time of previous trans err */
} kn230_trans_errcnt = { 0, 0 };

#define TRANSPAR 0x1              /* transient parity error */
#define SOFTPAR  0x2              /* soft parity error */
#define HARDPAR  0x3              /* hard parity error */

#define TEXTPG(pt) ((pt) & CTEXT)
#define SMEMPG(pt) ((pt) & CSMEM)
#define MAXSIMM 8
int tcount[MAXSIMM];			/* # of trans parity errs per simm*/
int scount[MAXSIMM];			/* # of soft errs on each simm */
int hcount[MAXSIMM];			/* # of hard errs on each simm */
int kn230_parityerr = 0;        	/* flag for parity err isolation */
caddr_t vatophys();			/* typedef of functions */
unsigned kn230_isolatepar();
int kn230transenable();

int kn230_memsize;                      /* integer size of system memory */
int kn230_bank1;                        /* integer value size of memory bank */
int kn230_bank2;                        /* integer value size of memory bank */
int kn230_bank3;                        /* integer value size of memory bank */
int kn230_bank4;                        /* integer value size of memory bank */

extern int nMDC;                        /* number of mdc chips in config file*/

/*
 * console and clock external variables
 */

extern u_int printstate;	/* how to print to the console */
extern char *rt_clock_addr;	/* addr of the mc146818clock chip */
extern struct timeval time;	/* the system time */
extern int hz;
extern int tick;
extern int tickadj;
extern int fixtick;		/* set to the number of microsec's that the
				   clock looses per second */
/*
 * option card info structure
 */

struct kn230_option {
  u_int option_id;         /* id# as read from OID register */
  char driver_name[32];    /* driver name as read in configuration file */
  char type;               /* 'D' or 'C',device or controller uba struct*/
  u_int exp0_csr;          /* add of expansion device 0's csr reg*/
  u_int exp1_csr;          /* add of expansion device 1's csr reg*/
  char option_name[64];    /* name string printed at boot time */
};

/*
 * NVRAM/Prestoserve support definitions
 *
 */

#define KN230_NVRAM_DIAG    0x1f000300         /* NVRAM diagnostic register */
#define NVRAM_SIZE          0x000000f0         /* MASK out nvram size bits */
#define KN230_NVRAM_FAILED  0x00000008         /* MASK out fail bit */
#define KN230_NVRAM_PRESENT 0x00000001         /* MASK out present bit */
#define KN230_NVRAM_RO      0x00000002         /* read only test failed */
#define KN230_NVRAM_RW      0x00000004         /* read/write test failed */

#define KN230_NVRAM_LOCATION  0x1f000304       /* NVRAM location register */

#define KN230_NVRAM_BKILL    0x00000002       /* status reg mask for bkill */
#define KN230_NVRAM_BFAIL    0x00000001       /* status reg mask for bfail */

#define NVRAM_DIAG           PHYS_TO_K1(KN230_NVRAM_DIAG)
#define NVRAM_LOCATION       PHYS_TO_K1(KN230_NVRAM_LOCATION)

volatile unsigned int kn230_nvram_estatus;      /* even status reg addr*/
volatile unsigned int kn230_nvram_ostatus;      /* odd status reg addr*/
volatile unsigned int kn230_nvram_econtrol;     /* even control reg addr*/
volatile unsigned int kn230_nvram_ocontrol;     /* even control reg addr*/
volatile unsigned int kn230_nvram_location;     /* nvram start addr value*/
unsigned kn230_nvram_start_addr;                /* adjusted start value*/
unsigned kn230_nvram_size;                      /* adjusted size value*/



/*****************************************************************************
 *
 * Interrupt handlers for the 6 hardware interrupts and 2 software
 * interrupts for MIPSMATE.
 *
 *****************************************************************************/

extern softclock(),softnet(),kn230_hardintr1(),kn230_hardintr0(),
	hardclock(), fpuintr(),kn230_memintr(),kn230_haltintr();


/* The routines that correspond to each of the 8 interrupt lines */
int (*kn230intr_vec[IPLSIZE])() = {
	softclock,		/* softint 0    SOFTCLOCK */
	softnet,		/* softint 1	NET */
	kn230_hardintr0,	/* hardint 0    DZ UART, IO EXPANSION */
	kn230_hardintr1,        /* hardint 1	LANCE, SII */
	hardclock,	        /* hardint 2	TOY */
	kn230_memintr,          /* hardint 3	Write Mem NXM */
	kn230_haltintr,	        /* hardint 4	HALT */
	fpuintr			/* hardint 5	FPU */
};

/*
 * The masks to use to look at each of the 8 interrupt lines.
 *
 */
int kn230iplmask[IPLSIZE] = {
        SR_IMASK1|SR_IEC,              /* softintr0 - SOFTCLOCK */
	SR_IMASK2|SR_IEC,              /* softintr1 - NET */
	SR_IMASK3|SR_IEC,              /* hardintr0 - DZ UART, IO EXPANSION */
	SR_IMASK4|SR_IEC,              /* hardintr1 - LANCE, SII */
	SR_IMASK5|SR_IEC,              /* hardintr2 - TOY */
	SR_IMASK6|SR_IEC,              /* hardintr3 - WRITE MEM NXM */
	SR_IMASK7|SR_IEC,              /* hardintr4 - HALT */
	SR_IMASK8|SR_IEC               /* hardintr5 - FPU */
};

/*
 * The SR reg masks for splxxx usage. See the defines in cpu.h.
 */

#define SR_HALT 0x4000                 /*  halt is hardint 4 */

int kn230splm[SPLMSIZE] = {
	SR_IEC | SR_IMASK0 | SR_HALT,	          /* 0 SPLNONE */
	SR_IEC | SR_IMASK1 | SR_HALT,             /* 1 SPLSOFTC	*/
	SR_IEC | SR_IMASK2 | SR_HALT,	          /* 2 SPLNET */
	0,				          /* 3 NOT_USED */
	0,			                  /* 4 NOT_USED */
	SR_IEC | SR_IMASK4 | SR_HALT,             /* 5 SPLBIO,SPLIMP,SPLTTY */
	SR_IEC | SR_IMASK5 | SR_HALT,	          /* 6 SPLCLOCK	*/
	SR_IEC | SR_IMASK6 | SR_HALT,	          /* 7 SPLMEM */
	SR_IEC | SR_IMASK8 | SR_HALT,	          /* 8 SPLFPU */
};

/*****************************************************************************
 * 
 * kn230_init(): Initialization routine for kn230 processor (MIPSMATE)
 *
 *                - initialize interrupt dispatch table: c0vec_tbl
 *                - initialize spl table: splm
 *                - initialize interrupt mask table: splm
 *                - direct lance interrupts to kn230_lance()
 *                - change from console trap handling to kernel trap handling
 *                - set up clock values
 *
 *****************************************************************************/

kn230_init()
{
	extern int c0vec_tblsize;
	extern int (*c0vec_tbl[])();
	extern int iplmask[];
	extern int splm[];
	extern struct cpusw *cpup;
	extern int (*(scb[])) ();

	/*
	 * Initialize the interrupt dispatch table c0vec_tbl.
	 * Initialize the spl table for the system we are running on.
	 * Initialize the interrupt masks for the system we are running on.
	 * This must be done very early on at boot time and must
	 * be done before any interrupt (or spl) is allowed.
	 */
	bcopy((int *)kn230intr_vec, c0vec_tbl, c0vec_tblsize);
	bcopy(kn230iplmask, iplmask, IPLSIZE * sizeof(int));
	bcopy(kn230splm, splm, (SPLMSIZE) * sizeof(int));

	/* Initialize the spl dispatch table and the intr dispatch routine */
	spl_init();

	/* 
	 * change over from console to kernel trap handling; this is
	 * done by changing the BEV bit in the status register.
         */
	clear_bev();

	/*
	 * Set up the system specific value for "hz", the number of clock
	 * interrupts per second; and corresponding tick and tickadj values.
         * These values will be used by hardclock. HZ=256 for mc146818
	 */
	hz = cpup->HZ;
	tick = 1000000 / hz;
	tickadj = 240000 / (60 * hz);
	fixtick = 1000000 - (tick * hz);     

	/*
	 * Assign the rt_clock_addr for this processor
	 */

	rt_clock_addr = (char *)PHYS_TO_K1(KN230_CLOCK);

	return (0);

}

/************************************************************************
 * 
 * kn230_conf:     Configuration routine for MIPSMATE
 *
 *                  - set up PROM environment entries
 *                  - set up FPU ; turn on interrupts
 *                  - configure system devices and controllers
 *                  - enable logging of transient parity errors
 *                  - report the state of the halt/reset button
 *
 ************************************************************************/

kn230_conf()
{
	extern int cold;
	extern unsigned cpu_systype;
	extern int cpu;			/* ULTRIX internal System type */
	extern int kn230_turn_nvram_on;

	cold = 1;                      

	/*
	 * get memory bank sizes for isloating parity errors. Can not
	 * call it any sooner than this because it uses the values 
	 * calculated by mapinit which is called before this in the
	 * startup routine.
	 * 
	 */
	kn230_getSIMMsizes();

	/*
 	 *Initialize PROM environment entries (hwconf.promenv[0-5].name fields)
	 */
	hwconf_init();

	/* 
	 * Report what system we are on
	 */
	printf("KN230 processor - system rev %d\n", (GETHRDREV(cpu_systype)));

	/* 
         * Deal with the CPU and FPU
         */
	coproc_find();


	/* 
         * probe for all the I/O contollers and 
	 * devices (DZ UART, LANCE, SII)
         */
	kn230_config_devices();

	/*
	 * configure nvram option, this enables
	 * Prestoserve if the nvram modules are present
	 *
	 */
	kn230_config_nvram();

	/*
	 * configure the expansion option card
	 *
	 */
	kn230_expansion_conf();


	/* report console state
	 *
	 * report state of console security and failed attempts if secure.
	 * report the state of HALT enable bit in the LED register
	 * to the console so the user knows how it is currently
	 * set, it can be changed through a prom env var.
         */

	printf("console status: \n");

	if ( *(int *)WEAR_ADDR & WEAR_SECURE)
	  printf("\tsecurity mode disabled\n");
	else
	  {
	    printf("\tsecurity can be used\n");          /* security enabled */
	    if ( *(int *)PASSWD_ADDR & PASSWD_FIELD )    /* passwd is set */
	      {
		printf("\tconsole password is set\n");
		if ( *(int *)PASSWD_ADDR & PASSWD_FAILS )
		  printf("\tfailed attempts: %d\n",
			 *(int *)PASSWD_ADDR &PASSWD_FAILS);
	      }
	  }

	if ( *(int *)LED_ADDR & KN230_LED0 )
	  printf("\thalt is enabled\n\n");
	else
	  printf("\treset is enabled\n\n");

	/*
	 * call transenable every 15 minutes to reenable transient partiy
	 * error logging
	 */
	timeout (kn230transenable, (caddr_t) 0, kn230transintvl * hz);   

	cold = 0;

	/*
	 * Safe to take all interrupts now, turn them on.
	 */
	splnone();

	return(0);
}



/******************************************************************************
 *
 *   kn230_config_devices:  configure all system devices and controllers,
 *                          called by kn230_conf.
 *
 *  - probe all I/O devices and controllers in one place
 *    and set the appropriate states/flags for further use.
 *    (LANCE, SII, I/O EXPANSION, DZ UART)
 *
 *   On board devices sit on a virtual bus called the "ibus"
 *
 *****************************************************************************/

kn230_config_devices() 
{
	int s;

	config_set_alive("ibus",0,0,0);

	/*
	 *   Probe the system console subsystem
 	 *   
	 *   If the option card is not installed and the config file has
	 *   the devices in it nMDC > 1 and the mdc driver will attempt
	 *   to configure a non-existant device which results in a hang.
	 *   This case will probably only be hit during debug, but it
	 *   can also happen if the option board is removed after installation
	 *   and the kernel is not remade.
	 */

	if (  *(int *)WEAR_ADDR & WEAR_IOPRESENT )  /* not present */
	  {
	    nMDC = 1 ;  /* safeguard against config file error */
	  }

	/* if it is present - driver will configure all mdc devices */

	ib_config_dev(PHYS_TO_K1(KN230_DZ0_CSR), KN230_DZ0_CSR, 0,
                          "mdc",(int)scb+DZ_UART_INDEX);

	/* 
	 * Probe the sii based scsi bus.
	 */
	ib_config_cont(PHYS_TO_K1(KN230_SII),KN230_SII,0,"sii",
                       (int)scb+SII_INDEX);

	/*
	 *  Config the Lance
	 */
	ib_config_dev(PHYS_TO_K1(KN230_LANCE),KN230_LANCE,0,
                     "ln",PHYS_TO_K1((int)scb+LANCE_INDEX));

	return(0);
}

 /****************************************************************************
  *
  *  kn230_expansion_conf()
  *
  *  configure the I/O expansion slot, called by kn230_conf
  *      
  * check WEAR register IOPRESENT bit to see if option card
  * is present. if present, read option ID register and use
  * data file to map id number to option info. If id# is not
  * in kn230_option_data.c report configuration error, otherwise
  * use info to configure option card device(s).
  *
  * NOTE: The dc7085 device driver (mdc.c) is a special case.
  *       It provides support for the console and the kn230 async
  *       card devices for a total of three dc7085 devices. The driver
  *       is already configured at this point, it is set up for
  *       the probe and attach routines to just return, so it is
  *       cool to configure them thru ib_config_dev.
  *
  ****************************************************************************/

kn230_expansion_conf()
{
  extern struct kn230_option kn230_option[];
  int option_id;
  int i=0;
  int found=0 ;
  u_int scb_addr, csr_addr;
  
	if ( !( *(int *)WEAR_ADDR & WEAR_IOPRESENT) ) 
	  {
	    option_id = *(int *)OID_ADDR & OID_NUMBER_MSK ;
	    printf("expansion option card present");

	    config_set_alive("ibus",0,0,0);

	    for ( i=0 ; kn230_option[i].option_id != -1 ; i++)
	      {

		if ( kn230_option[i].option_id == option_id)
		  {
		    found += 1;
		    if (found > 2)
		      {
			printf("data file error: only 2 devices per option\n");
			break;
		      }

		    if (found == 1)
		      {
			printf(": %s\n",kn230_option[i].option_name);
			csr_addr = kn230_option[i].exp0_csr;
			scb_addr = (int)scb+EXP0_INDEX;
		      }
		    else
		      {
			scb_addr = (int)scb+EXP1_INDEX;
			csr_addr = kn230_option[i].exp1_csr;
		      }

		    if (csr_addr == 0)
		      {
			printf("data file error: bad csr address\n");
			found = 0; 
			break;
		      }

		    if (nMDC>1) printf("\t");       /* for looks */

		    if ( kn230_option[i].type == 'D')
		      { 
			ib_config_dev(PHYS_TO_K1(csr_addr),
				      csr_addr,
				      0,
				      kn230_option[i].driver_name,
				      scb_addr );
		      }
		    else if  ( kn230_option[i].type == 'C')
		      {
			ib_config_cont(PHYS_TO_K1(csr_addr),
				      csr_addr,
				      0,
				      kn230_option[i].driver_name,
				      scb_addr );
		      }
		    else
		      printf("data file error: type field incorrect\n");

		  }
	      }

	    if (found == 0) 
	      {
	       printf("\nOPTION CARD NOT CONFIGURED\n");
	       printf("\tconfiguration error: could not match value in\n");
	       printf("\toption id register with entries in data file,\n");
	       printf("\tprobable cause is the 'iooption' console variable\n");
	      }
	  }
	else
	  printf("expansion option card not present\n");

}


/*****************************************************************************
 *
 * kn230_delay: MIPSMATE delay routine
 * 
 *               - delay for n microseconds
 *
 ****************************************************************************/

int kn230_delay_mult = 11 ;     /* optimized delay multiplier */

kn230_delay(n)
     int n;                      /* number of usecs to delay for */
{
  register int N = kn230_delay_mult * (n);
  while (--N > 0);
  return(0);
}

/*****************************************************************************
 *
 * kn230_haltintr:  Entry point for  HALT interrupt
 *
 * call console prom halt routine to do the rest
 *
 *****************************************************************************/

kn230_haltintr(ep)
int *ep;
{
    prom_halt(ep) ;
}


/******************************************************************************
 *
 * kn230_hardintr0() : Entry for interrupts from DZ UART or IO EXPAN CARD
 *
 * - determine source of interupt and dispatch it.
 * - must be in order of priority, handle only one
 *   and get out, we will return here if more than
 *   one IRQ bit is set, this will preserve priority.
 * 
 *****************************************************************************/

kn230_hardintr0(ep)
int *ep;
{
extern int (*(scb[])) ();

 if ( *(int *)ICSR_ADDR & KN230_DZIRQ)
    (*(scb[DZ_UART_INDEX/4])) (ep,DZ_UART_INDEX/4);

     else if ( *(int *)ICSR_ADDR & KN230_EXPIRQ0)
       (*(scb[EXP0_INDEX/4])) (ep,EXP0_INDEX/4); 
     
          else if ( *(int *)ICSR_ADDR & KN230_EXPIRQ1)
	    (*(scb[EXP1_INDEX/4])) (ep,EXP1_INDEX/4);
	
               else
	          printf("kn230: took stray interrupt on h/w intr level 0\n");
}

/******************************************************************************
 *
 * kn230_hardintr1: Entry point for interrupts from LANCE or SII interrupts
 *
 *  - determine source of interupt and dispatch it
 *  - lance has higher priority, service one interupt
 *    and get out, if net or disk intr is pending we
 *    will be called right back - preserves priority
 *
 *****************************************************************************/

kn230_hardintr1(ep)
int *ep;
{
extern int (*(scb[])) ();

  if ( *(int *)ICSR_ADDR & KN230_NIIRQ)
    (*(scb[LANCE_INDEX/4])) (ep,LANCE_INDEX/4); 
    
     else if (  *(int *)ICSR_ADDR & KN230_SCSIRQ)
       (*(scb[SII_INDEX/4])) (ep,SII_INDEX/4);
       
          else
	     printf("kn230: took stray interrupt on h/w intr level 1");
}


/******************************************************************************
 *
 * kn230transenable: Enable transient parity memory error logging 
 *
 *****************************************************************************/

kn230transenable()
{
  kn230translog = 1;
  timeout (kn230transenable, (caddr_t) 0, kn230transintvl * hz);
}


/******************************************************************************
 *
 * kn230_trap_error:
 *
 * Routine to handle trap errors: user-mode,ibe & dbe, & all kernel mode traps.
 * Try recover from user-mode errors and panic on kernel mode errors.
 *
 *****************************************************************************/

kn230_trap_error(ep, code, sr, cause, signo)
	register u_int *ep;		/* exception frame ptr */
	register u_int code;		/* trap code (trap type) */
	u_int sr, cause;		/* status and cause regs */
	int *signo;			/* set if we want to kill process */
{
	caddr_t pa;			/* the physical addr of the error */
	int epc;			/* the EPC of the error */	
	unsigned memreg;		/* memory parity error info */
	int pagetype;          		/* type of page */
	int vaddr;			/* virt addr of error */
	register struct proc *p;	/* ptr to current proc struct */
	long currtime;			/* current time value */
	int kn230_physmem;	        /* adjusted value when NVRAM present*/

	/*
	 * save the state of the system registers for 
	 * error logging.
	 *
	 */
	kn230_esr_icsr = *(int *)ICSR_ADDR;
	kn230_esr_leds = *(int *)LED_ADDR;
	kn230_esr_wear = *(int *)WEAR_ADDR;
	kn230_esr_oid  = *(int *)OID_ADDR;

	/*
	 * If nvram present adjust physmem value to include the 1Mbyte
	 * of NVRAM which is located in memory. physmem naturally doesn't know
	 * about the NVRAM because it is not marked in the bitmap like
	 * regular memory. Note physmem is in pages, so 1MB is 256 pages.
	 *
	 */
	if ( *(int *)NVRAM_DIAG & KN230_NVRAM_PRESENT )
	    kn230_physmem = physmem + 256;
	else
	    kn230_physmem = physmem;

	p = u.u_procp;
	if (USERMODE(sr)) {
		/*
		 * If address of bus error is in physical memory, then its
		 * a parity memory error.  Gather additional info in "memreg",
		 * for the error log & to determine how to recover.
		 * If its a transient error then continue the user process.
		 * If its a hard or soft parity error:
		 *    a) on a private process page, terminate the process
		 *	 (by setting signo = SIGBUS)
		 *    b) on a shared page, crash the system.
		 */
		pa = vatophys(ep[EF_BADVADDR]);
		if ( (int)pa != -1 && (btop((int)pa) < kn230_physmem)) {
			/*
			 * Note that we must save anything "interesting"
			 * from the exception frame, since kn230_isolatepar()
			 * may cause additional bus errors which will
			 * stomp on the exception frame in locore.
			 */
			vaddr = ep[EF_BADVADDR];
			epc = ep[EF_EPC];
			memreg = kn230_isolatepar(pa, vaddr);
	   		ep[EF_BADVADDR] = vaddr;
			ep[EF_EPC] = epc;
			/*
			 * If we get 3 or more in 1 second then disable logging
			 * them for 15 minutes.  The variable "kn230translog"
			 * is set by the kn230transenable routine.
			 */
			if (((memreg >> TYPEOFF) & TRANSPAR) == TRANSPAR) {
			    if (kn230translog) {
				currtime = time.tv_sec;
				if (currtime = kn230_trans_errcnt.trans_prev) {
					kn230translog = 0;
					mprintf("High rate of transient parity memory errors, logging disabled for 15 minutes\n");
					kn230_trans_errcnt.trans_last = 0;
					currtime = 0;
				}
			        log_kn230mem(ep, EL_PRIHIGH, pa, memreg);
				kn230_trans_errcnt.trans_prev = kn230_trans_errcnt.trans_last;
				kn230_trans_errcnt.trans_last = currtime;
			    }
			    return(0);
			}

			pagetype = cmap[pgtocm(btop(pa))].c_type;
			if (SHAREDPG(pagetype)) {
				log_kn230mem(ep, EL_PRISEVERE, pa,memreg);
				kn230consprint(MEMPKT, ep, pa);
				panic("memory parity error in shared page");
			} else {
			        log_kn230mem(ep, EL_PRIHIGH, pa,memreg);
		                printf("pid %d (%s) was killed on memory parity error\n",p->p_pid, u.u_comm);
				uprintf("pid %d (%s) was killed on memory parity error\n",p->p_pid, u.u_comm);
			}
		} else {
			uprintf("pid %d (%s) was killed on bus read error\n",
				p->p_pid, u.u_comm);
		}
	} else {
		/*
		 * Kernel mode errors.
		 * They all panic, its just a matter of what we log
		 * and what panic message we issue.
		 */
		switch (code) {

		case EXC_DBE:
		case EXC_IBE:
			/*
			 * Figure out if its a memory parity error
			 *     or a read bus timeout error
			 */
			pa = vatophys(ep[EF_BADVADDR]);
			if ( (int)pa != -1 && (btop((int)pa) < kn230_physmem))
			    {
				/*
				 * Note: must save anything "interesting"
				 * from the exception frame, since 
                                 * kn230_isolatepar() may cause
				 * additional bus errors which will
				 * stomp on the exception frame in locore.
				 */
				vaddr = ep[EF_BADVADDR];
				epc = ep[EF_EPC];
				memreg = kn230_isolatepar(pa, vaddr); 
				ep[EF_BADVADDR] = vaddr;
				ep[EF_EPC] = epc;
				log_kn230mem(ep, EL_PRISEVERE, pa,memreg); 
				kn230consprint(MEMPKT, ep, pa);
				panic("memory parity error in kernel mode");
			} else {
				log_kn230esr(ep, EL_PRISEVERE);
				kn230consprint(ESRPKT, ep, 0);
				panic("bus timeout");
			}
			break;
		case EXC_CPU:
			log_kn230esr(ep, EL_PRISEVERE);
			kn230consprint(ESRPKT, ep, 0);
			panic("coprocessor unusable");
			break;
		case EXC_RADE:
		case EXC_WADE:
			log_kn230esr(ep, EL_PRISEVERE);
			kn230consprint(ESRPKT, ep, 0);
			panic("unaligned access");
			break;
		default:
			log_kn230esr(ep, EL_PRISEVERE);
			kn230consprint(ESRPKT, ep, 0);
			panic("trap");
			break;
		}
	}
	/*
	 * Default user-mode action is to terminate the process
	 */
	*signo = SIGBUS;
	return(0);
}

/***************************************************************************** 
 *
 * kn230_memintr: Entry for WMERR interupt -- write to a non-existent address
 *
 * This does not happen synchronously (buffered write),
 * therefore we are not in process context and cannot terminate
 * a user process.  We must crash the system.
 *
 *****************************************************************************/

kn230_memintr(ep)
	u_int *ep;		/* exception frame ptr */
{
	volatile unsigned int icsr;

	/*
	 * read icsr and clear error
	 */

	icsr = *(int *)ICSR_ADDR;
	*(int *)ICSR_ADDR |= KN230_WMERR ;

	/*
	 * save the state of the system registers for 
	 * error logging.
	 *
	 */
	kn230_esr_icsr = icsr;
	kn230_esr_leds = *(int *)LED_ADDR;
	kn230_esr_wear = *(int *)WEAR_ADDR;
	kn230_esr_oid  = *(int *)OID_ADDR;

	if ( icsr & KN230_WMERR) {
	  log_kn230esr(ep, EL_PRISEVERE);
	  kn230consprint(ESRPKT, ep, 0); 
	  panic("NXM - write to non-existant memory");
	} 
	else {
	  log_kn230esr(ep, EL_PRISEVERE);
	  kn230consprint(ESRPKT, ep, 0);  
	  panic("stray memory error interrupt");
	}
	
}

/*****************************************************************************
 *
 * kn230consprint: print error information to console
 *
 *****************************************************************************/

kn230consprint(packet_type, ep, pa)

int packet_type;      /* ESR or MEM type error */
register u_int *ep;   /* exception frame pointer */
unsigned pa;          /* phys addr of MEM error */
{
  switch (packet_type) {
  case ESRPKT:
    cprintf("\nException condition\n");
    break;
  case MEMPKT:
    cprintf("\nMemory Error\n");
    break;
  default:
    cprintf("bad consprint - no packet_type given\n");
    break;
  }

  cprintf("\tCause reg\t\t= 0x%x\n", ep[EF_CAUSE]);
  cprintf("\tException PC\t\t= 0x%x\n", ep[EF_EPC]);
  cprintf("\tStatus reg\t\t= 0x%x\n", ep[EF_SR]);
  cprintf("\tBad virt addr\t\t= 0x%x\n", ep[EF_BADVADDR]);

  cprintf("\tInterupt CSR reg\t= 0x%x\n", *(int *)ICSR_ADDR);
  cprintf("\tWrite Error Add reg\t= 0x%x\n", *(int *)WEAR_ADDR);
  cprintf("\tLED reg\t\t\t= 0x%x\n", *(int *)LED_ADDR);
  cprintf("\tOption ID reg\t\t= 0x%x\n", *(int *)OID_ADDR);
  
  if (pa)
    cprintf("\tPhysical address of error: 0x%x\n",pa);
}


/*****************************************************************************
 *
 * log_kn230esr: Log Error & Status Registers to the error log buffer
 *
 *****************************************************************************/

log_kn230esr(ep, priority)
	register u_int *ep;	/* exception frame ptr */
	int priority;		/* for pkt priority */
{
	struct el_rec *elrp;

	elrp = ealloc(sizeof(struct el_esr5100), priority);
	if (elrp != NULL) {
	  LSUBID(elrp,ELCT_ESR,ELESR_5100,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
	  elrp->el_body.elesr.elesr.el_esr5100.esr_cause = ep[EF_CAUSE];
	  elrp->el_body.elesr.elesr.el_esr5100.esr_epc = ep[EF_EPC];
	  elrp->el_body.elesr.elesr.el_esr5100.esr_status = ep[EF_SR];
	  elrp->el_body.elesr.elesr.el_esr5100.esr_badva = ep[EF_BADVADDR];
	  elrp->el_body.elesr.elesr.el_esr5100.esr_sp = ep[EF_SP];
          elrp->el_body.elesr.elesr.el_esr5100.esr_icsr = kn230_esr_icsr;
          elrp->el_body.elesr.elesr.el_esr5100.esr_leds = kn230_esr_leds;
          elrp->el_body.elesr.elesr.el_esr5100.esr_wear = kn230_esr_wear;
          elrp->el_body.elesr.elesr.el_esr5100.esr_oid = kn230_esr_oid;
	  EVALID(elrp);
	}
}

/*****************************************************************************
 *
 * log_kn230mem: Log a memory error packet, so uerf can find it as a
 *               main memory error.
 *
 *****************************************************************************/

log_kn230mem(ep, priority, pa, memreg)
	register u_int *ep;	/* exception frame ptr */
	int priority;		/* pkt priority: panic: severe; else: high */
	int pa;			/* physical addr where memory err occured */
        unsigned memreg;        /* parity error information */
{
	struct el_rec *elrp;
	register struct el_mem *mrp;

	elrp = ealloc(EL_MEMSIZE, priority);
	if (elrp != NULL) {
	LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_5100,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		mrp = &elrp->el_body.elmem;
		mrp->elmem_cnt = 1;
		mrp->elmemerr.cntl = 1;
		mrp->elmemerr.type = ELMETYP_PAR;
		mrp->elmemerr.numerr = 1;
		mrp->elmemerr.regs[0] = memreg;
		mrp->elmemerr.regs[1] = pa;
		mrp->elmemerr.regs[2] = ep[EF_EPC];
		mrp->elmemerr.regs[3] = ep[EF_BADVADDR];
		EVALID(elrp);
	}
}


/****************************************************************************
 *
 * kn230_isolatepar:
 *
 * Overall goal here is to be able to recover a user process if the 
 * parity error was a soft error, and to log additional information.
 *
 * Isolate a memory parity error to which SIMM is in error.
 * This routine is machine specific, in that it "knows" how the memory
 * is laid out, i.e. how to convert a physical address to a module number.
 *
 * Block faults from occuring while we isolate the parity error by using
 * "nofault" facility thru the bbadaddr routine.
 *
 ****************************************************************************/

unsigned 
kn230_isolatepar(pa, va)
	register caddr_t pa;	/* the phys addr to convert to a SIMM */
	caddr_t va;		/* the virtual addr of the error */	
{
	register int i;		/* loop index */	
	register char *addr;	/* increment thru the word w/ parity error */
	register char *k1_addr;	/* kseg1 addr. for mem test writes */
	unsigned memreg;	/* collection of memory error info */
	int odd;		/* true if its the odd numbered SIMM */
	int simm;		/* which simm had the error */
	register int allzeros;	/* true if parity err occurs on all 0's write*/
	register int allones;	/* true if parity err occurs on all 1's write*/
	register int oneone;	/* true if parity err occurs on 1 1 write */
	int dp;			/* 0 for data bit, 1 for parity bit */
	int type;		/* error type: transient, soft, hard */
	int byte;		/* 0 for low byte; 1 for high byte in word */

	/*
	 * Round address down to long word, & clear flags.
	 */
	addr = (char *)((int)va & (~0x3));
	type = 0;
	dp = 0;
	/*
	 * Do badaddr probe on addr (a few times),
	 * to see if it was only a transient.
	 */
	kn230_parityerr = 0;
	for (i = 0; i < 4; i++) {
		if (bbadaddr(addr, 4)) {
			kn230_parityerr = 1;
			break;
		}
	}
	if (!kn230_parityerr) {
		type = TRANSPAR;
		byte = 0;
		odd = 1;
		goto getsimm;
	}
	/*
	 * Isolate the parity error to which SIMM is in error (which byte in
	 * the word) and isolate the type of error: soft or hard, data bit
	 * or parity bit.
	 *
	 * This is done by writing (& reading) each byte in the word first
	 * with all 0's then with all 1's (0xff) then with one 1 (0x1).
	 *
	 * Use k1_addr in order not to get TLBMOD exception when writing
	 * shared memory space
	 *
	 */

	k1_addr = (char *)(PHYS_TO_K1(((int)pa  & (~0x3)))); /*lw addr*/
	
	for (i = 0; i < 4; i++, addr += 1) {
		allzeros = 0;
		*k1_addr = 0x00;
		if (bbadaddr(addr, 1))
			allzeros = 1;
		allones = 0;
		*k1_addr = 0xff;
		if (bbadaddr(addr, 1))
			allones = 1;
		oneone = 0;
		*k1_addr = 0x1;
		if (bbadaddr(addr, 1))
			oneone = 1;
		/*
		 * If all 3 reads caused the error then this is the wrong
		 * byte, go on to the next byte
		 */
		if (allzeros && allones && oneone)
			continue;
		/*
		 * If only one of the allones/allzeros patterns caused a
		 * parity error, then we have a hard data bit stuck to
		 * zero or one.
		 */
		if ((allzeros && !allones && !oneone) ||
		    (allones && !allzeros && !oneone)) {
			type = HARDPAR;
			break;
		}
		/*
		 * If only the "oneone" (0x1) pattern caused a parity error,
		 *   then we have a parity bit stuck to zero.
		 * If only the "oneone" (0x1) pattern did NOT cause a parity
		 *   error then we have a parity bit stuck to one.
		 */
		if ((oneone && !allzeros && !allones) ||
		    (allzeros && allones && !oneone)) {
			type = HARDPAR;
			dp = 1;
			break;
		}
		/*
		 * If no parity error on all 3 patterns then we had a soft
		 * parity error in one of the data bits or in the parity bit
		 * of this byte.
		 */
		if (!allzeros && !allones && !oneone) {
			type = SOFTPAR;
			break;
		}
	}
	/*
	 * If i is 0 or 1, parity error is on the odd SIMM.
	 * If i is 2 or 3, parity error is on the even SIMM.
	 * Also record high or low byte position in half-word.
	 */
	switch (i) {
	case 0:
		byte = 0;
		odd = 1;
		break;
	case 1:
		byte = 1;
		odd = 1;
		break;
	case 2:
		byte = 0;
		odd = 0;
		break;
	case 3:
	default:
		byte = 1;
		odd = 0;
		break;
	}
getsimm:

	/*
	 * Record which SIMM
	 */
	if ((int)pa < kn230_bank1*1024*1024)
		if (odd)
			simm = 1;
		else
			simm = 2;
	else if ((int)pa < (kn230_bank1+kn230_bank2)*1024*1024)
		if (odd)
			simm = 3;
		else
			simm = 4;
	else if ((int)pa < (kn230_bank1+kn230_bank2+kn230_bank3)*1024*1024)
		if (odd)
			simm = 5;
		else
			simm = 6;
	else if ((int)pa < (kn230_bank1+kn230_bank2+kn230_bank3+kn230_bank4)*1024*1024)
		if (odd)
			simm = 7;
		else
			simm = 8;
	/*
	 * Increment error counts
	 */
	switch (type) {
	case TRANSPAR:
	default:
		tcount[simm]++;
		if (tcount[simm] > 255) {
			mprintf("Transient parity error count on simm # %d reached 255, reset to zero.\n", simm);
			tcount[simm] = 0;
		}
		break;
	case SOFTPAR:
		scount[simm]++;
		break;
	case HARDPAR:
		hcount[simm]++;
		break;
	}
	memreg = MEMREGFMT(simm, type, byte, dp, tcount[simm], scount[simm], hcount[simm]);

	return(memreg);
}



/****************************************************************************
 *
 * kn230_getSIMMsizes: Function is to identify the size of each of the
 *                     four banks of the kn230 SIMM memory. This info
 *                     is necessary for kn230_isolatepar, which uses it
 *                     to report which SIMM the parity error occured on.
 *
 *  Each bank of 2 SIMMs can be either 8M or 32M, there is a bit in the
 *  LED register which tells us what size the first bank contains. There
 *  is a configuration constraint of having the large (32M) memories 
 *  populate the banks before small memories (8M). Therefore, there are
 *  only a fixed number of possible combinations, and by knowing the
 *  size of memory and what the first bank is, we can figure out what
 *  size each bank contains.
 *
 *  NOTE: Due to the lack of info provided by the h/w, it is possible
 *        for this algorithm to fail. If there are bad pages, there
 *        exist conditions which will leave the SIMM layout incorrect
 *        for logging parity errors.
 ****************************************************************************/


kn230_getSIMMsizes()
{
  int memsize;
  int kn230_nvram_present=0;

 /*
  * get the intger value of memory size, it will not be an exact quanity
  * because the bitmap is marked unusable and there may also be bad memory.
  * Initialize all banks to zero to make life simpler.
  *
  * Need to account for NVRAM in memory, it is not marked in bitmap so
  * physmem doesn't know about it, if NVRAM is present, must make last
  * bank a 1Mbyte memory bank, the NVRAM SIMMs. 
  *
  */

  kn230_memsize = ( ctob(physmem) /1024 )/1024 ;
  kn230_bank1 = kn230_bank2 = kn230_bank3 = kn230_bank4 = 0;

  if ( *(int *)NVRAM_DIAG & KN230_NVRAM_PRESENT )
       kn230_nvram_present = 1;
 
  if  (kn230_memsize <= 8) 
      {
	  kn230_bank1 = 8;
	  if (kn230_nvram_present) kn230_bank2 = 1;
      }

  else if (kn230_memsize <= 16)
      {
	  kn230_bank1 = kn230_bank2 = 8;
	  if (kn230_nvram_present) kn230_bank3 = 1;
      }

  else if (kn230_memsize <= 24)
      {
	  kn230_bank1 = kn230_bank2 = kn230_bank3 = 8 ;
	  if (kn230_nvram_present) kn230_bank4 = 1;
      }

  else if (kn230_memsize <= 32)
     if ( *(int *)LED_ADDR & KN230_LED1 )   /* bank 1 is not 32M */
       kn230_bank1 = kn230_bank2 = kn230_bank3 = kn230_bank4 = 8 ;
     else
	 {
	     kn230_bank1 = 32 ;
	     if (kn230_nvram_present) kn230_bank2 = 1;
	 }

  else if (kn230_memsize <= 40)
    {
      kn230_bank1 = 32 ; 
      kn230_bank2 = 8 ;
      if (kn230_nvram_present) kn230_bank3 = 1;
    }

  else if (kn230_memsize <= 48)
    {
      kn230_bank1 = 32 ; 
      kn230_bank2 = kn230_bank3 = 8 ;
      if (kn230_nvram_present) kn230_bank4 = 1;
    }

  else if (kn230_memsize <= 56)
    {
      kn230_bank1 = 32 ; 
      kn230_bank2 = kn230_bank3 = kn230_bank4 = 8 ;
    }

  else if (kn230_memsize <= 64)
      {
	  kn230_bank1 = kn230_bank2 = 32; 
	  if (kn230_nvram_present) kn230_bank3 = 1;
      }

  else if (kn230_memsize <= 72)
    {
      kn230_bank1 = kn230_bank2 = 32 ; 
      kn230_bank3 = 8 ;
      if (kn230_nvram_present) kn230_bank4 = 1;
    }

  else if (kn230_memsize <= 80)
    {
      kn230_bank1 = kn230_bank2 = 32 ; 
      kn230_bank3 = kn230_bank4 = 8 ;
    }

  else if (kn230_memsize <= 96)
      {
	  kn230_bank1 = kn230_bank2 = kn230_bank3 = 32;
	  if (kn230_nvram_present) kn230_bank4 = 1;
      }

  else if (kn230_memsize <= 104)
    {
      kn230_bank1 = kn230_bank2 = kn230_bank3 = 32;
      kn230_bank4 = 8 ;
    }

  else if (kn230_memsize <= 128)
    kn230_bank1 = kn230_bank2 = kn230_bank3 = kn230_bank4 = 32 ;

  else
    printf("KN230: invalid memory configuration\n");
}


/*****************************************************************************
 *
 * kn230_config_nvram
 *
 *    - initialize pointers to cpu specific routines for Prestoserve
 *    - Check for nvram presence
 *    - initialize status and control register location according to 
 *      nvram location
 *    - make call to presto_init to initialize Prestoserve 
 *
 *****************************************************************************/
int kn230_cache_nvram = 1;
int kn230_nvram_mapped = 0;

kn230_config_nvram()
{
  	extern kn230_nvram_status();
	extern kn230_nvram_battery_status();
	extern kn230_nvram_battery_enable();
	extern kn230_nvram_battery_disable();
	extern u_int kn230_machineid();
	int hosed = 0;

	/* 
	 * Initialize the structure that will point to the cpu specific
	 * functions for Prestoserve
	 */

	presto_interface0.nvram_status = kn230_nvram_status;
	presto_interface0.nvram_battery_status= kn230_nvram_battery_status;
	presto_interface0.nvram_battery_disable= kn230_nvram_battery_disable;
	presto_interface0.nvram_battery_enable= kn230_nvram_battery_enable;

	/* 
	 * configure the NVRAM option if it is present
	 */

	if ( *(int *)NVRAM_DIAG & KN230_NVRAM_PRESENT )
	  {
	    printf("KN230 NVRAM present\n");

	    kn230_nvram_location =  *(unsigned *)NVRAM_LOCATION; 

	    /* 
	     * Determine physical start of nvram from KSEG1 value stored
	     * in location register. We could do something fancier here,
	     * but this allows me to verify that the location register is
	     * returning a legal value, so that if it doesn't we can bow
	     * out gracefully instead of crash and burn when we attempt
	     * to touch a non existant  memory address. This especially
	     * unsightful at this point in configure() because we will
	     * take multiple traps.
	     *
	     */
	    switch (kn230_nvram_location) {
		    
		  case 0xa0c00000: kn230_nvram_start_addr = 0xc00000 ;
		    break;
		  case 0xa1400000: kn230_nvram_start_addr = 0x1400000;
		    break;
		  case 0xa1c00000: kn230_nvram_start_addr = 0x1c00000;
		    break;
		  case 0xa2400000: kn230_nvram_start_addr = 0x2400000;
		    break;
		  case 0xa2c00000: kn230_nvram_start_addr = 0x2c00000;
		    break;
		  case 0xa3400000: kn230_nvram_start_addr = 0x3400000;
		    break;
		  case 0xa3c00000: kn230_nvram_start_addr = 0x3c00000;
		    break;
		  case 0xa4400000: kn230_nvram_start_addr = 0x4400000;
		    break;
		  case 0xa4c00000: kn230_nvram_start_addr = 0x4c00000;
		    break;
		  case 0xa5400000: kn230_nvram_start_addr = 0x5400000;
		    break;
		  case 0xa6400000: kn230_nvram_start_addr = 0x6400000;
		    break;
		  default:printf("KN230 NVRAM: Bad starting address\n");
		          hosed = 1;
		          break;
	    }
	    
	    if (hosed)
	     { 
	       printf("KN230 NVRAM: not intializing Prestoserve\n");
             }
	    else {

		    /* 
		     * set up all nvram information
		     *
		     * - locations of status/control registers are at
		     *   a fixed offset from first nvram address
		     *
		     * - starting nvram address for presto is 1K from the
		     *   physical start of the nvram board, that space is
		     *   used for diagnostics, etc..
		     *
		     * - size of nvram is available from the diag register,
		     *   and is 1K less than the full amount because of the
		     *   reserved space
		     *
		     */

		    kn230_nvram_start_addr += 0x400;

		    kn230_nvram_estatus = kn230_nvram_location + 0x200000; 
		    kn230_nvram_ostatus = kn230_nvram_location + 0x200004; 
		    kn230_nvram_econtrol = kn230_nvram_location - 0x200000; 
		    kn230_nvram_ocontrol = kn230_nvram_location-0x200000+0x4; 

		    kn230_nvram_size = (*(int *)NVRAM_DIAG & NVRAM_SIZE) >> 4;
		    kn230_nvram_size *= 0x100000; /* convert to bytes */
		    kn230_nvram_size -= 0x400; 	  /* - reserved space */

		    presto_init(kn230_nvram_start_addr, kn230_nvram_size, 
				kn230_nvram_mapped, kn230_cache_nvram,
			        kn230_machineid());

	    } /*end not hosed*/


	  } /*else not present*/

	else
	  printf("KN230 NVRAM not present\n");


}

/*****************************************************************************
 *
 * kn230_nvram_status
 *
 *     - provide presto with status of diagnostics run on nvram
 *       hence, let presto know what to do - recover, etc...
 *
 *****************************************************************************/

kn230_nvram_status()
{
	if ( *(int *)NVRAM_DIAG & KN230_NVRAM_FAILED)
		return(NVRAM_BAD);
	else if ( *(int *)NVRAM_DIAG & KN230_NVRAM_RO)
		return(NVRAM_RDONLY);
	else if ( *(int *)NVRAM_DIAG & KN230_NVRAM_RW)
		return(NVRAM_RDWR);
	else {
		printf("No nvram diag bits set for status");
		return(-1);
	}
}

/****************************************************************************
 * kn230_nvram_battery_status
 *
 *     - update the global battery information structure for
 *       Prestoserve
 *
 ****************************************************************************/

kn230_nvram_battery_status()
{

	nvram_batteries0.nv_nbatteries = 1;	   /* one battery */
	nvram_batteries0.nv_minimum_ok = 1;	   /* it must be good */
	nvram_batteries0.nv_primary_mandatory = 1; /* primary must be OK */
	nvram_batteries0.nv_test_retries = 3;	   /* call this routine 3 times
						    * for each "test" */

	if ((*(int *)kn230_nvram_estatus & KN230_NVRAM_BFAIL) &&
	    (*(int *)kn230_nvram_ostatus & KN230_NVRAM_BFAIL))
		{
			nvram_batteries0.nv_status[0] = BATT_OK;
			return(0);
		}
	else if ( !((*(int *)kn230_nvram_estatus & KN230_NVRAM_BFAIL)) &&
		 !((*(int *)kn230_nvram_ostatus & KN230_NVRAM_BFAIL)))
		{
			nvram_batteries0.nv_status[0] = BATT_NONE;
			return(0);
		}
	else
		return(1);
}

/****************************************************************************
 * kn230_nvram_battery_enable
 *
 *  - performs disable battery kill function
 *
 ****************************************************************************/
kn230_nvram_battery_enable()
{
	
	*(int *)kn230_nvram_econtrol = 0x0;
	wbflush();
	*(int *)kn230_nvram_ocontrol = 0x0;
	wbflush();

	if ( (*(int *)kn230_nvram_estatus & KN230_NVRAM_BKILL) &&
	    (*(int *)kn230_nvram_ostatus & KN230_NVRAM_BKILL))
		return(1);
	else
		return(0);
}

/****************************************************************************
 * kn230_nvram_battery_disable
 *
 * performs enable battery kill function
 * the excessive number of wbflushes is
 * to prevent future headaches when the
 * new r3020 write buffers merge writes.
 *
 ****************************************************************************/
kn230_nvram_battery_disable()
{

	*(int *)kn230_nvram_econtrol = 0x1;
	*(int *)kn230_nvram_ocontrol = 0x1;
	wbflush();
	
	*(int *)kn230_nvram_econtrol = 0x1;
	*(int *)kn230_nvram_ocontrol = 0x1;
	wbflush();
	
	*(int *)kn230_nvram_econtrol = 0x0;
	*(int *)kn230_nvram_ocontrol = 0x0;
	wbflush();
	
	*(int *)kn230_nvram_econtrol = 0x0;
	*(int *)kn230_nvram_ocontrol = 0x0;
	wbflush();
	
	*(int *)kn230_nvram_econtrol = 0x1;
	*(int *)kn230_nvram_ocontrol = 0x1;
	wbflush();
	
	
	if ((*(int *)kn230_nvram_estatus & KN230_NVRAM_BKILL) &&
	    (*(int *)kn230_nvram_ostatus & KN230_NVRAM_BKILL))
		return(0);
	else
		return(1);
	
}

#include "../h/socket.h"
#include "../net/net/if.h"
/*
 * Get the hardware E_net address for on-board ln0 and use it
 * to build a poor man's version of a unique processor ID.
 */
u_int
kn230_machineid()
{
	register struct ifnet *ifp = ifnet;
	struct ifdevea ifr;
	u_int i = 0;
	int error;

	if (ifnet == NULL) {
		printf("kn220: ifnet NULL\n");
		return (i);
	}

	while (ifp != NULL) {
		if (ifp->if_name[0] == 'l' && ifp->if_name[1] == 'n' &&
		    ifp->if_unit == 0) {	/* found ln0 */
			error = (*ifp->if_ioctl)(ifp, SIOCRPHYSADDR, &ifr);
			if (error)
				return (i);
			i = (u_int)ifr.default_pa[2];
			i = i << 8;
			i += (u_int)ifr.default_pa[3];
			i = i << 8;
			i += (u_int)ifr.default_pa[4];
			i = i << 8;
			i += (u_int)ifr.default_pa[5];
			return (i);
		}
		ifp = ifp->if_next;
	}
	return (i);
}
























