#ifndef lint
static char *sccsid = "@(#)if_dup.c	4.1	ULTRIX	7/2/90";
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

#include "dup.h"
#if NDUP > 0 || defined(BINARY)

/*
 *   Driver for the DUP-11, a synchronous character-oriented data 
 *	communications device, supported here for the bisync protocol.
 *
 *   Written  by	U. Sinkewicz		4/10/85
 */
/************************************************************************
 *			Modification History
 *
 *   18 August 1986 - 	U. Sinkewicz
 * 			reset routine did not initialize every dup
 *			device on the bus.  changed device id from
 *			NDUP to nNDUP in dupreset.
 ************************************************************************/

#include "../data/if_dup_data.c"

int dupdebug = 1;
extern struct protosw *iftype_to_proto(), *iffamily_to_proto();
/* extern struct timeval time; */



/* 
 * Driver information for autoconfigure.
 */
int	dupprobe(), dupattach(), duprint(), dupxint(), dupinit();
struct dupdevice *dupaddr;
struct	uba_device   *dupinfo[NDUP];
u_short	dupstd[] = { 0 };
struct uba_driver	dupdriver =
	 { dupprobe, 0, dupattach, 0, dupstd, "dup", dupinfo };

#define DUPUNIT(x)	minor(x)
int dupoutput(), dupinput(), dupreset(), dupioctl();


/*
 * Probe routine.  Generate an interrupt, wait around for the cpu to
 * service it, and reset the device registers.
 */
dupprobe(reg)
caddr_t reg;
{
	register struct dupdevice *dupaddr = (struct dupdevice *)reg;

#ifdef lint
	duprint(0); dupxint(0);
#endif

	dupaddr->pcsr = DEC | NOCRC | SYNC;
	dupaddr->rcsr = RTS | DTR;
	dupaddr->tcsr = TXITEN | TRAEN;
	DELAY(100000);
	dupaddr->pcsr = 0;
	dupaddr->rcsr = 0;
	dupaddr->tcsr = DEVRST;
	if (cvec && cvec != 0x200){
		cvec -= 4;
	}
	return(1);
	/* return (sizeof (struct dupdevice)); */
}



/*
 * Fill in the network interface record.  The system will initialize the
 * interface when it is ready to accept packets.
 */
dupattach(ui)
struct uba_device *ui;
{
	register struct dup_softc *ds = &dup_softc[ui->ui_unit];

ds->dup_if.if_unit = ui->ui_unit;
ds->dup_if.if_name = "dup";
ds->dup_if.if_init = dupinit;
ds->dup_if.if_output = dupoutput;
ds->dup_if.if_ioctl = dupioctl;
ds->dup_if.if_reset = dupreset;
if_attach(&ds->dup_if);
}


/*
 * Reset state of driver if a unibus reset occurrs.  
 */
dupreset(unit, uban)
int unit, uban;
{
	register struct uba_device *ui;
	register struct dupdevice *addr;

	if (unit >= nNDUP || (ui = dupinfo[unit])== 0 || ui->ui_alive == 0 ||
		ui->ui_ubanum != uban)
		return;

	addr->pcsr = 0;
	addr->rcsr = 0;
	addr->tcsr = DEVRST;

	printf("dup%d\n",unit);
	dupinit(unit);
}


/*
 * Initialize the device and unibus usage.
 */
dupinit(unit)
int unit;
{
	register struct dup_softc *ds = &dup_softc[unit];
	struct uba_device *ui = dupinfo[unit];
	register struct dupdevice *addr = (struct dupdevice *)ui->ui_addr;
	int s, i;

	/* Dupinit is called at boot time and at unibus reset. */

	if (ds->dup_if.if_addrlist == (struct ifaddr *)0){
		return; 
	}

	/*
	 * Allow receive interrupt when CTS comes up and set up for
	 * dialing.
	 */
	addr->pcsr = DEC | NOCRC | SYNC;
	addr->rcsr = DSIE | DTR;
	addr->tcsr = 0;

	/* 
	 * Initialize the counter in the software context structure and
	 * clear the buffer.
	 */
	ds->counter = 0;
	for (i=0; i<DUPSIZE; i++)
		ds->buf[i] = NULL;

	ds->state = DUPINIT;

}


dupoutput(ifp, m0, dst)
	register struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;

{
	struct mbuf *m;
	register struct dup_softc *ds = &dup_softc[ifp->if_unit];
	struct protosw *pr;
	int s, i, error;

	m = m0;

	/* 
	 * Initialize the counter in the software context structure and
	 * clear the buffer.
	 */
	ds->counter = 0;
	for (i=0; i<DUPSIZE; i++)
		ds->buf[i] = NULL;

	ds->state = DUPREADY;

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
	ds->state = DUPREADY;
	dupstart(ifp->if_unit);
	return(0);
}


/*
 * DUP start routine.  Should be called at priority 5.
 */

