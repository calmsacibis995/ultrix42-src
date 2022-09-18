/*
 * @(#)dyntypes.h	4.1	(ULTRIX) 7/2/90
 */

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1987 - 1989 by                    *
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
 *   Facility:	Kernel Memory Allocator
 *
 *   Abstract:	This module contains the type definitions for all data
 *		structures dynamically allocated by the kernel memory
 *		allocator.  It is suggested that the first 3 longwords
 *		of every structure so allocated by formatted as follows:
 *
 *			+----------------------------------+
 *			|               FLINK              |
 *			+----------------------------------+
 *			|               BLINK              |
 *			+----------+------+----------------+
 *			| OPTIONAL | TYPE |      SIZE      |
 *			+----------+------+----------------+
 *
 *   Creator:	Todd M. Katz	Creation Date:	September 30, 1987
 *
 *   Modification History:
 *
 *   07-Jan-1989	Todd M. Katz		TMK0002
 *	Add DYN_SIIBUF( SII RAM transmit/receive buffer ) and DYN_MSICMD(
 *	MSI Port Command Buffer ).
 *
 *   12-Jul-1988	Todd M. Katz		TMK0001
 *	Add DYN_CIPPDSLIB( CI PPD common system level logging information
 *	block ).
 */

#define	DYN_SB		 1		/* SCA System Block	  	     */
#define	DYN_PB		 2		/* SCA Path Block	  	     */
#define	DYN_CB		 3		/* SCS Connection Block  	     */
#define	DYN_CBVTDB	 4		/* SCS CB Vector Table Database	     */
#define	DYN_PCCB	 5		/* SCA Port Command and Control Block*/
#define	DYN_GVPBDDB	 6		/* Gen Vaxport Buf Descriptor Db     */
#define	DYN_GVPDG	 7		/* Generic Vaxport Datagram Buffer   */
#define	DYN_GVPMSG	 8		/* Generic Vaxport Message Buffer    */
#define	DYN_CICMD	 9		/* CI Port Command Buffer	     */
#define	DYN_CIPPDSLIB	10		/* CI PPD Common Sys Lev Log Info Blk*/
#define	DYN_SIIBUF	11		/* SII RAM Transmit/Receive Buffer   */
#define	DYN_MSICMD	12		/* MSI Port Command Buffer	     */
