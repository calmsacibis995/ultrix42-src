#ifndef lint
static char *sccsid = "@(#)sdc.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986,87,88 by			*
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
 *
 * Modification History:
 *   Apr-1-91	Matthew Sacks
 *	Changed rx23_type() so that it does a ready check and drive
 *	restore before doing the I/O operation which distinguishes high
 *	density (2880) from double density (1440) floppy disks.
 *	Otherwise, this density test causes Sync errors when
 *	floppy cartridges are interchanged.
 *
 *   30-Dec-88	Fred Canter
 *	Added sdreset() to reset ST506 controllers before calling the
 *	VMB boot drive to write out a crash dump.
 *
 *   28-Sep-88 Fred Canter
 *	Clean up comments. Use CFGTST regiser bit to determine floopy
 *	drive type, i.e., RX33 or RX23.
 *
 *   08-Aug-88 Fred Canter
 *	Make probe fail if not drives present (PVAX only).
 *	The installation will generate a bad config file if the
 *	sdc controller is present with no drives (undefined sdintr).
 *
 *   14-Jul-88 George Mathew
 *	Added support for RX23 floppy drive
 *
 *   19-May-88 Fred Canter
 *	Changes for operating in either compatibility or
 *	extended I/O mode on CVAXstar/PVAX.
 *
 *   15-Feb-88 Fred Canter
 *	Changes for VAX420 (CVAXstar/PVAX) support.
 *	Added RD33 support.
 *	Changed sd_delay from one instruction to a macro.
 *
 *   01-Jun-87 George Mathew
 *	Fix the problem with spurious deleted data mark. Check for Write Fault
 *	before Deleted Data Mark!
 *
 *   28-Apr-87 George Mathew
 *	DEVIOCGET ioctl reports correctly if the diskette is write preotected
 *
 *   23-Apr-97 darrell
 *	Changed the calls to vs_bufctl to pass a pointer to a structure
 *	that contains a pointer to the routine that vs_bufctl is to 
 *	call in this driver.
 *
 *   12-Mar-87 George Mathew
 *	Fix for the protection fault: if going through bbr code (using 
 *	rdwr_poll for read/write) and the no. of bytes to be transferred
 *	from the last sector in the track containing the bad sector is not
 *	512, transfer the correct no. of bytes.
 *
 *   11-Feb-87 gmm (George Mathew)
 *	Removed all the debug stuff related to deleted data mark (undid 
 *	most of 01/15/87 work). Included a couple of more delays hoping to
 *	get rid of the spurious deleted data marks.
 *
 *   28-Jan-87 gmm (George Mathew)
 *	Restore to track 0 whenever starting on a new diskette (Causes
 *	sync error otherwise). Put the last drive serviced at the end of the
 *	queue for the next round. Made some changes to take care of lint
 *	warning messages
 *
 *   15-Jan-87 gmm (George Mathew)
 *	Does not report I/O error if deleted data mark not written by the
 *	driver (through BBR) is reported by the controller. But error
 *	messages are printed. (To be removed before SDC)
 *
 *   06-Jan-87 gmm (George Mathew)
 *	Updated BBR code to the latest algorithm (ECO #20). BBR messages
 *	made the same as of uda. Added ioctl support for radisk. Removed
 *	sdreset(). Changes to diskette drive.
 *
 *   3-Dec-86  gmm (George Mathew)
 *	Performance enhancements. Changed the way deleted data mark is
 *	reported. First version after field test.
 *
 *   8-Oct-86  gmm (George Mathew)
 *	Fix a bug in bbr: if the sector is not bad, but the data
 *	is invalid (Forced error bit set) the block no. is correctly 
 *      calculated in sd_rpl() .
 *
 *   6-Oct-86  gmm (George Mathew)
 *	Check validity of unit number before status field updated in the
 *	open routine (sdopen()).
 *
 *   20-Sep-86 gmm (George Mathew)
 *	BBR bug fix: RCT gets updated correctly even if not the first one.
 *	Some changes for RX50 read/write to be more robust.
 *	DEVIOCGET ioctl updates stat field correctly.
 *
 *   9-Sep-86  gmm (George Mathew)
 *      Changed the way presence of RX33 drive is detected in sdslave.
 *	Improved some of the error messages
 *
 *   4-Sep-86  gmm (George Mathew)
 *	Improvements, bug fixes, and cleanup.
 *
 *  27-Aug-86  gmm (George Mathew)
 *	Many changes: RX50 improved performance, BBR improvements, and
 *	general cleanup.
 *
 *  26-Aug-86 -- rsp (Ricky Palmer)
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry and (2) use strlen instead of fixed storage
 *	for bcopy's.
 *
 *  14-Aug-86  gmm (George Mathew)
 *	Several driver improvements and clean out debug messages.
 *	Change slave names from sd to rd/rx.
 *
 *   5-Aug-86  gmm (George Mathew)
 *	Extensive rewrite for real VAXstar disk driver.
 *
 *   2-Jul-86  gmm (George Mathew)
 *	Added partial devioctl support and many improvements to driver.
 *
 *  18-Jun-86  gmm (Geroge Mathew)
 *      Created this VAXstar RD/RX disk driver file.
 *
 **********************************************************************/


#include "rd.h"
#include "rx.h"
#if NRX > 0
#define	NSX	1
#else
#define	NSX	0
#endif
#define		NSD	NRD+NSX

#if defined(BINARY) || NSD > 0
int	sdpip;		/* DEBUG */
int	sdnosval;	/* DEBUG */

extern int cvs_exmode_on;

#include "../data/sdc_data.c"


int	sdprobe(), sdslave(), sdattach(), sdintr(), sdustart();

u_short	sdstd[] = { 0 };
struct	uba_driver sdcdriver =
 { sdprobe, sdslave, sdattach, 0, sdstd, "rd", sddinfo, "sdc", sdminfo, 0 };

struct	sdspace	SD_bufmap[];
struct	vsdev vsdiskdev = { VS_SDC, 0, sdustart };

/* !!! IMPORTANT !!!*/
/* If any new disk type added to this structure (sdst), make sure the type 
 * number defined in sdcreg.h and the position of the new disk in this 
 * sturcture MATCH. */

struct	sdst {
	short	nsect;  /* no. of sectors(blocks) per track */
	short	ncyl;   /* no. of cylinders */
	short	nheads; /* no. of heads or tracks per cylinder */
	short	nspc;	/* no. of sectors(blocks) per cylinder */
	struct	size *sizes;
} sdst[] = {
	NRX50SECT,NRXCYL,NRX50HDS,NRX50SECT*NRX50HDS,sd_rx50_sizes,
	NRX33SECT,NRXCYL,NRX33HDS,NRX33SECT*NRX33HDS,sd_rx33_sizes,
	NRX23SECTH,NRXCYL,NRX23HDS,NRX23SECTH*NRX23HDS,sd_rx23h_sizes,
	NRX23SECTD,NRXCYL,NRX23HDS,NRX23SECTD*NRX23HDS,sd_rx23d_sizes,
	NRDSECT,NRD31CYL,NRD31HDS,NRDSECT*NRD31HDS,sd_rd31_sizes,
	NRDSECT,NRD32CYL,NRD32HDS,NRDSECT*NRD32HDS,sd_rd32_sizes,
	NRDSECT,NRD33CYL,NRD33HDS,NRDSECT*NRD33HDS,sd_rd33_sizes,
	NRDSECT,NRD53CYL,NRD53HDS,NRDSECT*NRD53HDS,sd_rd53_sizes,
	NRDSECT,NRD54CYL,NRD54HDS,NRDSECT*NRD54HDS,sd_rd54_sizes,
};

short	rx_table[50] =
		{ 1, 3, 5, 7, 9, 2, 4, 6, 8, 10,
		3, 5, 7, 9, 1, 4, 6, 8, 10, 2,
		5, 7, 9, 1, 3, 6, 8, 10, 2, 4,
		7, 9, 1, 3, 5, 8, 10, 2, 4, 6,
		9, 1, 3, 5, 7, 10, 2, 4, 6, 8 };

struct buf sdcbuf;	/* Pointer to controller queue */
char xbnbuf[SD_SIZE],tmpbuf[SD_SIZE],rct0[SD_SIZE],rct1[SD_SIZE],rct2[SD_SIZE],rct3[SD_SIZE];
short xbnflag = 0;
short rtr_cnt = 0;   /* no. of times read/write retried */
short dsket_type = -1; /* type of diskette */
int ddm_err = 0;  /* Deleted Data Mark error */
int rx50blk, rx50nsect;  /* starting block no. and no. of sectors for each 
			 * transfer for RX50 */
/*
 * HDC 9224 chip register access delay macro.
 * Must insure a minimum of 700 ns between disk
 * controller register accesses.
 * CVAXstar vs VAXstar CPU speed complicates this issue.
 * Best we can to is make sure delay long enough for faster CPU.
 */
int	sd_delay = 0;
#define	sdc_delay()	sd_delay = 1; sd_delay = 1; sd_delay = 1;

char *HARD_ERR = "HARD ERROR";
char *SOFT_ERR = "SOFT ERROR";
char *DEV_ID = "sd";

int     sd_rx23wakeup();
int sd_bbrcount = RTRY_CNT;   /* change this to adjust the retry count in 
		* sd_retry() while doing read/write for bbr. sd_bbrcount 
		* should be >= 1 !! */
int sddebug = 0;
int sdbbrdbug = 0;
int rx_reselect = 0;
int xbn_sum = 0;
int xbn_check = 0;
int start_sn ;	   /* physical starting sector no. in the track on which I/O *
		    * is being done */
int sd_poll = 0;   /* controller in poll mode */
int sd_nodelay = 0;  /* NODELAY flag for open */
int sd_openerr = 0;  /* Error in opening the device */
int sd_rxcyl = 0;  /* the last cylinder no. for Rx drive, used by STEP */


u_char rbn_addr1,rbn_addr2;

	 /***********************
	 * !!! DO NOT use BBR_TST option unless you are absolutely sure of 
	 * what you are doing. It should be used only for debugging bad block
	 * replacement code. !!! 
	 ************************/
#ifdef BBR_TST
	int bbr_sleep;
	int bbr_force;  /* setting this to non zero forces bbr on the block 
			 * even if found good in STEP 7 of put_rbn() */
#endif BBR_TST

extern struct nexus nexus[];
#define DELAYTEN	1000
#define LOOP_DELAY	1000000
	/* for the following macros: i=unit, j=partition */
#define BAD_LBN(i)	sd_st.sd_blkno-sd_st.ucb[i].lbnbase-sd_st.sd_nsect+sd_st.ucb[i].badsect
#define BAD_SECT(i,j)	sd_st.sd_blkno-sd_st.ucb[i].lbnbase-sd_st.sd_nsect-sd_part[i].pt_part[j].pi_blkoff+sd_st.ucb[i].badsect

extern	int	cpu;
extern	int	cpu_subtype;

sdprobe(reg, cntlr)
	caddr_t reg;
	int cntlr;
{

	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	register struct nb_regs *sdiaddr = (struct nb_regs *)nexus;
	int count = 0;
#ifdef lint	
	sdintr(0);
	reg = reg;
#endif

	/*
	 * ONLY on a VAXstar/TEAMmate
	 * Also CVAXstar/PVAX
	 */
	if ((cpu != VAXSTAR) && (cpu != C_VAXSTAR))
		return(0);
	/*
	 * Only if ST506 disk controller configured.
	 * PVAX server uses 2nd SCSI in place of ST506 controller.
	 */
	if ((vs_cfgtst & VS_SC_TYPE) != VS_ST506_SCSI)
		return(0);

	/*
	 * Don't allow controller to configure if no drives present
	 * (PVAX only). The installation will build a config file
	 * with the sdc0 controller, but no slaves on a PVAX with
	 * the ST506/SCSI controller if no ST506 drives are found.
	 * The causes the kernel build fail (undefined sdintr).
	 */
	if ((vs_cfgtst & (VS_DRV0PR | VS_DRV1PR | VS_DRV2PR)) == 0x0700)
		return(0);

	sdiaddr->nb_int_msk |= SINT_DC;
	sdaddr->dkc_cmd = SD_RESET;
	while(count < DELAYTEN) {
		if(sdaddr->dkc_stat & DKC_DONE )
			break;
		DELAY(10);
		count++;
	}
	if (count == DELAYTEN){
		printf("%s%c:%s: SD_RESET failed in sdprobe\n",DEV_ID,'c',HARD_ERR);
		return(0);
	}
	sdaddr->dkc_cmd = (SD_SETREG | UDC_TERM);
	sdc_delay();
	if((sdaddr->dkc_stat & DKC_DONE) == 0) {
		printf("%s%c:%s: Set Register Pointer command failed in sdprobe\n",DEV_ID,'c',HARD_ERR);
		return(0);
	}
	sdaddr->dkc_reg = (TERM_CRC | TERM_INT | TERM_DEL | TERM_WRPR |  TERM_WRFL);
	sdc_delay();

	sdaddr->dkc_cmd = SD_DESEL;   /* just to generate an interrupt */

	return(8);
}

sdslave(ui)
	struct uba_device *ui;
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	u_char cmd;

	sdaddr->dkc_cmd = (SD_SETREG | UDC_DHEAD);
	sdc_delay();
	sdaddr->dkc_reg = 0;  /* Play safe by using head 0 here */

	/* RESET TERM_CODES AGAIN?? */
	sdc_delay();
	sdaddr->dkc_cmd = (SD_SETREG | UDC_TERM);
	sdc_delay();
	sdaddr->dkc_reg = (TERM_CRC | TERM_INT | TERM_DEL | TERM_WRPR |  TERM_WRFL);

	sdc_delay();
	sdaddr->dkc_cmd = (SD_SETREG | UDC_RTCNT);
	switch(ui->ui_slave) {
		case 0:
		case 1:
		    sdaddr->dkc_reg = RT_CNT;  /* UDC_RTCNT */
		    sdc_delay();
		    sdaddr->dkc_reg = (MOD_HD | MOD_CHKECC | MOD_SRTRDN);  /* UDC_MODE */
		    sd_st.sd_drno = ui->ui_slave;
		    sd_st.sd_type[ui->ui_slave] = -1;
		    cmd = (SD_SELECT | DTRT_HDSK | ui->ui_slave);
		    if(sd_select(cmd,0)) {
			if(sddebug >=2) cprintf("%s%d:%s: Not selected\n",DEV_ID,ui->ui_slave,HARD_ERR);
			sd_st.sd_type[ui->ui_slave] = -1;
			    return(0);  
		    }
		    if(sd_rdfmt(ui->ui_slave)){
			printf("%s%d:%s:cannot read XBN\n",DEV_ID,ui->ui_slave,HARD_ERR);
			sd_st.sd_type[ui->ui_slave] = -1;
			return(0);
		    }
		    /* media type (sd_st.sd_type[]) updated in sd_rdfmt() */

		    /* Sanity check on RCT to see if bbr was stopped in the middle */
		    if(put_rbn(-1)) { /* Sanity check on RCT to see if bbr was stopped in the middel */
			printf("%s%d:%s: CANNOT RECOVER FROM PREVIOUS BBR\n",DEV_ID,ui->ui_slave,HARD_ERR);
		    }
		    sd_st.sd_cyl.sd_word = -1;
		    sd_st.sd_drno = -1;
		    ui->ui_type = sd_st.sd_type[ui->ui_slave];
		    return(1);
		    /* break; */

		case 2:
		    sd_st.sd_drno = ui->ui_slave;
		    sdaddr->dkc_reg = (RT_CNT | RT_INVRDY |RT_MOTOR);  /* UDC_RTCNT */
		    sdc_delay();
		    sdaddr->dkc_reg = (MOD_HD | MOD_CHKCRC | MOD_SRTRXH);   /* UDC_MODE */
		    rx_reselect = 1;
		    cmd = (SD_SELECT | DTRT_RX50 | DRV_NUM2);
		    if(sd_select(cmd,1)) {
			/* Select again to set READY in UDC_DSTAT */
			sdaddr->dkc_cmd = (SD_SETREG | UDC_RTCNT);
			sdc_delay();
			sdaddr->dkc_reg = (RT_CNT | RT_MOTOR);
			rx_reselect = 1;
			if(sd_select(cmd,1)) {
			    if(sddebug >=2) cprintf("%s%d:%s: Not selected \n",DEV_ID,ui->ui_slave,HARD_ERR);
			    return(0);   
			}
			/* if a diskette drive is present, TRK0 should be 
			 * set when RESTORE issued */
			if( sd_restore(1)) {
			    if(sddebug >=2) cprintf("%s%d:%s: Not selected\n",DEV_ID,ui->ui_slave,HARD_ERR);
				return(0);
			}

		    }
		    /* Even if there is no diskette drive, READY bit in
		     * UDC_DSTAT is 1 !!. So restore to see if drive
		     * really present */
		    else {
			if(sd_restore(1)) {
			    if(sddebug >=2) cprintf("%s%d:%s: Not selected\n",DEV_ID,ui->ui_slave,HARD_ERR);
			    return(0);
			}
		    }
		    sd_st.sd_cyl.sd_word = -1;
		    sd_st.sd_drno = -1;
		    sd_rxtype(ui->ui_slave);
		    /* On PVAX, CFGTST reg bit for floppy type (RX23 or RX33) */
		    if ((cpu == C_VAXSTAR) && ((vs_cfgtst&VS_DRV2RX33) == 0)) {
			    sd_rx23htype(ui->ui_slave);   
			    ui->ui_type = DT_RX23H; 
			    sd_st.sd_type[ui->ui_slave] = DT_RX23H;
		    }
		    else {
			    sd_rx33type(ui->ui_slave);   
			    ui->ui_type = DT_RX33; 
			    sd_st.sd_type[ui->ui_slave] = DT_RX33;
		    }
		    return(1);
		    /* break; */

		default:
			mprintf("%s%d:%s: wrong unit number for sdslave\n",DEV_ID,ui->ui_slave,SOFT_ERR);
			return(0);
	}
}

sdattach(ui)
	register struct uba_device *ui;
{

	/* Initialize iostat values */
	if(ui->ui_dk >= 0) {
		dk_mspw[ui->ui_dk] = .0000032;  /* 16bit transfer time */
	}
	sd_st.sd_softcnt[ui->ui_slave] = 0;  /* initialize soft error count */
	sd_st.sd_hardcnt[ui->ui_slave] = 0;  /* initialize hard error count */
	ui->ui_flags = 0;
}
 
sdopen(dev,flag)
	dev_t dev;
	int flag;
{
	register int unit = minor(dev) >> 3;
	register struct uba_device *ui;
	register int i;				
	int sdstrategy();			

	if (unit >= nNSD || (ui = sddinfo[unit]) == 0 || ui->ui_alive == 0)
		return (ENXIO);
	sd_st.sd_flags[unit] = 0;


	/* The diskette is checked to be on line everytime it is opened */
	if( (unit == DRV_NUM2) && (flag & FNDELAY) )
		sd_nodelay = 1;
	else
		sd_nodelay = 0;
	if(unit == DRV_NUM2) {
		dsket_type = -1;
		sd_rxcyl = 0; 
	}

	/*
	 *	See if we need to read in the partition table from the disk.
	 *	The conditions we will have to read from the disk is if the
 	 *	partition table valid bit has not been set for the volume
	 *	is invalid.
	 */

	/*
	 *	Assume that the default values before trying to
	 *	see if the partition tables are on the pack. The
	 *	reason that we do this is that the strategy routine
	 *	is used to read in the superblock but uses the 
	 *	partition info.  So we must first assume the
	 *	default values.
	 */

	/* If opening drive 2 (Diskette), read the partition table every time 
	 * since it can be either RX50/RX33/RX23H/RX23D diskette */

	if ( (sd_part[unit].pt_valid == 0) || (unit == DRV_NUM2) ) {
		for( i = 0; i <= 7; i++ ) {
			sd_part[unit].pt_part[i].pi_nblocks = 
				sdst[ui->ui_type].sizes[i].nblocks;
			sd_part[unit].pt_part[i].pi_blkoff =
				sdst[ui->ui_type].sizes[i].blkoffs;
		}

		sd_part[unit].pt_valid = PT_VALID;	

	/*
	 *	Default partition are now set. Call rsblk to set
	 *	the driver's partition tables, if any exists, from
	 *	the "a" partition superblock
	 */

		rsblk( sdstrategy, dev, &sd_part[unit] ); 
		if(sd_openerr) {
			sd_openerr = 0;
			if(sd_nodelay) {
				sd_nodelay = 0;
				return(0);
			}
			else
				return(EIO);
		}
	}
	return (0);
}

