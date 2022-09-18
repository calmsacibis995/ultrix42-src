#ifndef lint
static	char	*sccsid = "@(#)bsc_states.c	4.1		(ULTRIX)	7/2/90";
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
/*	bsc_states.c 	U. Sinkewicz	1.0	5/10/85			 *
 *					Protocol state processing for the*
 *					2780/3780 Emulator.		 *
 *									 */
/****************************************************************************
 * Input to bsc_output: comes from the socket and is an mbuf with data to 
 *			transmit but no control characters.
 * Input to bsc_input:	comes from the driver is bisync framed data in an mbuf
 *			chain.
 * Output from bsc_output: goes to the driver and is several mbufs of
 *			   different sizes that contain both data and control 
 *			   characters.
 * Output from bsc_input:  goes to the socket and is an mbuf stripped of 
 *		   	   everything but data.  
 ****************************************************************************/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/cpudata.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/mbuf.h"
#include "../h/kernel.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../net/net/if.h"
#include "../net/netbsc/bsc.h"
#include "../net/net/route.h"
#include "../net/netbsc/bsc_var.h"
#include "../net/netbsc/bsc_states.h"
#include "../net/netbsc/bsc_messages.h"
#include "../net/netbsc/bsc_timer.h"
#include "../h/errno.h"
#include "../h/protosw.h"
#include "../h/types.h"
#include "../h/ioctl.h"
#include "../h/uio.h"

unsigned short crctable[16]={0x0000,0xcc01,0xd801,0x1400,0xf001,0x3c00,
		       	     0x2800,0xe401,0xa001,0x6c00,0x7800,0xb401,
			     0x5000,0x9c01,0x8801,0x4400};

int	flag;
unsigned char	crc[2];
char	xfcs0, xfcs1, rfcs0, rfcs1;
char	byte, rcvs, xbcb, rbcb, xseq, ackseq;
char	c1, nakctr, count, punch, rseq, actseq; /* nakctr is number of NAKs
					         * rcvd for one sent block */

short	waiting, exitval, seqst;
short	ct0, ct1;

int bsc_mbuf_len;		/* The number of data characters you can fit 
				 * into m_up_ptr. */
struct mbuf *m_up;		/* Mbuf that contains processed data destined
				 * for the socket.  Characters from m_dvr 
				 * (mbuf from the driver) are copied into 
				 * m_up (mbuf going up to the socket).  
				 * Control characters and the crc 
			   	 * characters are eleminated.  Passed up to 
				 * socket.	*/
struct mbuf *m_up_ptr;  	/* Pointer to the one mbuf in the m_up chain 
				 * that we're currently working with. */
struct bsc_data *dat_up; 	/* Pointer to data area of m_up. */
int data_loc_up;	 	/* Location in the m_up_ptr buffer where you 
				 * are currently writing a character. */
struct bsc_data *dat_dvr; 	/* Pointer to data area of m_dvr. */
int	data_loc_dvr;	  	/* What byte we're currently reading in the
			     	 * data area of m_dvr.		*/
struct mbuf *m_dvr;	  	/* Contains raw data from the driver.  
			     	 * Comes from bscintr().	*/
struct mbuf *m_out;	  	/* Contains data and control characters that 
				 * go to the driver.
				 */
struct 	  bsc_ifaddr	*bsc_ifaddr;

bsc_output(bsc )
	register struct bscpcb *bsc;

{
	struct sockaddr_bsc *sin = &bsc->bscp_laddr;
	struct sockaddr_bsc *dst;
	register struct bsc_ifaddr *bia;
	struct ifaddr *ifa;
	register struct ifnet *ifp;
	extern short state;
	int	error;

	ifp = (struct ifnet *)bsc_ifpfinder( sin );
	if (ifp == 0)
		return(EADDRNOTAVAIL);

	bia = (struct bsc_ifaddr *)bsc_biafinder( ifp );
	if (bia == 0)
		return(EADDRNOTAVAIL);
	crcloc();
	switch(state){
	case MODEM:
		break;
	case REXMT:
		 break;
	case INIT:
		xbcb = 0xa0;
		xfcs0 = 0x8f;
		xfcs1 = 0xcf;
		ackseq = 0x70;
		error = xmtctl(ENQ,NULL,ifp);
		break;
	case SENDBLK:
		error = xmtdata(ifp,bsc);
		bsc->bscp_state = NULL;  
		break;
	}
	return(error);
}

bsc_input( m0 , dst)
	struct mbuf *m0;
	struct sockaddr_bsc *dst;

{
	register struct bsc_ifaddr *bia;
	register struct ifnet *ifp;
	struct bscpcb *bsc;
	struct socket *so;
	extern short state;
	int s;
	int	error;
	struct sockaddr_bsc *sin;
	int saveaffinity;

