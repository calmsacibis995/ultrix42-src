/* sccsid  =  @(#)ltatty.h	4.1	ULTRIX	7/2/90 */
 
/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *			Modification History				*
 *									*
 *									*
 ************************************************************************/
 
/*
 * The following structure is used to return information specific
 * to LAT ttys.
 */
 
#define MAXLTADESTSIZE 16
#define MAXLTASERVSIZE 16
#define MAXLTAPORTSIZE 16
 
struct ltattyi {
	char lta_dest_port[MAXLTADESTSIZE+1];
	char lta_server_name[MAXLTASERVSIZE+1];
	char lta_server_port[MAXLTAPORTSIZE+1];
};
