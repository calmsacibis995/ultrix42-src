#ifndef lint
static	char	*sccsid = "@(#)catio.c	4.1	(ULTRIX)	12/6/90";
#endif lint

/************************************************************************
 *									*
 *         Copyright (c) Digital Equipment Corporation, 1990		*
 *									*
 *   All Rights Reserved.  Unpublished rights  reserved  under  the	*
 *   copyright laws of the United States.				*
 *									*
 *   The software contained on this media  is  proprietary  to  and	*
 *   embodies  the  confidential  technology  of  Digital Equipment	*
 *   Corporation.  Possession, use, duplication or dissemination of	*
 *   the  software and media is authorized only pursuant to a valid	*
 *   written license from Digital Equipment Corporation.		*
 *									*
 *   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  by	*
 *   the U.S. Government is subject to restrictions as set forth in	*
 *   Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  or  in  FAR	*
 *   52.227-19, as applicable.						*
 *									*
 ************************************************************************/
/**/

/*
 * @OSF_COPYRIGHT@
 *
 *
 * HISTORY
 * $Log:	catio.c,v $
 * Revision 1.4  90/10/07  16:45:06  devrcs
 * 	Added EndLog Marker.
 * 	[90/09/28  14:52:56  gm]
 * 
 * Revision 1.3  90/09/23  16:24:32  devrcs
 * 	Switched to non-ansi prototypes.
 * 	[90/09/20  15:46:25  gm]
 * 
 * Revision 1.2  90/05/13  20:39:52  devrcs
 * 	new for OSF/1
 * 	[90/05/03  13:02:49  mbrown]
 * 
 * $EndLog$
 *
 * static char sccsid[] = "@(#)catio.c	1.5  com/cmd/msg,3.1,9013 10/24/89 11:01:19";
 *
 *
 * COMPONENT_NAME:  CMDMSG
 *
 * FUNCTIONS: descopen, descgets, descclose, descset, descerrck
 *
 * ORIGINS: 27
 *
 * IBM CONFIDENTIAL -- (IBM Confidential Restricted when
 * combined with the aggregated modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 1988, 1989
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

#include <stdio.h>
FILE *descfil;

/*
 * EXTERNAL PROCEDURES CALLED: None
 */


/*
 * NAME: descopen
 *
 * FUNCTION: Perform message utility input operations
 *
 * EXECUTION ENVIRONMENT:
 *	User Mode.
 *
 * NOTES: The routines in this file are used to provide a mechanism by which
 *     	  the routines in gencat can get the next input character without
 *  	  worrying about details such as if the input is coming from a file
 *  	  or standard input, etc.
 *
 * RETURNS : 0 - successful open
 *          -1 - open failure
 */


/*   Open a catalog descriptor file and save file descriptor          */ 

descopen(filnam) 
char *filnam;
			/*
			  filnam - pointer to message catalog name
			*/
{
	if ((descfil = fopen(filnam,"r")) == 0) 
		return (-1);
	
return (0);
}



/*
 * NAME: descgets
 *
 * FUNCTION: Read message catalog
 *
 * EXECUTION ENVIRONMENT:
 *	User mode.
 *
 * NOTES: Read the next line from the opened message catalog descriptor file.
 *
 * RETURNS: Pointer to message buffer -- scccessful
 *          NULL pointer -- error or end-of-file
 */


char *descgets(buff, bufsize)
char *buff; int bufsize;
			/*
			  buff - pointer to message buffer
			  bufsize - size of message buffer in bytes
			*/
{
	char *str;

	str = fgets(buff, bufsize, descfil);
	buff[bufsize-1] = '\0';        /* terminate in case length exceeded */
	return (str);
}



/*
 * NAME: descclose
 *
 * FUNCTION: Close message catalog
 *
 * EXECUTION ENVIRONMENT:
 *	User mode.
 *
 * NOTES: Close the message catalog descriptor file.
 *
 * RETURNS: None
 */


descclose()
{
	fclose(descfil);
	descfil = 0;
}



/*
 * NAME: descset
 *
 * FUNCTION: Establish message catalog file descriptor
 *
 * EXECUTION ENVIRONMENT:
 *	User mode.
 *
 * NOTES: Set the file descriptor to be used for message catalog access.
 *        
 *
 * RETURNS: None
 */


descset(infil)
FILE *infil;
{
	descfil = infil;
}


/*
 * NAME: descerrck
 *
 * FUNCTION: Check I/O stream status
 *
 * EXECUTION ENVIRONMENT:
 *	User mode.
 *
 * RETURNS: 0 - no error encountered
 *         -1 - error encountered
 */

descerrck()
{
	if (ferror(descfil))
		return(-1);
	else    return(0);
}

