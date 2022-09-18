
/*
 *	7/2/90	(ULTRIX)	@(#)sdcreg.h	4.1
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986,87,88 by			*
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
 ************************************************************************/

/***********************************************************************
 *
 * Modification History:
 *
 *   14-Jul-88 -- gmm 
 *	Added RX23 support
 *
 *   15-Feb-88 -- fred (Fred Canter)
 *	Added RD33 support.
 *
 *   06-Jan-87 -- gmm (George Mathew)
 *	Addeed RTRY_CNT
 *
 *   3-Dec-86  -- gmm (George Mathew)
 *	Added SD_SIZE
 *
 *   30-Sep-86 -- gmm (George Mathew)
 *	Added sd_flags (status flags) field in sd_st
 *
 *   3-Sep-86  -- gmm (George Mathew)
 *	Remove unnecesary comment.
 *
 *   5-Aug-86  -- gmm (George Mathew)
 *	More disk driver improvements.
 *
 *   2-Jul-86  -- gmm (George Mathew)
 *	Many driver improvements.
 *
 * 18-Jun-86  -- gmm (George Mathew)
 *	Created this header file for VAXstar disk driver.
 *
 **********************************************************************/


#define	FILL	1
#define	EMPTY	0

#define	SD_SIZE	512	/* size of a sector */
#define RTRY_CNT	10   /* max. retries for read/write */

/* RX parameters */
#define	NRXCYL		80

#define NRX50HDS	1
#define NRX50SECT	10

#define NRX33HDS	2
#define NRX33SECT	15

#define NRX23HDS	2
#define NRX23SECTH	18    /* High density: 18 sectors/track */
#define NRX23SECTD	9     /* Double Density: 9 sectors/track */
/* RD parameters */

#define	NRDSECT		17

#define NRD31CYL	615
#define	NRD31HDS	4

#define NRD32CYL	820
#define	NRD32HDS	6

#define NRD33CYL	1170
#define	NRD33HDS	7

#define	NRD53CYL	1024
#define	NRD53HDS	8

#define NRD54CYL	1225
#define	NRD54HDS	15

#define RX50_PCOMP	0004	/* Write precompensation for RX50 */
#define RX33_PCOMP	0001	/* Write precompensation for RX33 */
#define RD31_PCOMP	0002	/* Write precompensation for RD31 */

#define	dkc_reg	nb_dkc_reg	/* Register Data access */
#define	dkc_cmd	nb_dkc_cmd_stat	/* Controller command */
#define	dkc_stat nb_dkc_cmd_stat	/* Interrupt status */

/* !!! IMPORTANT !!! */
/* If the numeric value for the disk type is changed, or any new type added,
 * appropriate change to be done in sdst[] initialization in sdc.c. The order
 * in sdst[] SHOULD CORRESPOND to the values defined here */
#define	DT_RX50	0	/* RX50 */
#define	DT_RX33	1	/* RX33 */
#define DT_RX23H 2	/* RX23 High Density (18 sectors/track) */
#define DT_RX23D 3	/* RX23 Double Density (9 sectors/track) */
#define	DT_RD31	4	/* RD31 */
#define	DT_RD32 5	/* RD32 */
#define	DT_RD33 6	/* RD33 */
#define DT_RD53	7	/* RD53 */
#define DT_RD54	8	/* RD54 */

/* Media identifiers*/
#define MED_RD31	0x2564401F
#define MED_RD32	0x25644020
#define MED_RD33	0x25644021
#define	MED_RD53	0x25644035
#define MED_RD54	0x25644036

#define	SINT_DC	0001	/* Disk controller bit in Interrupt cntlr. regs.*/

#define NDRIVES	3
#define HLDELAY		0	/* Head load delay */
#define	DTRT_HDSK	0004	/* Data rate:Hard disk with 4-byte ID fields */
#define DTRT_RX33	0010	/* Data rate:RX33 */
#define DTRT_RX50	0014	/* Data rate:RX50 */
#define DRV_NUM0	0000	/* First Hard disk drive */
#define	DRV_NUM1	0001	/* Second Hard disk drive */
#define DRV_NUM2	0002	/* First Diskette drive */
#define	DRV_NUM3	0003	/* Not used */

