#ifndef	lint
static char *sccsid = "@(#)dli_usrreq.c	4.4	ULTRIX	12/6/90";
#endif	lint

/*
 * Program dli_usrreq.c,  Module DLI 
 *
 * Copyright (C) 1985 by
 * Digital Equipment Corporation, Maynard, Mass.
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Digital Equipment
 * Corporation.
 *
 * Digital assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by Digital.
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 * 1.00 10-Jul-1985
 *      DECnet-ULTRIX   V1.0
 * 4.1.1.3 28-Apr-1988
 *      DECnet-ULTRIX   V2.4 - do not free mbuf on getsockopt failure.
 *
 */

#include "../../h/param.h"
#include "../../h/systm.h"
#include "../../h/mbuf.h"
#include "../../h/socket.h"
#include "../../h/socketvar.h"
#include "../../h/protosw.h"
#include "../../h/errno.h"
#include "../../h/smp_lock.h"
#include "../../h/cpudata.h"

#include "../../h/user.h"
#include "../../h/buf.h"
#include "../../h/conf.h"
#include "../../h/proc.h"

#include "../../net/net/if.h"
#include "../../net/net/netisr.h"

#include "../../net/netinet/in.h"
#include "../../net/netinet/if_ether.h"

#include "../../net/netdnet/dli_var.h"

#include "../machine/cpu.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax



extern struct dli_line dli_ltable[];


/*
 * DLI protocol interface to socket abstraction.
 */

/*
 *		d l i _ u s r r e q
 *
 * Process a DLI request.
 *
 * Returns:		Nothing
 *
 * Inputs:
 *	so		= Pointer to the socket for this request.
 *	req		= Request function code.
 *	m		= Pointer to an MBUF chain.
 *	nam		= Pointer to an MBUF chain for addressing.
 *	rights		= Pointer to an MBUF chain for access rights.
 */
dli_usrreq( so,req,m,nam,rights )
register struct socket *so;
int req;
struct mbuf *m,*nam,*rights;
{
    int s = splnet();
    register struct sockaddr_dl *to_whom;
    register int i = 0;
    int error = 0, len;
    struct ifnet *ifp;
    int saveaffinity;  /* for nonsmp drivers.  8.18.88.us  */
    struct dli_line *ue, *dli_getlte_trust();


    /* lock line table entry and socket in correct order */
    if ( req != PRU_ATTACH )
	if ( (ue = dli_getlte_trust(so)) == NULL )
	{
	    if ( req == PRU_SEND )
		m_freem(m);
	    splx(s);
	    return(EBADF);
	}

