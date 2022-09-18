#ifndef lint
static char *sccsid = "@(#)if_dpv.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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

#include "dpv.h"
#if NDPV > 0 || defined(BINARY)

/*
 *   Driver for the DPV-11, a synchronous character-oriented data 
 *	communications device, supported here for the bisync protocol.
 *
 *   Written  by	U. Sinkewicz		4/10/85
 */
/*************************************************************************
 *			Modification History
 *
 *   August 18, 1986	U. Sinkewicz
 *			Reset routine did not initialize every dpv device
 *			on the bus.  Changed NDPV to nNDPV in dpvreset
 *			routine.
 *************************************************************************/

#include "../data/if_dpv_data.c"

int dpvdebug = 1;
extern struct protosw *iftype_to_proto(), *iffamily_to_proto();
/* extern struct timeval time; */

/* 
 * Driver information for autoconfigure.
 */
int	dpvprobe(), dpvattach(), dpvrint(), dpvxint(), dpvinit();
struct dpvdevice *dpvaddr;
struct	uba_device   *dpvinfo[NDPV];
u_short	dpvstd[] = { 0 };
struct uba_driver	dpvdriver =
	 { dpvprobe, 0, dpvattach, 0, dpvstd, "dpv", dpvinfo };

#define DPVUNIT(x)	minor(x)
int dpvoutput(), dpvinput(), dpvreset(), dpvioctl();


/*
 * Probe routine.  Generate an interrupt, wait around for the cpu to
 * service it, and reset the device registers.
 */
dpvprobe(reg)
caddr_t reg;
{
	register struct dpvdevice *dpvaddr = (struct dpvdevice *)reg;

#ifdef lint
	dpvrint(0); dpvxint(0);
#endif

	dpvaddr->pcscr = RESET;
	dpvaddr->pcscr = TXITEN | TRAEN;
	DELAY(100000);
	dpvaddr->pcscr = 0;
	if (cvec && cvec != 0x200){
		cvec -= 4;
	}
	return(1);
	/* return (sizeof (struct dpvdevice)); */
}

/*
 * Fill in the network interface record.  The system will initialize the
 * interface when it is ready to accept packets.
 */

dpvattach(ui)
struct uba_device *ui;
{
	register struct dpv_softc *ds = &dpv_softc[ui->ui_unit];

ds->dpv_if.if_unit = ui->ui_unit;
ds->dpv_if.if_name = "dpv";
ds->dpv_if.if_init = dpvinit;
ds->dpv_if.if_output = dpvoutput;
ds->dpv_if.if_ioctl = dpvioctl;
ds->dpv_if.if_reset = dpvreset;
if_attach(&ds->dpv_if);
}

/*
 * Reset state of driver if a unibus reset occurrs.  
 */
dpvreset(unit, uban)
int unit, uban;
{
	register struct uba_device *ui;
	register struct dpvdevice *addr;

	if (unit >= nNDPV || (ui = dpvinfo[unit])== 0 || ui->ui_alive == 0 ||
		ui->ui_ubanum != uban)
		return;

	addr->pcscr = RESET;
	addr->pcsar = 0;
	addr->rxcsr = 0;

	printf("dpv%d\n",unit);
	dpvinit(unit);
}


/*
 * Initialize the device and unibus usage.
 */
dpvinit(unit)
int unit;
{
	register struct dpv_softc *ds = &dpv_softc[unit];
	struct uba_device *ui = dpvinfo[unit];
	register struct dpvdevice *addr = (struct dpvdevice *)ui->ui_addr;
	int s, i;

	/* Dpvinit is called at boot time and at unibus reset. */

	if (ds->dpv_if.if_addrlist == (struct ifaddr *)0){
		return; 
	}

	/*
	 * Allow receive interrupt when CTS comes up and set up for
	 * dialing.
	 */
	addr->pcsar = PROTO;
	addr->pcscr = 0;
	addr->rxcsr = RXITEN | DTR;

	/* 
	 * Initialize the counter in the software context structure and
	 * clear the buffer.
	 */
	ds->counter = 0;
	for (i=0; i<DPVSIZE; i++)
		ds->buf[i] = NULL;

	ds->state = DPVINIT;
	ds->crc = (unsigned short)0;
}


dpvoutput(ifp, m0, dst)
	register struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;

{
	struct mbuf *m;
	register struct dpv_softc *ds = &dpv_softc[ifp->if_unit];
	struct protosw *pr;
	int s, i, error;

	m = m0;

	/* 
	 * Initialize the counter in the software context structure and
	 * clear the buffer.
	 */
	ds->counter = 0;
	for (i=0; i<DPVSIZE; i++)
		ds->buf[i] = NULL;

	ds->state = DPVREADY;

	/*
	 * Queue the mbuf message onto the interface.
	 */
	s = splimp();
	if (IF_QFULL (&ifp->if_snd)){
		IF_DROP(&ifp->if_snd);
		splx(s);
		m_freem(m);
		return(ENOBUFS);
	}

	/*
	 * Put the entire packet onto the send queue 
	 */
	IF_ENQUEUE(&ifp->if_snd,m);
	splx(s);
	ds->state = DPVREADY;
	dpvstart(ifp->if_unit);
	return(0);
}


