/*
 * @(#)ascreg.h	4.3	(ULTRIX)	11/15/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 /***********************************************************************
  * ascreg.h	07/11/89
  *
  * Modification History
  *
  * 11/15/90	Robert Scott
  *	Changed ASC_LOADCNTR to use (ASC_DMA | ASC_NOOP).
  *
  * 08/24/90    Maria Vella
  *	Added define for ASC_SSTEP_SELNOCMD.
  *
  * 04/16/90    Mitchell McConnell
  *	Changed definitions of asc registers to ints to try and fix
  *	R3000 bug re write buffer problem.
  *
  * 12/09/89    Mitchell McConnell
  *	Moved many defines from scsi_asc to here, where they belong.
  *
  * 11/01/89	Mitchell McConnell 
  *	Removed WBFLUSH macro, since this file will never be used
  *	where it would not equal 'wbflush'.
  *
  * 07/11/89	John A. Gallant - Creation of this file
  *
  * 07/27/89	John A. Gallant - modified to match hardware spec labels
  *
  * 09/08/89	Mitchell McConnell - added SEQUENCE STEP register defines.
  *
  ***********************************************************************/

/* NCR 53C94 Advanced SCSI controller (ASC) registers */

/* I/O register definitions for the ASC.  There are two seperate set of
registers, a read only set and a write only set.  The following structure
definitions and union will be used to define the register space. */

typedef struct 
{
	int r_asc_xcntrl;	/* transfer counter LSB */
	int r_asc_xcntrh;	/* transfer counter MSB */
	int r_asc_fifo;		/* FIFO access register, 16x9 deep */
	int r_asc_cmd;		/* ASC command register */
	int r_asc_status;	/* ASC status register */
	int r_asc_intr;		/* ASC interrupt register */
	int r_asc_sequ;		/* ASC sequence step register */
	int r_asc_fflag;	/* ASC FIFO flags/sequ step register */
	int r_asc_conf1;	/* Configuration 1 register */
	int r_asc_rsvd1;
	int r_asc_rsvd2;
	int r_asc_conf2;	/* Configuration 2 register */
	int r_asc_conf3;	/* Configuration 3 register */
} RO_ASCREG;

typedef struct 
{
	int w_asc_xcntl;	/* transfer count LSB */
	int w_asc_xcnth;	/* transfer count MSB */
	int w_asc_fifo;		/* FIFO access register, 16x9 deep */
	int w_asc_cmd;		/* ASC command register */
	int w_asc_dest;		/* ASC destinition bus ID register */
	int w_asc_timeout;	/* ASC select/reselect timeout register */
	int w_asc_period;	/* ASC synchronous period register */
	int w_asc_offset;	/* ASC synchronous offset register */
	int w_asc_conf1;	/* Configuration 1 register */
	int w_asc_clkcnv;	/* ASC clock conversion register */
	int w_asc_testm;	/* ASC test mode register */
	int w_asc_conf2;	/* Configuration 2 register */
	int w_asc_conf3;	/* Configuration 3 register */
} WO_ASCREG;

/* This union definition is for accessing the registers with a single pointer
type.  The following defines are for accessing the registers with a simple
name. */

typedef union
{
    RO_ASCREG rasc;
    WO_ASCREG wasc;
} ASCREG;

