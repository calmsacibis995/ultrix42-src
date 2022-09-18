/*
 *	@(#)cisysap.h	4.1	(ULTRIX)	7/2/90
*/

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 - 1989 by                    *
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
 *		Computer Interconnect Port Driver
 *
 *   Abstract:	This module contains Computer Interconnect Port Driver( CI )
 *		constants and data structure definitions visible to SYSAPs.
 *
 *   Creator:	Todd M. Katz	Creation Date:	April 22, 1985
 *
 *   Modification History:
 *
 *   19-Sep-1989	Pete Keilty
 *	Added CI port info. port_fcn_ext2.
 *
 *   18-Jan-1989	Todd M. Katz		TMK0003
 *	Add padding when it is necessary to keep longword alignment.  While
 *	some space is wasted such alignment is essential for ports of SCA to
 *	hardware platforms which require field alignment and access type to
 *	match( ie- only longword aligned entities may be longword accessed ).
 *
 *   03-May-1988	Todd M. Katz		TMK0002
 *	Rename ram_level -> fn_level within structure ucode_rev of the CIPIB.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased generality and
 *	robustness, made CI PPD and GVP completely independent from underlying
 *	port drivers, and added SMP support.
 */

/* CI Constants.
 */
#define	CI_NLOG			16	/* Number of CI port logout entries  */

/* CI Data Structure Definitions.
 */
typedef	struct _cilpib	{		/* CI Local Port Information	     */
    u_char	rpslogmap[ CIPPD_MAPSIZE ];/* Remote port state port log map */
} CILPIB;

typedef struct _cipib	{		/* CI Path Information		     */
    struct	{			/* Remote port microcode level	     */
	u_long	rom_level	:  8;	/*  PROM/Self-test ucode rev level   */
	u_long	fn_level	:  8;	/*  Functional ucode revision level  */
	u_long			: 16;	
    } ucode_rev;
    u_long	  port_fcn;		/* Remote port functionality mask    */
    u_long	  port_fcn_ext;		/* Rem port functionality extension  */
    u_long	  port_fcn_ext2;	/* Rem port functionality extension 2*/
    u_char	  rport_state;		/* Remote port state		     */
#define	PS_UNINIT		0	/*  Uninitialized		     */
#define	PS_UNINIT_MAINT		1	/*  Maintenance/Uninitialized	     */
#define	PS_DISAB		2	/*  Disabled			     */
#define	PS_DISAB_MAINT		3	/*  Maintenance/Disabled	     */
#define	PS_ENAB			4	/*  Enabled			     */
#define	PS_ENAB_MAINT		5	/*  Maintenance/Enabled		     */
    u_char	  reset_port;		/* Remote port's resetting port      */
    u_short			: 16;
} CIPIB;