#define	SK_STEP		0004	/* Step bit for Seek Command */
#define	SK_WAIT		0002	/* Wait for seek complete */
#define	SK_VERFY	0001	/* Verify position */

/* dkc_cmd */

#define SD_RESET	0000	/* RESET */
#define	SD_SETREG	0100	/* Set Reg. Pointer :bits 0-3 reg.no */
#define SD_DESEL	0001	/* Deselect Drive */
#define	SD_SELECT	0040	/* Drive Select: bits 0-1 Drive no,
				   bits 2-3: Data rate, bit 4:Head load delay */
#define	SD_RESTOR	0002	/* Restore Drive: bit 0: Wait for seek complete */
#define SD_STEP		0004	/* Step; bit 0:Wait for seek complete,
					 bit 1: Direction of motion */
#define	SD_POLL		0020	/* Poll Drives; bit x: Drive x (x: 0-3) */
#define SD_SEEK		0120	/* Seek/Read ID ; bit 0: verify position
				   bit 1: Wait for seek complete,
				   bit 2: Seek to desired cylinder */
#define SD_FMT		0140	/* Format Track; bit 0: Write Precompensation
					bit 1: Reduced write current
					bit 2: Deleted data mark */
#define	SD_RDTR		0132	/* Read Track; bit 0: Transfer Data field */
#define	SD_RDPHY	0130	/* Read Physical; bit 0: Transfer data */
#define	SD_RDLOG	0134	/* Read Logical; bit 0: Transfer data,
					bit 1: Bypass bad sectors */
#define	SD_WRPHY	0200	/* Write Physical; bits 0-2: Write precompensation,
					bit 3: Reduced Write Current
					bit 4: Deleted data mark
					bit 6: Bypass bad sectors */
#define SD_WRLOG	0240	/* Write Logical; bits 0-2: Write precompensation,
					bit 3: Reduced write current
					bit 4: Deleted data mark
					bit 6: Bypass bad sectors */

/* Read command bits*/
#define	RD_BYPS		0002	/* Bypass bad sectors */
#define	RD_XFER		0001	/* Transfer data */

/* Write command bits */
#define	WR_BYPS		0100	/* Bypass bad sectors */
#define	WR_DDMRK	0020	/* Deleted data mark */

/* Step command bits */
#define	STEP_OUT	0002	/* Direction of motion */
#define	STEP_WAIT	0001	/* Wait for seek complete */

/* Restore command bits */
#define REST_WAIT	0001	/* Wait for seek complete */

/* Vaxstar Disk reg. nos */
#define	UDC_DMA7	0x0	/* DMA address bits 7:0 */
#define	UDC_DMA15	0x1	/* DMA address bits 15;8 */
#define	UDC_DMA23	0x2	/* DMA address bits 23:16 */
#define	UDC_DSECT	0x3	/* Desired sector */
#define	UDC_DHEAD	0x4	/* Desire head (wo) */
#define	UDC_CHEAD	0x4	/* Current head (ro) */
#define	UDC_DCYL	0x5	/* Desired cylinder (wo) */
#define	UDC_CCYL	0x5	/* Current cylinder (ro) */
#define	UDC_SCNT	0x6	/* Sector count (wo) */
#define	UDC_RTCNT	0x7	/* Retry count (wo) */
#define	UDC_MODE	0x8	/* Operating mode (wo) */
#define	UDC_CSTAT	0x8	/* Chip status (ro) */
#define UDC_TERM	0x9	/* Termination conditions (wo) */
#define	UDC_DSTAT	0x9	/* Drive Status (ro) */
#define	UDC_DATA	0xA	/* Data (r/w) */

/* UDC_TERM bits */
#define	TERM_CRC	0200	/* CRC register preset */
#define	TERM_INT	0040	/* Interrupt on done */
#define	TERM_DEL	0020	/* Terminate on deleted data */
#define	TERM_CART	0010	/* Terminate on cartridge change */
#define	TERM_WRPR	0004	/* Terminate on write protect */
#define	TERM_RDCH	0002	/* Interrupt on ready change */
#define	TERM_WRFL	0001	/* Terminate on write fault */