#define asc_tclsb 	wasc.w_asc_xcntl	/* transfer count LSB */
#define asc_tcmsb 	wasc.w_asc_xcnth	/* transfer count MSB */
#define asc_fifo 	rasc.r_asc_fifo		/* FIFO acc reg, 16x9 deep */
#define asc_cmd 	wasc.w_asc_cmd		/* ASC command register */
#define asc_stat	rasc.r_asc_status	/* ASC status register */
#define asc_dbid 	wasc.w_asc_dest		/* ASC dest bus ID reg */
#define asc_intr	rasc.r_asc_intr		/* ASC interrupt register */
#define asc_srto 	wasc.w_asc_timeout	/* ASC sel/resel timeout reg */
#define asc_ss		rasc.r_asc_sequ		/* ASC sequence step reg */
#define asc_sp 		wasc.w_asc_period	/* ASC sync period reg */
#define asc_ffss	rasc.r_asc_fflag	/* ASC FIFO flags/sequ reg */
#define asc_so 		wasc.w_asc_offset	/* ASC sync offset reg */
#define asc_cnf1 	wasc.w_asc_conf1	/* Configuration 1 register */
#define asc_ccf 	wasc.w_asc_clkcnv	/* ASC clock conversion reg */
#define asc_tm 		wasc.w_asc_testm	/* ASC test mode register */
#define asc_cnf2 	wasc.w_asc_conf2	/* Configuration 2 register */
#define asc_cnf3 	wasc.w_asc_conf3	/* Configuration 3 register */

/* -------------------------------------------------------------------- */

/* These Macros have to be used AFTER !! the softc structure initialization
in the particular routines.  They use the sc pointer. */
/* HAVE to be fixed  USE a table structure in data ?? */
/* volatile ?? work a macro vax/mips ?? */

#define ASC_REG_BASE	((volatile ASCREG *)(sc->sc_slotvaddr + 0x00000))
#define ASC_BUF_BASE	((volatile char *)(sc->sc_slotvaddr + 0x80000))


#define ASC_REG_ADDR	(ASC_REG_BASE)
#define ASC_BUF_ADDR	(ASC_BUF_BASE)

#define ASC_REG		register volatile ASCREG
#define ASC_BUFF	register volatile char

/* -------------------------------------------------------------------- */
/* Definitions for the pseudo DMA engine control register. */

typedef struct
{
    u_long rambuf_dma;
} DMAAR;

#define	DMA_ADR_MASK	0x0001FFFF	/* mask for the address bits 16:0 */
#define	DMA_ASC_WRITE	0x80000000	/* direction bit 0=read/1=write */

#define ASC_AR_ADDR	((volatile DMAAR *)(sc->sc_slotvaddr + 0x40000))

#define DMA_AR		volatile DMAAR

/* -------------------------------------------------------------------- */
/* Constant definitions for the ASC registers. */

/* Command Register: Commands and misc flags */

#define ASC_DMA	0x80		/* Enable DMA for the command */

/* Miscellaneous group. */
#define ASC_NOOP	0x00		/* No operation command */
#define ASC_FLUSH	0x01		/* FIFO flush command */
#define ASC_RESET	0x02		/* Reset ASC chip command */
#define ASC_RSTBUS	0x03		/* Reset SCSI bus command */

/* Initiator state group. */
#define ASC_XINFO	0x10		/* Transfer information command */
#define ASC_ICCS	0x11		/* Initiator command complete seq. */
#define ASC_MSGACPT	0x12		/* Message accept command */
#define ASC_XPAD	0x18		/* Transfer pad bytes command */
#define ASC_SETATN	0x1A		/* Set ATTN bus line command */

/* Target state group. */
#define ASC_SMSG	0x20		/* Send message command */
#define ASC_SSTAT	0x21		/* Send status command */
#define ASC_SDATA	0x22		/* Send data command */
#define ASC_DISSEQ	0x23		/* Disconnect sequence command */
#define ASC_TRMSEQ	0x24		/* Terminate sequence command */
#define ASC_TCCS	0x25		/* Target command complete seq. */
#define ASC_DISCON	0x27		/* Disconnect command */
#define ASC_RMSGSEQ	0x28		/* Receive message seq. command */
#define ASC_RCMD	0x29		/* Receive CDB command */
#define ASC_RDATA	0x2A		/* Receive data command */
#define ASC_RCMDSEQ	0x2B		/* Receive command seq. command */
#define ASC_TABORT	0x00		/* Target abort DMA command */