	saveaffinity = switch_affinity(boot_cpu_mask);
	m_dvr = m0;
	rcvs = rec();
	switch(state){
	case REXMT:
		break;
	case INIT:	
		switch( rcvs ){
		case R_ACK:
			m_free(m0);
			s = splimp();
			state = SENDBLK;
			bsc = bsc_pcblookup(&bsb);
		 	so = bsc->bscp_socket;
			soisconnected(so);  
			splx(s);
			bsc->b_rexmt = 0;
			break;
		default:
			m_free(m0);
			break;
		}
		break;
	case SENDBLKLAST:
		switch(rcvs){
		case R_ACK:
			/* If last block is received on IBM side, then send 
			 * an eot message and wakeup the socket.	*/
			m_free(m0);			
			s = splimp();
			bsc = bsc_pcblookup(&bsb);
			so = bsc->bscp_socket;
			sin = &bsc->bscp_laddr;
			ifp = (struct ifnet *)bsc_ifpfinder( sin );
				if (ifp == 0){
					wakeup((caddr_t)&so->so_timeo); 
					sbdrop(&so->so_snd, so->so_snd.sb_cc); 
					so->so_error = EADDRNOTAVAIL;
					break;
				}
			splx(s);
			ct1 = 0;
			nakctr = 0;
			seqst = R_ACK;
			xbcb = 0x80|xseq;
			incmod(xseq,16);
			wakeup((caddr_t)&so->so_timeo); 
			sbdrop(&so->so_snd, so->so_snd.sb_cc); 
			error = xmtctl(EOT,NULL,ifp);
			/* SENDACK0 is a transition state so get the
			 * ack sequence set up in case we get to bid for the
			 * line.					*/
			ackseq = X70;   
			state = SENDACK0;
			break;
		case R_RVI:
			/* We treat the RVI as an ACK plus line turn around
			 * request.  Therefore we process the RVI exactly like 
			 * and ack.  We don't need to do anything special for
			 * the RVI case.  We don't even need to tell the 
			 * uper layers that an RVI character came in
			 * because we are on the last block and
			 * expect the other side to do a line bid anyway.
			 */
			m_free(m0);			
			s = splimp();
			bsc = bsc_pcblookup(&bsb);
			sin = &bsc->bscp_laddr;
		 	so = bsc->bscp_socket;
			ifp = (struct ifnet *)bsc_ifpfinder( sin );
				if (ifp == 0){
					wakeup((caddr_t)&so->so_timeo); 
					sbdrop(&so->so_snd, so->so_snd.sb_cc); 
					so->so_error = EADDRNOTAVAIL;
					break;
				}
			splx(s);
			ct1 = 0;
			nakctr = 0;
			seqst = R_ACK;
			xbcb = 0x80|xseq;
			incmod(xseq,16);
			wakeup((caddr_t)&so->so_timeo); 
			sbdrop(&so->so_snd, so->so_snd.sb_cc); 
			error = xmtctl(EOT,NULL,ifp);
			ackseq = X70;   
			state = SENDACK0;
			break;
		case R_SEQ:
			m_free(m0);
			ct1 = 0;
			s = splimp();
			bsc = bsc_pcblookup(&bsb);
			so = bsc->bscp_socket;
			sin = &bsc->bscp_laddr;
			splx(s);
			ifp = (struct ifnet *)bsc_ifpfinder( sin );
				if (ifp == 0){
					wakeup((caddr_t)&so->so_timeo); 
					sbdrop(&so->so_snd, so->so_snd.sb_cc); 
					so->so_error = EADDRNOTAVAIL;
					break;
				}
			error = xmtdata(ifp, bsc);
			state = SENDBLKLAST;
			break;
		case R_NAK:
		/* May need some trap here that lets you receive only
		 * x NAKs for the same block.			   */
			m_free(m0);
			nakctr++;
			seqst = R_NAK;
			ct1 = 0;
			if (ackseq == X70)
				ackseq = 0x61;
			else
				ackseq = X70;	
			bsc = bsc_pcblookup(&bsb);
			so = bsc->bscp_socket;
			sin = &bsc->bscp_laddr;
			ifp = (struct ifnet *)bsc_ifpfinder( sin );
				if (ifp == 0){
					wakeup((caddr_t)&so->so_timeo); 
					sbdrop(&so->so_snd, so->so_snd.sb_cc); 
					so->so_error = EADDRNOTAVAIL;
					break;
				}
			if (nakctr >= SEND_BLK_RETRY){
				bsc->b_rexmt = nakctr + 1;
				so->so_error = ECONNABORTED;
				wakeup((caddr_t)&so->so_timeo);
				sbdrop(&so->so_snd, so->so_snd.sb_cc); 	
				break;
			}
			error = xmtdata(ifp, bsc);
			state = SENDBLKLAST;
			break;
		default:
			++ct1;
			if(ct1 >= 6){
				bsc = bsc_pcblookup(&bsb);
				so = bsc->bscp_socket;
				wakeup((caddr_t)&so->so_timeo); 
				sbdrop(&so->so_snd, so->so_snd.sb_cc); 
				so->so_error = ENETDOWN;
				break;
			}else{
				m_free(m0);
				bsc = bsc_pcblookup(&bsb);
				so = bsc->bscp_socket;
				sin = &bsc->bscp_laddr;
				ifp = (struct ifnet *)bsc_ifpfinder( sin );
				if (ifp == 0){
					wakeup((caddr_t)&so->so_timeo); 
					sbdrop(&so->so_snd, so->so_snd.sb_cc); 
					so->so_error = EADDRNOTAVAIL;
					break;
				}
				error = xmtctl(ENQ,NULL,ifp);
				state = SENDBLKLAST;
			}
			break;
		}
		break;
	case SENDBLK:
		switch(rcvs){
		case R_ACK:
			m_free(m0);
			bsc = bsc_pcblookup(&bsb);
			/* It takes a while for the application layer
			 * to generate another block so we need to
			 * distinguish this state in order to send TTDs. */
			bsc->bscp_state = R_ACK;  
			bsc->b_rexmt = 0;	  
			bsc->b_timer = 0;	  
			s = splimp();
			ct1 = 0;
			nakctr = 0;
			seqst = R_ACK;
			xbcb = 0x80|xseq;
			incmod(xseq,16);
		 	so = bsc->bscp_socket;
			wakeup((caddr_t)&so->so_timeo);
			splx(s);
			sbdrop(&so->so_snd, so->so_snd.sb_cc); 
			break;
		case R_RVI:
		/*
		 * Note the wakeup situation here.  We get an RVI then send
		 * an EOT. Then we wakeup the send which falls to the
		 * receive outofband routine which sleeps.  In the meantime
		 * we put the RVI in  the outofband location in the socket
		 * queue.  When data eventually comes in, we wakeup the socket
		 * The socket sees the RVI character and immediately goes to
		 * read what is in the receive queue.
		 */
			m_free(m0);
			ct1 = 0;
			s = splimp();
			bsc = bsc_pcblookup(&bsb);
			sin = &bsc->bscp_laddr;
			splx(s);
		 	so = bsc->bscp_socket;
			ifp = (struct ifnet *)bsc_ifpfinder( sin );
				if (ifp == 0){
					wakeup((caddr_t)&so->so_timeo); 
					sbdrop(&so->so_snd, so->so_snd.sb_cc); 
					so->so_error = EADDRNOTAVAIL;
					break;
				}
			wakeup((caddr_t)&so->so_timeo);
			sbdrop(&so->so_snd, so->so_snd.sb_cc); 
			error = xmtctl(EOT,NULL,ifp);
			bsc_pulloutofband(bsc);
			ackseq = X70;  
			state = SENDACK;
			break;
		case R_NAK:
			m_free(m0);
			nakctr++;
			seqst = R_NAK;
			ct1 = 0;
			/* if (ackseq == X70)
			 *	ackseq = 0x61;
			 * else
			 *	ackseq = X70;	
			 */
			bsc = bsc_pcblookup(&bsb);
			sin = &bsc->bscp_laddr;
			so = bsc->bscp_socket;
			ifp = (struct ifnet *)bsc_ifpfinder( sin );
				if (ifp == 0){
					wakeup((caddr_t)&so->so_timeo); 
					sbdrop(&so->so_snd, so->so_snd.sb_cc); 
					so->so_error = EADDRNOTAVAIL;
					break;
				}
			/* You may want to send an EOT message here */
			if (nakctr >= SEND_BLK_RETRY){
				bsc->b_rexmt = nakctr + 1;
				so->so_error = ECONNABORTED;
				wakeup((caddr_t)&so->so_timeo);
				sbdrop(&so->so_snd, so->so_snd.sb_cc); 	
				break;
			}
			/* We are sending a TTD from the timers.  Just let
			 * the timers continue to do this.		*/
			if (bsc->bscp_state == R_ACK)  
				break;
			error = xmtdata(ifp, bsc);
			state = SENDBLK;
			break;
		case R_SEQ:
			m_free(m0);
			ct1 = 0;
			s = splimp();
			bsc = bsc_pcblookup(&bsb);
			sin = &bsc->bscp_laddr;
			so = bsc->bscp_socket;
			splx(s);
			ifp = (struct ifnet *)bsc_ifpfinder( sin );
				if (ifp == 0){
					wakeup((caddr_t)&so->so_timeo); 
					sbdrop(&so->so_snd, so->so_snd.sb_cc); 
					so->so_error = EADDRNOTAVAIL;
					break;
				}
			error = xmtdata(ifp, bsc);
			state = SENDBLK;
			break;
		default:
			++ct1;
			if(ct1 >= 6){
				bsc = bsc_pcblookup(&bsb);
				so = bsc->bscp_socket;
				wakeup((caddr_t)&so->so_timeo); 
				sbdrop(&so->so_snd, so->so_snd.sb_cc); 
				so->so_error = ENETDOWN;
				break;
			}else{
				m_free(m0);
				bsc = bsc_pcblookup(&bsb);
				sin = &bsc->bscp_laddr;
				so = bsc->bscp_socket;
				ifp = (struct ifnet *)bsc_ifpfinder( sin );
				if (ifp == 0){
					wakeup((caddr_t)&so->so_timeo); 
					sbdrop(&so->so_snd, so->so_snd.sb_cc); 
					so->so_error = EADDRNOTAVAIL;
					break;
				}
				error = xmtctl(ENQ,NULL,ifp);
				state = SENDBLK;
			}
			break;
		}
		break;
	case SENDACK0:
		switch(rcvs){
		case ENQ:
			m_free(m0);			
			s = splimp();
			bsc = bsc_pcblookup(&bsb);
			so = bsc->bscp_socket;
			bsc_pulloutofband(bsc);
			splx(s);
			ackseq = X70;
			error = xmtack(bsc); 
			ct1 = 0;
			seqst = R_ACK;
			xbcb = 0x80|xseq;
			incmod(xseq,16);
			state = SENDACK;
			break;
		case R_ACK:	
			/* Let the timers keep the line alive.  If
			 * the timers kick in to do a line bid that
			 * means that the other side doesn't want to
			 * to send a file so we go to SENDBLK state.
			 */
			m_free(m0);
			s = splimp();
			ct1 = 0;
			seqst = R_ACK;
			xbcb = 0x80|xseq;
			incmod(xseq,16);
			bsc = bsc_pcblookup(&bsb);
		 	so = bsc->bscp_socket;
			wakeup((caddr_t)&so->so_timeo);
			splx(s);
			break;
		default:
			++ct1;
			if(ct1 >= 6){   
				m_free(m0);  
				s = splimp();
				bsc = bsc_pcblookup(&bsb);
				splx(s);
				so = bsc->bscp_socket;
				so->so_error = ENETDOWN; 
				bsc->b_timer = 0;	
				wakeup((caddr_t)&so->so_timeo);	
				exitval = ERRORS;
			}
		break;
		}
		break;
	case SENDACK:
		switch(rcvs){
		case ENQ:
			m_free(m0);			
			s = splimp();
			bsc = bsc_pcblookup(&bsb);
			splx(s);
			bsc->b_rexmt++;
			so = bsc->bscp_socket;
			error = xmtack(bsc); 
			if (error && (bsc->b_rexmt >= SEND_BLK_RETRY)){
				bsc->b_rexmt = 0;
				so->so_error = ECONNABORTED;
				wakeup((caddr_t)&so->so_timeo);	
				break;	
			}
			ct1 = 0;
			seqst = R_ACK;
			xbcb = 0x80|xseq;
			incmod(xseq,16);
			state = SENDACK;
			break;
		case R_OKBLK:
			m_freem(m0); 
			s = splimp();
			bsc = bsc_pcblookup(&bsb);
			so = bsc->bscp_socket;
			bsc->b_rexmt = 0;
			splx(s);
			if (ackseq == X70)
				ackseq = 0x61;
			else
				ackseq = X70;
			/* An error in sending here means a message didn't get
			 * out.  The other side must send ENQ in response so
			 * let case ENQ handle the retransmit.	 */
			error = xmtack(bsc); 
			sbappend(&so->so_rcv, m_up);
			wakeup((caddr_t)&so->so_timeo);	
			break;
		case R_ERBLK:
			m_freem(m0);
			m_freem(m_up);   
			ct1 = 0;
			bsc = bsc_pcblookup(&bsb);
			bsc->b_rexmt++;
			so = bsc->bscp_socket;
			if (bsc->b_rexmt >= SEND_BLK_RETRY){
				so->so_error = ECONNABORTED;
				bsc->b_rexmt = 0;
				wakeup((caddr_t)&so->so_timeo);
				break;
			}
			sin = &bsc->bscp_laddr;
			ifp = (struct ifnet *)bsc_ifpfinder( sin );
				if (ifp == 0){
					so->so_error = EADDRNOTAVAIL;
					bsc->b_rexmt = 0;
					wakeup((caddr_t)&so->so_timeo);
					break;
				}
			error = xmtctl(NAK,NULL,ifp);
			if (error)
				bsc->b_rexmt--;
			state = SENDACK;
			break;
		case R_NAK:
			m_freem(m0);			
			ct1 = 0;
			seqst = R_NAK;
			bsc = bsc_pcblookup(&bsb);
			so = bsc->bscp_socket;
			sin = &bsc->bscp_laddr;
			ifp = (struct ifnet *)bsc_ifpfinder( sin );
				if (ifp == 0){
					so->so_error = EADDRNOTAVAIL;
					bsc->b_rexmt = 0;
					wakeup((caddr_t)&so->so_timeo);
					break;
				}
			error = xmtctl(DLE,X70,ifp);
			state = SENDACK;
			break;
		case EOT:
			m_free(m0);			
			s = splimp();
			bsc = bsc_pcblookup(&bsb);
			so = bsc->bscp_socket;
			bsc_pulloutofband(bsc);
			splx(s);
			ct1 = 0;
			seqst = R_ACK;
			xbcb = 0x80|xseq;
			incmod(xseq,16);
			state = SENDACK0;
			/* SENDACK0 is a transition state so get the
			 * ack sequence set up in case we get to bid for the
			 * line.					*/
			ackseq = X70;   
			wakeup((caddr_t)&so->so_timeo);
			break;
		default:
			++ct1;
			m_freem(m0); 
			if(ct1 >= 6){
				bsc = bsc_pcblookup(&bsb);
				so = bsc->bscp_socket;
				so->so_error = ENETDOWN;
				bsc->b_rexmt = 0;
				wakeup((caddr_t)&so->so_timeo);
				break;
			}else{
				bsc = bsc_pcblookup(&bsb);
				so = bsc->bscp_socket;
				sin = &bsc->bscp_laddr;
				ifp = (struct ifnet *)bsc_ifpfinder( sin );
					if (ifp == 0){
						so->so_error = EADDRNOTAVAIL;
						bsc->b_rexmt = 0;
						wakeup((caddr_t)&so->so_timeo);
						break;
					}
				error = xmtctl(NAK,NULL,ifp);
				state = SENDACK;
			}
			break;
		}
		break;
	}
}

