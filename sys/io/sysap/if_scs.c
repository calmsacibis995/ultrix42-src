#ifndef lint
static char *sccsid = "@(#)if_scs.c	4.2	(ULTRIX)	11/14/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 - 1989 by			*
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
/* Modification History:
 *
 *	23-Aug-1990	larry
 *		Can only pre map transmit ptes on a VAX.  For mips we have 
 *		to map transmit buffers just before the send and unmap
 *		when transfer is acked. 
 *
 *	20-Jun-1989	larry
 *		In scsnet_msgevent: init mbuf pointer to 0 and do not
 *			free mbuf when cant allocate cluster (just goto bad).
 *			The latter change should clear up an mbuf panic.
 *
 *	15-Jun-1989	jaw
 *		move IPL raising to earlier in scs_init code.  This is
 *		to fix a panic while doing an ifconf....
 *
 *	14-May-1989	Todd M. Katz		TMK0002
 *		This is the original modification history for the V3.1 pool
 *		merged by means of the previous two changes:
 *		1. The macro Scaaddr_lol() has been renamed to Scaaddr_low().
 *		   It now accesses only the low order word( instead of low
 *		   order longword ) of a SCA system address.  The macro
 *		   Scaaddr_hos() has been renamed to Scaaddr_hi().  Make use of
 *		   the new macro Scaaadr_mid().
 *		2. Modify TMK0001 to use the shorthand notation Lproc_name when
 *		   refering to the corresponding MSB field.
 *
 *	8-May-1989 - Larry Cohen
 *		Merge in delta's 9.2 and 9.3 from the 3.1 pool
 *
 *	4-Apr-1989 - Larry Cohen
 *		Add smp support.  Output serialized on each system.  Can
 *		send to  multiple systems at once.
 *
 *	15-Mar-1989	Tim Burke
 *		Changed queue manipulations to use the following macros:
 *		remque ..... Remove_entry
 *		insque ..... Insert_entry
 *
 *	28-Dec-1988 - Larry Cohen
 *		- remove "cant copy mbuf" printf.  Out of memory when
 *			when this happens. Consistent with other drivers.
 *
 *	17-Oct-1988 - Larry Cohen
 *		- validate host number during init.
 *		- remove scs_disconnect's because they will fail anyway.
 *			They need to be scheduled with ksched.
 *		- change reject failures from panics to printfs.
 *
 *	02-Sep-1988 - Todd M. Katz		TMK0001
 *		Pass local SYSAP name to SCS when crashing path.
 *
 *	31-Aug-1988 - Larry Cohen
 *		- bug fixes.  handle offset packets correctly.
 *
 *	29-July-1988 - Larry Cohen
 *		- added version number to link level protocol
 *		- made data structures configurable based on SCSNET_MAXHOSTS
 *
 *	12-July-1988 - Larry Cohen
 *		Use one typeII mbuf for receiving block transfers.  
 *		Return if not IFF_UP.
 *		TBIS when finish xmit instead of at start.
 *
 *	27-Jun-1988 - Larry Cohen
 *		when transmit times out crash the path
 *
 *      02-Jun-1988     Ricky S. Palmer
 *              Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *	15-Mar-88 - Larry Cohen
 *		remove m_clalloc and kmalloc pages only.
 *	
 *	15-Jan-88 - lp
 *	Changed usage of Mbutl as it is gone
 */

#include "../data/if_scs_data.c"

/****************************************************************************
 *			       Overview
 *
 * This driver provides an Internet only interface to the System
 * Communications Services (SCS) subsystem.
 * The driver is initialized by the ifconfig(8) command.  Broadcasting
 * is supported.
 *
 * RESTRICTIONS that apply are as follows:
 * 1.  The SCS message size must be at least 256 bytes.
 * 2.  Any host that participates in an SCS based subnet must have a host number
 *	between 1 and 16 inclusive.
 *
 * When the driver is initialized an attempt is made to connect to all known
 * SCS nodes.  Each connection attempt contains the local internet address.
 * If the remote node is an ULTRIX-32 node and a corresponding
 * driver is configured into the remote node, an accept will be issued that 
 * includes the remote nodes internet address.  Connection collisions
 * are handled simply by rejecting a request if the remote SCS system ID is 
 * greater than the SCS system ID of the local node.
 * After each connection is established the driver adds the connection
 * identification information to a known system list that is indexed off
 * of the internet host number.  The driver can now easily translate the
 * internet destination of a packet into a SCS connection identifier.
 * 
 * Transmission of packets is handled in the following manner:
 *	The protocol layer calls the output routine with a packet to 
 * transmit.   The output routine will size the packet and if it is less
 * than or equal to the size of an SCS datagram the packet is copied into
 * the datagram and shipped.  If the packet is larger than a datagram a 
 * SCS block transfer is used to transfer the data to the remote host.
 * 	The link level protocol for datagrams is quite simple.  A long word 
 * at the beginning of the datagram is reserved for the protocol family.  
 * (Only the Internet family is supported for now)  The data is then copied into
 * the remaining portion of the datagram.  If the driver is unable to
 * transmit the datagram for any reason, the  data is released.  Datagrams
 * are not retransmitted or queued. It is the responsibility of higher level
 * protocols to retransmit.  After the SCS layer transmits the datagram the
 * datagram buffer is deallocated.
 * Upon receiving the data the remote driver copies the data into an mbuf chain
 * and passes the chain up to the higher level protocols.  The datagram buffer
 * is requeued for reception.
 * 	The block transfer protocol is a little more complicated.  The local
 * driver must first determine if there enough resources to initiate the
 * transfer.   Initially there are enough resources to handle five concurrent
 * block transfers to each host.  If there are not enough resources the block
 * transfer is queued in the driver.  The resource in this case is a set
 * of pte's which are used to map the output packet into a virtually
 * contiguous buffer. The SCS subsystem requires that the data to be transferred
 * be virtually contiguous.  Unfortunately the data arrives as a noncontiguous
 * chain of mbufs.  Rather than allocate or reserve large chunks of memory and
 * copy the data into it, the driver tries its best to copy ptes instead.  It
 * can only do this if the data to be copied is in cluster sized and cluster
 * aligned chunks.   Once the data fails to meet the latter requirement, storage
 * is allocated and mapped.  The data is then copied into the new buffer.  
 * After the data is mapped a sequenced message is sent to the remote driver
 * requesting the initiation of a block transfer.
 * The format of a sequenced message is as follows:
 * 
 *	struct _bhandle bhandle;	
 *	u_long  rspid;		
 *	long totlen;
 *	u_short cmd;
 *	u_short off;
 *   	char proto_hdr[];	
 *
 *   bhandle is the local buffer handle.  The remote driver uses this handle to
 *	point SCS to the local buffer.  
 *
 *   rspid doubles as a transaction response identifier and as an index into
 *	a transaction control block that contains information about the
 *	transaction.
 *
 *   totlen is the length of the block transfer.     
 *
 *   cmd represents the purpose of the message.   The message could be a request
 *	to initiate a block transfer or an acknowledgement from the remote
 *	driver.
 *
 *   off is the offset into the local buffer where the transaction begins.
 *
 *   proto_hdr marks the the point in the sequenced message where the protocol
 *	type is stored and possibly one mbufs worth of data (which
 *	is usually a higher level protocol header)
 *
 *
 * Upon receiving the block transfer request the remote driver will gather up
 * enough storage to hold the incoming transfer.   Just as in the output case
 * the receive buffer must be virtually contiguous.  To accomodate SCS one 
 * type II mbuf is allocated.  If there is enough
 * memory for a receive buffer a SCS block request is started.
 * Otherwise a negative acknowledgement is returned.  If the local driver
 * receives a negative acknowledgement the output packet is discarded.  Since
 * the remote driver is initially prepared to handle all incoming requests
 * from each host and a host will not send more than it's quota, the only 
 * valid reason for a failure is the inability of the remote host to obtain
 * memory.  If the block transfer completes successfully the remote
 * driver sends back a positive acknowledgement.  The local driver will then
 * free up the output packet and check to see if there are any pending 
 * transactions.
 *
 *****************************************************************************/

/*
TODO:
have to fork off scs event disconnects 
Most should never happen but ...

Need to make scsmempt in spt.s configurable.  Currently we have enough
ptes for 32 hosts, 5 xfers per host, 17 ptes per xfer.

Currently need to reboot if set address of the interface incorrectly with
ifconfig.   Should do something like crash all paths if we set interface
to a new address and then reestablish connections with new address.

stats:
	keep track of failures to send data: datagram and block transfers.

protocol independent.

SMP flag to copy instead of pte flip.

SMP safe.

Place xfer control blocks in netsys structure.

Dynamically allocate data structures.

Get rid of overly complicated output code.

In future versions we may want to use the VERSION field in the conn data
to reject a connection.  Algorithm:  lower versions are passive and issue
an accept if they receive a higher number version.  Higher versions issue
the reject if they receive a lower number version during a remote connect
request or when their active connect request completes.
*/


int scsnet_timer = 0; 		/* watch dog timer on/off indicator */
int scsnet_timeo;


/*
 *	Commonly used macros
 */

#define Scsnet_msg_hdr_siz  ((char *)&scsnet_msg_x.proto_hdr[0] - \
				(char *)&scsnet_msg_x)

#define Scsnet_Return_ENOBUFS \
	(void)splx( ipl ); \
	m_freem(m0);	   \
	ifp->if_oerrors++;   \
	return( ENOBUFS ); 


#define Scsnet_If_Enqueue_Return \
        if (IF_QFULL(&ifp->if_snd)) {   \
                IF_DROP(&ifp->if_snd);  \
		smp_unlock(&scs_dst->lk_netsys); \
		Scsnet_Return_ENOBUFS   \
        }				\
	m = m_get(M_DONTWAIT, MT_DATA); \
	if (m == 0) {			\
		smp_unlock(&scs_dst->lk_netsys); \
		Scsnet_Return_ENOBUFS   \
	}				\
	{				\
	struct sockaddr *sdst; \
	sdst = mtod(m, struct sockaddr *); \
	*sdst = *dst;		\
	m->m_next = m0;			\
	}				\
        IF_ENQUEUE(&ifp->if_snd, m);	\
	smp_unlock(&scs_dst->lk_netsys); \
	(void)splx( ipl );		\
	return(0);			

