
/*
 *	@(#)sccreg.h	4.2	(ULTRIX)	8/16/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 * sccreg.h
 *
 * SCC SLU console driver
 *
 * Modification history
 *
 * 20-Feb-1990 - pgt (Philip Gapuz Te)
 * 	created file.
 *
 */

/* Serial line registers */
struct scc_reg {
  vu_short SCC_CMD;
  u_short pad1[1];
  vu_short SCC_DATA;
  u_short pad2[1];
};

/* saved SCC registers */
struct scc_saved_reg {
  char wr1;
  char wr3;
  char wr4;
  char wr5;
  char wr14;
};

/* IOASIC DMA registers */
struct ioc_reg {
  u_long XDMA_REG;
  long pad[3];
  u_long RDMA_REG;
};

#define CONSOLEMAJOR 0
#define NSCCLINE 4

#define SCC_KYBD  0
#define SCC_MOUSE 1
#define SCC_COMM1 2
#define SCC_COMM2 3

#define SCC0_A SCC_MOUSE
#define SCC0_B SCC_COMM1
#define SCC1_A SCC_KYBD
#define SCC1_B SCC_COMM2

#define SCC_PAGE_SIZE 4096 /* bytes */
#define SCC_HALF_PAGE 2048 
#define SCC_WORD 4

/* Driver and data specific structure */
struct	scc_softc {
  struct scc_reg *sc_regs[4];	   /* 3MIN SLU regs               */
  struct scc_saved_reg sc_saved_regs[4];
  struct ioc_reg *ioc_regs[4];  /* IOASIC DMA registers         */
  int rflag[4];                 /* flag for switching rbufs     */
  char *rbuf[4][2];          /* two receive buffers per unit */
  char *tbuf[4];             /* transmit buffers             */
  char *tptr[4];             /* transmit buffer start ptr    */
  long sc_flags[NSCCLINE];       /* Flags (one per line)		*/
  long sc_category_flags[NSCCLINE]; /* Category flags (one per line)*/
  u_long sc_softcnt[NSCCLINE];	/* Soft error count total	*/
  u_long sc_hardcnt[NSCCLINE];	/* Hard error count total	*/
  char sc_device[DEV_SIZE][NSCCLINE]; /* Device type string	*/
};

/* Baud rate support status */
struct baud_support {
  u_char baud_lo;               /* Low time constant - WR12 */
  u_char baud_hi;               /* Hi time constant - WR13 */
  u_char baud_support;          /* Set if baudrate supported */
};

/* dc7085 line control status definitions (dclcs) */
#define DC_SR		0x08		/* Secondary Receive		*/
#define DC_CTS		0x10		/* Clear To Send		*/
#define DC_CD		0x20		/* Carrier Detect		*/
#define DC_RI		0x40		/* Ring Indicate		*/
#define DC_DSR		0x80		/* Data Set Ready		*/
#define DC_LE		0x100		/* Line Enable			*/
#define DC_DTR		0x200		/* Data Terminal Ready		*/
#define DC_BRK		0x400		/* Break			*/
#define DC_ST		0x800		/* Secondary Transmit		*/
#define DC_RTS		0x1000		/* Request To Send		*/

/* DM lsr definitions */
#define SML_LE		0x01		/* Line enable			*/
#define SML_DTR		0x02		/* Data terminal ready		*/
#define SML_RTS		0x04		/* Request to send		*/
#define SML_ST		0x08		/* Secondary transmit		*/
#define SML_SR		0x10		/* Secondary receive		*/
#define SML_CTS		0x20		/* Clear to send		*/
#define SML_CAR		0x40		/* Carrier detect		*/
#define SML_RNG		0x80		/* Ring				*/
#define SML_DSR		0x100		/* Data set ready, not DM bit	*/

