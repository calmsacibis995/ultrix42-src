
/*
 *	@(#)scsi_debug.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *
 * scsi_debug.h		11/17/88
 *
 * CVAX/FIREFOXstar/PVAX/PMAX SCSI device driver (common debug defines)
 *
 * Modification history:
 *
 * 09/21/89     Mitch McConnell
 *	Added new macro to expand commands.
 *
 * 09/19/89	Mitch McConnell
 *	Added new debug category for debugging DMA.  Created #defines for
 *	debug bits.
 *
 * 11/17/88	John A. Gallant
 *	Creation date from pieces of the scsi modules.
 *
 ************************************************************************/

/* ---------------------------------------------------------------------- */

/* The ultimate debug definition.  This define will turn it all on. */

#undef SZDEBUG 

/* ---------------------------------------------------------------------- */

/* The MSB of scsidebug, has special meaning.  It is used to help define
the target that is being tracked.  Bit 31 is the TVALID, target valid, bit
if there is an ID in this byte to track, this bit must be set.  Bits 24 - 26
will contain the target ID.  The remaining bits 0 - 23, are used to define
the DEBUG level.

The scsi debug flags variable is defined as follows:

bit 0: routine entry and exit
bit 1: code flow paths through the routines.
bit 2: phase/state values
bit 3: state machine flow
bit 4: error handling
bit 5: expansion of commands and special level responses
bit 7: I/O mapping
bit 8: DMA-specific stuff

This list can be expanded and will probably need to be "defined". */

extern int scsidebug;

#define TVALID	0x80000000		/* targit bits are valid */
#define TMASK	0x07000000		/* mask for the targit bits */
#define TSHIFT	24			/* shift for target bits */

#define NTARG	0xFF			/* the debug has no target knowledge */

#define	SCSID_ENTER_EXIT	0x01
#define SCSID_FLOW		0x02
#define SCSID_PHASE_STATE	0x04
#define SCSID_STATE_FLOW	0x08
#define SCSID_ERRORS		0x10
#define SCSID_CMD_EXP		0x20
#define SCSID_IO_MAPPING	0x40
#define SCSID_DMA_FLOW		0x80
#define SCSID_DISCONNECT	0x0100


/* ---------------------------------------------------------------------- */

/* This Macro is an attempt to be able to track target specific messages, and
allow for specific subsets of the DEBUG statements be printed. 
The format for the Macro is not immediatly obvious.  The T argument is
for target specific tracking.  The F argument is for tracking particular
subsets of the statements.  This flag argument is compared with the 
scsidebug static variable to determine if the user wants to see the message.
The X argument is "ugly" it must be a complete printf argument set 
enclosed within "()", this will allow the pre-processor to include
it in the finial printf statement. */

/* NOTE: an attempt to describe what the following if statements are checking.

The first "if( scsidebug & (int)F )" is simply checking to see if any of the
flags for the Macro are turned on.  It is not checking for exact matching, ie
"== (int)F", this allows the same Macro to be used for different subsets.

The second "if( !(scsidebug & TVALID) || (((scsidebug & TVALID) &&	
		(((scsidebug && TMASK) >> TSHIFT) == T)) || (T == NTARG)))"
is checking the target information.  The if statement is large, to allow the
use of only one cprintf() call.  The first condition checks to see if the
target valid bit is set, if not, cprintf() is called for the subset.  If

the valid bit is set, checking has to be done to see if the current target
id matches what is in scsidebug.  The TMASK and TSHIFT move the ID field in
scsidebug to compare with T.  The last check for NTARG, will allow the Macro
to print even if there is a valid target ID in scsidebug. */

#ifdef SZDEBUG
#   define PRINTD(T,F,X);						\
    { 									\
	if( scsidebug & (int)F )						\
	    if( ((scsidebug & TVALID) == 0) || ((((scsidebug & TVALID) != 0) && \
		(((scsidebug & TMASK) >> TSHIFT) == T)) || (T == NTARG))) { \
		(void)cprintf X	;					\
		DELAY(99999);						\
		}							\
    }
#else /*  SZDEBUG */
#   define PRINTD(T,F,X);
#endif /*  SZDEBUG */


/* Macro in lieu of subroutine to expand commands, data, etc...		*/

#ifdef SZDEBUG
#define SZDEBUG_EXPAND(T, ptr, len);					\
    {									\
    int i;								\
    if (scsidebug & SCSID_CMD_EXP)					\
        if( ((scsidebug & TVALID) == 0) || ((((scsidebug & TVALID) != 0) && \
	    (((scsidebug & TMASK) >> TSHIFT) == T)) || (T == NTARG))) { \
            for (i = 0; i < len; i++)					\
	        cprintf(" %2x", (((unsigned)*(ptr + i)) & 0xff));	\
                cprintf("\n");						\
	    }								\
    }
#else /* SZDEBUG */
#define SZDEBUG_EXPAND(T, ptr, len);
#endif /* SZDEBUG */