#define Sysid_greater(h,t) 					\
	( Scaaddr_hi( h ) > Scaaddr_hi( t )	 	||	\
	  ( Scaaddr_hi( h ) == Scaaddr_hi( t ) &&		\
	    Scaaddr_mid( h ) > Scaaddr_mid( t ))	||	\
	  ( Scaaddr_hi( h ) == Scaaddr_hi( t )   &&		\
	    Scaaddr_mid( h ) == Scaaddr_mid( t ) &&		\
	    Scaaddr_low( h ) > Scaaddr_low( t )))

#define Print_sysid(s) \
        printf("sysid=%d,%d,%d\n",                      \
                Scaaddr_hi( s ), Scaaddr_mid( s ), Scaaddr_low( s ));

#define Scsnet_enqueue(m) {						\
	struct ifqueue *inq;						\
	u_long *proto_type;						\
	int ipl;							\
									\
	proto_type = mtod (m , u_long *);  /* protocol type */ 		\
	m->m_off += sizeof(long); /* point to beginning of data */ 	\
	m->m_len -= sizeof(long); 					\
									\
	if (*proto_type == AF_INET) {					\
	/* enqueue mbuf chain on protocol queue */			\
		if (nINET==0) {						\
			m->m_off -= sizeof(long);			\
			m_freem(m);					\
		} else {						\
			inq = &ipintrq;					\
			ipl = splimp();					\
			smp_lock(&inq->lk_ifqueue, LK_RETRY);		\
			schednetisr(NETISR_IP);				\
			if (IF_QFULL(inq)) {				\
				IF_DROP(inq);				\
				m->m_off -= sizeof(long);		\
				m_freem(m);				\
			} else						\
				IF_ENQUEUE(inq, m);			\
			smp_unlock(&inq->lk_ifqueue);			\
			(void)splx(ipl);				\
		}							\
	}								\
	else								\
		panic("scsnet_event - unknown proto");			\
    }								


/* Scsnet_resolve - resolve an Internet address into an SCS address */

#define Scsnet_resolve(inet_dst,scs_dst) {   				\
	register int rmthost = in_lnaof(inet_dst) & 0xff;		\
	register struct _netsystem *netsys;				\
									\
	/* check validity of inet address */				\
	if (rmthost>=SCSNET_MAXHOSTS || rmthost<0) {			\
		printf("scsnet_resolve: invalid host %d\n", rmthost);	\
		m_freem(m0);						\
		return(0);						\
	}								\
									\
	/*								\
	 * are we connected to this system yet?  If not then		\
	 * we return failure and hope that we are informed		\
	 * about this system later and connect to it.			\
	 */								\
	if ((netsys = knownsystems[rmthost]) == SCSNET_NO_SYS) {	\
		if (scsnetdebug)					\
		    printf("scsnet: conn not resolved to %d\n", rmthost);\
		m_freem(m0);						\
		return(0);						\
	}								\
									\
	if (netsys->status == SCSNET_CONN_OPEN)				\
		scs_dst = netsys;					\
	else {								\
		if (scsnetdebug)					\
		     printf("scsnet_resolve: conn is not open, status=%d\n", \
			netsys->status);				\
		m_freem(m0);						\
		return(0);						\
	}								\
    }									\


int scsnet_ioctl(), scsnet_output();
void scsnet_control(), scsnet_attach();
void scsnet_dgevent();
void scsnet_msgevent();
struct _netsystem * alloc_netsys();
void scsnet_watch();

int scsnetdebug = 0;

/* track packet sizes */
int scsnetstaton = 0;
int scsnetstat[10];
scaaddr scsnet_nosysid = { -1, -1 };


/* scsnet_attach() - make interface known to system
 * 
 * Inputs:
 * 	scsnetif - system wide network interface information
 * Output:
 * 	scsnetif 
 *
 */
void
scsnet_attach()
{
	register struct ifnet *ifp = &scsnetif;
	register int i;
	extern u_short scs_msg_size;

	if (scs_msg_size < scsnet_min_msg_siz) {
		printf("scsnet: attach failed, scs_msg_size is less than %d\n",
			scsnet_min_msg_siz);
		return;
	}
	ifp->if_name = "scs";
	/* 
	 * ASSUMPTION:  upper protocols always store protocol header at
	 * the head of the data chain in a small mbuf.  This being true
	 * we can transfer the protocol header in a msg buffer while the
	 * rest of the data is transmitted via a block transfer.
	 */
	ifp->if_mtu = scsnet_block_size + scs_msg_size - SCSNET_HDR_SIZ;
	ifp->if_ioctl = scsnet_ioctl;
	ifp->if_output = scsnet_output;
	ifp->if_flags |= IFF_BROADCAST;
	ifp->d_affinity = ALLCPU;
	lockinit(&lk_scsnetknownsys,  &lock_scsnetknownsys_d);
	lockinit(&lk_netsystems, &lock_scsnetknownsys_d);
	lockinit(&lk_scsnetrecvs, &lock_scsnetrecvs_d);
	lockinit(&scsnetif.if_snd.lk_ifqueue, &lock_device15_d);
	for (i=0; i<SCSNET_MAXHOSTS; i++)
		lockinit(&netsystems[i].lk_netsys, &lock_scsnetsys_d);
	if_attach(ifp);

}


/*******************************************************************
 *   scsnet_init() - Initialize the scs network sysap.
 *	1.  place scs network sysap in listening state so that remote
 *		network sysaps can establish a connection to us.
 *	2.  determine known systems and establish connections to 
 *		those systems that are running an scs network sysap.
 *      3.  mark up and running.
 *
 *  Inputs:
 *	knownsystems
 *	scsnet_local - local system SCS info.
 *	scsnetif
 *  Outputs:
 *	knownsystems
 */
scsnet_init() 
{
	struct ifnet *ifp = &scsnetif;
	register ISB *isb = &netisb;
	register struct _netsystem *netsystem;
	register struct ifaddr *ifa;
	register SIB *s;
	register int i;
	int ret;
	scaaddr sid;
	int ipl;
	extern int ifqmaxlen;


#ifdef INET
	/* this will have to change when we make things more 
	   general */

	smp_lock(&lk_ifnet, LK_RETRY);
	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		if (ifa->ifa_addr.sa_family == AF_INET)
			break;
	scs_inet_addr =  IA_SIN(ifa)->sin_addr;
	smp_unlock(&lk_ifnet);
	scsnet_lhost = in_lnaof(scs_inet_addr) & 0xff;
	if (scsnet_lhost < 0 || scsnet_lhost > SCSNET_MAXHOSTS - 1 ) {
		printf("scsnet: can not initialize because host number is invalid, host=%d\n", scsnet_lhost);
		return(1);
	}
#endif
	/* not yet, if address still unknown */
	if (ifp->if_addrlist == (struct ifaddr *)0)
			return (1);
	if (ifp->if_flags & IFF_RUNNING)
		return (0);

	scsnet_topsysaddr = netsystems;
	ifp->if_snd.ifq_maxlen = ifqmaxlen;

	/* scs_info_scs get lock at scs level so raise priority here */
        ipl = Splscs();
	/* obtain local system information */
	scs_info_scs(&scsnet_local);
	Move_scaaddr( scsnet_local.system.sysid, sid )

	if (scsnetdebug)
                printf("scs init - local sysid: %x,%x,%x, local inet host=%d\n",
                        Scaaddr_hi( sid ), Scaaddr_mid( sid ),
                        Scaaddr_low( sid ), scsnet_lhost );
		
	for (i=0; i<SCSNET_RECVS; i++)
		scsnet_recvs[i].state = SCSNET_BDONE;

        for (i=0; i<SCSNET_MAXHOSTS; i++)
                knownsystems[i] = SCSNET_NO_SYS;
	
	Init_queue(netsys_head)

	/* start listening */
	netcmsb.control = scsnet_control;
	bcopy (SYSAP_NET, netcmsb.lproc_name, NAME_SIZE);
        ipl = Splscs();
	if ((ret=scs_listen(&netcmsb))!= RET_SUCCESS) {
		(void)splx( ipl );
		if (scsnetdebug)
			printf("scs_listen failed, ret=%x\n", ret);
		return(1);
	}
	(void)splx( ipl );

	/*
	 *  Connect to all of the systems we know about
 	 */
	Zero_scaaddr(isb->next_sysid);  /* start at the beginning */
	do	{
			
	
		if ((netsystem=alloc_netsys())==SCSNET_NO_SYS)
			panic("scsnet: too many systems"); 
		smp_lock(&netsystem->lk_netsys, LK_RETRY);
		netsystem->rmt_index = SCSNET_NO_INDEX;
		if (scsnet_connect(netsystem, isb) != RET_SUCCESS)
			scsnet_rem_sys(netsystem, &netcmsb);
		smp_unlock(&netsystem->lk_netsys);
	} while (!Test_scaaddr( isb->next_sysid));
	(void)splx( ipl );
	
	scsnet_timeo = SCSNET_TIMEO;
	ifp->if_flags |= IFF_RUNNING | IFF_UP;
	return (0);

}

/*	
 *  scsnet_control:  event notification routine.
 *		
 *
 *		
 *    event	    	Action
 *
 *    CRE_CONN_REC    	If enough resources are available send an accept.
 *			Otherwise send a reject.
 *
 *    CRE_ACCEPT_DONE 	Add remote system to known systems list and reserve
 *			resources for I/O.
 *
 *    CRE_REJECT_DONE	none
 *    CRE_DISCONN_REC  	Issue disconnect to remote sysap and deallocate
 *			resources dedicated to connection.
 *    CRE_PATH_FAILURE  Issue disconnect to remote sysap and deallocate
 *			resources dedicated to connection. Try to reconnect
 *			in the event another path exists.
 *    CRE_DISCONN_DONE  none
 *    CRE_BLOCK_DONE  	Deallocate buffer space and send ack.
 *    CRE_CREDIT_AVAIL 	none
 *    CRE_NEW_PATH 	Issue connect to new system.
 *    CRE_CONN_DONE 	If successful then add to list of known systems and
 *			reserve resources for I/O.
 *			If remote sysap was busy then issue another connect.
 *			If connect failed deallocate any resources devoted
 *			to the connection request.
 *
 * Inputs:
 *
 *   cmsb	- Connection Management Services Block
 *   event	- Event type
 *
 * Outputs:
 *
 *   cmsb	- Connection Management Service Block pointer
 *		  INITIALIZED ( depending upon actions taken )
 *
 *   Return
 *   Values:	NONE
 */
	

