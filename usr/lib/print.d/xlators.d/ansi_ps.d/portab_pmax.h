/*        @(#)portab_pmax.h	4.1      7/2/90      */
/*
 *
 *	COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1987,
 *	1989.  ALL RIGHTS RESERVED.
 *
 *	THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE
 *	USED AND COPIED ONLY IN ACCORDANCE  WITH THE TERMS OF
 *	SUCH  LICENSE  AND  WITH  THE  INCLUSION OF THE ABOVE
 *	COPYRIGHT  NOTICE.  THIS SOFTWARE OR ANY OTHER COPIES
 *	THEREOF   MAY  NOT  BE  PROVIDED  OR  OTHERWISE  MADE
 *	AVAILABLE  TO  ANY  OTHER  PERSON.  NO  TITLE  TO AND
 *	OWNERSHIP  OF  THE  SOFTWARE  IS  HEREBY TRANSFERRED.
 *
 *	THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE
 *	WITHOUT  NOTICE  AND SHOULD  NOT BE  CONSTRUED  AS A
 *	COMMITMENT  BY  DIGITAL EQUIPMENT CORPORATION.
 *
 *	DIGITAL  ASSUMES  NO RESPONSIBILITY  FOR THE  USE  OR
 *	RELIABILITY  OF ITS SOFTWARE ON EQUIPMENT THAT IS NOT
 *	SUPPLIED BY DIGITAL.
 *
 */


/*
 *----------------------------------------------------------------------------
 *
 *  begin description
 *
 *	PORTAB_ULTRIX.H - INCLUDE file to provide portable data declarations
 *	for Ultrix implementations, implemented by setting the ULTRIX flag and
 *	then doing an INCLUDE on PORTAB_GEN.HC.
 *
 *  end description
 *
 *----------------------------------------------------------------------------
 */
 

/*
 *----------------------------------------------------------------------------
 *
 *  begin edit history
 *
 * Edit:	mhs	  30-OCT-1987 17:49:26 Created
 *
 *	    001	24-DEC-1987 10:27  mhw
 *		change VAX to CVAX to work with makehf utility
 *		
 *              27-FEB-1989 cp
 *              Adapted PORTAB_VAX.H to make this PORTAB_ULTRIX.H file.	
 *
 *  end edit history
 *
 *----------------------------------------------------------------------------
 */


#define ULTRIX	1			/* set the ULTRIX flag */
#define PMAX	1			/* set the PMAX flag */

#include "portab_gen.h"