sdstrategy(bp)
	register struct buf *bp;
{
	register int unit;
	register struct buf *dp;
	register struct pt *pt;
	register s;
	register xunit = minor(bp->b_dev) & 07;
	register struct uba_device *ui;
	register struct vsdev *vd;

	vd = &vsdiskdev;
	unit = dkunit(bp);
	pt = &sd_part[unit];
	if (unit >= nNSD) {
		printf("%s%d:%s: sdstrategy: unit number wrong\n",DEV_ID,unit,SOFT_ERR);
		goto bad;
	}
	ui = sddinfo[unit];
	if (ui == 0 || ui->ui_alive == 0) 
		goto bad;
	/*
	 *	Get partition table for the pack
	 */

	if ( sd_part[unit].pt_valid != PT_VALID )
		if(sd_st.sd_flags[unit] & DEV_OFFLINE)
			goto bad;
		else
			panic("sdstrategy: invalid partition table ");

	s = spl5();
	/* First link the buffer onto the drive queue */
	dp = &sdutab[unit];

	/* if it is a diskette, no sorting done since the type of diskette found
	 * out only later */

	if( unit == DRV_NUM2) {
		if(dp->b_actf == 0)
			dp->b_actf = bp;
		else
			dp->b_actl->av_forw = bp;
		dp->b_actl = bp;
		bp->av_forw = 0;
	}
	else {

#define b_cylin b_resid

		bp->b_cylin = (bp->b_blkno + sd_st.ucb[unit].lbnbase + pt->pt_part[xunit].pi_blkoff)/sdst[sd_st.sd_type[unit]].nspc;
		disksort(dp,bp);
	} 

	/* Link the drive onto the controller queue */
	if (dp->b_active == 0) {
		dp->b_forw = NULL;
		if (sdcbuf.b_actf == NULL )
			sdcbuf.b_actf = dp;
		else
			sdcbuf.b_actl->b_forw = dp;
		sdcbuf.b_actl = dp;
		dp->b_active = 1;
	}
	if(sdcbuf.b_active == 0) {
		sdcbuf.b_active = 1;
		/* sdustart() gets called from vs_bufctl() which allows *
		 * the disk controller and TK50 controller to share the *
		 * 16K buffer */
		vd->vsd_action = VS_ALLOC;
		if ((cpu == C_VAXSTAR) && cvs_exmode_on)
			sdustart();
		else
			vs_bufctl(vd);
	}
	splx(s);
	return;

bad:
	bp->b_flags |= B_ERROR;
	iodone(bp);
	return;
}

sdustart()
{
	register struct buf *bp,*dp;
	register int unit;
	register struct pt *pt;			
	register npf,o;
	register struct vsdev *vd;
	struct pte *pte, *mpte;
	struct proc *rp;
	struct uba_device *ui;
	int maxsize;

	int xunit,i;
	long sz;
	unsigned v;

	vd = &vsdiskdev;

loop:

	/* Check if any request pending for the controller */
	if ((dp = sdcbuf.b_actf) == NULL) {
		sdcbuf.b_active = 0;
		/* deallocate the 16K shared buffer */
		return(VS_DEALLOC);
	}

	/* Check if any request pending for the drive */
	if ((bp = dp->b_actf) == NULL) {
		dp->b_active = 0;
		sdcbuf.b_actf = dp->b_forw;
		goto loop;
	}


	/* Move drive to the end of the controller queue */
	if( dp->b_forw != NULL ) {
		sdcbuf.b_actf = dp->b_forw;
		sdcbuf.b_actl->b_forw = dp;
		sdcbuf.b_actl = dp;
		dp->b_forw = NULL;
	}

	unit = dkunit(bp);
	xunit = minor(bp->b_dev) & 07;
	sz = (bp->b_bcount+511) >> 9;
	sd_st.sd_buf = bp;
	sd_st.sd_bleft = bp->b_bcount;
	sd_st.ucb[unit].blk_type = BLK_HOST;
	pt = &sd_part[unit];

	/* following code figures out the proper ptes to
	   remap into system space so interrupt routine can
	   copy into buf structure. */ 
	v = btop(bp->b_un.b_addr);
	o = (int)bp->b_un.b_addr & PGOFSET;
	npf = btoc(sd_st.sd_bleft + o) ;
	rp = bp->b_flags&B_DIRTY ? &proc[2] : bp->b_proc;
	if ((bp->b_flags & B_PHYS) == 0)
	{
		sd_st.sd_addr = bp->b_un.b_addr;			
	}
	else {
		if (bp->b_flags & B_UAREA)
			pte = &rp->p_addr[v];
		else if (bp->b_flags & B_PAGET)
			pte = &Usrptmap[btokmx((struct pte *)bp->b_un.b_addr)];
		else if ((bp->b_flags & B_SMEM)  &&	/* SHMEM */
					((bp->b_flags & B_DIRTY) == 0))
			pte = ((struct smem *)rp)->sm_ptaddr + v;
		else {
			pte = vtopte(rp, v);
		}


		sd_st.sd_addr = (char *)((int)SD_bufmap + (int)o); 
		mpte = (struct pte *)sdbufmap; 

		for (i = 0; i< npf; i++) {
			if(pte->pg_pfnum == 0)
				panic("sdc: zero pfn in pte");
			*(int *)mpte++ = pte++->pg_pfnum | PG_V | PG_KW;
			mtpr(TBIS, (char *) SD_bufmap + (i*NBPG)); 
		}
		*(int *)mpte = 0;
		mtpr(TBIS, (char *)SD_bufmap + (i * NBPG));
	}


	switch(sd_st.sd_type[unit]) {  
		case DT_RD31:
		case DT_RD32:
		case DT_RD33:
		case DT_RD53:		
		case DT_RD54:
			if(pt->pt_part[xunit].pi_nblocks == -1)
				maxsize = sd_st.ucb[unit].hostsize - pt->pt_part[xunit].pi_blkoff;
			else
				maxsize = pt->pt_part[xunit].pi_nblocks;
			if (bp->b_blkno < 0 ||
			    ( dkblock(bp)+sz) > maxsize){  
				if(sddebug) cprintf("%s%d:%s: Accessing beyond block %d\n",DEV_ID,unit,SOFT_ERR,maxsize);
				sd_st.sd_flags[unit] |= DEV_EOM;
				goto bad;
			}
			sd_st.sd_blkno = bp->b_blkno + sd_st.ucb[unit].lbnbase + pt->pt_part[xunit].pi_blkoff;
			sd_st.sd_drno = unit;
			break;

		case DT_RX50:    
		case DT_RX33:

		    ui = sddinfo[unit];
		    sd_st.sd_drno = unit;

		    /* Check if the diskette type has to be detected. If the 
		     * device is opened the diskette type will have to be
		     * identified again. */

		    if(dsket_type == DT_RX33) {  /* last diskette in was RX33 */
			    if(sdst[DT_RX33].sizes[xunit].nblocks == -1)
				maxsize = sd_st.ucb[unit].hostsize- sdst[DT_RX33].sizes[xunit].blkoffs;
			    else
				maxsize = sdst[DT_RX33].sizes[xunit].nblocks;
			    if (bp->b_blkno < 0 ||
				(dkblock(bp)+sz) > maxsize){  
				    if(sddebug) cprintf("%s%d:%s: Accessing beyond block %d\n",DEV_ID,unit,SOFT_ERR,maxsize);
				    sd_st.sd_flags[unit] |= DEV_EOM;
				    goto bad;
			    }
			    sd_st.sd_blkno = bp->b_blkno + sd_st.ucb[unit].lbnbase + sdst[DT_RX33].sizes[xunit].blkoffs;
			    if(sdstart())
				goto loop;
			    /* return keeping the 16K shared buffer */
			    return(VS_KEEP);


		    } else if(dsket_type == DT_RX50){  /* last diskette in was RX50 */
			    if(sdst[DT_RX50].sizes[xunit].nblocks == -1)
				maxsize = sd_st.ucb[unit].hostsize- sdst[DT_RX50].sizes[xunit].blkoffs;
			    else
				maxsize = sdst[DT_RX50].sizes[xunit].nblocks;
			    if (bp->b_blkno < 0 ||
				(dkblock(bp)+sz) > maxsize){  
				    if(sddebug) cprintf("%s%d:%s: Accessing beyond block %d\n",DEV_ID,unit,SOFT_ERR,maxsize);
				    sd_st.sd_flags[unit] |= DEV_EOM;
				    goto bad;
			    }
			    sd_st.sd_blkno = bp->b_blkno + sd_st.ucb[unit].lbnbase + sdst[DT_RX50].sizes[xunit].blkoffs;
			    if(sdstart())
				goto loop;
			    /* return keeping the 16K shared buffer */
			    return(VS_KEEP);

		    }

		    /* The diskette type is unknown. First SELECT *
		     * with parameters for RX33 (head 1). If the SELECT fails *
		     * do the magic to see if the drive is really ready 
		     * (done in sd_diskette) */

		    if(sddebug) cprintf("sdustart: SELECTING DISKETTE ALL OVER\n");
		    if(sd_diskette()) {
			if(!sd_nodelay)printf("%s%d:%s: Drive select failed\n",DEV_ID,unit,HARD_ERR);
			sd_st.sd_flags[unit] |= DEV_OFFLINE;
			sd_st.sd_cmd = SD_RESET; /* Just to be safe so  that 
						  * interrupt routine  will take care*/
			sd_openerr = 1;
			goto bad;
		    }

		    /* Delay 70 milliseconds  for the data recovery circuit to *
		     * stabilize whenever diskette drive select line is changed *
		     * from not-selected to selected */

		    DELAY(70000); /* should be 70000?? */

		    sd_restore(1);

		    /* SEEK with head 1 to check if the diskette is an RX33. *
		     * RX50 will fail with head 1 */

		    if(sd_seek()) {
			if(sddebug) cprintf("sdustart: assuming RX50 diskette\n");
			/* UDC_RTCNT, UDC_MODE regs and DRIVE SELECT done 
			 * in sd_rdwr() for RX50, since that seems to be the only
			 * way to make it work, for now */

			/* Change the drive parameters if the previous diskette *
			 * type was not RX50 */

			if(dsket_type != DT_RX50) {
			    sd_rx50type(unit);  
			    sd_st.sd_type[unit] = DT_RX50;
			    dsket_type = DT_RX50;
			    ui->ui_type = DT_RX50; 
			}
			if(sdst[DT_RX50].sizes[xunit].nblocks == -1)
			    maxsize = sd_st.ucb[unit].hostsize - sdst[DT_RX50].sizes[xunit].blkoffs;
			else
			    maxsize = sdst[DT_RX50].sizes[xunit].nblocks;
			if (bp->b_blkno < 0 ||
			    (dkblock(bp)+sz) > maxsize){  
				if(sddebug) cprintf("%s%d:%s: Accessing beyond block %d\n",DEV_ID,unit,SOFT_ERR,maxsize);
				sd_st.sd_flags[unit] |= DEV_EOM;
				goto bad;
			}
			sd_st.sd_blkno = bp->b_blkno + sd_st.ucb[unit].lbnbase + sdst[DT_RX50].sizes[xunit].blkoffs;


		    } else {

			/* diskette is RX33 */
			if(sddebug) cprintf("sdustart: RX33 diskette\n");

			/* Change the drive parameters if the previous diskette *
			 * type was not RX33 */

			if(dsket_type != DT_RX33) {
			    sd_rx33type(unit);
			    sd_st.sd_type[unit] = DT_RX33;
			    dsket_type = DT_RX33;
			    ui->ui_type = DT_RX33; 
			}
			if(sdst[DT_RX33].sizes[xunit].nblocks == -1)
			    maxsize = sd_st.ucb[unit].hostsize - sdst[DT_RX33].sizes[xunit].blkoffs;
			else
			    maxsize = sdst[DT_RX33].sizes[xunit].nblocks;
			if (bp->b_blkno < 0 ||
			    (dkblock(bp)+sz) > maxsize){  
				if(sddebug) cprintf("%s%d:%s: Accessing beyond block %d\n",DEV_ID,unit,SOFT_ERR,maxsize);
				sd_st.sd_flags[unit] |= DEV_EOM;
				goto bad;
			}
			sd_st.sd_blkno = bp->b_blkno + sd_st.ucb[unit].lbnbase + sdst[DT_RX33].sizes[xunit].blkoffs;

			/* Delay for 500 milliseconds for the drive speed *
			 * to stabilize at 360 rpm (for RX33) (only for proto)*/

			DELAY(300000); 
		    }
		    break;

		case DT_RX23H:
		case DT_RX23D:

		    ui = sddinfo[unit];
		    sd_st.sd_drno = unit;

		    /* Check if the diskette type has to be detected. If the 
		     * device is opened the diskette type will have to be
		     * identified again. */

		    if( (dsket_type == DT_RX23H) || (dsket_type == DT_RX23D) ) {  /* there is a diskette present */
			    if(sdst[dsket_type].sizes[xunit].nblocks == -1)
				maxsize = sd_st.ucb[unit].hostsize- sdst[dsket_type].sizes[xunit].blkoffs;
			    else
				maxsize = sdst[dsket_type].sizes[xunit].nblocks;
			    if (bp->b_blkno < 0 ||
				(dkblock(bp)+sz) > maxsize){  
				    if(sddebug) cprintf("%s%d:%s: Accessing beyond block %d\n",DEV_ID,unit,SOFT_ERR,maxsize);
				    sd_st.sd_flags[unit] |= DEV_EOM;
				    goto bad;
			    }
			    sd_st.sd_blkno = bp->b_blkno + sd_st.ucb[unit].lbnbase + sdst[dsket_type].sizes[xunit].blkoffs;
			    if(sdstart())
				goto loop;
			    /* return keeping the 16K shared buffer */
			    return(VS_KEEP);


		    } 

		    /* Find if there is a diskette in the RX23 drive */
		    if(sd_diskette()) {
			if(!sd_nodelay) printf("%s%d:%s: Drive select failed\n",DEV_ID,unit,HARD_ERR);
			sd_st.sd_flags[unit] |= DEV_OFFLINE;
			sd_st.sd_cmd = SD_RESET;
			sd_openerr = 1;
			goto bad;
		     }
		     /* Delay for 700 milliseconds for the drive speed *
		      * to stabilize at 300 rpm (for RX23) */
		     timeout(sd_rx23wakeup,(caddr_t)bp,7*(hz/10));
		     return(VS_KEEP);

		     break;


		default:
			printf("%s%d:%s: Unknown Disk type : %d\n",DEV_ID,unit,SOFT_ERR,sd_st.sd_type[unit]);
			goto bad;
	}
	if(sdstart())
		goto loop;
	/* return keeping the 16K shared buffer */
	return(VS_KEEP);

bad:
	bp->b_flags |= B_ERROR;
	if(sd_st.sd_flags[unit] & DEV_EOM)
		bp->b_error = ENOSPC;
	dp = &sdutab[unit];
	dp->b_actf = bp->av_forw;
	iodone(bp);
	goto loop;
}

/* This routine gets called after 700 m.sec timeout for RX23 motor to speed up 
 */

sd_rx23wakeup(bp)
register struct buf *bp;
{
	register int unit,maxsize,sz,xunit;
	struct buf *dp;

	unit = dkunit(bp);
	sz = (bp->b_bcount+511) >> 9;
	xunit = minor(bp->b_dev) & 07;
        dsket_type = rx23_type(); /* find out the diskette type  */
        if(dsket_type == DT_RX23H)
	   sd_rx23htype(unit);
        else
	   sd_rx23dtype(unit);
        if(sdst[dsket_type].sizes[xunit].nblocks == -1)
	    maxsize = sd_st.ucb[unit].hostsize - sdst[dsket_type].sizes[xunit].blkoffs;
        else
	    maxsize = sdst[dsket_type].sizes[xunit].nblocks;
        if (bp->b_blkno < 0 ||
	    (dkblock(bp)+sz) > maxsize){  
		if(sddebug) cprintf("%s%d:%s: Accessing beyond block %d\n",DEV_ID,unit,SOFT_ERR,maxsize);
		sd_st.sd_flags[unit] |= DEV_EOM;
		bp->b_flags |= B_ERROR;
		if(sd_st.sd_flags[unit] & DEV_EOM)
			bp->b_error = ENOSPC;
		dp = &sdutab[unit];
		dp->b_actf = bp->av_forw;
		iodone(bp);
		sdustart();
        }
	else {
		sd_st.sd_blkno = bp->b_blkno + sd_st.ucb[unit].lbnbase + sdst[dsket_type].sizes[xunit].blkoffs;
		if(sdstart())
			sdustart();
	}
	return;
}

/* This routine checks if a diskette is in the drive and ready */

sd_diskette()
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	u_char cmd;

	sdaddr->dkc_cmd = (SD_SETREG | UDC_DHEAD);
	sdc_delay();
	sdaddr->dkc_reg = 1;  /* assume RX33: head 1 valid only for RX33/RX23 */
	sdc_delay();
	sdaddr->dkc_reg = 0; /* UDC_DCYL */
	sdc_delay();
	sdaddr->dkc_cmd = (SD_SETREG | UDC_RTCNT);
	sdc_delay();
	sdaddr->dkc_reg = (RT_CNT | RT_INVRDY |RT_MOTOR  );    /* UDC_RTCNT */
	sdc_delay();
	sdaddr->dkc_reg = (MOD_HD | MOD_CHKCRC | MOD_SRTRXH);   /* UDC_MODE */
	sdc_delay();
	sdaddr->dkc_cmd = (SD_SETREG | UDC_TERM);
	sdc_delay();
	sdaddr->dkc_reg = (TERM_CRC | TERM_INT | TERM_DEL | TERM_WRPR |  TERM_WRFL);
	cmd = (SD_SELECT | DTRT_RX33 | (0377&DRV_NUM2)); 
	rx_reselect = 0;
	if(sd_select(cmd,1)) {
		mprintf("%s%d:%s: Drive Select failed\n",DEV_ID,sd_st.sd_drno,HARD_ERR); 
		return(1);
	}
	if(rx_reselect == 1) {
		
		/* The drive is not shown to be ready. So SELECT the *
		 * drive with INVRDY 0 in UDC_RTCNT. */

		sdaddr->dkc_cmd = (SD_SETREG | UDC_DHEAD);
		sdc_delay();
		sdaddr->dkc_reg = 1;  /* assume RX33: head 1 valid only for RX33 */
		sdc_delay();
		sdaddr->dkc_reg = 0; /* UDC_DCYL */
		sdc_delay();
		sdaddr->dkc_cmd = (SD_SETREG | UDC_RTCNT);
		sdc_delay();
		sdaddr->dkc_reg = (RT_CNT |RT_MOTOR );   /*UDC_RTCNT */
		cmd = (SD_SELECT | DTRT_RX33 | (0377&DRV_NUM2)); 
		if(sd_select(cmd,1)) {
			mprintf("%s%d:%s: Drive Select failed\n",DEV_ID,sd_st.sd_drno,HARD_ERR); 
			return(1);
		}

		/* The previous SELECT succeeded. Issue STEP command. *
		 * The READY bit in UDC_DSTAT should now be 0. */

		if(sd_rxcyl == 0)
			cmd = SD_STEP;
		else
			cmd = (SD_STEP | STEP_OUT);

		if(sd_step(cmd)) {
			return(1);
		}

		/* Since STEP succeeded, there is a diskette in the *
		 * drive. But the drive has to be selected again *
		 * with INVRDY as 1 in UDC_RTCNT !! I GIVE UP!! */

		sdaddr->dkc_cmd = (SD_SETREG | UDC_DHEAD);
		sdc_delay();
		sdaddr->dkc_reg = 1;  /* assume RX33: head 1 valid only for RX33 */
		sdc_delay();
		sdaddr->dkc_reg = 0; /* UDC_DCYL */
		sdc_delay();
		sdaddr->dkc_cmd = (SD_SETREG | UDC_RTCNT);
		sdc_delay();
		sdaddr->dkc_reg = (RT_CNT | RT_INVRDY |RT_MOTOR );   /*UDC_RTCNT */
		sdc_delay();
		sdaddr->dkc_reg = (MOD_HD | MOD_CHKCRC | MOD_SRTRXH);   /* UDC_MODE */
		rx_reselect = 0;
		cmd = (SD_SELECT | DTRT_RX33 | (0377&DRV_NUM2)); 
		if(sd_select(cmd,1)) {
			mprintf("%s%d:%s: Drive Select failed\n",DEV_ID,sd_st.sd_drno,HARD_ERR); 
			return(1);
		}
	}
	return(0);
}

/* This routine returns the RX23 diskette type */