void
scsnet_control( event, cmsb )
    u_short			event;
    register CMSB		*cmsb;
{
    register struct _netsystem *sys = (struct _netsystem *)cmsb->aux;
    register SIB *s = &sys->sys_info;
    struct scsnet_conn_data *scd = (struct scsnet_conn_data *)cmsb->conn_data;
    int status, ret;

#ifdef SCSNETDEBUG
    if (scsnetdebug > 1) {
	printf("scsnet_control: sys=%x, event=%d, cmsb->status=%d, cmsb->sysid=%x,%x,%x\n", 
                sys, event, cmsb->status, Scaaddr_hi( cmsb->sysid )
                Scaaddr_mid( cmsb->sysid ), Scaaddr_low( cmsb->sysid ));
	printf(" lproc_name=%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n", 
		cmsb->lproc_name[0], cmsb->lproc_name[1], cmsb->lproc_name[2],
		cmsb->lproc_name[3], cmsb->lproc_name[4], cmsb->lproc_name[5],
		cmsb->lproc_name[6], cmsb->lproc_name[7], cmsb->lproc_name[8],
		cmsb->lproc_name[9], cmsb->lproc_name[10], cmsb->lproc_name[11],
		cmsb->lproc_name[12], cmsb->lproc_name[13],cmsb->lproc_name[14],
		cmsb->lproc_name[15]);
	printf(" rproc_name=%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n", 
		cmsb->rproc_name[0], cmsb->rproc_name[1], cmsb->rproc_name[2],
		cmsb->rproc_name[3], cmsb->rproc_name[4], cmsb->rproc_name[5],
		cmsb->rproc_name[6], cmsb->rproc_name[7], cmsb->rproc_name[8],
		cmsb->rproc_name[9], cmsb->rproc_name[10], cmsb->rproc_name[11],
		cmsb->rproc_name[12], cmsb->rproc_name[13],cmsb->rproc_name[14],
		cmsb->rproc_name[15]);
	
}

#endif
		
	
    if (event == CRE_BLOCK_DONE) {

    /* block transfer completed. 
     * Send and ack to the other side and free up local resource.
     */
	register struct scsnet_cntl *scscp = &scsnet_recvs[cmsb->blockid];
	register struct scsnet_msg *msg = (struct scsnet_msg *)scscp->csb.buf;
	register struct mbuf *m = scscp->mchain;
	register struct ifnet *ifp = &scsnetif;


#ifdef SCSNETDEBUG
	if (scsnetdebug > 2)
		printf("scsnet_control: rspid=%d\n", msg->rspid);
#endif
	/*
	 * queue data to the appropriate protocol.
	 */
	

	Scsnet_enqueue(m)

	ifp->if_ipackets++;
	
	/*
	 * send and acknowledment back to the remote host
	 */
	msg->cmd = SCSNET_BCMD_ACK;
	scscp->csb.connid = cmsb->connid;
	scscp->csb.size = Scsnet_msg_hdr_siz;
	scscp->csb.Disposal = RECEIVE_BUF;
	scscp->csb.Aux = 0;

	if ((status=scs_send_msg(&scscp->csb)) != RET_SUCCESS)
		printf("scsnet: send ack0 failed, status = %d\n", status);

	/*
	 * free up local resources 
	 */

	if ((status=scs_unmap_buf(&scscp->csb)) != RET_SUCCESS)
		printf("scsnet: unmap0 failed, status = %d\n", status);

	scscp->mchain = 0;
	scscp->state = SCSNET_BDONE;

    } else 

        switch( event ) {
        case CRE_CONN_DONE:
        {
    	/* An attempt to establish a SCS connection has completed.  Process it
     	 * according to its completion status.
     	 * Add this system to a list of known systems. 
     	 */
	     smp_lock(&sys->lk_netsys, LK_RETRY);
	     if( cmsb->status == ADR_SUCCESS ) {
	        /* mark connection open and add to list of known systems */
		if (scsnetdebug)
	            printf( "scsnet - Established SCS Connection to %c%c%c%c%c%c%c%c\n",
		        s->node_name[ 0 ], s->node_name[ 1 ],
		        s->node_name[ 2 ], s->node_name[ 3 ],
		        s->node_name[ 4 ], s->node_name[ 5 ],
		        s->node_name[ 6 ], s->node_name[ 7 ] );
	        if (scsnet_add_sys(in_lnaof(scd->rmt_inaddr)&0xff, 
				sys, cmsb)!=RET_SUCCESS) {
		    printf("scsnet: conn recv, could not allocate resources for connection to host %d\n", in_lnaof(scd->rmt_inaddr)&0xff);

		    /* 
		     * send a disconnect 
		     */

#ifdef notdef
		/* should not get here but if we do then we need to schedule the
		 * scs_disconnect with ksched or timeout.  TODO
		 */
	            if( scs_disconnect( cmsb ) != RET_SUCCESS )
	            	 panic( "SCS_NET - SCS Disconnect Failed\n" );
	            else
		        if (scsnetdebug)
	                    printf( "scsnet - SCS DISCONNECT to %c%c%c%c%c%c%c%c sent\n",
		        	s->node_name[ 0 ], s->node_name[ 1 ],
		        	s->node_name[ 2 ], s->node_name[ 3 ],
		        	s->node_name[ 4 ], s->node_name[ 5 ],
		        	s->node_name[ 6 ], s->node_name[ 7 ] );
#endif
		    scsnet_rem_sys(sys, cmsb);
		}

	    } else 
		/* 
		 * connection attempt failed
		 */
	    {
		ISB isb;

		bzero((caddr_t)&isb, sizeof(ISB));
		/*
	         * if remote sysap is busy keep trying 
		 */
	        if (cmsb->status == ADR_BUSY  && 
			sys->retries++ < scsnet_maxretries) {
	    	    Move_scaaddr( cmsb->sysid, isb.next_sysid );
		    sys->rmt_index = SCSNET_NO_INDEX;
		    if (scsnet_connect(sys, &isb) != RET_SUCCESS)
			scsnet_rem_sys(sys, cmsb);
		    smp_unlock(&sys->lk_netsys);
		    return;
	        }

		/*
	         * mark connection closed 
		 */
		if (scsnetdebug) {
	            printf( "scsnet - Unable to Establish SCS Connection to " );
	    	    printf( "%c%c%c%c%c%c%c%c\n",
		        s->node_name[ 0 ], s->node_name[ 1 ],
		        s->node_name[ 2 ], s->node_name[ 3 ],
		        s->node_name[ 4 ], s->node_name[ 5 ],
		        s->node_name[ 6 ], s->node_name[ 7 ] );
		}
	    	scsnet_rem_sys(sys, cmsb);

	    }

	    smp_unlock(&sys->lk_netsys);
	    break;
        }

        case CRE_CREDIT_AVAIL:
	    break;

        case CRE_DISCONN_REC:

        /* A SCS connection has terminated due to virtual path failure.  Clean
         * up and then disconnect the connection.
     	 */
        case CRE_PATH_FAILURE: {

	    ISB	isb;
	    struct _netsystem	*netsystem;

	    bzero((caddr_t)&isb, sizeof(ISB));
	    smp_lock(&sys->lk_netsys, LK_RETRY);
	    if (scsnetdebug) {
	        if (event == CRE_PATH_FAILURE)
		    printf( "Scsnet - Virtual Path Failed to: ");
	        else
		    printf( "Scsnet - Received disconnect from: ");

	        printf( "%c%c%c%c%c%c%c%c \n",
		    s->node_name[ 0 ], s->node_name[ 1 ],
		    s->node_name[ 2 ], s->node_name[ 3 ],
		    s->node_name[ 4 ], s->node_name[ 5 ],
		    s->node_name[ 6 ], s->node_name[ 7 ] );
	    }

	    /*
	     * Issue the SCS Disconnect to complete connection termination.
	     */
	    scsnet_rem_sys(sys, cmsb);
	    /*
	     * this disconnect is ok because I am not changing the state
	     * of an scs connection
	     */
	    if( scs_disconnect( cmsb ) != RET_SUCCESS )
	        panic( "SCS_NET - SCS Disconnect Failed\n" );
	    else
		if (scsnetdebug)
	            printf( "scsnet - SCS DISCONNECT sent to %c%c%c%c%c%c%c%c \n",
		        s->node_name[ 0 ], s->node_name[ 1 ],
		        s->node_name[ 2 ], s->node_name[ 3 ],
		        s->node_name[ 4 ], s->node_name[ 5 ],
		        s->node_name[ 6 ], s->node_name[ 7 ] );

	    /* 
	     * try to reconnect just in case there exists an alternate path
	     */

	    smp_unlock(&sys->lk_netsys);
	    if ((netsystem=alloc_netsys())==SCSNET_NO_SYS)
	       return;


	    /* Check to see if the system is still in the system-wide
	     * configuration database.  
	     */
	    Move_scaaddr( cmsb->sysid, isb.next_sysid )

	    smp_lock(&netsystem->lk_netsys, LK_RETRY);
	    if (scsnet_connect(netsystem, &isb) != RET_SUCCESS)
			scsnet_rem_sys(netsystem, cmsb);
	    smp_unlock(&netsystem->lk_netsys);

	    break;
	}

        /*
         * A previously unknown new system exists.
         * Try to establish a connection to the net sysap on the new system.
         */
        case CRE_NEW_PATH: {
	    ISB	isb;
	    register SIB		*sib;
	    struct _netsystem	*netsystem;
	
	    bzero((caddr_t)&isb, sizeof(ISB));
#ifdef notdef
		if (Comp_scaaddr(cmsb->sysid, scsnet_local.system.sysid) == 0 )
			return;  /* only connect to local system for now */
#endif

	    if (scsnetdebug)
	    	printf("scsnet control: new path\n");
	    /*
	     * find a free netsystem structure for this connection
 	     * and then try to connect.
	     */
	    if ((netsystem=alloc_netsys())==SCSNET_NO_SYS) {
	        printf( "SCS_NET - can not allocate resources for new system\n");
		return;
	    }

	    smp_lock(&netsystem->lk_netsys, LK_RETRY);

	    Move_scaaddr( cmsb->sysid, isb.next_sysid )

	    if (scsnet_connect(netsystem, &isb) != RET_SUCCESS) {
		scsnet_rem_sys(netsystem, cmsb);
		smp_unlock(&netsystem->lk_netsys);
		return;
	    }

	    sib = &netsystem->sys_info;

	    /*
	     * This entire process is interrupt driven once establishment of the
	     * SCS connection is initiated.
	     */

	    if (scsnetdebug)
	       printf( "SCS_NET - SCS Discovered %c%c%c%c%c%c%c%c\n",
	        sib->node_name[ 0 ], sib->node_name[ 1 ],
	        sib->node_name[ 2 ], sib->node_name[ 3 ],
	        sib->node_name[ 4 ], sib->node_name[ 5 ],
	        sib->node_name[ 6 ], sib->node_name[ 7 ] );

	    smp_unlock(&netsystem->lk_netsys);
	    break;
	}

        /* Termination of a SCS conn has completed. 
         */
        case CRE_DISCONN_DONE:
	    smp_lock(&sys->lk_netsys, LK_RETRY);
	    if (scsnetdebug)
	        printf( "scsnet - Completed Disconnect from %c%c%c%c%c%c%c%c\n",
		    s->node_name[ 0 ], s->node_name[ 1 ],
		    s->node_name[ 2 ], s->node_name[ 3 ],
		    s->node_name[ 4 ], s->node_name[ 5 ],
		    s->node_name[ 6 ], s->node_name[ 7 ] );

	    smp_unlock(&sys->lk_netsys);
	    break;

        case CRE_ACCEPT_DONE:
        /* An attempt to accept a SCS connection has completed.  Process it
         * according to its completion status.
         * We add this system to a list of known systems if successful. 
         */
	    smp_lock(&sys->lk_netsys, LK_RETRY);
	    if( cmsb->status == ADR_SUCCESS ) {
		/*
	         * mark connection open and add to list of known systems 
		 */
		if (scsnetdebug)
	    	    printf( "scsnet - accept done from %c%c%c%c%c%c%c%c\n",
		        s->node_name[ 0 ], s->node_name[ 1 ],
		        s->node_name[ 2 ], s->node_name[ 3 ],
		        s->node_name[ 4 ], s->node_name[ 5 ],
		        s->node_name[ 6 ], s->node_name[ 7 ] );

	        if (scsnet_add_sys(sys->rmt_index, sys, cmsb) == RET_SUCCESS) {
		    if (scsnetdebug)
		        printf("scsnet - accept succeeded\n");

	        } else {

		    if (scsnetdebug)
	                printf( "scsnet - can not add to list of systems\n");
		    cmsb->Reason = ADR_NOLISTENER;
		    if((status =  scs_disconnect( cmsb ) != RET_SUCCESS) ) 
			mprintf("scsnet control: disconnect failed, status=%x\n",
			    status);
		    scsnet_rem_sys(sys, cmsb);

	        }

	    } else {

		if (scsnetdebug)
	            printf( "scsnet - SCS  accept failed from %c%c%c%c%c%c%c%c\n",
		        s->node_name[ 0 ], s->node_name[ 1 ],
		        s->node_name[ 2 ], s->node_name[ 3 ],
		        s->node_name[ 4 ], s->node_name[ 5 ],
		        s->node_name[ 6 ], s->node_name[ 7 ] );
	        scsnet_rem_sys(sys, cmsb);

	    }
	    smp_unlock(&sys->lk_netsys);
	    break;

        case CRE_REJECT_DONE:
	    break;
        case CRE_CONN_REC:
	    /* 
	     * connection request received.  If a connection has already
 	     * been established to the remote sysap then reject this request.
	     * If we have already initiated a connection request then reject the
	     * new request iff the remote sysid is greater than the local sysid.
	     * (Any deterministic method for deciding who wins will do).
	     * If we have not sent a connection request then accept this one.
	     */
	    {
	    struct _netsystem *netsystem;
	    ISB isb;
	    int rmthost = in_lnaof(scd->rmt_inaddr) & 0xff;

	    bzero((caddr_t)&isb, sizeof(ISB));
	    if (rmthost < 0 || rmthost > SCSNET_MAXHOSTS - 1 ) {
		cmsb->Reason = ADR_CONNECTION;
		printf("scsnet: received connection request from invalid host, host#=%d \n", rmthost);
		if ( (status = scs_reject( cmsb )) != RET_SUCCESS )
		    mprintf("scsnet reject failed, status=%d\n", status);
		return;

	    }

	    scsnet_status(cmsb->sysid, &status);
	    if (scsnetdebug)
	        printf("scsnet conn req recv from %x %x %x, status=%d\n", 
                    Scaaddr_hi( cmsb->sysid ), Scaaddr_mid( cmsb->sysid ),
                    Scaaddr_low( cmsb->sysid ), status );


	    switch (status) {

	    case SCSNET_CONN_OPEN:

	        cmsb->Reason = ADR_CONNECTION;

		
		if (scsnetdebug)
		    printf("scsnet: conn already open to %x %x %x\n",
                       Scaaddr_hi( cmsb->sysid ),
                       Scaaddr_mid( cmsb->sysid ), Scaaddr_low( cmsb->sysid ));

		if ( (status = scs_reject( cmsb )) != RET_SUCCESS )
		    mprintf("scsnet: failed to reject connection attempt from host %d, status=%d\n", rmthost, status);

		return;
		break;

	    case SCSNET_CONN_SENT:
		if (scsnetdebug) {
		    Print_sysid(scsnet_local.system.sysid)
		    printf("scsnet: conn req already sent to %x %x %x\n",
                        Scaaddr_hi( cmsb->sysid ), Scaaddr_mid( cmsb->sysid ),
                        Scaaddr_low( cmsb->sysid ));
		}
	        if( Sysid_greater(cmsb->sysid, scsnet_local.system.sysid) ) {

		    if (scsnetdebug)
		        printf("rejected\n");
		    cmsb->Reason = ADR_CONNECTION;
		    if ( (status = scs_reject( cmsb )) != RET_SUCCESS )
		        mprintf("scsnet: failed to reject concurrent connection attempt from host %d, status=%d\n", rmthost, status);
		    return;
	        }
		break;

	    case SCSNET_CONN_REC:
		mprintf("scsnet: duplicate connection request from: %x %x %x\n",
                Scaaddr_hi( cmsb->sysid ), Scaaddr_mid( cmsb->sysid ),
                Scaaddr_low( cmsb->sysid ));

		cmsb->Reason = ADR_CONNECTION;
		if ((status =  scs_reject( cmsb )) != RET_SUCCESS )
		    mprintf("scsnet: failed to reject connection attempt from host %d, status=%d\n", rmthost, status);
		return;
		break;

	    }

	    /*
	     * There is no connection to this system yet.  Check version
	     * and if ok accept the connection.
	     */

#ifdef notdef
	    if (scd->version < SCSNET_VERSION_ID) {
		if (scsnetdebug)
		    printf("version mismatch: local: %d, remote: %d\n", 
			SCSNET_VERSION_ID, scd->version);
		cmsb->Reason = ADR_VERSION;
		if ((status =  scs_reject( cmsb )) != RET_SUCCESS )
		    mprintf("scsnet: failed to reject connection attempt from host %d, status=%d\n", rmthost, status);
		return;
	    }
#endif


	    if ((netsystem=alloc_netsys())==SCSNET_NO_SYS) {
	   	printf( "SCS_NET - could not accept connection from host %d, too many systems\n", in_lnaof(scd->rmt_inaddr) & 0xff);
		return;
	    }

	    smp_lock(&netsystem->lk_netsys, LK_RETRY);
	    /*
	     * accept the connection
	     */
	    cmsb->control = scsnet_control;
	    cmsb->dg_event = scsnet_dgevent;
	    cmsb->msg_event = scsnet_msgevent;
	    cmsb->init_rec_credit = 5;
	    cmsb->init_dg_credit = 5;
	    cmsb->min_snd_credit = 5;

	    Move_scaaddr( cmsb->sysid, isb.next_sysid )
	    if( scs_info_system(&isb, (s = &netsystem->sys_info)) 
			!= RET_SUCCESS ) {
	     	printf( "scsnet - Can't Find Recv System in Database\n" );
		panic("scsnet - control");
	    }

	    netsystem->status = SCSNET_CONN_REC;
	    netsystem->rmt_index = in_lnaof(scd->rmt_inaddr) & 0xff;

	    if (scsnetdebug) {
	        printf("scsnet:  conn recv, netsys=%x, rmtindex=%d, ourhost=%d\n",
			netsystem, netsystem->rmt_index, scsnet_lhost);
	        printf( "scsnet: accept sent to %c%c%c%c%c%c%c%c \n",
			s->node_name[ 0 ], s->node_name[ 1 ],
			s->node_name[ 2 ], s->node_name[ 3 ],
			s->node_name[ 4 ], s->node_name[ 5 ],
			s->node_name[ 6 ], s->node_name[ 7 ] );
	    }

	    /*
	     * send our internet address to the remote site
	     * so it can use it later on to figure out which connection
	     * to use
	     */
	    scd->rmt_inaddr = scs_inet_addr;
	    scd->version = SCSNET_VERSION_ID;

	    cmsb->aux = (u_char *)netsystem;
	    if (( status = scs_accept( cmsb )) != RET_SUCCESS )

	    /* Acceptance of the connection request failed.  Reject the
	     * connection request if the acceptance failed due to an
	     * allocation error.  Otherwise, panic as some serious problem
	     * has been encountered.  
	     */
		if ( status == RET_ALLOCFAIL ) {
		    netsystem->status = SCSNET_CONN_CLOSED;
		    cmsb->Reason = ADR_NORESOURCE;
		    if ( (status = scs_reject( cmsb )) != RET_SUCCESS )
		        mprintf("scsnet reject failed, status=%d\n", status);
		    }
		else
		    panic( "scsnet - ACCEPT Failed\n" );

	    smp_unlock(&netsystem->lk_netsys);
	    }
	    break;
	
        default:
	    panic( "scsnet - Unknown or Unexpected Event Reported\n" );
	}
}