/* Read registers */
#define SCC_RR0             0x00            /* Tx/Rx buffer status and Ext status */
#define SCC_RR1             0x01            /* Special Recv Condition status */
#define SCC_RR2             0x02            /* Interrupt vector */
#define SCC_RR3             0x03            /* Interrupt pending (channel A only) */
#define SCC_RR8             0x08            /* Receive buffer */
#define SCC_RR10            0x0a            /* Loop/Clock status */
#define SCC_RR12            0x0c            /* Lower byte of time constant */
#define SCC_RR13            0x0d            /* Upper byte of time constant */
#define SCC_RR15            0x0f            /* External/Status interrupt enable */

/* Write registers */
#define SCC_WR0             0x00            /* Command register */
#define SCC_WR1             0x01            /* Tx/Rx interrupt and data transfer mode */
#define SCC_WR2             0x02            /* Interrupt vector */
#define SCC_WR3             0x03            /* Receive parameters and control */
#define SCC_WR4             0x04            /* Tx/Rx misc parameters and modes */
#define SCC_WR5             0x05            /* Transmit parameters and controls */
#define SCC_WR6             0x06            /* Sync char or SDLC address field */
#define SCC_WR7             0x07            /* Sync char or SDLC flag */
#define SCC_WR8             0x08            /* Transmit buffer */
#define SCC_WR9             0x09            /* Master interrupt control and reset */
#define SCC_WR10            0x0a            /* Misc Tx/Rx control bits */
#define SCC_WR11            0x0b            /* Clock mode control */
#define SCC_WR12            0x0c            /* Lower byte of BRG time constant */
#define SCC_WR13            0x0d            /* Upper byte of BRG time constant */
#define SCC_WR14            0x0e            /* Misc control bits */
#define SCC_WR15            0x0f            /* External/Status interrupt control */

/* RR0 */
#define SCC_RR0_RCHAR_AVAIL        0x01 /* Rx character available */
#define SCC_RR0_ZCOUNT             0x02 /* Zero count */
#define SCC_RR0_TBUF_EMPTY         0x04 /* Tx buffer empty */
#define SCC_RR0_DCD                0x08 /* DCD */
#define SCC_RR0_SYNC               0x10 /* SYNC */
#define SCC_RR0_HUNT               0x10 /* HUNT */
#define SCC_RR0_CTS                0x20 /* CTS */
#define SCC_RR0_TX_UNDERRUN        0x40 /* Tx underrun/EOM */
#define SCC_RR0_BREAK              0x80 /* BREAK */
#define SCC_RR0_ABORT              0x80 /* ABORT */

/* RR1 */
#define SCC_RR1_ALL_SENT           0x01 /* All sent */
#define SCC_RR1_RCODE2             0x02 /* Residue code 2 */
#define SCC_RR1_RCODE1             0x04 /* Residue code 1 */
#define SCC_RR1_RCODE0             0x08 /* Residue code 0 */
#define SCC_RR1_PE                 0x10 /* Parity error */
#define SCC_RR1_DO                 0x20 /* Rx overrun error */
#define SCC_RR1_FE                 0x40 /* Framing error */
#define SCC_RR1_CRC                0x40 /* CRC error */
#define SCC_RR1_EOF                0x80 /* End of frame (SDLC) */

/* RR3 */
#define SCC_RR3_B_EXT_IP           0x01 /* Channel B Ext/Stat IP */
#define SCC_RR3_B_TIP              0x02 /* Channel B Tx IP */
#define SCC_RR3_B_RIP              0x04 /* Channel B Rx IP */
#define SCC_RR3_A_EXT_IP           0x08 /* Channel A Ext/Stat IP */
#define SCC_RR3_A_TIP              0x10 /* Channel A Tx IP */
#define SCC_RR3_A_RIP              0x20 /* Channel A Rx IP */

/* RR10 */
#define SCC_RR10_ON_LOOP           0x02 /* On loop */
#define SCC_RR10_LOOP_SEND         0x10 /* Loop sending */
#define SCC_RR10_2CLOCK_MISS       0x40 /* Two clocks missing */
#define SCC_RR10_1CLOCK_MISS       0x80 /* One clock missing */