/* Disconnected state group. */
#define ASC_RESEL	0x40		/* Reselect sequence command */
#define ASC_SELECT	0x41		/* Select w/out ATN seq. command */
#define ASC_SELATN	0x42		/* Select w/ATN seq. command */
#define ASC_SELATNSTOP	0x43		/* Select w/ATN and stop seq. command */
#define ASC_ESELRSEL	0x44		/* Enable Sel/Resel command */
#define ASC_DSELRSEL	0x45		/* Disable Sel/Resel command */
#define ASC_SELATN3	0x46		/* Select w/ATN3 command */

/* Status Register */
#define	ASC_VGC		0x08		/* valid group code */
#define ASC_TC		0x10		/* terminal count */
#define ASC_PE		0x20		/* parity error */
#define ASC_GE		0x40		/* gross error */
#define ASC_INTP	0x80		/* interrupt pending */

/* SCSI phase control lines */
#define ASC_PHASE_MSK	0x07		/* mask for the SCSI phase bits */
#define	ASC_IO		0x01		/* SCSI bus I/O signal */
#define	ASC_CD		0x02		/* SCSI bus C/D signal */
#define	ASC_MSG		0x04		/* SCSI bus MSG signal */

/* Interrupt Register */

#define ASC_SEL		0x01		/* the ASC has been selected */
#define ASC_SATN	0x02		/* the ASC has been selected w/ATN */
#define ASC_RSEL	0x04		/* the ASC has been reselected */
#define ASC_FC		0x08		/* function complete */
#define ASC_BS		0x10		/* bus service */
#define ASC_DIS		0x20		/* disconnected */
#define ASC_ILLCMD	0x40		/* illegal command */
#define ASC_SCSIRST	0x80		/* SCSI bus reset */

/* Sequence Step Register */

#define ASC_SSTEP_MSK	0x07		/* mask for the sequence step bits */
#define ASC_NSOM	0x08		/* sync offset max signal (Neg AST) */

#define ASC_SSTEP_TO	0x00		/* Select timeout	*/
#define ASC_SSTEP_MESSO 0x01		/* Message Out complete	*/
#define ASC_SSTEP_SELNOCMD 0x02		/* Select complete but no */
                                        /* command phase */
#define ASC_SSTEP_CMPLT	0x04		/* select complete	*/

/* FIFO and Sequence step register */

#define ASC_FIFO_MSK	0x1F		/* mask for the FIFO flag bits */
#define ASC_FIFO_LEN	16

/* Configuration 1 register */

#define ASC_C1_IDMSK	0x07		/* mask for the ASC id bits *.
#define ASC_C1_CTEST	0x08		/* */
#define ASC_C1_PARITY	0x10		/* */
#define ASC_C1_PTEST	0x20		/* */
#define ASC_C1_SRD	0x40		/* */
#define ASC_C1_SLOW	0x80		/* */

/* Configuration 2 register */

#define ASC_C2_DPE	0x01		/* */
#define ASC_C2_RPE	0x02		/* */
#define ASC_C2_BPA	0x04		/* */
#define ASC_C2_SCSI2	0x08		/* */
#define ASC_C2_DREQHI	0x10		/* */
#define ASC_C2_EBC	0x20		/* */
#define ASC_C2_EPL	0x40		/* */
#define ASC_C2_RFB	0x80		/* */

/* Configuration 3 register */

#define ASC_C3_T8	0x01		/* */
#define ASC_C3_ALTDMA	0x02		/* */
#define ASC_C3_SRB	0x04		/* */

/* -------------------------------------------------------------------- */


#define ASC_MAX_EXTMSG_LEN	255



/* Misc. Macros and defines to use with the ASC. */

#define ASC_LOADCNTR( a, n )					\
{								\
    a->asc_tcmsb = ((n & 0xFF00) >> 8);				\
    wbflush();							\
    a->asc_tclsb = (n & 0xFF);					\
    wbflush();							\
    a->asc_cmd = ASC_DMA | ASC_NOOP;					\
    wbflush();							\
}