scsnet_output(ifp, m0, dst)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
{
	register struct scsnet_cntl *scscp;
	register struct mbuf *m = m0;
	register int i;
	struct scsnet_msg *msg;
	CSB csb;
	u_char *off;
	struct _netsystem *nextsys;
	int notblock = 0;
	int len;
	struct in_addr inet_dst;
	struct _netsystem *scs_dst;
	long type;
	int status;
	int ipl;
	int broadcast = 0;
	struct mbuf *mchain = 0;
	extern u_short scs_msg_size;
	int rspid;
	
        if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING)) {
                return(ENETDOWN);
        }

	if (dst->sa_family == AF_INET) {
		inet_dst = ((struct sockaddr_in *)dst)->sin_addr;
		if ((broadcast = in_broadcast(inet_dst))) 
			nextsys = netsystems;
		else { 
			Scsnet_resolve(inet_dst, scs_dst)
		}
		type = AF_INET;
	} 
	else
	if (dst->sa_family != AF_UNSPEC) {
		printf("scsnet output:  unknown family %x\n", dst->sa_family);
		m_freem(m0);
		return (EAFNOSUPPORT);
	}

broadcastloop:
	if (broadcast) {
		while (nextsys <= scsnet_topsysaddr)
			if (nextsys->status == SCSNET_CONN_OPEN) {
				scs_dst = nextsys;
				nextsys++;
				m = m0;
				goto send;
			} else
				nextsys++;

		m_freem(m0);
		return (0);
	}
		