/* Xmtdata prepares data for transmission to the protocol.  When we get
 * the data at this level, there are a few flags in the data indicating
 * transparent/nontransparent and eof/eobuffer.  The 0th location in the
 * first mbuf of data indicates transparent communication if its value is
 * 1.  We note the value here and then delete it from the buffer so it
 * won't be transmitted as data.  The last location in the last mbuf we
 * get tells us if this is the last buffer in the file or not.  Again,
 * we see what the value is, take the proper action, and then delete so it 
 * won't be transmitted as data.
 */

xmtdata( ifp, bsc)
	struct ifnet *ifp;
	struct bscpcb *bsc;
{
	struct socket *so;
	struct sockaddr_bsc *dst;
	int i = 0;
	int j = 0;
	char c;
	struct mbuf *m, *mp, *m_head, *m_new;
	int len, off, error;
	int d_flag = 0;
	struct bsc_data *bm, *bmtmp;
	int saveaffinity;

	so = bsc->bscp_socket;
	crcloc();
	bmtmp = mtod(so->so_snd.sb_mb, struct bsc_data *);
	if (bmtmp->data[0] == DLE)
		d_flag = 1;
	else
		d_flag = 0;
	m_head = m_getclr(M_DONTWAIT, MT_DATA);
	if ( m_head == (struct mbuf *)NULL ){
		return(ENOBUFS);
	}
	m_head->m_len = MLEN;
	m_head->m_off = MMAXOFF - m_head->m_len; 

