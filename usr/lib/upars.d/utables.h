/*	@(#)utables.h	4.1				7/2/90				      */
 /*
 *
 * Program utables.h,  Module 
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
 * Program utables.h,  Module 
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 *
 * 1.00  10-Jul-1985
 *     DECnet Ultrix V1.0
 *
 */

/*******************************************************************************
**
**
**	utables.h
**
**
**	Constants and definitions for UPARS table builder and run-time routines.
**
**
*******************************************************************************/


/*
**	These values and macros define and extract flags and other data
**	in the compile and run-time parse tables of upars. IS_NOTHIN - 
**	IS_NUM denote a values' type.  TYPE_POS et al  give positions in
**	the flag word of a transion's table entry, whose contents describe
**	what kinds of data are in a transition's parse table entry.
**	The GET_xxx macros extract individual flags from the flag word.
*/




#define IS_NOTHIN	0
#define	IS_KEY		1
#define IS_ID		2
#define	IS_NUM		3


#define	TYPE_POS	0
#define	LABEL_POS	1
#define	ACTION_POS	2
#define	PARAM_POS	3
#define	MASKADDR_POS	5	
#define	MASK_POS	6	
#define	ARG_POS		8
#define DEBUG_POS	10	
#define	LAST_POS	11
#define EXIT_POS	12
#define	FAIL_POS	13
#define	RETURN_POS	14
#define ERROR_POS	15


#define	FLAG_MASK	0x1
#define	IS_MASK		0x3
#define	FLAGBITS	0x7FFF


#define	GET_FLAGS(x)	((x->flags) & FLAGBITS)
#define	GET_TYPE(x)	(((x->flags) >> TYPE_POS) & FLAG_MASK)
#define	GET_LABEL(x)	(((x->flags) >> LABEL_POS) & FLAG_MASK)
#define	GET_ACTION(x)	(((x->flags) >> ACTION_POS) & FLAG_MASK)
#define	GET_PARAM(x)	(((x->flags) >> PARAM_POS) & IS_MASK)
#define	GET_MASKADDR(x) (((x->flags) >> MASKADDR_POS) & FLAG_MASK)
#define	GET_MASK(x)	(((x->flags) >> MASK_POS) & IS_MASK)
#define	GET_DEBUG(x)	(((x->flags) >> DEBUG_POS) & FLAG_MASK)
#define	GET_ARG(x)	(((x->flags) >> ARG_POS) & IS_MASK)
#define	GET_LAST(x)	(((x->flags) >> LAST_POS) & FLAG_MASK)
#define	GET_EXIT(x)	(((x->flags) >> EXIT_POS) & FLAG_MASK)
#define GET_FAIL(x)	(((x->flags) >> FAIL_POS) & FLAG_MASK)
#define GET_RETURN(x)	(((x->flags) >> RETURN_POS) & FLAG_MASK)
#define GET_ERROR(x)    (((x->flags) >> ERROR_POS) & FLAG_MASK)


/*
**	This is the parse table type.  The flags are described above.
*/

typedef struct _utbl {
		short	 flags;            /* flag word */
		short	 type;             /* keyword number to match */
		struct _utbl	*sub;      /* or table routine to "call" */
		long	 arg;              /* optional transition argument */
		struct _utbl	*label;    /* success destination */
		struct _utbl	*fail;     /* failure destination */
		int	(*action)();       /* action routine to call */
		long	 mask;             /* bit mask */
		long	*mask_addr;        /* address to mask off */
		long	 param;            /* parameter to action routine */
	} _utbl_entry;