/* RR15 */
#define SCC_RR15_ZCOUNT_IE         0x02 /* Zero count IE */
#define SCC_RR15_DCD_IE            0x08 /* DCD IE */
#define SCC_RR15_SYNC_IE           0x10 /* Sync IE */
#define SCC_RR15_HUNT_IE           0x10 /* Hunt IE */
#define SCC_RR15_CTS_IE            0x20 /* CTS IE */
#define SCC_RR15_TX_UNDERRUN_IE    0x40 /* Tx underrun/EOM IE */
#define SCC_RR15_BREAK_IE          0x80 /* Break IE */
#define SCC_RR15_ABORT_IE          0x80 /* Abort IE */

/* WR0 */
#define SCC_WR0_RESET_EXT_INT      0x10 /* Reset Ext/Stat interrupts */
#define SCC_WR0_SEND_ABORT         0x18 /* Send abort (SDLC) */
#define SCC_WR0_ENINT_NEXT_RCHAR   0x20 /* Enable int on next Rx char */
#define SCC_WR0_RESET_TXIP         0x28 /* Reset Tx int pending */
#define SCC_WR0_ERROR_RESET        0x30 /* Error reset */
#define SCC_WR0_RESET_HIUS         0x38 /* Reset highest IUS */
#define SCC_WR0_RESET_RX_CRC_CHECK 0x40 /* Reset Rx CRC checker */
#define SCC_WR0_RESET_TX_CRC_GEN   0x80 /* Reset Tx CRC generator */
#define SCC_WR0_RESET_TX_UNDERRUN  0xc0 /* Reset Tx underrun/EOM */

/* WR1 */
#define SCC_WR1_EXT_IE             0x01 /* Ext int enable */
#define SCC_WR1_TIE                0x02 /* Tx int enable */
#define SCC_WR1_PSPC               0x04 /* Parity is special condition */
#define SCC_WR1_RINT               0x18 /* Rx int bits */
#define SCC_WR1_RINT_DIS           0x00 /* Rx int disable */
#define SCC_WR1_RINT_FIRST         0x08 /* Rx int on first char/special cond */
#define SCC_WR1_RINT_ALL           0x10 /* Rx int on all Rx char/special cond */
#define SCC_WR1_RINT_SPC           0x18 /* Rx int on special condition only */
#define SCC_WR1_WDMA_RX            0x20 /* Wait/DMA request on receive */
#define SCC_WR1_DMA_REQ            0x40 /* DMA request function */
#define SCC_WR1_WDMA_EN            0x80 /* Wait/DMA request enable */

/* WR3 */
#define SCC_WR3_RXEN               0x01 /* Rx enable */
#define SCC_WR3_SCLI               0x02 /* Sync character load inhibit */
#define SCC_WR3_ASM                0x40 /* Address search mode (SDLC) */
#define SCC_WR3_RCE                0x80 /* Rx CRC enable */
#define SCC_WR3_EHM                0x10 /* Enter hunt mode */
#define SCC_WR3_AUTO_EN            0x20 /* Auto enables */
#define SCC_WR3_RBITS              0xc0 /* Rx character size bits */
#define SCC_WR3_RBITS5             0x00 /* Rx 5 bits/character */
#define SCC_WR3_RBITS7             0x40 /* Rx 7 bits/character */
#define SCC_WR3_RBITS6             0x80 /* Rx 6 bits/character */
#define SCC_WR3_RBITS8             0xc0 /* Rx 8 bits/character */

