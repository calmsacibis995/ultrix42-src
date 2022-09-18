/*
 * Copyright (c) 1988-1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * SCCSID: @(#)interface.h	4.2	ULTRIX	1/25/91
 * Based on:
 * @(#) $Header: interface.h,v 1.35 91/01/10 16:30:35 mccanne Exp $ (LBL)
 */

#ifndef __STDC__
#define const
#endif
#ifdef __GNUC__
#define inline __inline
#else
#define inline
#endif

#include "md.h"			/* machine dependent stuff */
#include "os.h"			/* operating system stuff */

#ifndef __STDC__
extern char *malloc();
extern char *calloc();
#endif

extern int cflag;		/* only print 'cnt' packets */
extern int dflag;		/* print filter code */
extern int eflag;		/* print ethernet header */
extern int nflag;		/* leave addresses as numbers */
extern int Nflag;		/* remove domains from printed host names */
extern int qflag;		/* quick (shorter) output */
extern int Sflag;		/* print raw TCP sequence numbers */
extern int tflag;		/* print packet arrival time */
extern int vflag;		/* verbose */
extern int xflag;		/* print packet in hex */

extern int snaplen;
/* global pointers to beginning and end of current packet (during printing) */
extern unsigned char *packetp;
extern unsigned char *snapend;

extern long thiszone;			/* gmt to local correction */

extern char timestamp_fmt[];
extern long timestamp_scale;
extern void timestampinit();
extern int clock_sigfigs();

extern char *lookup_device();

extern void error();
extern void warning();

extern char *read_infile();
extern char *copy_argv();

extern void usage();
extern void show_code();
extern void init_addrtoname();

/* The printer routines. */

extern void ether_if_print();
extern void arp_print();
extern void ip_print();
extern void tcp_print();
extern void udp_print();
extern void icmp_print();
extern void default_print();

extern void ntp_print();
extern void nfsreq_print();
extern void nfsreply_print();
extern void ns_print();
extern void ddp_print();
extern void rip_print();
extern void tftp_print();
extern void bootp_print();
extern void snmp_print();
extern void sl_if_print();
extern void ppp_if_print();

#define min(a,b) ((a)>(b)?(b):(a))
#define max(a,b) ((b)>(a)?(b):(a))

/*
 * Sizeof MAXLINKHDR + ip header + tcp header.  
 * The printer routines assume these headers were captured.
 */
#define MIN_SNAPLEN 56

/* 
 * A better default, so most printers can print useful info. 
 * (In particular, this gets AppleTalk atp requests on an 
 * ethernet (14 link header).
 */
#define DEFAULT_SNAPLEN 68

/* 
 * Current version numbers.
 */
#define VERSION_MAJOR 2
#define VERSION_MINOR 0

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234
#endif
