#ifndef lint
static char *sccsid = "@(#)asjxxx.c	4.1	ULTRIX	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/************************************************************************
 *			Modification History
 *
 * 006	David L Ballenger, 04-Jun-1986
 *	Fix problem with second call to find_tunnel() in jxxxfix().
 *
 * 005	David L Ballenger, 30-May-1986
 *	Fix problem in calculating displacement which did not take into
 *	account that some things in a segment do not have their values
 *	"bumped".  This would occasionaly  allow branches to be deactivated
 *	and a byte displacement indicated when the displacement ultimately
 *	was not a byte value.  This problem is correct by the CALC_FEAR()
 *	macro.  This should prevent "Branch to far, use -J flag" errors.
 *
 * 004	David L Ballenger, 18-Apr-1986
 *	Increase the estimate of how much alignment may affect displacment
 *	and change the way still active jxxx's are handled at the end
 *	of an iteration.
 *
 * 003	David L Ballenger, 07-Mar-1986
 *	Take possible expansion of branch into effect when calculating
 *	displacement for backward branch.
 *
 * 002	David L Ballenger, 03-Mar-1986
 *	Fix problems with tunnelling.  The amount to be feared from the
 *	tunnel itself was not taken into account for backward tunnels.
 *	Also, the address for the tunnel itself was not being calculated
 *	correctly for backward or forward tunnels.
 *
 * 001	David L Ballenger, 20-Feb-1986
 *
 *	Modify behavior of jalign(), to handle case where a fill 
 *	expression is specified on a .align.
 *
 *	Also, change the way in which jbr and jxxx instructions are
 *	handled.  Statements after jbr's will always be longword aligned.
 *	Statements after jxxx instructions will be longword aligned if
 *	the jxxx instruction is exploded into a bxxx instruction followed
 *	by a brw instruction.
 *
 *	Based on:  asjxxx.c 4.7 6/30/83
 *
 ************************************************************************/

#include	<stdio.h>
#include	"as.h"
#include	"assyms.h"

#define	JBR	0x11
#define	BRW	0x31
#define	JMP	0x17
#define HALT	0x00

#ifndef DEBUG
#ifdef ASJXXX_DEBUG
static debug = 1;
#define DEBUG
#endif
#endif

/* Macro to print debugging info when a jxxx is deactiavted.
 */
#ifdef DEBUG
#define DEBUG_DEACT(from,to,estimate) \
   if (debug) \
      printf( \
	 "%cBRANCH(%#8x) to %s deactivated, (%d - %d) = %d estimated = %d\n",\
	 UNCONDITIONAL(from) ? 'U' : 'C', from, FETCHNAME(to), to->s_value, \
	 from->s_value, to->s_value - from->s_value, estimate);
#else
#define DEBUG_DEACT(from,to,estimate)
#endif

/* Define macros to be used in handling of jbr, jxxx and .aligns.
 *
 * ALIGN_FEAR		The amount of padding that could be inserted
 *			when aligning a label after a jbr or exploded
 *			jxxx.  This is something to be 'feared' and
 *			taken into account when calculating displacements.
 *			It is based on the value of LABEL_ALIGNMENT
 *			from as.h.
 *
 * UNALIGNED(pos)	Macro to use to see if 'pos' is not aligned
 *			appropriately for a label.
 *
 * ALIGN(pos)		Macro to use to align 'pos' appropriately for a
 *			label.
 * 
 * JBRDELTA		The number of bytes to add if a jbr is converted
 *			to a brw instead of a brb.
 *
 * JXXXDELTA		The number of bytes to add if a jxxx is converted
 *			to the opposite bxxx followed by a brw instead of
 *			the appropriate bxxx.
 *
 * JBRJDELTA		The number of bytes to add if a jbr is converted
 *			to a jmp instead of a brb.
 *
 * JXXXJDELTA		The number of bytes to add if a jxxx is converted
 *			to the opposite bxxx followed by a jmp instead of
 *			the appropriate bxxx.
 *
 * UNCONDITIONAL(sp)	Macro to use to see if a symtab entry is for a
 *			jbr (ie. unconditional branch).
 */
