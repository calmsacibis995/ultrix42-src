#ifndef lint
static char *sccsid = "@(#)scc.c	4.8      (ULTRIX)  1/22/91";
#endif	lint

/************************************************************************
 *									*
 *			Copyright (c) 1988,89 by			*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************
 *
/*
 * scc.c
 *
 * SCC SLU console driver
 *
 * Modification history
 *
 * 21-Jan-1991 - Randall Brown
 *	Modified use of tc_isolate_memerr() to now use tc_memerr_status
 *	struct.
 *
 * 06-Dec-1990 - Randall Brown 
 *	Added the call to tc_isolate_memerr() in the dma_xerror routine to
 *	log error information about the memory error.
 *
 * 06-Nov-1990 - pgt
 *      Added disabling of baud rate generator before time constants are
 *      set. Fixed the setting of breaks. Modify dma interrupt routines
 *      to do appropriate masking. Fixed handling of Speed Indicate.
 *	Did general clean-up. 
 * 
 * 13-Sep-90 Joe Szczypek
 *	Added new TURBOchannel console ROM support.  osconsole environment
 *	variable now returns 1 slot number if serial line, else 2 slot number
 *	if graphics.  Use this to determine how to do setup.  Note that new
 *	ROMs do not support multiple outputs...
 *
 * 7-Sept-1990 - pgt 
 *	Enabled modem control and break interrupts. Also fixed sccparam
 *	and added macros to handle enabling and disabling of modem and
 *	interrupts.
 *
 * 16-Aug-1990 - Randall Brown
 *	Enable use of mouse and keyboard ports.
 *
 * 20-Feb-1990 - pgt (Philip Gapuz Te)
 * 	created file.
 *
 */
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/devio.h"
#include "../../machine/common/cpuconf.h"
#include "../h/exec.h"
#include "../h/kmalloc.h"
#include "../h/sys_tpath.h"
#include "../io/uba/ubavar.h"	/* auto-config headers */

#include "../machine/cpu.h"
#include "../io/tc/ioasic.h"
#include "../io/tc/sccreg.h"
#include "../io/tc/slu.h"
#include "../io/tc/vsxxx.h"
#include "../io/tc/xcons.h"
#include "../io/tc/tc.h"

struct	tty scc_tty[NSCCLINE];   /* tty structure */
struct  slu slu;  
struct  scc_softc *sccsc;	/* software controller struct for scc */
u_char	sccmodem[NSCCLINE];	/* keeps track of modem state */
int     scc_cbaud[NSCCLINE];     /* baud rate integer flag */
int     scc_brk[4];             /* break condition */
struct	timeval scctimestamp[NSCCLINE];

char	sccsoftCAR;
char	sccdefaultCAR;
int	scc_cnt = NSCCLINE;

int	sccprobe(), sccattach(), scc_dma_rint();
int	scc_dsr_check(), scc_tty_drop(), scc_cd_drop(); /* Modem */
u_short	sccstd[] = { 0 };
struct uba_device *sccinfo[1];

struct uba_driver sccdriver = { sccprobe, 0, sccattach, 0, sccstd, "scc", 
				  sccinfo };

struct scc_softc scc_softc[1]; 

int	sccstart(), scc_dma_xint(), sccbaudrate();
int	ttrstrt();
int     sccspeedi(); scc_half_speed();

/*
 * Graphics device driver entry points.
 * Used to call graphics device driver as needed.
 */
extern	(*vs_gdopen)();
extern	(*vs_gdclose)();
extern	(*vs_gdread)();
extern	(*vs_gdwrite)();
extern	(*vs_gdselect)();
extern	(*vs_gdkint)();
extern	(*vs_gdioctl)();
extern	(*vs_gdstop)();
extern int (*v_consgetc)();
extern int (*v_consputc)();
extern int prom_getenv();

#define GRAPHIC_DEV 0x2 /* pick up from pm header file later */

#define LINEMASK	0x03		/* line unit mask */

int scc_other[4] = { 3, 2, 1, 0 };

/* ioasic register bits */
u_long scc_xdma_en[4] = { 0, 0, SSR_COMM1_XEN, SSR_COMM2_XEN};
u_long scc_rdma_en[4] = { 0, 0, SSR_COMM1_REN, SSR_COMM2_REN};
u_long scc_xint[4] = { 0, 0, SIR_COMM1_XINT, SIR_COMM2_XINT };
u_long scc_rint[4] = { 0, 0, SIR_COMM1_RINT, SIR_COMM2_RINT };
u_long scc_xerror[4] = { 0, 0, SIR_COMM1_XERROR, SIR_COMM2_XERROR };
u_long scc_rerror[4] = { 0, 0, SIR_COMM1_RERROR, SIR_COMM2_RERROR };

/*
 * Baud Rate Support
 *
 * When the baud rate on the right is specified, the line parameter register
 * is setup with the appropriate bits as specified in the left column.
 */
#define BAUD_UNSUPPORTED 0	/* Device does not provide this baud rate */
#define BAUD_SUPPORTED   1	/* Device does provide this baud rate     */

/*
 * The SCC manual provides a formula to compute the time constants for a
 * specified baudrate. For B110 and B134.5, the formula does not yield
 * whole integers. Thus, the chip baudrate in these two cases is very close
 * but not equal to the specified baudrate.
 */
struct baud_support scc_speeds[] = {
    {0,	                 0, 	                 BAUD_UNSUPPORTED}, /* B0    */
    {SCC_WR12_B50_LO,    SCC_WR13_B50_HI,	 BAUD_SUPPORTED},   /* B50   */
    {SCC_WR12_B75_LO,    SCC_WR13_B75_HI,	 BAUD_SUPPORTED},   /* B75   */
    {SCC_WR12_B110_LO,   SCC_WR13_B110_HI,	 BAUD_SUPPORTED},   /* B110  */
    {SCC_WR12_B134_5_LO, SCC_WR13_B134_5_HI,     BAUD_SUPPORTED},   /* B134  */
    {SCC_WR12_B150_LO,   SCC_WR13_B150_HI,	 BAUD_SUPPORTED},   /* B150  */
    {SCC_WR12_B200_LO,   SCC_WR13_B200_HI,	 BAUD_SUPPORTED},   /* B200  */
    {SCC_WR12_B300_LO,   SCC_WR13_B300_HI,	 BAUD_SUPPORTED},   /* B300  */
    {SCC_WR12_B600_LO,   SCC_WR13_B600_HI,	 BAUD_SUPPORTED},   /* B600  */
    {SCC_WR12_B1200_LO,  SCC_WR13_B1200_HI,	 BAUD_SUPPORTED},   /* B1200 */
    {SCC_WR12_B1800_LO,  SCC_WR13_B1800_HI,	 BAUD_SUPPORTED},   /* B1800 */
    {SCC_WR12_B2400_LO,  SCC_WR13_B2400_HI,	 BAUD_SUPPORTED},   /* B2400 */
    {SCC_WR12_B4800_LO,  SCC_WR13_B4800_HI,	 BAUD_SUPPORTED},   /* B4800 */
    {SCC_WR12_B9600_LO,  SCC_WR13_B9600_HI,	 BAUD_SUPPORTED},   /* B9600 */
    {SCC_WR12_B19200_LO, SCC_WR13_B19200_HI,     BAUD_SUPPORTED},   /* EXTA  */
    {SCC_WR12_B38400_LO, SCC_WR13_B38400_HI,     BAUD_SUPPORTED},   /* EXTB  */
};

extern int 	consDev;

#define SCC0_B_BASE (PHYS_TO_K1(0x1c100000))
#define SCC0_A_BASE (PHYS_TO_K1(0x1c100008))
#define SCC1_B_BASE (PHYS_TO_K1(0x1c180000))
#define SCC1_A_BASE (PHYS_TO_K1(0x1c180008))

#define SCC_READ(rsp, reg, val)   { \
	     (rsp)->SCC_CMD = ((u_short)(reg))<<8; \
	     (val) = (((rsp)->SCC_CMD)>>8)&0xff; \
		 DELAY(10);\
	     }

#define SCC_WRITE(rsp, reg, val)  { \
	     (rsp)->SCC_CMD = ((u_short)(reg))<<8; \
	     (rsp)->SCC_CMD = ((u_short)(val))<<8; \
		 DELAY(10);\
	     }

/* these macros can only set or clear one bit at a time */
#define SCC_MSET(unit, bit) { \
	     sccsc->sc_saved_regs[(unit)].wr5 |= (bit); \
	     SCC_WRITE(sccsc->sc_regs[(unit)], SCC_WR5, \
		       sccsc->sc_saved_regs[(unit)].wr5); \
             }

#define SCC_MCLR(unit, bit)   { \
	     sccsc->sc_saved_regs[(unit)].wr5 &= ~(bit); \
	     SCC_WRITE(sccsc->sc_regs[(unit)], SCC_WR5, \
		       sccsc->sc_saved_regs[(unit)].wr5); \
             }

#define SCC_MTEST(unit, bit)  (((sccsc->sc_regs[(unit)]->SCC_CMD)>>8)&(bit))

/* DTR makes use of the DTR of channel A, controlled by WR5 */
#define SCC_SET_DTR(unit)   SCC_MSET(scc_other[(unit)], SCC_WR5_DTR)
#define SCC_CLR_DTR(unit)   SCC_MCLR(scc_other[(unit)], SCC_WR5_DTR)

#define SCC_SET_RTS(unit)   SCC_MSET(scc_other[(unit)], SCC_WR5_RTS)
#define SCC_CLR_RTS(unit)   SCC_MCLR(scc_other[(unit)], SCC_WR5_RTS)

#define	SCC_SET_SS(unit)    SCC_MSET((unit), SCC_WR5_RTS)
#define	SCC_CLR_SS(unit)    SCC_MCLR((unit), SCC_WR5_RTS)

#define SCC_SET_BRK(unit)   SCC_MSET((unit), SCC_WR5_BRK)
#define SCC_CLR_BRK(unit)   SCC_MCLR((unit), SCC_WR5_BRK)

#define SCC_DSR(unit)   SCC_MTEST(scc_other[(unit)], SCC_RR0_SYNC)
#define SCC_CTS(unit)   SCC_MTEST((unit), SCC_RR0_CTS)
#define SCC_DCD(unit)   SCC_MTEST((unit), SCC_RR0_DCD)

#define SCC_SI(unit)	SCC_MTEST(scc_other[(unit)], SCC_RR0_CTS)
#define SCC_RI(unit)	SCC_MTEST(scc_other[(unit)], SCC_RR0_DCD)
#define SCC_XMIT(unit)  (SCC_DSR(scc_other[(unit)]) && SCC_CTS((unit)) && SCC_DCD((unit)))

#define SCC_DTR(unit) ((sccsc->sc_saved_regs[scc_other[(unit)]].wr5)&SCC_WR5_DTR)
#define SCC_RTS(unit) ((sccsc->sc_saved_regs[scc_other[(unit)]].wr5)&SCC_WR5_RTS)

#define SCC_MODEM_ON(unit)  { \
	SCC_WRITE(sccsc->sc_regs[scc_other[(unit)]], SCC_WR15, SCC_WR15_SYNC_IE); \
	sccsc->sc_regs[scc_other[(unit)]]->SCC_CMD = SCC_WR0_RESET_EXT_INT<<8; \
	sccsc->sc_regs[scc_other[(unit)]]->SCC_CMD = SCC_WR0_RESET_EXT_INT<<8; \
	SCC_WRITE(sccsc->sc_regs[(unit)], SCC_WR15, (SCC_WR15_CTS_IE|SCC_WR15_DCD_IE|SCC_WR15_BREAK_IE)); \
        sccsc->sc_regs[(unit)]->SCC_CMD = SCC_WR0_RESET_EXT_INT<<8; \
	sccsc->sc_regs[(unit)]->SCC_CMD = SCC_WR0_RESET_EXT_INT<<8; \
    }

