/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

struct scons_device {
	unsigned long	sc_status;	/* status register */
	unsigned long	sc_command;	/* command register */
	unsigned long	sc_rx;		/* receiver data register */
	unsigned long	sc_tx;		/* transmitter data register */
	unsigned long	sc_txbuf;	/* transmitter data buffer */
};

#define	SC_STAT_RXRDY	1		/* reciever has data available */
#define	SC_CMD_RXIE	1		/* reciever interrupt enable */
#define	SC_CMD_TXFLUSH	2		/* flush any buffered output */

#define SCONS0_BASE ((struct scons_device *)(0x1f000000+K1BASE))
#define SCONS1_BASE ((struct scons_device *)(0x1f000014+K1BASE))
