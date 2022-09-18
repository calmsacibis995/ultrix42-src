#ifndef lint
static char sccsid[] = "@(#)uucpdefs.c	4.1 ULTRIX 7/2/90";
#endif

/*****************
 *  definitions of global variables
 ****************/



/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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




#include "uucp.h"

char Progname[10];
int Ifn, Ofn;
char Rmtname[16];
char User[16];
char Loginuser[16];
char Myname[16];
int Bspeed;
char Wrkdir[WKDSIZE];

char *Thisdir = THISDIR;
char Spoolname[MAXFULLNAME];
char *Spool = SPOOL;
#ifdef	UUDIR
char DLocal[16];
char DLocalX[16];
char *Dirlist[MAXDIRS];
int Subdirs;
#endif
int Debug = 0;
int Pkdebug = 0;
int Packflg = 0;
int Pkdrvon = 0;
long Retrytime;
short Usrf = 0;			/* Uustat global flag */
char Seqlock[MAXFULLNAME];
char Seqfile[MAXFULLNAME];
int IsTcpIp = 0;        /* 1 == TCP/IP connection, else 0.  suppress ioctl */