/*
 * DPV start routine.  Should be called at priority 5.
 */

dpvstart(dev)
dev_t dev;
{
	int unit = DPVUNIT(dev);
	struct uba_device *ui = dpvinfo[unit];
	register struct dpv_softc *ds = &dpv_softc[unit];
	register struct dpvdevice *addr = (struct dpvdevice *)ui->ui_addr;
	struct mbuf *m, *mp;
	unsigned char c;
	int i; 		/* counter */
	int j = 0;  	/* counter */
	int s;
	struct bsc_data *bdata;
	int len;

	s = splimp();
	IF_DEQUEUE(&ds->dpv_if.if_snd, m);
		if ( m == 0){
			ds->state = DPVDONE;
			splx(s);
			m_freem(m);
			ds->counter = 0;
			ds->len = 0;
			return(0);
		}
	
	bdata = mtod(m, struct bsc_data *);
	/* 
	 * Copy from the mbuf to storage.
	 */
	ds->len = 0;  /* len is how much mbuf data is in ds */
	ds->counter = 0; /* counter is index in buf when taking data from ds */
	len = (unsigned)m->m_len;
loop:
	for ( i=0; i<len; i++)
		ds->buf[j++] = bdata->data[i];
	if ( m->m_next != NULL ){
		mp = m->m_next;
		len = (unsigned)mp->m_len;
		bdata = mtod(mp, struct bsc_data *);
		m = mp;
		goto loop;
	}
	ds->len = j;
	splx(s);

	ds->state = DPVPRETNS;

	/* 
	 * Reset the device here because if it's a retranxmit you
	 * need to disable receive interrupts from the device.
	 */  

	addr->pcsar = PROTO | NOCRC | SYNC;
	addr->rxcsr = DSITEN | RXITEN | RTS | DTR;
	if (addr->rxcsr & CTS){
		addr->pcscr = TXITEN | TRAEN;
		addr->tdsr = TSOM | (0377 & SYNC);
	}
} 


/*
 * Transmit interrupt routine.  Take characters from the buffer provided
 * by the start routine and put them in the device's transmit buffer.
 */
dpvxint(unit)
int unit;
{
	struct uba_device *ui = dpvinfo[unit];
	register struct dpvdevice *addr = (struct dpvdevice *)ui->ui_addr;
	register struct dpv_softc *ds = &dpv_softc[unit];
	unsigned char	c;
	int i;

	if ( ds->state == DPVPRETNS){
		ds->state = DPVWRITE;
		ds->counter = 0;
		addr->tdsr = TSOM | (0377 & SYNC);
		return(0);
	}

	/* This should never happen. */
	if ( ds->state == DPVINIT){
		addr->tdsr = TSOM | (0377 & PAD);
		return(0);
	}

	/* Now get ready to receive data. */
	if ( ds->state == DPVDONE){
		for (i=0; i<DPVSIZE; i++)
			ds->buf[i] = NULL;
		ds->counter = 0;
		ds->len = 0;	
		ds->state = DPVREADY;
		addr->pcsar = PROTO | NOCRC | SYNC;
		addr->pcscr = 0;
		addr->tdsr = TEOM;
		addr->rxcsr = DSITEN | RXITEN | RCVEN | DTR;
		return(0);
	}
			
	if( ds->state == DPVWRITE){
		i = ds->counter;
		c = ds->buf[i];
		ds->counter++;
		if ( ds->counter == ds->len )
			ds->state = DPVDONE;
		addr->tdsr = (0377 & c);
		return(0);
	}
}


dpvread(unit)
int unit;