	bmtmp = mtod(m_head, struct bsc_data *);
	if (d_flag){	/* transparent */
		bmtmp->data[i++] = DLE;
		bmtmp->data[i++] = STX;
		m_head->m_len = (unsigned)i;
		j = 1;
	}else{		/* nontransparent */
		bmtmp->data[i++] = STX;
		m_head->m_len = (unsigned)i;
		j = 0;
	}
	m_head->m_next = m_copy(so->so_snd.sb_mb, j, M_COPYALL);
	m = m_head->m_next;
	bm = mtod(m, struct bsc_data *);
	len = m->m_len;
	j = 0;
loop:
	for ( j; j < len; j++ ){ 
		c = bm->data[j];
		if ((c==ETX) && (bm->data[j-1]!=DLE) && (m->m_next==NULL)){
			state = SENDBLKLAST;	
			break;
		}
		/* We've seen the ETB indicator in the buffer so escape. */
		if ((c== ETB) && (bm->data[j-1]!=DLE))
			break;
	 	if (( c == DLE) || (c == SYNC))
			continue;
		crc16(c); 
		if (( c == ITB) && (d_flag == 0))
			break;
	}

	if ( c == ITB ){
		j++;
		m->m_len = j;
		m_new = m_getclr(M_DONTWAIT, MT_DATA);	
		if ( m_new == (struct mbuf *)NULL )
			return(ENOBUFS);
		m_new->m_len = MLEN;
		m_new->m_off = MMAXOFF - m_new->m_len; 

		bmtmp = mtod(m_new, struct bsc_data *);
		bmtmp->data[0] = crc[0];
		bmtmp->data[1] = crc[1];
		crcloc();
		/* if (d_flag){
		 *	bmtmp->data[2] = SYN;
		 *	bmtmp->data[3] = SYN;
		 *	bmtmp->data[4] = DLE;
		 *	crc16(c);
		 *	bmtmp->data[5] = STX;
		 *	crc16(c);
		 *	i = 6;
		 * }else{
		 *	bmtmp->data[2] = SYN;
		 *	bmtmp->data[3] = SYN;
		 *	i = 4;	     
		 * }
		 */
		i = 2;
		for (j; j<len; j++)
			bmtmp->data[i++] = bm->data[j];
		m_new->m_len = i;
		if (m->m_next == NULL){
			m->m_next = m_new;
			m_new->m_next = NULL;
			m = m->m_next;
			len = m->m_len;
			bm = mtod(m, struct bsc_data *);
			j = 2;
			goto loop;
		}
		if (m->m_next != NULL){
			mp = m->m_next;
			m->m_next = m_new;
			m_new->m_next = mp;
			m = m->m_next;
			len = m->m_len;
			bm = mtod(m, struct bsc_data *);
			j = 2;
			goto loop;
		}
	}