/* WR4 */
#define SCC_WR4_PENABLE            0x01 /* Parity enable */
#define SCC_WR4_EPAR               0x02 /* Parity even/odd */
#define SCC_WR4_SME                0x00 /* Sync mode enable */
#define SCC_WR4_STOP               0x0c /* stop bits */
#define SCC_WR4_ONESB              0x04 /* one stop bit/character */
#define SCC_WR4_ONHSB              0x08 /* one and a half stop bits/characters */
#define SCC_WR4_TWOSB              0x0c /* two stop bits/character */
#define SCC_WR4_SYNC8              0x00 /* eight bit sync character */
#define SCC_WR4_SYNC16             0x10 /* sixteen bit sync character */
#define SCC_WR4_SDLC               0x20 /* SDLC mode */
#define SCC_WR4_ESYNC              0x30 /* External sync mode */
#define SCC_WR4_CLOCK1             0x00 /* x1 clock mode */
#define SCC_WR4_CLOCK16            0x40 /* x16 clock mode */
#define SCC_WR4_CLOCK32            0x80 /* x32 clock mode */
#define SCC_WR4_CLOCK64            0xc0 /* x64 clock mode */

/* WR5 */
#define SCC_WR5_TCE                0x01 /* Tx CRC enable */
#define SCC_WR5_RTS                0x02 /* RTS */
#define SCC_WR5_SC                 0x04 /* SDLC/CRC-16 */
#define SCC_WR5_TXEN               0x08 /* Tx enable */
#define SCC_WR5_BRK                0x10 /* Send break */
#define SCC_WR5_TBITS              0x60 /* Tx character size bits */
#define SCC_WR5_TBITS5             0x00 /* Tx 5 bits/character */
#define SCC_WR5_TBITS7             0x20 /* Tx 7 bits/character */
#define SCC_WR5_TBITS6             0x40 /* Tx 6 bits/character */             
#define SCC_WR5_TBITS8             0x60 /* Tx 8 bits/character */
#define SCC_WR5_DTR                0x80 /* DTR */

/* WR9 */
#define SCC_WR9_INTACK             0x20 /* INTACK enable */
#define SCC_WR9_VIS                0x01 /* VIS */
#define SCC_WR9_NV                 0x02 /* NV */
#define SCC_WR9_DLC                0x04 /* DLC */
#define SCC_WR9_MIE                0x08 /* MIE */
#define SCC_WR9_STATUS             0x10 /* Status */
#define SCC_WR9_NORESET            0x00 /* No reset */
#define SCC_WR9_RESETB             0x40 /* Channel reset B */
#define SCC_WR9_RESETA             0x80 /* Channel reset A */
#define SCC_WR9_HARDRESET          0xc0 /* Force hardware reset */

/* WR10 */
#define SCC_WR10_6SYNC8            0x01 /* 6 bit/8 bit sync */
#define SCC_WR10_LOOP              0x02 /* Loop mode */
#define SCC_WR10_AFU               0x04 /* Abort/Flag on underrun */
#define SCC_WR10_MFI               0x08 /* Mark/Flag idle */
#define SCC_WR10_GAP               0x10 /* Go active on poll */
#define SCC_WR10_NRZ               0x00 /* NRZ */
#define SCC_WR10_NRZI              0x20 /* NRZI */
#define SCC_WR10_FM1               0x40 /* FM1 */
#define SCC_WR10_FM0               0x60 /* FM0 */
#define SCC_WR10_CPI               0x80 /* CRC preset I/O */

/* WR11 */
#define SCC_WR11_TRxC_XTAL         0x00 /* TRxC = xtal output */
#define SCC_WR11_TRxC_TXCLOCK      0x01 /* TRxC = transmit clock */
#define SCC_WR11_TRxC_BRGEN        0x02 /* TRxC = baud rate gen output */
#define SCC_WR11_TRxC_DPLL         0x03 /* TRxC = DPLL output */
#define SCC_WR11_TRxC_OUT          0x04 /* TRxC output */
#define SCC_WR11_TxC_RTxC          0x00 /* Transmit clock = RTxC */
#define SCC_WR11_TxC_TRxC          0x08 /* Transmit clock = TRxC */
#define SCC_WR11_TxC_BRGEN         0x10 /* Transmit clock = baud rate gen output */
#define SCC_WR11_TxC_DPLL          0x18 /* Transmit clock = DPLL */
#define SCC_WR11_RxC_RTxC          0x00 /* Receive clock = RTxC */
#define SCC_WR11_RxC_TRxC          0x20 /* Receive clock = TRxC */
#define SCC_WR11_RxC_BRGEN         0x40 /* Receive clock = baud rate gen output */
#define SCC_WR11_RxC_DPLL          0x60 /* Receive clock = DPLL */
#define SCC_WR11_RTxC_XTAL         0x80 /* RTxC xtal/no xtal */

