/*		@(#)keydefs.h	4.1				7/2/90	*/	
/*
 * Program keydefs.h ,  Module 
 *
 *									
 *			Copyright (c) 1985 by			
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.		
 *						
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Digital Equipment
 * Corporation.
 *
 * Digital assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by Digital.
 *
 *
 * Program keydefs.h ,  Module 
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 * 1.00  10-Jul-1985
 *     DECnet Ultrix V1.0
 *
 *
 */

/*********************************************************************************
**
**	keydefs.h
**
**	insert file for keyword table constants.
**
**
**	This file contains constants, definitions, and routines 
**      for UPARS keywords processing.	It is included by both the UPARS 
**      compiler, and by the run-time lexers. 
**
*******************************************************************************/



/*******************************************************************************
** 
**	Build-in keyword definitions.  These are assigned numbers below
**	the keywords defined by the user.  User's keywords, defined using
**	the keyword statement, are given numbers higher than these.
**	Note that the keywords start at 1:  this is an assembler-style
**	trick, such that search_keyword returns 0 at no match.
**	This hack is a tribute to, and very much indicative of, the 
**	traditional guts-ball TPARS cultural style. 
*/

#define	u_$NULL		1		/* null, matches anything */
#define	u_$ERROR	u_$NULL+1	/* error, return code     */


/*
**	These are the built-ins for byte parsing
*/

#define	u_$EOM		u_$ERROR+1	/* end of message */
#define	u_$IMAGE	u_$EOM+1	/* image fields   */
#define	u_$PARAM	u_$IMAGE+1	/* parameter      */
#define	u_$BYTE		u_$PARAM+1	/* byte           */
#define	u_$SKIP		u_$BYTE+1	/* skip           */
#define	u_$MATCH	u_$SKIP+1	/* match 	  */


/*
**	text parsing built-ins . . .
*/	

#define	u_$EOS		u_$ERROR+1	/* end of string   */
#define	u_$DIGIT	u_$EOS+1	/* single digit    */
#define u_$CHAR		u_$DIGIT+1	/* single alpha    */
#define u_$BLANK	u_$CHAR+1	/* blank           */
#define u_$ANY		u_$BLANK+1	/* any character   */
#define u_$STRING	u_$ANY+1	/* string	   */
#define u_$DECIMAL	u_$STRING+1	/* decimal number  */
#define u_$HEX		u_$DECIMAL+1	/* hex number      */
#define u_$OCTAL	u_$HEX+1	/* octal number    */
#define u_$LABEL	u_$OCTAL+1	/* label           */	


/*
**	These offsets define where the user's keywords start in the
**	actual run-time keyword table . . .
*/

#define	u_BYTE_OFFSET	u_$MATCH+1
#define	u_TEXT_OFFSET	u_$LABEL+1


