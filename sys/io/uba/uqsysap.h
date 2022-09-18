/*
 *	@(#)uqsysap.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986, 1987  by		*
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
 *	Modification History
 *
 * 17-Mar-88 -- map
 *	Change unmapped buffer handle to simply be a longword.
 */

#define	NCON	4		/* Number of possible connections	*/




/* UQ PPD Data Structure Definitions
 */

typedef struct _uqbhandle {			/* UQ buffer descriptor	    */
	union {					/* First word is overlaid   */
		struct {			/* Non-mapped descriptor    */
			u_long buf_add;		/* Buffer address to use    */
			u_long unused;		/* Unused longword MBZ      */
		}un_mapped;
		
		struct {			/* Mapped descriptor	    */
			u_long	map_idx;	/* Buf offset, map reg indx */
			u_long	map_base :30;	/* Base of map reg table    */
			u_long		:2;	/* MBZ		            */
		}mapped;

		struct {
			u_long		:31;	/* Don't care		    */
			u_long	map_flg	:1;	/* Mapping active flag	    */
		}mapflg;
	}bh;
} UQBHANDLE;



typedef struct _uqlpib {	/* UQ Local Port Information Block 	*/
	u_short	uq_state;	/* current state of port 		*/
	u_short	uq_credits[NCON]; /* uq port credits 			*/
	int	uq_type;	/* controller type 			*/
	u_short	uq_flags;	/* flag word				*/
	u_short	sa;		/* SA register contents for error log	*/
} UQLPIB;