send:
	len = ntohs((u_short)mtod(m, struct ip *)->ip_len);

#ifdef notdef
	if (scsnetstaton)
		if (len < 100)
			scsnetstat[0]++; 
		else if (len < 200) 
			scsnetstat[1]++; 
		else if (len < 500)
			scsnetstat[2]++; 
		else if (len < 1000)
			scsnetstat[3]++; 
		else if (len < 2000)
			scsnetstat[4]++; 
		else if (len < 3000)
			scsnetstat[5]++; 
		else if (len < 4000)
			scsnetstat[6]++; 
		else if (len < 5000)
			scsnetstat[7]++; 
		else
		    scsnetstat[8]++;
#endif

        ipl = Splscs();
	smp_lock(&scs_dst->lk_netsys, LK_RETRY);
	/* recheck status just in case the connection went away before we
	 * grabbed the lock.   We grab the lock here because we dont want
	 * to keep the output routine at high ipl very long.
	 */
	if (scs_dst->status != SCSNET_CONN_OPEN) {
		smp_unlock(&scs_dst->lk_netsys);
		if (broadcast) {
			(void)splx(ipl);
			goto broadcastloop;
		}
		else {
			if (scsnetdebug)
		    	    printf("scsnet: connection not resolved to %d\n", 
				in_lnaof(inet_dst) & 0xff);	
			m_freem(m0);					
			(void)splx(ipl);
			return(0);				
		}
	}

	/* if we can stuff everything into a datagram then do it.
	 * Otherwise try a block transfer
	 */
	if (len > scs_dg_size - SCSNET_HDR_SIZ) {


	    /* block transfer
	     * Move mbuf data to a virtually contiguous buffer.
	     * Place protocol header in sequenced msg.
	     * Send the sequenced msg to the remote host requesting
	     * it to block transfer the data from us to it.
	     * The block transfer will take place asynchronously.
	     * We release mbuf resourses when we receive a sequenced
	     * msg acknowledgment that the transfer completed.
	     * There can be SCSNET_XFERS outstanding transmissions.
	     * If we cannot transmit the current packet because
	     * the quota of outstanding transmissions has been reached then
	     * the packet is queued. 
	     * If we can not allocate resources for the block transfer 
	     * or the ack indicates that the remote
	     * host could not perform the block transfer
	     * then we release the resouces and return with ENOBUFS.
	     */
		 

	    /*
	     * Find a free buffer 
	     */
		

	    scscp = scsnet_xfers + scs_dst->rmt_index*SCSNET_XFERS;
	    {
	        register struct scsnet_cntl *lim = scscp + SCSNET_XFERS;

	        for (; scscp < lim;  scscp++) 
		    if (scscp->state & SCSNET_BDONE)
			goto got_one;

		if (scsnetdebug)
	        	printf("scsnet out: no free buffers for host= %d\n", 
				scs_dst->rmt_index);

		/* Currently smp locked by knownsystems but should use 
		 * its own lock if we get rid of knownsystems lock above
		 */
		Scsnet_If_Enqueue_Return 


got_one:
		scscp->timer = 0;
	        rspid = scs_dst->rmt_index*SCSNET_XFERS + scscp->xindex;

	    }
	
	
	   /*
	    * we have all the resources we need.  Theoretically
	    * the remote side does also.
	    */
	

	    if (broadcast) {
		 if ((mchain = m_copy(m0, 0, M_COPYALL))==0) {
			if (scsnetdebug)
			    printf("scsnet_out: could not copy broadcast msg\n");
		
			smp_unlock(&scs_dst->lk_netsys);
			Scsnet_Return_ENOBUFS 
		 }
	    }
	    else 
		mchain = m0;
		 

	   /*
	    * save pointer to mbuf chain so that we can free it later 
	    */
	    scscp->mchain = mchain;

#ifdef SCSNETDEBUG
	    if (scsnetdebug > 1)
		mprintf("scsnet_out: len=%d\n", len);
#endif
	   /*
	    * set up command packet 
	    */
	    msg = (struct scsnet_msg *)scscp->csb.buf;
	    msg->rspid = rspid;
	    msg->cmd = SCSNET_BCMD_REQUEST;

	   /*
	    * copy protocol type and header to message buffer
	    */

	    bcopy(&type, msg->proto_hdr, SCSNET_HDR_SIZ);
	    bcopy(mtod(m, caddr_t), msg->proto_hdr+SCSNET_HDR_SIZ, m->m_len);

	    scscp->csb.size = Scsnet_msg_hdr_siz + SCSNET_HDR_SIZ + m->m_len;

	    off = scscp->csb.buf + scscp->csb.size;

	    if (scscp->csb.size > scs_msg_size)
		panic("SCSNET: proto header to long");

	    len -= m->m_len;


	    /*
	     * Put leading small mbufs in seq message. This
	     * will ensure that leading data is page aligned.
	     */
	    m = m->m_next; /* point to data */
		
	    while (m && scscp->csb.size + m->m_len <= scs_msg_size)  {
		bcopy(mtod(m, caddr_t), off, m->m_len);
		off += m->m_len;
		len -= m->m_len;
		scscp->csb.size += m->m_len;
		m = m->m_next;
	    }
		
	    /*
	     * A page may also be chopped.  If the data is larger than
	     * NBPG then the upper levels try to use pages. Otherwise
	     * little mbufs are used.  The protocol layers may cut 
	     * a page in pieces by adjusting the offset to the page
	     * buffer.  The offset to a page mbuf should always be
	     * greater than MMAXOFF.  We have to be prepared to transmit
	     * an offset page at the beginning of a chain and a cut
	     * page at the end of a chain.
	     * We should also be prepared for type II mbufs in which
	     * the data may not start on a page boundary and can be
	     * more than CLBYTES in length.  We can not assume that
	     * the data area in a type II mbuf comes from the mbuf pool
	     * i.e.  the use of mbuf specific macros could fail.
	     */


	    /* We should now be pointing to data.  If the data is page
	     * aligned then we can use the pfn from the mbuf pages instead
	     * of copying.  
	     */
	    {
	        register caddr_t dp;
		register struct pte *frompte;
		register struct pte *topte = &scsmemptmap[scscp->xpte];
	        register caddr_t tova = (caddr_t) scsxtob(scscp->xpte);
		int rem = 0;
		

#define Mbufpage(m)	(m->m_off > MMAXOFF)  


		msg->off = 0;
	        while (m) { 
		    int pgoff = 0;
		    

		    dp = mtod(m, char *);
		    pgoff = (int)dp & PGOFSET;
	            if ( Mbufpage(m) && claligned(tova) 
				     && pgoff+m->m_len >= NBPG){
			    

			/*
			 * if the offset is nonzero then this is a 
			 * leading cut mbuf.  We can only offset from the
			 * beginning of a chain.  If we encounter a
			 * second offset mbuf we have to copy the data
			 * to a page aligned buffer and map the new buffer.
			 */
			if (pgoff) {
			    if (msg->off)
				break;
			    msg->off = pgoff;
			} 
			
/* Turn virtual address into a kernel map index */
#define mxtob(x) (((int)x - (int)Sysbase) >> PGSHIFT)

		        frompte = &Sysmap[mxtob(dp)];

			for (rem = pgoff + m->m_len; rem >= NBPG; rem -= NBPG) {
			    *topte++ = *frompte++;
			    len -= NBPG;
			    Tbis(tova);
			    tova += NBPG;
		        }

			len += pgoff;  /* the above loop has to be adjusted
					* for a mbuf that is offset
					*/
			
			/* 
			 * if there is still something left in this mbuf
			 * we can still map it if there is no other data.
			 * Otherwise we have to copy the
			 * remainder to a buffer.
			 */
			if (rem)  
			    if (m->m_next) {
				break;
			    } else {
			        *topte++ = *frompte++;
			        Tbis(tova);
				len -= rem;
				tova += rem;  
				rem = 0; /* dont need to do this I think */
			    }
			    
		    	m = m->m_next;

		    } else 
			break;
		}

	        /* 
		 * the rest of the data is in small mbufs or is
		 * not page aligned. 
		 * We grab as much space as needed and copy the 
		 * rest of the mbuf chain to it.
		 */

		if (m)  {
		    register int slen = 0;
		    caddr_t clbuf_t;
		    int alloc_len;
		    caddr_t va = tova;

		    /*
		     * need things page aligned so grag at least a page
		     */
		    if (len < NBPG)
		    	alloc_len =  NBPG;
		    else 
			alloc_len = len;
		    KM_ALLOC(clbuf_t, caddr_t, alloc_len, KM_DEVBUF, KM_NOWAIT);
		    if (clbuf_t == 0) {
			if (scsnetdebug)
	        	    printf("scsnet out: KM_ALLOC failed\n");
			if (broadcast)
				m_freem(mchain);
			scscp->mchain = 0;
			smp_unlock(&scs_dst->lk_netsys);
			Scsnet_Return_ENOBUFS 
		    }

		    scscp->clbuf_t = clbuf_t;      /* save to free up later */
		    scscp->clbuf_len = alloc_len;  /* save to free up later */

		    /*
		     *  map out the kmalloc'ed space and copy the remaining
		     *	data
 		     */
		    frompte = &kmempt[btokmemx((struct pte *) clbuf_t)];

		    for (i = rbtop(len) ; i > 0; i--) {
			    *topte++ = *frompte++;
			    Tbis(va);
			    va += NBPG;
		    }

		    if (rem) {
		        /*
		     	 * take care of remainder from last big mbuf
		     	 */

#define remoff	(m->m_len - rem)

		        bcopy(mtod(m, caddr_t) + remoff, tova, rem);
#undef remoff
		    	tova += rem;
			slen += rem;
		    	m = m->m_next;
		    }
			
		        
		    while (m) {

			if ((slen += m->m_len) > len) {
			    panic("scsnet: NO ROOM for tail mbuf");
			}
		        bcopy(mtod(m, caddr_t), tova, m->m_len);
		    	tova += m->m_len;
		    	m = m->m_next;
		    }

	        }

	        msg->totlen = tova - (caddr_t)scsxtob(scscp->xpte) - msg->off;
		
	    }

#ifdef notdef
	    tbsync();   /* for multi-processor translation buffer */
#endif

#ifdef mips
	    /*
	     * map buffer before we send it. Can not pre-map for mips box
	     * because of the pte translation that occurs for the CI
	     */
	    scscp->bufhdr.b_un.b_addr = (char *)scsxtob(scscp->xpte);
	    scscp->bufhdr.b_bcount = msg->totlen + msg->off;
	    scscp->bufhdr.b_flags = 0;
	    scscp->csb.Sbh = &scscp->bufhdr;

	    if ((status = scs_map_buf(&scscp->csb)) != RET_SUCCESS) {
			printf("scsnet: could not map transmit buffer, status=%x\n", status);
			if (broadcast)
				m_freem(mchain);
			scscp->mchain = 0;
			if (scscp->clbuf_t)
				KM_FREE(scscp->clbuf_t, KM_DEVBUF);

			smp_unlock(&scs_dst->lk_netsys);
			Scsnet_Return_ENOBUFS 
	    }
#endif
	    Move_bhandle(scscp->csb.lbhandle, msg->bhandle);

	    /* send message to remote - do not free mbufs till response */
	    /* We will need a background timer to check up on things */

	    scscp->csb.Disposal = RECEIVE_BUF;  /* for receive ack */

	    if ((status=scs_send_msg(&scscp->csb)) != RET_SUCCESS) {
		if (scsnetdebug)
			printf("scs_net: send failed, %x\n", status);
		if (broadcast)
			m_freem(mchain);
		scscp->mchain = 0;
		if (scscp->clbuf_t)
			KM_FREE(scscp->clbuf_t, KM_DEVBUF);

		smp_unlock(&scs_dst->lk_netsys);
		Scsnet_Return_ENOBUFS 

	    }

	    scscp->state = SCSNET_BSTARTED;

	}
	else  /* not a block transfer */
	{
		

	    notblock = 1;
	    csb.connid = scs_dst->connid;
	    if ((status=scs_alloc_dg(&csb))!=RET_SUCCESS) {
		if (scsnetdebug)
	            printf("scsnet: scs_alloc_dg failed, status= %x\n", status);
		smp_unlock(&scs_dst->lk_netsys);
		Scsnet_Return_ENOBUFS 
	    }
	    /* copy protocol type and data to scs buffer */
	    bcopy(&type, csb.buf, SCSNET_HDR_SIZ);
	    off = csb.buf + SCSNET_HDR_SIZ;
	    while (m) {
		bcopy(mtod(m, caddr_t), off, m->m_len);
		off += m->m_len;
		m = m->m_next;
	    }


	    if ((csb.size = off - csb.buf) > scs_dg_size) 
		printf("scsnet data gram too large: %d\n", csb.size);

	    csb.Disposal = DEALLOC_BUF;


	    /* send data to remote host */

	    if ((status = scs_send_dg(&csb)) != RET_SUCCESS)
	        printf("scsnet: send_dg failed, status= %d\n",status);
	}
	ifp->if_opackets++;
	smp_unlock(&scs_dst->lk_netsys);
	(void)splx( ipl );

	/* start timer to ensure that we eventually free mbuf chain */
	/* moved here to improve smp performance */
	if (notblock == 0 && !scsnet_timer++)
		timeout(scsnet_watch, 0, scsnet_timeo);

	if (broadcast) { 
		goto broadcastloop;
	}
	/* 
	 * do not free up mbuf chain if block transfer.  We will free 
	 * the mbuf chain when we receive an acknowledgment
	 */
	if (notblock)
		m_freem(m0);