/* WR12 */
#define SCC_WR12_B50_LO            0xfe
#define SCC_WR12_B75_LO            0xfe
#define SCC_WR12_B110_LO           0x2c
#define SCC_WR12_B134_5_LO         0xaf
#define SCC_WR12_B150_LO           0xfe
#define SCC_WR12_B200_LO           0x7e
#define SCC_WR12_B300_LO           0xfe
#define SCC_WR12_B600_LO           0x7e
#define SCC_WR12_B1200_LO          0xbe
#define SCC_WR12_B1800_LO          0x7e
#define SCC_WR12_B2400_LO          0x5e
#define SCC_WR12_B4800_LO          0x2e
#define SCC_WR12_B9600_LO          0x16
#define SCC_WR12_B19200_LO         0xa
#define SCC_WR12_B38400_LO         0x4

/* WR13 */
#define SCC_WR13_B50_HI            0x11
#define SCC_WR13_B75_HI            0x0b
#define SCC_WR13_B110_HI           0x08
#define SCC_WR13_B134_5_HI         0x06
#define SCC_WR13_B150_HI           0x05
#define SCC_WR13_B200_HI           0x04
#define SCC_WR13_B300_HI           0x02
#define SCC_WR13_B600_HI           0x01
#define SCC_WR13_B1200_HI          0x00
#define SCC_WR13_B1800_HI          0x00
#define SCC_WR13_B2400_HI          0x00
#define SCC_WR13_B4800_HI          0x00
#define SCC_WR13_B9600_HI          0x00
#define SCC_WR13_B19200_HI         0x00
#define SCC_WR13_B38400_HI         0x00

/* WR14 */
#define SCC_WR14_BRGEN_EN          0x01 /* BR generator enable */
#define SCC_WR14_BRGEN_PCLK        0x02 /* BR generator source = pclk */
#define SCC_WR14_REQ               0x04 /* DTR/REQ function */
#define SCC_WR14_DTR               0x00 /* DTR/REQ function */
#define SCC_WR14_AUTOECHO          0x08 /* Auto echo */
#define SCC_WR14_LOCALOOP          0x10 /* Local loopback */
#define SCC_WR14_ESM               0x20 /* Enter search mode */
#define SCC_WR14_RMC               0x40 /* Reset missing clock */
#define SCC_WR14_DPLL_DIS          0x60 /* Disable DPLL */
#define SCC_WR14_SOURCE_BRGEN      0x80 /* Set source = BR gen. */
#define SCC_WR14_SOURCE_RTxC       0xa0 /* Set source = RTxC */
#define SCC_WR14_FM                0xc0 /* Set FM mode */
#define SCC_WR14_NRZI              0xe0 /* Set NRZI mode */
 
/* WR15 */
#define SCC_WR15_ZCOUNT_IE         0x02 /* Zero count IE */
#define SCC_WR15_FIFO_EN           0x04 /* FIFO enable */
#define SCC_WR15_DCD_IE            0x08 /* DCD IE */
#define SCC_WR15_SYNC_IE           0x10 /* Sync IE */
#define SCC_WR15_HUNT_IE           0x10 /* Hunt IE */
#define SCC_WR15_CTS_IE            0x20 /* CTS IE */
#define SCC_WR15_TX_UNDERRUN_IE    0x40 /* Tx underrun/EOM IE */
#define SCC_WR15_BREAK_IE          0x80 /* Break IE */
#define SCC_WR15_ABORT_IE          0x80 /* Abort IE */


	
