/*
  static	char	*sccsid = "@(#)bbr.h	4.1	(ULTRIX)	7/2/90";
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 ************************************************************************
 *									
 *									
 *	Facility:  Dynamic Bad Block Replacement
 *									
 *	Abstract:  This module contains definitions for the
 *		   Dynamic BBR code.
 *		   
 *									
 *	Creator:	Mark Parenti	Creation Date:	May 6, 1986
 *
 *	Modification history:
 *
 * 	10-Jun-86 - map
 *		Initial version.
 */
/**/
/*
 *	RCT definitions
 */

/*	RCT header definitions						*/
#define	RCT_HDR_UNALL		0
#define	RCT_HDR_ALL_PRI		2
#define	RCT_HDR_ALL_NPR		3
#define	RCT_HDR_BAD		4
#define	RCT_HDR_ALT_BAD		5
#define	RCT_HDR_NULL		8


/*	RCT sector 0 structure						*/

typedef	struct	_rct_sector_0	{

	u_long		volser_low;	/* Low order volume serial number */
	u_long		volser_high;	/* High order volume serial number */
	u_short		flags;		/* Replacement flags		  */
	u_short		rsvd;		/* Reserved			  */
	u_long		lbn;		/* LBN being replaced		  */
	u_long		new_RBN;	/* RBN used for replacement	  */
	u_long		bad_RBN;	/* Bad RBN			  */

	} RCT_SECTOR_0;

/*	RCT sector 0 flag definitions					*/

#define	RCT_P1		0x8000		/* Phase 1 flag			*/
#define	RCT_P2		0x4000		/* Phase 2 flag			*/
#define	RCT_BR		0x2000		/* Replacement caused by bad RBN */
#define	RCT_FE		0x0080		/* Original data corrupt	*/
#define	RCT_VP		0x0002		/* Volume software write protect */

/*
 *	RCT sector 2-N format
 */

struct	desc {
		u_long	lbn	:28;	/* LBN field		*/
		u_long	hdr	:4;	/* Header field		*/
	};

typedef	struct	_rct_sector	{
		struct desc	desc[128];
	} RCT_SECTOR;

typedef	struct	_data_sector	{
		u_long	data	[128];	/* Data longwords		*/
	} DATA_SECTOR;

struct	rct_search {
	u_long		hash_block;	/* RCT block containing primary	*/
	int		hash_offset;	/* Offset of primary		*/
	u_long		hash_RBN;	/* Primary RBN			*/
	int		save_step;	/* Saved step for state machine	*/
	int		save_substep;	/* Ditto for substep		*/
	u_long		current_RBN;	/* RBN we are looking at	*/
	int		delta;		/* Ping-pong offset		*/
	u_long		RCT_block;	/* Current RCT block		*/
	int		eot;		/* End of RCT flag		*/
	int		empty;		/* Empty type			*/
	};
/*
 *	Various definitions
 */

#define	BBR_IN_PROGRESS		1
#define	BBR_SUCCESS		2
#define	BBR_FAILURE		3

#define	BBR_CMD_REF	4245946

#define	STEP4_MAX_READ		4
#define	STEP7_MAX_READ		4
#define	STEP7_MAX_REP		8

/*
 *	Reason invoked codes
 */

#define	BAD_BLOCK_FOUND		1
#define	BBR_PHASE_1		2
#define	BBR_PHASE_2		3
#define	FORCE_REPLACE		4
/*
 *	State Machine Definitions
 */

typedef	struct	_bbr_stab {

    int		( *action )();

	} BBR_STAB;	
/*
 *	Steps
 */

#define	BBR_STEP_1	1
#define	BBR_STEP_3	2
#define	BBR_STEP_4	3
#define	BBR_STEP_5	4
#define	BBR_STEP_6	5
#define	BBR_STEP_7	6
#define	BBR_STEP_7B	7
#define	BBR_STEP_7C	8
#define	BBR_STEP_8	9
#define	BBR_STEP_9	10
#define	BBR_STEP_10	11
#define	BBR_STEP_11	12
#define	BBR_STEP_12	13
#define	BBR_STEP_12B	14
#define	BBR_STEP_12C	15
#define	BBR_STEP_13	16
#define	BBR_STEP_14	17
#define	BBR_STEP_15	18
#define	BBR_STEP_16	19
#define	BBR_STEP_17	20
#define	BBR_STEP_18	21
#define	MULTI_READ	22
#define	MULTI_WRITE	23
#define	RCT_SEARCH	24
#define	BBR_MAX_STEP	24

/*
 *	Sub-steps
 */
#define	SUB_0		1
#define	SUB_1		2
#define	SEARCH_END	2
#define	REPLACE_END	2
#define	READ_END	3
#define	WRITE_END	4
#define	READ_END_2	5
#define	WRITE_END_2	6
#define	STUNT_END	7
#define	BBR_LIN_READ	7
#define	BBR_LINEAR	8
#define	BBR_MAX_SUBSTEP	8

/*
 *	RCT Search Result Codes
 */

#define	RCTS_PRIM_EMP		1
#define	RCTS_NONPRIM_EMP	2
#define	RCTS_FULL_TAB		3
#define	RCTS_READ_ERR		4
#define	RCTS_RCT_CORRUPT	5


/*
 *	IOCTL opcodes
 */

#define ACC_SCAN	1
#define ACC_CLEAR	2
#define ACC_REVEC	3
#define ACC_UNPROTECT	4
#define ACC_PRINT	5

/*
 *	BBR error codes for error print routine	
 */

#define	BBR_ERR_OFFLN	1
#define	BBR_ERR_MREAD	2
#define	BBR_ERR_RCT0	3
#define	BBR_ERR_MWRITE	4
#define	BBR_ERR_RCTF	5
#define	BBR_ERR_RCTCOR	6
#define	BBR_ERR_BADRBN	7
#define	BBR_ERR_WRITE	8

/*
 *	bbr_start return codes
 */

#define	BBR_RET_SUCC	0	/* Successfully started		*/
#define	BBR_RET_LOCK	1	/* BBR already in progress	*/
#define	BBR_RET_CTLR	2	/* Controller-initiated BBR	*/
#define	BBR_RET_WRTPRT	3	/* Unit write protected		*/
