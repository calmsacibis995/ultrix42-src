
#ifndef lint
static char *sccsid = "@(#)stc.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *		Copyright (c) 1984, 1986, 1987, 1988, 1989 by		*
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
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 * stc.c 	02-Mar-89
 *
 * Modification history
 *
 * TZK50 Tape Driver
 *
 * 02-Dec-89	Fred Canter
 *	Bug fix; mprintf to error log printed address of sense data
 *	bytes instead of the data in the sense data bytes.
 *	Bug fix; restore issues a MTNOCACHE ioctl which causes a hardware
 *	error on the TZK50. This prevented TK50 installtaions to MV2000
 *	and VS2000 systems.
 *
 * 17-May-89	Fred Canter
 *	Yet another bug fix. Back/forward space thru a file mark should
 *	fail but wasn't.
 *
 * 13-May-89	Fred Canter
 *	Fixed yet another bug uncovered by the V3.1 installation. There
 *	were many errors in the DEV_TPMARK flag handling. Code changes
 *	identified with "#ifdef TM_NO_FIX". To include the fix, don't
 *	define TM_NO_FIX.
 *
 * 10-May-89	Fred Canter
 *	Fix to last bug fix. Caused driver to return error on filemark
 *	read instead of returning the users' byte count.
 *
 * 25-Apr-89	Fred Canter
 *	Cleared all flags in the open routine. This fixed a bug which
 *	caused the driver to fail all reads after any fatal error, until
 *	the system was rebooted.
 *
 *	Changed printf to printd1 for the "Didn't space back over second
 *	filemark" debug message.
 *
 * 24-Apr-89	Fred Canter
 *	Rev 45 of the TZK50 firmware caused this driver to fail, because
 *	it did not expect a save data pointer message with no data
 *	remaining to be transferred (i.e., DMA count register = 0).
 *
 * 02-Mar-89	John A. Gallant
 *	In st_start(), correctly initialized the um pointer.
 *
 * 28-Sep-88	Fred Canter
 *	Clean up comments.
 *
 * 01-Sep-88	darrell
 *	ST_W_DMA: now saves a copy of sc_stflags which it restore, as
 *	the call to st_scsistart() clears sc_stflags.  Also corrected
 *	the check in ST_SP_CONT: that determines if the REQUEST SENSE
 *	succeeded.  This fixes a P1 bug that was elevated to a CLD
 *	(SPR 4st-00571)
 *
 * 06-Jun-88	Fred Canter
 *	Changes to allow this driver to use PVAX extended data buffer
 *	I/O mode.
 *
 * 15-Feb-88	Fred Canter
 *	Changes for VAX420 (CVAXstar/PVAX) support.
 *
 * 12-Jan-88	darrell
 *	Rewrote st_timer().  I had forgotten about the fact that a
 *	space command could take up to approximately 40 minutes to 
 *	complete.  The timer was set to go off every 3 minutes and 
 *	if a command had not completed as of the second time timeout
 *	the drive was assumed to be dead.  Each command is now looked
 *	up in a table and timed separately.
 *
 * 15-Sep-87	darrell
 *	Copied most if stintr() into another routine st_scsistart() which
 *	is used to start the scsi transfer.  stintr used to be called by
 *	st_start() to start the scsi transfer which was clearly not how
 *	an interrupt routine is supposed to work, and it also made
 *	returning from stintr() and st_start() quite complex.  As a result
 *	of these latest changes, st_start calls st_scsistart() to start
 *	the transfer, and stintr() is only called from interrupt -- which
 *	is much cleaner, and the "right" thing to do.  At the same time
 *	the sc_stflags field in the ST_R_DMA: case is restored after the
 *	Request Sense Command -- which fixes a panic which was happening
 *	in vs_bufctl.
 *
 * 20-Aug-87	darrell
 *	Added a timeout routine (st_timer) to timeout commands and reset 
 *	the controller.  A request diagnostic results command is now send
 *	to read the controller firmware and refuse to be opened if the
 *	firmware rev is below REV 34.  The first command sent to the drive
 *	now is a Receive Diagnostic Results command.  This should always
 *	return success if the drive is there and working, so we no longer 
 *	need to wait for three minutes with a Test Unit Ready of "not ready"
 *	in order to tell if the tape is there.
 *
 * 23-Apr-87	darrell
 *	Changed the calls to vs_bufctl to pass a pointer to a structure
 *	that contains a pointer to the function	that vs_bufctl is to call.
 *	This is the first step of cleaning up some of this code, and making
 *	it more maintainable.
 *
 * 15-Mar-87	darrell
 *	Removed the code that does a bus reset.  The bus reset was being
 *	done on all error conditions.  Now a bus reset is never done.
 *
 * 03-Mar-87	darrell
 *	Added multi-volume support.  Fixed various small bugs.  Increased
 *	the default number of fills to seven (7).  If the drive exists, but
 *	the tape is not loaded, and the open routine is called with FNDELAY,
 *	the driver now waits to make sure that there is not an outstanding
 *	rewind.
 *
 * 27-Jan-87	darrell
 *	Added support for the change in V32 of the TZK50 microcode that
 *	allows the reselect timeout (TZK50 timing out the host) to be
 *	disabled.  A change was also made to catch and return an iodone
 *	on buffers that were queued as a result of nbuffio, after a
 *	hard error was detected.  Also OR'ed the density bits into the
 *	category_stat field in the DEVIOGET case of stioctl -- so now
 *	dump can calculate the correct length of tape needed.
 *
 * 15-Jan-87	darrell
 *	Rewrote about half of this driver to support
 *	disconnects/reselects, make the tape stream, and
 * 	reduce the adverse performance impact this driver
 *	had on the system.  Seems to work well.  EOT is
 *	still not tested.
 *
 * 23-Oct-86	darrell
 *	Fixed a FNDELAY problem so finder now works.
 *
 * 26-Sep-86	darrell
 *	Cleaned up error message printouts, and fixed a problem where
 *	opening the device FNDELAY allowed the first command after
 *	changing a tape to fail.
 *
 * 26-Sep-86	darrell
 *	Removed a debug printf I had left in my last delta.
 *
 * 26-Sep-86	darrell
 *	Fixed a problem queueing multiple requests so that nbuf
 *	I/O will work.  Also enabled buffered writing.
 *
 * 12-Sep-86	darrell
 *	Fixed remaining ioctl's, put in the devioctl code to
 *	to work with the utilities using nbufio.  bp->b_resid
 *	is now returned correctly.  Fixed many other little bugs.
 *
 * 04-Sep-86	darrell
 *	Made ioctls work - all except MTIOCGET:, put in code to
 *	handle EOM, EOT, and other tape end contition, and did a
 *	lot of cleanup in removing old unused code, and comments,
 *	and changed ST_SP_COMPL: DATAI: to only read as many
 *	characters as are expected in program I/O -- as the STATUS
 *	byte was getting sucked in.
 *
 *  4-Sep-86    darrell (Darrell Dunnuck)
 *	First semi-working driver.
 *
 * 26-Aug-86	rsp (Ricky Palmer)
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry and (2) use strlen instead of fixed storage
 *	for bcopy's.
 *
 *  5-Aug-86	darrell
 *	First pass of real driver (compiles but not debugged).
 *
 * 12-Jun-86	darrell
 *	started with ht.c, and made it into this file.
 *
 */

/***********************************************************************
 * TODO:
 *
 *	1. This driver needs to support record sizes greater that 16k.
 *	   The maximum size supported by the TZK50 is 64k.  This driver 
 *	   should be changed to support record sizes up to 64k.
 ***********************************************************************
 */
  

/*  */
#if NST > 0 || defined(BINARY)

#include "../data/stc_data.c"

extern int cvs_exmode_on;
u_char inicmd_tmp;  /* keep this around since we can't |=  the scs_inicmd reg */
int st_from_open;   /* used to tell when the command was initiated from stopen*/
int st_retries = 0; /* retry counter					      */
int st_max_numof_fills = ST_DEFAULT_FILLS; /* Maximum number of fills to write*/
					   /* in order to try to keep the tape*/
					   /* drive streaming.		      */

extern struct vsbuf vsbuf;
struct vsdev vstapedev = { VS_ST, 0, 0 };
int	stprobe(), stslave(), stattach(), stintr(), st_start(),	sterror(), st_timer(), st_scsistart();

u_short	ststd[] = { 0 };

struct	uba_driver stcdriver = { stprobe, stslave, stattach, st_start,
				ststd, "st", stdinfo, "stc", stminfo,
				0 };

int wakeup();
extern int hz;
extern struct nexus nexus[];
struct	stspace	ST_bufmap[];

extern	int	cpu;
extern	int	cpu_subtype;

#define ST_TIMERRATE	30	/* how often the timer goes off		      */

int	unit_attn = 1;	/* An indication that the tape was removed */

/*
 * The following table is the timeout value for each command assuming that
 * the timer is set to go off every 30 seconds.
 */
char st_timetable[48] = {	 2,	/* ST_TUR		0x00	*/
				 6,	/* ST_REWIND		0x01	*/
				 0,	/* unused		0x02	*/
				 2,	/* ST_RQSNS		0x03	*/
				 0,	/* unused		0x04	*/
				 0,	/* ST_RBL		0x05	*/
				 0,	/* unused		0x06	*/
				 0,	/* unused		0x07	*/
				 8,	/* ST_READ		0x08	*/
				 0,	/* unused		0x09	*/
				 8,	/* ST_WRITE		0x0a	*/
				 2,	/* ST_TRKSEL		0x0b	*/
				 0,	/* unused		0x0c	*/
				 0,	/* unused		0x0d	*/
				 0,	/* unused		0x0e	*/
				 0,	/* unused		0x0f	*/
				 2,	/* ST_WFM		0x10	*/
				90,	/* ST_SPACE		0x11	*/
				 2,	/* ST_INQ		0x12	*/
				 0,	/* ST_VFY		0x13	*/
				 0,	/* ST_RBD		0x14	*/
				 2,	/* ST_MODSEL		0x15	*/
				 0,	/* ST_RESUNIT		0x16	*/
				 0,	/* ST_RELUNIT		0x17	*/
				 0,	/* unused		0x18	*/
				 0,	/* ST_ERASE		0x19	*/
				 2,	/* ST_MODSNS		0x1a	*/
				 6,	/* ST_LOAD/ST_UNLOAD	0x1b	*/
				 6,	/* ST_RCVDIAG		0x1c	*/
				 0,	/* ST_SNDDIAG		0x1d	*/
				90,	/* ST_FSPACER		0x1e	*/
				90,	/* ST_FSPACEF		0x1f	*/
				 0,	/* unused		0x20	*/
				 0,	/* unused		0x21	*/
				 0,	/* unused		0x22	*/
				 0,	/* unused		0x23	*/
				 0,	/* unused		0x24	*/
				 0,	/* unused		0x25	*/
				 0,	/* unused		0x26	*/
				 0,	/* unused		0x27	*/
				 0,	/* unused		0x28	*/
				 0,	/* unused		0x29	*/
				 0,	/* unused		0x2a	*/
				 0,	/* unused		0x2b	*/
				 2,	/* ST_P_CACHE		0x2c	*/
				 2,	/* ST_P_NOCACHE		0x2d	*/
				90,	/* ST_BSPACER		0x2e	*/
				90	/* ST_BSPACEF		0x2f	*/
};

/* STDEBUG intentionally left defined */
#define STDEBUG
#ifdef STDEBUG
#define ST_TRACK_SIZE     100	/* size of the tracking buffer */
#define ST_CMDTRACK_SIZE  600	/* size of the command tracking buffer */
char sttrack[ST_TRACK_SIZE];
char stcmdtrack[ST_CMDTRACK_SIZE];
int st_cmdindex = ((ST_CMDTRACK_SIZE) - 1);
int st_index = ST_TRACK_SIZE - 1;
/*
 * ST_TRACK - a macro that writes into the ST_TRACK_SIZE
 * array that is used as a trail of what this driver has
 * done.  The data is written into the array starting from
 * the highest address down to the lowest address to make
 * it easier to trace backwards using adb.
 *
 */
#define ST_TRACK(a) if(st_index < 0 )				\
			st_index = (ST_TRACK_SIZE - 1);		\
		    sttrack[st_index--] = a
/*
 * ST_CMDTRACK - a macro that writes into an array the command
 * that was sent to the drive -- to be used as a trail of 
 * what this driver has done. The data is written into the array 
 * starting from the highest address down to the lowest address
 * to make it easier to trace backwards using adb.
 */
#define ST_CMDTRACK(a) if(st_cmdindex < 0 )				\
			st_cmdindex = ((ST_CMDTRACK_SIZE) - 1);	\
		    stcmdtrack[st_cmdindex--] = a

int stdebug = 0;
#define printd if(stdebug)printf
#define printd1 if (stdebug > 1) printf
#define printd2 if (stdebug > 2) printf
#define printd3 if (stdebug > 3) printf
#define printd4 if (stdebug > 4) printf
#define printd5 if (stdebug > 5) printf
#define printd6 if (stdebug > 6) printf
int st_direct_track_mode = 0;	/* for testing multivolume dump */
u_char st_prev_cmd;	/* keeps track of the previous command sent to drive */

/*
 * Trace Tables
 */
char	*trace_xstate [] = {
	"ST_NEXT",
	"ST_SP_START",
	"ST_SP_CONT",
	"ST_RW_START",
	"ST_R_STDMA",
	"ST_W_STDMA",
	"ST_R_DMA",
	"ST_W_DMA",
	"ST_R_COPY",
	"ST_R_DMACONT",
	"ST_W_DMACONT",
	"ST_ERR"
};

char	*trace_fstate [] = {
	"ST_SP_ARB",
	"ST_SP_SEL",
	"ST_CMD_PHA",
	"ST_DATAO_PHA",
	"ST_DATAI_PHA",
	"ST_STATUS_PHA",
	"ST_MESSI_PHA",
	"ST_MESSO_PHA",
	"ST_RELBUS",
	"ST_ABORT"
};

char	*trace_event [] = {
	"ST_CONT",
	"ST_BEGIN",
	"ST_DMA_DONE",
	"ST_PAR_ERR",
	"ST_PHA_MIS",
	"ST_RESET",
	"ST_CMD",
	"ST_DMA",
	"ST_ABRT",
	"ST_ERROR",
	"ST_TIMEOUT",
	"ST_FREEB"
};
#endif STDEBUG

/*  */
stprobe(reg, cntlr)
	int cntlr;
	caddr_t reg;
{
	register struct nb1_regs *staddr = (struct nb1_regs *)qmem;
	register struct nb_regs *stiaddr = (struct nb_regs *)nexus;

#ifdef STDEBUG
	printd6("stprobe: probing TZK50\n");
#endif STDEBUG

	/*
	 * This driver only runs on VAXstar/Teammate.
	 * It cannot be used with CVAXstar/PVAX.
	 */
	if (cpu != VAXSTAR)
		return(0);
	stiaddr->nb_int_reqclr = SCS_INT_TAPE;	/* clear SCSI in INT_REQ reg */
	stiaddr->nb_int_msk |= SCS_INT_TAPE;	/* enable SCSI interrupts */

	/* the reset of the SCSI bus may cause an interrupt */
	staddr->scs_inicmd = SCS_INI_RST;	/* reset the SCSI bus */
	DELAY(100000);

	/* clear RST */
	staddr->scs_inicmd = 0;

	/* make the controller interrupt */
	/* may not need this stuff, the reset may cause an interrupt */
	if((stiaddr->nb_int_reqclr&SCS_INT_TAPE) == 0) {	/* didn't cause intr */
		staddr->scs_selena = SCS_ID0;
		staddr->scs_outdata = (SCS_ID0 | SCS_ID1);
		staddr->scs_inicmd = (SCS_INI_SEL | SCS_INI_ENOUT);
		DELAY(100000);	/* may not need this */
		staddr->scs_inicmd = 0;
		staddr->scs_selena = 0;
	}
	stiaddr->nb_int_reqclr = SCS_INT_TAPE;	/* clear SCSI in INT_REQ reg */

#ifdef STDEBUG
	if (cvec && cvec == 0x200)	/* check for interrupt */
		printd6("stprobe: controller didn't interrupt\n");
	else
		printd6("stprobe: controller found at vector %o\n", cvec);
#endif STDEBUG
	return(1);

	/*
	 * Determine if the drive exists, and is a TK50.
	 * We can get away with using a dev of zero in the 
	 * stcommand because it is not checked anywhere in
	 * this driver.
	 */
}

/*  */
stattach(ui)
	register struct uba_device *ui;
{
	register struct uba_ctlr *um;

	ui->ui_flags = 0;
	/*
	 * Link the drive buffer to the controller queue and
	 * leave it there since there is only one drive.  We'll
	 * check for outstanding requests queued for the drive.
	 */
	um = ui->ui_mi;
	um->um_tab.b_actf = &stutab[0];
	um->um_tab.b_actl = &stutab[0];
	um->um_tab.b_actf->b_actf = NULL;
}

/*  */
stslave(ui)
	register struct uba_device *ui;
{
	register struct st_softc *sc;
	register int s = spl5();

	sc = &st_softc[0];
	bcopy(DEV_TK50, sc->sc_device, strlen(DEV_TK50));
	splx(s);
	if (ui->ui_slave)
		return(0);
	return(1);
}