#define ALIGN_FEAR	((1 << LABEL_ALIGNMENT) - 1)
#define UNALIGNED(n)	(((n) & ALIGN_FEAR) != 0)
#define ALIGN(n)   	(((n) + ALIGN_FEAR) & ~ALIGN_FEAR)
#define	JBRDELTA	(1)
#define	JXXXDELTA	(3)
#define	JBRJDELTA	(d124)
#define	JXXXJDELTA	(d124+2)
#define UNCONDITIONAL(sp) ((sp)->s_jxfear == jbrfsize)

/* TUNNEL_ADDR(sp)	Macro to calculate the address to use for a tunnel.
 *			The s_value field points to the next instruction, so
 *			this adjusts backward to the beginning of the brb
 *			instruction in an unbumped jbr or the brw in a bumped
 *			jbr or jxxx.
 */
#define TUNNEL_ADDR(sp) ((sp)->s_value \
			 - JXXXDELTA \
			 + (((UNCONDITIONAL(sp)) && ((sp)->s_jxbump==0)) \
			    ? JBRDELTA : 0 \
			   ) \
			)
#define FORWARD_TUNNEL(sp) TUNNEL_ADDR(sp)
#define BACKWARD_TUNNEL(sp) ((sp)->s_value - JXXXDELTA)

/* TUNNEL_OK(from,to)	Macro to determine if to is a valid tunnel for from.
 *			They must have the same destination and be in the 
 *			same segment.  In addition, the potential tunnel, 
 *			'to', must either be an unconditional branch or a
 *			jxxx which has been (s_jxbump) or will be bumped
 *			(JXNOTYET).
 */
#define TUNNEL_OK(from,to)	( ((to)->s_dest==from->s_dest) \
				  && ((to)->s_index==from->s_index) \
			    	  && ( UNCONDITIONAL(to) \
			               || (to->s_tag == JXNOTYET) \
				       || (to->s_jxbump) \
			             ) \
			        )

int	jbrfsize = JBRDELTA;
int	jxxxfsize = JXXXDELTA;

/*
 *	These variables are filled by asscan.c with the
 *	last name encountered (a pointer buried in the intermediate file),
 *	and the last jxxx symbol table entry encountered.
 */
struct 	symtab	*lastnam;
struct	symtab	*lastjxxx;

initijxxx()
{
	jbrfsize = jxxxJUMP ? JBRJDELTA : JBRDELTA;
	jxxxfsize = jxxxJUMP ? JXXXJDELTA : JXXXDELTA;
	/*
	 *	Note: ifjxxxJUMP is set, then we do NOT do any tunnelling;
	 *	this was too complicated to figure out, and in the first
	 *	version of the assembler, tunnelling proved to be the hardest
	 *	to get to work!
	 */
}

/* align_next_instr
 *
 *	Routine to align the instruction following a jbr instruction or
 *	an explode jxxx instruction.  The current alignment is longword.
 */
static void
align_next_instr()
{
	/* Just write HALTs to the object until the next instruction
	 * has the proper alignment.  HALTs are used since there should
	 * be no way for the program to execute between the previous
	 * jbr or exploded jxxx and the next instruction.  If this 
	 * happens (due to an error in the algorithms here.  The HALTs
	 * will help detect the problem.
	 */
	while ( UNALIGNED(dotp->e_xvalue) ) 
		Outb(HALT);
}

/*
 *	Handle jxxx instructions
 */