	if ( (m->m_next != NULL) ){
		mp = m->m_next;
		len = mp->m_len;
		bm = mtod(mp, struct bsc_data *);
		m = mp;	
		if ( c != ITB)
			j = 0;
		goto loop;
	}
	/* Delete the marker in the buffer indicating EOF or EOB */
	m->m_len = m->m_len - 1; 
	m->m_next = m_getclr(M_DONTWAIT, MT_DATA);
	if ( m->m_next == (struct mbuf *)NULL ){
		return(ENOBUFS);
	}
	m->m_next->m_len = MLEN;
	m->m_next->m_off = MMAXOFF - m->m_next->m_len; 

	bmtmp = mtod(m->m_next, struct bsc_data *);
	i = 0;	
	if (state == SENDBLKLAST){
		if (d_flag){
			bmtmp->data[i++] = DLE;
			bmtmp->data[i++] = ETX;
			crc16(ETX);
		}else{
			bmtmp->data[i++] = ETX;
			crc16(ETX);
		}
	}else{
		if (d_flag){
			bmtmp->data[i++] = DLE;
			bmtmp->data[i++] = ETB;
			crc16(ETB);
		}else{
			bmtmp->data[i++] = ETB;
			crc16(ETB);
		}
	}

	bmtmp->data[i++] = crc[0];
	bmtmp->data[i++] = crc[1];
	m->m_next->m_len = i; 

	CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
	error = (*ifp->if_output)(ifp, m_head, (struct sockaddr *)dst);
	RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
	m_freem(m_head);
	return(error);
}
	
/*
 * Xmtctl is a generic routine that generates and transmits a message
 * containing one or two control characters plus sync and ending pad
 * characters.  Use this routine to transmit, for example, an EOT message.
 */

xmtctl(byte1, byte2, ifp)
	char byte1, byte2;
	register struct ifnet *ifp;

{
	struct mbuf *m_tmp;	/* Temp mbuf for control characters */
	int i = 0;
	struct sockaddr_bsc *dst;
	int	error;
	struct bsc_data *bdat;
	int saveaffinity;

	m_tmp = m_getclr(M_DONTWAIT, MT_DATA);
	if ( m_tmp == (struct mbuf *)NULL ){
		return(ENOBUFS);
	}
	m_tmp->m_len = MLEN;
	m_tmp->m_off = MMAXOFF - m_tmp->m_len; 
	bdat = mtod(m_tmp, struct bsc_data *);
	bdat->data[i++] = byte1;
	if ( byte2 != NULL )
		bdat->data[i++] = byte2;

	bdat->data[i++] = PAD;
 	bdat->data[i++] = PAD;

	m_tmp->m_len = (unsigned)i;

	CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
	error = (*ifp->if_output)(ifp, m_tmp, (struct sockaddr *)dst);
	RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);

	m_free(m_tmp);
	return(error);
}

/* 
 * Rec() is a key routine on the receive side.  Bscrcv tells what the data is
 * and rec processes that data.  For control messages coming in, rec
 * sees if the message is okay and returns either success or failure
 * to the case statements in bsc_input.  For data coming in, rec() calls
 * routines to get a clean mbuf (getrbuf) to write only the
 * data to and then rec() calls a routine to 'put()' the data to the
 * clean mbuf.  
 */