#define SCC_MODEM_OFF(unit)  { \
	SCC_WRITE(sccsc->sc_regs[scc_other[(unit)]], SCC_WR15, 0x00); \
	sccsc->sc_regs[scc_other[(unit)]]->SCC_CMD = SCC_WR0_RESET_EXT_INT<<8; \
	sccsc->sc_regs[scc_other[(unit)]]->SCC_CMD = SCC_WR0_RESET_EXT_INT<<8; \
	SCC_WRITE(sccsc->sc_regs[(unit)], SCC_WR15, SCC_WR15_BREAK_IE); \
	sccsc->sc_regs[(unit)]->SCC_CMD = SCC_WR0_RESET_EXT_INT<<8; \
	sccsc->sc_regs[(unit)]->SCC_CMD = SCC_WR0_RESET_EXT_INT<<8; \
    }

#define SCC_INT_ON(unit) { \
	if ((unit) == 0 || (unit) == 1) { \
	    sccsc->sc_saved_regs[(unit)].wr1 &= ~SCC_WR1_RINT; \
	    sccsc->sc_saved_regs[(unit)].wr1 |= SCC_WR1_RINT_ALL; \
	    SCC_WRITE(sccsc->sc_regs[(unit)], SCC_WR1, sccsc->sc_saved_regs[(unit)].wr1); \
        } else { \
	    sccsc->sc_saved_regs[scc_other[(unit)]].wr1 |= SCC_WR1_EXT_IE; \
	    SCC_WRITE(sccsc->sc_regs[scc_other[(unit)]], SCC_WR1, sccsc->sc_saved_regs[scc_other[(unit)]].wr1); \
	    sccsc->sc_saved_regs[(unit)].wr1 &= ~SCC_WR1_RINT; \
	    sccsc->sc_saved_regs[(unit)].wr1 |= (SCC_WR1_RINT_SPC| SCC_WR1_EXT_IE| SCC_WR1_WDMA_EN); \
	    SCC_WRITE(sccsc->sc_regs[(unit)], SCC_WR1, sccsc->sc_saved_regs[(unit)].wr1); \
        } \
    }

#define PRINT_SIGNALS() { cprintf("Modem signals: "); \
	if (SCC_DSR(2)) cprintf(" DSR2 "); \
	if (SCC_CTS(2)) cprintf(" CTS2 "); \
	if (SCC_DCD(2)) cprintf(" CD2 "); \
	if (SCC_SI(2)) cprintf(" SI2 "); \
	if (SCC_RI(2)) cprintf(" RI2 "); \
	if (SCC_DSR(3)) cprintf(" DSR3 "); \
	if (SCC_CTS(3)) cprintf(" CTS3 "); \
	if (SCC_DCD(3)) cprintf(" CD3 "); \
	if (SCC_SI(3)) cprintf(" SI3 "); \
	if (SCC_RI(3)) cprintf(" RI3 "); \
	cprintf("\n"); }

#define PRINT_SIR(sir) { \
	    if ((sir) & SIR_COMM1_RINT)  cprintf("R1 "); \
	    if ((sir) & SIR_COMM2_RINT)  cprintf("R2 "); \
	    if ((sir) & SIR_COMM1_XINT)  cprintf("X1 "); \
	    if ((sir) & SIR_COMM2_XINT)  cprintf("X2 "); \
	    }

#define PRINT_SSR(line) { \
	    if ((*(u_int *)(0xbc040100)) & scc_rdma_en[(line)]) \
	      cprintf("SSR: RDMA Enabled: %d\n", (line)); \
	    }


sccprobe(reg)
     int reg;
{
    /* the intialization is done through scc_cons_init, so
     * if we have gotten this far we are alive so return a 1
     */
    return(1);
}

sccattach()
{
    register struct scc_softc *sc = sccsc;
    register int i;
    register struct scc_reg *rsp, *rsp0;
    register struct scc_saved_reg *ssp, *ssp0;

    sccsoftCAR = 0xff;
    sccdefaultCAR = 0xff;

    scc_init(2);
    if (consDev == GRAPHIC_DEV)
        scc_init(3);

    for (i = 2; i < 4; i++) {
	SCC_CLR_DTR(i);          /* clear modem control signals */
	SCC_CLR_RTS(i);
	SCC_CLR_SS(i);
/*
 * pgt: can not use IOC_CLR(reg, mask) to clear system interrupt register bits 
 * because bits may change between the time the register value is saved and 
 * the time it is written back with the mask. See ioasic.h for definition.
 */
	IOC_WR(IOC_SIR, ~scc_rint[i]);
	IOC_WR(IOC_SIR, ~scc_xerror[i]);
	IOC_WR(IOC_SIR, ~scc_rerror[i]);
	/* set rdma ptr, enable rdma */
	sc->ioc_regs[i]->RDMA_REG = (u_long)(svtophy(sc->rbuf[i][sc->rflag[i]]
				     + SCC_HALF_PAGE - SCC_WORD) << 3);
	IOC_SET(IOC_SSR, scc_rdma_en[i]);
    }
}

/*
 * scc_init() sets the SCC modes
 */
scc_init(unit)
     register int unit;
{
    register struct scc_softc *sc = sccsc;
    register struct scc_reg *rsp, *rsp0;
    register struct scc_saved_reg *ssp, *ssp0;
    register int line = unit;
    
    rsp = sc->sc_regs[line];
    ssp = &sc->sc_saved_regs[line];
    
    /*
     * set modes:
     *   WR9   force hardware reset
     *   WR4   line 1: odd parity, one stop bit, x16 clock
     *         lines 0, 2, 3: ~no parity, one stop bit, x16 clock
     *   WR1   ~W/DMA: DMA request, receive; parity is special condition
     *   WR2   interrupt vector 0x00
     *   WR3   8 bits/char
     *   WR5   8 bits/char
     *   WR9   interrupt disabled 0x00
     *   WR10  NRZ
     *   WR11  tx & rx clocks = brgen, TRxC input, RTxC ~no xtal
     *   WR12  low time constant
     *   WR13  hi time constant
     *         baud rate: line 0 or 1: 4800 baud
     *                    line 2 or 3: 9600 baud
     *   WR14  brgen source = pclk
     *         line 2 or 3: channel B: ~DTR/REQ: request function, transmit
     *                      channel A: ~DTR/REQ: ~DTR function
     */
    if (line == 0 || line == 1) {
	SCC_WRITE(rsp, SCC_WR9, SCC_WR9_RESETA);        
    } else {
        SCC_WRITE(rsp, SCC_WR9, SCC_WR9_RESETB);        
    }
/* rpbfix : do we need to delay this long ???? */
    DELAY(20);
    if (line == 1 )  {              /* odd parity for mouse */ 
        ssp->wr4 = ( SCC_WR4_PENABLE | SCC_WR4_ONESB | SCC_WR4_CLOCK16);
	SCC_WRITE(rsp, SCC_WR4, ssp->wr4);
    } else {                        /* no parity for mouse and comm. ports */
        ssp->wr4 = (SCC_WR4_ONESB | SCC_WR4_CLOCK16);
	SCC_WRITE(rsp, SCC_WR4, ssp->wr4);
    }
    ssp->wr1 = (SCC_WR1_DMA_REQ | SCC_WR1_WDMA_RX | SCC_WR1_PSPC);
    SCC_WRITE(rsp, SCC_WR1, ssp->wr1);
    SCC_WRITE(rsp, SCC_WR2, 0xf0);     /* interrupt vector 0x00 */
    ssp->wr3 = SCC_WR3_RBITS8;
    SCC_WRITE(rsp, SCC_WR3, ssp->wr3);
    ssp->wr5 = SCC_WR5_TBITS8;
    SCC_WRITE(rsp, SCC_WR5, ssp->wr5); 
    SCC_WRITE(rsp, SCC_WR9, 0x00);     /*  WR9 interrupt disabled */
    SCC_WRITE(rsp, SCC_WR10, SCC_WR10_NRZ);
     /*
      * TRxC pin should be an input because we can not allow output from
      * the TRxC pin for asynchronous communication. See SCC manual.
      */
    SCC_WRITE(rsp, SCC_WR11, SCC_WR11_TxC_BRGEN | SCC_WR11_RxC_BRGEN);
	      
    if (line == 0 || line == 1) {  /* keyboard or mouse 4800 BPS */
        SCC_WRITE(rsp, SCC_WR12, SCC_WR12_B4800_LO);
	SCC_WRITE(rsp, SCC_WR13, SCC_WR13_B4800_HI);
    } else {                       /* 9600 BPS */
        SCC_WRITE(rsp, SCC_WR12, SCC_WR12_B9600_LO);
	SCC_WRITE(rsp, SCC_WR13, SCC_WR13_B9600_HI);
    } 
    if (line == 0 || line == 1) {  /* channel A - keyboard or mouse */
        ssp->wr14 = SCC_WR14_BRGEN_PCLK;
	SCC_WRITE(rsp, SCC_WR14, ssp->wr14);
    } else { 
        /*
	 * Communication ports 1 & 2 use the A channels of the mouse and
	 * keyboard respectively. Since the ~DTR/REQ bit selects DTR when it
	 * is zero, no need to do anything here.
	 */
	ssp->wr14 = (SCC_WR14_BRGEN_PCLK | SCC_WR14_REQ);
	SCC_WRITE(rsp, SCC_WR14, ssp->wr14);
    }
    SCC_WRITE(rsp, SCC_WR9, SCC_WR9_MIE);
}