#ifdef  notdef
	/* 
	 * Check for more to send
	 */
    	IF_DEQUEUE(&scsnetif.if_snd, m)
    	if (m == 0) 
	      return(0);
	 /* m contains temp info - the rest of the chain will
	  * free up later.
	  */ 
	m0 = m->m_next;
	/* initialize variables and try to send */
	notblock = broadcast = 0; 
	dst = mtod(m, stuct sockaddr *);
	m = m0;
	m_free(m);
	goto start;
#endif

	return(0);
}


/*
 * Process an ioctl request.
 */
scsnet_ioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	int error = 0;

	switch (cmd) {

	case SIOCSIFADDR:
		/*
		 * Everything else is done at a higher level.
		 */
		if (! (ifp->if_flags & IFF_UP) ) /* should use diff flag? */
			if (scsnet_init())
				return (EIO);
			ifp->if_flags |= IFF_UP;
			
		break;

	default:
		error = EINVAL;
	}
	return (error);
}

/*
 * scsnet_dgevent() - datagrams received here.  copy data into mbufs
 *	and insert them into appropriate queue (INET only for now)
 */
void
scsnet_dgevent(csb)
register CSB *csb;
{
	register struct ifnet *ifp = &scsnetif;
	register u_long totlen = csb->size;  /* length of packet */
	register u_char *cp = csb->buf;      /* pointer to packet data */
	register struct mbuf **mp; 	     /* mp is the next spot in the
				              * chain to assign mbuf 
				     	      */
	struct mbuf *top = 0;	     /* beginning of new mbuf chain */
	int ret;

        if (!(ifp->if_flags & IFF_UP))
		goto requeue;

	/* copy data from scs buffer to mbuf chain */

	mp = &top;

	while (totlen) {
		register struct mbuf *m;

		MGET(m, M_DONTWAIT, MT_DATA);
		if (m==0) {
			if (scsnetdebug)
			    printf("scsnet_dgevent: could not copy csb to mbuf \n");
			if (top)
			    m_freem(top);
			goto requeue;
		}
		if (totlen >= CLBYTES) {
			struct mbuf *p;
			MCLGET(m, p);
			if (p==0) 
				m->m_len = MIN(MLEN, totlen);
			else 
				m->m_len = CLBYTES;
		} else 
			m->m_len = MIN(MLEN, totlen);
		bcopy(cp, mtod(m, caddr_t), (unsigned)m->m_len);
		cp += m->m_len;
		*mp = m;
		mp = &m->m_next;
		totlen -= m->m_len;
	}

	/* enqueue packet on protocol queue */

	ifp->if_ipackets++;

	Scsnet_enqueue(top)

	/* requeue buffer for reception */
requeue:
	csb->Nbufs =0; 
	if ((ret = scs_queue_dgs (csb)) != RET_SUCCESS) 
		printf("scsnet_dgevent requeue failed 2, ret = %d\n", ret);


}



/*
 * scsnet_msgevent
 * Receive a message.  The message is either a request for starting a 
 * block transfer or an acknowledgment to the request.
 * If we are receiving an ack then we want to deallocate our message buffer.
 * If we are sending an ack we want to return the buffer to the receive 
 * pool to prepare for new requests.
 */


struct pte zeropte = {0,0,0,0,0,0,0,0};

