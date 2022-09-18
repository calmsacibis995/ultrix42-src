/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * Templates onto the IO space of the cpu board SCN2681 duart.
 * See the Signetics data book for information.
 */

/*
 * The per channel IO template.
 */
struct cn_chan {
	u_char pad0[3];u_char	cnc_mode;
	u_char pad1[3];u_char	cnc_stat;
	u_char pad2[3];u_char	cnc_cmd;
	u_char pad3[3];u_char	cnc_data;
};
#define cnc_clockselect cnc_stat

/*
 * The entire 2681 IO template.
 */
struct cn_reg {
	struct	cn_chan 	cn_chan_a;
	u_char	pad0[3];u_char	cn_inport_change;
	u_char	pad1[3];u_char	cn_isr;
	u_char	pad2[3];u_char	cn_ctupper;
	u_char	pad3[3];u_char	cn_ctlower;
	struct	cn_chan		cn_chan_b;
	u_char	pad4[3];u_char	cn_snipe;
	u_char	pad5[3];u_char	cn_inport;
	u_char	pad6[3];u_char	cn_startcount;
	u_char	pad7[3];u_char	cn_stopcount;
};
#define	cn_auxreg		cn_inport_change
#define	cn_imr			cn_isr
#define	cn_outport_conf		cn_inport
#define	cn_outport_set		cn_startcount
#define	cn_outport_clear	cn_stopcount
/*
 *	2681 interrupt auxreg/inport change reg
 *	controls the baud select clock and counter/timer functions.
 */
#define CNAUX_ENIP0	0x1	/* interrupt on input 0 status change */
#define CNAUX_ENIP1	0x2	/* interrupt on input 1 status change */
#define CNAUX_ENIP2	0x4	/* interrupt on input 2 status change */
#define CNAUX_ENIP3	0x8	/* interrupt on input 3 status change */
#define CNAUX_SET0	0x0	/* select baud rate set 0 */
#define CNAUX_SET1	0x80	/* select baud rate set 1 */
/*
 *	2681 interrupt status/mask registers bit definitions.
 *	This is a global register shared between ports A and B.
 */
#define	CNIMR_TXRDY_A	0x01	/* channel A Tx interrupt */
#define	CNIMR_RXRDY_A	0x02	/* channel A Rx interrupt */
#define	CNIMR_BREAK_A	0x04	/* channel A break interrupt */
#define	CNIMR_COUNTER	0x08	/* counter interrupt */
#define	CNIMR_TXRDY_B	0x10	/* channel B Tx interrupt */
#define	CNIMR_RXRDY_B	0x20	/* channel B Rx interrupt */
#define	CNIMR_BREAK_B	0x40	/* channel B break interrupt */
#define	CNIMR_CD_B	0x80	/* input port. wired to CD for channel B */
/*
 *	2681 status register bit definitions.
 *	One status register per port.
 */
#define	CNSTAT_RXRDY	0x01	/* data available in Rx FIFO */
#define	CNSTAT_FFULL	0x02	/* Rx FIFO full. (3 bytes) */
#define	CNSTAT_TXRDY	0x04	/* Tx will accept another byte */
#define	CNSTAT_TXEMPTY	0x08	/* Tx empty. (both buffer and shifter) */
#define	CNSTAT_OVERRUN	0x10	/* Rx FIFO overrun */
#define	CNSTAT_PARITY	0x20	/* parity error */
#define	CNSTAT_FRAME	0x40	/* framing error */
#define	CNSTAT_RXBREAK	0x80	/* Rx break detected */
#define	CNSTAT_ERROR	(CNSTAT_OVERRUN | CNSTAT_PARITY | CNSTAT_FRAME)
/*
 *	2681 command register bit definitions.
 *	One command register per port.
 */
#define	CNCMD_RXENABLE		0x01	/* start looking for data */
#define	CNCMD_RXDISABLE		0x02	/* stop accepting for data */
#define	CNCMD_TXENABLE		0x04	/* accept new data to send */
#define	CNCMD_TXDISABLE		0x08	/* do not accept new data to send */
#define	CNCMD_MRRESET		0x10	/* point to MR1 for next mode access */
#define	CNCMD_RXRESET		0x20	/* disable Rx, flush FIFO */
#define	CNCMD_TXRESET		0x30	/* disable Tx, flush buffer */
#define	CNCMD_ERRESET		0x40	/* clear error bits in status reg */
#define	CNCMD_BREAKRESET	0x50	/* clear break change status */
#define	CNCMD_BREAKSTART	0x60	/* Start sending break after Tx idle */
#define	CNCMD_BREAKSTOP		0x70	/* Terminate break condition */
/*
 *	2681 mode register 1 bit definitions.
 *	One mode register 1 per port.
 *	The first access to the mode register after a reset or after a 
 *	reset MR pointer command will access MR1. All other accesses will
 *	be to MR2
 */
#define	CNMR1_RXRTS		0x80	/* enable wierd FFULL control flow */
#define	CNMR1_RXFULLINTR	0x40	/* interrupt on FFULL, else on RDY */
#define	CNMR1_ERRSUM		0x20	/* sumarize all FIFO errors in stat */
#define	CNMR1_PARITY		0x00	/* generate and detect parity */
#define	CNMR1_FORCEPARITY	0x08	/* generate and detect parity */
#define	CNMR1_NOPARITY		0x10	/* do not generate and detect parity */
#define	CNMR1_MULTDROP		0x18	/* enter multi-drop mode */
#define	CNMR1_EVENPARITY	0x00	/* generate and accept even parity */
#define	CNMR1_ODDPARITY		0x04	/* generate and accept odd parity */
#define	CNMR1_BITS5		0x01	/* 5 bits per character */
#define	CNMR1_BITS6		0x01	/* 6 bits per character */
#define	CNMR1_BITS7		0x02	/* 7 bits per character */
#define	CNMR1_BITS8		0x03	/* 8 bits per character */
/*
 *	2681 mode register 2 bit definitions
 *	One mode register 2 per port.
 */
#define	CNMR2_AUTOECHO		0x40	/* retransmit all recieved data */
#define	CNMR2_LOCALLOOP   	0x80	/* internal loopback */
#define	CNMR2_REMOTELOOP   	0xC0	/* play echo deamon */
#define	CNMR2_TXRTS		0x20	/* enable wierd RTS handshake */
#define	CNMR2_TXCTS		0x10	/* honor CTS handshake */
#define	CNMR2_STOP1		0x07	/* generate 1 stop bit */
#define	CNMR2_STOP2		0x0f	/* generate 2 stop bit */
/*
 *	Global output port bit definitions.
 *	OP0 and OP1 are all that are available.
 */
#define	CNOP_DTR_B		0x01	/* assert DTR for channel B */
#define	CNOP_RTS_B		0x02	/* assert RTS for channel B */
/*
 *	Global input port bit definitions.
 *	IP2 is the only one available.
 */
#define	CNIP_CTS_B		0x04	/* maybe hooked to CTS or.. */
#define	CNIP_CD_B		0x04	/* .. maybe to CD. */