rx23_type()
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	register struct nb_regs *sdiaddr = (struct nb_regs *)nexus;
	register count = 0;
	u_char status,udc_cstat,udc_dstat;
	u_char udc_dhead,udc_dcyl,udc_scnt,udc_dsect,cmd;
	int retry;

	

	/*  Reset and restore the chip and drive, in order to
		ensure that the following density test is reliable */
	if (sd_diskette()) {
		if (sddebug)
        		cprintf ("drive is not ready\n");
		}
		else if (sddebug)
        			cprintf ("drive is ready\n");
	if (sd_restore(1)) {
		if (sddebug) 
			 cprintf ("Restore failed\n");
		}
		else if (sddebug)
        			cprintf ("Restore Success\n");
		

	/* read sector no. 18 to check if high density RX23 */
	udc_dhead = 0;
	udc_dcyl = 0;
	udc_scnt = 1;
	udc_dsect = 18;  /* High density */
	cmd = (SD_RDLOG | RD_XFER);
	for(retry=0; retry< 10; retry++) {
		count = 0;
		sd_poll = 1;
		sdiaddr->nb_int_msk &= ~SINT_DC;
		if(sd_rdwr(udc_dsect,udc_dhead,udc_dcyl,udc_scnt,cmd,0) ) {
			if (sdiaddr->nb_int_reqclr & SINT_DC)
			    sdiaddr->nb_int_reqclr = SINT_DC;
			continue;
		}
		while(!(sdiaddr->nb_int_reqclr & SINT_DC))  {
			DELAY(10);	
			if(++count >= LOOP_DELAY)
				break;
		}
		if(count >= LOOP_DELAY) {
			if (sdiaddr->nb_int_reqclr & SINT_DC)
			    sdiaddr->nb_int_reqclr = SINT_DC;
			continue;
		}
		sdc_delay(); /* to make sure status is correct */
		status = sdaddr->dkc_stat;
		if( (status & DKC_TERMCOD) != DKC_SUCCESS) {
			sdaddr->dkc_cmd = (SD_SETREG | UDC_CSTAT);
			sdc_delay();
			udc_cstat = sdaddr->dkc_reg;  /* read UDC_CSTAT */
			sdc_delay();
			sdaddr->dkc_cmd = (SD_SETREG | UDC_DSTAT);
			sdc_delay();
			udc_dstat = sdaddr->dkc_reg;
			cprintf("rx23_type: status = %o, udc_cstat = %o, udc_dstat = %o\n",status,udc_cstat,udc_dstat);
			if (sdiaddr->nb_int_reqclr & SINT_DC)
			    sdiaddr->nb_int_reqclr = SINT_DC;
			continue;
		}
		break;
	}
	if(count >= LOOP_DELAY) {
		sd_st.sd_cmd = SD_RESET; /* This should take care if the *
					  * cancelled command finishes later */
	}
	if (sdiaddr->nb_int_reqclr & SINT_DC)
	    sdiaddr->nb_int_reqclr = SINT_DC;
	DELAY(1);
	sdiaddr->nb_int_msk |= SINT_DC;
	if(retry >=10) {
		if(udc_cstat & CST_CMPER)
			cprintf("%s%d:%s: compare error\n",DEV_ID,sd_st.sd_drno,HARD_ERR);
		else if(udc_cstat & CST_ECCER)
			cprintf("%s%d:%s: eccerror\n",DEV_ID,sd_st.sd_drno,HARD_ERR);
		else if (udc_cstat & CST_SYNER) {
			cprintf("%s%d:%s: syncerr\n",DEV_ID,sd_st.sd_drno,HARD_ERR);
		}
		cprintf("RX23 Double Density\n");
		return(DT_RX23D);
	}
	/*cprintf("RX23  High Density\n");*/
	return(DT_RX23H);  
}

/* This routine selects the drive (coded in cmd) before doing any I/O */

sd_select(cmd,diskette)
	u_char cmd;
	int diskette;
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	register struct nb_regs *sdiaddr = (struct nb_regs *)nexus;
	register count = 0;
	register fail_count=0;
	u_char status,udc_dstat;

	while(fail_count++ < 2) {
		sdiaddr->nb_int_msk &= ~SINT_DC; 
		count = 0; 
		sdaddr->dkc_cmd = cmd;

		if(diskette){
			DELAY(75); /* for diskette max. of 64 microseconds */
		} else {
			DELAY(30);
		}
		while(!(sdaddr->dkc_stat & DKC_DONE)) { 

			if(++count >= LOOP_DELAY)  
				break; 
		}
		status = sdaddr->dkc_stat;
		if(count >= LOOP_DELAY) {
			sd_deselect();
			DELAY(10);
		}
		else
			break; 
	} 
	if(count >= LOOP_DELAY) {  
		cprintf("sd_select: SELECT not done within count = %d\n",count);
		sd_reset();
		if (sdiaddr->nb_int_reqclr & SINT_DC)
		    sdiaddr->nb_int_reqclr = SINT_DC;
		DELAY(1);
		sdiaddr->nb_int_msk |= SINT_DC;
		return(1);
	} 
	if(diskette) {
		DELAY(70);  
		sdaddr->dkc_cmd = (SD_SETREG | UDC_DSTAT);
		sdc_delay();
		udc_dstat = sdaddr->dkc_reg;  /* read UDC_DSTAT */
		if(sddebug >= 2) cprintf("sd_select: udc_dstat = %o,rx_reselect = %d\n",udc_dstat,rx_reselect);
		if ((udc_dstat & DST_READY) != DST_READY) {
			if(sddebug>=2) cprintf("sd_select: udc_dstat not ready\n");
			if(rx_reselect == 0) {
				rx_reselect = 1;
			}
			else {
				rx_reselect = 0;
				if (sdiaddr->nb_int_reqclr & SINT_DC)
				    sdiaddr->nb_int_reqclr = SINT_DC;
				DELAY(1);
				sdiaddr->nb_int_msk |= SINT_DC;
				return(1);
			}
		}
	}
	else {
		if ((status & DKC_DATERR) == DKC_DATERR)  {
			if(sddebug) cprintf("sd_select: dkc_stat = %o\n",status);
			if (sdiaddr->nb_int_reqclr & SINT_DC)
			    sdiaddr->nb_int_reqclr = SINT_DC;
			DELAY(1);
			sdiaddr->nb_int_msk |= SINT_DC;
			return(1);
		}
	}
	if (sdiaddr->nb_int_reqclr & SINT_DC)
	    sdiaddr->nb_int_reqclr = SINT_DC;
	DELAY(1);
	sdiaddr->nb_int_msk |= SINT_DC;
	return(0);
}

/* This routine deselcts all the drives */

sd_deselect()
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	register struct nb_regs *sdiaddr = (struct nb_regs *)nexus;
	register count = 0;

	sdiaddr->nb_int_msk &= ~SINT_DC; 
	sdaddr->dkc_cmd = SD_DESEL;   
	DELAY(64);
	while(!(sdaddr->dkc_stat & DKC_DONE)) {
		if(++count >= LOOP_DELAY)
			break; 
		;
	}
	if(count >= LOOP_DELAY) {
		if(sddebug) cprintf("sd_deselect: DESELECT not done within count = %d, dkc_stat = %o\n",count,sdaddr->dkc_stat);
		/* Reset the controller to cancel the DESELCT command in case it finishes */
		sd_reset();
		if (sdiaddr->nb_int_reqclr & SINT_DC)
		    sdiaddr->nb_int_reqclr = SINT_DC;
		DELAY(1);
		sdiaddr->nb_int_msk |= SINT_DC;
		return(1);
	} 
	if (sdiaddr->nb_int_reqclr & SINT_DC)
	    sdiaddr->nb_int_reqclr = SINT_DC;
	DELAY(1);
	sdiaddr->nb_int_msk |= SINT_DC;
	return(0);
}

/* This routine issues the RESTORE command to the controller */

sd_restore(diskette)
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	register struct nb_regs *sdiaddr = (struct nb_regs *)nexus;
	register count = 0;
	u_char status;

	sdiaddr->nb_int_msk &= ~SINT_DC; 
	if(diskette)
		sdaddr->dkc_cmd = SD_RESTOR;
	else
		sdaddr->dkc_cmd = (SD_RESTOR | REST_WAIT);
	if(diskette){
		DELAY(64); /* for diskette max. of 64 microseconds */
	} else {
		DELAY(30);
	}
	while(!(sdaddr->dkc_stat & DKC_DONE)) { 
		if(++count >= LOOP_DELAY)
			break; 
	}
	if(count >= LOOP_DELAY) {
		if(sddebug) cprintf("sd_restore: RESTORE not done within count = %d\n",count);
		/* Reset the controller to cancel the RESTORE command in case it finishes */
		sd_reset();
		if (sdiaddr->nb_int_reqclr & SINT_DC)
		    sdiaddr->nb_int_reqclr = SINT_DC;
		DELAY(1);
		sdiaddr->nb_int_msk |= SINT_DC;
		return(1);
	} 
	status = sdaddr->dkc_stat;
	if(diskette) {
		if( (status & DKC_TERMCOD) == DKC_VERERR) {
			if(sddebug) cprintf("sd_restore: Diskette Not restored\n");
			if (sdiaddr->nb_int_reqclr & SINT_DC)
			    sdiaddr->nb_int_reqclr = SINT_DC;
			DELAY(1);
			sdiaddr->nb_int_msk |= SINT_DC;
			return(1);
		}
	}
	if (sdiaddr->nb_int_reqclr & SINT_DC)
	    sdiaddr->nb_int_reqclr = SINT_DC;
	DELAY(1);
	sdiaddr->nb_int_msk |= SINT_DC;
	return(0);
}

sd_step(cmd)
	u_char cmd;
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	register struct nb_regs *sdiaddr = (struct nb_regs *)nexus;
	register count = 0;
	u_char udc_dstat;

	sdiaddr->nb_int_msk &= ~SINT_DC; 
	sdaddr->dkc_cmd = cmd;
	DELAY(64);
	while(!(sdiaddr->nb_int_reqclr & SINT_DC)) {
		DELAY(10);	
		if(++count >= LOOP_DELAY)
			break; 
		;
	}
	if(count >= LOOP_DELAY) {
		if(sddebug) cprintf("sd_step: STEP not done within count = %d\n",count);
		/* Reset the controller to cancel the STEP command in case it finishes */
		sd_reset();
		if (sdiaddr->nb_int_reqclr & SINT_DC)
		    sdiaddr->nb_int_reqclr = SINT_DC;
		DELAY(1);
		sdiaddr->nb_int_msk |= SINT_DC;
		return(1);
	} 
	sdaddr->dkc_cmd = (SD_SETREG | UDC_DSTAT);
	sdc_delay();
	udc_dstat = sdaddr->dkc_reg;  /* read UDC_DSTAT */
	if(sddebug) cprintf("sd_step: udc_dstat = %o\n",udc_dstat);
	if(rx_reselect == 1) {
		rx_reselect = 0;
		if( (udc_dstat & DST_READY) == DST_READY) {
			mprintf("%s%d: Diskette Drive not ready\n",DEV_ID,sd_st.sd_drno);
			if (sdiaddr->nb_int_reqclr & SINT_DC)
			    sdiaddr->nb_int_reqclr = SINT_DC;
			DELAY(1);
			sdiaddr->nb_int_msk |= SINT_DC;
			return(1);
		}
	}
	if (sdiaddr->nb_int_reqclr & SINT_DC)
	    sdiaddr->nb_int_reqclr = SINT_DC;
	DELAY(1);
	sdiaddr->nb_int_msk |= SINT_DC;
	return(0);
}

sd_seek()
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	register struct nb_regs *sdiaddr = (struct nb_regs *)nexus;
	register count = 0;
	u_char status;

	sdiaddr->nb_int_msk &= ~SINT_DC; 
	if(cpu == C_VAXSTAR)
		sdaddr->dkc_cmd = (SD_SEEK | SK_STEP );
	else
		sdaddr->dkc_cmd = (SD_SEEK | SK_STEP | SK_VERFY);
	DELAY(64); 

	/* Cannot RESET the controller to cancel the comand if it 
	 * did not complete within a time frame. */

	while(!(sdiaddr->nb_int_reqclr & SINT_DC)) { 
		DELAY(10);	
		if(++count >= LOOP_DELAY)
			break;
	}
	if(count >= LOOP_DELAY) {
		if(sddebug) cprintf("sd_seek: SEEK not done within count = %d\n",count);
		sd_st.sd_cmd = SD_RESET; /* Since seek cannot be cancelled by a  					  * RESET, even if the seek cmd finishes
					  * later, sdintr will ignore due to this */
		if (sdiaddr->nb_int_reqclr & SINT_DC)
		    sdiaddr->nb_int_reqclr = SINT_DC;
		DELAY(1);
		sdiaddr->nb_int_msk |= SINT_DC;
		return(1);
	}
		
	if(cpu == C_VAXSTAR)  /* GMM */
		DELAY(100);
	status = sdaddr->dkc_stat;
	if ( (status & DKC_TERMCOD) != DKC_SUCCESS) {  
		if(sddebug) cprintf("sd_seek: command failed: status = %o\n",status); 
		if (sdiaddr->nb_int_reqclr & SINT_DC)
		    sdiaddr->nb_int_reqclr = SINT_DC;
		DELAY(1);
		sdiaddr->nb_int_msk |= SINT_DC;
		return(1);
	}
	if (sdiaddr->nb_int_reqclr & SINT_DC)
	    sdiaddr->nb_int_reqclr = SINT_DC;
	DELAY(1);
	sdiaddr->nb_int_msk |= SINT_DC;
	return(0);
} 

/* Resets the controller */

sd_reset()
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	register count= 0;
	sdaddr->dkc_cmd = SD_RESET;
	DELAY(64);
	while(count < DELAYTEN) {
		if((sdaddr->dkc_stat & DKC_DONE) != 0)
			break;
		DELAY(10);
		count++;
	}
	if (count == DELAYTEN){
		cprintf("sd_reset: RESET failed\n");
		sd_st.sd_cmd = SD_RESET; /* This should take care if the *
					  * cancelled command finishes later */
	}
	sdaddr->dkc_cmd = (SD_SETREG | UDC_TERM);
	sdc_delay();
	sdaddr->dkc_reg = (TERM_CRC | TERM_INT | TERM_DEL | TERM_WRPR |  TERM_WRFL);
	return(0);
}

/*
 * Reset the ST506 controller (if present).
 * The VMB boot driver requires this reset to kill off
 * any DMA that might be in progress at the time of a crash.
 * This routine is called from dumpsys() in machdep.c.
 * We wait 1 second for any DMA in progress to run down.
 */

sdreset()
{
	register struct nb1_regs *sdaddr;
	register int i;

	if (mfpr(MAPEN) & 1)	/* memory management must be off, just */
	   return;		/* return if m/m on.		       */

	if ((vs_cfgtst & VS_SC_TYPE) != VS_ST506_SCSI)
	    return;

	sdaddr = (struct nb1_regs *)QMEMVAXSTAR;
	/* delay about 1 second (DO NOT use DELAY() in physical mode */
	for (i = 0; i < 5000000; i++) ;
	sdaddr->dkc_cmd = SD_RESET;
	for (i = 0; i < 5000; i++) ;	/* delay */
}

sdstart()
{
	register struct buf *bp, *dp;
	register cylin;
	register struct pt *pt;
	register struct sdst *st;
	struct uba_device *ui;
	short sn;
	u_char tmphd;
	u_char   udc_dhead,udc_dcyl,udc_scnt,udc_dsect,cmd ;
	int unit,xunit,logical_sn;

	if( (bp = sd_st.sd_buf) == NULL) {
		cprintf("bp is NULL\n");
		return(1);
	}
	unit = dkunit(bp);
	dp = &sdutab[unit];
	xunit = minor(bp->b_dev) & 07;
	pt = &sd_part[unit];
	ui = sddinfo[unit];
	if(sd_st.sd_blkno > (sd_st.ucb[unit].lbnbase + sd_st.ucb[unit].hostsize)) {
		cprintf("%s%d:%s: Wrong block no %d\n",DEV_ID,unit,SOFT_ERR,sd_st.sd_blkno);
		bp->b_flags |= B_ERROR;
		dp->b_actf = bp->av_forw;
		iodone(bp);
		return(1);
	}
	if( (sd_st.sd_blkno - sd_st.ucb[unit].lbnbase) <
		pt->pt_part[xunit].pi_blkoff) { 
	        cprintf("%s%d:%s: Wrong partition: %d,block = %d\n",DEV_ID,unit,SOFT_ERR,xunit,sd_st.sd_blkno);
		bp->b_flags |= B_ERROR;
		dp->b_actf = bp->av_forw;
		iodone(bp);
		return(1);
	} 

	st = &sdst[sd_st.sd_type[unit]];

	sd_st.ucb[unit].blk_type = BLK_HOST;
	cylin = sd_st.sd_blkno/st->nspc ;
	if(cylin >= st->ncyl) {
		printf("%s%d:%s: Invalid cylinder: %d\n",DEV_ID,unit,HARD_ERR,sd_st.sd_cyl.sd_word);
		panic("invalid cylinder");
	}
	if(unit == DRV_NUM2)
		sd_rxcyl = cylin; 
	if(sd_st.sd_type[unit] == DT_RX50) { /* use DEC's ten sector format */
		sn = rx_table[sd_st.sd_blkno % 50];
		if( ++cylin > 79)
			cylin = 0;
		sd_st.sd_hd = 0;   /* only one head */

		sd_st.sd_nsect = st->nsect;
		rx50nsect = (sd_st.sd_bleft + 511) >> 9;  /* required no. of sectors */
		if(rx50nsect > sd_st.sd_nsect)
			if(sd_st.sd_blkno % st->nsect)
				rx50nsect = st->nsect - (sd_st.sd_blkno % st->nsect);
			else
				rx50nsect = sd_st.sd_nsect;
		logical_sn = sd_st.sd_blkno%st->nspc;
		logical_sn %= st->nsect;
		if((logical_sn + rx50nsect) > st->nsect)
			rx50nsect = st->nsect - logical_sn;
		rx50blk = sd_st.sd_blkno;
		if(rx50nsect == 1) {
			sd_st.sd_nsect = 1;
		} else {
			sn = 1;
			if(!(bp->b_flags & B_READ)) {
			/* If to write, first read the full track and replace only the 
			 * sectors to be written into */
				sd_st.sd_cyl.sd_word = cylin;
				tmphd = 0160 & (sd_st.sd_cyl.sd_byte[1] << 4);
				sd_st.sd_cmd = SD_RDLOG;
				cmd =  (SD_RDLOG | RD_XFER); 
				udc_dsect = (0377&sn);  /* UDC_DSECT */ 
				udc_dhead = (tmphd | (0017 & sd_st.sd_hd));  /* UDC_DHEAD */
				udc_dcyl = sd_st.sd_cyl.sd_byte[0];  /* UDC_DCYL */
				udc_scnt = 0377 & sd_st.sd_nsect ;
				if(rx50read(udc_dsect,udc_dhead,udc_dcyl,udc_scnt,cmd,sd_st.ucb[unit].blk_type) ) {
					bp->b_flags |= B_ERROR;
					dp->b_actf = bp->av_forw;
					iodone(bp);
					return(1);
				}
			}
		}
		sd_st.sd_bcount = rx50nsect * SD_SIZE;
		sd_st.sd_blkno += rx50nsect;
	}
	else {
		sd_st.sd_hd = (sd_st.sd_blkno -(cylin*st->nspc))/st->nsect;
		if(sd_st.sd_hd >= st->nheads) {
			printf("%s%d:%s: Invalid head:%d \n",DEV_ID,unit,HARD_ERR,sd_st.sd_hd);
			panic("invalid head");
		}
		sn = sd_st.sd_blkno%st->nspc;
		sn %= st->nsect;
		sd_st.sd_nsect = (sd_st.sd_bleft +511) >> 9;
		if( (sn+sd_st.sd_nsect) > st->nsect) 
			sd_st.sd_nsect = st->nsect - sn;
		if(unit == DRV_NUM2)
			sn++;  /* for diskette, sn starts with 1 */
		sd_st.sd_bcount = sd_st.sd_nsect * SD_SIZE;
		sd_st.sd_blkno += sd_st.sd_nsect;
	}
	start_sn = sn;  

	sd_st.sd_cyl.sd_word = cylin;
	tmphd = 0160 & (sd_st.sd_cyl.sd_byte[1] << 4);

	if (bp->b_flags & B_READ) {
		sd_st.sd_cmd = SD_RDLOG;
		cmd =  (SD_RDLOG | RD_XFER); 
	}
	else {
		sd_st.sd_cmd = SD_WRLOG;
		if(sd_st.sd_type[unit] == DT_RX50)
			cmd = (SD_WRLOG | RX50_PCOMP);
		else if(sd_st.sd_type[unit] == DT_RX33)
			cmd = (SD_WRLOG | RX33_PCOMP);
		else {
			if(cylin > sd_uib[unit].pccyl)
				cmd = (SD_WRLOG | RD31_PCOMP);
			else
				cmd = SD_WRLOG ;
		}
		if((sd_st.sd_type[unit] == DT_RX50) && (rx50nsect > 1) )  
			rx50transfer(sd_st.sd_addr,FILL);
		else
			sdtransfer(sd_st.sd_addr,FILL);
	}
	udc_dsect = (0377&sn);  /* UDC_DSECT */ 
	udc_dhead = (tmphd | (0017 & sd_st.sd_hd));  /* UDC_DHEAD */
	udc_dcyl = sd_st.sd_cyl.sd_byte[0];  /* UDC_DCYL */
	udc_scnt = 0377 & sd_st.sd_nsect ;

#ifdef BBR_TST
	if(bp->b_flags & B_BAD) {
			sd_st.ucb[sd_st.sd_drno].badsect = 0 ; 
			sd_st.ucb[sd_st.sd_drno].badbn = sd_st.sd_blkno - sd_st.sd_nsect + sd_st.ucb[sd_st.sd_drno].badsect - sd_st.ucb[sd_st.sd_drno].lbnbase ; 
			sd_st.ucb[sd_st.sd_drno].blk_type = BLK_BAL;
			if(sdbbrdbug) cprintf("B_BAD: in sdstart: badsect = %d,badbn = %d,sd_blkno = %d, sd_nsect = %d\n",sn,sd_st.ucb[sd_st.sd_drno].badbn,sd_st.sd_blkno,sd_st.sd_nsect);
			if(put_rbn(sd_st.ucb[sd_st.sd_drno].badbn)) {
				cprintf("B_BAD: put_rbn failed\n");
			} 
			bbr_sleep = 0;
			dp->b_actf = bp->av_forw;
			vd->vsd_action = ST_WANTBACK;
			if ((cpu == C_VAXSTAR) && cvs_exmode_on)
				sdustart();
			else
			        vs_bufctl(vd); /*sdustart() gets called from vs_bufctl*/
			return(0);
	}
#endif BBR_TST

	if(sd_rdwr(udc_dsect,udc_dhead,udc_dcyl,udc_scnt,cmd,sd_st.ucb[unit].blk_type)) {
	        if(cpu == C_VAXSTAR) cprintf("sdstart: sd_rdwr failed\n"); /* GMM */
		if(sddebug) cprintf("sdstart: sd_rdwr failed\n");
		bp->b_flags |= B_ERROR;
		dp->b_actf = bp->av_forw;
		iodone(bp);
		return(1);
	}
	if(ui->ui_dk >= 0) {
		dk_busy |= 1<<ui->ui_dk;
		dk_xfer[ui->ui_dk]++;
		dk_wds[ui->ui_dk] += sd_st.sd_bcount>>6;
	}
	return(0);
}

