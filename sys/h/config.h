/* @(#)config.h	4.1  (ULTRIX)        7/2/90     */

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
/*
 * Modification history:
 *
 * 12-20-87 -- larry
 *	Add two fields to the end of config_adpt structure.
 *	The config program will init them to 0.
 *	The fields are the bus number and nexus number respectively.
 *	The fields are used by the SCS code to determine if a CI adapter
 *	is "alive".
 *
 * 7-jul-86   -- jaw 	added adapter alive bit for Mr. Installation.
 *
 * 5-Jun-86   -- jaw 	changes to config.
 *
 */

struct config_adpt {
	char 	*p_name;
	int	p_num;
	char	*c_name;
	int	c_num;
	char	c_type;		
	/* if c_type = 'D' then c_ptr is pointer to device struct */
	/* if c_type = 'C' then c_ptr is pointer to controller struct */
	/* if c_type = 'A' then c_ptr is set if ALIVE */
	char	*c_ptr;	   
	short	c_bus_num;
	short	c_nexus_num;
};

#define CONFIG_ALIVE 	1  /* c_ptr is set if adapter is alive */