scc_cons_init()
{
    extern int (*vcons_init[])();
    extern int rex_base;
    int i;
    register struct scc_reg *rsp;
    register struct scc_softc *sc;
    register struct scc_saved_reg *ssp;
    int scc_mouse_init(), scc_mouse_putc(), scc_mouse_getc();
    int scc_kbd_init(), scc_kbd_putc(), scc_kbd_getc(), scc_putc();

    sccsc = &scc_softc[0];
    
    sc = sccsc;
    sc->sc_regs[0] = (struct scc_reg *)SCC1_A_BASE;
    sc->sc_regs[1] = (struct scc_reg *)SCC0_A_BASE;
    sc->sc_regs[2] = (struct scc_reg *)SCC0_B_BASE;
    sc->sc_regs[3] = (struct scc_reg *)SCC1_B_BASE;

    sc->ioc_regs[2] = (struct ioc_reg *)IOC_COMM1_DMA_BASE;
    sc->ioc_regs[3] = (struct ioc_reg *)IOC_COMM2_DMA_BASE;
    for (i = 2; i < 4; i++) {
        KM_ALLOC(sc->tbuf[i], char *, SCC_PAGE_SIZE, KM_DEVBUF, KM_CLEAR);
	KM_ALLOC(sc->rbuf[i][0], char *, SCC_PAGE_SIZE, KM_DEVBUF, (KM_CLEAR | KM_NOCACHE));
	KM_ALLOC(sc->rbuf[i][1], char *, SCC_PAGE_SIZE, KM_DEVBUF, (KM_CLEAR | KM_NOCACHE));
	sc->rflag[i] = 0;
    }

    /*
     *
     * Query the prom. The prom can be set such that the user 
     * could use either the alternate tty or the graphics console.
     * You get the graphics console if the first bit is set in
     * osconsole.  The user sets the console variable
     */
       if (!rex_base) {
	 if ((atoi(prom_getenv("osconsole")) & 0x1) == 1) {
	   slu.mouse_init = scc_mouse_init;
	   slu.mouse_putc = scc_mouse_putc;
	   slu.mouse_getc = scc_mouse_getc;
	   slu.kbd_init = scc_kbd_init;
	   slu.kbd_putc = scc_kbd_putc;
	   slu.kbd_getc = scc_kbd_getc;
	   slu.slu_tty  = scc_tty;
	   slu.slu_putc = scc_putc;
	   for( i = 0 ; vcons_init[i] ; i++ )
	     if ((*vcons_init[i])()) {	/* found a virtual console */
	       consDev = GRAPHIC_DEV;
	       return;
	     }
	 }	
	 scc_init(0); /* need to initialize channel A for modem control */
	 scc_init(1);
	 /* 
	  * set up line 3 as alternate console line: no parity, 9600 baud
	  */
	 scc_init(3);
       } else {
	 if ((strlen(rex_getenv("osconsole"))) > 1) {
	   slu.mouse_init = scc_mouse_init;
	   slu.mouse_putc = scc_mouse_putc;
	   slu.mouse_getc = scc_mouse_getc;
	   slu.kbd_init = scc_kbd_init;
	   slu.kbd_putc = scc_kbd_putc;
	   slu.kbd_getc = scc_kbd_getc;
	   slu.slu_tty  = scc_tty;
	   slu.slu_putc = scc_putc;
	   for( i = 0 ; vcons_init[i] ; i++ )
	     if ((*vcons_init[i])()) {	/* found a virtual console */
	       consDev = GRAPHIC_DEV;
	       return;
	     }
	 }
	 scc_init(0); /* need to initialize channel A for modem control */
	 scc_init(1);
	 /* 
	  * set up line 3 as alternate console line: no parity, 9600 baud
	  */
	 scc_init(3);
       }

    
    /* enable functions */
    rsp = sc->sc_regs[3];
    ssp = &sc->sc_saved_regs[3];
    ssp->wr14 |= SCC_WR14_BRGEN_EN;
    SCC_WRITE(rsp, SCC_WR14, ssp->wr14);	     /*	 WR14 BRG enable */
    ssp->wr5 |= SCC_WR5_TXEN;
    SCC_WRITE(rsp, SCC_WR5, ssp->wr5);        /*  WR5 Tx enable */
    ssp->wr3 |= SCC_WR3_RXEN;
    SCC_WRITE(rsp, SCC_WR3, ssp->wr3);	      /*  WR3 Rx enable */
}

sccopen(dev, flag)
     dev_t dev;
{
    register struct scc_softc *sc = sccsc;
    register struct tty *tp;
    register int unit;
    register int maj, error;
    int inuse;  /*hold state of inuse bit while blocked waiting for carr*/
    register struct scc_reg *rsp, *rsp0;
    register struct scc_saved_reg *ssp0;

    maj = major(dev);
    unit = minor(dev);
    /*
     * If a diagnostic console is attached to SLU line 3,
     * don't allow open of the printer port (also line 3).
     * This could cause lpr to write to the console.
     */
    if((consDev != GRAPHIC_DEV) && (unit == 3))
	return (ENXIO);
    
    /* don't allow open of minor device 0 of major device SCCMAJOR */
    /* because it is already reserved for /dev/console */
    if ((maj != CONSOLEMAJOR) && (unit == 0))
	return (ENXIO);
    /* only allow open of /dev/console of major device 0 */
    if ((maj == CONSOLEMAJOR) && (unit != 0)) 
	return (ENXIO);
    if ((consDev != GRAPHIC_DEV) && (maj == CONSOLEMAJOR) && (unit == 0))
	unit |= 3;	/* diag console on SLU line 3 */
    
    if (unit >= scc_cnt)
	return (ENXIO);
    /*
     * Call the graphics device open routine
     * if there is one and the open if for the fancy tube.
     */
    if (vs_gdopen && (unit <= 1)) {
	error = (*vs_gdopen)(dev, flag);
	if (error == 0) {
	    sccparam(unit);  
	    SCC_INT_ON(unit); /* turn on interrupts for kbd and mouse */
	}
	return(error);
    }

    tp = &scc_tty[unit];
    rsp = sc->sc_regs[unit];
    if (tp->t_state&TS_XCLUDE && u.u_uid != 0) {
	return (EBUSY);
    }
    
    while (tp->t_state&TS_CLOSING) { /* let DTR stay down for awhile */
	sleep((caddr_t)&tp->t_rawq, TTIPRI);
    }
    tp->t_addr = (caddr_t)tp;
    tp->t_oproc = sccstart;
    tp->t_baudrate = sccbaudrate;

    tty_def_open(tp, dev, flag, (sccsoftCAR&(1<<(unit&LINEMASK))));
    
    if ((tp->t_state & TS_ISOPEN) == 0) {
	/*
	 * Prevent spurious startups by making the 500ms timer
	 * initially high.
	 */
	sccmodem[unit] = MODEM_DSR_START;
	if((maj == CONSOLEMAJOR) && ((minor(dev)&3) == 0)) {
	    tp->t_cflag &= ~CBAUD;
	    tp->t_cflag |= B9600;
	    /* modem control not supported on console */ 
	    tp->t_cflag |= CLOCAL; 
	    tp->t_cflag_ext &= ~CBAUD;
	    tp->t_cflag_ext |= B9600;
	    tp->t_flags = ANYP|ECHO|CRMOD;
	    tp->t_iflag |= ICRNL; /* Map CRMOD */
	    tp->t_oflag |= ONLCR; /* Map CRMOD */
	}
    }
    sccparam(unit);	
    SCC_INT_ON(unit);    /* enable interrupts */
    (void) spltty();

    /*
     * No modem control provided for lines with softCAR set.
     */
    if (tp->t_cflag & CLOCAL) {
	/*
	 * This is a local connection - ignore carrier
	 * receive enable interrupts enabled above
	 */
	tp->t_state |= TS_CARR_ON;		
	/* 
	 * Turn off external modem interrupts
	 */
        SCC_MODEM_OFF(unit);
	SCC_SET_DTR(unit);
	SCC_SET_RTS(unit);
	SCC_SET_SS(unit);

	/*
	 * Set state bit to tell tty.c not to assign this line as the
	 * controlling terminal for the process which opens this line.
	 */
	if ((flag & O_NOCTTY) && (u.u_procp->p_progenv == A_POSIX))
	    tp->t_state |= TS_ONOCTTY;
	(void) spl0();
	return ((*linesw[tp->t_line].l_open)(dev, tp));
    }
    /*
     *  this is a modem line
     */
    SCC_MODEM_ON(unit);  /* enable modem interrupts */
    SCC_SET_DTR(unit);
    SCC_SET_RTS(unit);
    SCC_SET_SS(unit);	
	/*
	 * After DSR first comes up we must wait for the other signals
	 * before commencing transmission.
	 */
    if ((flag & (O_NDELAY|O_NONBLOCK)) == 0) {
	/*
	 * Delay before examining other signals if DSR is being followed
	 * otherwise proceed directly to scc_dsr_check to look for
	 * carrier detect and clear to send.
	 */
	if (SCC_DSR(unit)) {
	    sccmodem[unit] |= (MODEM_DSR_START|MODEM_DSR);
	    tp->t_dev = dev; /* need it for timeouts */
	    /*
	     * Give CD and CTS 30 sec. to 
	     * come up.  Start transmission
	     * immediately, no longer need
	     * 500ms delay.
	     */
	    timeout(scc_dsr_check, tp, hz*30);
	    scc_dsr_check(tp);
	}
    }
    if (flag & (O_NDELAY|O_NONBLOCK))
	tp->t_state |= TS_ONDELAY;
    else
	while ((tp->t_state & TS_CARR_ON) == 0) {
	    tp->t_state |= TS_WOPEN;
	    inuse = tp->t_state&TS_INUSE;
	    sleep((caddr_t)&tp->t_rawq, TTIPRI);
	    /*
	     * See if we were awoken by a false call to the modem
	     * line by a non-modem.
	     */
	    if (sccmodem[unit]&MODEM_BADCALL){
		(void) spl0();
		return(EWOULDBLOCK);
	    }
	    /* if we opened "block if in use"  and
	     *  the terminal was not inuse at that time
	     *  but is became "in use" while we were
	     *  waiting for carrier then return
	     */
	    if ((flag & O_BLKINUSE) && (inuse==0) &&
		(tp->t_state&TS_INUSE)) {
		(void) spl0();
		return(EALREADY);
	    }
	}
    /*
     * Set state bit to tell tty.c not to assign this line as the
     * controlling terminal for the process which opens this line.
     */
    if ((flag & O_NOCTTY) && (u.u_procp->p_progenv == A_POSIX))
	tp->t_state |= TS_ONOCTTY;
    (void) spl0();
    return ((*linesw[tp->t_line].l_open)(dev, tp));
}

sccclose(dev, flag)
     dev_t dev;
{
    register struct scc_softc *sc = sccsc;
    register struct tty *tp;
    register int unit, maj;
    register int s;
    extern int wakeup();
    
    unit = minor(dev);
    maj = major(dev);
    if((consDev != GRAPHIC_DEV) && (major(dev) == CONSOLEMAJOR) && ((unit&LINEMASK) == 0))
	unit |= 3;	/* diag console on SLU line 3 */
    /*
     * Call the graphics device close routine
     * if ther is one and the close is for it.
     */
    if (vs_gdclose && (unit <= 1)) {
	(*vs_gdclose)(dev, flag);
	return;
    }
    tp = &scc_tty[unit];
    /*
     * Do line discipline specific close functions then return here
     * in the old line disc for final closing.
     */
    if (tp->t_line)
	(*linesw[tp->t_line].l_close)(tp);
    /*
     * Clear breaks for this line on close.
     */
    s = spltty();
    SCC_CLR_BRK(unit);
    splx(s);
    if ((tp->t_cflag&HUPCL) || (tp->t_state&TS_WOPEN) || (tp->t_state&TS_ISOPEN)==0) {
	tp->t_state &= ~TS_CARR_ON;   /* prevents recv intr. timeouts */
	/*
	 * Drop appropriate signals to terminate the connection.
	 */
	SCC_CLR_DTR(unit);
	SCC_CLR_RTS(unit);
	SCC_CLR_SS(unit);
	if ((tp->t_cflag & CLOCAL) == 0) {
	    s = spltty();
	    /*drop DTR for at least a sec. if modem line*/
	    tp->t_state |= TS_CLOSING;
	    /*
	     * Wait at most 5 sec for DSR to go off.
	     * Also hold DTR down for a period.
	     */
	    if (SCC_DSR(unit)) {
		timeout(wakeup,(caddr_t)&tp->t_dev,5*hz);
		sleep((caddr_t)&tp->t_dev, PZERO-10);
	    }
	    /*
	     * Hold DTR down for 200+ ms.
	     */
	    timeout(wakeup, (caddr_t) &tp->t_dev, hz/5);
	    sleep((caddr_t)&tp->t_dev, PZERO-10);
	    
	    tp->t_state &= ~(TS_CLOSING);
	    wakeup((caddr_t)&tp->t_rawq);
	    splx(s);
	}
	/*
	 * No disabling of interrupts is done.	Characters read in on
	 * a non-open line will be discarded.
	 */
    }
    /* reset line to default mode */
    sccsoftCAR &= ~(1<<(unit&LINEMASK));
    sccsoftCAR |= (1<<(unit&LINEMASK)) & sccdefaultCAR;
    sccmodem[unit] = 0;
    /* ttyclose() must be called before clear up termio flags */
    ttyclose(tp);
    tty_def_close(tp);
}

sccread(dev, uio)
     dev_t dev;
     struct uio *uio;
{
    register struct tty *tp;
    register int unit;
    
    unit = minor(dev);
    if((consDev != GRAPHIC_DEV) && (major(dev) == CONSOLEMAJOR) && ((unit&LINEMASK) == 0))
	unit |= 3;	/* diag console on SLU line 3 */
    tp = &scc_tty[unit];
    return ((*linesw[tp->t_line].l_read)(tp, uio));
}

