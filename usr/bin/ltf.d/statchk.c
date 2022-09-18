# ifndef lint
static char *sccsid = "@(#)statchk.c	4.1	ULTRIX	7/17/90";
# endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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

/* ------------------------------------------------------------------------
 * Modification History:
 *
 *	26-Mar-86	statchk written by Jeff Fries, modified for 
 *			ltf by Suzanne Logcher
 *
 * ------------------------------------------------------------------------
 */
#ifndef U11
#include "ltfdefs.h"
/* Routine to obtain generic device status */
FILE *statchk(tape, ch_mode)
	char	*tape;
	char	*ch_mode;
{
	int	to;
	FILE	*file_desc;
	int	mode;
	struct devget mt_info;

	if (Tape) {

	    /* Get mode in integer form */
	    if (*ch_mode == 'w')
		mode = O_WRONLY;
	    else if (*ch_mode == 'r')
		mode = O_RDONLY;

	    /* Force device open to obtain status */
	    to = open(tape,mode|O_NDELAY);

	    /* If open error, then error must be no such device and address */
	    if (to < 0)
		return(FALSE);
	    /* Get generic device status */

	    if (ioctl(to,DEVIOCGET,(char *)&mt_info) < 0)
	            PERROR "\n%s: %s %s\n", Progname, CANTDEVIO, tape);
	    else {
	
		/* Check for device on line */
		if(mt_info.stat & DEV_OFFLINE){
	            PERROR "\n%s: %s %s%s %s %s%u %s\n", Progname, ERRDEV, tape, OFFL1, mt_info.device, ERRUNIT, mt_info.unit_num, OFFL2);
	            exit(FAIL);
		}

		/* Check for device write locked when in write mode */
		else
		    if((mt_info.stat & DEV_WRTLCK) && (mode != O_RDONLY)){
			PERROR "\n%s: %s %s%s %s %s%u\n", Progname, ERRDEV, tape, WRTLCK, mt_info.device, ERRUNIT, mt_info.unit_num);
			exit(FAIL);
		    }
	    }/*F if (ioctl(to,DEVIOCGET ... */
	    if (close(to) < 0) {
	        PERROR "\n%s: %s %s\n\n", Progname, CANTCLS, tape);
	        perror(tape);
	        exit(FAIL);
	    }
	}/*T if (Tape) */   

	/* Re-Open as user requested */
	file_desc = fopen(tape, ch_mode);
	if (file_desc < 0) 
	    return(FALSE);
	return(file_desc);
}
#endif
/**\\**\\**\\**\\**\\**  EOM  statchk.c  **\\**\\**\\**\\**\\*/
/**\\**\\**\\**\\**\\**  EOM  statchk.c  **\\**\\**\\**\\**\\*/