rec()
{
	crcloc();

	dat_dvr = mtod(m_dvr, struct bsc_data *);

	rsom(SYNC);
	bscrcv();
	switch(byte){
	case EOT:
		return(EOT);
		break;
	case ENQ:
		return(ENQ);
		break;
	case NAK:
		return(R_NAK);
		break;
	case STX:
			getrbuf();
			byte = rcvdat();
			compress();	
			while ((byte != ETB) && (byte != ETX)){
				if (flag == 1){
					break;
				}
				if((byte&0x05) == 0x05){
					punch = 1;
					put();
				}else{
					punch = 0;
					put();
				}
				/* if(byte == 0){
				 *	break;
				 * }
				 */
				byte = rcvdat();
				compress();	
				while ((byte != ETB) && (byte != ETX)){
					if (flag == 1){
						break;
					}
					if ( byte == IUS){
						rcvcrc(0);
						if(crc[0] != 0){
							return(R_ERBLK);
						}
						rcvcrc(1);
						if(crc[1] != 0){
							return(R_ERBLK);
						}
						crcloc();
					}
					/* if(byte == 0){
 					 *	break;
					 * }
					 */
					put();
					byte = rcvdat();
					compress();	
			} 
		} 
		rcvcrc(0);
		rcvcrc(1);
		bscrcv();
		if(crc[0] != 0){
			return(R_ERBLK);
		 }
		if(crc[1] != 0){
			return(R_ERBLK);
		}
		return(R_OKBLK);
		break;
	case DLE:
		bscrcv();
		switch(byte){
		case X70:
			if (ackseq != X70)
				return(R_SEQ);

			/* If timers need to kick in while state is SENDACK0
			 * the following if statement will prevent the ackseq
			 * from being bumped.  Without this if statement you
			 * would always be out of sequence.
			 */
			if ( state == SENDACK0) 
				return(R_ACK);
			ackseq = 0x61;
			return(R_ACK);
			/* else
			 *	return(R_ERROR);
			 */
			break;
		case 0x61:	
			if (ackseq != 0x61)
				return(R_SEQ);
			ackseq = X70;
			return(R_ACK);
			/* else
			 *	return(R_ERROR);
			 */
			break;
		case 0X7c:	
			/* bsc_pulloutofband(bsc); */
			return(R_RVI);
			break;
		case ENQ: 
			/* bsc_pulloutofband(bsc); */
			return(ENQ);
			break;
		case STX:
			getrbuf();
			byte = rcvdat();
			compress();	
			while ((byte != ETB) && (byte != ETX)){
				if (flag == 1){
					break;
				}
				if((byte&0x05) == 0x05){
					punch = 1;
					put();
				}else{
					punch = 0;
					put();
				}
				/* if(byte == 0){
				 *	break;
				 * }
				 */
				byte = rcvdat();
				compress();	
				while ((byte != ETB) && (byte != ETX)){
					if (flag == 1){
						break;
					}
					if ( byte == IUS){
						rcvcrc(0);
						if(crc[0] != 0){
							return(R_ERBLK);
						}
						rcvcrc(1);
						if(crc[1] != 0){
							return(R_ERBLK);
						}
						crcloc();
					}
					/* if(byte == 0){
 					 *	break;
					 * }
					 */
					put();
					byte = rcvdat();
					compress();	
				} 
			} 
			rcvcrc(0);
			rcvcrc(1);
			bscrcv();
			if(crc[0] != 0){
				return(R_ERBLK);
			 }
			if(crc[1] != 0){
				return(R_ERBLK);
			}
			return(R_OKBLK);
			break;
		default:
			return(R_ERROR);
		}
		break;
	default:
		return(R_ERROR);
	}
}


/* Do all compression interpretation here except for HT.  HT in a format
 * record is handled by the daemon.
 */
compress()
{
	char byte_save;

loopss:
	if (byte==ESC){
		byte = rcvdat();
		switch(byte){
		case 0x61:  /* / */
			byte = LF;
			put();
			break;
		case 0xe2: /* S */
			byte = LF;
			put();
			byte = LF;
			put();
			break;
		case 0xe3: /* T */
			byte = 0x40;
			put();
			byte = 0x40;
			put();
			byte = 0x40;
			put();
			break;
		case 0xc1: /* A */
			byte = 0x0c;
			put();
			break;
		default:
			byte_save = byte;
			byte = ESC;
			put();
			byte = byte_save;
			put();
			break;
		} 
	byte = rcvdat();
	if (byte == ESC)
		goto loopss;
	} /* if */
	else 
		return;
}

rcvcrc(i)
int i;
{
	bscrcv();
	crc[i] = crc[i] - (unsigned)byte;
	return(0);
}

rcvdat()
{

	for (;;){
		bscrcv();
		if(byte != DLE) {
			if(byte == SYNC)
				continue;
			crc16(byte);
			return(byte);
		} else {
			bscrcv();
			if(byte == SYNC)
				continue;
			crc16(byte);
			return(byte);
		}
	}

 }


xmtack(bsc)
	struct bscpcb *bsc;
{
	struct ifnet *ifp;
	struct sockaddr_bsc *sin = &bsc->bscp_laddr;
	struct mbuf *m_tmp;
	struct sockaddr_bsc *dst;
	struct bsc_data *bdat;
	int error;
	int i = 0;
	int saveaffinity;

	ifp = (struct ifnet *)bsc_ifpfinder( sin );
	if (ifp == 0)
		return(EADDRNOTAVAIL);

	m_tmp = m_getclr(M_DONTWAIT, MT_DATA);
	if ( m_tmp == (struct mbuf *)NULL ){
		return(ENOBUFS);
	}
	m_tmp->m_len = MLEN;
	m_tmp->m_off = MMAXOFF - m_tmp->m_len;
	bdat = mtod(m_tmp, struct bsc_data *);
	bdat->data[i++] = DLE;
	bdat->data[i++] = ackseq;
 	bdat->data[i++] = PAD;

	m_tmp->m_len = (unsigned)i;

	CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
	error = (*ifp->if_output)(ifp, m_tmp, (struct sockaddr *)dst);
	RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);

	m_free(m_tmp); 
	return(error);
}

