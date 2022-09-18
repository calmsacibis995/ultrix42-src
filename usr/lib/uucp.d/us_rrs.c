#ifndef lint
static char sccsid[] = "@(#)us_rrs.c	4.1 (decvax!larry) 7/2/90";
#endif

 
/*
 * We get the job number from a command file "cfile".
 * using the jobn as the key to search thru "R_stat"
 * file and modify the corresponding status as indicated
 * in "stat".	"Stat" is defined in "uust.h".
 * return:
 *	0	-> success
 *	FAIL	-> failure
 */

/****************
 * Mods:
 * 	decvax!larry - store data in binary format
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
#ifdef UUSTAT
#include <sys/types.h>
#include "uust.h"



long	ftell();
us_rrs(cfilel,stat)
char *cfilel;
short stat;
{
	FILE	*fp;
	register short i;
	struct us_rsf u;
	char cfile[20],  *lxp, *name, buf[BUFSIZ];
	char *strcpy();
	long	pos;
	long time();
	short n;
 
	/*
	 * strip path info
	 */
	strcpy(cfile, lastpart(cfilel));
	DEBUG(9, "\nenter us_rrs, cfile: %s", cfile);
	DEBUG(9, "  request status: %o\n", stat);
	
	/*
	 * extract the last 4 digits
	 * convert to digits
	 */
	name = cfile + strlen(cfile) - 4;  
	for(i=0; i<=15; i++) {
		if (ulockf(LCKRSTAT, 15) != FAIL) 
			break;
		sleep(1);
	}
	if (i > 15) {
		DEBUG(3, "ulockf of %s failed\n", LCKRSTAT);
		return(FAIL);
	}
	if ((fp = fopen(R_stat, "r+")) == NULL) {
		DEBUG(3, "fopen of %s failed\n", R_stat);
		rmlock(LCKRSTAT);
		return(FAIL);
	}
	while(fread(&u, sizeof(u), 1, fp) != NULL){
		if (strncmp(name, u.jobn,4) == SAME) {
			u.jobn[4] = '\0';
			DEBUG(6, " jobn : %s\n", u.jobn);

			pos = ftell(fp);
			u.ustat = stat;
			u.stime = time((long *)0);
			fseek(fp, pos-(long)sizeof(u), 0);
			fwrite(&u, sizeof(u), 1, fp);
			break;
		}

	}
	fflush(fp);
	fclose(fp);
	rmlock(LCKRSTAT);
	return(FAIL);
}
#endif
