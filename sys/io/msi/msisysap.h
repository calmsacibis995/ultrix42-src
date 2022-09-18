/*
 * @(#)msisysap.h	2.1	(ULTRIX)	5/9/89
 */

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************
 *
 *
 *   Facility:	Systems Communication Architecture
 *		Mayfair Storage Interconnect Port Driver
 *
 *   Abstract:	This module contains Mayfair Storage Interconnect Port
 *		Driver( MSI ) constants and data structure definitions
 *		visible to SYSAPs.
 *
 *   Creator:	Todd M. Katz	Creation Date:	December 06, 1988
 *
 *   Modification History:
 */

/* MSI Constants.
 */
#define	MSI_MAXNUM_PORT		 8	/*  DSSI( ports )		     */
#define	MSI_MAPSIZE ( MSI_MAXNUM_PORT / 8 ) /* Size of port bit map in bytes */

/* MSI Data Structure Definitions.
 */
typedef	struct _msilpib	{		/* MSI Local Port Information	     */
    u_long	dg_size;		/* Size of application datagram	     */
    u_long	msg_size;		/* Size of application sequenced msg */
    u_long	pd_ovhd;		/* Size of PD + PPD header overhead  */
    u_long	ppd_ovhd;		/* Size of PPD header overhead	     */
    u_char	rpslogmap[ MSI_MAPSIZE ];/* Remote port state port logmap    */
    u_long			: 24;
} MSILPIB;

typedef	struct	_msirpi {		/* MSI Remote Port Information	     */
    struct	{			/* Remote port microcode level	     */
	u_char	st_level;		/*  Self-test ucode rev level	     */
	u_char	fn_level;		/*  Functional ucode revision level  */
	u_short			: 16;	
    } ucode_rev;
    u_short	port_fcn[ 2 ];		/* Port functionality mask	     */
    struct	{			/* System state information	     */
	u_short	reset_port	:  8;	/*  Port which caused last reset     */
	u_short	port_state	:  3;	/*  Port state( defined by CIPIB )   */
			       /* PS_UNINIT	  - Uninitialized	     */
			       /* PS_UNINIT_MAINT - Maintenance/Uninitialized*/
			       /* PS_DISAB	  - Disabled		     */
			       /* PS_DISAB_MAINT  - Maintenance/Disabled     */
			       /* PS_ENAB	  - Enabled		     */
			       /* PS_ENAB_MAINT	  - Maintenance/Enabled	     */
	u_short	sys_state1	:  5;	/*  Implementation specific state    */
	u_short	sys_state2;
    } sys_state;
    struct	{			/* Port functionality extension	     */
	u_short			: 16;
	u_short	maxbodylen	: 13;	/*  Max num of bytes in packet body  */
	u_short			:  3;
    } port_fcn_ext;
} MSIRPI;

typedef struct _msipib	{		/* MSI Path Information		     */
    struct _msirpi	rpinfo;		/* Remote port information	     */
} MSIPIB;