/* This routine does the actual read/write. All the registers are updated here
   again just to be safe. (Sometimes causes syncerror otherwise) */

sd_rdwr(udc_dsect,udc_dhead,udc_dcyl,udc_scnt,cmd,flag)
	u_char udc_dsect,udc_dhead,udc_dcyl,udc_scnt,cmd;
	short flag;
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	register struct nb_regs *sdiaddr = (struct nb_regs *)nexus;
	u_char sel_cmd; 

	sdaddr->dkc_cmd = (SD_SETREG | UDC_DMA7);
	sdc_delay();
	if(flag) {  /* not host area block */
		sdaddr->dkc_reg = rbn_addr1;	/* UDC_DMA7 */
		sdc_delay();
		sdaddr->dkc_reg = rbn_addr2;  /*UDC_DMA15 */
	} else {
		sdaddr->dkc_reg = 0;	/* UDC_DMA7 */
		sdc_delay();
		sdaddr->dkc_reg = 0;   /* UDC_DMA15 */
	}
	sdc_delay();
	sdaddr->dkc_reg = 0;   /* UDC_DMA23 */
	sdc_delay();
	sdaddr->dkc_reg = udc_dsect;  /* UDC_DSECT */
	sdc_delay();
	sdaddr->dkc_reg = udc_dhead ;  /* UDC_DHEAD */
	sdc_delay();
	sdaddr->dkc_reg = udc_dcyl;  /* UDC_DCYL */
	sdc_delay();
	sdaddr->dkc_reg = udc_scnt;
	sdc_delay();

	if(sd_st.sd_drno == DRV_NUM2) {
		if ((sd_st.sd_type[sd_st.sd_drno] == DT_RX50) || (sd_st.sd_type[sd_st.sd_drno] == DT_RX23D) )  { 
			sdaddr->dkc_reg = (RT_CNT | RT_INVRDY |RT_MOTOR | RT_LOSPD);   /* UDC_RTCNT */
			sdc_delay();
			sdaddr->dkc_reg = (MOD_HD | MOD_CHKCRC | MOD_SRTRXL);  /* UDC_MODE */
			sel_cmd = (SD_SELECT | DTRT_RX50 | DRV_NUM2); 
		} else if( (sd_st.sd_type[sd_st.sd_drno] == DT_RX33) || (sd_st.sd_type[sd_st.sd_drno] == DT_RX23H) ) { 
			sdaddr->dkc_reg = (RT_CNT | RT_INVRDY | RT_MOTOR); /* UDC_RTCNT */
			sdc_delay();
			sdaddr->dkc_reg = (MOD_HD | MOD_CHKCRC | MOD_SRTRXH);  /* UDC_MODE */
			sel_cmd = (SD_SELECT | DTRT_RX33 | DRV_NUM2); 
		}
		else {
			printf("sd_rdwr: Wrong type in Drive 2: %d\n",sd_st.sd_type[sd_st.sd_drno]);
			return(1);
		}
		sdaddr->dkc_cmd = (SD_SETREG | UDC_TERM);
		sdc_delay();
		sdaddr->dkc_reg = (TERM_CRC | TERM_INT | TERM_DEL | TERM_WRPR |  TERM_WRFL);
		rx_reselect = 0;
		if(sd_select(sel_cmd,1)) {
			printf("%s%d:%s: Drive select failed\n",DEV_ID,sd_st.sd_drno,HARD_ERR);
			if(sd_poll) {
				sd_poll = 0;
				sdiaddr->nb_int_msk &= ~SINT_DC;
			}
			return(1);
		}
		if(sd_poll) {
			sd_poll = 0;
			sdiaddr->nb_int_msk &= ~SINT_DC;
		}
	}
	else {
		sdaddr->dkc_reg = RT_CNT;  /* UDC_RTCNT */
		sdc_delay();
		sdaddr->dkc_reg = (MOD_HD | MOD_CHKECC | MOD_SRTRDN);  /* UDC_MODE */
		sdc_delay();
		sdaddr->dkc_cmd = (SD_SETREG | UDC_TERM);
		sdc_delay();
		sdaddr->dkc_reg = (TERM_CRC | TERM_INT | TERM_DEL | TERM_WRPR |  TERM_WRFL);
		sel_cmd = (SD_SELECT | DTRT_HDSK | sd_st.sd_drno);
		if(sd_select(sel_cmd,0)) {
			printf("%s%d:%s: Drive select failed\n",DEV_ID,sd_st.sd_drno,HARD_ERR);
			sd_st.sd_flags[sd_st.sd_drno] |= DEV_OFFLINE;
			if(sd_poll) {
				sd_poll = 0;
				sdiaddr->nb_int_msk &= ~SINT_DC;
			}
			return(1);
		}
		if(sd_poll) {
			sd_poll = 0;
			sdiaddr->nb_int_msk &= ~SINT_DC;
		}
	}
	/*if( (cpu == C_VAXSTAR) && (sd_st.sd_drno == DRV_NUM2) ) {
		if(sd_seek()) {
			cprintf("RX23 seek failed\n");
			return(1); 
		}
		DELAY(18000);
	}    */
	sdaddr->dkc_cmd = (SD_SETREG | UDC_DATA);
	sdc_delay();
	DELAY(100); 
	sdaddr->dkc_cmd = cmd;
	return(0);
}

rx50read(udc_dsect,udc_dhead,udc_dcyl,udc_scnt,cmd,type) 
u_char udc_dsect,udc_dhead,udc_dcyl,udc_scnt,cmd;
short type;
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	register struct nb_regs *sdiaddr = (struct nb_regs *)nexus;
	register count = 0;
	u_char status,udc_cstat,udc_dstat;
	int retry;
	for(retry=0; retry< 10; retry++) {
		count = 0;
		sd_poll = 1;
		sdiaddr->nb_int_msk &= ~SINT_DC;
		if(sd_rdwr(udc_dsect,udc_dhead,udc_dcyl,udc_scnt,cmd,type) ) {
			if (sdiaddr->nb_int_reqclr & SINT_DC)
			    sdiaddr->nb_int_reqclr = SINT_DC;
			continue;
		}
		while(!(sdiaddr->nb_int_reqclr & SINT_DC))  {
			DELAY(10);	
			if(++count >= LOOP_DELAY)
				break;
		}
		if(count >= LOOP_DELAY) {
			if (sdiaddr->nb_int_reqclr & SINT_DC)
			    sdiaddr->nb_int_reqclr = SINT_DC;
			continue;
		}
		sdc_delay(); /* to make sure status is correct */
		status = sdaddr->dkc_stat;
		if( (status & DKC_TERMCOD) != DKC_SUCCESS) {
			sdaddr->dkc_cmd = (SD_SETREG | UDC_CSTAT);
			sdc_delay();
			udc_cstat = sdaddr->dkc_reg;  /* read UDC_CSTAT */
			sdc_delay();
			sdaddr->dkc_cmd = (SD_SETREG | UDC_DSTAT);
			sdc_delay();
			udc_dstat = sdaddr->dkc_reg;
			if(sddebug) cprintf("rx50read: status = %o, udc_cstat = %o, udc_dstat = %o\n",status,udc_cstat,udc_dstat);
			if (sdiaddr->nb_int_reqclr & SINT_DC)
			    sdiaddr->nb_int_reqclr = SINT_DC;
			continue;
		}
		break;
	}
	if(count >= LOOP_DELAY) {
		sd_st.sd_cmd = SD_RESET; /* This should take care if the *
					  * cancelled command finishes later */
	}
	if (sdiaddr->nb_int_reqclr & SINT_DC)
	    sdiaddr->nb_int_reqclr = SINT_DC;
	DELAY(1);
	sdiaddr->nb_int_msk |= SINT_DC;
	if(retry >=10) {
		if(udc_cstat & CST_CMPER)
			printf("%s%d:%s: compare error\n",DEV_ID,sd_st.sd_drno,HARD_ERR);
		else if(udc_cstat & CST_ECCER)
			printf("%s%d:%s: eccerror\n",DEV_ID,sd_st.sd_drno,HARD_ERR);
		else if (udc_cstat & CST_SYNER) {
			printf("%s%d:%s: syncerr\n",DEV_ID,sd_st.sd_drno,HARD_ERR);
		}
		return(1);
	}
	return(0);
}

int	sd_stray = 0;	/* TODO1: debug */
sdintr(star)
	int star;
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	register struct buf *bp ,*dp;
	register err = 0;
	register struct uba_device *ui;
	register struct vsdev *vd;
	int xunit;
	u_char status,udc_cstat,udc_dstat;

#ifdef lint
	star = star;
#endif

	vd = &vsdiskdev;
	sdc_delay();
	status = sdaddr->dkc_stat;
	if( ((status & DKC_INTPEND) != DKC_INTPEND) ||
	    ((status & DKC_DONE) != DKC_DONE) ) {
/* TODO1: debug
		mprintf("%s:%s: stray interrupt \n",DEV_ID,SOFT_ERR);
*/
		sd_stray++;
		return;
	}
	
	if(!xbnflag) {
		if( (bp = sd_st.sd_buf) == NULL) {
			mprintf("%s:%s: No valid buffer\n",DEV_ID,SOFT_ERR); 
			if(sddebug >=2) cprintf("sdintr:No valid buffer: cmd = %o, dkc_stat = %o\n",sd_st.sd_cmd,status);
			return;
		}
		dp = &sdutab[dkunit(bp)];
		xunit = minor(bp->b_dev) & 07;
	}

	switch(sd_st.sd_cmd) {
		
	    case SD_STEP:
	    case SD_DESEL:
	    case SD_SELECT:
	    case SD_SEEK:
	    case SD_RESTOR:
	    case SD_FMT:
	    case SD_RDTR:
		mprintf("%s%d:%s: Command not yet implemented thru interrupt, command = %c\n",DEV_ID,sd_st.sd_drno,SOFT_ERR,sd_st.sd_cmd);
		return;
		/* break; */

	    case SD_RDPHY:
	    case SD_RDLOG:
	    case SD_WRPHY:
	    case SD_WRLOG:
		if(!xbnflag) {
		    ui = sddinfo[dkunit(bp)];
		    if(ui->ui_dk >= 0) 
			    dk_busy &= ~(1 << ui->ui_dk);
		}
		sdaddr->dkc_cmd = (SD_SETREG | UDC_CSTAT);
		sdc_delay();
		udc_cstat = sdaddr->dkc_reg;  /* read UDC_CSTAT */
		sdc_delay();
		sdaddr->dkc_cmd = (SD_SETREG | UDC_DSTAT);
		sdc_delay();
		udc_dstat = sdaddr->dkc_reg;
		if( (status & DKC_TERMCOD) != DKC_SUCCESS) {
		    if( (sddebug) && (rtr_cnt == 0)) {
			cprintf("sdintr: udc_cstat = %o ",udc_cstat);
			cprintf("sdintr: drive = %d, block = %d, head = %o,cyln = %o,bleft = %d,bcount = %d\n",sd_st.sd_drno,sd_st.sd_blkno,sd_st.sd_hd,sd_st.sd_cyl.sd_word,sd_st.sd_bleft,sd_st.sd_bcount);
			sdaddr->dkc_cmd = (SD_SETREG | UDC_DMA7);
			cprintf("sdintr:DMA7 = %o ",sdaddr->dkc_reg); /* DMA7*/
			cprintf("sdintr:DMA15 = %o ",sdaddr->dkc_reg); /* DMA15 */
			cprintf("sdintr:DMA23 = %o\n",sdaddr->dkc_reg); /* DMA23 */
			sd_st.ucb[sd_st.sd_drno].badsect = 0; 
			cprintf("sdintr:DSECT = %o ",sdaddr->dkc_reg); /* DSECT */
			cprintf("sdintr:CHEAD = %o ",sdaddr->dkc_reg); /* CHEAD */
			cprintf("sdintr:CCYL = %o ",sdaddr->dkc_reg); /* CCYL */
			sdaddr->dkc_cmd = (SD_SETREG | UDC_DSTAT);
			cprintf("sdintr:DSTAT = %o\n",sdaddr->dkc_reg); /* DSTAT */
		    } else {
			sdc_delay();
			sdaddr->dkc_cmd = (SD_SETREG | UDC_DSECT);
			sd_st.ucb[sd_st.sd_drno].badsect = 0; 
		    }
		    if ( (udc_cstat & CST_ECCER)  ||
			 (udc_cstat & CST_CMPER) ){
			    if(rtr_cnt == 0)sd_st.sd_softcnt[sd_st.sd_drno]++;
			    if(!xbnflag)
				if(++rtr_cnt < RTRY_CNT){
				    if(sd_st.sd_type[sd_st.sd_drno] == DT_RX50)
					    sd_st.sd_blkno -= rx50nsect;
				    else
					    sd_st.sd_blkno -= sd_st.sd_nsect;
				    if(!sdstart())
					return;
				} 
			    rtr_cnt = 0;

			    if( (sd_st.sd_drno == DRV_NUM2) || xbnflag ) 
				 err++;
			    else if(sd_retry(0)) 
				err++;

			    if(err) {
				if(ddm_err) {
				    printf("%s%d%c: hard error sn %d\n",DEV_ID,sd_st.sd_drno,'a'+xunit,BAD_SECT(sd_st.sd_drno,xunit));
				    printf("%s%d: Forced Error Modifier set LBN %d\n",DEV_ID,sd_st.sd_drno,BAD_LBN(sd_st.sd_drno));
				    ddm_err = 0;
				    sd_st.sd_bleft -= (sd_st.ucb[sd_st.sd_drno].badsect + 1 ) * SD_SIZE;
				}
				else if(udc_cstat & CST_CMPER)
					printf("%s%d:%s: compare error\n",DEV_ID,sd_st.sd_drno,HARD_ERR);
				    else printf("%s%d:%s: eccerror\n",DEV_ID,sd_st.sd_drno,HARD_ERR);
				    if(sdbbrdbug) {
					cprintf("%s%d: block = %d, head = %o, cylinder = %o\n",DEV_ID,sd_st.sd_drno,sd_st.sd_blkno-sd_st.sd_nsect,sd_st.sd_hd,sd_st.sd_cyl.sd_word);
					sdaddr->dkc_cmd = (SD_SETREG | UDC_DSECT);
					cprintf("sdintr:DSECT = %o ",sdaddr->dkc_reg); /* DSECT */
					cprintf("sdintr:CHEAD = %o ",sdaddr->dkc_reg); /* CHEAD */
					cprintf("sdintr:CCYL = %o ",sdaddr->dkc_reg); /* CCYL */
					cprintf("sdintr:CSTAT = %o ",udc_cstat);
					sdaddr->dkc_cmd = (SD_SETREG | UDC_DSTAT);
					cprintf("sdintr:DSTAT = %o\n",sdaddr->dkc_reg); /* DSTAT */
				    }
			    }
		    }
		    else if ((udc_cstat & CST_SYNER) ) {
			if(rtr_cnt == 0)sd_st.sd_softcnt[sd_st.sd_drno]++;
			if(!xbnflag)
			    if(++rtr_cnt < RTRY_CNT){
				if(sd_st.sd_type[sd_st.sd_drno] == DT_RX50) {
				    sd_st.sd_blkno -= rx50nsect;
				}
				else
				    sd_st.sd_blkno -= sd_st.sd_nsect;
				if(!sdstart())
				    return;
			    } 
			    rtr_cnt = 0;
			    printf("%s%d:%s: syncerr\n",DEV_ID,sd_st.sd_drno,HARD_ERR);
			    err++;
		    }
		    else if ( (status & DKC_OVRUN) == DKC_OVRUN) {
			printf("%s%d:%s: overrun/underrun\n",DEV_ID,sd_st.sd_drno,HARD_ERR);
			err++;
			sd_reset();
		    }
		    else if( (udc_dstat & DST_WRFAULT) == DST_WRFAULT) {
			if(rtr_cnt == 0)sd_st.sd_softcnt[sd_st.sd_drno]++;
			if(!xbnflag)
			    if(++rtr_cnt < RTRY_CNT){
				if(!sd_deselect()) {
				    DELAY(100);
				    /* Select the drive and start all over again */
			            if(sd_st.sd_type[sd_st.sd_drno] == DT_RX50)
				       sd_st.sd_blkno -= rx50nsect;
			            else
				       sd_st.sd_blkno -= sd_st.sd_nsect;
			            if(!sdstart())
				       return;
				}
			    } 
			printf("%s%d:%s: WRITE FAULT\n",DEV_ID,sd_st.sd_drno,HARD_ERR);
			rtr_cnt = 0;
			err++;
		    }
		    else if ( (udc_cstat & CST_DELDT) == CST_DELDT) {
			DELAY(10);
			sd_reset();
		        if(rtr_cnt == 0)sd_st.sd_softcnt[sd_st.sd_drno]++;
		        if(!xbnflag)
			     if(++rtr_cnt < RTRY_CNT){
			         if(sd_st.sd_type[sd_st.sd_drno] == DT_RX50)
				       sd_st.sd_blkno -= rx50nsect;
			         else
				       sd_st.sd_blkno -= sd_st.sd_nsect;
			         if(!sdstart())
				       return;
			     } 
		        rtr_cnt = 0;

		        if( (sd_st.sd_drno == DRV_NUM2) || xbnflag ) 
				 err++;
		        else if(sd_retry(1)) {

			        if(ddm_err) {
				        printf("%s%d%c: hard error sn %d\n",DEV_ID,sd_st.sd_drno,'a'+xunit,BAD_SECT(sd_st.sd_drno,xunit));
					printf("%s%d: Forced Error Modifier set LBN %d\n",DEV_ID,sd_st.sd_drno,BAD_LBN(sd_st.sd_drno));
					ddm_err = 0;
					sd_st.sd_bleft -= (sd_st.ucb[sd_st.sd_drno].badsect + 1 ) * SD_SIZE; 
			        }
			        else 
					printf("%s%d:%s:\n",DEV_ID,sd_st.sd_drno,HARD_ERR);
				err++;
			}
		    }
		    else if ( (status & DKC_BADSECT) == DKC_BADSECT) {
			if( (sd_st.sd_drno == DRV_NUM2) || xbnflag ) {
			     printf("%s%d:%s: bad sector\n",DEV_ID,sd_st.sd_drno,HARD_ERR);
			     err++;
			}
			else if(sd_retry(0)) 
			    err++;
		    }
		    else if( (udc_dstat & DST_WRPROT) == DST_WRPROT) {
			cprintf("%s%d: WRITE PROTECTED\n",DEV_ID,sd_st.sd_drno);
			sd_st.sd_flags[sd_st.sd_drno] |= DEV_WRTLCK;
			err++;
		    }
		    else {
			mprintf("%s%d:%s: Unknown error type, UDC_CSTAT = %o, UDC_DSTAT = %o,DKC_STAT = %o\n",DEV_ID,sd_st.sd_drno,SOFT_ERR,udc_cstat,udc_dstat,status);
			err++;
		    }
		}
		rtr_cnt = 0;
		if(sd_st.sd_drno == DRV_NUM2) {  
		     if( (udc_dstat & DST_WRPROT) == DST_WRPROT) {
			sd_st.sd_flags[sd_st.sd_drno] |= DEV_WRTLCK;
		     }
		} 
		if(err) {
		    if(!xbnflag) {
			bp->b_flags |= B_ERROR;
			dp->b_actf = bp->av_forw;
			bp->b_resid = sd_st.sd_bleft;
			iodone(bp);
			sd_st.sd_status |= (CNT_DONE | CNT_ERR);
			/*sdustart() gets called from vs_bufctl*/
			vd->vsd_action = VS_WANTBACK;
			if ((cpu == C_VAXSTAR) && cvs_exmode_on)
				sdustart();
			else
			        vs_bufctl(vd);
		    }
		    sd_st.sd_hardcnt[sd_st.sd_drno]++;
		    sd_st.sd_status |= (CNT_DONE | CNT_ERR);
		    return;
		}
		break;

	    default:
		mprintf("%s%d:%s: wrong command: %o\n",DEV_ID,sd_st.sd_drno,SOFT_ERR,sd_st.sd_cmd);
		return;  

	}

	if(xbnflag) {
		sdtransfer(xbnbuf,EMPTY);
		sd_st.sd_status &= ~CNT_ERR;
		sd_st.sd_status |= CNT_DONE;
		return;
	}


	if(bp->b_flags & B_READ)
		if((sd_st.sd_type[sd_st.sd_drno] == DT_RX50) && (rx50nsect > 1) )
			rx50transfer(sd_st.sd_addr,EMPTY);
		else
			sdtransfer(sd_st.sd_addr,EMPTY);

	if(sd_st.sd_bleft > sd_st.sd_bcount)
		sd_st.sd_bleft -= sd_st.sd_bcount;
	else
		sd_st.sd_bleft = 0;

	sd_st.sd_addr += sd_st.sd_bcount;
	if(sd_st.sd_bleft > 0) {

		if(!(bp->b_flags & B_READ)) {
			/* Diskette drive requires some time to complete tunnel 
			 * erasure of data just written */
			if(sd_st.sd_type[sd_st.sd_drno] == DT_RX50)
				DELAY(700);  
			if(sd_st.sd_type[sd_st.sd_drno] == DT_RX33)
				DELAY(400); 
		}

		if(sdstart()) {
			/*sdustart() gets called from vs_bufctl*/
			vd->vsd_action = VS_WANTBACK;
			if ((cpu == C_VAXSTAR) && cvs_exmode_on)
				sdustart();
			else
				vs_bufctl(vd);
		}
	}
	else {
		dp->b_actf = bp->av_forw;
		bp->b_resid = sd_st.sd_bleft;
		iodone(bp);
		sd_st.sd_status |= CNT_DONE;
		sd_st.sd_status &= ~(CNT_BSY | CNT_ERR);
		vd->vsd_action = VS_WANTBACK;
		if ((cpu == C_VAXSTAR) && cvs_exmode_on)
			sdustart();
		else
		        vs_bufctl(vd); /*sdustart() gets called from vs_bufctl*/
	}
	return;

}