void
scsnet_msgevent(csb)
	register CSB *csb;
{
	register struct scsnet_msg *msg = (struct scsnet_msg *)csb->buf;
	register int totlen = msg->totlen;
	register struct scsnet_cntl *bp = 0;
	register struct mbuf *m = 0;
	register int recvx;
	caddr_t recvbuf;
	int status;
	

	if (msg->cmd == SCSNET_BCMD_REQUEST) {
	    /* start block transfer from the remote host 
	     * First gather up required resources. 
	     * If can not get resources send back a negative ack.
	     */
	
	    register int clen = csb->size - Scsnet_msg_hdr_siz;
	    register struct mbuf *p;
	    register struct ifnet *ifp = &scsnetif;
	    void scsnet_freem();
	    struct mbuf *mclgetx();

            if (!(ifp->if_flags & IFF_UP))
		goto bad;

	    MGET(m, M_DONTWAIT, MT_DATA)
	    if (m==0) 
		goto bad;
	    if (clen > MLEN) {
	    	MCLGET(m,p)
	    	if (p==0) 
		    goto bad;
	    }
	    
	    m->m_len = clen;

	    /*
	     * copy protocol header into mbuf 
	     */
	    bcopy(msg->proto_hdr, mtod(m, caddr_t), m->m_len);


	     /*
	      * get a free transfer control block
	      */
	    smp_lock(&lk_scsnetrecvs, LK_RETRY);
	    for (recvx=0, bp=scsnet_recvs; bp < &scsnet_recvs[SCSNET_RECVS];
		bp++,recvx++)
			if (bp->state & SCSNET_BDONE)
				break;

	    if (recvx >= SCSNET_RECVS) {
		if (scsnetdebug)
			printf("scsnet: could not allocate recvbuf\n");
	        smp_unlock(&lk_scsnetrecvs);
		goto bad;
	    }

	    bp->state = SCSNET_BSTARTED;	
	    smp_unlock(&lk_scsnetrecvs);

	    KM_ALLOC(recvbuf, caddr_t, totlen, KM_DEVBUF, KM_NOWAIT);
	    if (recvbuf == 0) {
		if (scsnetdebug)
			printf("scsnet: could not kmalloc recvbuf\n");
	        bp->state = SCSNET_BDONE;	
		goto bad;
	    }

	    m->m_next = mclgetx(scsnet_freem, (int)recvbuf, recvbuf, 
			totlen, M_DONTWAIT);
	    if (m->m_next == 0) {
		KM_FREE(recvbuf, KM_DEVBUF);
		if (scsnetdebug)
			printf("scsnet: could not allocate recv cluster\n");
	        bp->state = SCSNET_BDONE;	
		goto bad;
	    }
		
	    /*
	     * set up buffer and map it 
	     */

	    bp->bufhdr.b_un.b_addr = recvbuf;
	    bp->bufhdr.b_flags = 0;
	    csb->Sbh = &bp->bufhdr;
	    bp->bufhdr.b_bcount = msg->totlen;
	    csb->Blockid = recvx;  /* will identify us during BLOCK_DONE */

	    if ((status = scs_map_buf(csb)) != RET_SUCCESS) {
			printf("scsnet: could not map recv buffer\n");
	        	bp->state = SCSNET_BDONE;	
			goto bad;
	    }

	    /*
	     * Save info for Block Done event.  When the block transfer
	     * completes we will need the protocol header in the csb
	     * buffer and we will need the pointer to the rest of data (m).
	     * The local block handle (lbhandle) is needed to
	     * unmap the block when we are finished with it.
	     * We also save the buffer to use for an ACK later on.
	     */
	    Move_bhandle(csb->lbhandle, bp->csb.lbhandle)
	    bp->csb.buf = csb->buf;	      
	    bp->mchain = m;			


	    /*
	     * Start up the block transfer
	     */
	    Move_bhandle(msg->bhandle, csb->rbhandle)
	    csb->Rboff = msg->off;
	    csb->Lboff = 0;
	    csb->size = msg->totlen;
	    if ((status=scs_req_data(csb))!=RET_SUCCESS){
		if (scsnetdebug)
		    printf("scsnet: req_data failed , status= %x\n",status);
		if ((status = scs_unmap_buf(csb) != RET_SUCCESS))
			printf("scsnet0: unmap failed, status=%d\n",
				status);
	        bp->state = SCSNET_BDONE;	
		goto bad;
	    }

	    bp->csb.Aux = csb->Aux;  /* save address of netsystem structure */
	    return;
bad:
	    if (m)
	        m_freem(m);

	    msg->cmd = SCSNET_BCMD_NACK;
	    csb->size = Scsnet_msg_hdr_siz;
	    csb->Disposal = RECEIVE_BUF;
	    if (bp)
	    	bp->mchain = 0;			
	    if ((status=scs_send_msg(csb)) != RET_SUCCESS)
		printf("scsnet: send nack failed, status = %d\n", status);
	    return;
	} else

	if (msg->cmd == SCSNET_BCMD_ACK || msg->cmd == SCSNET_BCMD_NACK) {
	    /*
	     * acknowledgement.
	     * free mbuf chain.
	     */
	    register struct scsnet_cntl *scscp = scsnet_xfers + msg->rspid; 
#ifdef notdef
	    register struct pte *topte = &scsmemptmap[scscp->xpte];
	    register caddr_t tova = (caddr_t) scsxtob(scscp->xpte);
	    register int i;
#endif
			

	    if (scscp->state & SCSNET_BDONE) {
		panic("scsnet: block transfer dup"); 
	    }

	    if (scscp->mchain)
	    	m_freem(scscp->mchain);

	    scscp->mchain = 0;

#ifdef mips
	    /*
	     * unmap buffer for mips only
	     */
	    if ((status = scs_unmap_buf(&scscp->csb)) != RET_SUCCESS) 
		printf("scsnet: could not unmap xmit buffer, status %x, %x\n",
			status, scscp);
#endif
	
	    /* 
	     * reuse msg buffer for next xmit
	     */

	    scscp->csb.buf = csb->buf;

#ifdef notdef
	    /*
	     * invalidate pte's and buffer cache entries
	     */

	    for(i=0; i<scscp->clbuf_len; i += NBPG) {
			*topte++ = zeropte;
			Tbis(tova);
			tova += NBPG;
	    }
#endif

	    if (scscp->clbuf_t) {
		KM_FREE(scscp->clbuf_t, KM_DEVBUF);
	    	scscp->clbuf_t = 0;
	    }
	    scscp->state = SCSNET_BDONE;

	    /*
	     * Since we have freed up one output buffer we can check
	     * for a pending transmission and send it.
	     */

	    smp_lock(&scsnetif.if_snd.lk_ifqueue, LK_RETRY);
	    IF_DEQUEUE(&scsnetif.if_snd, m)
	    smp_unlock(&scsnetif.if_snd.lk_ifqueue);
	    if (m == 0) 
		return;

	    scsnet_output(&scsnetif, m->m_next, mtod(m, struct sockaddr *));

	    m_free(m); /* only free temp header - the rest of the chain will
			* free up elsewhere.
			*/ 
	    return;

	} else 

	    printf("scsnet: received invalid message\n", msg->cmd);
	

		
}



/*
 * scsnet_add_sys() - add system to list of known systems.  First
 *	check to see if inet address is valid.
 * Inputs:
 *	rmthost - host number of system to add.
 *	sys - netsystem structure that contains connection info.
 *	ASSUME: sys is smp locked.
 *		IPL_SCS 
 *	cmsb - contains connid.
 * Outputs:
 *	updated knownsystems list.
 *	initialized block transfer structures;
 * Return value:
 *	RET_SUCCESS - ok.
 *	RET_FAILURE - invalid inet address. 
 */

int
scsnet_add_sys(rmthost, sys, cmsb)
	register int rmthost;
	struct _netsystem *sys;
	CMSB *cmsb;
{
	register struct scsnet_cntl *scscp;
	register int firstpte, i;
	int status;

	if (scsnetdebug)
		printf("scsnet_add_sys: rmthost=%d\n", rmthost);

	/* check validity of inet address */
	if (rmthost>=SCSNET_MAXHOSTS || rmthost<0) {
		printf("scs_net: can not add invalid host %d\n", rmthost);
		return(RET_FAILURE);
	}
	smp_lock(&lk_scsnetknownsys, LK_RETRY);
	if (knownsystems[rmthost] != SCSNET_NO_SYS)  
	    if (rmthost == scsnet_lhost) {
		/* we already have connection info for the localhost */
		sys->status = SCSNET_CONN_CLOSED;
		sys->retries = 0;
		Move_scaaddr( scsnet_nosysid, sys->sys_info.sysid)
		smp_unlock(&lk_scsnetknownsys);
		return(RET_SUCCESS);
	    }
	    else
		{
		mprintf("scsnet: conn already exists to %d, state = %x\n", 
			rmthost, knownsystems[rmthost]->status);
		smp_unlock(&lk_scsnetknownsys);
		return(RET_FAILURE);
	    }

        Insert_entry( sys->flink, netsys_head );

	/* initialize block transfer buffers */
	scscp = scsnet_xfers + rmthost*SCSNET_XFERS;
	firstpte = rmthost*SCSNET_XFERS*(SCSNET_XFER_SIZ+1); /* +1 for offset */
	for (i=0; i<SCSNET_XFERS; i++, scscp++) {
		scscp->xindex = i;
		scscp->xpte = firstpte + i*(SCSNET_XFER_SIZ+1);
		scscp->bufhdr.b_un.b_addr = (char *)scsxtob(scscp->xpte);
		scscp->bufhdr.b_bcount = (SCSNET_XFER_SIZ+1) * NBPG;
		scscp->bufhdr.b_flags = 0;
		scscp->csb.Sbh = &scscp->bufhdr;
		scscp->netsys = sys;
		Move_connid( cmsb->connid, scscp->csb.connid )
#ifdef vax
		if ((status = scs_map_buf(&scscp->csb)) != RET_SUCCESS) {
			printf("scsnet: could not map vax xmit buffer, status %x\n",
				status);
			knownsystems[rmthost] = SCSNET_NO_SYS;
			smp_unlock(&lk_scsnetknownsys);
			return(RET_FAILURE);
		}
#endif
		scscp->state = SCSNET_BDONE;
	        /*
	         * allocate a msg buffer and save it for the acknowledgement 
	         * We may want to preallocate our buffers to save time 
	         */

	        if ((status = scs_alloc_msg(&scscp->csb)) != RET_SUCCESS) {
	            printf("scsnet add: no free seq msgs for host= %d\n", 
			rmthost);
		    knownsystems[rmthost] = SCSNET_NO_SYS;
		    smp_unlock(&lk_scsnetknownsys);
	            return(RET_FAILURE);
	        }
	}

    	Move_scaaddr( cmsb->rport_addr, sys->msb.rport_addr )
	sys->msb.lport_name = cmsb->lport_name;
	Move_name( SYSAP_NET, sys->msb.Lproc_name )

	Move_scaaddr( cmsb->sysid, sys->sys_info.sysid)
	sys->connid = cmsb->connid;
	sys->rmt_index = rmthost;
	sys->status = SCSNET_CONN_OPEN;
	knownsystems[rmthost] = sys;
	smp_unlock(&lk_scsnetknownsys);
	return(RET_SUCCESS);
}

/*
 * scsnet_rem_sys:  remove system from known system data base and deallocate
 *	any resources reserved for communication with that system
 *
 * Inputs:   
 *	sys - for system identication. 
 *	ASSUME - sys is smp locked.
 *		 IPL_SCS
 *	cmsb - for connection identification.
 *
 * Outputs:
 *	knownsystems
 *	scsnet_xmits
 *	scsnet_recvs
 *
 * Return Value:
 *	RET_FAILURE - invalid system id
 *	RET_SUCCESS 
 *	
 */