    /*
     * Service request.
     */
    switch (req)
    {
	/*
	 * DLI attaches to a socket via PRU_ATTACH, reserving space.
	 */
		case PRU_ATTACH:
			if( ! suser() )
			{
				error = EPERM;
				break;
			}
			error = soreserve(so, DLI_SENDSPACE, DLI_RECVSPACE);
			if (!error)
			{
				error = dli_open(so);
			}
			if (!error)
			{
				soisconnected(so);
			}
			break;

	/*
	 * PRU_DISCONNECT set up as a NOP so that close() returns success.
	 */
		case PRU_DISCONNECT:
			break;

	/*
	 * PRU_DETACH detaches the DLI protocol from the socket.
	 */
		case PRU_DETACH:
			dli_close((struct dli_line *) so->so_pcb);
			so->so_pcb = NULL;
	   		sbflush(&so->so_rcv);
	    		sbflush(&so->so_snd);
	    		soisdisconnected(so);
			break;

#define offsetof(structure, member) ((int)&((structure *)0)->member)
	/*
	 * Bind name to socket
	 */
		case PRU_BIND:
			if (nam->m_len < offsetof(struct sockaddr_dl, choose_addr))
			{
				error = EINVAL;
				break;
			}
			to_whom = mtod(nam, struct sockaddr_dl *);
			switch (to_whom->dli_substructype)
			{
				case DLI_802:
					len = sizeof(struct sockaddr_802);
					break;
				case DLI_POINTOPOINT:
					len = sizeof(struct sockaddr_pdl);
					break;
				case DLI_ETHERNET:
					len = sizeof(struct sockaddr_edl);
					break;
				default:
					error = EINVAL;
					break;
			}
			if (error)
				break;
			if (offsetof(struct sockaddr_dl, choose_addr) + len > nam->m_len)
			{
				error = EINVAL;
				break;
			}
			error = dli_bind((struct dli_line *) so->so_pcb, to_whom);
			break;
	

	/*
	 * Data is available for transmission.
	 */
		case PRU_SEND:
			to_whom = NULL;
			if ( nam != NULL)
			{
				if (nam->m_len < offsetof(struct sockaddr_dl, choose_addr))
				{
					error = EINVAL;
					break;
				}
				to_whom = mtod(nam, struct sockaddr_dl *);
				switch (to_whom->dli_substructype)
				{
					case DLI_802:
						len = sizeof(struct sockaddr_802);
						break;
					case DLI_POINTOPOINT:
						len = sizeof(struct sockaddr_pdl);
						break;
					case DLI_ETHERNET:
						len = sizeof(struct sockaddr_edl);
						break;
					default:
						error = EINVAL;
						break;
				}
				if (error)
					break;
				if (offsetof(struct sockaddr_dl, choose_addr) + len > nam->m_len)
				{
					error = EINVAL;
					break;
				}
			}
			error = dli_output((struct dli_line *) so->so_pcb, m, to_whom);
			break;
	
	
	/*
	 * Return value of user's bound socket address.
	 */
		case PRU_SOCKADDR:
			if ( nam->m_len < sizeof(struct sockaddr_dl) )
			{
			    struct mbuf *page;
			    MCLGET(nam,page);
			    if ( page == NULL )
			    {
				error = ENOBUFS;
				break;
			    }
			}
			error = dli_fetchbind((struct dli_line *) so->so_pcb, mtod(nam, struct sockaddr_dl *), &nam->m_len);
			break;
	

	/*
	 * An ioctl() was issued
	 */
		case PRU_CONTROL:
			if ( (ifp = ((struct dli_line *) so->so_pcb)->dli_if) ){
				smp_unlock(&so->lk_socket);
				smp_unlock(&((struct dli_line *)so->so_pcb)->dli_lk);
				CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
				error = (*ifp->if_ioctl)(ifp, (int)m, (caddr_t)nam);
				RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
			}else
			{
				struct ifnet *ifunit();
				register struct ifreq *ifr = (struct ifreq *) nam;
				smp_unlock(&so->lk_socket);
				smp_unlock(&((struct dli_line *)so->so_pcb)->dli_lk);
				if (ifp = ifunit(ifr->ifr_name)){
					CALL_TO_NONSMP_DRIVER( (*ifp), saveaffinity);
					error = (*ifp->if_ioctl)(ifp, (int)m, (caddr_t)nam);
					RETURN_FROM_NONSMP_DRIVER( (*ifp), saveaffinity);
				}else error = ENXIO;
			}
			smp_lock(&((struct dli_line *)so->so_pcb)->dli_lk, LK_RETRY);
			if ( ((struct dli_line *)so->so_pcb)->dli_so != so )
				panic("pr_usrreq: socket went away!");
			smp_lock(&so->lk_socket, LK_RETRY);
			break;

		default:
		    error = EOPNOTSUPP;
		    break;
	
    }
    if ( req != PRU_ATTACH )
	smp_unlock(&ue->dli_lk);
    splx(s);
    return (error);
}

/*
 *              d l i _ c t l o u t p u t
 *
 *
 * All set and get socket options from the socket layer come
 * through here.  For now, this routine performs some functions
 * previously done by the socket layer and dli_usrreq().
 *
 * Note: The socket lock is not asserted when this routine is called.
 *
 * Returns:             status
 *
 * Inputs:
 *      op              = Option request code.
 *      so              = Pointer to the socket for this request.
 *      level           = Socket level.
 *      optname         = Option name passed by application.
 *      m               = Pointer to an MBUF containing or to contain
 *                              option data.
 */
dli_ctloutput(op, so, level, optname, mp)
	int op;
	struct socket *so;
	int level, optname;
	struct mbuf **mp;
{
    	int s = splnet();
	int error = 0;
	struct mbuf *m, *page;
	struct dli_line *dli_getlte_trust();

    	/* lock line table entry and socket in correct order */
	if ( dli_getlte_trust(so) == NULL )
	{
		splx(s);
	  	return(EBADF);
	}

	switch (op) {

		/*
		 * A getsockopt() was issued.
		 */
		case PRCO_GETOPT:
			if ( (m = m_get(M_DONTWAIT, MT_SOOPTS)) == NULL )
			{
				error = ENOBUFS;
				break;
			}
			*mp = m;
             		MCLGET(m,page);
             		if (page)
			{
				if ( so->so_proto->pr_protocol != level )
				{
					struct dli_line *ue = (struct dli_line *) so->so_pcb;
					if ( ue->dli_proto )
					    error = (*ue->dli_proto->pr_ctloutput)(op, so, level, optname, mp);
					else
					    error = EOPNOTSUPP;
				}
				else
					error = dli_getopt((struct dli_line *) so->so_pcb, mtod(m, u_char *), &m->m_len, (int)optname);
			}
			else
				error = ENOBUFS;
			break;
	

		/*
		 * A setsockopt() was issued.
		 */
		case PRCO_SETOPT:
			m = *mp;
			if (m)
			{
				if ( so->so_proto->pr_protocol != level )
				{
					struct dli_line *ue = (struct dli_line *) so->so_pcb;
					if ( ue->dli_proto )
					    error = (*ue->dli_proto->pr_ctloutput)(op, so, level, optname, mp);
					else
					    error = EOPNOTSUPP;
				}
				else
					error = dli_setopt((struct dli_line *) so->so_pcb, mtod(m, u_char *), m->m_len, (int)optname);
			}
			else
			{
				error = EFAULT;
			}
			if (m) m_free(m);
			break;
		default:
		        error = EOPNOTSUPP;
			break;
	}
	smp_unlock(&((struct dli_line *)so->so_pcb)->dli_lk);
    	splx(s);
	return (error);
}