crcloc(  )
{
	crc[0]=0;
	crc[1]=0;  
}

/*
 * Strip off the leading rec_ch (sync) characters from the mbuf you get
 * from the driver.
 */
rsom( rec_ch )
	char rec_ch;
{
	int i = 0;

	data_loc_dvr = 0;
	while ( dat_dvr->data[i] == rec_ch ){
		i++;
		}
	data_loc_dvr = i;
	flag = 0;
	return (0);
}

/*
 * Get a character from the mbuf you get from the driver.
 */
bscrcv()
{
	if ( m_dvr->m_len == data_loc_dvr){
		if (m_dvr->m_next == NULL){
			if ( byte == ETX)
				flag = 1;
			if (byte == ETB)
				flag = 1;
			return(0);
		}
		m_dvr = m_dvr->m_next;
		data_loc_dvr = 0;
		dat_dvr = mtod(m_dvr, struct bsc_data *);
	}
	byte = dat_dvr->data[data_loc_dvr++];
	flag = 0;
	return(0);
}

/*
 * Get an mbuf that will be used only for the purpose of collecting incoming
 * data that has been stripped of bisync control characters.
 */
getrbuf()
{
	int i;

	m_up = m_get(M_DONTWAIT, MT_DATA);
	if ( m_up == (struct mbuf *)NULL ){
		return(ENOBUFS);
	}
	m_up->m_len = MLEN;
	m_up->m_off = MMAXOFF - m_up->m_len;
	bsc_mbuf_len = m_up->m_len;
	data_loc_up = 0;
	dat_up = mtod(m_up, struct bsc_data *);
	/* Before storing data, be sure entire buffer is clean */
	for (i=0; i<bsc_mbuf_len; i++)
		dat_up->data[i] = 0x40;
	m_up_ptr = m_up;		/* save a pointer to the head of the
					 * receive queue.  The pointer you save
					 * is what gets passed up to the socket
					 * receive queue.                    */
	return(0);
}

/*
 * Now write only data characters to m_up.  Put data characters only from m_dvr
 * the mbuf you get from the driver, into m_up. You may want to do some buffer
 * size checking in here.
 */
put()
{
	struct mbuf *m_temp;
	int i;

	if (bsc_mbuf_len == data_loc_up){
		m_temp = m_get(M_DONTWAIT, MT_DATA);
		if ( m_temp == (struct mbuf *)NULL )
			return(ENOBUFS);
		m_temp->m_len = MLEN;
		m_temp->m_off = MMAXOFF - m_temp->m_len;
		data_loc_up = 0;
		bsc_mbuf_len = m_temp->m_len;
		/* Now link m_temp into the m_up chain and reset 
		 * m_up_ptr to point to the new mbuf.	      */
		m_up_ptr->m_next = m_temp;
		m_up_ptr = m_up_ptr->m_next;
		dat_up = mtod(m_up_ptr, struct bsc_data *);
		/* Before storing data, be sure entire buffer is clean  */
		for (i=0; i<bsc_mbuf_len; i++)
			dat_up->data[i] = 0x40;
	}
	dat_up->data[data_loc_up++] = byte;
	return(0);
}


/* crc16 takes 1 character as input and updates the CRC[] for the whole
   message */

crc16(crc_ch)
	char crc_ch;
{
	unsigned short tmp,result;

	result= (crc[1] & 0x00ff) << 8; 	/* put crc[] into result, */
	result |= (crc[0] & 0x00ff);		/* ie  result=crc[0-1] */

	tmp= (0xffff & result) ^ (0xff & crc_ch); /* next 3 - calc. new crc */
	tmp= (0xfff & (tmp >> 4)) ^ crctable[tmp & 0xf];
	result= (0xfff & (tmp >>4)) ^ crctable[tmp & 0xf];

	crc[0]=(result & 0x00ff);		/* put new crc in crc[] */
        crc[1]=(result & 0xff00) >> 8;

}

/*
 * Pull out of band byte out of a segment.
 */
bsc_pulloutofband(bsc)
	struct bscpcb *bsc;
{
	struct socket *so;
	register struct mbuf *m;

	so = bsc->bscp_socket;
	bsc->b_iobc = byte;
	so->so_oobmark = so->so_rcv.sb_cc + 1;
}

bscintr()
{
/* 
 * This is where the driver input routine returns.
 * It calls the mother routine, bsc_states to respond to the incoming 
 * message.
 */

	register struct mbuf *m;
	int s;
	struct sockaddr_bsc *sin;
	struct ifnet *ifp;
	int saveaffinity;

	saveaffinity = switch_affinity(boot_cpu_mask);
	/*
	 * Get datagram off input queue.
	 */
	s = splimp();
	IF_DEQUEUE(&bscintrq, m);
	splx(s);

	if (ifp)
		sin = (struct sockaddr_bsc *)&ifp->if_addr;

	(*bscnetsw[1].pr_input)(m, sin);

}





