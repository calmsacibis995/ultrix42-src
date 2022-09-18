/* "@(#)rusers.h	4.1 (ULTRIX)        7/2/90 */

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1986 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                            				*
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   This software is  derived  from  software  received  from  the     *
 *   University    of   California,   Berkeley,   and   from   Bell     *
 *   Laboratories.  Use, duplication, or disclosure is  subject  to     *
 *   restrictions  under  license  agreements  with  University  of     *
 *   California and with AT&T.                                          *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 * @(#) from SUN 1.6
 */

#define RUSERSPROC_NUM 1
#define RUSERSPROC_NAMES 2
#define RUSERSPROC_ALLNAMES 3
#define RUSERSPROG 100002
#define RUSERSVERS_ORIG 1
#define RUSERSVERS_IDLE 2
#define RUSERSVERS 2

#define MAXUSERS 100

struct utmparr {
	struct utmp **uta_arr;
	int uta_cnt
};

struct utmpidle {
	struct utmp ui_utmp;
	unsigned ui_idle
};

struct utmpidlearr {
	struct utmpidle **uia_arr;
	int uia_cnt
};

int xdr_utmparr();
int xdr_utmpidlearr();