dupstart(dev)
dev_t dev;
{
	int unit = DUPUNIT(dev);
	struct uba_device *ui = dupinfo[unit];
	register struct dup_softc *ds = &dup_softc[unit];
	register struct dupdevice *addr = (struct dupdevice *)ui->ui_addr;
	struct mbuf *m, *mp;
	unsigned char c;
	int i; 		/* counter */
	int j = 0;  	/* counter */
	int s;
	struct bsc_data *bdata;
	int len;

	s = splimp();
	IF_DEQUEUE(&ds->dup_if.if_snd, m);
		if ( m == 0){
			ds->state = DUPDONE;
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

	ds->state = DUPPRETNS;

	/* 
	 * Reset the device here because if it's a retranxmit you
	 * need to disable receive interrupts from the device.
	 */
	addr->pcsr = DEC | NOCRC | SYNC;
	addr->rcsr = DSIE | RTS | DTR;
	if (addr->rcsr & CTS){
		addr->tcsr = TXITEN | TRAEN;  
		addr->tbuf = TSOM | (0377 & SYNC);
	}
} 


/*
 * Transmit interrupt routine.  Take characters from the buffer provided
 * by the start routine and put them in the device's transmit buffer.
 */
dupxint(unit)
int unit;
{
	struct uba_device *ui = dupinfo[unit];
	register struct dupdevice *addr = (struct dupdevice *)ui->ui_addr;
	register struct dup_softc *ds = &dup_softc[unit];
	unsigned char	c;
	int i;

	if ( ds->state == DUPPRETNS){
		ds->state = DUPWRITE;
		ds->counter = 0;
		addr->tbuf = TSOM | (0377 & SYNC);
		return(0);
	}

	/* This should never happen. */
	if ( ds->state == DUPINIT){
		addr->tbuf = TSOM | (0377 & PAD);
		return(0);
	}

	/* Now get ready to receive data. */
	if ( ds->state == DUPDONE){
		for (i=0; i<DUPSIZE; i++)
			ds->buf[i] = NULL;
		ds->counter = 0;
		ds->len = 0;
		ds->state = DUPREADY;
		addr->pcsr = DEC | NOCRC | SYNC;
		addr->tcsr = 0;
		addr->tbuf = TEOM;
		addr->rcsr = DSIE | RXITEN | RCVEN | DTR;
		return(0);
	}
		
	if( ds->state == DUPWRITE){
		i = ds->counter;
		c = ds->buf[i];
		ds->counter++;
		if ( ds->counter == ds->len )
			ds->state = DUPDONE;
		addr->tbuf = (0377 & c);
		return(0);
	}
}


dupread(unit)
int unit;

{
	struct uba_device *ui = dupinfo[unit];
	register struct dup_softc *ds = &dup_softc[ui->ui_unit];
	register struct dupdevice *addr = (struct dupdevice *)ui->ui_addr;
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
			if ( m->m_next == NULL)
				panic(" no mbufs available\n");
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
	for (i=0; i<DUPSIZE; i++)
		ds->buf[i] = NULL;
	ds->counter = 0;
	ds->len = 0;
	ds->state = DUPREADY;
	ds->crc = (unsigned short)0;

	addr->pcsr = DEC | NOCRC | SYNC;
	addr->rcsr = DSIE | DTR;
	addr->tcsr = 0;

}

duprint(unit)
int unit;
{
	unsigned short	tmp;
	unsigned char c;
	struct uba_device *ui = dupinfo[unit];
	register struct dupdevice *addr = (struct dupdevice *)ui->ui_addr;
	register struct dup_softc *ds = &dup_softc[ui->ui_unit];
	struct ifqueue *inq;
	int i;

	tmp = addr->rbuf;
	c = tmp;

	if (ds->state == DUPPRETNS) {
		if (addr->rcsr & CTS){
			addr->pcsr = DEC | NOCRC | SYNC;
			addr->rcsr = RTS | DTR;
			addr->tcsr = TXITEN | TRAEN;
			addr->tbuf = TSOM | (0377 & SYNC);
			return(0);
		}else
			return(0);
	}
	if (ds->state == DUPREADY) {
		if (addr->rcsr & CTS){
			addr->pcsr = DEC | NOCRC | SYNC;
			addr->rcsr = RTS | DTR;
			addr->tcsr = TXITEN | TRAEN;
			addr->tbuf = TSOM | (0377 & SYNC);
			return(0);
		}
		if (addr->rcsr & CARRIER){
			ds->state = DUPREAD;
			ds->counter = 0;
			addr->pcsr = DEC | NOCRC | SYNC;
			addr->tcsr = 0;
			addr->rcsr = RXITEN | RCVEN | DTR;
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
		if (ds->buf[i-1] == DLE){
			ds->buf[i] = c;
			ds->counter++;
			break;
		}
		if ((ds->buf[i-1] == ETB) || (ds->buf[i-2] == ETB)){
			ds->buf[i] = c;
			ds->counter++;
			break;
		}
		if ((ds->buf[i-1] == ETX) || (ds->buf[i-2] == ETX)){
			ds->buf[i] = c;
			ds->counter++;
			break;
		}
		if ((ds->buf[i-1] == IUS) || (ds->buf[i-2] == IUS)){
			ds->buf[i] = c;
			ds->counter++;
			break;
		}
		ds->state = DUPDONE;
		addr->pcsr = DEC | NOCRC | SYNC;
		addr->tcsr = 0;
		addr->rcsr = DTR;
		break;
	default:
		i = ds->counter;
		ds->buf[i] = c;
		ds->counter++;
	}

	if (ds->state == DUPDONE)
		dupread(unit);

	return(0);
}

/*
 * Process an ioctl request.  Routine changed 9/18 to coincide with
 * tcp/ip subnetrouting changes.
 */
dupioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	int s = splimp(), error = 0;
	
	switch (cmd) {
	
	case SIOCSIFADDR:
		dupinit(ifp->if_unit);
		ifp->if_flags |= IFF_UP;
		break;
	default:
		error = EINVAL;
	}
	splx(s);
	return(error);
}

#endif