sdwait()
{

	register count = 0;
	while(!(sd_st.sd_status & CNT_DONE ))   /* wait for status change */
		if(++count >= LOOP_DELAY)
			return(1);
	sd_st.sd_status &= ~CNT_DONE;
	return(0);
}

/* This routine transfers the data from/to the controller's buffer to/from the
 * user buffer ( for RDs and RX33 diskette) */

sdtransfer(bpp,op)
	char *bpp;
	short op;
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	register short  nbytes;
	register char   *buf0, *buf1;

	buf0  = bpp;

	if (sd_st.sd_bleft >= sd_st.sd_bcount)
		nbytes =  sd_st.sd_bcount;
	else
		nbytes=sd_st.sd_bleft;

	if ((cpu == C_VAXSTAR) && cvs_exmode_on)
		buf1 = (char *)cvseddbmem;
	else
		buf1 = (char *)sdaddr->nb_ddb;
	if( op == FILL)
		 bcopy (buf0, buf1, nbytes);
	else
		 bcopy (buf1, buf0, nbytes);

}

/* This routine transfers the data from/to the controller's buffer to/from the
 * user buffer ( for RX50 diskette) */

rx50transfer(bpp,op)
	char *bpp;
	short op;
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	register char *buf0, *buf1;
	register i,sector;
	int last,nbytes;

	buf0 = bpp;
	if(sd_st.sd_bleft >= sd_st.sd_bcount)
		last = sd_st.sd_bcount % SD_SIZE;
	else
		last = sd_st.sd_bleft % SD_SIZE;
	nbytes = SD_SIZE;
	for(i=0; i< rx50nsect; i++) {
		if( i== (rx50nsect-1)) 
			if (last) {
				nbytes = last;
			}
		sector = rx_table[rx50blk % 50];
		if ((cpu == C_VAXSTAR) && cvs_exmode_on)
		    buf1 = (char *)cvseddbmem + SD_SIZE*(sector - 1);
		else
		    buf1 = (char *)sdaddr->nb_ddb + SD_SIZE*(sector - 1);
		if(op == FILL)
			 bcopy (buf0, buf1, nbytes);
		else
			 bcopy (buf1, buf0, nbytes);
		buf0 += nbytes;
		rx50blk++;
	}
	return;
}

/* This routine reads the XBNs (first 3 blocks) to get all format information */

sd_rdfmt(unit)
	register int unit;
{
	register sector,i;
	register struct sd_uib *uibptr;
	register char *char_ptr, *buf_ptr;
	short *data_ptr;
	u_char sect,head,cylin,scnt,cmd;
	short sd_sum;

	sd_st.sd_hd = sd_st.sd_cyl.sd_word  = 0;
	head = cylin = 0;
	scnt =1;
	cmd =  (SD_RDLOG | RD_XFER); 
	sd_st.sd_drno = unit;
	uibptr = &sd_uib[unit];
	for( sector = 0; sector < 3; sector++ ) {
		xbnflag = 1;
		sect = sector & 0377;
		sd_st.sd_cmd = SD_RDLOG;
		sd_st.sd_bleft = sd_st.sd_bcount = SD_SIZE;
		sd_st.sd_status = 0;
		if(sd_rdwr(sect,head,cylin,scnt,cmd,0))
			continue;
		if(sdwait())
			continue;;
		xbnflag = 0;
		if(sd_st.sd_status & CNT_ERR) {
			sd_st.sd_status &= ~CNT_ERR;
			cprintf("sd_rdfmt: reading XBN %d failed\n",sector);
			continue;
		}
		for( i = 0; i < 9; i++ )
			if( xbnbuf[i] != 0x00 ){
				cprintf("sd_rdfmt:bytes 0-8 not zero in xbn %d\n",sector);
				goto repeat;
			}
		if( xbnbuf[9] != 0x36 ) {
			cprintf("sd_rdfmt: byte 9 not 0x36 in xbn %d\n",sector);
			continue;
		}
		sd_sum = 0;
		data_ptr = (short *)&xbnbuf[0];
		for( i = 255; --i >= 0; )
			sd_sum += *data_ptr++;
		if( *data_ptr == sd_sum ) {
			char_ptr = (char *)uibptr;
			buf_ptr = &xbnbuf[0];
			for(i=0; i<UIBSIZE; i++) {
				/* This is done because the filler is only 10 bytes long *
				   and the next element is int which should be word *
				   aligned */

				*char_ptr++ = *buf_ptr++;
			}
			sd_st.ucb[unit].xbn_un.xbn_short[0] = sd_uib[unit].xbnsize[0];
			sd_st.ucb[unit].xbn_un.xbn_short[1] = sd_uib[unit].xbnsize[1];
			sd_st.ucb[unit].dbn_un.dbn_short[0] = sd_uib[unit].dbnsize[0];
			sd_st.ucb[unit].dbn_un.dbn_short[1] = sd_uib[unit].dbnsize[1];
			sd_st.ucb[unit].lbn_un.lbn_short[0] = sd_uib[unit].lbnsize[0];
			sd_st.ucb[unit].lbn_un.lbn_short[1] = sd_uib[unit].lbnsize[1];
			sd_st.ucb[unit].rbn_un.rbn_short[0] = sd_uib[unit].rbnsize[0];
			sd_st.ucb[unit].rbn_un.rbn_short[1] = sd_uib[unit].rbnsize[1];
			sd_st.ucb[unit].med_un.med_short[0] = sd_uib[unit].media[0];
			sd_st.ucb[unit].med_un.med_short[1] = sd_uib[unit].media[1];
			sd_st.ucb[unit].vol_un.vol_short[0] = sd_uib[unit].volume[0];
			sd_st.ucb[unit].vol_un.vol_short[1] = sd_uib[unit].volume[1];
			if(sddebug) cprintf("sd_rdfmt:xbnsize=%d, dnsize=%d,lbnsize=%d,rbnsize=%d, sec=%d\n",sd_st.ucb[unit].xbn_un.xbnsize,sd_st.ucb[unit].dbn_un.dbnsize,sd_st.ucb[unit].lbn_un.lbnsize,sd_st.ucb[unit].rbn_un.rbnsize,sd_uib[unit].sec);
			if(sddebug) cprintf("sur=%d, cyl=%d, pccyl=%d, rccyl=%d, seekrate=%d,crc_or_ecc=%d\n",sd_uib[unit].sur,sd_uib[unit].cyl,sd_uib[unit].pccyl,sd_uib[unit].rccyl,sd_uib[unit].seekrate,sd_uib[unit].crc_or_ecc);
			if(sddebug) cprintf("rctsize=%d, rctcopies=%d, media=%x,sec_interleave=%d,sur_skew=%d,cyl_skew=%d, volume=%d\n",sd_uib[unit].rctsize,sd_uib[unit].rctcopies,sd_st.ucb[unit].med_un.media,sd_uib[unit].sec_interleave,sd_uib[unit].sur_skew,sd_uib[unit].cyl_skew,sd_st.ucb[unit].vol_un.volume);

			sd_st.ucb[unit].lbnbase = sd_st.ucb[unit].xbn_un.xbnsize + sd_st.ucb[unit].dbn_un.dbnsize;
			sd_st.ucb[unit].rbnbase = sd_st.ucb[unit].lbnbase + sd_st.ucb[unit].lbn_un.lbnsize;
			sd_st.ucb[unit].hostsize = sd_st.ucb[unit].lbn_un.lbnsize - (sd_uib[unit].rctsize * sd_uib[unit].rctcopies);

			switch(sd_st.ucb[unit].med_un.media) {

				case MED_RD31:
					cprintf("Drive %d is RD31\n",unit);
					sd_st.sd_type[unit] = DT_RD31;
					break; 

				case MED_RD32:
					cprintf("Drive %d is RD32\n",unit);
					sd_st.sd_type[unit] = DT_RD32;
					break; 

				case MED_RD33:
					cprintf("Drive %d is RD33\n",unit);
					sd_st.sd_type[unit] = DT_RD33;
					break; 

				case MED_RD53:
					cprintf("Drive %d is RD53\n",unit);
					sd_st.sd_type[unit] = DT_RD53;
					break;

				case MED_RD54:
					cprintf("Drive %d is RD54\n",unit);
					sd_st.sd_type[unit] = DT_RD54;
					break; 

				default:
					cprintf("Unknown Disk type %x in Drive %d\n",sd_st.ucb[unit].med_un.media,unit);
					return(1);

			} 

			/* Are the users allowed to change nsect,nspce etc ?? */

			if(sd_uib[unit].sec != sdst[sd_st.sd_type[unit]].nsect) {
				cprintf("WARNING: no. of sectors per track differ from standard %d to formatted %d\n",sdst[sd_st.sd_type[unit]].nsect,sd_uib[unit].sec);
				if(sd_uib[unit].sec < sdst[sd_st.sd_type[unit]].nsect) {
					cprintf("WARNING: no. of sectors per track changed from %d to %d\n",sdst[sd_st.sd_type[unit]].nsect,sd_uib[unit].sec);
					sdst[sd_st.sd_type[unit]].nsect = sd_uib[unit].sec;
					sdst[sd_st.sd_type[unit]].nspc = sd_uib[unit].sec * sdst[sd_st.sd_type[unit]].nheads;
				}
				else
					cprintf("USING STANDARD VALUES\n");
			}

			if(sd_uib[unit].cyl != sdst[sd_st.sd_type[unit]].ncyl) {
				cprintf("WARNING: no. of cylinders differ from standard %d to formatted %d\n",sdst[sd_st.sd_type[unit]].ncyl,sd_uib[unit].cyl);
				if(sd_uib[unit].cyl < sdst[sd_st.sd_type[unit]].ncyl) {
					cprintf("WARNING: no. of cylinders changed from %d to %d\n",sdst[sd_st.sd_type[unit]].ncyl,sd_uib[unit].cyl);
					sdst[sd_st.sd_type[unit]].ncyl = sd_uib[unit].cyl;
				}
				else
					cprintf("USING STANDARD VALUES\n");
			}

			if(sd_uib[unit].sur != sdst[sd_st.sd_type[unit]].nheads) {
				cprintf("WARNING: Number of surfaces differ between standard and format values\n");
				cprintf("standard: %d, format value %d\n",sdst[sd_st.sd_type[unit]].nheads,sd_uib[unit].sur);
				cprintf("USING STANDARD VALUE\n");
			}

			if(sddebug) cprintf("lbnbase = %d, rbnbase = %d, hostsize = %d\n",sd_st.ucb[unit].lbnbase,sd_st.ucb[unit].rbnbase,sd_st.ucb[unit].hostsize);

			return( 0 );
		}
		else {
			cprintf("sd_rdfmt: checksum error\n");
		}
		xbn_sum = sd_sum;
		xbn_check = *data_ptr;
repeat:
		;
	}
	xbnflag = 0;
	return( 1 );
}

/* Initialize the common data for floppies */

sd_rxtype(unit)
	register int unit;
{
	
	sd_st.ucb[unit].xbn_un.xbnsize = 0; 
	sd_st.ucb[unit].dbn_un.dbnsize = 0; 
	sd_st.ucb[unit].rbn_un.rbnsize = 0; 
	sd_st.ucb[unit].lbnbase = 0;
	sd_st.ucb[unit].rbnbase = 0;
	return;
}

/* Fill RX50 information */

sd_rx50type(unit)
	register int unit;
{

	sd_st.ucb[unit].lbn_un.lbnsize = 800; 
	sd_st.ucb[unit].med_un.media = 0x25658021L;  
	sd_st.ucb[unit].hostsize = 800;
	return;
}

/* Fill RX33 information */

sd_rx33type(unit)
	register int unit;
{

	sd_st.ucb[unit].lbn_un.lbnsize = 2400; 
	sd_st.ucb[unit].med_un.media = 0x25658021L;  
	sd_st.ucb[unit].hostsize = 2400;
	return;
}

/* Fill RX23 information */

sd_rx23htype(unit)
	register int unit;
{

	sd_st.ucb[unit].lbn_un.lbnsize = 2880; 
	sd_st.ucb[unit].med_un.media = 0x25658021L;  
	sd_st.ucb[unit].hostsize = 2880;
	return;
}

sd_rx23dtype(unit)
	register int unit;
{

	sd_st.ucb[unit].lbn_un.lbnsize = 1440; 
	sd_st.ucb[unit].med_un.media = 0x25658021L;  
	sd_st.ucb[unit].hostsize = 1440;
	return;
}

/* This routine is called if any block is found to be bad. The vrfy_flag decides
 * if the block is to be replaced after finding the real bad sector. The bad
 * sector no. in UDC_DSECT is not very reliable at the time of an error. So
 * all the sectors are read/written once again one at a time to catch the real 
 * bad sector. */

sd_retry(vrfy_flag)
int vrfy_flag;     /* flag to decide if block to be replaced */
{
	register i,block,frag_count;
	char *buf0;
	u_char cmd;

	if(sdbbrdbug >= 2) cprintf("SD_RETRY\n");
	if( (sd_st.sd_cmd == SD_RDLOG) || (sd_st.sd_cmd == SD_RDPHY) )
		cmd = (sd_st.sd_cmd | RD_XFER);
	else
		cmd = sd_st.sd_cmd;
	for(i=0; i<sd_st.sd_nsect; i++) {
		sd_st.ucb[sd_st.sd_drno].blk_type = BLK_BAL;
		buf0 = sd_st.sd_addr + (i * SD_SIZE); 
		block = sd_st.sd_blkno - sd_st.sd_nsect + i ; 
		/* if the last sector to be transferred should be less than 512*/
		if( (i == (sd_st.sd_nsect -1)) && (sd_st.sd_bleft <= sd_st.sd_bcount) ) 
			frag_count = sd_st.sd_bleft % SD_SIZE;
		else
			frag_count = 0;
		if(sdbbrdbug >=3) cprintf("sd_retry: block = %d, i=%d,start_sn=%d\n",block,i,start_sn);
		if( rdwr_poll(block,buf0,1,i,cmd,sd_bbrcount,frag_count) ) { 
			sd_st.ucb[sd_st.sd_drno].badsect = i ;  /* the logical 
					* no. of the bad sector within the 
					* sectors being transferred */
			if(sdbbrdbug) cprintf("sd_retry:I rdwr_poll failed:  block = %d, i=%d,start_sn=%d\n",block,i,start_sn);
			if(vrfy_flag)
				if(ddm_err)
					return(1);
			if(sd_rpl(frag_count))
				return(1);
			else
				continue;
		}
	}
	return(0);
}

/* This routine finds the bad block number, gets the replacement block, replaces
 * the bad block and does I/O on the replaced block */

sd_rpl(frag_count)
register frag_count;
{
	register rpl_blk;
	register  error;
	u_char cmd;
	char *buf0;


	if(sdbbrdbug >=2)cprintf("SD_RPL\n");
	if( (sd_st.ucb[sd_st.sd_drno].blk_type == BLK_HOST) ||
	    (sd_st.ucb[sd_st.sd_drno].blk_type == BLK_BAL)) { /* first time for a block */
		sd_st.ucb[sd_st.sd_drno].badbn = sd_st.sd_blkno - sd_st.sd_nsect + sd_st.ucb[sd_st.sd_drno].badsect - sd_st.ucb[sd_st.sd_drno].lbnbase ; 

		if(sdbbrdbug) cprintf("sd_rpl: sd_blkno = %d, sd_nsect = %d,badsect = %d,badlbn = %d\n",sd_st.sd_blkno,sd_st.sd_nsect,sd_st.ucb[sd_st.sd_drno].badsect,sd_st.ucb[sd_st.sd_drno].badbn);
		if(sdbbrdbug >= 2) cprintf("sd_rpl: blk_type = %d,badsect = %d\n",sd_st.ucb[sd_st.sd_drno].blk_type,sd_st.ucb[sd_st.sd_drno].badsect);

		if( get_rbn(sd_st.ucb[sd_st.sd_drno].badbn))
			return(1);
		if( sd_st.ucb[sd_st.sd_drno].oldrbn < 0) {
			/* no current RBN, so make one */

			if(sdbbrdbug >=3) cprintf("sd_rpl: calling put_rbn\n");
			if(put_rbn(sd_st.ucb[sd_st.sd_drno].badbn)) {
				cprintf("sd_rpl: put_rbn failed\n");
				return(1);
			}
			if( sd_st.ucb[sd_st.sd_drno].oldrbn < 0) {
				if(sdbbrdbug) cprintf("sd_rpl: oldrbn <0 after put_rbn\n");
				sd_st.ucb[sd_st.sd_drno].oldrbn = (sd_st.sd_blkno -sd_st.sd_nsect + sd_st.ucb[sd_st.sd_drno].badsect)  - sd_st.ucb[sd_st.sd_drno].rbnbase;   
			}
		}
	} else if (sd_st.ucb[sd_st.sd_drno].blk_type == BLK_RBN) {  /* RBN to be replaced */
			if(sdbbrdbug) cprintf("sd_rpl: blk_type = %d, sd_badbn = %d,badlbn = %d\n",sd_st.ucb[sd_st.sd_drno].blk_type,sd_st.sd_blkno,sd_st.ucb[sd_st.sd_drno].badsect);
			if(sdbbrdbug) cprintf("sd_rpl:calling put_rbn for BLK_RBN\n");
			if(put_rbn(sd_st.ucb[sd_st.sd_drno].badbn)) {
				cprintf("sd_rpl: put_rbn failed\n");
				return(1);
			}
			if( sd_st.ucb[sd_st.sd_drno].oldrbn < 0) {
				if(sdbbrdbug) cprintf("sd_rpl: oldrbn <0 after put_rbn\n");
				sd_st.ucb[sd_st.sd_drno].oldrbn = (sd_st.sd_blkno -sd_st.sd_nsect + sd_st.ucb[sd_st.sd_drno].badsect)  - sd_st.ucb[sd_st.sd_drno].rbnbase;    
			}
	}
	else  {
		cprintf("sd_rpl: UNKNOWN BLK_TYPE: %d\n",sd_st.ucb[sd_st.sd_drno].blk_type);
		return(1);  
	}

	/* now do the read/write from the RBN, rather than the LBN */

	rpl_blk = sd_st.ucb[sd_st.sd_drno].oldrbn + sd_st.ucb[sd_st.sd_drno].rbnbase ;
	if( (sd_st.sd_cmd == SD_RDLOG) || (sd_st.sd_cmd == SD_RDPHY) )
		cmd = (sd_st.sd_cmd | RD_XFER);
	else
		cmd = sd_st.sd_cmd;
	if(sdbbrdbug) cprintf("sd_rpl: rpl_blk = %d\n",rpl_blk);
	sd_st.ucb[sd_st.sd_drno].blk_type = BLK_RBN;
	buf0 = sd_st.sd_addr + (sd_st.ucb[sd_st.sd_drno].badsect * SD_SIZE);
	if( (error = rdwr_poll(rpl_blk,buf0,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,frag_count)) ) {
		if(sdbbrdbug) cprintf("sd_rpl: I rdwr_poll failed: error= %d\n",error);
		if ( (!ddm_err) && (error == BBR_RETRY) ) {
			if(sd_rpl(frag_count))
				return(1);
			else
				return(0);
		}
		else {
			return(1);
		}
	}

	if(sdbbrdbug >=2) cprintf("sd_rpl: returning success\n");
	return(0);
}


