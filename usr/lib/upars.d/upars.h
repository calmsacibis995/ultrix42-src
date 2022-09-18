/*		@(#)upars.h	4.1				7/2/90	*/	
/*
 * Program upars.h,  Module 
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
 * Program upars.h,  Module 
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 *
 * 1.00  10-Jul-1985
 *     DECnet Ultrix V1.0
 */

/*******************************************************************************
**
**
**	upars.h
**
**	Global definitions for UPARS compiler and run-time system.
**
*******************************************************************************/







/*
**	Global types.
*/

#define	u_BOOL	int
#define VOID	(void)



#define	TRUE	1
#define	FALSE	0


/*
**
**	Argument numbers
*/

#define	TYPE		0
#define SUBROUTINE	1
#define ARG		2
#define LABEL		3
#define ACTION		4
#define	MASK		5
#define	MASK_ADDR	6
#define	PARAMETER	7



/*
** 
*/

#define u_NAME_LENGTH 	33