sccwrite(dev, uio)
     dev_t dev;
     struct uio *uio;
{
    register struct tty *tp;
    register int unit;
    
    unit = minor(dev);
    if((consDev != GRAPHIC_DEV) && (major(dev) == CONSOLEMAJOR) && ((unit&LINEMASK) == 0))
	unit |= 3;	/* diag console on SLU line 3 */
    /*
     * Don't allow writes to the mouse,
     * just fake the I/O and return.
     */
    if (vs_gdopen && (unit == 1)) {
	uio->uio_offset = uio->uio_resid;
	uio->uio_resid = 0;
	return(0);
    }
    
    tp = &scc_tty[unit];
    return ((*linesw[tp->t_line].l_write)(tp, uio));
}

sccselect(dev, rw)
     dev_t dev;
{
    register int unit = minor(dev);
    
    if((consDev != GRAPHIC_DEV) && (major(dev) == CONSOLEMAJOR) && (unit == 0))
	dev |= 3;
    if ((unit == 1) && vs_gdselect) {
	return((*vs_gdselect)(dev, rw));
    }
    return(ttselect(dev, rw));
}


sccintr()
{
    register struct scc_reg *rsp;
    register char ip;   
    register struct scc_softc *sc = sccsc;
    register int sir;   
    
    IOC_RD(IOC_SIR, sir);                  /* read sir */
    while (sir & SCC_INTR) { 
	/* error routines must appear before the regular routines */
	if (sir & SIR_COMM1_RERROR)            /* comm. port1 receive DMA error */
	    scc_dma_rerror(SCC_COMM1);
	if (sir & SIR_COMM1_RINT)              /* comm. port1 receive interrupt */
	    scc_dma_rint(SCC_COMM1);
	if (sir & SIR_COMM2_RERROR)            /* comm. port2 receive DMA error */
	    scc_dma_rerror(SCC_COMM2);
	if (sir & SIR_COMM2_RINT)              /* comm. port2 receive interrupt */
	    scc_dma_rint(SCC_COMM2);
	if (sir & SIR_COMM1_XERROR)            /* comm. port1 transmit DMA error */
	    scc_dma_xerror(SCC_COMM1);
	if (sir & SIR_COMM1_XINT)              /* comm. port1 transmit interrupt */
	    scc_dma_xint(SCC_COMM1);         
	if (sir & SIR_COMM2_XERROR)            /* comm. port2 transmit DMA error */
	    scc_dma_xerror(SCC_COMM2);
	if (sir & SIR_COMM2_XINT)              /* comm. port2 transmit interrupt */
	    scc_dma_xint(SCC_COMM2);

	if (sir & SIR_SCC1) {                  /* SCC(1) serial interrupt */
	    rsp = sc->sc_regs[SCC1_A];         /* channel A of SCC(1) */
	    SCC_READ(rsp, SCC_RR3, ip);        /* read RR3A for interrupts pending */
	    while (ip) {
		if (ip & SCC_RR3_B_EXT_IP) {   /* channel B external/status IP */
		    scc_ext_rint(SCC_COMM2);
		    sc->sc_regs[SCC1_B]->SCC_CMD = SCC_WR0_RESET_EXT_INT<<8;
		}
		if (ip & SCC_RR3_A_EXT_IP) {   /* channel A external/status IP */
		    scc_ext_rint(SCC_COMM2);
		    sc->sc_regs[SCC1_A]->SCC_CMD = SCC_WR0_RESET_EXT_INT<<8;
		}
		if (ip & SCC_RR3_B_RIP)        /* channel B receive IP */
		    scc_spec_rint(SCC_COMM2);    
		if (ip & SCC_RR3_A_RIP)        /* channel A receive IP */
		    scc_mkbd_rint(SCC_KYBD);
		SCC_READ(rsp, SCC_RR3, ip);    /* read RR3A */
	    }
	}
	if (sir & SIR_SCC0) {                  /* SCC(0) serial interrupt */
	    rsp = sc->sc_regs[SCC0_A];
	    SCC_READ(rsp, SCC_RR3, ip);        /* read RR3A */
	    while (ip) {
		IOC_RD(IOC_SIR, sir);
		if (ip & SCC_RR3_B_EXT_IP) {
		    scc_ext_rint(SCC_COMM1);
		    sc->sc_regs[SCC0_B]->SCC_CMD = SCC_WR0_RESET_EXT_INT<<8;
		    SCC_READ(rsp, SCC_RR3, ip);  
		}
		if (ip & SCC_RR3_A_EXT_IP) {
		    scc_ext_rint(SCC_COMM1);
		    sc->sc_regs[SCC0_A]->SCC_CMD = SCC_WR0_RESET_EXT_INT<<8;
		}
		if (ip & SCC_RR3_B_RIP)
		    scc_spec_rint(SCC_COMM1);
		if (ip & SCC_RR3_A_RIP)
		    scc_mkbd_rint(SCC_MOUSE);
		SCC_READ(rsp, SCC_RR3, ip);    /* read RR3A */
	    }
	}
	IOC_RD(IOC_SIR, sir);                  /* read sir */
    } /* while */
}

/* should never happen */
scc_dma_rerror(line)
     register int line;
{
    register struct scc_softc *sc = sccsc;

    printf("Com. Port. Receive DMA Overrun, line = %d\n", line);
    sc->ioc_regs[line]->RDMA_REG = (u_long)   /* reset DMA pointer */
	(svtophy(sc->rbuf[line][sc->rflag[line]] 
	+ SCC_HALF_PAGE - SCC_WORD) << 3);
    IOC_WR(IOC_SIR, ~scc_rerror[line]);	      /* clear error bit to restart */
}

scc_dma_xerror(line)
     register int line;
{
    register struct scc_softc *sc = sccsc;
    caddr_t pa;
    struct tc_memerr_status status;

    pa = (caddr_t)((sc->ioc_regs[line]->XDMA_REG) >> 3);
    printf("Com. Port. Transmit DMA Read Error, line = %d\n", line);
    status.pa = pa;
    status.va = 0;
    status.log = TC_LOG_MEMERR;
    status.blocksize = 1;
    tc_isolate_memerr(&status);
    IOC_SET(IOC_SSR, scc_xdma_en[line]);     /* enable transmit DMA */
    IOC_WR(IOC_SIR, ~scc_xerror[line]);
}

/*
 * Used to pass mouse (or tablet) reports to the graphics
 * device driver interrupt service routine.
 * Entire report passed instead of byte at a time.
 */
struct	mouse_report	current_rep;
u_short pointer_id;
#define MOUSE_ID	0x2

scc_mkbd_rint(unit)
     register int unit;
{
    register struct scc_softc *sc = sccsc;
    register u_short ch;	/* ch is 16 bits long */
    register u_short c;
    struct mouse_report *new_rep;
    u_short data;
    register struct scc_reg *rsp;
    register char ip;
    
    /*
     * If console is a graphics device,
     * pass keyboard input characters to
     * its device driver's receive interrupt routine.
     * Save up complete mouse report and pass it.
     */
    if ((unit <= 1) && vs_gdkint) {
	new_rep = &current_rep;			/* mouse report pointer */
	rsp = sc->sc_regs[unit];                      /* line = mouse or keybd */
	SCC_READ(rsp, SCC_RR3, ip);                   /* read IP bits from RR3A */
	while (ip & SCC_RR3_A_RIP) {                /* channel A receiver */
	    c = (rsp->SCC_DATA)>>8;			/* read char */
	    ch = (unit<<8)| c;                  /* encode line in ch */
	    if(unit == 0) {		/* keyboard char */
		(*vs_gdkint)(ch);
	    } else {			/* mouse or tablet report */
		if (pointer_id == MOUSE_ID) { /* mouse report */
		    data = ch & 0xff;	/* get report byte */
		    ++new_rep->bytcnt;	/* inc report byte count */
		    if (data & START_FRAME) { /* 1st byte of report? */
			new_rep->state = data;
			if (new_rep->bytcnt > 1)
			    new_rep->bytcnt = 1;  /* start new frame */
		    }
		    else if (new_rep->bytcnt == 2) {	/* 2nd byte */
			new_rep->dx = data;
		    }
		    else if (new_rep->bytcnt == 3) {	/* 3rd byte */
			new_rep->dy = data;
			new_rep->bytcnt = 0;
			(*vs_gdkint)(0400); /* 400 says line 1 */
		    }
		} else { /* tablet report */
		    data = ch;	/* get report byte */
		    ++new_rep->bytcnt;	/* inc report byte count */
		    
		    if (data & START_FRAME) { /* 1st byte of report? */
			new_rep->state = data;
			if (new_rep->bytcnt > 1)
			    new_rep->bytcnt = 1;  /* start new frame */
		    }
		    else if (new_rep->bytcnt == 2)	/* 2nd byte */
			new_rep->dx = data & 0x3f;
		    
		    else if (new_rep->bytcnt == 3)	/* 3rd byte */
			new_rep->dx |= (data & 0x3f) << 6;
		    
		    else if (new_rep->bytcnt == 4)	/* 4th byte */
			new_rep->dy = data & 0x3f;
		    
		    else if (new_rep->bytcnt == 5){	/* 5th byte */
			new_rep->dy |= (data & 0x3f) << 6;
			new_rep->bytcnt = 0;
			(*vs_gdkint)(0400); /* 400 says line 1 */
		    }
		}
	    }
	    SCC_READ(rsp, SCC_RR3, ip);         /* read IP bits again */
	} /* while */
    } /* if */ 
}


scc_spec_rint(line)
     register int line;
{
    scc_dma_rint(line);
}