ijxout(opcode, ap, nact)
	struct	Opcode	opcode;
	struct	arg	*ap;
	int	nact;
{
	if (passno == 1){
		/*
		 *	READ THIS BEFORE LOOKING AT jxxxfix()
		 *
		 *	Record the jxxx in a special symbol table entry
		 */
		register struct symtab *jumpfrom;

		/*
		 *	We assume the MINIMAL length
		 */
		putins(opcode, ap, nact); 
		jumpfrom = lastjxxx;
		jumpfrom->s_tag = JXACTIVE;
		jumpfrom->s_jxbump = 0;
		if (opcode.Op_popcode == JBR)
			jumpfrom->s_jxfear = jbrfsize;
		else
			jumpfrom->s_jxfear = jxxxfsize;
		if (lastnam == 0)
			yyerror("jxxx destination not a label");
		jumpfrom->s_dest = lastnam;
		jumpfrom->s_type = dotp->e_xtype;	/*only TEXT or DATA*/
		jumpfrom->s_index = dotp-usedot;
		/*
		 *	value ALWAYS (ALWAYS!!!) indexes the next instruction
		 *	after the jump, even in the jump must be exploded
		 *	(bumped)
		 */
		jumpfrom->s_value = dotp->e_xvalue;
		njxxx++;
	} else {/* pass2, resolve */
		/*
		 *	READ THIS AFTER LOOKING AT jxxxfix()
		 */
		reg	long		oxvalue;
		reg	struct	exp 	*xp; 
		reg	struct	symtab	*tunnel;
		reg	struct	arg	*aplast;
			struct	Opcode	nopcode;

		aplast = ap + nact - 1;
		xp = aplast->a_xp;
		if (lastjxxx->s_tag == JXTUNNEL){
			lastjxxx->s_tag = JXINACTIVE;
			tunnel = lastjxxx->s_dest;
			xp->e_xvalue = TUNNEL_ADDR(tunnel);
		}
		if (lastjxxx->s_jxbump == 0){	/*wasn't bumped, so is short form*/
			putins(opcode, ap, nact);
			if (opcode.Op_popcode == JBR)
				align_next_instr();
		} else {
			if (opcode.Op_popcode != JBR){
				/*
				 *	branch reverse conditional byte over
				 *	branch unconditional word
				 */
				oxvalue = xp->e_xvalue;
				xp->e_xvalue = ALIGN(lastjxxx->s_value);
				nopcode = opcode;
				nopcode.Op_popcode ^= 1;
				putins(nopcode, ap, nact);
				xp->e_xvalue = oxvalue;
			}
			nopcode.Op_eopcode = CORE;
			nopcode.Op_popcode = jxxxJUMP ? JMP : BRW;
			putins(nopcode, aplast, 1);
			align_next_instr();
		}
	}
}

/* jalign
 *
 * Routine called to set up the symtab entry for a .align in pass one.
 * In pass 2 it is called to insert the appropriate amount of alignment.
 */
jalign(align_expr, fill_expr, sp)
	register int		align_expr;
	register int		fill_expr;
	register struct symtab  *sp;
{
	flushfield(NBPW/4);
	if (passno == 1) {
		sp->s_tag = JXALIGN;
		/*
		 * We have something to fear when calculating the
		 * displacements for jbr/jxxx's.  The s_jxfear field
		 * gives the maximum number of fill bytes which might
		 * be inserted to achieve the desired alignment.
		 */
		sp->s_jxfear = (1 << align_expr) - 1;
		sp->s_type = dotp->e_xtype;
		sp->s_index = dotp-usedot;
		/*
		 *	We guess that the align will take up at least one
		 *	byte in the code output.  We will correct for this
		 *	initial high guess when we explode (bump) aligns
		 *	when we fix the jxxxes.  We must do this guess
		 *	so that the symbol table is sorted correctly
		 *	and labels declared to fall before the align
		 *	really get their, instead of guessing zero size
		 *	and have the label (incorrectly) fall after the jxxx.
		 *	This is a quirk of our requirement that indices into
		 *	the code stream point to the next byte following
		 *	the logical entry in the symbol table
		 */
		dotp->e_xvalue += 1;
		sp->s_value = dotp->e_xvalue;
		njxxx++;
	} else {

		/* In pass 2 align the next instruction by the appropriate 
		 * amount.  This is done by writing bytes of the fill
		 * expression to the output file until the appropriate
		 * alignment is reached.
		 */
		register int mask;

		mask = (1 << align_expr) - 1;
		while (dotp->e_xvalue & mask)
			Outb(fill_expr);
	}
}

