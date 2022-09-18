#ifdef notdef
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

/*
 * cpu_board.h -- cpu board specific defines
 */

#define	SBE_ADDR	0x1e000800	/* system bus error address */

#define	CPU_CONFIG	0x1e080006	/* cpu bd configuration register */
#define	CONFIG_NOCP1	0x01		/* coprocessor 1 not present */
#define	CONFIG_NOCP2	0x02		/* coprocessor 2 not present */
#define	CONFIG_POWERUP	0x04		/* power-up or cpu bd reset */

#define	LED_LMEM_RUN	0x40		/* enable local memory */
#define	LED_FPBD_RUN	0x80		/* enable fp board */

#define IDPROM_BRDTYPE	0x1ff00003	/* cpu bd type, see below */
#define	IDPROM_REV	0x1ff00007	/* cpu bd revision */
#define	IDPROM_SN1	0x1ff0000b	/* cpu bd serial number, digit 1 */
#define	IDPROM_SN2	0x1ff0000f	/* cpu bd serial number, digit 2 */
#define	IDPROM_SN3	0x1ff00013	/* cpu bd serial number, digit 3 */
#define	IDPROM_SN4	0x1ff00017	/* cpu bd serial number, digit 4 */
#define	IDPROM_SN5	0x1ff0001b	/* cpu bd serial number, digit 5 */
#define IDPROM_CKSUM	0x1fffffff	/* cpu bd idprom checksum */

/* possible values for cpu bd type in idprom */
#define BRDTYPE_R2300	1
#define BRDTYPE_R2600	2
#define BRDTYPE_R2800	3
#endif /* notdef */
#define	LED_REG		0x1e000000
#define	LED_MASK	0xff		/* led bits */