scc_dma_rint(unit)
     register int unit;
{
    register struct scc_softc *sc = sccsc;
    register struct tty *tp = &scc_tty[unit];
    register u_short c;	
    register int flg;
    register int status, vector, status0;
    register int num;
    register char *endptr;
    register char *ptr;
    register struct scc_reg *rsp;
    register char ip;
    int overrun = 0;
    int counter = 0;
    register int c_mask;

    IOC_CLR(IOC_SSR, scc_rdma_en[unit]);    /* disable rdma */
    IOC_WR(IOC_SIR, ~(scc_rint[unit]));       /* clear rdma int */

    /* compute the offset from the beginning of the page */
    /* read rdma ptr */
    endptr = (char *)(((sc->ioc_regs[unit]->RDMA_REG) >> 3) & 0xfff); 
    /* add the base of the receive buffer to the endptr */
    num = sc->rflag[unit];     /* save flag */
    /* pgt - check for boundary crossing */
    if (endptr == 0) 
	endptr += (long)(sc->rbuf[unit][num]) + SCC_PAGE_SIZE ;
    else 
        endptr += (long)(sc->rbuf[unit][num]);
    endptr -= SCC_WORD;        /* move to previous word */

    sc->rflag[unit] ^= 1;      /* toggle buffer flag */
    /*
     * set rdma ptr to other buffer 
     */
    sc->ioc_regs[unit]->RDMA_REG = (u_long)(svtophy(sc->rbuf[unit][sc->rflag[unit]] 
					    + SCC_HALF_PAGE - SCC_WORD) << 3);
    
    /* unit = comm1 or comm2 */
    rsp = sc->sc_regs[scc_other[unit]];   /* channel A */
    SCC_READ(rsp, SCC_RR3, ip);		/* read IP from RR3A */
    if (ip & SCC_RR3_B_RIP) {             /* channel B receiver */
	rsp = sc->sc_regs[unit];   /* channel B */
	SCC_READ(rsp, SCC_RR1, status);	/* read status */    
	rsp->SCC_CMD = (SCC_WR0_ERROR_RESET)<<8;
    } else {
	status = 0;
    }
    
    IOC_SET(IOC_SSR, scc_rdma_en[unit]);     /* enable rdma */
    if ((tp->t_state & TS_ISOPEN) == 0) {
	wakeup((caddr_t)&tp->t_rawq);
	return;
    }
    flg = tp->t_iflag;
    
    ptr = sc->rbuf[unit][num] + SCC_HALF_PAGE - SCC_WORD;

    /* need to do following because SCC sets unused bits to ones */
    switch(tp->t_cflag&CSIZE) {
      case CS5:
	c_mask = 0x1f;
	break;
      case CS6:
	c_mask = 0x3f;
	break;
      case CS7:
	c_mask = 0x7f;
	break;
      case CS8:
	c_mask = 0xff;
    }
    while (ptr < endptr) {
	c = ((*(u_short *)ptr)>>8)&c_mask;
	ptr += SCC_WORD;
	
	if (flg & ISTRIP){
	    c &= 0177;
	} else {
	    c &= 0377;
	    if ((c == 0377) && (tp->t_line == TERMIODISC) &&
		(flg & PARMRK))
		(*linesw[tp->t_line].l_rint)(0377,tp);
	}
	(*linesw[tp->t_line].l_rint)(c, tp);
    }

    c = ((*(u_short *)ptr)>>8)&c_mask;
    /*
     *		This code handles the following termio input flags.  Also
     *		listed is what the default should be for propper Ultrix
     *		backward compatibility.
     *
     *		IGNBRK		FALSE
     *		BRKINT		TRUE
     *		IGNPAR		TRUE
     *		PARMRK		FALSE
     *		INPCK		TRUE
     *		ISTRIP		TRUE
     */
    if (status) {
	/* SCC_FE is interpreted as a break */
	if (status & SCC_RR1_FE) {  
	    if (do_tpath) {
		tp->t_tpath |= TP_DOSAK;
		(*linesw[tp->t_line].l_rint)(c, tp);
		return;
	    }
	    if (flg & IGNBRK)
		return;
	    if (flg & BRKINT) {
		if ((tp->t_lflag_ext & PRAW) && 
		    (tp->t_line != TERMIODISC))
		    c = 0;
		else {
		    ttyflush(tp, FREAD | FWRITE);
		    gsignal(tp->t_pgrp, SIGINT);
		    return;
		}
	    }
	    /*
	     * TERMIO: If neither IGNBRK or BRKINT is set, a
	     * break condition is read as a single '\0',
	     * or if PARMRK is set as '\377', '\0' , '\0'.
	     */
	    else {
		if (flg & PARMRK){
		    (*linesw[tp->t_line].l_rint)(0377,tp);
		    (*linesw[tp->t_line].l_rint)(0,tp);
		}
		c = 0;
	    }
	}
	/* Parity Error */
	else 
	    if (status & SCC_RR1_PE){
	    /*
	     * If input parity checking is not enabled, clear out
	     * parity error in this character.
	     */
	    if ((flg & INPCK) == 0)
		;	     /* don't do anything */
	    else {
		if (flg & IGNPAR)
		    return;
		/* If PARMRK is set, return a character with
		 * framing or parity errors as a 3 character
		 * sequence (0377,0,c).
		 */
		if (flg & PARMRK){
		    (*linesw[tp->t_line].l_rint)(0377,tp);
		    (*linesw[tp->t_line].l_rint)(0,tp);
		}
		/*
		 * TERMIO: If neither PARMRK or IGNPAR is set, a
		 * parity error is read as a single '\0'.
		 */
		else
		    c = 0;
	    }
	}
	
	/* SVID does not say what to do with overrun errors */
	if (status & SCC_RR1_DO) {
	    if(overrun == 0) {
		printf("scc%d: input silo overflow\n", 0);
		overrun = 1;
	    }
	    sc->sc_softcnt[unit]++;
	}
    }
    if (flg & ISTRIP){
	c &= 0177;
    } else {
	c &= 0377;
	/* 
	 * If ISTRIP is not set a valid character of 377
	 * is read as 0377,0377 to avoid ambiguity with
	 * the PARMARK sequence.
	 */
	if ((c == 0377) && (tp->t_line == TERMIODISC) && (flg & PARMRK))
	    (*linesw[tp->t_line].l_rint)(0377,tp);
    }
    (*linesw[tp->t_line].l_rint)(c, tp);
}

/*ARGSUSED*/
sccioctl(dev, cmd, data, flag)
     dev_t dev;
     register int cmd;
     caddr_t data;
     int flag;
{
    register struct scc_softc *sc = sccsc;
    register int unit;
    register struct tty *tp;
    register int s;
    struct uba_device *ui;
    struct devget *devget;
    int error;
    register struct scc_reg *rsp;
    register int status;
    register int timo;

    unit = minor(dev);
    if((consDev != GRAPHIC_DEV) && (major(dev) == CONSOLEMAJOR) && ((unit&LINEMASK) == 0))
	unit |= 3;	/* diag console on SLU line 3 */
    
    /*
     * If there is a graphics device and the ioctl call
     * is for it, pass the call to the graphics driver.
     */
    if (vs_gdioctl && (unit <= 1)) {
	return((*vs_gdioctl)(dev, cmd, data, flag));
    }
    tp = &scc_tty[unit];
    error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
    if (error >= 0)
	return (error);
    error = ttioctl(tp, cmd, data, flag);
    if (error >= 0) {
	/*
	 * If the call is to set terminal attributes which are
	 * represented in the device's line parameter register then
	 * call the param routine to update the device registers.
	 */
	switch(cmd) {
	  case TCSANOW:			/* POSIX termios */
	  case TCSADRAIN:		/* POSIX termios */
	  case TCSAFLUSH:		/* POSIX termios */
	  case TCSETA:			/* SVID termio */
	  case TCSETAW:			/* SVID termio */
	  case TCSETAF:			/* SVID termio */
	  case TIOCSETP:		/* Berkeley sgttyb */
	  case TIOCSETN:		/* Berkeley sgttyb */
	  case TIOCLBIS:		/* Berkeley lmode */
	  case TIOCLBIC:		/* Berkeley lmode */
	  case TIOCLSET:		/* Berkeley lmode */
	  case TIOCLGET:		/* Berkeley lmode */
	    sccparam(unit);
	    break;
	}
	return (error);
    }
    switch (cmd) {
	
      case TIOCSBRK:
	s = spltty();
	rsp = sc->sc_regs[unit];
	SCC_READ(rsp, SCC_RR1, status);	/* read status */
	for(timo=10000; timo > 0; --timo) {
	    if (status & SCC_RR1_ALL_SENT) 
		break;	
	    else {
		SCC_READ(rsp, SCC_RR1, status);
	    }
	}

	SCC_SET_BRK(unit);
	splx(s);
	break;
	
      case TIOCCBRK:
	s = spltty();
	SCC_CLR_BRK(unit);
	splx(s);
	break;
      case TIOCSDTR:
	(void) sccmctl(dev, DC_DTR | DC_RTS, DMBIS);
	break;
	
      case TIOCCDTR:
	(void) sccmctl(dev, DC_DTR | DC_RTS, DMBIC);
	break;
	
      case TIOCMSET:
	(void) sccmctl(dev, dmtoscc(*(int *)data), DMSET);
	break;
	
      case TIOCMBIS:
	(void) sccmctl(dev, dmtoscc(*(int *)data), DMBIS);
	break;
	
      case TIOCMBIC:
	(void) sccmctl(dev, dmtoscc(*(int *)data), DMBIC);
	break;
	
      case TIOCMGET:
	*(int *)data = scctodm(sccmctl(dev, 0, DMGET));
	break;
	
      case TIOCNMODEM:  /* ignore modem status */
	/*
	 * By setting the software representation of modem signals
	 * to "on" we fake the system into thinking that this is an
	 * established modem connection.
	 */
	s = spltty();
	sccsoftCAR |= (1<<(unit&LINEMASK));
	if (*(int *)data) /* make mode permanent */
	    sccdefaultCAR |= (1<<(unit&LINEMASK));
	tp->t_state |= TS_CARR_ON;
	tp->t_cflag |= CLOCAL;		/* Map to termio */
	SCC_MODEM_OFF(unit);
	splx(s);
	break;
	
      case TIOCMODEM:  
	s = spltty();
	SCC_MODEM_ON(unit);
	sccsoftCAR &= ~(1<<(unit&LINEMASK));
	if (*(int *)data) /* make mode permanent */
	    sccdefaultCAR &= ~(1<<(unit&LINEMASK));
	/*
	 * See if signals necessary for modem connection are present
	 */
	if (SCC_XMIT(unit)) {
	    tp->t_state &= ~(TS_ONDELAY);
	    tp->t_state |= TS_CARR_ON;
	    sccspeedi(unit);  /* check speed indicate */
	    sccmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
	} else {
	    tp->t_state &= ~(TS_CARR_ON);
	    sccmodem[unit] &= ~(MODEM_CTS|MODEM_CD|MODEM_DSR);
	}
	tp->t_cflag &= ~CLOCAL;		/* Map to termio */
	splx(s);
	break; 
	
      case TIOCWONLINE: /* look at modem status - sleep if no carrier */
	s = spltty();
	/*
	 * See if signals necessary for modem connection are present
	 */
	if (SCC_XMIT(unit)) {
	    tp->t_state |= TS_CARR_ON;
	    sccspeedi(unit);
	    tp->t_state &= ~(TS_ONDELAY);
	    sccmodem[unit] = MODEM_CTS|MODEM_CD|MODEM_DSR;
	} else {
	    while ((tp->t_state & TS_CARR_ON) == 0)
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	}
	splx(s);
	break;
	
      case DEVIOCGET:				/* device status */
	devget = (struct devget *)data;
	bzero(devget,sizeof(struct devget));
	if (tp->t_cflag & CLOCAL) {
	    sc->sc_category_flags[unit&LINEMASK] |= DEV_MODEM;
	    sc->sc_category_flags[unit&LINEMASK] &= ~DEV_MODEM_ON;
	} else
	    sc->sc_category_flags[unit&LINEMASK] |= (DEV_MODEM|DEV_MODEM_ON);
	devget->category = DEV_TERMINAL;	/* terminal cat.*/
	devget->bus = DEV_NB;			/* NO bus	*/
	bcopy(DEV_VS_SLU,devget->interface,
	      strlen(DEV_VS_SLU));		/* interface	*/
	bcopy(DEV_UNKNOWN,devget->device,
	      strlen(DEV_UNKNOWN));		/* terminal	*/
	devget->adpt_num = 0;			/* NO adapter	*/
	devget->nexus_num = 0;			/* fake nexus 0 */
	devget->bus_num = 0;			/* NO bus	*/
	devget->ctlr_num = 0;			/* cntlr number */
	devget->slave_num = unit&LINEMASK;	/* line number	*/
	bcopy("scc", devget->dev_name, 3);	/* Ultrix "scc"	*/
	devget->unit_num = unit&LINEMASK;	/* scc line?	*/
	devget->soft_count =
	    sc->sc_softcnt[unit&LINEMASK];	/* soft err cnt */
	devget->hard_count =
	    sc->sc_hardcnt[unit&LINEMASK];	/* hard err cnt */
	devget->stat = sc->sc_flags[unit&LINEMASK]; /* status	*/
	devget->category_stat =
	    sc->sc_category_flags[unit&LINEMASK]; /* cat. stat. */
	break;
	
      default:
	if (u.u_procp->p_progenv == A_POSIX) 
	    return (EINVAL);
	return (ENOTTY);
    }
    return (0);
}      