/*  Most of the routines related to bad block replacement (get_rbn,put_rbn etc)
 *  have been taken from the RQDX3 microcode with required adaptations.
 */

/*
 *  this routine will locate the RBN which corresponds with the given LBN
 *
 *  To do this, we must read the RCT, so of course if the "ignore media format"
 *  modifier was set when the unit was brought online, just pack up and go.
 *  Otherwise, we map the LBN to its primary RBN:  we compute how many LBNs map
 *  to each RBN (total number of LBNs divided by total number of RBNs), and
 *  divide the given LBN by this number.  This gives us the RBN which would be
 *  allocated to this LBN if the RBN were free.  Since the RBN can be in use
 *  (replacing another LBN which maps to the same RBN), the "primary" RBN may
 *  not be the right one.  We could just do a linear search from here, looking
 *  at higher-numbered RBNs, but that would waste some of the space in the
 *  current RCT block.  So, instead, we look both up and down, ping-ponging
 *  around the primary RBN, until we run out of gas totally and are forced to
 *  go to the next RCT block.  In that block, and in all others, we do a linear
 *  search, and when we reach the end of the RCT, we wrap back around to the
 *  front.  The search continues until we find the LBN we want, or we find an
 *  unused RBN.  If we find the LBN we want, we are golden.  If we find instead
 *  an unused RBN, it means that the LBN cannot be found (i.e., it has not been
 *  replaced yet and this is a new occurrence of it being bad).
 *
 *  At this point, an example may help.  Suppose that a unit has 50000 LBNs
 *  and 160 RBNs.  We want to find the RBN associated with LBN 3000.  We
 *  calculate that there are 50000/160=312 LBNs for each RBN.  This means that
 *  LBN 3000 maps into RBN 3000/312=9.  Thus RBN 9 is the "primary" RBN for LBN
 *  3000.  We read the RCT block which contains RBN 9 (there are 128 RBNs per
 *  RCT block, so we read RCT block 2, taking the two block header into
 *  account).  We look at offset 9, to see if it contains LBN 3000.  If it
 *  does, we stop, and return RBN 9 as our answer.  If it doesn't, but contains
 *  some other LBN instead, we keep looking, since we don't stop unless we
 *  match or we find an unused RBN.  Thus we look at entry 10, then 8, 11, 7,
 *  12, 6, 13, 5, 14, 4, 15, 3, 16, 2, 17, 1, 18, 0, 19, 20, 21, 22, ..., in
 *  order, until either we match our LBN or find an RBN which is unused.
 *  Notice the peculiar order; the numbers ping-pong back and forth between
 *  higher RBNs and lower RBNs, until a limit is reached (0 in this case) when
 *  the scan continues in the other direction only.  If we don't find the LBN
 *  we want in all of RCT block 2, and none of the entries indicate that that
 *  RBN is unused, we then go to RCT block 3, and look at entries 0, 1, 2, up
 *  to 127, and continue in this fashion until all possibilities have been
 *  exhausted.  Got it?
 *
 *  To simplify matters, this routine always returns both the entry which
 *  corresponds with the given LBN (i.e., the "old" RBN), and the first unused
 *  entry (i.e., the "new" RBN).
 */
get_rbn( lbn )
	int lbn;
{
	register int *rct2ptr;
	register i, j, k, rctblock, rctoffset ;
	int lbn_rct;
	int rbn;

	/*
	*  be pessimistic and assume failure
	*/
	sd_st.ucb[sd_st.sd_drno].oldrbn = -1;
	sd_st.ucb[sd_st.sd_drno].newrbn = -1;
	/*
	*  get what the entry we want should look like (high and low parts)
	*/

	lbn_rct = lbn | 0x30000000;
	/*
	*  compute the "primary" RBN of this LBN, and where to find it
	*/
	rbn = lbn * sd_st.ucb[sd_st.sd_drno].rbn_un.rbnsize / sd_st.ucb[sd_st.sd_drno].lbn_un.lbnsize;
	rctblock = rbn / 128 + 2;
	rctoffset = ( rbn & 127 )* sizeof(int);
	if(sdbbrdbug >=2) cprintf( "GET_RBN: lbn = %d,lbn_rct = %x, rbn = %d,rctblock = %d, rctoffset = %d\n",
	    lbn,lbn_rct, rbn, rctblock, rctoffset );
	/*
	*  read the RCT block which contains the "primary" RBN's entry
	*/
	if( rd_rct(rctblock, xbnbuf ) ) { 
		cprintf("get_rbn: returninng: rd_rct failed\n");
		return( -1 );
	}
	/*
	*  point to the "primary" entry first
	*/
	rct2ptr = (int *)&xbnbuf[rctoffset];
	if(sdbbrdbug >=3) cprintf( "rct2ptr = %o\n",rct2ptr);
	/*
	*  if the "primary" entry is unused, we are done (there was no "old" RBN)
	*/
	if(sdbbrdbug >= 2) cprintf("get_rbn: rct entry in rctoffset = %x\n",*rct2ptr);
	if( (*rct2ptr & 0xFFFF0000) == 0x00000000 ) {
		sd_st.ucb[sd_st.sd_drno].newrbn = rbn;
		if(sdbbrdbug) cprintf("get_rbn: returning:  Primary entry unused,newrbn = %d\n",sd_st.ucb[sd_st.sd_drno].newrbn);
		return( 0 );
	} 
		
		
	
	/*
	*  if the "primary" entry is the one we want, we have found the "old" RBN
	*  and now want to look for a "new" RBN
	*/
	if( lbn_rct == *rct2ptr) {
		if(sdbbrdbug) cprintf("get_rbn: Wanted one is the primary entry: rbn = %d\n",rbn);
		sd_st.ucb[sd_st.sd_drno].oldrbn = rbn ;
	}
	/*
	*  begin the ping-pong search
	*/
	for( i = 4; i < SD_SIZE; i+=4  ) {
		j = rctoffset + i;
	/*
	 *  don't check any entry past the limit
	 */
		if( j <= (SD_SIZE - 4) ) {
			rct2ptr = (int *)&xbnbuf[j];
		    if(sdbbrdbug >=3) cprintf( "rct2ptr = %o\n",rct2ptr);
		    /*
		     *  if this entry is unused, we are done
		     */
			if( (*rct2ptr & 0xFFFF0000) == 0x00000000 ) {
				/*sd_st.ucb[sd_st.sd_drno].newrbn = rbn + i / 2;*/
				sd_st.ucb[sd_st.sd_drno].newrbn = rbn + i/4 ;
				if(sdbbrdbug) cprintf("get_rbn: returning: found an unused entry in ping-pong I, newrbn = %d \n",sd_st.ucb[sd_st.sd_drno].newrbn);
				return( 0 );
			}
	    /*
	     *  if we hit the end of the RCT, wrap around to the beginning
	     */
			if( (*rct2ptr & 0xFFFF0000) == 0x80000000 ) {
				k = 2;
				goto RCT_LOOP;
			}
	    /*
	     *  if we find what we want, remember it
	     */
			if( lbn_rct == *rct2ptr) {
				sd_st.ucb[sd_st.sd_drno].oldrbn = rbn + i/4 ;
				if(sdbbrdbug) cprintf("get_rbn: Found oldrbn = %d, rct_entry = %o\n",sd_st.ucb[sd_st.sd_drno].oldrbn,*rct2ptr);
			}
		}
		j = rctoffset - i;
	/*
	 *  don't check any entry past the limit
	 */
		if( j >= 0 ) {
			rct2ptr = (int *)&xbnbuf[j];
	    if(sdbbrdbug >=3) cprintf( "rct2ptr = %o\n",rct2ptr);
	    /*
	     *  if this entry is unused, we are done
	     */
			if( (*rct2ptr & 0xFFFF0000) == 0x00000000 ) {
				sd_st.ucb[sd_st.sd_drno].newrbn = rbn - i/4 ;
				if(sdbbrdbug) cprintf("get_rbn: returning: found an unused entry in ping-pong II, newrbn = %d ,rbn=%d, i=%d\n",sd_st.ucb[sd_st.sd_drno].newrbn,rbn,i);
				return( 0 );
			}
	    /*
	     *  if we hit the end of the RCT, wrap around to the beginning
	     */
			if( (*rct2ptr & 0xFFFF0000) == 0x80000000 ) {
				k = 2;
				goto RCT_LOOP;
			}
	    /*
	     *  if we find what we want, remember it
	     */
			if( lbn_rct == *rct2ptr) {
				sd_st.ucb[sd_st.sd_drno].oldrbn = rbn - i/4 ;
				if(sdbbrdbug) cprintf("get_rbn: Found oldrbn = %d, rct_entry = %o\n",sd_st.ucb[sd_st.sd_drno].oldrbn,*rct2ptr);
			}
		}
	}
	/*
	*  we aren't done yet, but have exhausted the first block we looked at;
	*  try the next block, and continue until it feels right
	*/
	k = rctblock + 1;
RCT_LOOP:
	/*
	*  there are two ways to get out of this loop:  either find an unused
	*  RBN, or else discover that finding an unused RBN is impossible
	*/
	for( i = k; ; i++ ) {
	/*
	 *  read the next RCT block
	 */
		if( rd_rct( i, xbnbuf ) ) {
			cprintf("get_rct: returning:  rd_rct failed in linear search\n");
			return( -1 );
		}
		else {
	    /*
	     *  don't ping-pong, but go straight through this block
	     */
			for( j = 0; j < SD_SIZE; j +=4 ) {
		/*
		 *  if we return to the original block and offset, then we
		 *  must conclude that the RCT is full
		 */
				if( ( i == rctblock ) && ( j == rctoffset ) ) {
					cprintf("get_rbn: returning: RCT FULL\n");
					return( -1 );
				}
				rct2ptr = (int *)&xbnbuf[j];
	    if(sdbbrdbug >=3) cprintf( "rct2ptr = %o\n",rct2ptr);
		/*
		 *  if this entry is unused, we are done
		 */
				if( (*rct2ptr & 0xFFFF0000) == 0x00000000 ) {
					sd_st.ucb[sd_st.sd_drno].newrbn = ( i - 2 ) * 128 + j/4 ;
					if(sdbbrdbug) cprintf("get_rbn: returning: found newrbn in linear search, newrbn =%d\n",sd_st.ucb[sd_st.sd_drno].newrbn);
					return( 0 );
				}
		/*
		 *  if we hit the end of the RCT, wrap around to the beginning
		 */
				if( (*rct2ptr & 0xFFFF0000) == 0x80000000 ) {
					k = 2;
					goto RCT_LOOP;
				}
		/*
		 *  if we find what we want, remember it
		 */
				if( lbn_rct == *rct2ptr) {
					sd_st.ucb[sd_st.sd_drno].oldrbn = ( i - 2 ) * 128 + j/4 ;
					if(sdbbrdbug) cprintf("get_rbn: found oldrbn = %d\n",sd_st.ucb[sd_st.sd_drno].oldrbn);
				}
			}
	    }
	}
}



/*
 *  this routine implements the RCT multi-read algorithm
 *
 *  The algorithm consists of reading each of the RCT copies in turn, using a
 *  compare operation.  If the compare fails, the next copy is tried, until
 *  either we succeed or we run out of copies.
 */
rd_rct(i, buffer )
	register int i;
	char *buffer;
{
	register int *data_ptr, *temp_ptr;
	register blkno;
	register int j, k;
	u_char cmd;

    /*
     *  do this until we succeed or until there are no more copies
     */
    /* set up arguments for reading the first copy */

	if(sdbbrdbug >= 3) cprintf("RD_RCT\n");
	cmd = (SD_RDLOG | RD_XFER);
	sd_st.ucb[sd_st.sd_drno].blk_type = BLK_RCTGET;
	blkno = sd_st.ucb[sd_st.sd_drno].lbnbase + sd_st.ucb[sd_st.sd_drno].hostsize + i;
	if(sdbbrdbug >= 2) cprintf( "rd_rct: reading RCT block %d,physical block: %d\n", i,blkno );
	for( j = sd_uib[sd_st.sd_drno].rctcopies; --j >= 0; ) {
		if( !rdwr_poll(blkno,buffer,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0)) {

	    /* set up parameters for reading rct into tmpbuf */
			if( !rdwr_poll(blkno,tmpbuf,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0)) {
		/*
		 *  we have read twice in a row now with no errors; make sure
		 *  the same data was read both times by comparing them
		 */
				data_ptr = (int *)buffer;
				temp_ptr = (int *)&tmpbuf[0];
				for( k = 128; --k >= 0; )
					if( *data_ptr++ != *temp_ptr++ )
						break;
				if( k < 0 )
					return( 0 );
			}
		}
	/*
	 *  this copy is not good, try the next one
	 */
		blkno += sd_uib[sd_st.sd_drno].rctsize;
		if(sdbbrdbug) cprintf("rd_rct: Reading next RCT copy: %d\n",j);
	}
    printf("rd_rct: RCT read failed\n"); 
    return( -1 );
}

/*
 *  this routine will replace a bad LBN with a good RBN
 */
