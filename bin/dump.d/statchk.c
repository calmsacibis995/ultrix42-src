
# ifndef lint
static char *sccsid = "@(#)statchk.c	4.1      (ULTRIX)        7/2/90";
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
 * Modification History: /usr/src/etc/dump/statchk.c
 *
 * 29 Jun 88 -- Sam
 *	Bugfix - modified code to properly open disk files, and cleaned up 
 *	for general readability.
 *
 *  8 Sep 87 -- fries
 *      Added oflag check to maintain non V2.0 or later "rmt" compatability.
 *
 * 16 Oct 86 -- fries
 *      Removed the MTCACHE ioctl to enable device caching. It is now
 *	set to caching by default in the device driver.
 *
 *  9 Sep 86 -- fries
 *      Modified code to allow device to be opened just once for both
 *	status and usage.
 *
 *  5 May 86 -- fries
 *      Modified code to continue on generic ioctl failures.
 *
 * 13 Feb 86 -- fries
 *      Added code to perform local/ remote device generic status	
 *	checks.
 *
 * ------------------------------------------------------------------------*/

#include "dump.h"

#ifndef	REMOTE

/* Routine to obtain generic device status */
statchk(tape,mode)
	char	*tape;
	int	mode;
{
	struct stat stat_buf;
	struct mtop tape_op;
	int error = 0;
	char tname[30];
	char *cp;

	if((stat(tape,&stat_buf) >= 0)
	  && (((stat_buf.st_mode & S_IFMT) == S_IFBLK)
	    ||((stat_buf.st_mode & S_IFMT) == S_IFCHR))){

	    /* If a Block Special, then modify name to */
	    /* a Character Special file		   */
	    if ((stat_buf.st_mode & S_IFMT) == S_IFBLK){
	       cp = (char *)tape;
	       strncpy(tname,tape,5);
	       tname[5] = 'r';
	       tname[6] = '\0';
	       strcat(tname,cp+5);
	    }
	    else strcpy(tname,tape);	

            do{	
		/* Open device */
		if(device_open == 0){

	   	    /* open output device */
	   	    to = open(tname,mode|O_NDELAY);

		   /* Assume any error is no such device or address */
		   if (to < 0){
		       return(-1);
		   }
		   else{
		       device_open++;
		   }
		}

		error = 0;

		/* Get generic device status */
		if (ioctl(to,DEVIOCGET,(char *)&mt_info) < 0){
		   return(-2);
		}
	
		/* Check for device on line */
		if(mt_info.stat & DEV_OFFLINE){
	 	    fprintf(stderr,"\n");
	 	    msg("\7Place %s device unit #%u ONLINE\n\n",mt_info.device,mt_info.unit_num);
		    error++;
		}

		/* Check for device write locked when in write mode */
	 	if((mt_info.stat & DEV_WRTLCK) && (mode != O_RDONLY)){
	  	    fprintf(stderr,"\n");
          	    msg("\7WRITE ENABLE %s device unit #%u\n",mt_info.device,mt_info.unit_num);
	  	    error++;
	 	}

		if(error){
		    if (mt_info.category == DEV_TAPE){
			if (!query("Cannot open tape. Do you want to retry the open?"))
	         	    return(-3);
	            }
		    else{
              		if (!query("Cannot open disk. Do you want to retry the open?"))
  			    return(-3);
		    }
		    device_open = 0;
		    close(to);
		}
	    } while (error);

	    /* If all is ok, then return file descriptor */
	    return(to);
	}

	/* Open the device without going through status checks */
 	do{
	    if (open(tape,mode) < 0){
		if (!devblocks){
	 	    if (!query("Cannot open tape. Do you want to retry the open?"))
			return(-3);
		    }
	        else{
		    if (!query("Cannot open disk. Do you want to retry the open?"))
			return(-3);
		    }
	    	}
	    else device_open++;
	}while (device_open == 0);
	return(0);
}

#else

/* Perform a remote device status check */
rmtstatchk(tape,mode)
	char	*tape;
	int	mode;
{
	int error = 0;
	char tname[30];
	char *cp;

	/* If in compatability mode, or if the status check fails, */
	/* use the "vanilla" open routine */
	if ((oflag) || ((statbfp = rmtstat(tape)) == NULL))
		goto vanilla_rmt_open;

	/* If it's a block- or character- special device then it's a *
	/* candidate for N-buffered I/O */
	if (((statbfp->st_mode & S_IFMT) == S_IFBLK)
	    || ((statbfp->st_mode & S_IFMT) == S_IFCHR)){

		/* If a Block Special, then modify name to */
		/* a Character Special file		   */
		if ((statbfp->st_mode & S_IFMT) == S_IFBLK){
	   		cp = (char *)tape;
           		strncpy(tname,tape,5);
           		tname[5] = 'r';
	   		tname[6] = '\0';
	   		strcat(tname,cp+5);
		}
		else strcpy(tname,tape);	

		do{
			if(device_open == 0){

			    /* Force Remote device open to obtain status */
			    if (rmtopen(tname,mode) < 0)
	      			return(-1);
	 			device_open++;
			    }
	
			error = 0;

			/* Get generic device status */
			if ((devgetp = rmtgenioctl()) == NULL)
	  	    	    return(-2);
	
			/* Check for device on line */
			if(devgetp->stat & DEV_OFFLINE){
	  	    	    fprintf(stderr,"\n");
	  	            msg("\7Place Remote  %s device unit #%u ONLINE\n",devgetp->device,devgetp->unit_num);
	  	            error++;
        		}

			/* Check for device write locked when in write mode */
	 		if((devgetp->stat & DEV_WRTLCK) && (mode != O_RDONLY)){
	   	            fprintf(stderr,"\n");
           	    	    msg("\7WRITE ENABLE Remote %s device unit #%u\n",devgetp->device,devgetp->unit_num);
	   	    	    error++;
	 		}

			if(error){
		    	    if (devgetp->category == DEV_TAPE){
	      			if (!query("Cannot open tape. Do you want to retry the open?"))
	         	        return(-3);
	      		    }
		    	    else{
             		        if (!query("Cannot open disk. Do you want to retry the open?"))
  		 	        return(-3);
			    }
			    device_open = 0;
		    	    rmtclose();
			}
		} while (error);

		return(0);
	}

vanilla_rmt_open:

	/* Open the device without going through status checks */
 	do{
	    if (rmtopen(tape,mode) < 0){
		if (!devblocks){
	 	    if (!query("Cannot open tape. Do you want to retry the open?"))
			return(-3);
		    }
	        else{
		    if (!query("Cannot open disk. Do you want to retry the open?"))
			return(-3);
		    }
	    	}
	    else device_open++;
	}while (device_open == 0);
	return(0);
}
#endif