dmtoscc(bits)
     register int bits;
{
    register int b;
    
    b = (bits >>1) & 0370;
    if (bits & SML_ST) b |= DC_ST;
    if (bits & SML_RTS) b |= DC_RTS;
    if (bits & SML_DTR) b |= DC_DTR;
    if (bits & SML_LE) b |= DC_LE;
    return(b);
}

scctodm(bits)
     register int bits;
{
    register int b;
    
    b = (bits << 1) & 0360;
    if (bits & DC_DSR) b |= SML_DSR;
    if (bits & DC_DTR) b |= SML_DTR;
    if (bits & DC_ST) b |= SML_ST;
    if (bits & DC_RTS) b |= SML_RTS;
    return(b);
}

sccparam(unit)
     register int unit;
{
    register struct scc_softc *sc = sccsc;
    register struct tty *tp;
    register struct scc_reg *rsp, *rsp0;
    register struct scc_saved_reg *ssp, *ssp0;
    register int rxen = 1, s, status;
    register int timo;
    
    s = spltty();
    tp = &scc_tty[unit];

    if (tp->t_state & TS_BUSY) {
        tp->t_state |= TS_NEED_PARAM;
	return;
    }

    rsp = sc->sc_regs[unit];
    SCC_READ(rsp, SCC_RR1, status);	/* read status */
    for(timo=10000; timo > 0; --timo) {
        if (status & SCC_RR1_ALL_SENT) 
	    break;	
	else {
	    SCC_READ(rsp, SCC_RR1, status);
	}
    }

    ssp = &sc->sc_saved_regs[unit];
    
    if ((((tp->t_cflag&CBAUD)==B0) && (u.u_procp->p_progenv != A_POSIX)) ||
	(((tp->t_cflag_ext & CBAUD)==B0) &&
	 (u.u_procp->p_progenv == A_POSIX))) {
	SCC_CLR_DTR(unit);
	SCC_CLR_RTS(unit);
	SCC_CLR_SS(unit);
	splx(s);
	return;
    }
    /*
     * If diagnostic console on line 3,
     * line parameters must be: 9600 BPS, 8 BIT, NO PARITY, 1 STOP.
     */
    if ((unit == 3) && (consDev != GRAPHIC_DEV)) {
	/* 
         * do nothing here because line 3 parameters have already 
	 * been set in scc_cons_init.
	 */
	;
    } else if (unit == 2 || unit == 3) {
        /*
	 * Set parameters in accordance with user specification.
	 */
	ssp->wr4 = SCC_WR4_CLOCK16;
	/*
	 * Berkeley-only dinosaur
	 */
	if (tp->t_line != TERMIODISC) {
	    if ((tp->t_cflag_ext&CBAUD) == B110)
		tp->t_cflag |= CSTOPB;
	}
	/*
	 * Set device registers according to the specifications of the
	 * termio structure.
	 */
	if ((tp->t_cflag & CREAD) == 0)
	    /* disable receiver */
	    rxen = 0;            
	
	if (tp->t_cflag & CSTOPB) 
	    ssp->wr4 |= SCC_WR4_TWOSB;
	else
	    ssp->wr4 |= SCC_WR4_ONESB;
	if (tp->t_cflag & PARENB) {
	    if ((tp->t_cflag & PARODD) == 0) 
		/* set even */
		ssp->wr4 |= (SCC_WR4_EPAR | SCC_WR4_PENABLE);
	    else
		/* else set odd */
		ssp->wr4 |= SCC_WR4_PENABLE;
	}
	SCC_WRITE(rsp, SCC_WR4, ssp->wr4);
	/*
	 * character size.
	 * clear bits and check for 5, 6, 7 & 8 bits.
	 */
	ssp->wr3 &= ~(SCC_WR3_RBITS|SCC_WR3_RXEN);
	ssp->wr5 &= ~(SCC_WR5_TBITS|SCC_WR5_TXEN);
	switch(tp->t_cflag&CSIZE) {
	  case CS5:
	    ssp->wr3 |= SCC_WR3_RBITS5;
	    ssp->wr5 |= SCC_WR5_TBITS5;
	    break;
	  case CS6:
	    ssp->wr3 |= SCC_WR3_RBITS6;
	    ssp->wr5 |= SCC_WR5_TBITS6;
	    break;
	  case CS7:
	    ssp->wr3 |= SCC_WR3_RBITS7;
	    ssp->wr5 |= SCC_WR5_TBITS7;
	    break;
	  case CS8:
	    ssp->wr3 |= SCC_WR3_RBITS8;
	    ssp->wr5 |= SCC_WR5_TBITS8;
	    break;
	}
	SCC_WRITE(rsp, SCC_WR3, ssp->wr3);
	SCC_WRITE(rsp, SCC_WR5, ssp->wr5);
	ssp->wr14 &= ~(SCC_WR14_BRGEN_EN);
	SCC_WRITE(rsp, SCC_WR14, ssp->wr14);	     /*	 WR14 BRG disable */
	SCC_WRITE(rsp, SCC_WR12, scc_speeds[tp->t_cflag&CBAUD].baud_lo);
	SCC_WRITE(rsp, SCC_WR13, scc_speeds[tp->t_cflag&CBAUD].baud_hi);
	scc_cbaud[unit] = tp->t_cflag&CBAUD;
	if ((tp->t_cflag & CLOCAL) == 0)
	    sccspeedi(unit);  /* check speed indicate */

	/* 
	 * enable functions 
	 */
	ssp->wr14 |= SCC_WR14_BRGEN_EN;
	SCC_WRITE(rsp, SCC_WR14, ssp->wr14);	     /*	 WR14 BRG enable */
	if (rxen) {
	    ssp->wr3 |= SCC_WR3_RXEN;
	    SCC_WRITE(rsp, SCC_WR3, ssp->wr3);	      /*  WR3 Rx enable */
	}
	ssp->wr5 |= SCC_WR5_TXEN;
	SCC_WRITE(rsp, SCC_WR5, ssp->wr5);        /*  WR5 Tx enable */
    }
    splx(s);
}


/*
 * scc_dma_xint(unit) - transmit DMA interrupt service routine
 *	
 * algorithm:
 *	-clear transmit DMA interrupt
 *  	-disable transmit DMA for comm. port designated by 'unit'
 * 	-same as dcxint with pdma stuff removed and new ndflush:
 *		-get tty for 'unit'
 *		-reset 'unit' to 3 if line 0 is console
 *		-set t_state 
 *		-flush 'cc' bytes from output queue
 *              -if we need param, call sccparam
 *		-invoke start routine
 *
 */
scc_dma_xint(unit)
     register int unit; 	
{
    register struct scc_softc *sc = sccsc;
    register struct tty *tp;
    register char *ptr;
    register struct scc_reg *rsp;
    register int cc;

    /* must disable the transmit DMA before clearing the interrupt */
    IOC_CLR(IOC_SSR, scc_xdma_en[unit]);    /* disable transmit DMA */
    IOC_WR(IOC_SIR, ~(scc_xint[unit]));       /* clear transmit int */
    /* read transmit DMA offset */
    ptr = (char *)(((sc->ioc_regs[unit]->XDMA_REG) >> 3) & 0xfff);
    if (ptr == 0) 
	ptr += (u_long)sc->tbuf[unit] + SCC_PAGE_SIZE;
    else 
	ptr += (u_long)sc->tbuf[unit];
    
    tp = &scc_tty[unit];
    if ((consDev != GRAPHIC_DEV) && (unit == 0) && /* ? */
	(major(tp->t_dev) == CONSOLEMAJOR)) {
	unit = 3;
    }
    
    tp->t_state &= ~TS_BUSY;

    cc = (u_long *)ptr - (u_long *)sc->tptr[unit]; /* cc bytes */
    ndflush(&tp->t_outq, cc); /* cc bytes */

    if (tp->t_state & TS_NEED_PARAM) {
        tp->t_state &= ~TS_NEED_PARAM;
	sccparam(unit);
    }
    if (tp->t_line) {
	(*linesw[tp->t_line].l_start)(tp);
    } else  {
	sccstart(tp);
    }
}


sccstart(tp)
     register struct tty *tp;
{	
    register struct scc_softc *sc = sccsc;
    register int cc;
    int s, unit;
    register char *bp;
    register char *cp;
    register int c_mask;
    
    s = spltty();
    /*
     * Do not do anything if currently delaying, or active.  Also only
     * transmit when CTS is up.
     */
    unit = minor(tp->t_dev) & 3;
    if ((tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) || 
	(((tp->t_cflag & CLOCAL) == 0) && 
	 ((tp->t_state&TS_CARR_ON) && (sccmodem[unit]&MODEM_CTS)==0))) 
        goto out;

    if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
	if (tp->t_state&TS_ASLEEP) {
	    tp->t_state &= ~TS_ASLEEP;
	    wakeup((caddr_t)&tp->t_outq);
	}
	if (tp->t_wsel) {
	    selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
	    tp->t_wsel = 0;
	    tp->t_state &= ~TS_WCOLL;
	}
    }
    if (tp->t_outq.c_cc == 0)
	goto out;

    if ((tp->t_lflag_ext & PRAW) || (tp->t_oflag_ext & PLITOUT) ||
	((tp->t_oflag & OPOST) == 0)) {
	cc = ndqb(&tp->t_outq, 0);
    } else {
	cc = ndqb(&tp->t_outq, DELAY_FLAG);
	if (cc == 0) {
	    cc = getc(&tp->t_outq);
	    timeout(ttrstrt, (caddr_t)tp, (cc&0x7f) + 6);
	    tp->t_state |= TS_TIMEOUT;
	    goto out;
	}
    }
    tp->t_state |= TS_BUSY;
    if ((consDev != GRAPHIC_DEV) && (unit == 0) && /* ? */
	(major(tp->t_dev) == CONSOLEMAJOR))
	unit = 3;
    
    /*
     * prepare for DMA:
     *	-point cp to first char in clist block,
     * 	-point bp to cc words from end of page,
     *	-save bp in scc_softc for use by ndflush.
     *	-using cp and bp, copy cc bytes from clist to
     *		word-aligned DMA page.
     */
    cp = tp->t_outq.c_cf;
    bp = sc->tbuf[unit] + SCC_PAGE_SIZE - (cc * SCC_WORD);
    sc->tptr[unit] = bp;
    /*
     * need to do the following because when character size is set to five,
     * the data format allows character sizes of one to five. See SCC
     * manual.
     */
    if ((tp->t_cflag&CSIZE) == CS5)
       c_mask = 0x1f;
    else
       c_mask = 0xff;
    while (cc-- > 0) {
	*(u_short *)bp = (((u_short)*cp++)&c_mask)<<8;
	bp += SCC_WORD;
    }
    /*
     * set DMA transmit ptr
     */
    sc->ioc_regs[unit]->XDMA_REG = (u_long)(svtophy(sc->tptr[unit]) << 3);
    IOC_SET(IOC_SSR, scc_xdma_en[unit]);     /* enable transmit DMA */
  out:	

    splx(s);
}