scsnet_rem_sys(sys, cmsb)
	register struct _netsystem *sys;
	register CMSB *cmsb;
{
	register int rmthost = sys->rmt_index;
	register struct scsnet_cntl *scscp;
	register int firstpte, i;
	int status;

	if (scsnetdebug)
	    printf("scsnet_rem_sys: rmthost=%d, status=%d\n", rmthost, sys->status);

	if (sys->status != SCSNET_CONN_OPEN) {
		sys->status = SCSNET_CONN_CLOSED;
		sys->retries = 0;
		sys->rmt_index = SCSNET_NO_INDEX;
		Move_scaaddr( scsnet_nosysid, sys->sys_info.sysid)
		return(RET_SUCCESS);
	}
	
	/*
	 * check validity of inet address 
	 */
	if (rmthost>=SCSNET_MAXHOSTS || rmthost<0) {
		printf("scsnet_rem_sys: invalid host %d\n", rmthost);
		return(RET_FAILURE);
	}

	smp_lock(&lk_scsnetknownsys, LK_RETRY);

	Remove_entry( sys->flink);

	knownsystems[rmthost] = SCSNET_NO_SYS;

	/*
	 * unmap all xmit blocks dedicated to remote host
	 */
	scscp = scsnet_xfers + rmthost*SCSNET_XFERS;
	for (i=0; i<SCSNET_XFERS; i++, scscp++) {

	    if (scscp->state&SCSNET_BDONE)
	        if ((status=scs_dealloc_msg(&scscp->csb)) != RET_SUCCESS)
		    printf("scsnet: dealloc0 failed, status = %x\n", status);

#ifdef vax
	    if ((status = scs_unmap_buf(&scscp->csb)) != RET_SUCCESS) 
		printf("scsnet: could not unmap xmit buffer, status %x, %x\n",
			status, scscp);
#endif

	    if (scscp->clbuf_t)
		KM_FREE(scscp->clbuf_t, KM_DEVBUF);

	    if (scscp->mchain) {
		m_freem(scscp->mchain);
		scscp->mchain = 0;
	    }

	    scscp->clbuf_t = 0;
	    scscp->state = SCSNET_BDONE;
	    scscp->timer = 0;


	}

	/*
	 * look for block transfer requests that were in progress and
	 * clean them up.  
 	 * Note the aux/Aux field of the CMSB and the CSB respectively
	 * contain the address of the netsystem structure dedicated to
	 * a connection.
	 */

	for (scscp=scsnet_recvs; scscp < &scsnet_recvs[SCSNET_RECVS]; scscp++){
		if ((scscp->state&SCSNET_BDONE)==0 &&
		   (cmsb->aux == scscp->csb.Aux) ) {
		    /* 
		     * The assumption is made that a disconnect is not
		     * sent while a block transfer is in progress.
		     * The only way to get here is with a path failure.
		     */

		    if (scsnetdebug)
			printf("scsnet: unmapping recv buffer\n");

	    	    if ((status = scs_unmap_buf(&scscp->csb)) != RET_SUCCESS) 
			printf("scsnet: could not unmap recv buffer, status %x, %x\n",
			status, scscp);
		    scscp->state = SCSNET_BDONE;
		    scscp->timer = 0;
		    scscp->csb.Aux = 0;
		    if (scscp->mchain) {
			m_freem(scscp->mchain);
		    	scscp->mchain = 0;
		    }
	            if ((status=scs_dealloc_msg(&scscp->csb)) != RET_SUCCESS)
		        printf("scsnet: dealloc2 failed, status = %x\n", status);
		}
	}
	sys->status = SCSNET_CONN_CLOSED;
	sys->retries = 0;
	Move_scaaddr( scsnet_nosysid, sys->sys_info.sysid)
	
	smp_unlock(&lk_scsnetknownsys);
	return(RET_SUCCESS);

}

/* scsnet_connect  -  start a connection to a remote node.
 * 
 * Inputs: 
 * 	netsystem - system status information.
 *	ASSUME: netsystem is smp locked.
 *	scs_inet_addr - internet address of local host
 *	scsnetif - network interface information
 *	isb - ISB
 * Outputs:
 *	netsystem - connection status
 * Return Values:
 *	RET_SUCCESS
 *	RET_FAILURE - scs connect failed
 */

scsnet_connect(netsystem, isb)
struct _netsystem *netsystem;
ISB *isb;
{

	register struct ifaddr *ifa;
	struct ifnet *ifp = &scsnetif;
	extern struct in_addr scs_inet_addr;
	SIB *sib = &netsystem->sys_info;
	int error;
	int ipl;
	CMSB cmsb;
	struct scsnet_conn_data *scd 
		= (struct scsnet_conn_data *)cmsb.conn_data;


    	ipl = Splscs();
	if (scs_info_system( isb, sib ) == RET_NOSYSTEM){
		printf("scsnet connect - no systems\n");
		(void)splx( ipl );
		return(RET_FAILURE);
	}

	/*  check for correct type */
	if (U_long(*"U-32") != sib->swtype) {
		(void)splx( ipl );
		return(RET_FAILURE);
	}
		
	bzero((caddr_t)&cmsb, sizeof(CMSB));
    	Move_scaaddr( isb->sysid, cmsb.sysid )

	cmsb.control = scsnet_control;
	bcopy (SYSAP_NET, cmsb.lproc_name, NAME_SIZE);
	bcopy (SYSAP_NET, cmsb.rproc_name, NAME_SIZE);
	cmsb.dg_event = scsnet_dgevent;
	cmsb.msg_event = scsnet_msgevent;
	cmsb.init_rec_credit = 5;
	cmsb.init_dg_credit = 5;
	cmsb.min_snd_credit = 5;
    	Zero_scaaddr( cmsb.rport_addr )
    	Move_scaaddr( sib->sysid, cmsb.sysid )
	cmsb.aux = (u_char *)netsystem;

	/*
	 * send our internet address to the remote site
	 * so it can be used later on to determine correct connection
	 */
	scd->rmt_inaddr = scs_inet_addr;
	scd->version = SCSNET_VERSION_ID;
	

	if ((error=scs_connect(&cmsb))!=RET_SUCCESS) {
		if (scsnetdebug)
		  printf("scsnet_connect: failed to sysid: %x %x %x, error= %x\n", 
                        Scaaddr_hi( cmsb.sysid ),
                        Scaaddr_mid( cmsb.sysid ),
                        Scaaddr_low( cmsb.sysid ),
			error);
		(void)splx( ipl );
		return(RET_FAILURE);
	}

	netsystem->retries = 0;
	netsystem->status = SCSNET_CONN_SENT;
	netsystem->rmt_index = SCSNET_NO_INDEX;
	(void)splx( ipl );

	if (scsnetdebug)
	    printf("scsnet_connect: connecting to :: %x,%x,%x\n",
                Scaaddr_hi( cmsb.sysid ),
                Scaaddr_mid( cmsb.sysid ),
                Scaaddr_low( cmsb.sysid ));

	return(RET_SUCCESS);
}
	
/* alloc_netsys - allocate a netsys structure
 * 
 * Inputs:
 *	netsystems - pool of netsys structures. One is allocated 
 *			per connection
 *	ASSUME:  IPL_SCS
 *	
 * Return Codes:
 *	- address of a netsys structure
 *	SCSNET_NO_SYS - no netsys structures available
 *
 */

struct _netsystem *
alloc_netsys()
{
	register int i;

	smp_lock(&lk_netsystems, LK_RETRY);
	for(i=0; i<SCSNET_MAXHOSTS; i++)
		if (netsystems[i].status == SCSNET_CONN_CLOSED)
			break;
	if (i>=SCSNET_MAXHOSTS) {
		smp_unlock(&lk_netsystems);
		return(SCSNET_NO_SYS);
	}
	netsystems[i].rmt_index = SCSNET_NO_INDEX;
	if (scsnet_topsysaddr < &netsystems[i])
		scsnet_topsysaddr = &netsystems[i];
	netsystems[i].status = SCSNET_NETSYS_ALLOCATED;
	smp_unlock(&lk_netsystems);
	return(&netsystems[i]);
}

/* scsnet_status()
 *
 * check status of a remote connection.
 *
 * Input: sysid - sca system id of remote host 
 *
 * Output:
 *	status - the status of a connection.  
 *		SCSNET_CONN_CLOSED	0	
 *		SCSNET_CONN_SENT	1	
 *		SCSNET_CONN_OPEN	2
 *		SCSNET_CONN_REC		3
 *
 * Return code:
 *	RET_FAILURE - invalid host number
 *	RET_SUCCESS - a connection has been made or is in progess
 */

scsnet_status(sysid, status)
	scaaddr sysid;
	int *status;
{
	register int i;
	register struct _netsystem *sys = netsystems;
	
	*status = SCSNET_CONN_CLOSED;

	for (i=0; i<SCSNET_MAXHOSTS; i++, sys++)
		if (Comp_scaaddr(sys->sys_info.sysid, sysid) ) {
			*status = sys->status;
			return(RET_SUCCESS);
		}
		

	return(RET_FAILURE);

}

/* scsnet_watch() - 
 * watch dog timer to periodically check for unack'ed xmits. 
 * if a timeout has occured we assume the connection is dead.
 * The only way to recover resources is to crash the path.
 */


void
scsnet_watch()
{
	register struct scsnet_cntl *scscp;
	register struct scsnet_cntl *last = &scsnet_xfers[SCSNET_XMITS];
	register int keeptiming = 0;
	int status, ipl;
	
	for (scscp = scsnet_xfers;  scscp < last; scscp++) 
	    if (scscp->state&SCSNET_BSTARTED) 
	        if (scscp->timer++ > 4) {

		mprintf("scsnet: transmit timeout to host: %d\n", 
			(scscp - scsnet_xfers)/((sizeof(struct scsnet_cntl)) *
				SCSNET_XFERS));

        	ipl = Splscs();

        	if(scs_crash_path( &scscp->netsys->msb ) == RET_NOPATH ) 
           	    panic( "scsnet: scs_crash_path failure.\n" );

		(void)splx( ipl );


		{
		    register struct mbuf *m;
        	    ipl = Splscs();
	    	    smp_lock(&scsnetif.if_snd.lk_ifqueue, LK_RETRY);
	    	    IF_DEQUEUE(&scsnetif.if_snd, m)
	            smp_unlock(&scsnetif.if_snd.lk_ifqueue);
		    (void)splx( ipl );
	    	    if (m == 0) 
		        return;

	    	    scsnet_output(&scsnetif, m->m_next, mtod(m,struct sockaddr *)); 
	    	    /* only free temp info - the rest of the chain will
		     * free up elsewhere.
		     */ 
	    	    m_free(m);
		}

		} else
		    keeptiming++;
		    

	if (keeptiming) 
		timeout(scsnet_watch, 0, scsnet_timeo);
	else
		scsnet_timer = 0;
		
			
}


/*
 * scsnet_freem - free type II receive mbuf.
 *
 *	Input:  
 *		recvbuf  - pointer to buffer.
 */

void
scsnet_freem(recvbuf)
	int recvbuf;
{
	caddr_t rb = (caddr_t)recvbuf;

	KM_FREE(rb, KM_DEVBUF);

}

