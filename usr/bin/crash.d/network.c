#ifndef lint
static char *sccsid = "@(#)network.c	4.2	(ULTRIX)	10/8/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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

#include	"crash.h"
#include	<sys/types.h>
#define KERNEL
#include	<sys/file.h>
#undef KERNEL
#include	<sys/socket.h>
#include	<sys/socketvar.h>
#include	<net/if.h>
#include	<net/route.h>
#include	<netinet/in.h>
#include	<netinet/in_pcb.h>
#include	<netinet/if_ether.h>
#include	<netdb.h>

prsock(c, all, detail)
	int c;
	int	all;
	int detail;
{
	struct	file	*fp;
	struct socket socket;
	
	fp = &filetab[c];
	if(!all && fp->f_count == 0)
		return;
	if(fp->f_type != 2)
		return;			/* not a socket */
	if(readmem((char *)&socket, (int)fp->f_data, sizeof socket) !=
	    sizeof socket) {
		printf(" read error on socket\n");
		return;
	}
	printf("%4d %08x ", c, fp->f_data);
	switch(socket.so_type) {
		case SOCK_STREAM :
			printf(" STRM ");
			break;
		case SOCK_DGRAM :
			printf("DGRAM ");
			break;
		case SOCK_RAW :
			printf(" RAW  ");
			break;
		case SOCK_RDM :
			printf(" RDM  ");
			break;
		case SOCK_SEQPACKET :
			printf("SQPAK ");
	}
	printf("%08x %4d %4d %4d   %08x %4d   %08x\n", socket.so_pcb,
	socket.so_qlen, socket.so_qlimit, socket.so_snd.sb_cc,
	socket.so_snd.sb_sel, socket.so_rcv.sb_cc, socket.so_rcv.sb_sel);
	if (detail == 1) {
		prlock_long(&socket.lk_socket, 0);
		if (socket.so_rcv.sb_mb != NULL) {
			printf("Recieve mbufs:\n");
			prmbuf_chain(socket.so_rcv.sb_mb);
		}
		if (socket.so_snd.sb_mb != NULL) {
			printf("Recieve mbufs:\n");
			prmbuf_chain(socket.so_snd.sb_mb);
		}
	}
}

pr_sockhdr() {
	printf("Slot Sockaddr Type    PCB    Qlen Qlim  Scc    Sproc    Rcc    Rproc\n");
}

int
getarp(c, a)
	int c;
	struct arptab *a;
{
	if (c < 0)
		return(0);
	if (c > tab[ARP_T].ents) {
		printf("%4d out of range\n", c);
		return(0);
	}
	if(readmem((char *)a, tab[ARP_T].first + c * tab[ARP_T].size,
	sizeof *a) != sizeof *a) {
		printf("read error on arp table\n");
		return(0);
	}
	return(1);
}

int
findarp(str)
	char *str;
{
	int kludge;
	struct arptab a;
	int i;

	kludge = (int)inet_addr(str);
	for (i = 0; i < ARPTAB_SIZE; i++) {
		if (!getarp(i, &a))
			return(-1);
		if (a.at_iaddr.s_addr == (u_long)kludge)
			return(i);
	}
	return(-1);
}

void
prarp(c, all)
	int c;
	int all;
{
	struct arptab a;
	char ether[20];
	struct hostent *hp = NULL;
	extern struct hostent *gethostbyaddr();

	if (!getarp(c, &a))
		return;
	if (!all && (a.at_iaddr.s_addr == 0 || a.at_flags == 0))
		return;
	prether(a.at_enaddr, ether);
	hp = gethostbyaddr(&a.at_iaddr);
	if (hp) printf("%10s ",hp->h_name);
	else printf("_no_name__ ");
	printf("%4d %4d %15s  ", (c % ARPTAB_BSIZ), (c / ARPTAB_BSIZ), inet_ntoa(a.at_iaddr));
	printf("%15s", ether);
	printf("  %8x  %5d  %5x\n", a.at_hold, a.at_timer, a.at_flags);
}

int
prether(e, buf)
	u_char e[6];
	char *buf;
{
	int index;
	int i;

	sprintf(buf, "%d", e[0]);
	for (i = 1; i < 6; i++) {
		index = strlen(buf);
		sprintf(&buf[index], ".%x", e[i]);
	}
}
prinpcbhead()
{
	printf("   Foreign Host  FPort      Local Host   LPort  Socket       PCB        Options\n");
}
prinpcb(vaddr)
	unsigned vaddr;
{
	struct inpcb pcb;
	struct inpcb *pnxt;
	unsigned head;
	struct hostent *host;
	
	prinpcbhead();
	head=vaddr;
	do {
		if (get_pcb(vaddr, &pcb) == 0)
			return(0);
		prinaddr(pcb.inp_faddr);
		printf("%6d ",ntohs(pcb.inp_fport));
		prinaddr(pcb.inp_laddr);
		printf("%6d ",ntohs(pcb.inp_lport));
		printf("0x%8x ",pcb.inp_socket);
		printf("0x%8x ",pcb.inp_ppcb);
		if (pcb.inp_options != NULL)
			printf("*\n");
		else
			printf("\n");

		vaddr = (unsigned)pcb.inp_next;
	} while ((vaddr != NULL) && (vaddr != head));
}


prinaddr(addr)
	struct in_addr addr;
{
	static struct in_addr last;
	static struct hostent host, *h;

	if (addr.s_addr != 0) {
		if (addr.s_addr != last.s_addr)
			h = gethostbyaddr(&addr,4,AF_INET);
		if (h != NULL) {
			printf("%-16.16s ",h->h_name);
			last.s_addr=addr.s_addr;
			host = *h;
		} else
			printf("        Unknown ");
	} else
		printf("%-16s ",inet_ntoa(addr));

}
int
get_pcb(addr, pcb)
	unsigned addr;
	struct inpcb *pcb;
{

	if (readmem((char *)pcb, (int)addr, sizeof(*pcb)) 
	    != sizeof(*pcb)) {
		perror("pcb read");
		printf("read error on pcb at 0x%x\n", addr);
		return(0);
	}
	return(1);
}

