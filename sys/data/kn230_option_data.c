/*
 * @(#)kn230_option_data.c	4.3  (ULTRIX)        10/9/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 * Modification History: kn230_option_data.c
 *
 * 09-Oct-1990    Paul Grist
 *    merged 4.MM to 4.TI, added option-name string to interface.
 *
 * 14-Mar-1990    Paul Grist
 *    created this file to provide the necessary information
 *    needed to configure the option card on DECsystem 5100.
 *    This information is linked to the installed physical
 *    board by the id number stored in the OID (option ID register).
 *
 */

struct kn230_option {
  int option_id;           /* id# as read from OID register */
  char driver_name[32];    /* driver name as read in configuration file */
  char type;               /* 'D' or 'C',device or controller uba struct*/
  unsigned int exp0_csr;   /* add of expansion device 0's csr reg*/
  unsigned int exp1_csr;   /* add of expansion device 1's csr reg*/
  char option_name[64];    /* name string printed at boot time */  
};

/*
 * Instructions for adding support for a new option card
 *
 *
 * The kn230 cpu board provides support for ONE option card with
 * two interrupts available for the expansion option. This allows
 * the expansion option to have a maximum of two devices.
 *
 * An entry for each device must be made in the kn230_option table
 * below with a value for each of the fields. The first two entries
 * are for the kn230 async card and can be used as an example. The
 * fields of the table are:
 *
 *	option id number: The value which will be read out of the 
 *		          Option ID register for the option card
 *
 *	driver name:	  The device driver name as it appears in
 *			  the system configuration file
 *
 *	type:		  Does the device driver use a "device" or
 *			  "controller" uba structure, the only two
 *			  values allowed are 'D' or 'C'
 *
 *	expansion0 csr:	  The csr address of the device which
 *			  will interrupt on the first line
 *	
 *	expansion1 csr:   The csr address of the device which will
 *			  interrupt on the second line
 *
 *      option_name:      The option name string printed at boot time.
 *
 *  If there is only one device on the card it should use the first
 *  available interrupt, and place it's csr address in the expansion 0
 *  field. ONLY one expansion csr field should be used, the other should
 *  be zero.
 */

struct kn230_option kn230_option [] =
{

/* option driver       expansion 0 expansion 1         option name          */
/*  id#	   name	 type  csr address csr address	          string            */
/* ====== ====== ====  =========== ===========	 ========================   */
 
{  0x1,	 "mdc",  'D',  0x15000000, 0x0,	       "Async comm (8 ports)"  },
{  0x1,	 "mdc",  'D',  0x0,	   0x15200000, "Async comm (8 ports)"  },

/* add additional option card devices here */


/*
 * DO NOT DELETE the Null entry which marks the end of the table
 * or your system will not configure properly.
 */
  {	-1,	"",	'0',	0,		0,	"\n"	} 

};