/* UDC_CSTAT bits */
#define	CST_RET		0200	/* Retry required */
#define	CST_ECCAT	0100	/* Error correction attempted */
#define	CST_ECCER	0040	/* ECC/CRC error */
#define	CST_DELDT	0020	/* Deleted Data mark */
#define	CST_SYNER	0010	/* Synchronization error */
#define	CST_CMPER	0004	/* Compare error */
#define	CST_DRSEL	0003	/* Drive selected */

/* UDC_DSTAT bits */
#define DST_SELACK	0200	/* Select acknowledge */
#define DST_INDEX	0100	/* Index point */
#define DST_SKCOM	0040	/* Seek complete */
#define	DST_TRK00	0020	/* Track 0 */
#define DST_CARTCH	0010	/* Cartridge changed */
#define	DST_WRPROT	0004	/* Write protect */
#define DST_READY	0002	/* Drive Ready */
#define	DST_WRFAULT	0001	/* Write fault */

/* UDC_RTCNT bits */
#define	RT_CNT		0360	/* Retry count, in one's complement form */
#define	RT_INVRDY	0004	/* Invert ready */
#define	RT_MOTOR	0002	/* Motor on */
#define	RT_LOSPD	0001	/* Diskette speed select */

/* UDC_MODE bits */
#define	MOD_HD		0200	/* hard disk mode ( 1 for both hard disk and diskettes*/
#define	MOD_CHKCRC	0000	/* CRC code for diskettes */
#define	MOD_CHKECC	0100	/* ECC code for hard disk */
#define	MOD_DENS	0000	/* density: 0 for all cases */
#define	MOD_SRTRXL	0001	/* RX33 at 300 rpm/250 KHz */
#define	MOD_SRTRXH	0002	/* RX33 at 360 rpm/500 KHz */
#define	MOD_SRTRDN	0000	/* RDxx normal */
#define	MOD_SRTRDR	0006	/* RDxx Restore */
/* dkc_stat bits */
#define	DKC_INTPEND	0200	/* bit 7: Interrupt pending */
#define DKC_DMAREQ	0100	/* bit 6: DMA request */
#define	DKC_DONE	0040	/* bit 5: Command done */
#define DKC_TERMCOD	0030	/* bit 4/3: Termination code */
#define DKC_RDYCHNG	0004	/* bit 2: Ready change */
#define	DKC_OVRUN	0002	/* bit 1: Overrun/underrun */
#define DKC_BADSECT	0001	/* bit 0: Bad sector */

/* DKC_TERMCOD: termination codes */
#define DKC_SUCCESS	0000	/* Successful completion */
#define	DKC_RDERR	0010	/* Error in READ ID sequence */
#define	DKC_VERERR	0020	/* Error in VERIFY sequence */
#define	DKC_DATERR	0030	/* Error in Data Transfer sequence */


/* Controller status */
#define	CNT_NOTBSY	0x0	/* Controller not ready */
#define	CNT_BSY		0x01	/* Controller in use */
#define	CNT_DONE	0x02	/* Controller command done */
#define	CNT_ERR		0x04	/* Error in last controller command */

#define	MAXLEN		15872	/* max. length of usable buffer. */