/* jxxxfear()
 *
 * 	Routine to determine how much a jbr or jxxx has to fear from an
 *	intervening jbr, jxxx or align.  In other words how many more
 *	bytes that jbr, jxxx, or align might add to the instruction stream
 *	between the source of the and destination of the branch.
 */
static int
jxxxfear(intermediate)
	register struct symtab
		*intermediate;	/* The intervening instruction */
{
	/* How much this instuction might contribute is based on the type.
	 */
	switch(intermediate->s_tag) { 
	case JXALIGN:
		/* 
		 * The amount by which the alignment might increase is
		 * contained in s_jxfear.
		 */
		return(intermediate->s_jxfear);
	case JXINACTIVE: 
		/*
		 * Inactive jbr, jxxx, or aligns can't hurt us unless the
		 * alignment can still change as indicated by the
		 * s_jxneedalign field of the intermdiate.
		 */
		if (intermediate->s_jxneedalign) {
			/*
			 * In this case return the amount (ALIGN_FEAR)
			 * which may be needed to align the label 
			 * following brb or brw.
			 */
			return(ALIGN_FEAR);
		} else {
			/* This is completely inactive and has no effect.
			 */
			return(0);
		}
	case JXACTIVE: 
	case JXNOTYET:
		/*
		 * A jbr or jxxx which is active or has been marked to be
		 * bumped will add the amount by which the next instruction 
		 * will or can be bumped (s_jxfear) plus the amount that
		 * might be needed to align the next instruction (ALIGN_FEAR).
		 */
		return(intermediate->s_jxfear + ALIGN_FEAR);
	default:
		/* Any other type of instruction has no affect.
		 */
		return(0);
	}
}

/* search_forward()
 *
 * Routine to search forward for a tunnel.
 */
static struct symtab *
search_forward(segno,starthint,jumpfrom)
	int			segno;		/* Current segment */
	struct symtab		**starthint;
	register struct symtab	*jumpfrom;
{
	register struct symtab	**cosp,
				*sp,
				*ub;
	register int		fear;

	fear = 0;

	/* Loop forward through the segment.
	 */
	SEGITERATE(segno, starthint+1,0,cosp,sp,ub,++) {

		if (sp->s_tag <= JXQUESTIONABLE)
			continue;

		/* Check the current displacment taking into account
		 * what we have to fear from any jbr, jxxx, or aligns
		 * that may have been crossed.
		 */
		if ((fear + (FORWARD_TUNNEL(sp) - jumpfrom->s_value)) > MAXBYTE)
			/*
			 * Too far!
			 */
			break ;

		/* See if this is a tunnel, i.e. an unconditional branch
		 * or a bumped branch to the same location.
		 */
		if (TUNNEL_OK(jumpfrom,sp)) {
	   		return(sp);
		}

		/* Accumulate what we might have to fear from this statement.
		 * This will have an affect on the next iteration.
		 */
		fear += jxxxfear(sp);

	}
	/* Didn't find a tunnel.
	 */
	return((struct symtab *)0);
}

/* search_backward()
 *
 * Routine to search backward for a tunnel.
 */
static struct symtab *
search_backward(segno,starthint,jumpfrom)
	int			segno;
	struct symtab		**starthint;
	register struct symtab	*jumpfrom;
{
	register struct symtab	**cosp,
				*sp,
				*ub;
	register int		fear;

	fear = 0;

	/* Loop backward through the segment.
	 */
	SEGITERATE(segno, starthint-1,1,cosp,sp,ub,--) {

		if (sp->s_tag <= JXQUESTIONABLE)
			continue;

		/* Accumulate what we might have to fear.  In a backwards
		 * tunnel, we may have something to fear from the tunnel
		 * itself, so we add in the 'fear' amount for what might
		 * be the tunnel before using this in estimating the
		 * distance to the tunnel.
		 */
		fear -= jxxxfear(sp);
		
		/* Check the current displacment taking into account
		 * what we have to fear from any jbr, jxxx, or aligns
		 * that may have been crossed.
		 */
		if ((fear + (BACKWARD_TUNNEL(sp) - jumpfrom->s_value)) < MINBYTE)
			/*
			 * Too far!
			 */
			break ;

		/* See if this is a tunnel, i.e. an unconditional branch
		 * or a bumped branch to the same location.
		 */
		if (TUNNEL_OK(jumpfrom,sp)) {
			/*
			 * Yes
			 */
	   		return(sp);
		}
	}
	/* Didn't find a tunnel.
	 */
	return((struct symtab *)0);
}

