#ifndef lint
static char sccsid[] = "@(#)us_sst.c	4.1 (decvax!larry) 7/2/90";
#endif

 
/*
 * searches thru L_stat file using "rmtname"
 * as the key.
 * If the entry is found, then modify the system
 * status as indicated in "flag" and return.
 * return:
 *	0	-> success
 *	FAIL	-> failure
 */

/*******************
 * Mods:
 *	decvax!larry - store data in binary format
 *******************/




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
#ifdef UUSTAT
#include <sys/types.h>
#include "uust.h"



us_sst(flag)
short flag;
{
	register FILE *fp;
	register short i;
	int found = 0;
	struct us_ssf s;
	long	ftell();
	long	pos;
	char buf[BUFSIZ];
	long time();
 
	DEBUG(9, " enter us_sst, status is : %.2d\n", flag);
	if (flag == us_s_dev)
		return(0);  /* don't log instances of NO DEVICE */
	for(i=0; i<=15; i++) {
		if (ulockf(LCKLSTAT, 15) != FAIL)
			break;
		sleep(1);
	}
	if (i > 15) {
		DEBUG(3, "ulockf of %s failed\n", LCKLSTAT);
		return(FAIL);
	}
	if ((fp = fopen(L_stat, "r+")) == NULL) {
		DEBUG(3, "fopen of %s failed\n", LCKLSTAT);
		rmlock(LCKLSTAT);
		return(FAIL);
	}
 
	while(fread(&s, sizeof(s), 1, fp) != NULL){
		DEBUG(9, "s.sysname : %6.6s\n", s.sysname);
		if (strncmp(s.sysname, Rmtname, 7) == SAME) {
			pos = ftell(fp);
			fseek(fp,pos - sizeof(s), 0);
			found++;
			break;
		}

	}
 
	strncpy(s.sysname, Rmtname, 7);
	s.sysname[7] = '\0';
out:
	if (!found) 
		s.sucti = s.sti = 0;
	/* log last time a conversation completed or at least started */
	if(flag == us_s_ok || flag == us_s_gress)
		s.sucti = time((long *)0);
	else 
		s.sti = time((long *)0);
	s.sstat = flag;
	fwrite(&s, sizeof(s), 1, fp);
	fflush(fp);
	fclose(fp);
	rmlock(LCKLSTAT);
	return(0);
}
#endif
