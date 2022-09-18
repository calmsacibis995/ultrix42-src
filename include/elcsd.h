/* sccsid  =  @(#)elcsd.h	4.1	ULTRIX	7/2/90 */

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/************************************************************************
 * Modification history:
 *
 *  8-Mar-88 -- arce
 *	removed elver. Now it is defined ELVER in errlog.h
 *
 * 09-Jun-86 -- bjg
 *	Changed default mode on /usr/adm/syserr/syserr.<hostname>; 
 *	added error msg 70 for ioctl failure when getting system strt time
 *
 * 29-Apr-86 -- pmk
 *	Added 4 new messages 66-69
 *	Changed where elcsdlog goes, now in /usr/adm
 *	Added 3 new defines ELLOCK,ELSHTDWN_WNDW & FSPERCENT
 *
 * 02-Apr-86 -- pmk
 *	Added 3 new messages 63,64,&65. Changed where cntl. sockets
 *	are now put, in /dev.
 *
 * 19-Feb-86 -- pmk
 *	Added new elcsd message 62, removed trailer
 *	now in errlog.h, and added svelbuf[].
 *
 * 29 Jan 86 -- pmk 
 *	Added new elcsd message 61 and elicntlsckt
 *
 * 01 Dec 85 -- pmk 
 *	Created elcsd.h file for elcsd.c
 *
 ************************************************************************/

#ifndef __elcsd__
#define __elcsd__

#define MODE		0644
#define RHTSIZE		256
#define MAX_PATH	255
#define MAX_NLEN	32
#define WTSIZE  	10
#define OFTSIZE  	10
#define OFTINDX  	1
#define FSYNC		1
#define NOFSYNC		0
#define ALERT		1
#define NONALERT	0
#define BSIZE		512
#define LOCLOG		0x0001
#define LOGREM		0x0002
#define REMLOG		0x0004
#define PRILOG		0x0005
#define ELSTRTRLOG 	1
#define ELGOAHEAD 	2
#define ELSHUTDOWN 	3
#define ELSTARTUP 	4
#define ELNOGO	 	-1
#define ELWNDALL 	-1
#define ELOPEN_WNDW 	1
#define ELCLOSE_WNDW 	2
#define ELSHTDWN_WNDW 	3
#define ELDISABLE 	3
#define ELINITIALIZE 	4
#define ELRECONFIG 	5
#define ELLOCK	 	6
#define FSPERCENT	2		/* shutdown logging at 2% left of fs */

char el[] = "/syserr.";
char remotes[] = "remotes";
char cfgfile[] = "/etc/elcsd.conf";
char logpath[] = "/usr/adm/elcsdlog";
char elcsckt[] = "/dev/elcscntlsckt";
char elisckt[] = "/dev/elicntlsckt";

struct elcfg {
	short status;			/* status on/off line,remote logging */
	short limfsize;			/* limit errlog file size to */
	char elpath[MAX_PATH];		/* errlog file dir.path */
	char bupath[MAX_PATH];		/* backup el file dir.path */
	char supath[MAX_PATH];		/* single user mode el file dir.path */
	char rhpath[MAX_PATH];		/* remote el file dir.path (one file)*/
	char rhostn[MAX_NLEN];		/* logging local to remote hostname */
};
struct elcfg elcfg;

struct elrht {
	char hname[MAX_NLEN];		/* remote hostnames to log */
	char where;			/* R(remote file) or S(seperate file)*/
};
struct elrht elrht[RHTSIZE];
struct elrht *chkrht();

struct eloft {
	int fdes;
	char hname[MAX_NLEN];
	char fname[MAX_PATH];
	long marker;
};
struct eloft eloft[OFTSIZE] = { 0 };
struct eloft *eloftp, *gofts(),*goftp();

struct wndpkt {
	short msgtyp;
	short class;
	short type;
	short ctldevtyp;
	short num;
	short unitnum;
};
struct wndmsg {
	struct wndpkt wndpkt;
	int sfalen;
	struct sockaddr_un sfa;
};
struct wndwt {
	short class;
	short type;
	short ctldevtyp;
	short num;
	short unitnum;
	int wsalen;
	struct sockaddr_un wsa;
};
struct wndwt wndwt[WTSIZE];