/*
 * loopback module routines to be moved to dli_usrreq.c.
 */

#define DLOOP_MAXUSER	100
struct protosw *dloop_user[DLOOP_MAXUSER];


/*
 *		d l o o p _ i n i t
 *
 * This routine handles initialization for the loop module.
 *
 * Outputs:		None.
 *
 * Inputs:		None.
 *
 * Version History:
 */
dloop_init()
{
    bzero(dloop_user, sizeof(dloop_user));
}




/*
 *              d l o o p _ c t l o u t p u t
 *
 *
 * All set and get socket options from the socket layer come
 * through here.  For now, this routine performs some functions
 * previously done by the socket layer and dli_usrreq().
 *
 * Returns:             status
 *
 * Inputs:
 *      op              = Option request code.
 *      so              = Pointer to the socket for this request.
 *      level           = Socket level.
 *      optname         = Option name passed by application.
 *      m               = Pointer to an MBUF containing or to contain
 *                              option data.
 */
dloop_ctloutput(op, so, level, optname, parm)
	int op;
	struct socket *so;
	int level, optname;
	caddr_t parm;
{
    	int s = splnet();
	int i, error = 0;
	struct dlp_port *req;

	switch (op) {

		case PRCO_PIF:
                    switch ( optname ) {

                        case DLOOP_OPENPORT:
			    for ( i = 0; i < DLOOP_MAXUSER; i++ )
				if ( dloop_user[i] == NULL )
				    break;
			    if ( i == DLOOP_MAXUSER )
			    {
				error = ENOBUFS;
			    }
			    else
			    {
				req = (struct dlp_port *) parm;
				dloop_user[i] = req->dlp_pr;
				req->dlp_id = i;
			    }
                            break;

                        case DLOOP_CLOSEPORT:
			    req = (struct dlp_port *) parm;
			    i = req->dlp_id;
			    if ( dloop_user[i] == req->dlp_pr )
				dloop_user[i] = NULL;
			    else
				error = EFAULT;
                            break;

		    }
		    break;

		default:
		        error = EOPNOTSUPP;
			break;
	}
    	splx(s);
	return (error);
}


/*
 *		d l o o p _ i n p u t
 *
 * This routine is the looback input interface used by other kernel
 * modules.  
 *
 * Outputs:		None.
 *
 * Inputs:		pointer to ifnet structure
 *			pointer to data link header
 *			input buffer.
 *
 * Version History:
 */
struct mbuf *dloop_input(ifp, hdr, m)
register struct ifnet *ifp;
struct ether_header *hdr;
register struct mbuf *m;
{
    return(m);
}




struct ifqueue dlointrq =                    /* loop input queue */
  { 0, 0, 0, IFQ_MAXLEN, 0 };


/*
 *		d l o o p _ o u t p u t
 *
 * This routine is the looback output interface used by other kernel
 * modules. 
 *
 * Outputs:		None.
 *
 * Inputs:		port id.
 *			output message in mbuf chain format.
 *
 * Version History:
 */
dloop_output(pid, m)
u_int pid;
register struct mbuf *m;
{
    int s;
    u_int error = 0;


    if ( dloop_user[pid] == NULL )
	return(EFAULT);

    m->m_un1.mun1_context = (int) pid;

    s = splimp();
    smp_lock(&dlointrq.lk_ifqueue, LK_RETRY);
    if (IF_QFULL(&dlointrq))
    {
                IF_DROP(&dlointrq);
                m_freem(m);
		smp_unlock(&dlointrq.lk_ifqueue);
                error = ENOBUFS;
    }
    else
    {
                IF_ENQUEUE(&dlointrq, m);
                schednetisr(NETISR_DLO);
		smp_unlock(&dlointrq.lk_ifqueue);
                error = 0;
    }
    splx(s);
    return(error);

}


 /*
 *		d l o i n t r
 *
 * DLI domain looback routine. This routine is called from the network software
 * ISR routine to process incoming packets. The first MBUF in any chain
 * contains a looback receive descriptor with destination information.
 *
 * Outputs:		None.
 *
 * Inputs:		None.
 */
dlointr()
{
    register struct mbuf *m;
    int s;
    u_int pid;


next:
    /*
     * Try to pull an input message (MBUF chain) from the OSI input queue.
     */
    s = splimp();
    smp_lock(&dlointrq.lk_ifqueue, LK_RETRY);
    IF_DEQUEUE(&dlointrq, m);
    smp_unlock(&dlointrq.lk_ifqueue);
    splx(s);
    if (m)
    {
	pid = (u_int) m->m_un1.mun1_context;
	m->m_un1.mun1_context = 0;
	if ( dloop_user[pid] != NULL )
	{
	    (*dloop_user[pid]->pr_input)(pid, m, DLI_LOOP, NULL);
	}
	else
	    m_freem(m);
        goto next;
    }
}