/*  */
stopen(dev, flag)
	register dev_t dev;
	register int flag;
{
	register struct uba_device *ui;
	register struct st_softc *sc;
	int unit = UNIT(dev);
	int retry = 0;
	int retval;

#ifdef STDEBUG
	ST_TRACK('o');
#endif STDEBUG
	if (unit >= nNST || (sc = &st_softc[unit])->sc_openf ||
	    (ui = stdinfo[unit]) == 0 || ui->ui_alive == 0) {
		return (ENXIO);
	}
	/*
	 * This is a strange use of the FNDELAY flag.  It
	 * is here to allow the installation finder program
	 * to open the device when the tape cartridge is not
	 * inserted.  The installation finder program needs
	 * to do an ioctl, so open must succeed whether or
	 * not a cartridge is present.
	 */

	/* Clear all flags so driver does not lock up after hard error */
	sc->sc_flags = 0;
	sc->sc_category_flags = 0;
	sc->sc_stflags &= ~ST_NODEVICE;
	sc->sc_sns.errclass = 0;
	/* this allows the mechanism that sends the commands to the 
	 * drive to be able to tell that these commands are coming
	 * from the open routine and need to be handled in a special
	 * way.
	 */
	st_from_open++;
	do {
	    sc->sc_sns.snskey = 0;
	  if (unit_attn) {
	    sc->sc_stflags |= st_rcvdiag(dev);
	  }
	  if (!(sc->sc_stflags & ST_NODEVICE)) {
	    stcommand(dev, ST_TUR, 1);
	    if(sc->sc_stflags & ST_NEED_SENSE) {
	    stcommand(dev, ST_RQSNS, 1);
	    switch(retval = sterror(sc)) {
		case ST_SUCCESS:
		    break;

		case ST_RETRY:
		    timeout(wakeup, (caddr_t)sc, hz/2);
		    sleep((caddr_t)sc, PZERO + 1);
		    retry++;
		    if (!(sc->sc_flags & DEV_EOM)) {
			sc->sc_flags = 0;
		    }
		    break;

		case ST_FATAL:
		    if (!(sc->sc_flags & DEV_EOM)) {
			sc->sc_flags = 0;
		    }
		    if(flag & FNDELAY) {
			sc->sc_stflags |= ST_NODEVICE;
		    }
		    else {
			return(ENXIO);
		    }
		    break;
	    }
	    }
	    else {
		retval = ST_SUCCESS;
	    }
	  }
	  else {
	    retval = ST_SUCCESS;
	  }
	} while ((retry < 10) && (retval == ST_RETRY));
	if (retry >= 10) {
	    if (!(flag & FNDELAY)) {
	    	DEV_UGH(sc->sc_device, unit, "offline");
	    	return(EIO);
	    }
	}
	/*
	 * If ST_NODEVICE is not set, the device exists,
	 * and we want to do a ST_MODSEL command
	 */
	if (!(sc->sc_stflags & ST_NODEVICE)) {
	    stcommand(dev, ST_MODSEL, 1);
	}
	sc->sc_openf = 1;
	sc->sc_blkno = (daddr_t)0;
	sc->sc_nxrec = INF;
	st_from_open = 0;
	sc->sc_rate = ST_TIMERRATE;
	if ((sc->sc_timer & ST_TIMERON) == 0) {
	    sc->sc_timer = ST_TIMERON;
	    timeout(st_timer, 0, (sc->sc_rate * hz)); /* schedule timer starting */
	}
#ifdef STDEBUG
	ST_TRACK('O');
#endif STDEBUG
	return (0);
}

/*  */
int st_rcvdiag(dev)
	register dev_t dev;
{
	register struct st_softc *sc = &st_softc[0];
	int unit = UNIT(dev);
	int i;
	u_char *byteptr;

	byteptr = (u_char *)&sc->st_dat;
	/* zero the receive data area */
	for (i = 0; i < ST_RECDIAG_LEN; i++) {
		*byteptr++ = 0;
	}
	/*
	 * Issue this command twice, as the first one
	 * after powerup always fails.
	 */
	stcommand(dev, ST_RECDIAG, 1);
	stcommand(dev, ST_RECDIAG, 1);
	if (sc->st_dat.dat.recdiag.ctlr_selftest != 0) {
		printf("st0: controller failed selftest\n");
		return(ST_NODEVICE);
	}
	if (sc->st_dat.dat.recdiag.drv_selftest != 0 ) {
		printf("st0: drive failed selftest, code = 0x%x\n",
			sc->st_dat.dat.recdiag.drv_selftest);
		return(ST_NODEVICE);
	}
#ifdef STDEBUG
	printd("st_rcvdiag: Rev. %d\n", sc->st_dat.dat.recdiag.ctlr_fw_rev);
#endif STDEBUG
	if (sc->st_dat.dat.recdiag.ctlr_fw_rev < 32 ) {
		printf("st0: controller firmware rev %d not supported\n",
			sc->st_dat.dat.recdiag.ctlr_fw_rev);
		return(ST_NODEVICE);
	}
	/* clear unit_attn so that we won't call st_rcvdiag() on each open */
	unit_attn = 0;
	return(ST_SUCCESS);
}

/*  */
/*
 * This timer is started in stopen(), and "goes off" every 30 seconds.
 * Whenever the timer "goes off" sc->sc_ticks is incremented and the
 * timeout value is looked up in st_timetable[] and checked against the
 * number sc->sc_ticks.  If the number of ticks in st_timetable[] has
 * been exceeded, we assume either the TZK50 controller or the TK50
 * drive has failed to respond and we attempt to reset the controller
 * and drive and return all outstanding requests with an error.
 */
st_timer(unused)
int unused;
{
    register struct uba_ctlr *um = stdinfo[0]->ui_mi;
    register struct st_softc *sc = &st_softc[0];
    register struct buf *dp;
    register struct buf *bp;
    register struct nb1_regs *staddr = (struct nb1_regs *)qmem;
    register struct nb_regs *stiaddr = (struct nb_regs *)nexus;
    struct vsbuf *vs;
    struct vsdev *vd;
    int	i, s, zs, z;

    dp = &stutab[0];
    bp = dp->b_actf;
    vs = &vsbuf;
    vd = &vstapedev;

#ifdef STDEBUG
    printd("tick..");
#endif STDEBUG
    s = spl5();
    /*
     * if the drive is not open, return.
     */
    if (sc->sc_openf == 0) {
#ifdef STDEBUG
	printd("\n");
#endif STDEBUG
	sc->sc_timer = 0;
	sc->sc_ticks = 0;
	splx(s);
	return;
    }
    if (dp->b_actf == dp) {
	sc->sc_timer = ST_TIMERON;
#ifdef STDEBUG
	printd(" No work\n");
#endif STDEBUG
    }
    else {
	sc->sc_ticks++;
	if (sc->sc_ticks > (char)st_timetable[sc->sc_curcmd]) {
	    sc->sc_ticks = 0;
#ifdef STDEBUG
		printd("st_timer: sc->sc_curcmd = 0x%x\n", sc->sc_curcmd);
		if (sc->sc_selstat == ST_DISCONN) {
		    printd("st0: waiting for reselect...\n");
		}
		else {
		    printd("st0: timeout -- What to do now?\n");
		    st_dump_softc(sc);
		}
#endif STDEBUG
		/* call st_start with ST_RESET to reset the bus */ 
		sc->sc_stflags |= ST_ENCR_ERR;
		sc->sc_xstate = ST_ERR;
		sc->sc_xevent = ST_RESET;
		/*
		 * If the tape doesn't own the RAM buffer, call vs_bufctl
		 * to have it allocated
		 */
		if (vs->vs_active != vd) {
		    vd->vsd_action = VS_ALLOC;
		    vd->vsd_funcptr = st_start;
		    vs_bufctl(vd);
#ifdef STDEBUG
		    ST_TRACK('*');
#endif STDEBUG
		    return;
		}
		else {
		    st_start(sc);
		    /*
		     * Make sure that the tape isn't hanging the system
		     * by keeping the RAM buffer
		     */
		    if (vs->vs_active == vd) {
			vd->vsd_action = VS_DEALLOC;
			vs_bufctl(vd);
		    }
	    }
	}
#ifdef STDEBUG
	else {
	    printd(" work\n");
	}
#endif STDEBUG
    }
    /* queue another watch dog timer */
    sc->sc_timer = ST_TIMERON;
    timeout(st_timer, 0, (sc->sc_rate * hz));
    splx(s);
    return;
}

/*  */
stclose(dev, flag)
	register dev_t dev;
	register int flag;
{
	register struct st_softc *sc = &st_softc[0];
	register int unit = 0;
	register int sel = SEL(dev);

#ifdef STDEBUG
	ST_TRACK('c');
#endif STDEBUG
	sc->sc_flags &= ~DEV_EOM;

	if (flag == FWRITE || ((flag & FWRITE) &&
	    (sc->sc_flags & DEV_WRITTEN))) {
		stcommand(dev, ST_WFM, 2);
		sc->sc_flags &= ~DEV_EOM;
		stcommand(dev, ST_P_BSPACEF, 1);
		sc->sc_flags &= ~DEV_EOM;
	}
	/* if we need to rewind... */
	if ((sel == MTLR) || (sel == MTHR)) {
		stcommand(dev, ST_REWIND, 0);
	}

	sc->sc_openf = 0;
#ifdef STDEBUG
	ST_TRACK('C');
#endif STDEBUG
}

/*  */
stcommand(dev, com, count)
	register dev_t dev;
	register int com;
	register int count;
{
	register struct buf *bp = &cstbuf[UNIT(dev)];

#ifdef STDEBUG
	ST_TRACK('m');
#endif STDEBUG
	while (bp->b_flags & B_BUSY) {
		if(bp->b_bcount == 0 && (bp->b_flags & B_DONE))
			break;
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO);
	}
	/* Load the buffer.  The b_count field gets used to hold the command
	 * count.  The b_resid field gets used to hold the command mnermonic.
	 * These two fields are "known" to be "save" to use for this purpose.
	 * (Most other drivers also use these fields in this way.)
	 */
	bp->b_flags = B_BUSY|B_READ;
	bp->b_dev = dev;
	bp->b_command = com;
	bp->b_bcount = count;
	bp->b_blkno = 0;
	ststrategy(bp);
	/*
	 * In the case of rewind from close, don't wait.
	 * This is the only case where count can be 0.
	 */
	if (count == 0)
		return;

	iowait(bp);

	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);

	bp->b_flags &= B_ERROR;
#ifdef STDEBUG
	ST_TRACK('M');
#endif STDEBUG
}

/*  */
ststrategy(bp)
	register struct buf *bp;
{
	register struct uba_ctlr *um;
	register struct st_softc *sc = &st_softc[0];
	register struct buf *dp;
	register struct vsdev *vd;
	register int s;
	int unit = 0;

#ifdef STDEBUG
	ST_TRACK('t');
#endif STDEBUG
	vd = &vstapedev;
	if ((sc->sc_flags & DEV_EOM) && !((sc->sc_flags & DEV_CSE) ||
	    (dis_eot_st[unit] & DISEOT))) {
		bp->b_resid = bp->b_bcount;
		bp->b_error = ENOSPC;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}
#ifdef	TM_NO_FIX
	sc->sc_category_flags &= ~DEV_TPMARK;
#else
	if ((bp->b_flags&B_READ) && (bp->b_flags&B_RAWASYNC) && 
	    ((sc->sc_category_flags&DEV_TPMARK) ||
	    (sc->sc_flags&DEV_HARDERR))) {
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}
#endif	TM_NO_FIX

	bp->av_forw = NULL;
	/* 
	 * If ST_NODEVICE is set, the device was opened
	 * with FNDELAY, but the device didn't respond.
	 * We'll try again to see if the device is here,
	 * if it is not, return an error
	 */
	if (sc->sc_stflags & ST_NODEVICE) {
	    DEV_UGH(sc->sc_device,unit,"offline");
	    bp->b_resid = bp->b_bcount;
	    bp->b_error = ENOSPC;
	    bp->b_flags |= B_ERROR;
	    biodone(bp);
	    return;
	}
	um = stdinfo[unit]->ui_mi;
	s = spl5();
	dp = &stutab[unit];
	if (dp->b_actf == NULL)
		dp->b_actf = bp;
	else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;
	if (dp->b_active == 0) {
		dp->b_active = 1;
		sc->sc_xstate = ST_NEXT;
		sc->sc_xevent = ST_BEGIN;
		/*
		 * Call the routine that allocates the VAXSTAR 16K buffer
		 * that the disk controller and tape controller share.
		 * When the buffer is allocated to us (the tape driver)
		 * st_start will be called from the allocation routine
		 */
		vd->vsd_action = VS_ALLOC;
		vd->vsd_funcptr = st_start;
		vs_bufctl(vd);
	}
	splx(s);
#ifdef STDEBUG
	ST_TRACK('T');
#endif STDEBUG
}

/*  */
/*
 * Interrupts come through here
 */