/* find_tunnel
 *
 *	Routine to find a tunnel for a conditional branch so that it
 *	won't have to be exploded into a branch of the opposite condition
 *	followed by a branch word.  A tunnel is an unconditional branch
 *	to the same location as the original conditional branch.
 */
static void
find_tunnel(segno,starthint,jumpfrom)
	int			segno;
	struct symtab		**starthint;
	register struct symtab	*jumpfrom;
{
	register struct symtab	*tunnel;

	if (jxxxJUMP || UNCONDITIONAL(jumpfrom)) {
		/*
		 * Don't attempt to find a tunnel.
		 */
		tunnel = 0;
	} else 	if ((jumpfrom->s_dest->s_value - jumpfrom->s_value) >= 0) {
		/*
		 * The destination is ahead of us.  Assume that the
		 * most likely tunnel is ahead of us.
		 */
		tunnel = search_forward(segno,starthint,jumpfrom);
		if (tunnel == 0) {
			/*
			 * That didn't work so search backwards.
			 */
			tunnel = search_backward(segno,starthint,jumpfrom);
		}
	} else {
		/* Reverse the search order.
		 */
		tunnel = search_backward(segno,starthint,jumpfrom);
		if (tunnel == 0) {
			tunnel = search_forward(segno,starthint,jumpfrom);
		}
	}
	if (tunnel) {
		/* Indicate that a tunnel was found.
		 */
		jumpfrom->s_dest = tunnel;
		jumpfrom->s_tag = JXTUNNEL;
	} else {
		jumpfrom->s_tag = JXNOTYET ;
	}
#ifdef DEBUG
	if (debug)
		printf("%cBRANCH(%#8x) to %10s %10s, disp = %4d, value %8d\n",
			UNCONDITIONAL(jumpfrom) ? 'U' : 'C',
			jumpfrom ,
			FETCHNAME( (tunnel ? tunnel : jumpfrom)->s_dest ),
			tunnel ? "TUNNEL" : "BUMPED" ,
			jumpfrom->s_dest->s_value - jumpfrom->s_value ,
			jumpfrom->s_value);
#endif
}

/* CALC_FEAR
 *
 * Macro to calculate the amount to be feared from intervening aligns,
 * jbrs and jxxxs.  Loops backward or forward through the current segment
 * from the current jbr or jxxx to its destination.  For each questionable
 * item (JXACTIVE, JXNOTYET, JXALIGN, or JXINACTIVE)  it calls jxxxfear()
 * to see how many more bytes the item might add to the instruction stream.
 *
 * The arguments are:
 *
 *	current		This is the current value of the copointer used
 *			in the enclosing SEGITERATE loop.  It is used to
 *			determine where in the loop to start.
 *
 *	end		This is the destination of the jbr or jxxx and
 *			determines when the loop terminates.
 *
 *	disp		Variable containing the initial displacement or
 *			distance between the branch and its destination.
 *
 *	op		Either + or -. This is used to constuct the correct
 *			operators for traversing the segment forwards or
 *			backwards, and to add or subtract the fear to / from
 *			the displacement.
 *
 * This is used in jxxxfix(), look there for more details.
 */
#define CALC_FEAR(current, end, disp, op) \
	{ \
		register struct symtab *ptr, **co_ptr; \
		\
		for (co_ptr = (current op 1), ptr = *co_ptr; \
		     ptr != end ; \
		     ptr = * op/**/op co_ptr \
		    ) { \
		    	if (ptr->s_tag > JXQUESTIONABLE) { \
				disp op= jxxxfear(ptr); \
			} \
		} \
	} 