#define ASC_GETCNTR( a, n )					\
{								\
    n = ((a->asc_tcmsb << 8) & 0xFF00);				\
    n += (a->asc_tclsb & 0xFF);					\
}

#define	ASC_DMA_XLEN	8192		/* use an 8k working limit */
#define	ASC_TIMEOUT	0x99		/* magic # from ASC documents */

#define ASC_WAIT_COUNT	10000		/* Delay count used for the ASC chip */
#define ASC_ATN		1		/* mostly just a flag */



#define WHO_ASC		0
#define WHO_LOGERR	1

#define GET_SCSI_CMD_LEN(cmd)	((((cmd >> 5) & 0x07) == 1) ? 10 : 6)

#define ASC_BUS_ID_MASK	0x7		/* Only bits 0-2 are valid */

#define PHASE(sr)	(int)((sr & ASC_PHASE_MSK) << 2)

#define DMA_CMD(cmd)	(cmd | ASC_DMA)

#define ASC_SELECT_CMD(cmd)						\
        ( (cmd == DMA_CMD(ASC_SELECT)) ||				\
	  (cmd == DMA_CMD(ASC_SELATN)) || 				\
          (cmd == DMA_CMD(ASC_SELATNSTOP))				\
        )

#define ASCII_A		0x30

#define ASC_CNTLR_DEFAULT	7	/* Default SCSI id for controller */

#define BPFREE		-1

#define NO_DMA		0
#define DO_DMA		1

#define DBOFF_LEN	256

#define TARGBUF_START	2048
#define TARGBUF_LEN	((int)(16 * 1024))

#define ASC_MAX_XFER	((int)(64 * 1024))

/* Buffer used by double buffering logic */

#define OTHER_BUFF(x)		(1 - x)




/* Synchronous transfer register stuff.  Here is where these numbers came
   from.  The SII driver uses 63 (dec.) as the value for 'm' (where 'm' is
   defined as the divided by 4 nanoseconds), which computes to a value of
   252 ns. for the sync. transfer period.  The closest we can come as an
   approximation to this value for the 53C94 is 240 ns.  To compute these
   numbers, we took the 3Max clock speed (25 MHz) and converted to 
   nanoseconds (by taking the reciprocal of 25 * 10E06, which = .00000004 or
   40 ns.).  Since the value which is sent to the target in the Sync. Data
   Transfer Request is based on 4 nanoseconds, we must take the value which
   was chosen above (240 ns) and divide by 4, giving a value of 60.  This
   is the value 'm' which will be sent to the target as our proposed 
   transfer period.  Now we have to compute the value for the 53C94 Sync.
   Tranfer Period register.  Since the "real" time is 240 ns., we have to
   compute the register value based upon the table in the NCR specification
   and the clock speed of 3Max.  The table is reproduced below:

   		Register Value			Clocks Per Byte

		  0 0 1 0 0			        5
		  0 0 1 0 1				5
		  0 0 1 1 0				6
		  0 0 1 1 1				7

		      .					.
		      .					.
		      .					.
		      .					.

		  1 1 1 1 1				31
		  0 0 0 0 0				32
		  0 0 0 0 1				33
		  0 0 0 1 0				34
		  0 0 0 1 1				35

   Based upon this table, 5 clocks at 40ns per byte would be equivalent to
   200 ns.  The same calculation to yield 240 ns. would equal 6 (240/40).

   Of course, all of this is just to come up with a reasonable approximation
   of what our "ideal" values would be.  If the target cannot operate within
   these parameters, it will respond with its own, and we will meekly accept.

*/


#define ASC_SYNC_XFER_REG	7	/* value to program ASC Sync. Transfer */
    					/* Period register.		       */
    
#define ASC_SYNC_XFER_PER	70	/* = value 'm' in SCSI-1 spec. Table   */
                                        /* 5.6                                 */

#define ASC_SYNC_OFFSET		15 	/* Req/Ack offset    */
#define ASC_SYNC_OFFSET_BURST	7 	/* Req/Ack offset for 3min */