stintr()
{
    register struct uba_ctlr *um = stdinfo[0]->ui_mi;
    register struct st_softc *sc = &st_softc[0];
    register struct buf *dp;
    register struct buf *bp;
    register struct nb1_regs *staddr = (struct nb1_regs *)qmem;
    register struct nb_regs *stiaddr = (struct nb_regs *)nexus;
    register struct vsbuf *vs;
    register struct vsdev *vd;
    char *stv;			/* virtual address of st page tables	      */
    struct pte *pte, *mpte;	/* used for mapping page table entries	      */
    char *bufp;
    unsigned v;
    int o, npf;
    struct proc *rp;
    u_char *byteptr;
    int i, s, retval, count;
    int sel_dly = 0;
    int	arb_tries;
    int cmdcnt;
    int complete;
    int	num_expected;

    vs = &vsbuf;
    vd = &vstapedev;
    dp = &stutab[0];
    bp = dp->b_actf;

#ifdef STDEBUG
	ST_TRACK('i');
#endif STDEBUG
    /*
     * Check to make sure the tape owns the 16k hardware
     * buffer before touching any device registers.  If
     * the tape doesn't have the buffer, request it.
     */
    if(vs->vs_status != VS_ST) {
	sc->sc_stflags &= ~ST_RETD_KEEP;
	vd->vsd_action = VS_ALLOC;
	vd->vsd_funcptr = stintr;
	vs_bufctl(vd);
#ifdef STDEBUG
	ST_TRACK('A');
#endif STDEBUG
	return;
    }

#ifdef STDEBUG
    printd6('T');
    if (stdebug > 6) {
	printd("stintr:\n");
	stdumpregs();
    }
#endif STDEBUG

    /*
     * If a disconnect, give the buffer back.  We don't
     * need to save any pointers because we only have
     * one drive, and will take up where we left off
     * when a reselect occurs.
     */
    if((staddr->scs_status & SCS_BSYERR) == SCS_BSYERR) {
#ifdef STDEBUG
	printd2("stintr: disconnecting...\n");
#endif STDEBUG
	sc->sc_selstat = ST_DISCONN;
	staddr->scs_mode = 0;
	i = staddr->scs_reset;
	if(sc->sc_stflags & ST_RETD_KEEP) {
	    sc->sc_stflags &= ~ST_RETD_KEEP;
	    vd->vsd_action = VS_DEALLOC;
	    vs_bufctl(vd);
#ifdef STDEBUG
	ST_TRACK('B');
#endif STDEBUG
	}
	else {
	    return(VS_DEALLOC);
	}
	return;
    }

    /*
     * If ST_DISCONN,  check for reselect.
     */
    if(sc->sc_selstat == ST_DISCONN) {
	if(((staddr->scs_curstat & (SCS_BSY|SCS_SEL|SCS_IO)) == (SCS_SEL|SCS_IO))) {
	    sc->sc_selstat = ST_RESELECT;
#ifdef STDEBUG
	    ST_TRACK('G');
	    printd2("stintr: reselecting...\n");
#endif STDEBUG
	    inicmd_tmp = SCS_INI_BSY;
	    staddr->scs_inicmd = inicmd_tmp;
	    STWAIT_WHILE(((staddr->scs_curstat & SCS_SEL) == SCS_SEL),10000,retval);
	    if(retval >= 10000) {
		mprintf("st0: device failed to reselect\n");
		goto abort;
	    }
	    inicmd_tmp &= ~SCS_INI_BSY;
	    staddr->scs_inicmd = inicmd_tmp;
	    sc->sc_selstat = ST_SELECT;
	}
    }

    /*
     * The target controls the bus phase. We wait for REQ and BSY
     * to be asserted on the bus before determining which phase
     * is active.  Once REQ and BSY are asserted, the appropiate action
     * is taken. 
     */
    complete = 0;
    do {
	STWAIT_WHILE((((staddr->scs_curstat & SCS_REQ) != SCS_REQ) ||
		((staddr->scs_curstat & SCS_BSY) != SCS_BSY)),1000000,retval);
	if (retval >= 1000000) {
#ifdef STDEBUG
	    printd("\t\tstintr: SCS_REQ or SCS_BSY failed to set\n");
	    stdumpregs();
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('H');
#endif STDEBUG
	    goto abort;
	}
	if (staddr->scs_status & SCS_PARERR) {
	    mprintf ("st0: parity error\n");
#ifdef STDEBUG
	    printd("st0: parity error\n");
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('I');
#endif STDEBUG
	    goto abort;
	}
	else if (staddr->scs_curstat & SCS_RST) {
	    mprintf ("st0: bus reset\n");
#ifdef STDEBUG
	    printd("st0: bus reset\n");
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('J');
#endif STDEBUG
	    goto abort;
	}
	i = staddr->scs_reset;
	stiaddr->nb_int_reqclr = SCS_INT_TAPE;
	/*
	 * Zero the scs_mode register to clear all 
	 * DMA status bits from the last DMA transfer
	 */
	staddr->scs_mode = 0;
	/*
	 * Turn off the SCS_INI_ENOUT driver
	 */
	inicmd_tmp &= ~SCS_INI_ENOUT;
	staddr->scs_inicmd = inicmd_tmp;
	/*
	 * Read the bus phase, set the phase to match.
	 */
	staddr->scs_tarcmd = ((staddr->scs_curstat & SCS_PHA_MSK) >> 2);
	switch (staddr->scs_curstat & SCS_PHA_MSK) {

	case SCS_DATAO:
#ifdef STDEBUG
	    printd2("stintr: SCS_DATAO:\n");
#endif STDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = ST_DATAO_PHA;
	    if (sc->st_opcode != ST_WRITE) {
		byteptr = (u_char *)&sc->sc_dat[0];
		if (sc->st_opcode == ST_MODSEL) {
		    cmdcnt = ST_MODSEL_LEN;
		}
		else {
		    if (sc->st_opcode == ST_INQ) {
			cmdcnt = ST_CMD_LEN;
		    }
		    else {
#ifdef STDEBUG
			printd ("stintr: SCS_DATAO: unknown command 0x%x\n",
			    sc->st_opcode);
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('K');
#endif STDEBUG
			goto abort;
		    }
		}
		inicmd_tmp = SCS_INI_ENOUT;
		staddr->scs_inicmd = inicmd_tmp;
		/*
		 * Send the command packet
		 */
#ifdef STDEBUG
		printd2("Data Output Packet: \n");		/* 1 of 3 */
#endif STDEBUG
		for ( ; (cmdcnt > 0); cmdcnt--) {
		    STWAIT_UNTIL(((staddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
		    if (retval >= 10000) {
#ifdef STDEBUG
			printd("stintr: SCS_DATAO: REQ not set\n");
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('L');
#endif STDEBUG
			goto abort;
		    }
#ifdef STDEBUG
		    printd2 (" %x", *byteptr);			/* 2 of 3 */
#endif STDEBUG
		    staddr->scs_outdata = *byteptr++;
		    inicmd_tmp |= SCS_INI_ACK;
		    staddr->scs_inicmd = inicmd_tmp;
		    STWAIT_UNTIL(((staddr->scs_curstat & SCS_REQ) != SCS_REQ),10000,retval);
		    if (retval >= 10000) {
#ifdef STDEBUG
			printd("stintr: SCS_DATAO: REQ didn't go false\n");
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('N');
#endif STDEBUG
			goto abort;
		    }
		    inicmd_tmp &= ~SCS_INI_ACK;
		    staddr->scs_inicmd = inicmd_tmp;
		}
#ifdef STDEBUG
		printd2 ("\n");					/* 3 of 3 */
#endif STDEBUG
		break;
	    }
	    /* Copy the data from the 16K buffer to memory 
	     * (user space or kernel space).  Will have to
	     * set up own page table entries when copying to
	     * user space.
	     */
	    /*
	     * If bytecount is too large, throw out the transfer
	     */
	    if (bp->b_bcount > 16384) {
		mprintf("st0: buffer too large\n");
		DEV_UGH(sc->sc_device,0,"buffer too large");
		bp->b_flags |= B_ERROR;
#ifdef STDEBUG
	ST_TRACK('P');
#endif STDEBUG
		goto abort;
		break;
	    }
	    /*
	     * set up the count;
	     */
	    if ((sc->sc_stflags & ST_WAS_DISCON) && (sc->sc_savcnt != 0)) {
		sc->sc_bcount = -(sc->sc_savcnt);
		staddr->scd_cnt = sc->sc_savcnt ;
#ifdef STDEBUG
		printd1("stintr#1: scd_cnt = 0x%x, sc_savcnt = 0x%x\n",
			staddr->scd_cnt, sc->sc_savcnt);
#endif STDEBUG
	    }
	    else {
		sc->sc_bcount = (short)bp->b_bcount;
		staddr->scd_cnt = (short)-(bp->b_bcount);
	    }
	    /*
	     * Map the user page tables to my page tables (mpte)
	     */
		if ((cpu == C_VAXSTAR) && cvs_exmode_on)
			stv = (char *)cvseddbmem + 0x10000;
		else
			stv = (char *)&staddr->nb_ddb[0];
		if ((bp->b_flags & B_PHYS) == 0) {
		    bufp = (char *)bp->b_un.b_addr;
		}
		else {
		    /*
	    	     * Map to user space
		     */
	    	    v = btop(bp->b_un.b_addr);
		    o = (int)bp->b_un.b_addr & PGOFSET;
		    npf = btoc(bp->b_bcount + o);
		    rp = (bp->b_flags&B_DIRTY) ? &proc[2] : bp->b_proc;
		    if (bp->b_flags & B_UAREA) {
			pte = &rp->p_addr[v];
		    }
		    else if (bp->b_flags & B_PAGET) {
			pte = &Usrptmap[btokmx((struct pte *)bp->b_un.b_addr)];
		    }
		    else {
			pte = vtopte(rp, v);
		    }
		    bufp = (char *)ST_bufmap + o;
		    mpte = (struct pte *)stbufmap;
		    for (i = 0; i < npf; i++) {
			if (pte->pg_pfnum == 0)
			    panic("st0: zero pfn in pte");
			*(int *)mpte++ = pte++->pg_pfnum | PG_V | PG_KW;
			mtpr(TBIS, (char *) ST_bufmap + (i * NBPG));
		    }
		    *(int *)mpte = 0;
		    mtpr(TBIS, (char *) ST_bufmap + (i * NBPG));
		}
		sc->sc_stflags |= ST_MAPPED;
		sc->sc_savstv = (long)stv;
		sc->sc_savbufp = (long)bufp;
	    /*
	     * If the DMA transfer was interrupted by a disconnect
	     * the remaining data to be transfered needs to be 
	     * reloaded into the 16K buffer, and then another
	     * DMA transfer started.  This only needs to be done
	     * on a write DMA transfer.
	     */
	    if ((sc->sc_stflags & ST_WAS_DISCON) && (sc->sc_savcnt != 0)) {
		/*
		 * This is a quick way to subtract the the number of
		 * bytes not transfered before the disconnect, from
		 * the total number of bytes to transfer.  The result
		 * is the number of bytes that need to be transfered.
		 * sc->sc_savcnt is in two's complement form.  The
		 * number of bytes to be transfered is used as an offset
		 * and added to bufp to get the address of where to start
		 * transfering data from.
		 */
		sc->sc_stflags &= ~ST_WAS_DISCON;
		bufp =  bufp + (((short)bp->b_bcount + sc->sc_savcnt) & 0xffff);
		bcopy (bufp, stv, -(sc->sc_savcnt));
#ifdef STDEBUG
		printd1("stintr: bufp offset = 0x%x, count = 0x%x\n",
			(((short)bp->b_bcount + sc->sc_savcnt) & 0xffff),
			-(sc->sc_savcnt));
#endif STDEBUG
	    }
	    else {
		bcopy (bufp, stv, bp->b_bcount);
	    }
	    /*
	     * Enable interrupts, and DMA mode.  
    	     */
	    staddr->scs_mode = (SCS_DMA | SCS_INTEOP | SCS_PARCK | SCS_INTPAR);
	    inicmd_tmp = SCS_INI_ENOUT;
	    staddr->scs_inicmd = inicmd_tmp;
	    staddr->scd_dir = SCD_DMA_OUT;
	    /*
	     * Set the starting address in the 16K buffer.  This
	     * is really an offset into the buffer.  It needs to
	     * be written twice.  Whatever is in bits <7:0> gets
	     * moved into bits <15:8> with the bits <15:14> zeroed,
	     * and what ever you write into this register get put
	     * into bits <7:0>.  Therefore, the first write contains
	     * the bits destined for <15:8>, and the second write
	     * contains the bits for <7:0>.
	     */
	    if ((cpu == C_VAXSTAR) && cvs_exmode_on) 
		staddr->nb_scd_adr.l = 0x10000;
	    else {
		staddr->nb_scd_adr.c[0] = 0;
		staddr->nb_scd_adr.c[0] = 0;
	    }
	    /*
	     * Start the DMA transfer
	     */
	    staddr->scs_dmasend = 1;
	    sc->sc_stflags |= (ST_DID_DMA | ST_RETD_KEEP);
	    return(VS_KEEP);
	    break;

	case SCS_DATAI:
#ifdef STDEBUG
	    printd2("stintr: SCS_DATAI:\n");
#endif STDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = ST_DATAI_PHA;
	    if (sc->st_opcode != ST_READ) {
		byteptr = (u_char *)&sc->sc_sns;
		switch (sc->st_opcode) {
		    case ST_RQSNS:
			num_expected = ST_RQSNS_LEN;
			break;

		    case ST_INQ:
			num_expected = ST_INQ_LEN;
			byteptr = (u_char *)&sc->st_dat;
			break;

		    case ST_MODSNS:
			num_expected = ST_MODSNS_LEN;
			byteptr = (u_char *)&sc->st_dat;
			break;

		    case ST_RECDIAG:
			num_expected = ST_RECDIAG_LEN;
			byteptr = (u_char *)&sc->st_dat;
			break;

		    default:
#ifdef STDEBUG
			printd("stintr: SCS_DATAI: unexpected command = %x\n",
				sc->st_opcode);
#endif STDEBUG
		    break;
		}
		inicmd_tmp = 0;
		staddr->scs_inicmd = inicmd_tmp;
		/*
		 * Receive the command packet
		 */
#ifdef STDEBUG
		printd2("Data In Packet:");				/* 1 of 3 */
#endif STDEBUG
		for ( ; (num_expected > 0); num_expected--) {
		    STWAIT_UNTIL(((staddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
		    if (retval >= 10000) {
#ifdef STDEBUG
			printd("stintr: SCS_DATAI: REQ not set\n");
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('Q');
#endif STDEBUG
			goto abort;
		    }
		    *byteptr++ = staddr->scs_curdata;
#ifdef STDEBUG
		    printd2(" %x", staddr->scs_curdata);		/* 2 of 3 */
#endif STDEBUG
		    inicmd_tmp |= SCS_INI_ACK;
		    staddr->scs_inicmd = inicmd_tmp;
		    STWAIT_UNTIL(((staddr->scs_curstat & SCS_REQ) != SCS_REQ),10000,retval);
		    if (retval >= 10000) {
#ifdef STDEBUG
			printd("stintr: SCS_DATAO: REQ didn't go false\n");
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('R');
#endif STDEBUG
			goto abort;
		    }
		    inicmd_tmp &= ~SCS_INI_ACK;
		    staddr->scs_inicmd = inicmd_tmp;
		}
#ifdef STDEBUG
		printd2("\n");					/* 3 of 3 */
#endif STDEBUG
		break;
	    }
	    /*
	     * Start of DMA code.
	     *
	     * set up the count;
	     */
	    if ((sc->sc_stflags & ST_WAS_DISCON) && (sc->sc_savcnt != 0)) {
		sc->sc_bcount = -(sc->sc_savcnt);
		staddr->scd_cnt = sc->sc_savcnt ;
#ifdef STDEBUG
		printd1("stintr#2: scd_cnt = 0x%x, sc_savcnt = 0x%x\n",
			staddr->scd_cnt, sc->sc_savcnt);
#endif STDEBUG
	    }
	    else {
		sc->sc_bcount = (short)bp->b_bcount;
		staddr->scd_cnt = (short)-(bp->b_bcount);
	    }
	    /*
	     * Enable interrupts, and DMA mode.  
	     */
	    staddr->scs_mode = (SCS_DMA | SCS_INTEOP | SCS_PARCK | SCS_INTPAR);
	    staddr->scs_inicmd = 0;
	    staddr->scd_dir = SCD_DMA_IN;
	    /*
	     * Set the starting address in the 16K buffer.  This
	     * is really an offset into the buffer.  It needs to
	     * be written twice.  Whatever is in bits <7:0> gets
	     * moved into bits <15:8> with the bits <15:14> zeroed,
	     * and what ever you write into this register get put
	     * into bits <7:0>.  Therefore, the first write contains
	     * the bits destined for <15:8>, and the second write
	     * contains the bits for <7:0>.
	     */
	    if ((cpu == C_VAXSTAR) && cvs_exmode_on) 
		staddr->nb_scd_adr.l = 0x10000;
	    else {
		staddr->nb_scd_adr.c[0] = 0;
		staddr->nb_scd_adr.c[0] = 0;
	    }
	    /*
	     * Start the DMA transfer
	     */
	    staddr->scs_dmaircv = 1;
	    sc->sc_stflags |= (ST_DID_DMA | ST_RETD_KEEP);
	    return(VS_KEEP);
	    break;

	case SCS_CMD:
#ifdef STDEBUG
	    printd2("stintr: SCS_CMD:\n");
#endif STDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = ST_CMD_PHA;
	    /*
	     * clear savecnt and stflags
	     */
	    sc->sc_stflags = 0;
	    sc->sc_savcnt = 0;
	    inicmd_tmp = SCS_INI_ENOUT;
	    staddr->scs_inicmd = inicmd_tmp;
	    byteptr = (u_char *)&sc->st_command;
#ifdef STDEBUG
	    if ((st_prev_cmd == ST_WFM) && (*byteptr != ST_SPACE)) {
		printd1("Didn't space back over second filemark\n");
	    }
	    st_prev_cmd = *byteptr;
	    printd2("cmd pkt: %x %x %x %x %x\n", *byteptr, *(byteptr + 1),
		*(byteptr + 2), *(byteptr + 3), *(byteptr + 4), *(byteptr + 5));
#endif STDEBUG
	    /*
	     * Send the command packet
	     */
	    for (cmdcnt = ST_CMD_LEN; (cmdcnt > 0); cmdcnt--) {
		STWAIT_UNTIL(((staddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
		if (retval >= 10000) {
#ifdef STDEBUG
		    printd("stintr: SCS_CMD: REQ not set\n");
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('S');
#endif STDEBUG
		    goto abort;
		}
#ifdef STDEBUG
		ST_CMDTRACK(*byteptr); 
#endif STDEBUG
		staddr->scs_outdata = *byteptr++;
		inicmd_tmp |= SCS_INI_ACK;
		staddr->scs_inicmd = inicmd_tmp;
		STWAIT_UNTIL(((staddr->scs_curstat & SCS_REQ) != SCS_REQ),10000,retval);
		if (retval >= 10000) {
#ifdef STDEBUG
		    printd("stintr: SCS_CMD: REQ didn't go false\n");
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('U');
#endif STDEBUG
		    goto abort;
		}
		inicmd_tmp &= ~SCS_INI_ACK;
		staddr->scs_inicmd = inicmd_tmp;
	    }
	    break;

	case SCS_STATUS:
#ifdef STDEBUG
	    printd2("stintr: SCS_STATUS:\n");
#endif STDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = ST_STATUS_PHA;
	    /*
	     * Save the residual count if the command is a read
	     * or a write.
	     */
	    if((sc->st_opcode==ST_READ)||(sc->st_opcode==ST_WRITE)) {
/* Fix for rev 45 TZK50 */
/*
		if ((sc->sc_prevpha == ST_DATAI_PHA) ||
		    (sc->sc_prevpha == ST_DATAO_PHA)) {
*/
		    stiaddr->nb_hltcod = 0;
		    sc->sc_resid = -(staddr->scd_cnt);
/*
		}
		else {
		    sc->sc_resid = bp->b_bcount;
		}
*/
	    }
	    STWAIT_UNTIL(((staddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
	    if (retval >= 10000) {
#ifdef STDEBUG
		printd("stintr: SCS_STATUS: REQ didn't set\n");
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('V');
#endif STDEBUG
		goto abort;
	    }
	    sc->sc_status = staddr->scs_curdata;
	    inicmd_tmp = SCS_INI_ACK;
	    staddr->scs_inicmd = inicmd_tmp;
#ifdef STDEBUG
	    printd2("stintr: SCS_STATUS: ppha = 0x%x, status = 0x%x, scd_cnt = 0x%x\n",
		sc->sc_prevpha, sc->sc_status, staddr->scd_cnt);
#endif STDEBUG
	    STWAIT_WHILE(((staddr->scs_curstat & SCS_REQ) == SCS_REQ),50000,retval);
	    if (retval >= 50000) {
#ifdef STDEBUG
		printd("stintr: SCS_STATUS: REQ didn't go false\n");
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('W');
#endif STDEBUG
		goto abort;
	    }
	    inicmd_tmp = 0;
	    staddr->scs_inicmd = inicmd_tmp;
	    /*
	     * Check the status
	     */
	    if (sc->sc_status != ST_GOOD) {
		sc->sc_stflags |= ST_NEED_SENSE;
	    }
	    break;

	case SCS_MESSO:
#ifdef STDEBUG
	    printd2("stintr: SCS_MESSO:\n");
#endif STDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = ST_MESSO_PHA;
	    sc->sc_stflags = 0;
	    /*
	     * Send the Identify message
	     */
	    if(staddr->scs_curstat & SCS_REQ) {
		staddr->scs_outdata = ST_ID_DIS;
		inicmd_tmp |= (SCS_INI_ENOUT | SCS_INI_ATN);
		staddr->scs_inicmd = inicmd_tmp;
		DELAY(2);
		inicmd_tmp &= ~SCS_INI_ATN;
		staddr->scs_inicmd = inicmd_tmp;
		inicmd_tmp |= SCS_INI_ACK;
		staddr->scs_inicmd = inicmd_tmp;
		STWAIT_WHILE((staddr->scs_curstat & SCS_REQ),100000,retval);
		if(retval >= 100000) {
#ifdef STDEBUG
		printd("stintr: SCS_MESSO: SCS_REQ failed to set\n");
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('X');
#endif STDEBUG
		    goto abort;
		}
		inicmd_tmp &= ~SCS_INI_ACK;
		staddr->scs_inicmd = inicmd_tmp;

		if (staddr->scs_status & SCS_PARERR) {
#ifdef STDEBUG
		printd("stintr: SCS_MESSO: parity error\n");
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('Y');
#endif STDEBUG
		    goto abort;
		}
	    }
	    break;

	case SCS_MESSI:
#ifdef STDEBUG
	    printd2("stintr: SCS_MESSI:\n");
#endif STDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = ST_MESSI_PHA;
	    sc->sc_message = staddr->scs_curdata;
	    inicmd_tmp |= SCS_INI_ACK;
	    staddr->scs_inicmd = inicmd_tmp;
	    STWAIT_WHILE(((staddr->scs_curstat & SCS_REQ) == SCS_REQ),1000000,retval);
	    if(retval >= 1000000) {
#ifdef STDEBUG
	    printd("stintr: SCS_MESSI: SCS_REQ failed to clear\n");
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('Z');
#endif STDEBUG
		goto abort;
	    }
	    inicmd_tmp &= ~SCS_INI_ACK;
	    staddr->scs_inicmd = inicmd_tmp;
	    switch (sc->sc_message) {
		case ST_CMDCPT:
		    /*
		     * The command has completed, release the bus.
		     */
		    /* disable interrupts */
		    stiaddr->nb_int_msk &= ~SCS_INT_TAPE;
		    inicmd_tmp = 0;
		    staddr->scs_inicmd = 0;
		    staddr->scs_mode = 0;
		    staddr->scs_tarcmd = 0;
		    staddr->scs_selena = 0;
		    /*
		     * clear SCS_PARERR, SCS_INTREQ, and SCS_BSYERR 
		     * in mode reg, and return 
		     */
		    /*
		     * clear any pending interrupts
		     */
		    stiaddr->nb_int_reqclr = SCS_INT_TAPE;
		    i = staddr->scs_reset;
		    sc->sc_selstat = ST_IDLE;
		    sc->sc_fstate = 0;
		    /*
		     * Call st_start to complete the transfer
		     */
		    sc->sc_stflags &= ~ST_DID_DMA;
		    st_start();
		    complete++;
		    break;
			
		case ST_SDP:
		    if (sc->sc_prevpha == ST_DATAO_PHA) {
			stiaddr->nb_hltcod = 0;
		    	sc->sc_savcnt = (short)staddr->scd_cnt - 1;
			sc->sc_stflags |= ST_WAS_DISCON;
		    }
/*
		    else {
			sc->sc_savcnt = 0;
		    }
*/
		    if (sc->sc_prevpha == ST_DATAI_PHA) {
			sc->sc_stflags |= ST_WAS_DISCON;
			stiaddr->nb_hltcod = 0;
			sc->sc_savcnt = (short)staddr->scd_cnt;
			/* Fix for rev 45 TZK50 */
			if (sc->sc_savcnt == 0)
			    sc->sc_stflags |= ST_COPY_DONE;
			    /*
			     * Map the user page tables to my page tables (mpte)
			     */
			    if ((cpu == C_VAXSTAR) && cvs_exmode_on)
				stv = (char *)cvseddbmem + 0x10000;
			    else
				stv = (char *)&staddr->nb_ddb[0];
			    if ((bp->b_flags & B_PHYS) == 0) {
				bufp = (char *)bp->b_un.b_addr;
			    }
			    else {
				/*
				 * Map to user space
				 */
				v = btop(bp->b_un.b_addr);
				o = (int)bp->b_un.b_addr & PGOFSET;
				npf = btoc(bp->b_bcount + o);
				rp = (bp->b_flags&B_DIRTY) ? &proc[2] : bp->b_proc;
				if (bp->b_flags & B_UAREA) {
				    pte = &rp->p_addr[v];
				}
				else if (bp->b_flags & B_PAGET) {
				    pte = &Usrptmap[btokmx((struct pte *)bp->b_un.b_addr)];
				}
				else {
				    pte = vtopte(rp, v);
				}
				bufp = (char *)ST_bufmap + o;
				mpte = (struct pte *)stbufmap;
				for (i = 0; i < npf; i++) {
				    if (pte->pg_pfnum == 0)
				    panic("st0: zero pfn in pte");
				    *(int *)mpte++ = pte++->pg_pfnum | PG_V | PG_KW;
				    mtpr(TBIS, (char *) ST_bufmap + (i * NBPG));
				}
				*(int *)mpte = 0;
				mtpr(TBIS, (char *) ST_bufmap + (i * NBPG));
			    }
			    sc->sc_stflags |= ST_MAPPED;
			    sc->sc_savstv = (long)stv;
			    sc->sc_savbufp = (long)bufp;
			/*
			 * This is a quick way to subtract the the number of
			 * bytes not transfered before the disconnect, from
			 * the total number of bytes to transfer.  The result
			 * is the number of bytes that need to be transfered.
			 * sc->sc_savcnt is in two's complement form.
			 */
			bufp = bufp + ((short)bp->b_bcount - sc->sc_bcount);
#ifdef STDEBUG
				printd1("stintr: count = 0x%x\n",
			    (((short)sc->sc_bcount + sc->sc_savcnt) & 0xffff));
#endif STDEBUG
			bcopy (stv, bufp,
			    (((short)sc->sc_bcount + sc->sc_savcnt) & 0xffff));
#ifdef STDEBUG
			if (stdebug > 5) {
			    int j;
			    char *tmpstv;
			    tmpstv = stv;
			    printd("b_bcount = %d, bytes to read = %d\n",
				bp->b_bcount,
				(((short)bp->b_bcount + sc->sc_savcnt) & 0xffff));
			    for (j = 0; j < (((short)bp->b_bcount + sc->sc_savcnt) & 0xffff); j++) {
				printd("%c", *tmpstv++);
			    }
			    printd("\n");
			}
#endif STDEBUG
		    }
#ifdef STDEBUG
		    printd1("stintr: save data pointers, scd_cnt = 0x%x\n",
			sc->sc_savcnt);

#endif STDEBUG
		    break;

		case ST_DISCON:
#ifdef STDEBUG
		    printd2("stintr: disconnect message\n");
#endif STDEBUG
		    staddr->scs_mode = (SCS_MONBSY|SCS_PARCK|SCS_INTPAR);
#ifdef STDEBUG
	ST_TRACK('4');
#endif STDEBUG
		    sc->sc_stflags |= ST_RETD_KEEP;
		    return(VS_KEEP);
		    break;
			
		case ST_ID_NODIS:
#ifdef STDEBUG
		    printd2("stintr: identify message\n");
#endif STDEBUG
		    break;

		case ST_ID_DIS:
#ifdef STDEBUG
		    printd2("stintr: identify w/disconnect message\n");
#endif STDEBUG
		    break;

		default:

#ifdef STDEBUG
		    printd("stintr: MESSI: message = 0x%x\n", sc->sc_message);
#endif STDEBUG
#ifdef STDEBUG
	ST_TRACK('5');
#endif STDEBUG
		    goto abort;
	    }
	    break;

	default:
	    ;
#ifdef STDEBUG
	    mprintf("st0: unknown phase = 0x%x\n",
		staddr->scs_status & SCS_PHA_MSK);
#endif STDEBUG
	}
    } /* end of do loop */
    while (complete == 0);
    /*
     * If a VS_KEEP was returned, at command completion we
     * need to call vs_bufctl with either VS_DEALLOC, or
     * VS_WANTBACK.  If we haven't returned a VS_KEEP,
     * we just return VS_DEALLOC or VS_WANTBACK.
     */
    if (sc->sc_stflags & ST_RETD_KEEP) {
	sc->sc_stflags &= ~ST_RETD_KEEP;
	if (dp->b_actf == NULL) {
	    vd->vsd_action = VS_DEALLOC;
	}
	else {
	    vd->vsd_funcptr = st_start;
	    vd->vsd_action = VS_WANTBACK;
	}
	vs_bufctl(vd);
    }
    else {
	if (dp->b_actf == NULL) {
	    return(VS_DEALLOC);
	}
	else {
	    vd->vsd_funcptr = st_start;
	    return(VS_WANTBACK);
	}
    }
    return;

abort:
    /*
     * Too much is wrong, we cannot recover.  The transfer will be 
     * aborted by setting sc_xstate to ST_ERR and sc_xevent to ST_RESET.
     * st_start will be called to rundown the request queue, marking
     * each buffer with B_ERROR, and call st_reset to reset the
     * SCSI bus.
     */
#ifdef STDEBUG
    printd ("stintr: ST_ABORT: fevent = 0x%x\n", sc->sc_fevent);
    stdumpregs();
#endif STDEBUG
    mprintf("st0: aborting transfer\n");
    sc->sc_stflags |= ST_ENCR_ERR;
    sc->sc_xstate = ST_ERR;
    sc->sc_xevent = ST_RESET;
    st_start(sc);
    if (dp->b_actf != NULL) {
	printf("st0: stintr: queue not empty after reset");
    }
    /*
     * If a VS_KEEP was returned, at command completion we
     * need to call vs_bufctl with either VS_DEALLOC, or
     * VS_WANTBACK.  If we haven't returned a VS_KEEP,
     * we just return VS_DEALLOC or VS_WANTBACK.
     */
    if (sc->sc_stflags & ST_RETD_KEEP) {
	sc->sc_stflags &= ~ST_RETD_KEEP;
	vd->vsd_action = VS_DEALLOC;
	vs_bufctl(vd);
    }
    else {
	return(VS_DEALLOC);
    }
}

/*  */
stread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int unit = 0;

	return (physio(ststrategy, &rstbuf[unit], dev, B_READ,
		minphys, uio));
}

/*  */
stwrite(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register int unit = 0;

	return (physio(ststrategy, &rstbuf[unit], dev, B_WRITE,
		minphys, uio));
}

/*  */
stioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register struct uba_device *ui = stdinfo[0];
	register struct st_softc *sc = &st_softc[UNIT(dev)];
	register struct buf *bp = &cstbuf[UNIT(dev)];
	register int callcount;
	register int fcount;
	struct mtop *mtop;
	struct mtget *mtget;
	struct devget *devget;
	int unit = 0;

	/* we depend of the values and order of the MT codes here */
	static stops[] = { ST_WFM,ST_P_FSPACEF,ST_P_BSPACEF,ST_P_FSPACER,
			   ST_P_BSPACER,ST_REWIND,ST_UNLOAD,ST_P_CACHE,
			   ST_P_NOCACHE,ST_RQSNS };

	switch (cmd) {

	case MTIOCTOP:				/* tape operation */
		/* 
		 * If ST_NODEVICE is set, the device was opened
		 * with FNDELAY, but the device didn't respond.
		 * We'll try again to see if the device is here,
		 * if it is not, return an error
		 */
		if (sc->sc_stflags & ST_NODEVICE) {
		    DEV_UGH(sc->sc_device,unit,"offline");
		    return(ENXIO);
		}
		mtop = (struct mtop *)data;
		switch (mtop->mt_op) {

		case MTWEOF:
			callcount = 1;
			fcount = mtop->mt_count;
			break;

		case MTFSF: case MTBSF:
			callcount = 1;
			fcount = mtop->mt_count;
			break;

		case MTFSR: case MTBSR:
			callcount = 1;
			fcount = mtop->mt_count;
			break;

		case MTREW: case MTOFFL:
			sc->sc_flags &= ~DEV_EOM;
			callcount = 1;
			fcount = 1;
			break;

		case MTNOP:
			return(0);

		case MTCACHE:
		case MTNOCACHE:
			return(ENXIO);

		case MTCSE:
#ifndef	TM_NO_FIX
			sc->sc_category_flags &= ~DEV_TPMARK;
#endif	TM_NO_FIX
			sc->sc_flags |= DEV_CSE;
			return(0);

		case MTCLX: case MTCLS:
			return(0);

		case MTENAEOT:
			dis_eot_st[unit] = 0;
			return(0);

		case MTDISEOT:
			dis_eot_st[unit] = DISEOT;
			sc->sc_flags &= ~DEV_EOM;
			return(0);

		default:
			return (ENXIO);
		}
		if (callcount <= 0 || fcount <= 0)
			return (EINVAL);
		while (--callcount >= 0) {
			stcommand(dev, stops[mtop->mt_op], fcount);
			if (bp->b_flags & B_ERROR)
				break;
		}
		return (geterror(bp));

	case MTIOCGET:				/* tape status */
		mtget = (struct mtget *)data;
		mtget->mt_dsreg = 0;
		mtget->mt_erreg = 0;
		mtget->mt_resid = sc->sc_resid;
		mtget->mt_type = MT_ISST;
		break;

	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		devget->category = DEV_TAPE;
		devget->bus = DEV_NB;
		bcopy(DEV_VS_TAPE,devget->interface,strlen(DEV_VS_TAPE));
		bcopy(DEV_TK50,devget->device,strlen(DEV_TK50));
		devget->adpt_num = 0;
		devget->nexus_num = 0;
		devget->bus_num = 0;
		devget->ctlr_num = 0;
		devget->slave_num = 0;
		bcopy("st", devget->dev_name, 3);
		devget->unit_num = 0;
		devget->soft_count = sc->sc_softcnt;
		devget->hard_count = sc->sc_hardcnt;
		devget->stat = sc->sc_flags;
		devget->category_stat = (sc->sc_category_flags | DEV_6666BPI);
		break;

	default:
		return (ENXIO);
	}
	return (0);
}

/*  */
stdump()
{
}

/*  */
/*
 * Build a packet that is then passed one byte at a time
 * to the SCSI device.
 */
st_bldpkt(sc, cmd, count)
register struct st_softc *sc;
long	cmd;
long	count;
{
	u_char *byteptr;
	int i;

	/*
	 * Get a byte address of the begining of sc->st_cmd space,
	 * and clear the data space
	 */
	byteptr = (u_char *)&sc->st_command;
	for (i=0; i < ST_MAX_CMD_LEN; i++)
		*byteptr++ = 0;
#ifdef STDEBUG
	if (sc->sc_flags & DEV_CSE) {
	printd1("st_bldpkt: command = 0x%x\n", cmd);
	}
#endif STDEBUG
	/* build the comand packet */
	switch (cmd) {
		case ST_READ:
		case ST_WRITE:
		case ST_RBD:
			sc->st_read.fixed = 0;	/* only records */
			sc->st_read.xferlen2 = LTOB(count,2);
			sc->st_read.xferlen1 = LTOB(count,1);
			sc->st_read.xferlen0 = LTOB(count,0);
			break;
		case ST_RQSNS:
			sc->st_rqsns.alclen = ST_RQSNS_LEN;
			break;
		case ST_MODSNS:
			sc->st_modsns.alclen = ST_MODSNS_LEN;
			break;
		case ST_INQ:
			sc->st_rqsns.alclen = ST_INQ_LEN;
			break;
		case ST_REWIND:
		case ST_RBL:
		case ST_TUR:
			break;
		case ST_WFM:
			sc->st_wfm.numoffmks2 = 0; /* max of 65536 file marks */
			sc->st_wfm.numoffmks1 = LTOB(count,1);
			sc->st_wfm.numoffmks0 = LTOB(count,0);
			break;
		case ST_P_BSPACEF:
			count = -count;
		case ST_P_FSPACEF:
			cmd = ST_SPACE;
			sc->st_space.code = 1;
			sc->st_space.count2 = LTOB(count,2);
			sc->st_space.count1 = LTOB(count,1);
			sc->st_space.count0 = LTOB(count,0);
			break;
		case ST_P_BSPACER:
			count = -count;
		case ST_P_FSPACER:
			cmd = ST_SPACE;
			sc->st_space.code = 0;
			sc->st_space.count2 = LTOB(count,2);
			sc->st_space.count1 = LTOB(count,1);
			sc->st_space.count0 = LTOB(count,0);
			break;
		case ST_VFY:
			sc->st_vfy.fixed = 1;	/* only records */
			sc->st_vfy.bytcmp = 0;	/* CRC verify only */
			sc->st_vfy.verflen2 = LTOB(count,2);
			sc->st_vfy.verflen1 = LTOB(count,1);
			sc->st_vfy.verflen0 = LTOB(count,0);
			break;
		case ST_ERASE:
			sc->st_erase.longbit = 0; /* don't erase to end
							of tape */
			break;
		case ST_UNLOAD:
			sc->st_load.load = 0;  /* unload only for now */
			sc->st_load.reten = 0; /* not used on MAYA    */
			break;
		case ST_RECDIAG:
			sc->st_recdiag.aloclen1 = 0;
			sc->st_recdiag.aloclen0 = ST_RECDIAG_LEN;
			break;
		case ST_SNDDIAG:
			return(ST_RET_ERR);
			break;
		case ST_MODSEL:
		case ST_P_CACHE:
			cmd = ST_MODSEL;
			sc->st_modsel.pll = 0x0e;
			sc->st_modsel.bufmode = 1;
			sc->st_modsel.rdeclen = 0x08;
			sc->st_modsel.vulen = 1;
#ifdef STDEBUG
			sc->st_modsel.vu7 = st_direct_track_mode;
#endif STDEBUG
			sc->st_modsel.notimo = 1;
			sc->st_modsel.nof = st_max_numof_fills;
			break;
		case ST_P_NOCACHE:
			cmd = ST_MODSEL;
			sc->st_modsel.pll = 0x0e;
			sc->st_modsel.bufmode = 0;
			sc->st_modsel.rdeclen = 0x08;
			sc->st_modsel.vulen = 1;
			sc->st_modsel.nof = 0;
			break;
		case ST_TRKSEL:
			return(ST_RET_ERR);
			break;
		case ST_RESUNIT:
		case ST_RELUNIT:
#ifdef STDEBUG
 			printd("st_bldpkt: unimplemented command 0x%x\n",
 				cmd);
#endif STDEBUG
			return (ST_RET_ERR);
			break;
				
		default:
#ifdef STDEBUG
			printd("st_bldpkt: unknown command = 0x%x\n",
				cmd);
#endif STDEBUG
			return (ST_RET_ERR);
			break;
	}
	/* The only unit on the VAXSTAR is logical unit zero.
	 * 
	 * st_read is used here to get to the fields in the 
	 * structure for all commands.
	 */
	sc->st_read.lun = 0;
	sc->st_read.link = 0;
	sc->st_read.flag = 0;
	sc->st_read.mbz = 0;
	sc->st_opcode = cmd;
	return(ST_SUCCESS);
}
/**/
st_start()
{
    register struct nb1_regs *staddr = (struct nb1_regs *)qmem;
    register struct nb_regs *stiaddr = (struct nb_regs *)nexus;
    register struct buf *bp;
    register struct buf *dp;
    register struct st_softc *sc;
    register struct vsdev *vd;
    struct uba_ctlr *um;
    struct uba_device *ui;
    char *stv;			/* virtual address of st page tables	      */
    struct pte *pte, *mpte;	/* used for mapping page table entries	      */
    char *bufp;
    unsigned v;
    int o, npf;
    struct proc *rp;
    int fuz;			/* return value from a call to fuzzy()	      */
    int ret;			/* return value from various routines	      */
    int sp_ret;
    int i, tmpstflags, tmpresid, tmpstat;
    short count;
    u_char *byteptr;
	
    vd = &vstapedev;
    um = stdinfo[0]->ui_mi;
    dp = &stutab[0];
    sc =  &st_softc[0];
    bp = dp->b_actf;

    /*
     * st_start is one big state machine.  The states are kept
     * in the st_softc structure.  The state variables that 
     * are used in this state machine are:
     *
     *	sc->sc_xstate	used to dispatch to major states
     *	sc->sc_xevent	used to dispatch to minor (sub)states
     */

    for (;;) {	/* forever */
#ifdef STDEBUG
	printd2("st_start: state = %s, event = %s\n",
	    trace_xstate[sc->sc_xstate], trace_event[sc->sc_xevent]);
#endif STDEBUG
    switch(sc->sc_xstate) {
/**/
	case ST_NEXT:
	    switch(sc->sc_xevent) {
		case ST_CONT:
		    /*
		     * Remove the completed request from the queue,
		     * return the buffer, return.  We will be called
		     * back from vs_bufctl() when the vaxstar buffer
		     * is allocated to us.
		     */
		    bp = dp->b_actf;
		    dp->b_actf = bp->av_forw;
		    bp->b_resid = sc->sc_resid;
#ifdef STDEBUG
		    printd1("resid = %d\n", bp->b_resid);
#endif STDEBUG
		    sc->sc_flags |= DEV_DONE;
		    biodone(bp);
		    sc->sc_xevent = ST_BEGIN;
		    st_retries = 0;
		    /*
		     * The comand has ended, reset the timer to zero
		     */
		    sc->sc_ticks = 0;
		    if(dp->b_actf == NULL) {
			dp->b_active = 0;
#ifdef STDEBUG
	ST_TRACK('@');
#endif STDEBUG
			return(VS_DEALLOC);
		    }
		    else {
#ifdef STDEBUG
	ST_TRACK('#');
#endif STDEBUG
	    		vd->vsd_funcptr = st_start;
			return(VS_WANTBACK);
		    }
		    break;

		case ST_BEGIN:
		    sc->sc_stflags &= ~ST_MAPPED;
		    if ((bp = dp->b_actf) == NULL) {
			dp->b_active = 0;
#ifdef STDEBUG
	ST_TRACK('$');
#endif STDEBUG
			return(VS_DEALLOC);
		    }
		    if((sc->sc_flags & DEV_EOM) && !((sc->sc_flags & DEV_CSE) ||
			(dis_eot_st[0] & DISEOT))) {
			bp->b_resid = bp->b_bcount;
			bp->b_error = ENOSPC;
			bp->b_flags |= B_ERROR;
			sc->sc_xevent = ST_CONT;
			break;
		    }
#ifdef	TM_NO_FIX
		    if((sc->sc_category_flags & DEV_TPMARK) &&
			(bp->b_flags & B_READ)) {
			bp->b_resid = 0;
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
			sc->sc_xevent = ST_CONT;
			break;
		    }
#endif	TM_NO_FIX
		    if((bp->b_flags&B_READ) && (bp->b_flags&B_RAWASYNC) && 
			((sc->sc_category_flags&DEV_TPMARK) ||
			(sc->sc_flags&DEV_HARDERR))) {
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
			sc->sc_xevent = ST_CONT;
			break;
		    }

		    /* log progress */
		    sc->sc_progress = time;

		    if (bp == &cstbuf[0]) {
			/* 
			 * execute control operation with the specified count
			 */
			if (bp->b_command == ST_REWIND) {
			    um->um_tab.b_active = SREW;
			    sc->sc_flags &= ~DEV_EOM;
			}
			else {
			    um->um_tab.b_active = SCOM;
			}
	    	    	sc->sc_xstate = ST_SP_START;
			sc->sc_xevent = ST_CONT;
	    	    	break;
		    }
		    /*
		     * If it's not a control operation, it must be
		     * data.
		     */
		    sc->sc_xstate = ST_RW_START;
		    sc->sc_xevent = ST_CONT;
		    break;

		default:
			;
#ifdef STDEBUG
		    printd("st_start: ST_NEXT: state = %s, event = %s\n",
			trace_xstate[sc->sc_xstate], trace_event[sc->sc_xevent]);
#endif STDEBUG
	    }
	    break;

/*  */
	case ST_SP_START:
	    if (sc->st_opcode != ST_RQSNS) {
		sc->sc_flags &= ~DEV_WRITTEN;
	    }
	    /*
	     * The write to b_resid has to follow the
	     * read of b_command. This is because both
	     * b_command and b_resid are the same field
	     * (overloaded).  The write to b_resid destroys
	     * the data in b_command.  This is a black
	     * hole waiting to be fallen into!
	     */
	    sc->sc_curcmd = bp->b_command;
	    bp->b_resid = 0;
	    st_bldpkt(sc, sc->sc_curcmd, bp->b_bcount);
	    switch (fuz = st_scsistart()) {
		case ST_SUCCESS:
		    sc->sc_xstate = ST_NEXT;
		    sc->sc_xevent = ST_CONT;
		    break;

		case ST_RET_ERR:
		/* 
		 * If this command was issued from stopen, don't
		 * do the ST_RQSNS if it fails
		 */
		if (!(st_from_open)) {
		    st_bldpkt(sc, ST_RQSNS, 0);
		    if(st_scsistart() == ST_SUCCESS) {
#ifdef STDEBUG
			if (stdebug > 1) {
			    stprintsense();
			}
#endif STDEBUG
			switch (sp_ret = sterror(sc)) {
			    case ST_SUCCESS:
				sc->sc_xstate = ST_NEXT;
				sc->sc_xevent = ST_CONT;
				break;

			    case ST_RETRY:
				sc->sc_xstate = ST_NEXT;
				sc->sc_xevent = ST_CONT;
				if ((st_retries < 3 ) && (sc->st_opcode != ST_TUR)) {
				    sc->sc_xevent = ST_BEGIN;
				    st_retries++;
				}
				else {
				    bp->b_flags |= B_ERROR;
				    sc->sc_xstate = ST_NEXT;
				    sc->sc_xevent = ST_CONT;
				}
				break;

			    case ST_FATAL:
		    		if((sc->sc_flags & DEV_EOM) &&
				    !((sc->sc_flags & DEV_CSE) ||
				    (dis_eot_st[0] & DISEOT))) {
				    bp->b_error = ENOSPC;
				}
				bp->b_flags |= B_ERROR;
				sc->sc_xstate = ST_NEXT;
				sc->sc_xevent = ST_CONT;
				break;

			    default:
#ifdef STDEBUG
				printd("st_start: ST_SP_CONT: unknown sp_ret = 0x%x\n",
					sp_ret);
#endif STDEBUG

				bp->b_flags = B_ERROR;
				sc->sc_xstate = ST_NEXT;
				sc->sc_xevent = ST_CONT;
				break;
			}
			if((sc->st_opcode == ST_RQSNS) || (sc->st_opcode == ST_SPACE)) {
				sc->sc_xevent = ST_CONT;
			}
			break;
		    }
		    else {
			sc->sc_xstate = ST_NEXT;
			sc->sc_xevent = ST_CONT;
			break;
		    }
		}
		else {
			sc->sc_xstate = ST_NEXT;
			sc->sc_xevent = ST_CONT;
			break;
		}
		break;

		case ST_IP:
		    sc->sc_xstate = ST_SP_CONT;
#ifdef STDEBUG
	ST_TRACK('%');
#endif STDEBUG
		    sc->sc_stflags |= ST_RETD_KEEP;
		    return(VS_KEEP);
		    break;

		default:
#ifdef STDEBUG
		    printd("st_start: ST_SP_START: return value = 0x%x\n", fuz);
#endif STDEBUG
		    bp->b_flags = B_ERROR;
		    sc->sc_xstate = ST_NEXT;
		    sc->sc_xevent = ST_CONT;
		    break;
	    }
	    break;
		
/*  */
	case ST_SP_CONT:
	    /*
	     * If this command is a ST_RQSNS, look at the
	     * status bytes and determine what to do next,
	     * otherwise process as ususal.
	     */
	    if (sc->st_opcode != ST_RQSNS) {
		if (sc->sc_stflags == ST_NORMAL) {
		    sc->sc_xstate = ST_NEXT;
		    sc->sc_xevent = ST_CONT;
		    break;
		}
		if (sc->sc_stflags & ST_NEED_SENSE) {
		    st_bldpkt(sc, ST_RQSNS, 0);
		    if(st_scsistart() == ST_SUCCESS) {
#ifdef STDEBUG
			if (stdebug > 1) {
			    stprintsense();
			}
#endif STDEBUG
			switch (sp_ret = sterror(sc)) {
			    case ST_SUCCESS:
#ifndef	TM_NO_FIX
				if ((sc->sc_category_flags & DEV_TPMARK) &&
				    ((sc->sc_curcmd == ST_P_FSPACER) ||
				     (sc->sc_curcmd == ST_P_BSPACER)))
					bp->b_flags |= B_ERROR;
#endif	TM_NO_FIX
				sc->sc_xstate = ST_NEXT;
				sc->sc_xevent = ST_CONT;
				break;

			    case ST_RETRY:
				sc->sc_xstate = ST_NEXT;
				sc->sc_xevent = ST_CONT;
				if ((st_retries < 3 ) && (sc->st_opcode != ST_TUR)) {
					sc->sc_xevent = ST_BEGIN;
					st_retries++;
				}
				else {
				    bp->b_flags |= B_ERROR;
				    sc->sc_xstate = ST_NEXT;
				    sc->sc_xevent = ST_CONT;
				}
				break;

			    case ST_FATAL:
				bp->b_flags |= B_ERROR;
				sc->sc_xstate = ST_NEXT;
				sc->sc_xevent = ST_CONT;
				break;

			    default:
#ifdef STDEBUG
				printd("st_start: ST_SP_CONT: unknown sp_ret = 0x%x\n",
					sp_ret);
#endif STDEBUG

				bp->b_flags = B_ERROR;
				sc->sc_xstate = ST_NEXT;
				sc->sc_xevent = ST_CONT;
				break;
			}
			if((sc->st_opcode == ST_RQSNS) || (sc->st_opcode == ST_SPACE)) {
				sc->sc_xevent = ST_CONT;
			}
			break;
		    }
		    else {
			sc->sc_xstate = ST_NEXT;
			sc->sc_xevent = ST_CONT;
			break;
		    }
		}
		else {
		    if ((sc->sc_stflags & ST_ENCR_ERR) == ST_ENCR_ERR) {
			bp->b_flags |= B_ERROR;
		    	sc->sc_xstate = ST_NEXT;
		    	sc->sc_xevent = ST_CONT;
			break;
		    }
		    else {
			sc->sc_xstate = ST_NEXT;
			sc->sc_xevent = ST_CONT;
			break;
		    }
		}
	    }

#ifdef STDEBUG
	    if (stdebug > 1) {
		stprintsense();
	    }
#endif STDEBUG
	    sc->sc_xstate = ST_NEXT;
	    sc->sc_xevent = ST_CONT;
	    break;

/**/
	case ST_RW_START:
	    /*
	     * If bytecount is too large, throw out the transfer
	     */
	    if (bp->b_bcount > 16384) {
		mprintf("st0: buffer too large\n");
		DEV_UGH(sc->sc_device,0,"buffer too large");
		bp->b_flags |= B_ERROR;
		sc->sc_xstate = ST_NEXT;
		sc->sc_xevent = ST_CONT;
		break;
	    }
	    if (bp->b_flags & B_READ) {
		sc->sc_curcmd = ST_READ;
		st_bldpkt(sc, sc->sc_curcmd, bp->b_bcount);
		sc->sc_xstate = ST_R_STDMA;
	    }
	    else {
		sc->sc_curcmd = ST_WRITE;
		st_bldpkt(sc, sc->sc_curcmd, bp->b_bcount);
		sc->sc_xstate = ST_W_STDMA;
	    }
	    break;

/**/
	case ST_R_COPY:
	    /* Copy the data from the 16K buffer to memory 
	     * (user space or kernel space).  Will have to
	     * set up own page table entries when copying to
	     * user space.
	     */
	    /*
	     * If bytecount is too large, throw out the transfer
	     */
	    if (bp->b_bcount > 16384) {
		mprintf("st0: buffer too large\n");
		DEV_UGH(sc->sc_device,0,"buffer too large");
		bp->b_flags |= B_ERROR;
		sc->sc_xstate = ST_NEXT;
		sc->sc_xevent = ST_CONT;
		break;
	    }
	    /*
	     * Map the user page tables to my page tables (mpte)
	     */
		if ((cpu == C_VAXSTAR) && cvs_exmode_on)
			stv = (char *)cvseddbmem + 0x10000;
		else
			stv = (char *)&staddr->nb_ddb[0];
		if ((bp->b_flags & B_PHYS) == 0) {
		    bufp = (char *)bp->b_un.b_addr;
		}
		else {
		    /*
	    	     * Map to user space
		     */
	    	    v = btop(bp->b_un.b_addr);
		    o = (int)bp->b_un.b_addr & PGOFSET;
		    npf = btoc(bp->b_bcount + o);
		    rp = (bp->b_flags&B_DIRTY) ? &proc[2] : bp->b_proc;
		    if (bp->b_flags & B_UAREA) {
			pte = &rp->p_addr[v];
		    }
		    else if (bp->b_flags & B_PAGET) {
			pte = &Usrptmap[btokmx((struct pte *)bp->b_un.b_addr)];
		    }
		    else {
			pte = vtopte(rp, v);
		    }
		    bufp = (char *)ST_bufmap + o;
		    mpte = (struct pte *)stbufmap;
		    for (i = 0; i < npf; i++) {
			if (pte->pg_pfnum == 0)
			panic("st0: zero pfn in pte");
			*(int *)mpte++ = pte++->pg_pfnum | PG_V | PG_KW;
			mtpr(TBIS, (char *) ST_bufmap + (i * NBPG));
		    }
		    *(int *)mpte = 0;
		    mtpr(TBIS, (char *) ST_bufmap + (i * NBPG));
	        }
		sc->sc_stflags |= ST_MAPPED;
		sc->sc_savstv = (long)stv;
		sc->sc_savbufp = (long)bufp;
	    if (sc->sc_stflags & ST_WAS_DISCON) {
		sc->sc_stflags &= ~ST_WAS_DISCON;
		/*
		 * Subtract the residual count from the number
		 * of bytes in the last DMA tranfer to the drive, 
		 * and add that to bufp to get the address to start
		 * tranfering data to.
		 */
#ifdef STDEBUG
		printd4("bufp = 0x%x\n", bufp);
#endif STDEBUG
		bufp = bufp + ((short)bp->b_bcount - sc->sc_bcount);
#ifdef STDEBUG
		printd4("newbufp = 0x%x, (offset 0x%x), count = %d\n",
			bufp, ((short)bp->b_bcount - sc->sc_bcount), 
			(sc->sc_bcount - sc->sc_resid));
#endif STDEBUG
		/* Fix for rev 45 TZK50 */
		if ((sc->sc_stflags & ST_COPY_DONE) == 0)
		    bcopy (stv, bufp, (sc->sc_bcount - sc->sc_resid));
	    }
	    else {
#ifdef STDEBUG
		printd1("stv = 0x%x, bufp = 0x%x, bcount = %d, resid = %d\n",
		    stv, bufp, bp->b_bcount, sc->sc_resid);
#endif STDEBUG
		bcopy (stv, bufp, (bp->b_bcount - sc->sc_resid));
	    }
#ifdef STDEBUG
	    if (stdebug > 5) {
		int j;
		char *tmpstv;
		if ((cpu == C_VAXSTAR) && cvs_exmode_on)
			tmpstv = (char *)cvseddbmem + 0x10000;
		else
			tmpstv = (char *)&staddr->nb_ddb[0];
		printd("b_bcount = %d, b_bcount - sc_resid = %d\n",
			bp->b_bcount, (bp->b_bcount - sc->sc_resid));
		for (j = 0; j < (bp->b_bcount - sc->sc_resid); j++) {
		    printd("%c", *tmpstv++);
		}
		printd("\n");
	    }
#endif STDEBUG
	    sc->sc_stflags &= ~ST_MAPPED;
	    sc->sc_xstate = ST_NEXT;
	    break;

/*  */
	case ST_R_STDMA:
	    sc->sc_flags &= ~DEV_WRITTEN;
	    bp->b_resid = 0;
	    switch (fuz = st_scsistart()) {
		case ST_SUCCESS:
		    sc->sc_xstate = ST_R_COPY;
		    sc->sc_xevent = ST_CONT;
		    break;

		case ST_IP:
		    sc->sc_xstate = ST_R_DMA;
		    sc->sc_stflags |= ST_RETD_KEEP;
		    return(VS_KEEP);
		    break;

		case ST_RET_ERR:
		    sc->sc_xstate = ST_R_DMA;
		    break;

		default:
		    ;
#ifdef STDEBUG
		    printd("st0: ST_R_STDMA: unknown return = 0x%x\n", fuz);
#endif STDEBUG
	    }
	    break;

/*  */
	case ST_R_DMA:
	    /*
	     * Determine if something went wrong that requires that
	     * a request sense command or a space command needs to be
	     * done here.  If a request sense needs to be done, send
	     * the command, and evaluate the result, and determine 
	     * if we need to return an error, return a short read,
	     * or retry the command.
	     */
	    if(sc->sc_stflags & ST_NEED_SENSE) {
		/*
		 * TODO: 
		 * Determine what to do if the ST_RQSNS fails
		 */
		/*
		 * Save sc_stflags, as SCS_MESSO: in st_scsistart clears
		 * sc_stflags, and if cleared, we won't know whether to
		 * return to, or callback vs_bufctl - resulting in 
		 * the driver hanging.
		 */
		tmpstflags = sc->sc_stflags;
		st_bldpkt(sc, ST_RQSNS, 0);
		tmpstat = st_scsistart();
		/* restore the previous value in sc_stflags */
		sc->sc_stflags = tmpstflags;
		if(tmpstat == ST_SUCCESS) {
#ifdef STDEBUG
		    if (stdebug > 1) {
			stprintsense();
		    }
#endif STDEBUG
		    switch(ret = sterror(sc)) {
			case ST_SUCCESS:
			    sc->sc_xstate = ST_R_COPY;
			    sc->sc_xevent = ST_CONT;
			    /* Fix rev 45 TZK50 */
			    /* return requested byte count when filemark read */
			    if (sc->sc_category_flags & DEV_TPMARK) {
				sc->sc_resid = bp->b_bcount;
				sc->sc_xstate = ST_NEXT;
			    }
			    break;

			case ST_RETRY:
			    if (st_retries < 10) {
#ifdef STDEBUG
				printd("retrying read\n");
#endif STDEBUG
				st_retries++;
				sc->sc_xstate = ST_NEXT;
				sc->sc_xevent = ST_BEGIN;
			    }
			    else {
				st_retries = 0;
				bp->b_flags |= B_ERROR;
			        sc->sc_xstate = ST_NEXT;
			        sc->sc_xevent = ST_CONT;
			    }
			    break;

			case ST_FATAL:
			    if((sc->sc_flags & DEV_EOM) &&
				(sc->sc_flags & DEV_CSE) &&
				(sc->sc_sns.eom)) {
				sc->sc_xstate = ST_R_COPY;
				sc->sc_xevent = ST_CONT;
				break;
			    }
			    if((sc->sc_flags & DEV_EOM) &&
				!((sc->sc_flags & DEV_CSE) ||
				(dis_eot_st[0] & DISEOT))) {
				bp->b_error = ENOSPC;
			    }
			    bp->b_flags |= B_ERROR;
			    sc->sc_xstate = ST_NEXT;
			    sc->sc_xevent = ST_CONT;
			    break;

			default:
#ifdef STDEBUG
			    printd("st_start: ST_R_DMA: unknown ret from sterror = 0x%x\n",
				ret);
#endif STDEBUG
			    bp->b_flags |= B_ERROR;
			    sc->sc_xstate = ST_NEXT;
			    sc->sc_xevent = ST_CONT;
			    break;
		    }
		}
		else {
		    bp->b_flags |= B_ERROR;
		    sc->sc_xstate = ST_NEXT;
		    sc->sc_xevent = ST_CONT;
		}
		break;
	    }
	    else {
		sc->sc_xstate = ST_R_COPY;
		sc->sc_xevent = ST_CONT;
	    }
	    break;

/*  */
	case ST_W_STDMA:
	    sc->sc_flags |= DEV_WRITTEN;
	    bp->b_resid = 0;
	    switch (fuz = st_scsistart()) {
		case ST_SUCCESS:
		    sc->sc_xstate = ST_NEXT;
		    sc->sc_xevent = ST_CONT;
		    break;

		case ST_IP:
		    sc->sc_xstate = ST_W_DMA;
#ifdef STDEBUG
	ST_TRACK('^');
#endif STDEBUG
		    sc->sc_stflags |= ST_RETD_KEEP;
		    return(VS_KEEP);
		    break;

		case ST_RET_ERR:
		    sc->sc_xstate = ST_W_DMA;
		    break;

		default:
		    ;
#ifdef STDEBUG
		    printd("st0: ST_W_STDMA: unknown return = 0x%x\n", fuz);
#endif STDEBUG
	    }
	    break;

/*  */
	case ST_W_DMA:
	    	/*
	    	 * Determine if something went wrong that requires that
	    	 * a request sense command or a space command needs to be
	    	 * done here.  If a request sense needs to be done, send
	    	 * the command, and evaluate the result, and determine 
	    	 * if we need to return an error, return a short read,
	    	 * or retry the command.
	    	 */
	    if(sc->sc_stflags & ST_NEED_SENSE) {
		/*
		 * TODO: 
		 * Determine what to do if the ST_RQSNS fails
		 */
		/*
		 * Save sc_stflags, as SCS_MESSO: in st_scsistart clears
		 * sc_stflags, and if cleared, we won't know whether to
		 * return to, or callback vs_bufctl - resulting in 
		 * the driver hanging.
		 */
		tmpstflags = sc->sc_stflags;
		st_bldpkt(sc, ST_RQSNS, 0);
		tmpstat = st_scsistart();
		/* restore the previous value in sc_stflags */
		sc->sc_stflags = tmpstflags;
		if(tmpstat == ST_SUCCESS) {
#ifdef STDEBUG
		    if (stdebug > 1) {
			stprintsense();
		    }
#endif STDEBUG
		    switch(ret = sterror(sc)) {
			case ST_SUCCESS:
			    sc->sc_xstate = ST_NEXT;
			    sc->sc_xevent = ST_CONT;
			    break;

			case ST_RETRY:
			    if (st_retries < 10) {
#ifdef STDEBUG
				printd("retrying write\n");
#endif STDEBUG
				st_retries++;
				sc->sc_xstate = ST_NEXT;
				sc->sc_xevent = ST_BEGIN;
			    }
			    else {
				st_retries = 0;
				bp->b_flags |= B_ERROR;
			        sc->sc_xstate = ST_NEXT;
			        sc->sc_xevent = ST_CONT;
			    }
			    break;

			case ST_FATAL:
			    /* 
			     * If DEV_CSE is set, the utility knows what
			     * it's doing, so just continue letting it
			     * write.
			     */
			    if((sc->sc_flags & DEV_EOM) &&
				(sc->sc_flags & DEV_CSE) &&
				(sc->sc_sns.eom)) {
				sc->sc_xstate = ST_NEXT;
				sc->sc_xevent = ST_CONT;
				break;
			    }
			    /*
			     * This case is really not a fatal error.
			     * The data is really written, but it is 
			     * easier to handle it here as an error
			     */
		    	    if((sc->sc_flags & DEV_EOM) &&
				!((sc->sc_flags & DEV_CSE) ||
				(dis_eot_st[0] & DISEOT))) {
				bp->b_error = ENOSPC;
				sc->sc_xstate = ST_NEXT;
				sc->sc_xevent = ST_CONT;
				break;
			    }
			    bp->b_flags |= B_ERROR;
			    sc->sc_xstate = ST_NEXT;
			    sc->sc_xevent = ST_CONT;
			    break;

			default:
#ifdef STDEBUG
    			    printd("st_start: ST_W_DMA: unknown ret from sterror = 0x%x\n",
			ret);
#endif STDEBUG
			    bp->b_flags |= B_ERROR;
			    sc->sc_xstate = ST_NEXT;
			    sc->sc_xevent = ST_CONT;
			    break;
		    }
		    break;
		}
		else {
			    bp->b_flags |= B_ERROR;
			    sc->sc_xstate = ST_NEXT;
			    sc->sc_xevent = ST_CONT;
			    break;
		}
	    }
	    else {
		sc->sc_xstate = ST_NEXT;
		sc->sc_xevent = ST_CONT;
	    }
	    break;

/*  */
	case ST_ERR:
	    switch (sc->sc_xevent) {
		case ST_RESET:
		    /* 
		     * Reset the bus and then run down the request queue
		     * returning an error for all requests till the queue
		     * is empty.
		     */
		    while (dp->b_actf != NULL) {
			if (bp == &cstbuf[0]) {
			    bp->b_flags = B_ERROR;
			    bp->b_bcount = 0;
			    dp->b_actf = bp->av_forw;
			    biodone(bp);
			}
			else {
			    bp = dp->b_actf;
			    bp->b_resid = bp->b_bcount;
			    bp->b_error = EIO;
			    bp->b_flags |= B_ERROR;
			    dp->b_actf = bp->av_forw;
			    biodone(bp);
			}
		    }
		    st_reset();
		    if(sc->sc_stflags & ST_RETD_KEEP) {
			sc->sc_stflags &= ~ST_RETD_KEEP;
			vd->vsd_action = VS_DEALLOC;
			vs_bufctl(vd);
		    }
		    else {
			return(VS_DEALLOC);
		    }
		    return(VS_DEALLOC);
		    break;

		case ST_ABRT:
		case ST_PHA_MIS:
		case ST_FREEB:
#ifdef STDEBUG
		    printd ("st_start: ST_ERR: xevent = 0x%x\n", sc->sc_xevent);
		    stdumpregs();
		    printd ("st_start: ST_ERR: giving up ...\n");
#endif STDEBUG
		    bp->b_flags |= B_ERROR; 
		    sc->sc_xstate = ST_NEXT;
		    sc->sc_xevent = ST_CONT;
		    break;

		default:
#ifdef STDEBUG
		    printd ("st_start: ST_ERR: xevent = 0x%x\n",
			sc->sc_xevent);
#endif STDEBUG
		    bp->b_flags |= B_ERROR; 
		    sc->sc_xstate = ST_NEXT;
		    sc->sc_xevent = ST_CONT;
		    break;
	    }
	    break;

	default:
	    ;
#ifdef STDEBUG
	    printd("st0: st_start: unknown xstate  = 0x%x\n", sc->sc_xstate);
#endif STDEBUG
	}
    }
}

/*  */
/*
 * Print sense data out of sc->sc_sns, from ST_RQSNS command
 */
stprintsense()
{
	register struct st_softc *sc;
	u_char *byteptr;
	int i;

	sc = st_softc;
	byteptr = (u_char *)&sc->sc_sns;
	mprintf("request sense data: %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
	    *byteptr, *(byteptr + 1), *(byteptr + 2), *(byteptr + 3),
	    *(byteptr + 4), *(byteptr + 5), *(byteptr + 6), *(byteptr + 7),
	    *(byteptr + 8), *(byteptr + 9), *(byteptr + 10), *(byteptr + 11),
	    *(byteptr + 12), *(byteptr + 13), *(byteptr + 14), *(byteptr + 15),
	    *(byteptr + 16), *(byteptr + 17), *(byteptr + 18), *(byteptr + 19));
}

sterror(sc)
register struct st_softc *sc;
{
	int st_ret = 0;	/* the return flag to return if no error	      */
	int softerr=0;	/* a soft error has occured, report only	      */
	int harderr=0;	/* a hard error has occured, report, and terminate    */

#ifdef STDEBUG
	printd1("sterror:\n");
	if (stdebug > 1)
	    stprintsense();
#endif STDEBUG
	if (sc->sc_sns.errclass != 7)
	    return(ST_FATAL);
	switch (sc->sc_sns.snskey) {
		case ST_NOSENSE:
			st_ret = ST_SUCCESS;
			if (sc->sc_sns.eom) {
				if ((sc->sc_curcmd == ST_READ) || 
					(sc->sc_curcmd == ST_WRITE)) {
					if (dis_eot_st[0] != DISEOT) {
						sc->sc_flags |= DEV_EOM;
						st_ret = ST_FATAL;
					}
				}
			}
			if (sc->sc_sns.filmrk) {
#ifdef STDEBUG
				printd("sterror: read filemark\n");
#endif STDEBUG
				sc->sc_category_flags |= DEV_TPMARK;
			}
			if (sc->sc_sns.ili) {
				st_ret = ST_FATAL;
#ifdef STDEBUG
				printd3("sterror: sc_curcmd = 0x%x/n", sc->sc_curcmd);
#endif STDEBUG
				if (sc->sc_curcmd == ST_READ) {
					st_ret = ST_SUCCESS;
				}
			}
			break;

		case ST_RECOVERR:
			st_ret = ST_SUCCESS;
			softerr++;
			break;

		case ST_NOTREADY:
			st_ret = ST_RETRY;
			break;

		case ST_MEDIUMERR:
			st_ret = ST_FATAL;
			if (sc->sc_sns.eom) {
				DEV_UGH(sc->sc_device,0,"encountered BOT");
			}
			break;

		case ST_HARDWARE:
			st_ret = ST_FATAL;
			harderr++;
			break;

		case ST_ILLEGALREQ:
			st_ret = ST_FATAL;
			printf("st0: software error\n");
			harderr++;
			break;

		case ST_UNITATTEN:
			st_ret = ST_RETRY;
			sc->sc_flags &= ~DEV_EOM;
			/* Check device selftest and Rev levels... */
			unit_attn = 1;
			break;
	
		case ST_DATAPROTECT:
			st_ret = ST_FATAL;
			sc->sc_flags |= DEV_WRTLCK;
			DEV_UGH(sc->sc_device,0,"media write protected");
			harderr++;
			break;

		case ST_BLANKCHK:
			st_ret = ST_FATAL;
			break;

		case ST_ABORTEDCMD:
			st_ret = ST_FATAL;
#ifdef STDEBUG
			printd("Aborted Command\n");
#endif STDEBUG
			break;

		case ST_VOLUMEOVFL:
			st_ret = ST_FATAL;
			DEV_UGH(sc->sc_device,0,"volume overflow");
			harderr++;
			break;

		case ST_MISCOMPARE:
			st_ret = ST_FATAL;
			harderr++;
			break;

		default:
#ifdef STDEBUG
			printd("sterror: unknown sense key = 0x%x\n",
				sc->sc_sns.snskey);
#endif STDEBUG
			st_ret = ST_FATAL;
			harderr++;
			break;
	}
	if (softerr) {
		mprintf("st0: soft error, sense key = 0x%x\n",
			sc->sc_sns.snskey);
			stprintsense();
	}
	if (harderr) {
		sc->sc_flags |= DEV_HARDERR;
		sc->sc_hardcnt++;
		printf("st0: hard error, sense key = 0x%x\n",
			sc->sc_sns.snskey);
		stprintsense();
	}
	return(st_ret);
}

/*  */
st_reset()
{
	register struct nb1_regs *staddr = (struct nb1_regs *)qmem;
	register struct nb_regs *stiaddr = (struct nb_regs *)nexus;
	register struct st_softc *sc = &st_softc[0];
	int i;

	/* 
	 * Disable interrupts and reset the bus
	 */
	stiaddr->nb_int_msk &= ~SCS_INT_TAPE;
	staddr->scs_inicmd = SCS_INI_RST;
	DELAY(50);
	inicmd_tmp = 0;
	staddr->scs_inicmd = inicmd_tmp;
	sc->sc_selstat = ST_IDLE;
	sc->sc_fstate = 0;
	/*
	 * reading this register clears pending interrupts, parity
	 * errors, and busyerr.  No meaningful value is returned.
	 */
	i = staddr->scs_reset;
	stiaddr->nb_int_reqclr = SCS_INT_TAPE;
	return;
}

/*  */

#ifdef STDEBUG
stdumpregs()
{
    register struct nb1_regs *staddr = (struct nb1_regs *)qmem;
    register struct nb_regs *stiaddr = (struct nb_regs *)nexus;

    printd ("\t\tDevice register dump:\n");
    printd ("\t\tinicmd_tmp = 0x%x\n", inicmd_tmp);
    printd ("\t\tscs_inicmd = 0x%x\n", staddr->scs_inicmd);
    printd ("\t\tscs_mode = 0x%x\n", staddr->scs_mode);
    printd ("\t\tscs_tarcmd = 0x%x\n", staddr->scs_tarcmd);
    printd ("\t\tscs_curstat = 0x%x\n", staddr->scs_curstat);
    printd ("\t\tscs_status = 0x%x\n", staddr->scs_status);
    stiaddr->nb_hltcod = 0;
    printd ("\t\tscd_cnt = 0x%x\n", staddr->scd_cnt);
    printd ("\t\tnb_int_msk = 0x%x\n", stiaddr->nb_int_msk);
}
#endif STDEBUG

/*  */

/*
 * st_scsistart() is called to start the transfer on the SCSI bus.
 */

st_scsistart()
{
    register struct uba_ctlr *um = stdinfo[0]->ui_mi;
    register struct st_softc *sc = &st_softc[0];
    register struct buf *dp;
    register struct buf *bp;
    register struct nb1_regs *staddr = (struct nb1_regs *)qmem;
    register struct nb_regs *stiaddr = (struct nb_regs *)nexus;
    register struct vsbuf *vs;
    register struct vsdev *vd;
    char *stv;			/* virtual address of st page tables	      */
    struct pte *pte, *mpte;	/* used for mapping page table entries	      */
    char *bufp;
    unsigned v;
    int o, npf;
    struct proc *rp;
    u_char *byteptr;
    int i, s, retval, count;
    int sel_dly = 0;
    int	arb_tries;
    int cmdcnt;
    int complete;
    int	num_expected;

    vs = &vsbuf;
    vd = &vstapedev;
    dp = &stutab[0];
    bp = dp->b_actf;

    if(sc->sc_selstat == ST_IDLE) {
    /*
     * Arbitrate for the bus, and select the 
     * device.
     */
    /*
     * reset any parity errors, busy errors,
     * or pending interrupts, and enable parity
     * checking.
     */
    i = staddr->scs_reset;
    staddr->scs_mode = SCS_PARCK;
    /*
     * reset the arbitration bit
     */
    staddr->scs_mode &= ~SCS_ARB;
    /*
     * set ID register, and start arbitration for bus
     */
    staddr->scs_outdata = SCS_ID0;
    staddr->scs_mode = (SCS_PARCK | SCS_ARB);
    /*
     * spin waiting for the arbitration to start
     */
#ifdef STDEBUG
    printd6("st_scsistart: ST_SP_ARB: spinning...\n");
#endif STDEBUG
    STWAIT_WHILE(((staddr->scs_inicmd & SCS_INI_AIP) == 0),1000,retval);
    if (retval >= 1000) {
#ifdef STDEBUG
	printd("st0: SCS_INI_AIP failed to set\n");
#endif STDEBUG
	goto abort;
    }
    DELAY(3);	/* arbitration delay */
    /*
     * check for lost arbitration, if haven't lost,
     * then check for higher IDs on the bus. If my
     * id is the highest, I've won the arbitration
     */
    if((staddr->scs_inicmd & SCS_INI_LA) == 0) {
	if(staddr->scs_curdata <= SCS_ID0) {
	    /*
	     * check for lost arbitration again, just
	     * to be sure, and then assert select
	     */
	    if((staddr->scs_inicmd & SCS_INI_LA) == 0) {
		/*
		 * there needs to be a 1.2us delay before putting the
		 * ID on the bus in the select state.  It is here. 
		 * It may not be needed, but just to be safe it's here.
		 */
		DELAY(2);
		arb_tries = 0;
	    }
	    else {
		if (arb_tries <= ST_MAX_ARB_TRIES) {
		    arb_tries++;
		}
		else {
		    /*
		     * Too many arb_tries, print an error, and close 
		     * down the device 
		     */
		    mprintf("st0: arbitration failed after %d tries\n",
			arb_tries);
		    sc->sc_stflags |= ST_ENCR_ERR;
#ifdef STDEBUG
		    printd("st0: arbitration failed after %d tries\n",
			arb_tries);
#endif STDEBUG
		    goto abort;
		}
	    }
	}
    }
    /* clear any pending interupts */
    i = staddr->scs_reset;
    /* enable interrupts */
    stiaddr->nb_int_reqclr = SCS_INT_TAPE;
    stiaddr->nb_int_msk |= SCS_INT_TAPE;
    /*
     * put initiator and target IDs into data register,
     * and do the select.
     */
#ifdef STDEBUG
    printd6("tarcmd = 0x%x, status = 0x%x\n",
	    staddr->scs_tarcmd, staddr->scs_status);
#endif STDEBUG
    staddr->scs_tarcmd = 0;
#ifdef STDEBUG
    printd6("tarcmd = 0x%x, status = 0x%x\n",
    	    staddr->scs_tarcmd, staddr->scs_status);
#endif STDEBUG
    staddr->scs_outdata = (SCS_ID1 | SCS_ID0);
    inicmd_tmp=(SCS_INI_ATN|SCS_INI_SEL|SCS_INI_BSY|SCS_INI_ENOUT);
    staddr->scs_inicmd = inicmd_tmp;
    staddr->scs_mode &= (~SCS_ARB & ~SCS_TARG);
    /*
     * Should not get an interrupt selecting
     * the device.
     */
    staddr->scs_selena = SCS_ID0;
    DELAY(1);
    inicmd_tmp &= ~SCS_INI_BSY;
    staddr->scs_inicmd = inicmd_tmp;
    /*
     * delay at least 400ns, and start looking for BSY
     * from the target.
     */
    sel_dly = 0;
    DELAY(1);
    STWAIT_WHILE(((staddr->scs_curstat & SCS_BSY) == 0),ST_MAX_SEL_DLY,retval);
    if ((staddr->scs_curstat & SCS_BSY) == 0) {
	    mprintf("st0: device failed to select\n");
	    goto abort;
    }
    /*
     * reset select and data bus
     */
    inicmd_tmp &= ~SCS_INI_SEL;
    staddr->scs_inicmd = inicmd_tmp;
    sc->sc_selstat = ST_SELECT;
    }

    /*
     * The target controls the bus phase. We wait for REQ and BSY
     * to be asserted on the bus before determining which phase
     * is active.  Once REQ and BSY are asserted, the appropiate action
     * is taken. 
     */
    complete = 0;
    do {
	STWAIT_WHILE((((staddr->scs_curstat & SCS_REQ) != SCS_REQ) ||
		((staddr->scs_curstat & SCS_BSY) != SCS_BSY)),1000000,retval);
	if (retval >= 1000000) {
#ifdef STDEBUG
	    printd("\t\tst_scsistart: SCS_REQ  or SCS_BSY failed to set\n");
	    stdumpregs();
#endif STDEBUG
	    goto abort;
	}
	if (staddr->scs_status & SCS_PARERR) {
	    mprintf ("st0: parity error\n");
#ifdef STDEBUG
	    printd("st0: parity error\n");
#endif STDEBUG
	    goto abort;
	}
	else if (staddr->scs_curstat & SCS_RST) {
	    mprintf ("st0: bus reset\n");
#ifdef STDEBUG
	    printd("st0: bus reset\n");
#endif STDEBUG
	    goto abort;
	}
	i = staddr->scs_reset;
	stiaddr->nb_int_reqclr = SCS_INT_TAPE;
	/*
	 * Zero the scs_mode register to clear all 
	 * DMA status bits from the last DMA transfer
	 */
	staddr->scs_mode = 0;
	/*
	 * Turn off the SCS_INI_ENOUT driver
	 */
	inicmd_tmp &= ~SCS_INI_ENOUT;
	staddr->scs_inicmd = inicmd_tmp;
	/*
	 * Read the bus phase, set the phase to match.
	 */
	staddr->scs_tarcmd = ((staddr->scs_curstat & SCS_PHA_MSK) >> 2);
	switch (staddr->scs_curstat & SCS_PHA_MSK) {

	case SCS_DATAO:
#ifdef STDEBUG
	    printd2("st_scsistart: SCS_DATAO:\n");
#endif STDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = ST_DATAO_PHA;
	    if (sc->st_opcode != ST_WRITE) {
		byteptr = (u_char *)&sc->sc_dat[0];
		if (sc->st_opcode == ST_MODSEL) {
		    cmdcnt = ST_MODSEL_LEN;
		}
		else {
		    if (sc->st_opcode == ST_INQ) {
			cmdcnt = ST_CMD_LEN;
		    }
		    else {
#ifdef STDEBUG
			printd ("st_scsistart: SCS_DATAO: unknown command 0x%x\n",
			    sc->st_opcode);
#endif STDEBUG
			goto abort;
		    }
		}
		inicmd_tmp = SCS_INI_ENOUT;
		staddr->scs_inicmd = inicmd_tmp;
		/*
		 * Send the command packet
		 */
#ifdef STDEBUG
		printd2("Data Output Packet: \n");		/* 1 of 3 */
#endif STDEBUG
		for ( ; (cmdcnt > 0); cmdcnt--) {
		    STWAIT_UNTIL(((staddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
		    if (retval >= 10000) {
#ifdef STDEBUG
			printd("st_scsistart: SCS_DATAO: REQ not set\n");
#endif STDEBUG
			goto abort;
		    }
#ifdef STDEBUG
		    printd2 (" %x", *byteptr);			/* 2 of 3 */
#endif STDEBUG
		    staddr->scs_outdata = *byteptr++;
		    inicmd_tmp |= SCS_INI_ACK;
		    staddr->scs_inicmd = inicmd_tmp;
		    STWAIT_UNTIL(((staddr->scs_curstat & SCS_REQ) != SCS_REQ),10000,retval);
		    if (retval >= 10000) {
#ifdef STDEBUG
			printd("st_scsistart: SCS_DATAO: REQ didn't go false\n");
#endif STDEBUG
			goto abort;
		    }
		    inicmd_tmp &= ~SCS_INI_ACK;
		    staddr->scs_inicmd = inicmd_tmp;
		}
#ifdef STDEBUG
		printd2 ("\n");					/* 3 of 3 */
#endif STDEBUG
		break;
	    }
	    /* Copy the data from the 16K buffer to memory 
	     * (user space or kernel space).  Will have to
	     * set up own page table entries when copying to
	     * user space.
	     */
	    /*
	     * If bytecount is too large, throw out the transfer
	     */
	    if (bp->b_bcount > 16384) {
		mprintf("st0: buffer too large\n");
		DEV_UGH(sc->sc_device,0,"buffer too large");
		bp->b_flags |= B_ERROR;
		goto abort;
		break;
	    }
	    /*
	     * set up the count;
	     */
	    if ((sc->sc_stflags & ST_WAS_DISCON) && (sc->sc_savcnt != 0)) {
		sc->sc_bcount = -(sc->sc_savcnt);
		staddr->scd_cnt = sc->sc_savcnt ;
#ifdef STDEBUG
		printd1("st_scsistart#1: scd_cnt = 0x%x, sc_savcnt = 0x%x\n",
			staddr->scd_cnt, sc->sc_savcnt);
#endif STDEBUG
	    }
	    else {
		sc->sc_bcount = (short)bp->b_bcount;
		staddr->scd_cnt = (short)-(bp->b_bcount);
	    }
	    /*
	     * Map the user page tables to my page tables (mpte)
	     */
		if ((cpu == C_VAXSTAR) && cvs_exmode_on)
			stv = (char *)cvseddbmem + 0x10000;
		else
			stv = (char *)&staddr->nb_ddb[0];
		if ((bp->b_flags & B_PHYS) == 0) {
		    bufp = (char *)bp->b_un.b_addr;
		}
		else {
		    /*
	    	     * Map to user space
		     */
	    	    v = btop(bp->b_un.b_addr);
		    o = (int)bp->b_un.b_addr & PGOFSET;
		    npf = btoc(bp->b_bcount + o);
		    rp = (bp->b_flags&B_DIRTY) ? &proc[2] : bp->b_proc;
		    if (bp->b_flags & B_UAREA) {
			pte = &rp->p_addr[v];
		    }
		    else if (bp->b_flags & B_PAGET) {
			pte = &Usrptmap[btokmx((struct pte *)bp->b_un.b_addr)];
		    }
		    else {
			pte = vtopte(rp, v);
		    }
		    bufp = (char *)ST_bufmap + o;
		    mpte = (struct pte *)stbufmap;
		    for (i = 0; i < npf; i++) {
			if (pte->pg_pfnum == 0)
			    panic("st0: zero pfn in pte");
			*(int *)mpte++ = pte++->pg_pfnum | PG_V | PG_KW;
			mtpr(TBIS, (char *) ST_bufmap + (i * NBPG));
		    }
		    *(int *)mpte = 0;
		    mtpr(TBIS, (char *) ST_bufmap + (i * NBPG));
		}
		sc->sc_stflags |= ST_MAPPED;
		sc->sc_savstv = (long)stv;
		sc->sc_savbufp = (long)bufp;
	    /*
	     * If the DMA transfer was interrupted by a disconnect
	     * the remaining data to be transfered needs to be 
	     * reloaded into the 16K buffer, and then another
	     * DMA transfer started.  This only needs to be done
	     * on a write DMA transfer.
	     */
	    if ((sc->sc_stflags & ST_WAS_DISCON) && (sc->sc_savcnt != 0)) {
		/*
		 * This is a quick way to subtract the the number of
		 * bytes not transfered before the disconnect, from
		 * the total number of bytes to transfer.  The result
		 * is the number of bytes that need to be transfered.
		 * sc->sc_savcnt is in two's complement form.  The
		 * number of bytes to be transfered is used as an offset
		 * and added to bufp to get the address of where to start
		 * transfering data from.
		 */
		sc->sc_stflags &= ~ST_WAS_DISCON;
		bufp =  bufp + (((short)bp->b_bcount + sc->sc_savcnt) & 0xffff);
		bcopy (bufp, stv, -(sc->sc_savcnt));
#ifdef STDEBUG
		printd1("st_scsistart: bufp offset = 0x%x, count = 0x%x\n",
			(((short)bp->b_bcount + sc->sc_savcnt) & 0xffff),
			-(sc->sc_savcnt));
#endif STDEBUG
	    }
	    else {
		bcopy (bufp, stv, bp->b_bcount);
	    }
	    /*
	     * Enable interrupts, and DMA mode.  
    	     */
	    staddr->scs_mode = (SCS_DMA | SCS_INTEOP | SCS_PARCK | SCS_INTPAR);
	    inicmd_tmp = SCS_INI_ENOUT;
	    staddr->scs_inicmd = inicmd_tmp;
	    staddr->scd_dir = SCD_DMA_OUT;
	    /*
	     * Set the starting address in the 16K buffer.  This
	     * is really an offset into the buffer.  It needs to
	     * be written twice.  Whatever is in bits <7:0> gets
	     * moved into bits <15:8> with the bits <15:14> zeroed,
	     * and what ever you write into this register get put
	     * into bits <7:0>.  Therefore, the first write contains
	     * the bits destined for <15:8>, and the second write
	     * contains the bits for <7:0>.
	     */
	    if ((cpu == C_VAXSTAR) && cvs_exmode_on) 
		staddr->nb_scd_adr.l = 0x10000;
	    else {
		staddr->nb_scd_adr.c[0] = 0;
		staddr->nb_scd_adr.c[0] = 0;
	    }
	    /*
	     * Start the DMA transfer
	     */
	    staddr->scs_dmasend = 1;
	    sc->sc_stflags |= (ST_DID_DMA | ST_RETD_KEEP);
	    return(ST_IP);
	    break;

	case SCS_DATAI:
#ifdef STDEBUG
	    printd2("st_scsistart: SCS_DATAI:\n");
#endif STDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = ST_DATAI_PHA;
	    if (sc->st_opcode != ST_READ) {
		byteptr = (u_char *)&sc->sc_sns;
		switch (sc->st_opcode) {
		    case ST_RQSNS:
			num_expected = ST_RQSNS_LEN;
			break;

		    case ST_INQ:
			num_expected = ST_INQ_LEN;
			byteptr = (u_char *)&sc->st_dat;
			break;

		    case ST_MODSNS:
			num_expected = ST_MODSNS_LEN;
			byteptr = (u_char *)&sc->st_dat;
			break;

		    case ST_RECDIAG:
			num_expected = ST_RECDIAG_LEN;
			byteptr = (u_char *)&sc->st_dat;
			break;

		    default:
#ifdef STDEBUG
			printd("st_scsistart: SCS_DATAI: unexpected command = %x\n",
				sc->st_opcode);
#endif STDEBUG
		    break;
		}
		inicmd_tmp = 0;
		staddr->scs_inicmd = inicmd_tmp;
		/*
		 * Receive the command packet
		 */
#ifdef STDEBUG
		printd2("Data In Packet:");				/* 1 of 3 */
#endif STDEBUG
		for ( ; (num_expected > 0); num_expected--) {
		    STWAIT_UNTIL(((staddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
		    if (retval >= 10000) {
#ifdef STDEBUG
			printd("st_scsistart: SCS_DATAI: REQ not set\n");
#endif STDEBUG
			goto abort;
		    }
		    *byteptr++ = staddr->scs_curdata;
#ifdef STDEBUG
		    printd2(" %x", staddr->scs_curdata);		/* 2 of 3 */
#endif STDEBUG
		    inicmd_tmp |= SCS_INI_ACK;
		    staddr->scs_inicmd = inicmd_tmp;
		    STWAIT_UNTIL(((staddr->scs_curstat & SCS_REQ) != SCS_REQ),10000,retval);
		    if (retval >= 10000) {
#ifdef STDEBUG
			printd("st_scsistart: SCS_DATAO: REQ didn't go false\n");
#endif STDEBUG
			goto abort;
		    }
		    inicmd_tmp &= ~SCS_INI_ACK;
		    staddr->scs_inicmd = inicmd_tmp;
		}
#ifdef STDEBUG
		printd2("\n");					/* 3 of 3 */
#endif STDEBUG
		break;
	    }
	    /*
	     * Start of DMA code.
	     *
	     * set up the count;
	     */
	    if ((sc->sc_stflags & ST_WAS_DISCON) && (sc->sc_savcnt != 0)) {
		sc->sc_bcount = -(sc->sc_savcnt);
		staddr->scd_cnt = sc->sc_savcnt ;
#ifdef STDEBUG
		printd1("st_scsistart#2: scd_cnt = 0x%x, sc_savcnt = 0x%x\n",
			staddr->scd_cnt, sc->sc_savcnt);
#endif STDEBUG
	    }
	    else {
		sc->sc_bcount = (short)bp->b_bcount;
		staddr->scd_cnt = (short)-(bp->b_bcount);
	    }
	    /*
	     * Enable interrupts, and DMA mode.  
	     */
	    staddr->scs_mode = (SCS_DMA | SCS_INTEOP | SCS_PARCK | SCS_INTPAR);
	    staddr->scs_inicmd = 0;
	    staddr->scd_dir = SCD_DMA_IN;
	    /*
	     * Set the starting address in the 16K buffer.  This
	     * is really an offset into the buffer.  It needs to
	     * be written twice.  Whatever is in bits <7:0> gets
	     * moved into bits <15:8> with the bits <15:14> zeroed,
	     * and what ever you write into this register get put
	     * into bits <7:0>.  Therefore, the first write contains
	     * the bits destined for <15:8>, and the second write
	     * contains the bits for <7:0>.
	     */
	    if ((cpu == C_VAXSTAR) && cvs_exmode_on) 
		staddr->nb_scd_adr.l = 0x10000;
	    else {
		staddr->nb_scd_adr.c[0] = 0;
		staddr->nb_scd_adr.c[0] = 0;
	    }
	    /*
	     * Start the DMA transfer
	     */
	    staddr->scs_dmaircv = 1;
	    sc->sc_stflags |= (ST_DID_DMA | ST_RETD_KEEP);
	    return(ST_IP);
	    break;

	case SCS_CMD:
#ifdef STDEBUG
	    printd2("st_scsistart: SCS_CMD:\n");
#endif STDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = ST_CMD_PHA;
	    /*
	     * clear savecnt and stflags
	     */
	    sc->sc_stflags = 0;
	    sc->sc_savcnt = 0;
	    inicmd_tmp = SCS_INI_ENOUT;
	    staddr->scs_inicmd = inicmd_tmp;
	    byteptr = (u_char *)&sc->st_command;
#ifdef STDEBUG
	    if ((st_prev_cmd == ST_WFM) && (*byteptr != ST_SPACE)) {
		printd1("Didn't space back over second filemark\n");
	    }
	    st_prev_cmd = *byteptr;
	    printd2("cmd pkt: %x %x %x %x %x\n", *byteptr, *(byteptr + 1),
		*(byteptr + 2), *(byteptr + 3), *(byteptr + 4), *(byteptr + 5));
#endif STDEBUG
	    /*
	     * Send the command packet
	     */
	    for (cmdcnt = ST_CMD_LEN; (cmdcnt > 0); cmdcnt--) {
		STWAIT_UNTIL(((staddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
		if (retval >= 10000) {
#ifdef STDEBUG
		    printd("st_scsistart: SCS_CMD: REQ not set\n");
#endif STDEBUG
		    goto abort;
		}
#ifdef STDEBUG
		ST_CMDTRACK(*byteptr); 
#endif STDEBUG
		staddr->scs_outdata = *byteptr++;
		inicmd_tmp |= SCS_INI_ACK;
		staddr->scs_inicmd = inicmd_tmp;
		STWAIT_UNTIL(((staddr->scs_curstat & SCS_REQ) != SCS_REQ),10000,retval);
		if (retval >= 10000) {
#ifdef STDEBUG
		    printd("st_scsistart: SCS_CMD: REQ didn't go false\n");
#endif STDEBUG
		    goto abort;
		}
		inicmd_tmp &= ~SCS_INI_ACK;
		staddr->scs_inicmd = inicmd_tmp;
	    }
	    break;

	case SCS_STATUS:
#ifdef STDEBUG
	    printd2("st_scsistart: SCS_STATUS:\n");
#endif STDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = ST_STATUS_PHA;
	    /*
	     * Save the residual count if the command is a read
	     * or a write.
	     */
	    if((sc->st_opcode==ST_READ)||(sc->st_opcode==ST_WRITE)) {
/* Fix for rev 45 TZK50 */
/*
		if ((sc->sc_prevpha == ST_DATAI_PHA) ||
		    (sc->sc_prevpha == ST_DATAO_PHA)) {
*/
		    stiaddr->nb_hltcod = 0;
		    sc->sc_resid = -(staddr->scd_cnt);
/*
		}
		else {
		    sc->sc_resid = bp->b_bcount;
		}
*/
	    }
	    STWAIT_UNTIL(((staddr->scs_curstat & SCS_REQ) == SCS_REQ),10000,retval);
	    if (retval >= 10000) {
#ifdef STDEBUG
		printd("st_scsistart: SCS_STATUS: REQ didn't set\n");
#endif STDEBUG
		goto abort;
	    }
	    sc->sc_status = staddr->scs_curdata;
	    inicmd_tmp = SCS_INI_ACK;
	    staddr->scs_inicmd = inicmd_tmp;
#ifdef STDEBUG
	    printd2("st_scsistart: SCS_STATUS: ppha = 0x%x, status = 0x%x, scd_cnt = 0x%x\n",
		sc->sc_prevpha, sc->sc_status, staddr->scd_cnt);
#endif STDEBUG
	    STWAIT_WHILE(((staddr->scs_curstat & SCS_REQ) == SCS_REQ),50000,retval);
	    if (retval >= 50000) {
#ifdef STDEBUG
		printd("st_scsistart: SCS_STATUS: REQ didn't go false\n");
#endif STDEBUG
		goto abort;
	    }
	    inicmd_tmp = 0;
	    staddr->scs_inicmd = inicmd_tmp;
	    /*
	     * Check the status
	     */
	    if (sc->sc_status != ST_GOOD) {
		sc->sc_stflags |= ST_NEED_SENSE;
	    }
	    break;

	case SCS_MESSO:
#ifdef STDEBUG
	    printd2("st_scsistart: SCS_MESSO:\n");
#endif STDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = ST_MESSO_PHA;
	    sc->sc_stflags = 0;
	    /*
	     * Send the Identify message
	     */
	    if(staddr->scs_curstat & SCS_REQ) {
		staddr->scs_outdata = ST_ID_DIS;
		inicmd_tmp |= (SCS_INI_ENOUT | SCS_INI_ATN);
		staddr->scs_inicmd = inicmd_tmp;
		DELAY(2);
		inicmd_tmp &= ~SCS_INI_ATN;
		staddr->scs_inicmd = inicmd_tmp;
		inicmd_tmp |= SCS_INI_ACK;
		staddr->scs_inicmd = inicmd_tmp;
		STWAIT_WHILE((staddr->scs_curstat & SCS_REQ),100000,retval);
		if(retval >= 100000) {
#ifdef STDEBUG
		printd("st_scsistart: SCS_MESSO: SCS_REQ failed to set\n");
#endif STDEBUG
		    goto abort;
		}
		inicmd_tmp &= ~SCS_INI_ACK;
		staddr->scs_inicmd = inicmd_tmp;

		if (staddr->scs_status & SCS_PARERR) {
#ifdef STDEBUG
		printd("st_scsistart: SCS_MESSO: parity error\n");
#endif STDEBUG
		    goto abort;
		}
	    }
	    break;

	case SCS_MESSI:
#ifdef STDEBUG
	    printd2("st_scsistart: SCS_MESSI:\n");
#endif STDEBUG
	    sc->sc_prevpha = sc->sc_fstate;
	    sc->sc_fstate = ST_MESSI_PHA;
	    sc->sc_message = staddr->scs_curdata;
	    inicmd_tmp |= SCS_INI_ACK;
	    staddr->scs_inicmd = inicmd_tmp;
	    STWAIT_WHILE(((staddr->scs_curstat & SCS_REQ) == SCS_REQ),1000000,retval);
	    if(retval >= 1000000) {
#ifdef STDEBUG
	    printd("st_scsistart: SCS_MESSI: SCS_REQ failed to clear\n");
#endif STDEBUG
		goto abort;
	    }
	    inicmd_tmp &= ~SCS_INI_ACK;
	    staddr->scs_inicmd = inicmd_tmp;
	    switch (sc->sc_message) {
		case ST_CMDCPT:
		    /*
		     * The command has completed, release the bus.
		     */
		    /* disable interrupts */
		    stiaddr->nb_int_msk &= ~SCS_INT_TAPE;
		    inicmd_tmp = 0;
		    staddr->scs_inicmd = 0;
		    staddr->scs_mode = 0;
		    staddr->scs_tarcmd = 0;
		    staddr->scs_selena = 0;
		    /*
		     * clear SCS_PARERR, SCS_INTREQ, and SCS_BSYERR 
		     * in mode reg, and return 
		     */
		    /*
		     * clear any pending interrupts
		     */
		    stiaddr->nb_int_reqclr = SCS_INT_TAPE;
		    i = staddr->scs_reset;
		    sc->sc_selstat = ST_IDLE;
		    sc->sc_fstate = 0;
		    complete++;
		    if (sc->sc_status == ST_GOOD) {
			return(ST_SUCCESS);
		    }
		    else {
			return(ST_RET_ERR);
		    }
		    break;
			
		case ST_SDP:
		    if (sc->sc_prevpha == ST_DATAO_PHA) {
			stiaddr->nb_hltcod = 0;
		    	sc->sc_savcnt = (short)staddr->scd_cnt - 1;
			sc->sc_stflags |= ST_WAS_DISCON;
		    }
/*
		    else {
			sc->sc_savcnt = 0;
		    }
*/
		    if (sc->sc_prevpha == ST_DATAI_PHA) {
			sc->sc_stflags |= ST_WAS_DISCON;
			stiaddr->nb_hltcod = 0;
			sc->sc_savcnt = (short)staddr->scd_cnt;
			/* Fix for rev 45 TZK50 */
			if (sc->sc_savcnt == 0)
			    sc->sc_stflags |= ST_COPY_DONE;
			    /*
			     * Map the user page tables to my page tables (mpte)
			     */
			    if ((cpu == C_VAXSTAR) && cvs_exmode_on)
				stv = (char *)cvseddbmem + 0x10000;
			    else
				stv = (char *)&staddr->nb_ddb[0];
			    if ((bp->b_flags & B_PHYS) == 0) {
				bufp = (char *)bp->b_un.b_addr;
			    }
			    else {
				/*
				 * Map to user space
				 */
				v = btop(bp->b_un.b_addr);
				o = (int)bp->b_un.b_addr & PGOFSET;
				npf = btoc(bp->b_bcount + o);
				rp = (bp->b_flags&B_DIRTY) ? &proc[2] : bp->b_proc;
				if (bp->b_flags & B_UAREA) {
				    pte = &rp->p_addr[v];
				}
				else if (bp->b_flags & B_PAGET) {
				    pte = &Usrptmap[btokmx((struct pte *)bp->b_un.b_addr)];
				}
				else {
				    pte = vtopte(rp, v);
				}
				bufp = (char *)ST_bufmap + o;
				mpte = (struct pte *)stbufmap;
				for (i = 0; i < npf; i++) {
				    if (pte->pg_pfnum == 0)
				    panic("st0: zero pfn in pte");
				    *(int *)mpte++ = pte++->pg_pfnum | PG_V | PG_KW;
				    mtpr(TBIS, (char *) ST_bufmap + (i * NBPG));
				}
				*(int *)mpte = 0;
				mtpr(TBIS, (char *) ST_bufmap + (i * NBPG));
			    }
			    sc->sc_stflags |= ST_MAPPED;
			    sc->sc_savstv = (long)stv;
			    sc->sc_savbufp = (long)bufp;
			/*
			 * This is a quick way to subtract the the number of
			 * bytes not transfered before the disconnect, from
			 * the total number of bytes to transfer.  The result
			 * is the number of bytes that need to be transfered.
			 * sc->sc_savcnt is in two's complement form.
			 */
			bufp = bufp + ((short)bp->b_bcount - sc->sc_bcount);
#ifdef STDEBUG
			printd1("st_scsistart: count = 0x%x\n",
			    (((short)sc->sc_bcount + sc->sc_savcnt) & 0xffff));
#endif STDEBUG
			bcopy (stv, bufp,
			    (((short)sc->sc_bcount + sc->sc_savcnt) & 0xffff));
#ifdef STDEBUG
			if (stdebug > 5) {
			    int j;
			    char *tmpstv;
			    tmpstv = stv;
			    printd("b_bcount = %d, bytes to read = %d\n",
				bp->b_bcount,
				(((short)bp->b_bcount + sc->sc_savcnt) & 0xffff));
			    for (j = 0; j < (((short)bp->b_bcount + sc->sc_savcnt) & 0xffff); j++) {
				printd("%c", *tmpstv++);
			    }
			    printd("\n");
			}
#endif STDEBUG
		    }
#ifdef STDEBUG
		    printd1("st_scsistart: save data pointers, scd_cnt = 0x%x\n",
			sc->sc_savcnt);

#endif STDEBUG
		    break;

		case ST_DISCON:
#ifdef STDEBUG
		    printd2("st_scsistart: disconnect message\n");
#endif STDEBUG
		    staddr->scs_mode = (SCS_MONBSY|SCS_PARCK|SCS_INTPAR);
		    return(ST_IP);
		    break;
			
		case ST_ID_NODIS:
#ifdef STDEBUG
		    printd2("st_scsistart: identify message\n");
#endif STDEBUG
		    break;

		case ST_ID_DIS:
#ifdef STDEBUG
		    printd2("st_scsistart: identify w/disconnect message\n");
#endif STDEBUG
		    break;

		default:

#ifdef STDEBUG
		    printd("st_scsistart: MESSI: message = 0x%x\n", sc->sc_message);
#endif STDEBUG
		    goto abort;
	    }
	    break;

	default:
	    ;
#ifdef STDEBUG
	    mprintf("st0: unknown phase = 0x%x\n",
		staddr->scs_status & SCS_PHA_MSK);
#endif STDEBUG
	}
    } /* end of do loop */
    while (complete == 0);
    return;

abort:
    /*
     * Too much is wrong, we cannot recover.  The transfer will be 
     * aborted by setting sc_xstate to ST_ERR and sc_xevent to ST_RESET.
     * st_start will be called to rundown the request queue, marking
     * each buffer with B_ERROR, and call st_reset to reset the 
     * SCSI bus.
     */
#ifdef STDEBUG
    printd ("st_scsistart: ST_ABORT: fevent = 0x%x\n", sc->sc_fevent);
    stdumpregs();
#endif STDEBUG
    mprintf("st0: aborting transfer\n");
    sc->sc_stflags |= ST_ENCR_ERR;
    sc->sc_xstate = ST_ERR;
    sc->sc_xevent = ST_RESET;
    return(ST_RET_ERR);
}

#ifdef STDEBUG
/*  */
st_dump_softc(sc)
struct st_softc *sc;
{
	printd("Dump of st_softc:\n");
	printd("sc_openf = 0x%x\n", sc->sc_openf);
	printd("sc_flags = 0x%x\n", sc->sc_flags);
	printd("sc_softcnt = 0x%x\n", sc->sc_softcnt);
	printd("sc_hardcnt = 0x%x\n", sc->sc_hardcnt);
	printd("sc_stflags = 0x%x\n", sc->sc_stflags);
	printd("sc_curcmd = 0x%x\n", sc->sc_curcmd);
	printd("sc_prevpha = 0x%x\n", sc->sc_prevpha);
	printd("sc_selstat = 0x%x\n", sc->sc_selstat);
	printd("sc_xstate = 0x%x\n", sc->sc_xstate);
	printd("sc_xevent = 0x%x\n", sc->sc_xevent);
	printd("sc_fstate = 0x%x\n", sc->sc_fstate);
	printd("sc_fevent = 0x%x\n", sc->sc_fevent);
	printd("sc_status = 0x%x\n", sc->sc_status);
	printd("sc_message = 0x%x\n", sc->sc_message);
	if (sc->sc_status == ST_CHKCND) {
	    stprintsense();
	}
}
#endif STDEBUG
#endif