/*
 *	Pass 1.5, resolve jxxx instructions and .align in .text
 */
jxxxfix() 
{
	register struct symtab 	*jumpfrom,
				*dest,
				**cojumpfrom;
		 struct symtab	*ubjumpfrom;

	int	segno;		/* current segment number	*/

	/*
	 *	consider each segment in turn...
	 */
	for (segno = 0; segno < NLOC + NLOC; segno++){
	    /*
	     *	Do a lazy topological sort.
	     */
	    
	    register int displ;		/* estimated displacement	*/
	    unsigned n_changed;		/* # of jxxx's changed in iteration */
	    unsigned n_still_active;	/* # of jxxx's still active */
	    struct symtab **co_active;	/* Saved value of cojumpfrom for
	    			         * jumpfrom which is still active.
					 */

#ifdef DEBUG
	    int n_iterations=0;	/* # for current segment	*/ 
#endif

	    /* Iterate through the segment, fixing the jbr/jxxx's.
	     * Contiune to do this as long as the previous iteration
	     * found something to bump, or some were deactivated,
	     * but there are still active ones.
	     */
	    do {
#ifdef DEBUG
		if (debug) {
			printf("\nSegment %d, iteration %d\n",
				segno, ++n_iterations);
		}
#endif
		/* Nothing changed or active yet.
		 */
		n_changed = 0;
		n_still_active = 0;
		co_active = NULL;

		SEGITERATE(segno, 0, 0, cojumpfrom, jumpfrom, ubjumpfrom, ++){

			/* Ignore if not an active jbr/jxxx
			 */
			if (jumpfrom->s_tag != JXACTIVE) {
				continue;
			}

			/* Find the destination of the jbr/jxxx, and make
			 * sure that it is in the same segment.
			 */
			dest = jumpfrom->s_dest;
			if (jumpfrom->s_index != dest->s_index){
				yyerror("Intersegment jxxx");
				continue;
			}

			/* Do an intial estimate of the displacement
			 * needed for the destination address.
			 */
			displ = dest->s_value - jumpfrom->s_value;

			if ( ! ISBYTE(displ) ) {

				/* It is not a byte displacement, so try to
				 * prevent the expansion of a jxxx by
				 * finding a tunnel, but if this doesn't
				 * doesn't work, mark it for expansion.
				 */
				find_tunnel(segno,cojumpfrom,jumpfrom) ;

				/* In either case make note that something
				 * has changed.
				 */
				n_changed++;
			
			} else {
				if (displ >= 0) {

					/* An unconditional forward branch 
					 * also needs to take the alignment
					 * following it into account.
					 */
					if (UNCONDITIONAL(jumpfrom)) {
						displ += ALIGN_FEAR;
					}
					/* It is a forward branch.  Do a 
					 * forward search for any intervening
					 * jbr's, jxxx's, or align's, as these
					 * may affect the actual displacement.
					 */
					CALC_FEAR(cojumpfrom,dest,displ,+) ;
				} else {
					/* Do a backward search as above, for
					 * a backward branch.
					 */
					CALC_FEAR(cojumpfrom,dest,displ,-) ;
				}

				if (ISBYTE(displ)){
					/*
					 * The displacement will still fit in
					 * a byte so this jbr/jxx can be 
					 * deactivated.  Note that a jxxx
					 * has changed state.
					 */
					jumpfrom->s_tag = JXINACTIVE;
					jumpfrom->s_jxneedalign
						= UNCONDITIONAL(jumpfrom);
					DEBUG_DEACT(jumpfrom,dest,displ) ;
					n_changed++;
				} else if (n_changed > 0) {
					/* Keep track that a jxxx/jbr is
					 * still active.
					 */
					n_still_active++;
				} else if (co_active == NULL) {

					/* Remember the first still active
					 * jxxx so that we can use it later.
					 */
					co_active = cojumpfrom;
					n_still_active++;
				} else if (! UNCONDITIONAL(*co_active)
					   && UNCONDITIONAL(jumpfrom)) {
					
					/* Better to bump an unconditional
					 * branch than a conditional branch.
					 */
					co_active = cojumpfrom;
					n_still_active++;
				} else {
					n_still_active++;
				}
			}
		}

		if (co_active && n_changed == 0 ) {
			/*
			 * Nothing changed in this iteration, so force
			 * an active jxxx to change into a tunnel or a
			 * bumped jxxx.  This may allow us to deactivate
			 * some other still active jxxx's.
			 */
#ifdef DEBUG
			if (debug)
				fputs("*** Forcing BUMP or TUNNEL ***\n",
				      stdout);
#endif
			find_tunnel(segno,co_active,*co_active);
			/*
			 * Decrement the number still active, so that we
			 * won't do another iteration if that was the only
			 * one.
			 */
			n_still_active--;
		}
		/* Now go through the segment bumping what needs to be
		 * bumped and aligning what can and needs to be aligned.
		 * This is done regardless of whether anything changed
		 * or is still active, so that the case all aligns will
		 * be handled, even if there were no branches.
		 */
		jxxxbump(segno);

		/* End of iterating through all symbols in this
		 * segment.  Keep doing it as long as there is anything
		 * still active.
		 */
	    } while (n_still_active > 0) ;

#ifdef DEBUG
	    if (debug)
	    	jxxxcheck(segno);
#endif
	}
}