#define BBR_NORETRY	1	/* no retry read/write while handling bad blocks */
#define BBR_RETRY	2	/* retry read/write while handling bad blocks */
struct sd_st {
	short	sd_status;	/* controller status */
	short	sd_drno;	/* drive number currently transferring */
	int	sd_blkno;	/* block no */
	int	sd_nsect;	/* no. of sectors being written */
	short	sd_hd;	/* current head */
	union {
		short	sd_word;
		u_char	sd_byte[2];
	}sd_cyl;		/* current cylinder */
	int	sd_bleft;	/* bytes left */
	int	sd_bcount;	/* no. bytes being transferred */
	u_char	sd_cmd;		/* last command */
	struct	buf *sd_buf;	/* buffer pointer */
	char	*sd_addr;
	short 	sd_type[NDRIVES];	/* disk/diskette type */
	int	sd_softcnt[NDRIVES];	/* no. of soft errors */
	int	sd_hardcnt[NDRIVES];	/* no. of hard errors */
	long	sd_flags[NDRIVES];	/* status flags */
	struct	ucb{
		union{
			int xbnsize;	/* no. of XBNs */
			short xbn_short[2];
		} xbn_un;
		union{
			int dbnsize;	/* no. of DBNs */
			short dbn_short[2];
		} dbn_un;
		union{
			int lbnsize;	/* no. of LBNs */
			short lbn_short[2];
		} lbn_un;
		union{
			int rbnsize;	/* no. of RBNs */
			short rbn_short[2];
		} rbn_un;
		union{
			int media;	/* media identifier */
			short med_short[2];
		} med_un;
		union{
			int volume;	/* serial no. of unit */
			short vol_short[2];
		}vol_un;
		int lbnbase;  	/* block no. of the first LBN */
		int rbnbase;	/* block no. of first RBN */
		int hostsize;	/* no. of available  host LBNs */
		int oldrbn;	/* during BBR, old rbn */
		int newrbn;	/* during BBR, new rbn */
		short badsect;	/* bad sector no */
		int badbn;	/* bad block no. being replaced */
		short blk_type;	/* type of block: host/rct/rbn etc */
	}ucb[NDRIVES];
} ;

/* block types */
#define	BLK_HOST	0	/* host area block */
#define	BLK_RCTGET	1	/* Reading RCT first time */
#define	BLK_RCTPUT	2	/* BBR */
#define	BLK_RBN		3	/* working on RBN */
#define	BLK_BAL		4	/* working on rest of sectors after bad block */

#define         bit0            0x00000001
#define         bit1            0x00000002
#define         bit2            0x00000004
#define         bit3            0x00000008
#define         bit4            0x00000010
#define         bit5            0x00000020
#define         bit6            0x00000040
#define         bit7            0x00000080
#define         bit8            0x00000100
#define         bit9            0x00000200
#define         bit10           0x00000400
#define         bit11           0x00000800
#define         bit12           0x00001000
#define         bit13           0x00002000
#define         bit14           0x00004000
#define         bit15           0x00008000

/*
 *  The structure sd_uib taken from uib.h in rqdx3 microcode
 *
 *  definition of uib structure
 *
 *  filler_1		-- header bytes (for data integrity)
 *  xbnsize		-- number of XBNs
 *  dbnsize		-- number of DBNs
 *  lbnsize		-- number of LBNs
 *  rbnsize		-- number of RBNs
 *  sec			-- actual number of sectors (per track)
 *  sur			-- actual number of surfaces
 *  cyl			-- actual number of cylinders
 *  pccyl		-- first cylinder for (write) precompensation
 *  rccyl		-- first cylinder for reduced (write) current
 *  seekrate		-- rate at which seeks take place (zero for buffered)
 *  crc_or_ecc		-- 0 for CRC, 1 for ECC
 *  rctsize		-- size of unit's replacement control table
 *  rctcopies		-- number of RCT copies
 *  media		-- media identifier (creation of Ed Gardner)
 *  sec_interleave	-- sector-to-sector interleave factor
 *  sur_skew		-- surface-to-surface skew amount
 *  cyl_skew		-- cylinder-to-cylinder skew amount
 *  gap0		-- gap0 value for reformatted tracks
 *  gap1		-- gap1 value for reformatted tracks
 *  gap2		-- gap2 value for reformatted tracks
 *  gap3		-- gap3 value for reformatted tracks
 *  sync		-- sync value for reformatted tracks
 *  filler_2		-- stuff specific to the DUP formatter
 *  volume		-- serial number of unit
 */

#define UIBSIZE	100
#define FILLER_1	10
struct sd_uib {
			char		filler_1[FILLER_1];
			short		xbnsize[2];
			short		dbnsize[2];
			short		lbnsize[2];
			short		rbnsize[2];
			short		sec;
			short		sur;
			short		cyl;
			short		pccyl;
			short		rccyl;
			short		seekrate;
			short		crc_or_ecc;
			short		rctsize;
			short		rctcopies;
			short		media[2];
			short		sec_interleave;
			short		sur_skew;
			short		cyl_skew;
			short		gap0;
			short		gap1;
			short		gap2;
			short		gap3;
			short		sync;
			char		filler_2[32];
			short		volume[2];
};