{
	struct uba_device *ui = dpvinfo[unit];
	register struct dpv_softc *ds = &dpv_softc[ui->ui_unit];
	register struct dpvdevice *addr = (struct dpvdevice *)ui->ui_addr;
	struct ifqueue *inq;
	register struct mbuf *m, *m_head;
	int i, j, temp, len;
	struct bsc_data *bm;
	int s;

	/*
	 * If done with a text receive, reset the crc flags.
	 * Copy the ds->buf[] data into an mbuf, schedule an interrupt,
	 * and queue the new mbuf onto the receive queue.
	 */

	MGET(m_head, M_DONTWAIT, MT_DATA);
	if ( m_head == NULL){
		panic(" no mbufs available\n");
		}
	m = m_head;
	/* Now copy from array ds->buf into the mbuf */
	m->m_len = MLEN;
	m->m_off = MMAXOFF - m->m_len;
	bm = mtod(m, struct bsc_data *);
	j = 0;
	temp = ds->counter;
loop1:
	len = m->m_len;
	if ( temp <= len){
		for ( i=0; i < temp; i++)
			bm->data[i] = ds->buf[j++];
		m->m_len = temp;
		m->m_next = 0;
	}else{
		for ( i=0; i < len; i++)
			bm->data[i] = ds->buf[j++];
		temp = temp - len;
		MGET(m->m_next, M_DONTWAIT, MT_DATA);
			if ( m->m_next == NULL){
				panic("dpvread - no mbufs available\n");
			}
		m = m->m_next;
		m->m_len = MLEN;
		m->m_off = MMAXOFF - m->m_len;
		bm = mtod(m, struct bsc_data *);
		goto loop1;
	}

	schednetisr(NETISR_BSC);
	s = splimp();

	inq = &bscintrq;
	if (IF_QFULL (inq)){
		cprintf(" QFULL\n");
		IF_DROP(inq);	
		m_freem(m_head);
		return;
	}
	IF_ENQUEUE(inq,m_head);
	splx(s);

	/* 
	 * Clear the old buffer
	 */
	for (i=0; i<DPVSIZE; i++)
		ds->buf[i] = NULL;
	ds->counter = 0;
	ds->len = 0;
	ds->state = DPVREADY;
	ds->crc = (unsigned short)0;

	addr->pcsar = PROTO | NOCRC | SYNC;
	addr->rxcsr = DSITEN | RXITEN | DTR;
	addr->pcscr = 0;

}

dpvrint(unit)
int unit;
{
	unsigned short	tmp;
	unsigned char c;
	struct uba_device *ui = dpvinfo[unit];
	register struct dpvdevice *addr = (struct dpvdevice *)ui->ui_addr;
	register struct dpv_softc *ds = &dpv_softc[ui->ui_unit];
	struct ifqueue *inq;
	int i;

	tmp = addr->rdsr;
	c = tmp;

	if  (ds->state == DPVPRETNS) {
		if (addr->rxcsr & CTS){
			addr->pcsar = PROTO | NOCRC | SYNC;
			addr->rxcsr = RTS | DTR;
			addr->pcscr = TXITEN | TRAEN;
			addr->tdsr = TSOM | (0377 & SYNC);
			return(0);
		}else
			return(0);
	}

	if (ds->state == DPVREADY) {
		if (addr->rxcsr & CTS) {
			addr->pcsar = PROTO | NOCRC | SYNC;
			addr->rxcsr = RTS | DTR;
			addr->pcscr = TXITEN | TRAEN;
			addr->tdsr = TSOM | (0377 & SYNC);
			return(0);
		}
		if (addr->rxcsr & CARRIER) {
			ds->state = DPVREAD;
			ds->counter = 0;
			addr->pcsar = PROTO | NOCRC | SYNC; 
			addr->pcscr = 0;
			addr->rxcsr = RXITEN | RCVEN | DTR;
			return(0);
		}else
			return(0);	
	}
	/* 
	 * Store the character locally.  Then
	 * see if the character is a control character.  If so, write the
	 * buffer out.
	 */

	switch (c){
	case PAD:
		i = ds->counter;
		if ( ds->buf[i-1] == DLE ){
			ds->buf[i] = c;
			ds->counter++;
			break;
		}
		if (( ds->buf[i-1] == ETB ) || (ds->buf[i-2] == ETB)){
			ds->buf[i] = c;
			ds->counter++;
			break;
		}
		if (( ds->buf[i-1] == ETX ) || (ds->buf[i-2] == ETX)){
			ds->buf[i] = c;
			ds->counter++;
			break;
		}
		if (( ds->buf[i-1] == IUS ) || (ds->buf[i-2] == IUS)){
			ds->buf[i] = c;
			ds->counter++;
			break;
		}
		ds->state = DPVDONE;
		addr->pcsar = PROTO | NOCRC | SYNC;
		addr->pcscr = 0;
		addr->rxcsr = DTR;
		break;
	default:
		i = ds->counter;
		ds->buf[i] = c;
		ds->counter++;
	}

	if (ds->state == DPVDONE)
		dpvread(unit);

	return(0);
}


/*
 * Process an ioctl request.  Routine changed 9/18 to coincide with
 * tcp/ip subnetrouting changes.
 */
dpvioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	int s = splimp(), error = 0;
	
	switch (cmd) {
	
	case SIOCSIFADDR:
		dpvinit(ifp->if_unit);
		ifp->if_flags |= IFF_UP;
		break;
	default:
		error = EINVAL;
	}
	splx(s);
	return(error);
}
#endif