#ifdef DEBUG
/*
 * Debugging routine to check JXXX's and print out debugging info.
 */
jxxxcheck(segno)
	int segno;
{
	register	struct	symtab	**cosp, *sp;
	register	struct	symtab		*ub;

	/* Iterate through the whole segment.
	 */
	SEGITERATE(segno, 0, 0, cosp, sp, ub, ++){
		switch(sp->s_tag) {
		case LABELID:
			fprintf(stdout,
				"%s, value = %8d\n",
					FETCHNAME(sp), sp->s_value);
			break;
		case JXTUNNEL:
			fprintf(stdout,
				"TUNNEL(%#8x) to %#10x, disp = %4d, value = %8d\n",
				sp,
				sp->s_dest,
				TUNNEL_ADDR(sp->s_dest) - sp->s_value,
				sp->s_value);
			break ;
		case JXINACTIVE:
			if (sp->s_dest == 0) {
				fprintf(stdout,
					"\tALIGN(%#8x), value = %8d\n",
					sp, sp->s_value);
				break;
			}
			fprintf(stdout,
				"\t%cBRANCH(%#8x) to %10s, disp = %4d, value = %8d %s\n",
				UNCONDITIONAL(sp)?'U':'C',
				sp,
				FETCHNAME(sp->s_dest),
				sp->s_dest->s_value - sp->s_value,
				sp->s_value,
				sp->s_jxbump ? "BUMPED":"");
			if (sp->s_jxneedalign)
				fputs(" ERROR needs alignment \n",stdout);
			break ;
		case JXACTIVE:
		case JXNOTYET:
			fprintf(stdout,
				"BRANCH(%#8x) to %#10x, disp = %4d, value = %8d ERROR tag = %s\n",
				sp,
				sp->s_dest,
				sp->s_dest->s_value - sp->s_value,
				sp->s_value,
				sp->s_tag==JXACTIVE?"JXACTIVE":"JXNOTYET");
			break ;
		}
	}
}
#endif

/*
 *	Go through the symbols in a given segment number,
 *	and see which entries are jxxx entries that have
 *	been logically "exploded" (expanded), but for which
 *	the value of textually following symbols has not been
 *	increased.  Attempt to align as much as we can so that
 *	that we can (possibly) deactivate jbr/jxxx's in fewer
 *	iterations.  This may also prevent some jbr/jxxx's
 *	from being exploded unnecessarily.
 */

