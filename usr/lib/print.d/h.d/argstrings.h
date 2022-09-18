#ifndef lint
/* @(#)argstrings.h	4.1      ULTRIX 7/2/90 */
#endif

/***************************************************************/
/* argstrings header file                                      */
/***************************************************************/
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  16/05/88 -- root
 * date and time created 88/05/16 17:27:13 by root
 * 
 * ***************************************************************
 * 
 * 1.2  17/05/88 -- thoms
 * Uniqueified #defines,  added bogus extra one to denote numeric arg.
 * 
 * 
 * ***************************************************************
 * 
 * 1.3  18/05/88 -- maxwell
 * removed references to valid_range
 * 
 * 
 * ***************************************************************
 * 
 * 1.4  20/05/88 -- thoms
 * Put back MAXARG definition
 * 
 * 
 * ***************************************************************
 *
 * 1.5 10/07/88 -- thoms
 * Added copyright notice and modification history
 *
 * SCCS history end
 */

extern char *index();

#define as_data_types	0
#define as_input_trays	1
#define as_output_trays	2
#define as_orientations	3
#define as_page_sizes	4
#define as_messages	5
#define as_sides	6

#define num_opts	7

/* The next two are so that the checking functions can
 * check all different parameters including strings
 * and numeric (digit string) parameters
 */

#define as_string	998
#define as_numerical	999

#define MAXARG	10000		/* maximum numerical PostScript arg */



struct arg_pair {
	char   *arg;                    /* valid arg str */
	char   *cfentry;                /* entry in control file */
	int    minlen;                  /* len of min unique match */
};

extern void		init_args();
extern void		get_args();
