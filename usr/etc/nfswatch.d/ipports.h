/*
 * SCCSID: @(#)ipports.h	4.2	ULTRIX	1/25/91
 * Based on:
 * $Header: /sparky/a/davy/system/nfswatch/RCS/ipports.h,v 3.0 91/01/23 08:24:17 davy Exp $
 *
 * ipport.h - port definitions used by nfswatch, not provided by Ultrix
 *
 * Jeffrey Mogul
 * DECWRL
 *
 * $Log:	ipports.h,v $
 * Revision 3.0  91/01/23  08:24:17  davy
 * NFSWATCH Version 3.0.
 * 
 * Revision 1.1  90/08/17  15:46:47  davy
 * Initial revision
 * 
 * Revision 1.1  90/04/20  13:59:23  mogul
 * Initial revision
 * 
 */

/*
 * In Ultrix and BSD, programs always use "getservbyname" to
 * do these translations, but I guess someone at Sun had to
 * build a case table at some point.
 */
#define IPPORT_ROUTESERVER	520	/* routing control protocol	*/
#define IPPORT_ECHO		  7	/* packet echo server		*/
#define IPPORT_DISCARD		  9	/* packet discard server	*/
#define IPPORT_SYSTAT		 11	/* system stats			*/
#define IPPORT_DAYTIME		 13	/* time of day server		*/
#define IPPORT_NETSTAT		 15	/* network stats		*/
#define IPPORT_FTP		 21	/* file transfer		*/
#define IPPORT_TELNET		 23	/* remote terminal service	*/
#define IPPORT_SMTP		 25	/* simple mail transfer protocol*/
#define IPPORT_TIMESERVER	 37	/* network time synchronization	*/
#define IPPORT_NAMESERVER	 53	/* domain name lookup		*/
#define IPPORT_WHOIS		 43	/* white pages			*/
#define IPPORT_MTP		 57	/* ???				*/
#define IPPORT_TFTP		 69	/* trivial file transfer	*/
#define IPPORT_RJE		 77	/* remote job entry		*/
#define IPPORT_FINGER		 79	/* finger			*/
#define IPPORT_TTYLINK		 87	/* ???				*/
#define IPPORT_SUPDUP		 95	/* SUPDUP			*/
#define IPPORT_EXECSERVER	512	/* rsh				*/
#define IPPORT_LOGINSERVER	513	/* rlogin			*/
#define IPPORT_CMDSERVER	514	/* rcmd				*/
#define IPPORT_BIFFUDP		512	/* biff mail notification	*/
#define IPPORT_WHOSERVER	513	/* rwho				*/