put_rbn( lbn)
	int lbn;
{
	register int *temp_ptr, *rct0_ptr,*rct2_ptr,*rct3_ptr,*inv_ptr,i;
	int j, ddm, error, oldrctblock,  newrctblock, *data_ptr;
	int oldrbn, newrbn,block,bbr_recurs,tst_fail;
	u_char cmd;
	char invbuf[SD_SIZE];

	if(sdbbrdbug >= 2) cprintf("PUT_RBN\n");
	rct0_ptr = (int *)rct0;
	rct2_ptr = (int *)rct2;
	rct3_ptr = (int *)rct3;
	ddm = 0;

	/*
	*  STEP 1 -- 
	*    If we are not replacing a valid bad block, then we must be bringing
	*    the volume online, so go to step 3.
	*
	*	ECO# 13
	*	BBR recursion counter is initialized to 0. This counter is
	*	important for step 9 because it prevents infinite loops in the
	*	replacement procedures, which would cause the supply of RBNs
	*	to be exhausted.
	*/
	if(sdbbrdbug >= 2) cprintf("PUT_RBN: STEP1\n");
	oldrbn = -1;
	newrbn = -1;

	bbr_recurs = 0;  /* intitialize bbr recursion counter to 0. ECO# 13 */
	if( lbn >= 0 )
		goto STEP_4;
	/*
	*  STEP 3 -- If we are recovering from a failure or loss of context that
	*    occurred during phase 1 of bad block replacement, then go to step 7.
	*    If we are recovering from a failure or loss of context that occurred
	*    during phase 2 of bad block replacement, then go to step 11.
	*    Otherwise, exit with success.  If any errors occur, we cannot
	*    proceed at all.
	*/
	if(sdbbrdbug >= 2) cprintf("PUT_RBN: STEP 2\n");
	if( error = rd_rct(0, rct0 ) )
		goto STEP_18A;


	if( error = wr_rct(0, rct0 ) )
		goto STEP_18A;
	rd_rct(1, rct1 );
	wr_rct(1, rct1 );
	ddm = *(rct0_ptr + 4*16) & bit7; /* bit 7: Forced error flag */
	lbn = *(rct0_ptr + 6*16);
	newrbn = *(rct0_ptr + 8*16);
	oldrbn = *(rct0_ptr + 10*16);
	sd_st.ucb[sd_st.sd_drno].newrbn = newrbn;
	sd_st.ucb[sd_st.sd_drno].oldrbn = oldrbn;
	if( *(rct0_ptr + 4*16) & bit15)  /* bit 15: Phase 1 flag */
		if( error = rd_rct( 1, rct1 ) )
			goto STEP_18A;
		else
			goto STEP_7;
	if( *(rct0_ptr + 4*16) & bit14)  /* bit 14: Phase 2 flag */
		if( error = rd_rct(1, rct1 ) )
			goto STEP_18A;
		else
			goto STEP_11;
	goto STEP_14;
	/*
	*  	ECO# 13/20
	*  STEP 4 -- A sector sized buffer is cleared. Then read the current
	*    contents of the bad block into the buffer. 
	*	The buffer is cleared first for the rare case when no data can
	*	be transferred .
	*
	*	Then up to 4 reads are performed with error recovery and error
	*	correction enabled to attempt to recover the data. The read is
	*	successful if no errors are detected or only a forced error is
	*	detected.  In addition to saving the data, the process 
	*	remembers whether or not the read succeeded. This information
	*	will be used in Step 6 in determining whether to set or clear
	*	the FE flag in RCT block 0.
	*
	*	If a forced error is encountered, remember the fact for Step 6.
	*/
STEP_4:
	if(sdbbrdbug) cprintf("PUT_RBN: STEP 4\n");
	printf( "%s%d: Bad Block Reported at LBN %d \n", DEV_ID,sd_st.sd_drno,lbn);
	bzero(rct1,SD_SIZE);
	block = lbn + sd_st.ucb[sd_st.sd_drno].lbnbase;
	cmd = (SD_RDLOG | RD_XFER);
	for( i = 0; i< 4; i++) {
		if(error = rdwr_poll(block,rct1,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0))
			continue;
		else
			break;
	}

#ifdef BBR_TST
	if(sdbbrdbug) cprintf("error = %d\n",error);
#endif BBR_TST

	if(error) 
		ddm = 1;
	/*
	*  STEP 5 -- Record the data obtained when the bad block was read during
	*    step 4 in sector 1 of each RCT copy.  If the data cannot be
	*    successfully recorded in at least one copy of the RCT, report the
	*    error to the error log and go to step 18.
	*/
	if(sdbbrdbug) cprintf("PUT_RBN: STEP 5\n");
	if( error = wr_rct( 1, rct1 ) )
		goto STEP_18A;
	/*
	*  STEP 6 -- Record the bad block's LBN, whether or not the saved data is
	*    valid, and the fact that we are now in phase 1 of replacement in
	*    sector 0 of each RCT copy.  This means that we must read sector 0,
	*    modify it, and write it back to each RCT copy.  If we cannot read
	*    any sector 0 successfully, we go to step 18.  If we cannot
	*    successfully write at least one copy of the RCT, we go to step 17.
	*/
	if(sdbbrdbug) cprintf("PUT_RBN: STEP 6\n");
	if( error = rd_rct( 0, rct0 ) )
		goto STEP_17;
	*(rct0_ptr + 4*16) &= ~( bit15|bit14|bit13|bit7 );
	*(rct0_ptr + 4*16) |= bit15;
	if( ddm   )
		*(rct0_ptr + 4*16) |= bit7;
	*(rct0_ptr + 6*16) = lbn;
	if( error = wr_rct( 0, rct0 ) )
		goto STEP_18A;
	/*
	*  	ECO# 13/20
	*  STEP 7 -- The suspected bad block is tested with error recovery 
	*    disabled. If any write or read operation fails during any of the 
	*    following tests, remember that fact for later steps and exit at 
	*    step e below. The test procedure includes the following:
	*
	*   a. Read the bad block 4 times, unless an error occurs, to determine
	*      if the original contents contain an error. If an error is 
	*      detected (not forced error)  go to step e.
	*
	*   b. Write the saved data followed by 4 reads to stress test the block
	*      in question. If an error is detected during the write or read
	*      go to step e.
	*
	*   c. Write the inversion (1's complement) of the saved data. Then
	*      read the block 4 times to stress test it. The inversion of the
	*      saved data will be written with the Forced Error modifier to
	*      protect the integrity of the block. 
	*
	*   d. Repeat steps b and c 8 times, or until an error occurs. 
	*
	*   e. Go to the next step to put the saved data back in the bad block.
	*      This will ensure that disk integrity can survive power failures
	*      and unexpected drive errors which may occur in subsequent steps.
	*/
STEP_7:
	block = lbn + sd_st.ucb[sd_st.sd_drno].lbnbase;
	if(sdbbrdbug) cprintf("PUT_RBN: STEP 7, block =%d\n",block);
/* STEP_7A: */
	tst_fail = 1;  /* Assume failure to start with*/
	cmd = (SD_RDLOG | RD_XFER);
	for( i = 0; i< 4; i++) {
	    if(error = rdwr_poll(block,tmpbuf,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,1,0))
		if(!ddm_err)
		    goto STEP_8;
	}
/* STEP_7B: */
	temp_ptr = (int *)rct1;
	inv_ptr = (int *)invbuf;
	bzero(invbuf,SD_SIZE);
	for( i = 0; i < 128; i++)
	    *inv_ptr++ = ~(*temp_ptr++);
	for(j=0; j<8; j++) {
	    cmd = SD_WRLOG;
	    if( error = rdwr_poll(block,rct1,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,1,0))
		goto STEP_8;
	    cmd = (SD_RDLOG | RD_XFER);
	    for( i = 0; i< 4; i++) {
		if(error = rdwr_poll(block,tmpbuf,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,1,0))
		    if(!ddm_err)
			goto STEP_8;
	    }
/* STEP_7C: */
	    cmd = (SD_WRLOG | WR_DDMRK);
	    if( error = rdwr_poll(block,invbuf,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,1,0))
		goto STEP_8;
	    cmd = (SD_RDLOG | RD_XFER);
	    for( i = 0; i< 4; i++) {
		if(error = rdwr_poll(block,tmpbuf,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,1,0)) {
		    if(!ddm_err)
			goto STEP_8;
		} else
		    goto STEP_8;
	    }
	}
	tst_fail = 0;  /* The tests succeeded */

	/*
	*	ECO # 20
	*  STEP 8 -- We write the saved data back out to the bad block using a
	*    write operation. The write is performed with the Forced Error
	*    modifier if the saved data is invalid (ie. if the FE bit is set in
	*    the memory-resident copy of the RCT block 0). 
	*
	*    If the write succeeds, it is followed by a read addressed to the
	*    bad block's LBN. The read succeeds if the returned status code is
	*    success or if ddm_err is set AND the write was performed with the
	*    Forced Error modifer. If the read succeeds, then the data is 
	*    compared with the data written. The operation succeeds if the data
	*    is the same. 
	*
	*    If any write,read, or compare operation failed in Steps 7 or 8, go
	*    to the next step, else go to step 13.
	*/

STEP_8:
	if(sdbbrdbug) cprintf("PUT_RBN: STEP 8\n");

#ifdef BBR_TST
	cprintf("ddm = %d\n",ddm);
#endif BBR_TST

	if(ddm)
		cmd = (SD_WRLOG | WR_DDMRK);
	else
		cmd = SD_WRLOG;
	block = lbn + sd_st.ucb[sd_st.sd_drno].lbnbase;
	if( error = rdwr_poll(block,rct1,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0))
		goto STEP_9;
	if(tst_fail)  /* if any read/write failed in Step 7 */
		goto STEP_9;
	cmd = (SD_RDLOG | RD_XFER);
	if( error = rdwr_poll(block,tmpbuf,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0))
		if(!(ddm_err && ddm))
			goto STEP_9;
	ddm_err = 0;
	data_ptr = (int *)rct1;
	temp_ptr = (int *)tmpbuf;
	for( i = 128; --i >= 0; )
		if( *data_ptr++ != *temp_ptr++ )
			goto STEP_9;

	printf( "%s%d: Transient error on LBN %d, Block not bad \n", DEV_ID,sd_st.sd_drno,lbn);

#ifdef BBR_TST
	if(bbr_force) {
		printf("%s%d: LBN %d being forced to be replaced\n",DEV_ID,sd_st.sd_drno,lbn);
		goto STEP_9;
	}
#endif BBR_TST
	
	goto STEP_13; 

	/*
	*	ECO# 13
	*	The BBR recursion count is checked to see if this step has
	*	been executed twice indicating that replacement blocks were
	*	found bad during the write and compare operation in Step 12C
	*	and/or read operation in Step 12A.
	*
	*	  if the BBR recursion count is 2 then go to step 16.
	*
	*	  if the BBR recursion count is less than 2 then increment the
	*	  count.
	*  STEP 9 -- We scan the RCT and determine what new RBN the bad block
	*    should be replaced with, whether or not the bad block has been
	*    previously replaced, and (if appropriate) the bad block's old RBN.
	*    The RCT is not updated at this time.  If the RCT scan fails, we
	*    report the error to the error log and go to step 16.
	*/
STEP_9:
	if(sdbbrdbug) cprintf("PUT_RBN: STEP 9\n");
	if(bbr_recurs == 2) {
		if(sdbbrdbug) cprintf("PUT_RBN: bbr_recurs = 2\n");
		goto STEP_16;
	}
	bbr_recurs++;
	if( error = get_rbn(lbn ) )
		goto STEP_16;
	newrbn = sd_st.ucb[sd_st.sd_drno].newrbn;
	oldrbn = sd_st.ucb[sd_st.sd_drno].oldrbn;
	if(sdbbrdbug) cprintf("PUT_RBN: newrbn= %d, oldrbn=%d\n",newrbn,oldrbn);
	/*
	*  STEP 10 -- Record the new RBN, whether or not the bad block has been
	*    previously replaced, (if appropriate) the bad block's old RBN, and
	*    the fact that we are in phase 2 of bad block replacement in sector 0
	*    of each RCT copy.  The RCT must be updated without reading, instead
	*    using the copy of sector 0 last read from or written to the RCT.
	*    If the RCT cannot be updated, report the error to the error log and
	*    go to step 16.
	*/

	if(sdbbrdbug) cprintf("PUT_RBN: STEP 10\n");
	*(rct0_ptr + 4*16) &= ~(bit15|bit14|bit13);
	*(rct0_ptr + 4*16) |= bit14;
	if( oldrbn >= 0 )
		*(rct0_ptr + 4*16) |= bit13;
	*(rct0_ptr + 8*16) = newrbn;
	*(rct0_ptr + 10*16) = oldrbn;
	if( error = wr_rct(0, rct0 ) )
		goto STEP_16;
	/*
	*  STEP 11 -- We update the RCT to indicate that the bad block has been
	*    replaced with the new RBN, and that the old RBN (if any) is
	*    unusable.  If this requires updating two blocks in the RCT, then
	*    both blocks must be read before either is written.  If a block
	*    cannot be read successfully, report the error to the error log and
	*    go to step 16.  If a block cannot be written successfully, report
	*    the error to the error log and go to step 15.
	*/
STEP_11:
	if(sdbbrdbug) cprintf("PUT_RBN: STEP 11\n");
	newrctblock = newrbn / 128 + 2;
	if( error = rd_rct( newrctblock, rct2 ) )
		goto STEP_16;
	*(rct2_ptr + (newrbn & 127)) = lbn | 0x30000000;
	if( oldrbn >= 0 ) {
		if(sdbbrdbug) cprintf("PUT_RBN: Has to invalidate oldrbn = %d\n",oldrbn);
		oldrctblock = oldrbn / 128 + 2;
		if( oldrctblock != newrctblock ) {
			if( error = rd_rct( oldrctblock, rct3 ) )
				goto STEP_16;
			*(rct3_ptr + (oldrbn & 127)) = 0x40000000;
			if( error = wr_rct( oldrctblock, rct3 ) )
				goto STEP_15A;
		}
		else {
			*(rct2_ptr + (oldrbn & 127)) = 0x40000000;
		}
	}
	if( error = wr_rct( newrctblock, rct2 ) )
		goto STEP_15B;
	/*
	*  STEP 12 -- We write the contents of the old bad block to the new RBN.
	*    If the saved data is invalid, the "force error" modifier is set.  If
	*    the write fails, go to step 9 to rescan the RCT for another RBN
	*    (note that the current new RBN will become the old RBN for this next
	*    pass).  The write command succeeds if no error is detected and the
	*    saved data is valid or if only a forced error is detected and the
	*    saved data is invalid.
	*/

	if(sdbbrdbug) cprintf("PUT_RBN: STEP 12\n");
	if(ddm)
		cmd = (SD_WRLOG | WR_DDMRK);
	else
		cmd = SD_WRLOG;
	block = newrbn + sd_st.ucb[sd_st.sd_drno].rbnbase;
	if( error = rdwr_poll(block,rct1,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0))
		goto STEP_9;
	cmd = (SD_RDLOG | RD_XFER);
	if( error = rdwr_poll(block,tmpbuf,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0))
		if(!(ddm_err && ddm))
			goto STEP_9;
	ddm_err = 0;
	data_ptr = (int *)rct1;
	temp_ptr = (int *)tmpbuf;
	for( i = 128; --i >= 0; )
		if( *data_ptr++ != *temp_ptr++ )
			goto STEP_9;
	if(replace(lbn)) {  /* Big trouble if replace failed */
		printf( "%s%d: REPLACE FAILURE at LBN %d \n", DEV_ID,sd_st.sd_drno,lbn);
		goto STEP_16;
	}

	/*
	*  STEP 13 -- We update sector 0 of each copy of the RCT to indicate that
	*    we are no longer in the middle of replacing a bad block.  The RCT
	*    must be updated without reading sector 0, instead using the copy of
	*    sector 0 last read from or written to the RCT.  If the RCT cannot be
	*    updated, report the error to the error log and go to step 17.
	*/
STEP_13:
	if(sdbbrdbug) cprintf("PUT_RBN: STEP 13\n");
	*(rct0_ptr + 4*16) &= ~( bit15|bit14|bit13|bit7 );
	*(rct0_ptr + 6*16) = 0;
	*(rct0_ptr + 8*16) = 0;
	*(rct0_ptr + 10*16) = 0;
	if( error = wr_rct( 0, rct0 ) )
		goto STEP_17;
	/*
	*  STEP 14 -- Exit successfully.
	*/
	if(newrbn >= 0) {
		printf( "%s%d: LBN %d replaced \n", DEV_ID,sd_st.sd_drno,lbn);
		mprintf( "%s%d: Replacement Block No = %d\n", DEV_ID,sd_st.sd_drno, newrbn );
	}

STEP_14:
	if(sdbbrdbug) cprintf("PUT_RBN: STEP 14, oldrbn = %d\n",newrbn);
	sd_st.ucb[sd_st.sd_drno].oldrbn = newrbn;
	return( 0 );

	/*
	*  STEP 15 -- We restore the RCT to indicate that the new RBN is
	*    unallocated and usable, and that the bad block is either not
	*    replaced or is revectored to the old RBN, whichever was its original
	*    status.  The RCT must be updated without reading any blocks from it,
	*    instead using the copies of the relevant blocks which were read in
	*    step 11.  Any errors are reported to the error log but are otherwise
	*    ignored.
	*/
STEP_15A:
	if(sdbbrdbug) cprintf("PUT_RBN: STEP 15A\n");
	*(rct3_ptr + oldrbn) = lbn | 0x30000000;
	wr_rct( oldrctblock, rct3 );
	goto STEP_16;
STEP_15B:
	if(sdbbrdbug) cprintf("PUT_RBN: STEP 15B\n");
	*(rct2_ptr + newrbn) = 0x00000000;
	wr_rct( newrctblock, rct2 );
	if( oldrbn >= 0 )
		if( oldrctblock != newrctblock )
			goto STEP_15A;
	/*
	*  STEP 16 -- Again we try to write the saved data to the suspected bad
	*    block.  The "force error" modifier bit is set if and only if the
	*    data is invalid.  Any errors are reported to the error log but are
	*    otherwise ignored.
	*/
STEP_16:
	if(sdbbrdbug) cprintf("PUT_RBN: STEP 16\n");
	if(ddm)
		cmd = (SD_WRLOG | WR_DDMRK);
	else
		cmd = SD_WRLOG;
	block = lbn + sd_st.ucb[sd_st.sd_drno].lbnbase;
	rdwr_poll(block,rct1,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0);
	/*
	*  STEP 17 -- We update sector 0 of each of the RCT copies to indicate
	*    that it is no longer in the middle of replacing a bad block.  The
	*    RCT must be updated without reading sector 0, instead using the copy
	*    of sector 0 last read from or written to the RCT.  Any errors are
	*    reported to the error log but are otherwise ignored.
	*/
STEP_17:
	if(sdbbrdbug) cprintf("PUT_RBN: STEP 17\n");
	*(rct0_ptr + 4*16) &= ~( bit15|bit14|bit13|bit7 );
	*(rct0_ptr + 6*16) = 0;
	*(rct0_ptr + 8*16) = 0;
	*(rct0_ptr + 10*16) = 0;
	wr_rct( 0, rct0 );
	/*
	*  STEP 18 -- The replacement has failed.  Return a status indicating
	*    this fact.  Force the unit to become "Unit-Available".
	*/
STEP_18A:
	if(sdbbrdbug) cprintf("PUT_RBN: STEP 18A\n");
/* STEP_18B: */
	if(sdbbrdbug) cprintf("PUT_RBN: STEP 18B\n");
	return( -1 );
}

/* This routine does I/O in a polled mode rather than interrupt mode. All the
 * reads/writes for BBR are done thru this routine */

rdwr_poll(blk,buf0,nsect,sect_no,cmd,count,frag_count)
	int blk,nsect,count,frag_count;
	char *buf0;
	short sect_no;
	u_char cmd;
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	register struct nb_regs *sdiaddr = (struct nb_regs *)nexus;
	register struct sdst *st;
	register unit,i,nbytes;
	union {
		short cyl_word;
		char cyl_byte[2];
	} cylin;
	union {
		short addr_word;
		char addr_byte[2];
	}addr;
	short sn,head;
	u_char udc_dsect,udc_dhead,udc_dcyl,udc_scnt,tmphd;
	u_char status,udc_cstat,udc_dstat;
	char * buf1;

	if(sdbbrdbug >=3) cprintf("rdwr_poll: reading sector %d\n",sect_no);
	unit = sd_st.sd_drno;
	st = &sdst[sd_st.sd_type[unit]];
	cylin.cyl_word = blk/st->nspc;
	if(cylin.cyl_word >= st->ncyl) {
		printf("rdwr_poll: invalid cylinder: %d\n",cylin.cyl_word);
		return(BBR_NORETRY);
	}
	sn = blk%st->nspc;
	sn %= st->nsect;
	head = (blk - (cylin.cyl_word*st->nspc))/st->nsect;
	if(head  >= st->nheads) {
		printf("rdwr_poll: invalid head%d\n",head);
		return(BBR_NORETRY);
	}
	tmphd = 0160 & (cylin.cyl_byte[1] << 4);
	udc_dsect = 0377&sn;
	udc_dhead = (tmphd | (0017 & head));
	udc_dcyl = cylin.cyl_byte[0];
	udc_scnt = nsect;
	addr.addr_word = sect_no*SD_SIZE;
	rbn_addr1 = addr.addr_byte[0];
	rbn_addr2 = addr.addr_byte[1];
	if(frag_count)
		nbytes = frag_count;
	else
		nbytes = nsect*SD_SIZE;
	if( ((cmd & SD_WRLOG) == SD_WRLOG) || ((cmd & SD_WRPHY) == SD_WRPHY) ){
		if(cylin.cyl_word > sd_uib[unit].pccyl)
			cmd = (cmd | RD31_PCOMP);
		if ((cpu == C_VAXSTAR) && cvs_exmode_on)
		    buf1 = (char *)cvseddbmem + addr.addr_word;
		else
		    buf1 = (char *)sdaddr->nb_ddb + addr.addr_word;
	        bcopy (buf0, buf1, nbytes);
	}
	if(sdbbrdbug >= 3) cprintf("rdwr_poll: dsect= %o, dhead= %o, dcyl= %o, scnt= %o,cmd= %o\n",udc_dsect,udc_dhead,udc_dcyl,udc_scnt,cmd);
	for(i=0; i<count; i++) {
		sdiaddr->nb_int_msk &= ~SINT_DC;
		sd_poll = 1;
		if(sd_rdwr(udc_dsect,udc_dhead,udc_dcyl,udc_scnt,cmd,sd_st.ucb[sd_st.sd_drno].blk_type) ) {
			if (sdiaddr->nb_int_reqclr & SINT_DC)
			    sdiaddr->nb_int_reqclr = SINT_DC;
			continue;
		}
		while(!(sdiaddr->nb_int_reqclr & SINT_DC))
			DELAY(10);	
		status = sdaddr->dkc_stat;
		ddm_err = 0;
		if( (status & DKC_TERMCOD) != DKC_SUCCESS) {
			sdaddr->dkc_cmd = (SD_SETREG | UDC_CSTAT);
			sdc_delay();
			udc_cstat = sdaddr->dkc_reg;
			sdc_delay();
			udc_dstat = sdaddr->dkc_reg;
			if( (udc_cstat & CST_CMPER) ||
			    (udc_cstat & CST_ECCER) ) {
				if( i==(count-1)) {
					if (sdiaddr->nb_int_reqclr & SINT_DC)
					    sdiaddr->nb_int_reqclr = SINT_DC;
					DELAY(1);
					sdiaddr->nb_int_msk |= SINT_DC; 
					if(sdbbrdbug >= 2) cprintf("rdwr_poll: udc_cstat = %o\n",udc_cstat);
					if(sdbbrdbug >=3) cprintf("rdwr_poll: dsect= %o, dhead= %o, dcyl= %o, scnt= %o,cmd= %o\n",udc_dsect,udc_dhead,udc_dcyl,udc_scnt,cmd);
					return(BBR_RETRY);
				}
				else {
					if (sdiaddr->nb_int_reqclr & SINT_DC)
					    sdiaddr->nb_int_reqclr = SINT_DC;
					continue;
				}
			}
			else if( (udc_dstat & DST_WRFAULT) == DST_WRFAULT) {
				if( i == (count-1)) {
					if (sdiaddr->nb_int_reqclr & SINT_DC)
					    sdiaddr->nb_int_reqclr = SINT_DC;
					DELAY(1);
					sdiaddr->nb_int_msk |= SINT_DC; 
					printf("rdwr_poll: WRITE FAULT\n");
					return(BBR_NORETRY);
				}
				else {
					sd_deselect();
					DELAY(100);
					if (sdiaddr->nb_int_reqclr & SINT_DC)
					    sdiaddr->nb_int_reqclr = SINT_DC;
					continue;
				}
			}
			else if ( (udc_cstat & CST_DELDT) == CST_DELDT) {
				if( i== (count-1)) {
					ddm_err = 1;
					break;
				}
				else {
					if (sdiaddr->nb_int_reqclr & SINT_DC)
					    sdiaddr->nb_int_reqclr = SINT_DC;
					continue;
				}

			}
			else {
				sdiaddr->nb_int_reqclr = SINT_DC;
				sdiaddr->nb_int_msk |= SINT_DC; 
				if(sdbbrdbug) cprintf("rdwr_poll: error: returing BBR_NORETRY\n");
				return(BBR_NORETRY);
			}
		}
		else 
			break;
	}
	sdiaddr->nb_int_reqclr = SINT_DC;
	sdiaddr->nb_int_msk |= SINT_DC; 
	if(i >= count) {
		if(sdbbrdbug) cprintf("rdwr_poll: returning BBR_NORETRY\n");
		return(BBR_NORETRY);
	}

	if( ((cmd & SD_RDLOG) == SD_RDLOG) || ((cmd & SD_RDPHY) == SD_RDPHY) ) {
		if ((cpu == C_VAXSTAR) && cvs_exmode_on)
		    buf1 = (char *)cvseddbmem + addr.addr_word;
		else
		    buf1 = (char *)sdaddr->nb_ddb + addr.addr_word;
	        bcopy (buf1, buf0, nbytes);
	}

	if(sdbbrdbug >=3) cprintf("rdwr_poll: returning success \n"); 
	if(ddm_err) {
		if(sdbbrdbug) cprintf("rdwr_poll: DDM_ERR\n");
		return(BBR_RETRY);
	}
	return(0);

}

/*
 *  this routine will replace a bad block by reformatting the track which the
 *  block is on and marking its header in such a way that the block cannot be
 *  found any more
 */