sccstop(tp, flag)
     register struct tty *tp;
{
    register struct scc_softc *sc = sccsc;
    register int s, cc;
    register char *ptr;
    int	unit;
    
    /*
     * If there is a graphics device and the stop call
     * is for it, pass the call to the graphics device driver.
     */
    unit = minor(tp->t_dev);
    if ((consDev != GRAPHIC_DEV) && (unit == 0) && /* ? */
	(major(tp->t_dev) == CONSOLEMAJOR)) {
	unit = 3;
    }
    if (vs_gdstop && (unit <= 1)) {
	(*vs_gdstop)(tp, flag);
	return;
    }
    
    s = spltty();
    if (tp->t_state & TS_BUSY) {
	/* disable transmit DMA */
	IOC_CLR(IOC_SSR, scc_xdma_en[unit]); 
	/* 
	 * pgt - need to clear transmit int to handle boundary condition, 
	 * otherwise scc_dma_xint could be called and queue would be 
	 * ndflushed twice
	 */
	IOC_WR(IOC_SIR, ~(scc_xint[unit]));       /* clear transmit int */
	/* line discipline will flush entire queue */
	if ((tp->t_state&TS_TTSTOP)==0) {
	    ;
	} else { /* suspend */
	    /* read transmit DMA ptr */
	    ptr = (char *)(((sc->ioc_regs[unit]->XDMA_REG) >> 3) & 0xfff);
	    /* if we made it to a page boundary pointer will be zero */
	    if (ptr == 0)
		ptr += (long)(sc->tbuf[unit]) + SCC_PAGE_SIZE ;
	    else 
		ptr += (long)(sc->tbuf[unit]);
	    cc = (u_long *)ptr - (u_long *)sc->tptr[unit]; /* cc bytes */
	    ndflush(&tp->t_outq, cc); /* cc bytes */
	}
	tp->t_state &= ~TS_BUSY;
    }
    splx(s);
}


sccmctl(dev, bits, how)
     dev_t dev;
     int bits, how;
{
    register struct scc_softc *sc = sccsc;
    register int unit, mbits;
    int b, s;
    
    unit = minor(dev);
    if ((unit != 2) && (unit != 3))
	return(0);	/* only line 2 and 3 has modem control */
    s = spltty();
    mbits = (SCC_DTR(unit)) ? DC_DTR : 0;
    mbits |= (SCC_RTS(unit)) ? DC_RTS : 0;
    mbits |= (SCC_DCD(unit)) ? DC_CD : 0;
    mbits |= (SCC_DSR(unit)) ? DC_DSR : 0;
    mbits |= (SCC_CTS(unit)) ? DC_CTS : 0;
    switch (how) {
      case DMSET:
	mbits = bits;
	break;
	
      case DMBIS:
	mbits |= bits;
	break;
	
      case DMBIC:
	mbits &= ~bits;
	break;
	
      case DMGET:
	(void) splx(s);
	return(mbits);
    }
    if (mbits & DC_DTR) {
	SCC_SET_DTR(unit);
	SCC_SET_RTS(unit);
	SCC_SET_SS(unit);
    } else {
	SCC_CLR_DTR(unit);
	SCC_CLR_RTS(unit);
	SCC_CLR_SS(unit);
    }
    (void) splx(s);
    return(mbits);
}


scc_ext_rint(i)
     register i;
{
    register struct scc_softc *sc = sccsc;
    register struct tty *tp;
    register struct scc_reg *rsp;
    register int brk;

    tp = &scc_tty[i];
    /*
     * assume break condition will last long enough for it to be processed by
     * this routine. otherwise, null characters would be DMA'd into the receive
     * buffer.
     */
    if (i == 2 || i == 3) {
        rsp = sc->sc_regs[i];
	brk = ((rsp->SCC_CMD)>>8) & SCC_RR0_BREAK;
	/* check if there is a state change in BREAK */
	if (brk && !scc_brk[i])  {
	    scc_brk[i] = 1;	
	    sccbrkint(i);
	} else if (!brk && scc_brk[i]) {
	    scc_brk[i] = 0;
	    sccbrkint(i);
	} else if (brk && scc_brk[i]) {
	    /*
	     * handle the case where successive breaks arrive
	     * need to call brkint twice to handle the termination of an
	     * earlier break and the beginning of the next break
	     */
	    scc_brk[i] = 0;
	    sccbrkint(i);
	    scc_brk[i] = 1;
	    sccbrkint(i);
	}
    } 

    if ((tp->t_cflag & CLOCAL) == 0) {
	sccspeedi(i);	/* check for speed indicate changes */
	/*
	 * Drop DTR immediately if DSR has gone away.
	 * If really an active close then do not
	 *    send signals.
	 */
	if (!(SCC_DSR(i))) {
	    if (tp->t_state&TS_CLOSING) {
		untimeout(wakeup, (caddr_t) &tp->t_dev);
		wakeup((caddr_t) &tp->t_dev);
	    }
	    if (tp->t_state&TS_CARR_ON) {
		scc_tty_drop(tp);
	    }
	} else {		/* DSR has come up */
	    /*
	     * If DSR comes up for the first time we allow
	     * 30 seconds for a live connection.
	     */
	    if ((sccmodem[i] & MODEM_DSR)==0) {
		sccmodem[i] |= (MODEM_DSR_START|MODEM_DSR);
		/*
		 * we should not look for CTS|CD for about
		 * 500 ms.
		 */
		timeout(scc_dsr_check, tp, hz*30);
		scc_dsr_check(tp);}

	}
	
	/*
	 * look for modem transitions in an already
	 * established connection.
	 */
	if (tp->t_state & TS_CARR_ON) {
	    if (SCC_DCD(i)) {
		/*
		 * CD has come up again.
		 * Stop timeout from occurring if set.
		 * If interval is more than 2 secs then
		 * drop DTR.
		 */
		if ((sccmodem[i] & MODEM_CD) == 0) {
		    untimeout(scc_cd_drop, tp);
		    if (scc_cd_down(tp)) {
			/* drop connection */
			scc_tty_drop(tp);
		    }
		    sccmodem[i] |= MODEM_CD;
		}
	    } else {
		/*
		 * Carrier must be down for greater than
		 * 2 secs before closing down the line.
		 */
		if (sccmodem[i] & MODEM_CD) {
		    /* only start timer once */
		    sccmodem[i] &= ~MODEM_CD;
		    /*
		     * Record present time so that if carrier
		     * comes up after 2 secs, the line will drop.
		     */
		    scctimestamp[i] = time;
		    timeout(scc_cd_drop, tp, hz * 2);
		}
	    }
	    
	    /* CTS flow control check */
	    
	    if (!(SCC_CTS(i))) {
		/*
		 * Only allow transmission when CTS is set.
		 */
		tp->t_state |= TS_TTSTOP;
		sccmodem[i] &= ~MODEM_CTS;
		sccstop(tp, 0);
	    } else if (!(sccmodem[i] & MODEM_CTS)) {
		/*
		 * Restart transmission upon return of CTS.
		 */
		tp->t_state &= ~TS_TTSTOP;
		sccmodem[i] |= MODEM_CTS;
		sccstart(tp);
	    }
	}
	
	/*
	 * See if a modem transition has occured.  If we are waiting
	 * for this signal, cause action to be take via
	 * scc_start_tty.
	 */
	if ((SCC_XMIT(i)) &&
	    (!(sccmodem[i] & MODEM_DSR_START)) &&
	    (!(tp->t_state & TS_CARR_ON))) {
	    scc_start_tty(tp);
	}
    }
}

/*
 * Note: When a break condition occurs, the SCC chip detects the condition,
 *       sets the BREAK bit and generates an ext/status interrupt. Upon
 *       termination of the break, the receive FIFO will contain a single 
 *       NULL character. This NULL character will cause a dma rint. The
 *       Framing Error bit will not be set for this character, but if odd
 *       parity has been selected, the parity error bit will be set. 
 */ 
sccbrkint(unit)
     register int unit;
{
    register struct scc_softc *sc = sccsc;
    register struct tty *tp;
    register struct scc_reg *rsp;
    register struct scc_saved_reg *ssp;
    register u_short c;	
    register int flg;
    
    tp = &scc_tty[unit];
    rsp = sc->sc_regs[unit];
    ssp = &sc->sc_saved_regs[unit];
    c = (unit<<8)| 0x0000;
    /*
     * We need to disable both the SCC rint and the IOASIC rdma int
     * because we just want to read and discard the NULL character
     * that will be deposited in the receive FIFO upon termination
     * of the break.
     */
    if (scc_brk[unit]) {
	ssp->wr1 &= ~SCC_WR1_RINT;   /* disable rint on special conditions */
	SCC_WRITE(rsp, SCC_WR1, ssp->wr1);
	IOC_CLR(IOC_SSR, scc_rdma_en[unit]);    /* disable rdma */
	
	if ((tp->t_state & TS_ISOPEN) == 0) {   /* process break */
	    wakeup((caddr_t)&tp->t_rawq);
	    return;
	}
	flg = tp->t_iflag;
	if (do_tpath) {
	    tp->t_tpath |= TP_DOSAK;
	    (*linesw[tp->t_line].l_rint)(c, tp);
	    return;
	}
	if (flg & IGNBRK)
	  return;
	if (flg & BRKINT) {
	    if ((tp->t_lflag_ext & PRAW) && 
		(tp->t_line != TERMIODISC))
	      c = 0;
	    else {
		ttyflush(tp, FREAD | FWRITE);
		gsignal(tp->t_pgrp, SIGINT);
		return;
	    }
	}
	else {
	    if (flg & PARMRK){
		(*linesw[tp->t_line].l_rint)(0377,tp);
		(*linesw[tp->t_line].l_rint)(0,tp);
	    }
	    c = 0;
	}
	if (flg & ISTRIP){
	    c &= 0177;
	}
	else {
	    c &= 0377;
	    if ((c == 0377) && (tp->t_line == TERMIODISC) &&
		(flg & PARMRK))
	      (*linesw[tp->t_line].l_rint)(0377,tp);
	}
	(*linesw[tp->t_line].l_rint)(c, tp);

    } else {
	c = ((rsp->SCC_DATA)>>8)&0xff;	/* read NULL character */
	IOC_SET(IOC_SSR, scc_rdma_en[unit]);  /* enable RDMA */
	ssp->wr1 &= ~SCC_WR1_RINT;		/* clear rint bits */
	ssp->wr1 |= SCC_WR1_RINT_SPC;		/* enable rint */
	SCC_WRITE(rsp, SCC_WR1, ssp->wr1);
    }
}

int  sccputc();
int  sccgetc();

sccputc(c)
     register int c;
{
    if (consDev == GRAPHIC_DEV) {
	if ( v_consputc ) {
	    (*v_consputc) (c);
	    if ( c == '\n' )
		(*v_consputc)( '\r' );
	    return;
	}
    }
    scc_putc(3, c);
    if ( c == '\n')
	scc_putc(3, '\r');
}

/*
 * polled-mode DMA: need to do this because SCC can not be touched in
 * scc_putc.
 */
scc_putc(unit, c)
     int unit;
     register int c;
{
    register struct scc_softc *sc = sccsc;
    register int s;
    register struct scc_reg *rsp;
    char *ptr;
    int intr_pending = 0, save_word;
    u_int sir, save_SSR, save_xmt_ptr;

    if (unit == 2 || unit == 3) {
	/* set pointer to be the word before the end of the page */
	ptr = sc->tbuf[unit] + SCC_PAGE_SIZE - SCC_WORD;
	
	/* rpbfix : do we need to be at extreme ??? */
	s = splextreme();
	IOC_RD(IOC_SSR, save_SSR);	/* save copy of SSR */
	IOC_WR(IOC_SSR, (save_SSR & ~(scc_xdma_en[unit]))); /* disable XMT DMA */
	IOC_RD(IOC_SIR, sir);
	if (sir & scc_xint[unit]) {
	    intr_pending = 1;
	    IOC_WR(IOC_SIR, ~(scc_xint[unit]));
	} else {
	    save_xmt_ptr = sc->ioc_regs[unit]->XDMA_REG;
	    save_word = *(int *)ptr;
	}
	*(u_short *)ptr = ((u_short)c) << 8;
	sc->ioc_regs[unit]->XDMA_REG = (u_long)(svtophy(ptr) << 3);
	IOC_SET(IOC_SSR, scc_xdma_en[unit]);
	IOC_RD(IOC_SIR, sir);
	while ((sir & scc_xint[unit]) == 0) 
	    IOC_RD(IOC_SIR, sir);
	
	if (intr_pending == 0) {
	    IOC_CLR(IOC_SSR, scc_xdma_en[unit]);
	    IOC_WR(IOC_SIR, ~(scc_xint[unit]));
	    sc->ioc_regs[unit]->XDMA_REG = save_xmt_ptr;
	    *(long *)ptr = save_word;
	    IOC_WR(IOC_SSR, save_SSR);
	}
	splx(s);
    } else {
	s = splhigh();
	rsp = sc->sc_regs[unit];
	while ((((rsp->SCC_CMD)>>8) & SCC_RR0_TBUF_EMPTY) == 0)
	    ;
	rsp->SCC_DATA = (c&0xff)<<8;        /* output char */  
	while ((((rsp->SCC_CMD)>>8) & SCC_RR0_TBUF_EMPTY) == 0)
	    ;
	splx(s);
    }
}

