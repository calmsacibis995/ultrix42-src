#ifdef	line
static char *sccsid = "@(#)tc_option_data.c	4.6	(ULTRIX)	1/31/91";
#endif	lint

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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

/*
 * Modification History: tc_option_data.c
 *
 * 23-Jan-91	Brian Nadeau
 *	Add place holders for Multimedia
 *
 * 06-Dec-90	Mark Parenti
 *	Add VMEbus adapter
 *
 *  4-Jul-90	Randall Brown
 *	Added new entry for PMAC-AA, 4 line async option card.
 *
 * 26-Jan-90	Randall Brown
 *	Changed file to reference tc structures. Moved from maxoption_data.c
 *
 * 29-Dec-89	afd
 *	Added 2 new fields for when to enable the interrupt line for devices.
 *	Added "type" and "adapter config routine" fields to the table.
 *
 * 26-Oct-89	afd
 *	Put in the correct module names for the options that we know about.
 *
 * 12-Sep-89	afd
 *	Created this file to contain a data table that maps the 8 byte
 *	IO module name in the option ROM to the device or controler name
 *	in the config file.
 *
 */

#include "../io/tc/tc.h"

extern	int	xviaconf();

struct tc_option tc_option [] =
{
    /*	module		driver	intr_b4	itr_aft		adpt	*/
    /*	name		name	probe	attach	type	config	*/
    /*	------		------	-------	-------	----	------	*/

    {   "PMAF-AA ",     "fza",    0,      1,    'D',    0},     /* FDDI */
    {	"PMAC-AA ",	"dc",	  0,	  1,	'D',	0},	/* SLU */
    {	"PMAD-AA ",	"ln",	  0,	  1,	'D',	0},	/* Lance */
    {	"PMAZ-AA ",	"asc",	  0,	  1,	'C',	0},	/* SCSI */
    {	"PMAG-BA ",	"fb",	  0,	  0,	'D',	0},	/* CFB */
    {	"PMAG-AA ",	"fb",	  0,	  0,	'D',	0},	/* MFB */
    {	"PMAG-CA ",	"ga",	  0,	  1,	'D',	0},	/* 2DA */
    {	"PMAG-DA ",	"gq",	  0,	  1,	'D',	0},	/* LM-3DA */
    {	"PMAG-FA ",	"gq",	  0,	  1,	'D',	0},	/* HE-3DA */
    {	"PMABV-AA",	"vba",	  1,	  1,	'A',	xviaconf}, /* VME */
    {   "PMMM-AA ",	"mmlp",	  0,	  0,	'D',	0},	/* MM0 */
    {   "PMMM-AB ",	"mmlp",	  0,	  0,	'D',	0},	/* MM1 */
    {   "PMMM-AC ",	"mmlp",	  0,	  0,	'D',	0},	/* MM2 */
    {   "PMMM-AD ",	"mmlp",	  0,	  0,	'D',	0},	/* MM3 */
    /*
     * Do not delete any table entries above this line or your system
     * will not configure properly.
     *
     * Add any new controllers or devices here.
     * Remember, the module name must be blank padded to 8 bytes.
     */


    /*
     * Do not delete this null entry, which terminates the table or your
     * system will not configure properly.
     */
    {	"",		""	}	/* Null terminator in the table */
};