replace( lbn )
	int lbn;
{
	register struct sdst *st;
	register int i, j, sector, surface, cylinder;
	int block,phy_block;
	u_char cmd;

	if(sdbbrdbug >=2) cprintf( "\nreformatting near LBN %ld", lbn );
	if(sdbbrdbug) cprintf("REPLACE\n");
	phy_block = lbn + sd_st.ucb[sd_st.sd_drno].lbnbase;
	/*
	*  remember the physical location of the defective block
	*/
	st = &sdst[sd_st.sd_type[sd_st.sd_drno]];
	sector = sd_st.ucb[sd_st.sd_drno].badsect + start_sn; 
	cylinder = phy_block/st->nspc;
	if(cylinder >= st->ncyl) {
		printf("replace: invalid cylinder no: %d\n",cylinder);
		return(-1);
	}
	surface = (phy_block - (cylinder*st->nspc))/st->nsect;
	if(surface >= st->nheads) {
		printf("replace: invalid head no: %d\n",surface);
		return(-1);
	}
	fill_id(cylinder,surface);
	block = lbn - sector;
	for( i = 0; i < sd_uib[sd_st.sd_drno].sec; i++ ) {
	/*
	 *  any block on this track which has an entry in the RCT is known
	 *  to be bad, and its header will be marked so that attempts to
	 *  access it will fail
	 */
		if( ( !get_rbn( block + i ) ) && ( sd_st.ucb[sd_st.sd_drno].oldrbn >= 0 ) ) {
			if(sdbbrdbug) cprintf("replace: Sector %d really bad\n",i);
			for( j = 0; id_table[j][2] != i; j++ )
				;
			id_table[j][2] = -1;
		}
	/*
	 *  copy the data in each block to one of the XBNs 
	 */

		phy_block = block + i + sd_st.ucb[sd_st.sd_drno].lbnbase;
		cmd = (SD_RDLOG | RD_XFER);
		rdwr_poll(phy_block,rct1,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0);
		if(wr_xbn( i + 3, rct1 ))
			return(-1);

	}

	/*
	*  do the actual reformatting
	*/
	if( reformat( cylinder,surface ))
		return(-1);
	/*
	*  restore the data from the XBNs to each block on this track
	*/
	cmd = SD_WRLOG;
	for( i = 0; i < sd_uib[sd_st.sd_drno].sec; i++ ) {
		if(rd_xbn( i + 3, rct1 ))
			return(-1);
		phy_block = block + i + sd_st.ucb[sd_st.sd_drno].lbnbase;
		rdwr_poll(phy_block,rct1,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0);
	}
	return(0);
}

/*
 *  this routine implements the XBN multi-read algorithm
 *
 *  The algorithm consists of reading each of the XBN copies in turn, using a
 *  compare operation.  If the compare fails, the next copy is tried, until
 *  either we succeed or we run out of copies.
 */
rd_xbn( i, buffer )
	register int i;
	char *buffer;
{
	register int  *data_ptr, *temp_ptr;
	register block,j, k;
	u_char cmd;

	if(sdbbrdbug >= 2) cprintf("RD_XBN block %d\n",i);
	block = i;
	for( j = 3; --j >= 0; ) {
		cmd = (SD_RDLOG | RD_XFER);
		if( !rdwr_poll(block,buffer,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0)) {
			if( !rdwr_poll(block,tmpbuf,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0)) {
				data_ptr = (int *)buffer;
				temp_ptr = (int *)tmpbuf;
				for( k = 128; --k >= 0; )
					if( *data_ptr++ != *temp_ptr++ )
						break;
				if( k < 0 )
					return( 0 );
			}
		}
		block += sd_uib[sd_st.sd_drno].sec;
	}
	return( -1 );
}

/*
 *  this routine implements the XBN multi-write algorithm
 *
 *  The algorithm consists of writing each of the XBN copies in turn, using a
 *  compare operation.  If the compare fails, the block is rewritten with the
 *  "force error" flag set (so that future reads of it will fail).  Each copy
 *  is done this way, and the operation succeeds if any write/compare worked
 *  correctly.
 */
wr_xbn( i, buffer )
	register int i;
	char *buffer;
{
	register int *data_ptr, *temp_ptr;
	register block, j, k;
	int good;
	u_char cmd;

	if(sdbbrdbug) cprintf("WR_XBN block %d\n",i);
	good = 0;
	block = i;
	for( j = 3; --j >= 0; ) {
		cmd = SD_WRLOG;
		if( !rdwr_poll(block,buffer,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0)) {
			cmd = (SD_RDLOG | RD_XFER);
			if( !rdwr_poll(block,tmpbuf,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0)) {
				data_ptr = (int *)buffer;
				temp_ptr = (int *)tmpbuf;
				for( k = 128; --k >= 0; )
					if( *data_ptr++ != *temp_ptr++ )
						break;
				if( k < 0 )
					good = 1;
				else {
					cmd = (SD_WRLOG | WR_DDMRK);
					rdwr_poll(block,buffer,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0);
				}
			}
		}
		block += sd_uib[sd_st.sd_drno].sec;
	}
	return( good ? 0 : -1 );
}

/*
 *  this routine will fill the id table for formatting purposes
 *
 *  This is a convoluted routine which takes sector-to-sector interleave,
 *  surface-to-surface skew, and cylinder-to-cylinder skew into account.  Take
 *  my word for it that it works, and don't touch the code. ( From rqdx3 microcode)
 */
fill_id(cylinder,surface )
	register int cylinder,surface;
{
	register int i, j, skew;

	if(sdbbrdbug) cprintf("FILL_ID\n");
	skew = ( surface * sd_uib[sd_st.sd_drno].sur_skew +
		cylinder * sd_uib[sd_st.sd_drno].cyl_skew ) % sd_uib[sd_st.sd_drno].sec;
	for( j = 0; j < sd_uib[sd_st.sd_drno].sec; j++ )
		id_table[j][3] = 0;
	for( i = 0; i < sd_uib[sd_st.sd_drno].sec; i++ ) {
		j = ( skew + i * sd_uib[sd_st.sd_drno].sec_interleave ) % sd_uib[sd_st.sd_drno].sec;
		while( id_table[j][3] != 0 )
			j = ( j + 1 ) % sd_uib[sd_st.sd_drno].sec;
		id_table[j][0] = cylinder;
		id_table[j][1] = surface + ( ( cylinder >> 4 ) & 0xF0 );
		id_table[j][2] = i;
		id_table[j][3] = 2;
	}
	return(0);
}

/*
 *  this routine will format a given cylinder and surface
 *
 *  This provides the low-level code necessary to actually alter the headers
 *  which need to be marked bad.  Again, touch this code at your own risk.(from
 *  rqdx3 microcode)
 */
reformat(cylinder,surface )
	register int cylinder,surface;
{
	register struct nb1_regs *sdaddr = (struct nb1_regs *)qmem;
	register struct nb_regs *sdiaddr = (struct nb_regs *)nexus;
	register int i,j ;
	char *buf0;
	u_char status,cmd;
	union {
		short addr_word;
		u_char addr_byte[2];
	} addr;

	/* Update disk data buffer with id_table values */
	/* Select the drive */
	/* Load UDC_DHEAD */
	/* Load the reqd. regs */
	/* Load UDC_MODE */
	/* Restore */
	/* STEP to the required cylinder */
	/* Issue FORMAT TRACK command */
	
	if(sdbbrdbug) cprintf("REFORMAT\n");
	addr.addr_word = sd_st.ucb[sd_st.sd_drno].badsect * SD_SIZE;
	if ((cpu == C_VAXSTAR) && cvs_exmode_on)
		buf0 = (char *)(cvseddbmem) + addr.addr_word;
	else
		buf0 = (char *)(sdaddr->nb_ddb) + addr.addr_word;
	for(i=0; i<sd_uib[sd_st.sd_drno].sec; i++) {
		for(j=0; j<4; j++) {
			*buf0++ = id_table[i][j];
		}
	}

	sdaddr->dkc_cmd = (SD_SETREG | UDC_DMA7);
	sdc_delay();
	sdaddr->dkc_reg = addr.addr_byte[0];;	/* UDC_DMA7 */
	sdc_delay();
	sdaddr->dkc_reg = addr.addr_byte[1];;	/* UDC_DMA15 */
	sdc_delay();
	sdaddr->dkc_reg = 0;  	/* UDC_DMA23 */

	sdiaddr->nb_int_msk &= ~SINT_DC;
	cmd = (SD_SELECT | DTRT_HDSK | (0377&sd_st.sd_drno));
	if(sd_select(cmd,0))
		return(-1);  

	sdiaddr->nb_int_reqclr = SINT_DC;
	sdaddr->dkc_cmd = (SD_SETREG | UDC_DHEAD);
	sdc_delay();
	sdaddr->dkc_reg = surface;
	sdc_delay();
	sdaddr->dkc_cmd = (SD_SETREG | UDC_DMA7);
	sdc_delay();
	sdaddr->dkc_reg = -sd_uib[sd_st.sd_drno].gap0;  /* UDC_DMA7 */
	sdc_delay();
	sdaddr->dkc_reg = -sd_uib[sd_st.sd_drno].gap1;  /* UDC_DMA15 */
	sdc_delay();
	sdaddr->dkc_reg = -sd_uib[sd_st.sd_drno].gap2;  /* UDC_DMA23 */
	sdc_delay();
	sdaddr->dkc_reg = -sd_uib[sd_st.sd_drno].gap3;  /* UDC_DSECT */
	sdc_delay();
	sdaddr->dkc_cmd = (SD_SETREG | UDC_DCYL);
	sdc_delay();
	sdaddr->dkc_reg = ~sd_uib[sd_st.sd_drno].sync;  /* UDC_DCYL */
	sdc_delay();
	sdaddr->dkc_reg =  ~sd_uib[sd_st.sd_drno].sec;  /* UDC_SCNT */
	sdc_delay();
	sdaddr->dkc_reg = ~4;	/* UDC_RTCNT */
	sdc_delay();
	sdaddr->dkc_reg = (MOD_HD | MOD_CHKECC | MOD_SRTRDR);  /* UDC_MODE */
	sdc_delay();
	for(i=0; i<10; i++) {
		sdaddr->dkc_cmd = (SD_RESTOR | REST_WAIT);
		while(!(sdiaddr->nb_int_reqclr & SINT_DC))
			DELAY(10);	
		
		status = sdaddr->dkc_stat;
		if( (status & DKC_TERMCOD) == DKC_VERERR) {
			sdiaddr->nb_int_reqclr = SINT_DC;
			continue;
		}
		else 
			break;
	}
	sdiaddr->nb_int_reqclr = SINT_DC;
	if(i >= 10) {
		cprintf("reformat: RESTORE failed: dkc_stat=%o\n",status);
		sdiaddr->nb_int_msk |= SINT_DC;
		return(-1); 
	}
	sdaddr->dkc_cmd = (SD_SETREG | UDC_MODE);
	sdc_delay();
	sdaddr->dkc_reg = (MOD_HD | MOD_CHKECC | MOD_SRTRDN);  /* restore to buffered seek */

	/* Step to the required cylinder */
	for(i=0; i < cylinder; i++) {
		sdaddr->dkc_cmd = (SD_STEP | STEP_WAIT);
		while(!(sdiaddr->nb_int_reqclr & SINT_DC))
			DELAY(10);	
		sdiaddr->nb_int_reqclr = SINT_DC;
	}

	/* Issue the Format command */
	if(cylinder > sd_uib[sd_st.sd_drno].pccyl)
		sdaddr->dkc_cmd = (SD_FMT | RD31_PCOMP);  /* Write precompensation */
	else
		sdaddr->dkc_cmd = SD_FMT;
	
	while(!(sdiaddr->nb_int_reqclr & SINT_DC))
		DELAY(10);	
	/*  How to check for failure ?? */
	sdiaddr->nb_int_reqclr = SINT_DC;
	sdiaddr->nb_int_msk |= SINT_DC;
	status = sdaddr->dkc_stat;
	if((status & DKC_DONE) != DKC_DONE) {
		cprintf("reformat: FORMAT command not done: dkc_stat=%o\n",status);
		return(-1);
	} else
		return(0);


}


/*
 *  this routine implements the RCT multi-write algorithm
 *
 *  The algorithm consists of writing each of the RCT copies in turn, using a
 *  compare operation.  If the compare fails, the block is rewritten with the
 *  "force error" flag set (so that future reads of it will fail).  Each copy
 *  is done this way, and the operation succeeds if any write/compare worked
 *  correctly.
 */
wr_rct(i, buffer )
	int i;
	char *buffer;
{
	register int *data_ptr, *temp_ptr;
	register blkno;
	register int j, k;
	int good;
	u_char cmd;

	if(sdbbrdbug) cprintf("WR_RCT block %d\n",i);
	good = 0;

	blkno = sd_st.ucb[sd_st.sd_drno].lbnbase + sd_st.ucb[sd_st.sd_drno].hostsize + i;
	/*
	*  do this until we succeed or until there are no more copies
	*/
	for( j = sd_uib[sd_st.sd_drno].rctcopies; --j >= 0; ) {
		cmd = SD_WRLOG;
		if( !rdwr_poll(blkno,buffer,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0)) {
	    	/* read rct into tmpbuf */
			cmd = (SD_RDLOG | RD_XFER);
			if( !rdwr_poll(blkno,tmpbuf,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0)) {
		/*
		 *  we have written and then read the data now with no errors;
		 *  make sure we read back the same data that we wrote
		 */
				data_ptr = (int *)buffer;
				temp_ptr = (int *)&tmpbuf[0];
				for( k = 128; --k >= 0; )
					if( *data_ptr++ != *temp_ptr++ )
						break;
				if( k < 0 )
					good = 1;
				else {
		    /*
		     *  turn on the "force error" flag
		     */
					cmd = (SD_WRLOG | WR_DDMRK);
					rdwr_poll(blkno,buffer,1,sd_st.ucb[sd_st.sd_drno].badsect,cmd,RTRY_CNT,0); 
				}
			}
		}
		/*
		 *  this copy is done, do the next one
		 */
		blkno += sd_uib[sd_st.sd_drno].rctsize;
	}
	return( good ? 0 : -1 );
}
sdread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = minor(dev) >> 3;

	if (unit >= nNSD)
		return (ENXIO);
	return (physio(sdstrategy, &rsdbuf[unit], dev, B_READ, minphys, uio));
}

sdwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = minor(dev) >> 3;

	if (unit >= nNSD)
		return (ENXIO);
	return (physio(sdstrategy, &rsdbuf[unit], dev, B_WRITE, minphys, uio));
}

/*ARGSUSED*/
sdioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	struct pt *pt = (struct pt *)data;
	struct uba_device *ui;
	int unit = minor(dev) >> 3;
	int i;
	int error;
	struct devget *devget;
	struct dkacc *dkacc = (struct dkacc *)data;

#ifdef BBR_TST
	struct buf *bbr_buf;
#endif BBR_TST

	switch (cmd) {

	case DIOCGETPT:	/* get partition table info */
	case DIOCDGTPT:	/*  Return default partition table */
		if(cmd == DIOCGETPT)
			/*
			 *	Do a structure copy into the user's data area
			 */

			*pt = sd_part[unit];

		else {

			ui = sddinfo[unit];

			/*
			 * Get number of sector per cyliinder
			 */

			/*
			 * Get and store the default block count and offset
			 */
			for( i = 0; i <= 7; i++ ) {
				pt->pt_part[i].pi_nblocks = 
					sdst[ui->ui_type].sizes[i].nblocks;
				pt->pt_part[i].pi_blkoff =
					sdst[ui->ui_type].sizes[i].blkoffs;
			}

		}

		/* Change all of the -1 nblocks to the actual value */

		for(i=0; i<=7; i++) {
			if(pt->pt_part[i].pi_nblocks == -1)
				pt->pt_part[i].pi_nblocks = 
					sd_st.ucb[unit].hostsize - pt->pt_part[i].pi_blkoff;
		}
		pt->pt_magic = PT_MAGIC;
		return (0);

	case DIOCSETPT: /* set the driver partition tables */
		/*
		 *	Only super users can set the pack's partition
		 *	table
		 */

		if ( !suser() )
			return(EACCES);
		
		/*
		 *	Before we set the new partition tables make sure
		 *	that it will no corrupt any of the kernel data
		 *	structures
		 */
		if ( ( error = ptcmp( dev, &sd_part[unit], pt ) ) != 0 )
			return(error);

		/*
		 *	Using the user's data to set the partition table
		 *	for the pack
		 */

		sd_part[unit] = *pt;

		/*
		 *	See if we need to update the superblock of the
		 *	"a" partition of this disk
		 */
		ssblk(dev,pt);

		/*
		 *	Just make sure that we set the valid bit
		 */

		sd_part[unit].pt_valid = PT_VALID;
		return(0);

	case DEVIOCGET:	
		ui = sddinfo[unit];
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		devget->category = DEV_DISK;
		devget->bus = DEV_NB;
		bcopy(DEV_VS_DISK, devget->interface,
		      strlen(DEV_VS_DISK));
		switch(sd_st.sd_type[unit]) {
		case DT_RX50:
		case DT_RX33:
			bcopy(DEV_RX33, devget->device, strlen(DEV_RX33));
			break;
		case DT_RX23H:
		case DT_RX23D:
			bcopy(DEV_RX23,devget->device,strlen(DEV_RX23));
			break;
		case DT_RD31:
			bcopy(DEV_RD31, devget->device, strlen(DEV_RD31));
			break;
		case DT_RD32:
			bcopy(DEV_RD32, devget->device, strlen(DEV_RD32));
			break;
		case DT_RD33:
			bcopy(DEV_RD33, devget->device, strlen(DEV_RD33));
			break;
		case DT_RD53:
			bcopy(DEV_RD53, devget->device, strlen(DEV_RD53));
			break;
		case DT_RD54:
			bcopy(DEV_RD54, devget->device, strlen(DEV_RD54));
			break;
		default:
			bcopy(DEV_UNKNOWN, devget->device,
			      strlen(DEV_UNKNOWN));
			break;
		}
		devget->adpt_num = 0;
		devget->nexus_num = 0;
		devget->bus_num = 0;
		devget->ctlr_num = 0;
		devget->slave_num = ui->ui_slave;
		switch(sd_st.sd_type[unit]) {
		case DT_RX50:
		case DT_RX33:
		case DT_RX23H:
		case DT_RX23D:
			bcopy("rx", devget->dev_name, 3);
			break;
		case DT_RD31:
		case DT_RD32:
		case DT_RD33:
		case DT_RD53:
		case DT_RD54:
			bcopy("rd", devget->dev_name, 3);
			break;
		default:
			bcopy(DEV_UNKNOWN, devget->dev_name,
			      strlen(DEV_UNKNOWN));
			break;
		}
		devget->unit_num = unit;
		devget->soft_count = sd_st.sd_softcnt[ui->ui_unit];/* soft error count	*/
		devget->hard_count = sd_st.sd_hardcnt[ui->ui_unit];/* hard error count	*/
		devget->stat = sd_st.sd_flags[unit];
		devget->category_stat = DEV_DISKPART;	/* which partition   */
		return(0);

	case DKIOCACC:
		if( !suser() )
			return(EACCES);
		ui = sddinfo[unit];
		if(ui->ui_alive == 0)
			return(EACCES);
		switch (dkacc->dk_opcode) {
		case ACC_REVEC:
#ifdef BBR_TST
			cprintf("ACC_REVEC: blkno = %d\n",dkacc->dk_lbn);
			bbr_buf = geteblk(512);
			bbr_buf->b_bcount = 512;
			bbr_buf->b_dev = dev;
			bbr_buf->b_blkno = dkacc->dk_lbn;
			bbr_buf->b_flags |= (B_READ | B_BAD);
			bbr_sleep = 1;
			sdstrategy(bbr_buf);
			while(bbr_sleep)
				;
			brelse(bbr_buf);
			return(0);
#else
			return(EACCES);
#endif BBR_TST
		case ACC_SCAN:
			dkacc->dk_status = NO_ACCESS;
			return(0);
		default:
			return(ENXIO);
		}

	default:
		return (ENXIO);
	}
}
 

sddump(dev)
{

#ifdef lint
	dev = dev;
#endif lint

	return(ENXIO);
}

sdsize(dev)
{
	int unit = minor(dev) >> 3;
	struct uba_device *ui;

	if (unit >= nNSD || (ui = sddinfo[unit]) == 0 || ui->ui_alive == 0)
		return (-1);
	/*
	 *	Sanity check		
	 */
	if ( sd_part[unit].pt_valid != PT_VALID )
		panic("sdsize: invalid partition table ");

	if  (sd_part[unit].pt_part[minor(dev) & 07].pi_nblocks != -1) 
		return (sd_part[unit].pt_part[minor(dev) & 07].pi_nblocks); 
	else
		return(sd_st.ucb[unit].hostsize - sd_part[unit].pt_part[minor(dev) & 07].pi_blkoff);
}

#endif
