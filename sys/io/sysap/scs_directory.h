/*
 *	@(#)scs_directory.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 by                           *
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
 *		Systems Communication Services Directory SYSAP
 *
 *   Abstract:	This module contains the constants, macros, data structure
 *		definitions, and sequenced message definitions used
 *		exclusively by the Systems Communication Services Directory
 *		SYSAP( SCS$DIRECTORY ).
 *
 *   Creator:	Todd M. Katz	Creation Date:	December 1, 1987
 *
 *   Modification History:
 *
 *   18-Sep-1989	Pete Keilty
 *	Changed TMK special to DIRECTORY$SCS.
 *
 *   08-Jan-1988	Todd M. Katz
 *	Added comments.
 *
 *   08-Dec-1987	Todd M. Katz
 *	Formated module and revised comments.
 */

/* SCS$DIRECTORY Constants.
 */
#define	DIRNAME		"SCS$DIRECTORY   "
#define	DIRDATA		"DIRECTORY$SCS   "
#define	DIRPANIC_ACCEPT	"scs$directory - accept failed\n"
#define	DIRPANIC_DISCON	"scs$directory - disconnect failed\n"
#define	DIRPANIC_EVENT	"scs$directory - unknown event\n"
#define	DIRPANIC_REJECT	"scs$directory - reject failed\n"
#define	DIRPANIC_RSP	"scs$directory - response transmission failure\n"

/* SCS$DIRECTORY Sequenced Message Definitions.
 */
typedef struct	{			/* SCS Directory Request	     */
    u_short	form;			/* Form of request		     */
#define	BY_NAME			 0	/*  Directory lookup is by name	     */
#define	BY_ENTRY		 1	/*  Directory lookup is by number    */
    u_short	entry;			/* Entry number( form == BY_ENTRY )  */
    u_char	proc_name[ NAME_SIZE ];	/* Name of SYSAP( form == BY_NAME )  */
					/*  ( Blank filled )		     */
} SCS_DIR_REQ;

typedef struct	{			/* SCS Directory Response	     */
    u_short	status;			/* Status of request		     */
    u_short	entry;			/* Entry number			     */
    u_char	proc_name[ NAME_SIZE ];	/* SYSAP name ( blank filled )	     */
    u_char	proc_info[ DATA_SIZE ];	/* SYSAP information		     */
} SCS_DIR_RSP;

/* SCS$DIRECTORY Macros.
 */
#define	Rsp		(( SCS_DIR_RSP * )req )
