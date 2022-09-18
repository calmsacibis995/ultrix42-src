#ifndef lint
static char *sccsid = "@(#)mbuf.c	4.1	(ULTRIX)	7/17/90";
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

#include "crash.h"
#undef MFREE
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <netinet/in.h>

int
get_sock(addr, sock)
	unsigned addr;
	struct socket *sock;
{

	if (readmem((char *)sock, (int)addr, sizeof(*sock)) != sizeof(*sock)) {
		perror("socket read");
		printf("read error on socket at 0x%x\n", addr);
		return(0);
	}
	return(1);
}

int
get_mbuf(addr, my_mbuf)
	unsigned addr;
	struct mbuf *my_mbuf;
{

	if (readmem((char *)my_mbuf, (int)addr, sizeof(*my_mbuf)) 
	    != sizeof(*my_mbuf)) {
		perror("mbuf read");
		printf("read error on mbuf at 0x%x\n", addr);
		return(0);
	}
	return(1);
}

int total_alloc;

struct sockaddr_in from;

mbuf_list(addr, flag)
	unsigned  addr;
	int flag;	/* 1 = send side
			   0 = receive side */
{
	struct socket sock;
	struct mbuf my_mbuf;
	struct mbuf *my_mbufp = &my_mbuf;
	int index = 0;
	unsigned act_addr;

	if (get_sock(addr, &sock) == 0)
		return(0);

	addr = (flag == 1 ? (unsigned)sock.so_snd.sb_mb : 
			    (unsigned)sock.so_rcv.sb_mb);

	if (addr == NULL) {
		printf("socket %x has null mbuf pointer\n", sock);
		return(0);
	}
	print_mbufhd();
	do {
		/*
		 * get and print addr mbuf
		 */
		act_addr = (flag == 1 ? (unsigned)sock.so_snd.sb_mb :
					(unsigned)sock.so_rcv.sb_mb);
		if (act_addr == 0)
			goto done;
		if (get_mbuf(act_addr, &my_mbuf) == 0)
				goto done;
		index++;
		addr = (unsigned)&my_mbuf;
		addr += my_mbuf.m_off;
		bcopy((struct sockaddr_in *)addr, &from,
			sizeof(struct sockaddr_in));
		prmbuf(&my_mbuf, act_addr, 1);
		if (flag)
			 sock.so_snd.sb_mb = my_mbuf.m_act;
		else
			 sock.so_rcv.sb_mb = my_mbuf.m_act;
		addr = (unsigned)my_mbuf.m_next;
		if (addr == NULL) {
			printf("mbuf 0x%x has null body\n", act_addr);
			continue;
		}
		/*
		 * now addr points to the first data mbuf
		 */
		do {
			index++;
			if (get_mbuf(addr, &my_mbuf) == 0)
				return(0);
			prmbuf(&my_mbuf, addr, 0);
			addr = (unsigned)my_mbuf.m_next;
		} while (addr);
	} while ((flag == 1 ? sock.so_snd.sb_mb : sock.so_rcv.sb_mb));
done:
	if (flag)
		printf("snd_cc %d total_mbuf_len %d\n", sock.so_snd.sb_cc, 
			total_alloc);
	else
		printf("rcv_cc %d total_mbuf_len %d\n", sock.so_rcv.sb_cc, 
	        	total_alloc);
}

print_mbufhd()
{
	printf("Slot  Address           Offset         Size\n");
}
prmbuf_chain(vaddr)
	unsigned vaddr;
{
	struct mbuf my_mbuf;
	struct mbuf *mchain;
	vaddr = (unsigned) dtom(vaddr); /* align to mbuf */
	if (get_mbuf(vaddr, &my_mbuf) == 0)
		return(0);
	do {
		mchain = my_mbuf.m_act;
		do {
			if (get_mbuf(vaddr, &my_mbuf) == 0)
				return(0);
			prmbuf(&my_mbuf, vaddr, 0);
			vaddr = (unsigned)my_mbuf.m_next;
		} while (vaddr);
		vaddr = (unsigned) mchain;
	} while (mchain);
}
prmbuf(my_mbuf, vaddr, flag)
	struct mbuf *my_mbuf;
	unsigned vaddr;
	int flag;
{
	int index;
	struct sockaddr_in *net_addr;

	switch (my_mbuf->m_type) {

	      case MT_FREE:
		      printf(" free!");
		      break;

	      case MT_DATA:
		      printf("  data");
		      break;

	      case MT_SOCKET:
		      printf("socket");
		      break;

	      case MT_PCB:
		      printf("   pcb");
		      break;

	      case MT_RTABLE:
		      printf(" route");
		      break;

	      case MT_HTABLE:
		      printf("  host");
		      break;

	      case MT_ATABLE:
		      printf("   arp");
		      break;

	      case MT_SONAME:
		      printf("soname");
		      break;

	      case MT_ZOMBIE:
		      printf("zombie");
		      break;

	      case MT_SOOPTS:
		      printf("soopts");
		      break;

	      case MT_FTABLE:
		      printf("fraghd");
		      break;

	      case MT_RIGHTS:
		      printf("rights");
		      break;

/*	      case MT_OPT:
		      printf("   opt");
		      break;
*/
	      case MT_IFADDR:
		      printf("ifaddr");
		      break;

	      case MT_ACCESS:
		      printf("access");
		      break;

	      default:
		      printf("*undef");
		      break;
	}
	if (my_mbuf->m_off > MMAXOFF) {
		vaddr = my_mbuf->m_off;
		my_mbuf->m_off = 0;
	}
	printf(" %8x %3d %5d ", 
	      vaddr, my_mbuf->m_off, my_mbuf->m_len);
	if (my_mbuf->m_type == MT_SONAME) {
		net_addr = mtod(my_mbuf, struct sockaddr_in *);
		printf(" %s",inet_ntoa(net_addr->sin_addr));
	}
	printf("\n");
	total_alloc += my_mbuf->m_len;
}