jxxxbump(segno)
	int	segno;
{
	register	struct	symtab	**cosp, *sp;
	register	struct	symtab		*ub;
	register	int		cum_bump;
	register	int		align_ok;

#ifdef DEBUG
	if (debug)
		fputs("Bumping\n",stdout);
#endif

	cum_bump = 0;	/* No cumulative bump yet. */
	align_ok = 1;	/* No active jbr/jxxx's yet, so ok to align. */

	/* Iterate through the whole segment.
	 */
	SEGITERATE(segno, 0, 0, cosp, sp, ub, ++){
		switch(sp->s_tag) {
		case JXACTIVE:
			/*
			 * Once an active jbr/jxxx has been seen we can
			 * no longer do alignment, since it may later be
			 * exploded.
			 */
			align_ok = 0;
			/* FALLTHROUGH */
		case OKTOBUMP:
		case LABELID:
		case FLOATINGSTAB:
		case IGNOREBOUND:
		case OBSOLETE:
		case JXQUESTIONABLE:
		case JXTUNNEL:
			/*
			 * Simply add the cumulative bump to these.
			 */
			sp->s_value += cum_bump;
			break;
		case JXINACTIVE:
			/*
			 * Add the cumulative bump then see if we can 
			 * (align_ok) or need to (s_jxneedalign) align
			 * the instruction following this one.
			 */
			sp->s_value += cum_bump;
			if (align_ok && sp->s_jxneedalign) {
				/*
				 * Add the amount needed to align the next
				 * instruction, but don't change s_value
				 * for this instruction, since this will
				 * be needed later if this instruction 
				 * provides a tunnel.   This also make sense
				 * because the alignment actually inserts
				 * HALT instructions between this  and the
				 * next.
				 */
				cum_bump += ALIGN(sp->s_value) - sp->s_value;

				/* Indicate that this no longer needs 
				 * alignment.
				 */
				sp->s_jxneedalign = 0;
			}
			break;

		case JXNOTYET:
			/*
			 * This one needs to be bumped, so do it and mark it
			 * inactive.  It's s_jxfear is used to bump the 
			 * cumulative bump which is then used to bump it's
			 * own s_value.
			 */
			sp->s_tag = JXINACTIVE;
			sp->s_jxbump = 1;
			cum_bump += sp->s_jxfear;
			sp->s_value += cum_bump;

			/* We want to align the instruction following the
			 * brw or bxxx/brw that this is turning into.  Can
			 * it be done now.
			 */
			if (align_ok) {
				/*
				 * Yes, so add the alignment factor into
				 * the cumulative bump and clear s_jxneedalign
				 * to indicate that this has been done.
				 */
				cum_bump += ALIGN(sp->s_value) - sp->s_value;
				sp->s_jxneedalign = 0;
			} else {
				/* Can't calculate how much alignment needs
				 * to be done, so indicate that it should be
				 * done in a later call.
				 */
				sp->s_jxneedalign = 1;
			}
			break ;
		case JXALIGN:
			/* 
			 * Add any cumulative bump and calculate the alignment
			 * needed, if posssible.  If not, it will be done in
			 * a later call.
			 */
			sp->s_value += cum_bump;
			if (align_ok) {

				/* How many extra bytes are there over the
				 * required alignment?  This is caluclated
				 * on (sp->s_value - 1) rather than just
				 * sp->s_value.  See jalign() for detatils.
				 */
				int extra = (sp->s_value - 1) & 
					      ((unsigned)sp->s_jxfear);
				if (extra == 0) {
					/*
					 * We over estimated, back off to
					 * do the alignment.
					 */
					sp->s_value--;
					cum_bump--;
				} else {
					sp->s_jxfear -= extra ;
					sp->s_value += sp->s_jxfear;
					cum_bump += sp->s_jxfear;
				}
				/* Now mark it inactive and indicate that
				 * it no longer needs alignment.
				 */
				sp->s_jxneedalign = 0;
				sp->s_tag = JXINACTIVE;
			}
			break;
		}
	}
	/* Finally bump the location value for the segment.
	 */
	usedot[segno].e_xvalue += cum_bump;
}