struct elparam {
	long pid;
	long sid;
};

char *elcsmode[] = {
	"multi-user mode",
	"single user mode",
	"windowing only mode, no logging to disk!",
	0
};

char *elcsmsg[] = {
/*0*/	"\0",
/*1*/	"exiting, open error on",
/*2*/	"exiting, ioctl error getting errlog pid",
/*3*/	"startup in",
/*4*/	"exiting, ioctl error setting errlog pid",
/*5*/	"exiting, error gethostname:",
/*6*/	"select error",
/*7*/	"write pipe error",
/*8*/	"exiting, failure to log local errors to any errlog file",
/*9*/	"ioctl error moving kernel errlog output pointer",
/*10*/	"remote host shutdown",
/*11*/	"stream socket accept error",
/*12*/	"error forking wndw process",
/*13*/	"datagram socket sendto error",
/*14*/	"failure to log local priority errors to any errlog file",
/*15*/	"stream socket send error",
/*16*/	"gethostbyaddr error",
/*17*/	"stream socket recv error",
/*18*/	"unknown host check /etc/hosts file for entry:",
/*19*/	"logging remote systems too!",
/*20*/	"read pipe error",
/*21*/	"unix domain socket sendto error",
/*22*/	"no errlog path for",
/*23*/	"exiting, no hostname - hostname not set!",
/*24*/	"ioctl error reinitializing kernel errlog pointers",
/*25*/	"ioctl error clearing errlog pid",
/*26*/	"shutdown",
/*27*/	"unix domain bind error on path /dev/elcscntlsckt",
/*28*/	"unix domain socket error",
/*29*/	"unknown service elcsd/udp, check /etc/services file for entry",
/*30*/	"unknown host check /etc/hosts file for entry:",
/*31*/	"datagram socket bind error",
/*32*/	"datagram socket error",
/*33*/	"stream socket listen error",
/*34*/	"stream socket bind error",
/*35*/  "stream socket error",
/*36*/	"exiting, can't open/setup main or backup errlog file",
/*37*/	"can't open/setup main errlog file, opened backup errlog file!",
/*38*/	"open/setup single user mode errlog file",
/*39*/	"error coping sinlge user errlog file to main errlog file",
/*40*/	"error coping backup errlog file to main errlog file",
/*41*/	"can't log remote system errors!",
/*42*/	"bytes remaining in errlog file, until limit size is exceeded",
/*43*/	"exceeded errlog file limit size, error not logged to disk!",
/*44*/	"logging remotely to",
/*45*/	"stream socket connect error",
/*46*/	"can't log remotely!",
/*47*/	"open failure on file",
/*48*/	"read error",
/*49*/	"write error",
/*50*/	"error can't open/setup main errlog file",
/*51*/	"error can't open/setup backup errlog file",
/*52*/	"exiting, error can't open/setup single user errlog file",
/*53*/	"error writing to errlog file",
/*54*/	"error can't open/setup errlog file for host:",
/*55*/	"logging locally to",
/*56*/	"exiting, can't open elcsd.conf file",
/*57*/	"exiting, elcsd.conf needed info. or commented out",
/*58*/  "reinitialized kernel errlog buffer pointers",
/*59*/	"reconfigured per elcsd.conf",
/*60*/	"host address not in /etc/hosts", 
/*61*/	"elcsd.conf not set up right, missing or removed delimiter or not 7 entries between delimiters!", 
/*62*/	"error on saved kernel errlog buffer file from dump:", 
/*63*/	"remote host startup", 
/*64*/  "single user file corrupted",
/*65*/  "system name not defined in remote host table",
/*66*/  "exiting, can't move kernel errlog output pointer",
/*67*/  "file system almost full, CAN'T log errors to",
/*68*/  "reset errlog pid in kernel",
/*69*/  "exiting, can't malloc space for file system data",
/*70*/	"can't get system date",
	0
};
#endif /* __elcsd__ */