/* pgt - new sccgetc() */
sccgetc()
{
    register u_char c;
    register int line;
    
    /*
     * Line number we expect input from. 
     */
    if (consDev == GRAPHIC_DEV)
	line = 0x0;
    else
	line = 0x3;

    c = scc_getc(line);

    if (v_consgetc)
	return ((*v_consgetc)(c & 0xff));
    else
	return (c & 0xff);
}

scc_getc(unit)
     int unit;
{
    register struct scc_softc *sc = sccsc;
    register u_char c, status;
    register int timo;
    register struct scc_reg *rsp;

    rsp = sc->sc_regs[unit];
    SCC_WRITE(rsp, SCC_WR9, 0x00); /* disable MIE ? */
    if (unit == 2 || unit == 3)
	IOC_CLR(IOC_SSR, scc_rdma_en[unit]);    /* disable rdma ? */

    for(timo=1000000; timo > 0; --timo) {
	if (((rsp->SCC_CMD)>>8) & SCC_RR0_RCHAR_AVAIL) {
	    SCC_READ(rsp, SCC_RR1, status);
	    c = ((rsp->SCC_DATA)>>8)&0xff;      /* read data */
	    DELAY(50000);
	    if (status & (SCC_RR1_PE | SCC_RR1_DO | SCC_RR1_FE)) 
	        continue;
	    break;
	}
    }
    SCC_WRITE(rsp, SCC_WR9, SCC_WR9_MIE); /* enable MIE ? */
    if (unit == 2 || unit == 3)
	IOC_SET(IOC_SSR, scc_rdma_en[unit]);    /* enable rdma ? */
    if (timo == 0)
	return(-1);
    else
	return(c & 0xff);
}


scc_mouse_init()
{
    register struct scc_softc *sc = sccsc;
    register struct scc_reg *rsp;
    register struct scc_saved_reg *ssp;
    register int unit = 1;

    rsp = sc->sc_regs[unit];
    ssp = &sc->sc_saved_regs[unit];
    scc_init(unit);
    /* 
     * enable functions ?
     */
    ssp->wr14 |= SCC_WR14_BRGEN_EN;
    SCC_WRITE(rsp, SCC_WR14, ssp->wr14);	     /*	 WR14 BRG enable */
    ssp->wr3 |= SCC_WR3_RXEN;
    SCC_WRITE(rsp, SCC_WR3, ssp->wr3);	      /*  WR3 Rx enable */
    ssp->wr5 |= SCC_WR5_TXEN;
    SCC_WRITE(rsp, SCC_WR5, ssp->wr5);        /*  WR5 Tx enable */
}

scc_mouse_putc(c)
int c;
{
    scc_putc(1, c);
}

scc_mouse_getc()
{
    return (scc_getc(1));
}

scc_kbd_init()
{
    register struct scc_softc *sc = sccsc;
    register struct scc_reg *rsp;
    register struct scc_saved_reg *ssp;
    register int unit = 0;

    rsp = sc->sc_regs[unit];
    ssp = &sc->sc_saved_regs[unit];
    scc_init(unit);
    /* 
     * enable functions ?
     */
    ssp->wr14 |= SCC_WR14_BRGEN_EN;
    SCC_WRITE(rsp, SCC_WR14, ssp->wr14);	     /*	 WR14 BRG enable */
    ssp->wr3 |= SCC_WR3_RXEN;
    SCC_WRITE(rsp, SCC_WR3, ssp->wr3);	      /*  WR3 Rx enable */
    ssp->wr5 |= SCC_WR5_TXEN;
    SCC_WRITE(rsp, SCC_WR5, ssp->wr5);        /*  WR5 Tx enable */
}

scc_kbd_putc(c)
int c;
{
    scc_putc(0, c);
}

scc_kbd_getc()
{
    return (scc_getc(0));
}


/*
 * Modem Control Routines
 */

/*
 *
 * Function:
 *
 *	scc_cd_drop
 *
 * Functional description:
 *
 * 	Determine if carrier has dropped.  If so call scc_tty_drop to terminate
 * 	the connection.
 *
 * Arguments:
 *
 *	register struct tty *tp  -  terminal pointer ( for terminal attributes )
 *
 * Return value:
 *
 *	none
 *
 */
scc_cd_drop(tp)
     register struct tty *tp;
{
    register struct scc_softc *sc = sccsc;
    register int unit;
    
    unit = minor(tp->t_dev);
    if ((tp->t_state & TS_CARR_ON) && (!(SCC_DCD(unit)))) {
	scc_tty_drop(tp);
	return;
    }
    sccmodem[unit] |= MODEM_CD;
}

/*
 *
 * Function:
 *
 *	scc_dsr_check
 *
 * Functional description:
 *
 *	DSR must be asserted for a connection to be established.  Here we 
 *	either start or terminate a connection on the basis of DSR.
 *
 * Arguments:
 *
 *	register struct tty *tp  -  terminal pointer (for terminal attributes)
 *
 * Return value:
 *
 *	none
 *
 */
scc_dsr_check(tp)
     register struct tty *tp;
{
    register struct scc_softc *sc = sccsc;
    register int unit;
    
    unit = minor(tp->t_dev);
    if (sccmodem[unit] & MODEM_DSR_START) {
	sccmodem[unit] &= ~MODEM_DSR_START;
	/*
	 * since dc7085 chip on PMAX only provides DSR then assume that CD
	 * has come up after 1 sec and start tty.  If CD has not
	 * come up the modem should deassert DSR thus closing the line
	 *
	 * On 3max, we look for DSR|CTS|CD before establishing a
	 * connection.
	 */
	if (SCC_XMIT(unit)) {
	    scc_start_tty(tp);
	}
	return;
    }
    if ((tp->t_state&TS_CARR_ON)==0)
	scc_tty_drop(tp);
}

/*
 *
 * Function:
 *
 *	scc_cd_down
 *
 * Functional description:
 *
 *	Determine whether or not carrier has been down for > 2 sec.
 *
 * Arguments:
 *
 *	register struct tty *tp  -  terminal pointer ( for terminal attributes )
 *
 * Return value:
 *
 *	1 - if carrier was down for > 2 sec.
 *	0 - if carrier down <= 2 sec.
 *
 */
scc_cd_down(tp)
     register struct tty *tp;
{
    register int msecs, unit;
    
    unit = minor(tp->t_dev);
    msecs = 1000000 * (time.tv_sec - scctimestamp[unit].tv_sec) + 
	(time.tv_usec - scctimestamp[unit].tv_usec);
    if (msecs > 2000000){
	return(1);
    }
    else{
	return(0);
    }
}

/*
 *
 * Function:
 *
 *	scc_tty_drop
 *
 * Functional description:
 *
 *	Terminate a connection.
 *
 * Arguments:
 *
 *	register struct tty *tp  -  terminal pointer ( for terminal attributes )
 *
 * Return value:
 *
 *	none
 *
 */
scc_tty_drop(tp)
     struct tty *tp;
{
    register struct scc_softc *sc = sccsc;
    register int unit;
    
    unit = minor(tp->t_dev);
    if (tp->t_flags & NOHANG)
	return;
    /* 
     * Notify any processes waiting to open this line.  Useful in the
     * case of a false start.
     */
    sccmodem[unit] = MODEM_BADCALL;
    tp->t_state &= ~(TS_CARR_ON|TS_TTSTOP|TS_BUSY|TS_ISUSP);
    wakeup((caddr_t)&tp->t_rawq);
    gsignal(tp->t_pgrp, SIGHUP);
    gsignal(tp->t_pgrp, SIGCONT);
    SCC_CLR_DTR(unit);
    SCC_CLR_RTS(unit);
    SCC_CLR_SS(unit);
}


/*
 *
 * Function:
 *
 *	scc_start_tty
 *
 * Functional description:
 *
 *	Establish a connection.
 *
 * Arguments:
 *
 *	register struct tty *tp  -  terminal pointer ( for terminal attributes )
 *
 * Return value:
 *
 *	none
 *
 */
scc_start_tty(tp)
     register struct tty *tp;
{
    register int unit;
    
    unit = minor(tp->t_dev);
    tp->t_state &= ~(TS_ONDELAY);
    tp->t_state |= TS_CARR_ON;
    sccspeedi(unit);
    if (sccmodem[unit] & MODEM_DSR)
	untimeout(scc_dsr_check, tp);
    sccmodem[unit] |= MODEM_CD|MODEM_CTS|MODEM_DSR;
    scctimestamp[unit].tv_sec = scctimestamp[unit].tv_usec = 0;
    wakeup((caddr_t)&tp->t_rawq);
}

sccbaudrate(speed)
     int speed;
{
    return(scc_speeds[speed & CBAUD].baud_support);
}

/* checks for SI */
sccspeedi(unit)
     register int unit;
{
    register struct scc_softc *sc = sccsc;
    register struct tty *tp;
    register struct scc_reg *rsp;
    register struct scc_saved_reg *ssp;

    tp = &scc_tty[unit];
    rsp = sc->sc_regs[unit];
    ssp = &sc->sc_saved_regs[unit];

    ssp->wr14 &= ~(SCC_WR14_BRGEN_EN);
    SCC_WRITE(rsp, SCC_WR14, ssp->wr14);	     /*	 WR14 BRG disable */
    if (SCC_SI(unit))  {  /* if Speed Indicate is set */
        if (scc_cbaud[unit] != (tp->t_cflag&CBAUD)) { /* not full speed */ 
	    scc_cbaud[unit] = tp->t_cflag&CBAUD;
	    SCC_WRITE(rsp, SCC_WR12, scc_speeds[scc_cbaud[unit]].baud_lo);
	    SCC_WRITE(rsp, SCC_WR13, scc_speeds[scc_cbaud[unit]].baud_hi);
	}
    } else {
        if (scc_cbaud[unit] == (tp->t_cflag&CBAUD)) { /* full speed */
	    scc_cbaud[unit] = scc_half_speed(scc_cbaud[unit]);
	    SCC_WRITE(rsp, SCC_WR12, scc_speeds[scc_cbaud[unit]].baud_lo);
	    SCC_WRITE(rsp, SCC_WR13, scc_speeds[scc_cbaud[unit]].baud_hi);
        } 
    }
    ssp->wr14 |= (SCC_WR14_BRGEN_EN);
    SCC_WRITE(rsp, SCC_WR14, ssp->wr14);	     /*	 WR14 BRG enable */
}   

scc_half_speed(cbaud)
     register int cbaud;
{
    register int ncbaud = cbaud;

    switch (cbaud) {
    case B38400: case B19200: case B9600: 
    case B4800: case B1200: case B600: 
        ncbaud--;
	break;
    case B2400: ncbaud = B1200;
	break;
    case B300: ncbaud = B150;
	break;
    case B150: ncbaud = B75;
	break;
    default:ncbaud = cbaud; /* no half-speed counterpart */
	break;
    }
    return(ncbaud);
}



