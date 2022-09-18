/*
 * Copyright (c) 1988-1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  Optimization module for tcpdump intermeddiate representation.
 *
 * SCCSID: @(#)optimize.c	4.2	ULTRIX	1/25/91
 * Based on:
 * rcsid[] = "@(#) $Header: optimize.c,v 1.25 91/01/07 16:03:21 mccanne Exp $ (LBL)"
 */

#include <stdio.h>
#include <sys/types.h>

#include <sys/time.h>
#include <net/bpf.h>

#include "interface.h"
#include "gencode.h"

#define A_REGNO BPF_MEMWORDS
#define X_REGNO (BPF_MEMWORDS+1)

/*
 * This define is used to represent *both* the accumulator and
 * x register in use-def computations.
 * Currently, the use-def code assumes only one definition per instruction.
 */
#define AX_REGNO N_MEMWORDS

/*
 * A flag to indicate that further optimization is needed.
 * Iterative passes are continued until a given pass yields no 
 * branch movement.
 */
static int done;

/*
 * A block is marked if only if its mark equals the current mark.
 * Rather than traverse the code array, marking each item, 'cur_mark' is
 * incremented.  This automatically makes each element unmarked.
 */
static int cur_mark;
#define isMarked(p) ((p)->mark == cur_mark)
#define unMarkAll() cur_mark += 1
#define Mark(p) ((p)->mark = cur_mark)

static void opt_init();
static void opt_cleanup();

static void make_marks();
static void mark_code();

static void intern_blocks();

static int eq_slist();

static int n_blocks;
struct block **blocks;

/*
 * A bit vector set representation of the dominators.  
 * We round up the set size to the next power of two.  
 */
static int ns_nwords;
struct block **levels;
u_long *space;
#define BITS_PER_WORD (8*sizeof(u_long))
/*
 * True if a is in nodeset {p}
 */
#define NS_MEMBER(p, a) \
((p)[(unsigned)(a) / BITS_PER_WORD] & (1 << ((unsigned)(a) % BITS_PER_WORD)))

/*
 * Add 'a' to nodeset p.
 */
#define NS_INSERT(p, a) \
(p)[(unsigned)(a) / BITS_PER_WORD] |= (1 << ((unsigned)(a) % BITS_PER_WORD))

/*
 * Delete 'a' from nodeset p.
 */
#define NS_DELETE(p, a) \
(p)[(unsigned)(a) / BITS_PER_WORD] &= ~(1 << ((unsigned)(a) % BITS_PER_WORD))

/*
 * a := a intersect b
 */
#define NS_INTERSECT(a, b)\
{\
	u_long *_x = a, *_y = b;\
	int _n = ns_nwords;\
	while (--_n >= 0) *_x++ &= *_y++;\
}

/*
 * a := a union b
 */
#define NS_UNION(a, b)\
{\
	u_long *_x = a, *_y = b;\
	int _n = ns_nwords;\
	while (--_n >= 0) *_x++ |= *_y++;\
}

static nodeset all_dom_sets;
static nodeset all_closure_sets;

#define MAX(a,b) ((a)>(b)?(a):(b))

static void
find_levels_r(b)
	struct block *b;
{
	int level;

	if (isMarked(b))
		return;

	Mark(b);
	b->link = 0;

	if (b->jt) {
		find_levels_r(b->jt);
		find_levels_r(b->jf);
		level = MAX(b->jt->level, b->jf->level) + 1;
	} else
		level = 0;
	b->level = level;
	b->link = levels[level];
	levels[level] = b;
}

/*
 * Level graph.  The levels go from 0 at the leaves to 
 * N_LEVELS at the root.  The levels[] array points to the
 * first node of the level list, whose elements are linked
 * with the 'link' field of the struct block.
 */
static void
find_levels(root)
	struct block *root;
{
	bzero((char *)levels, n_blocks * sizeof(*levels));
	unMarkAll();
	find_levels_r(root);
}

/* 
 * Find dominator relationships.
 * Assumes graph has been leveled.
 */
static void
find_dom(root)
	struct block *root;
{
	int i;
	struct block *b;
	u_long *x;

	/*
	 * Initialize sets to contain all nodes.
	 */
	x = all_dom_sets;
	i = n_blocks * ns_nwords;
	while (--i >= 0)
		*x++ = ~0;
	/* Root starts off empty. */
	for (i = ns_nwords; --i >= 0;)
		root->dom[i] = 0;
	
	/* root->level is the highest level no found. */
	for (i = root->level; i >= 0; --i) {
		for (b = levels[i]; b; b = b->link) {
			NS_INSERT(b->dom, b->id);
			if (b->jt == 0)
				continue;
			NS_INTERSECT(b->jt->dom, b->dom);
			NS_INTERSECT(b->jf->dom, b->dom);
		}
	}
}

/* 
 * Find the backwards transitive closure of the flow graph.  These sets
 * are backwards in the sense that we find the set of nodes that reach
 * a given node, not the set of nodes that can be reached by a node.
 *
 * Assumes graph has been leveled.
 */
static void
find_closure(root)
	struct block *root;
{
	int i;
	struct block *b;

	/*
	 * Initialize sets to contain no nodes.
	 */
	bzero((char *)all_closure_sets, 
	      n_blocks * ns_nwords * sizeof(*all_closure_sets));
	
	/* root->level is the highest level no found. */
	for (i = root->level; i >= 0; --i) {
		for (b = levels[i]; b; b = b->link) {
			NS_INSERT(b->closure, b->id);
			if (b->jt == 0)
				continue;
			NS_UNION(b->jt->closure, b->closure);
			NS_UNION(b->jf->closure, b->closure);
		}
	}
}

/* 
 * Return the register number that is used by s.  If A and X are both
 * used, return AX_REGNO.  If no register is used, return -1.
 * 
 * The implementation should probably change to an array access, and
 * and a special case for trx and tra.
 */
static int
reguse(s)
	struct stmt *s;
{
	switch (s->code) {
	case GEOp:
	case GTOp:
	case EQOp:
	case AddIOp:
	case SubIOp:
	case MulIOp:
	case DivIOp:
	case AndIOp:
	case OrIOp:
	case LshIOp:
	case RshIOp:
	case NegOp:
	case TaxOp:
	case StmOp:
	case RetAOp:
		return A_REGNO;

	case LdOp:
	case LdHOp:
	case LdBOp:
	case LdLenOp:
	case LdIOp:
	case LdXIOp:
	case LdxmsOp:
	case NopOp:
	case RetOp:
		return -1;

	case ILdOp:
	case ILdHOp:
	case ILdBOp:
	case TxaOp:
	case StmXOp:
		return X_REGNO;

	case AddXOp:
	case SubXOp:
	case MulXOp:
	case DivXOp:
	case AndXOp:
	case OrXOp:
	case LshXOp:
	case RshXOp:
		return AX_REGNO;

	case LdmOp:
	case LdmXOp:
		return s->k;
	}
	abort();
	/* NOTREACHED */
}

/* 
 * Return the register number that is defined by 's'.  We assume that
 * a single stmt cannot define more than one register.  If no register
 * is defined, return -1.
 *
 * The implementation should probably change to an array access, and
 * and a special case for txr and tar.
 */
static int
regdef(s)
	struct stmt *s;
{
	switch (s->code) {
	case GEOp:
	case GTOp:
	case EQOp:
	case NopOp:
	case RetOp:
	case RetAOp:
		return -1;

	case LdOp:
	case LdHOp:
	case LdBOp:
	case LdLenOp:
	case LdIOp:
	case ILdOp:
	case ILdHOp:
	case ILdBOp:
	case AddIOp:
	case SubIOp:
	case MulIOp:
	case DivIOp:
	case AndIOp:
	case OrIOp:
	case LshIOp:
	case RshIOp:
	case NegOp:
	case AddXOp:
	case SubXOp:
	case MulXOp:
	case DivXOp:
	case AndXOp:
	case OrXOp:
	case LshXOp:
	case RshXOp:
	case TxaOp:
	case LdmOp:
		return A_REGNO;

	case LdXIOp:
	case LdxmsOp:
	case TaxOp:
	case LdmXOp:
		return X_REGNO;

	case StmOp:
	case StmXOp:
		return s->k;
	}
	abort();
	/* NOTREACHED */
}

static void
compute_local_ud(b)
	struct block *b;
{
	struct slist *s;
	regset def = 0, use = 0;
	int regno;

	for (s = b->stmts; s; s = s->next) {
		regno = reguse(&s->s);
		if (regno >= 0) {
			if (regno == AX_REGNO) {
				if (!REGELEM(def, X_REGNO))
					use |= REGMASK(X_REGNO);
				if (!REGELEM(def, A_REGNO))
					use |= REGMASK(A_REGNO);
			}
			else if (regno < N_MEMWORDS) {
				if (!REGELEM(def, regno))
					use |= REGMASK(regno);
			}
			else
				abort();
		}
		regno = regdef(&s->s);
		if (regno >= 0)
			def |= REGMASK(regno);
	}
	if (!REGELEM(def, A_REGNO) && BPF_ISJUMP(b->s.code))
		use |= REGMASK(A_REGNO);
		
	b->def = def;
	b->in_use = use;
}

/*
 * Assume graph is already leveled.
 */
static void
find_ud(root)
	struct block *root;
{
	int i, maxlevel;
	struct block *p;

	/*
	 * root->level is the highest level no found;
	 * count down from there.
	 */
	maxlevel = root->level;
	for (i = maxlevel; i >= 0; --i)
		for (p = levels[i]; p; p = p->link) {
			compute_local_ud(p);
			p->out_use = 0;
		}

	for (i = 1; i <= maxlevel; ++i) {
		for (p = levels[i]; p; p = p->link) {
			p->out_use |= p->jt->in_use | p->jf->in_use;
			p->in_use |= p->out_use;
		}
	}
}

/* 
 * These data structures used in a Cocke and Shwarz style
 * value numbering scheme.  Since the flowgraph is acyclic,
 * exit values can be propagated from a node's predecessors
 * provided it is uniquely defined.
 */
struct valnode {
	enum bpf_code code;
	long v0, v1;
	long val;
	struct valnode *next;
};
	
#define MODULUS 213
static struct valnode *hashtbl[MODULUS];
static int curval;
static int maxval;

/* Integer constants mapped with the LdIOp opcode. */
#define K(i) F(LdIOp, i, 0L)

struct vmapinfo {
	int is_const;
	long const_val;
};

struct vmapinfo *vmap;
struct valnode *vnode_base;
struct valnode *next_vnode;

static void
init_val()
{
	curval = 0;
	next_vnode = vnode_base;
	bzero((char *)vmap, maxval * sizeof(*vmap));
	bzero((char *)hashtbl, sizeof hashtbl);
}

/* Because we really don't have an IR, this stuff is a little messy. */
static long
F(code, v0, v1)
	enum bpf_code code;
	long v0, v1;
{
	u_int hash;
	int val;
	struct valnode *p;

	hash = (u_int)code ^ (v0 << 4) ^ (v1 << 8);
	hash %= MODULUS;

	for (p = hashtbl[hash]; p; p = p->next)
		if (p->code == code && p->v0 == v0 && p->v1 == v1)
			return p->val;

	val = ++curval;
	if (code == LdIOp || code == LdXIOp) {
		vmap[val].const_val = v0;
		vmap[val].is_const = 1;
	}
	p = next_vnode++;
	p->val = val;
	p->code = code;
	p->v0 = v0;
	p->v1 = v1;
	p->next = hashtbl[hash];
	hashtbl[hash] = p;

	return val;
}

static inline void
vstore(s, valp, newval, alter)
	struct stmt *s;
	long *valp;
	long newval;
	int alter;
{
	if (alter && *valp == newval)
		s->code = NopOp;
	else
		*valp = newval;
}

static void
fold_op(s, v0, v1)
	struct stmt *s;
	long v0, v1;
{
	long a, b;

	a = vmap[v0].const_val;
	b = vmap[v1].const_val;

	switch (s->code) {
	case AddIOp:
	case AddXOp:
		a += b;
		break;

	case SubIOp:
	case SubXOp:
		a -= b;
		break;

	case MulIOp:
	case MulXOp:
		a *= b;
		break;

	case DivIOp:
	case DivXOp:
		if (b == 0)
			error("division by zero");
		a /= b;
		break;

	case AndIOp:
	case AndXOp:
		a &= b;
		break;

	case OrIOp:
	case OrXOp:
		a |= b;
		break;

	case LshIOp:
	case LshXOp:
		a <<= b;
		break;

	case RshIOp:
	case RshXOp:
		a >>= b;
		break;

	case NegOp:
		a = -a;
		break;

	default:
		abort();
	}
	s->k = a;
	s->code = LdIOp;
	done = 0;
}

static inline struct slist *
this_op(s)
	struct slist *s;
{
	while (s != 0 && s->s.code == NopOp)
		s = s->next;
	return s;
}

static void
opt_peep(b)
	struct block *b;
{
	struct slist *s;
	struct slist *next, *last;
	int val;
	long v;

	s = b->stmts;
	if (s == 0)
		return;

	last = s;
	while (1) {
		s = this_op(s);
		if (s == 0)
			break;
		next = this_op(s->next);
		if (next == 0)
			break;
		last = next;

		if (s->s.code == StmOp &&
		    next->s.code == LdmXOp &&
		    s->s.k == next->s.k) {
			done = 0;
			next->s.code = TaxOp;
		}
		if (s->s.code == LdIOp &&
			 next->s.code == TaxOp) {
			s->s.code = LdXIOp;
			next->s.code = TxaOp;
			done = 0;
		}
		/*
		 * This is an ugly special case, but it happens
		 * when you say tcp[k] or udp[k] where k is a constant.
		 */
		if (s->s.code == LdIOp &&
		    next->s.code == LdxmsOp) {
			struct slist *add, *tax, *ild;

			add = this_op(next->next);
			if (add == 0 || add->s.code != AddXOp)
				break;

			tax = this_op(add->next);
			if (tax == 0 || tax->s.code != TaxOp)
				break;

			ild = this_op(tax->next);
			if (ild == 0 || 
			    (ild->s.code != ILdOp && ild->s.code != ILdHOp &&
			     ild->s.code != ILdBOp))
				break;
			/* 
			 * XXX We need to check that X is not
			 * subsequently used.  We know we can eliminate the
			 * accumaltor modifications since it is defined
			 * by the last stmt of this sequence.
			 *
			 * We want to turn this sequence:
			 *
			 * (004) ldi     #0x2		{s}
			 * (005) ldxms   [14]		{next}
			 * (006) addx			{add}
			 * (007) tax			{tax}
			 * (008) ild     [x+0]		{ild}
			 *
			 * into this sequence:
			 *
			 * (004) nop
			 * (005) ldxms   [14]
			 * (006) nop
			 * (007) nop
			 * (008) ild     [x+2]
			 *
			 * The nops get removed later.
			 */
			ild->s.k += s->s.k;
			s->s.code = NopOp;
			add->s.code = NopOp;
			tax->s.code = NopOp;
			done = 0;
		}
		s = next;
	}
	/*
	 * If we have a subtract to do a comparsion, and the X register
	 * is a known constant, we can merge this value into the 
	 * comparison.
	 */
	if (last->s.code == SubXOp && !REGELEM(b->out_use, A_REGNO)) {
		val = b->val[X_REGNO];
		if (vmap[val].is_const) {
			b->s.k += vmap[val].const_val;
			last->s.code = NopOp;
			done = 0;
		}
	}
	/*
	 * Likewise, a constant subtract can be simplified.
	 */
	else if (last->s.code == SubIOp && !REGELEM(b->out_use, A_REGNO)) {
		b->s.k += last->s.k;
		last->s.code = NopOp;
		done = 0;
	}
	/*
	 * If the accumulator is a known constant, we can compute the
	 * comparison result.
	 */
	val = b->val[A_REGNO];
	if (vmap[val].is_const) {
		v = vmap[val].const_val;
		switch (b->s.code) {
		case EQOp:
			v = v == b->s.k;
			break;

		case GTOp:
			v = v > b->s.k;
			break;

		case GEOp:
			v = v >= b->s.k;
			break;

		default:
			abort();
		}
		if (b->jf != b->jt)
			done = 0;
		if (v)
			b->jf = b->jt;
		else
			b->jt = b->jf;
	}
}

static inline enum bpf_code
opadjust(code, diff)
	enum bpf_code code;
	int diff;
{
	return (enum bpf_code)((int)code - diff);
}

/*
 * Update compute the symbolic value of expression of 's', and update
 * anything it defines in the value table 'val'.  If 'alter' is true,
 * do various optimizations.  This code would be cleaner if symblic
 * evaluation and code transformations weren't folded together.
 */
static void
opt_stmt(s, val, alter)
	struct stmt *s;
	long val[];
	int alter;
{
	long v;

	switch (s->code) {

	case LdOp:
	case LdHOp:
	case LdBOp:
		v = F(s->code, s->k, 0L);
		vstore(s, &val[A_REGNO], v, alter);
		break;

	case ILdOp:
	case ILdHOp:
	case ILdBOp:
		v = val[X_REGNO];
		if (alter && vmap[v].is_const) {
			s->code = opadjust(s->code, (int)ILdOp - (int)LdOp);
			s->k += vmap[v].const_val;
			v = F(s->code, s->k, 0L);
			done = 0;
		}
		else
			v = F(s->code, s->k, v);
		vstore(s, &val[A_REGNO], v, alter);
		break;
		
	case LdLenOp:
		v = F(s->code, 0L, 0L);
		vstore(s, &val[A_REGNO], v, alter);
		break;

	case LdIOp:
		v = K(s->k);
		vstore(s, &val[A_REGNO], v, alter);
		break;

	case LdXIOp:
		v = K(s->k);
		vstore(s, &val[X_REGNO], v, alter);
		break;

	case LdxmsOp:
		v = F(s->code, s->k, 0L);
		vstore(s, &val[X_REGNO], v, alter);
		break;

	case NegOp:
		if (alter && vmap[val[A_REGNO]].is_const) {
			s->code = LdIOp;
			s->k = -vmap[val[A_REGNO]].const_val;
			val[A_REGNO] = K(s->k);
		}
		else
			val[A_REGNO] = F(s->code, val[A_REGNO], 0L);
		break;

	case AddIOp:
	case SubIOp:
	case MulIOp:
	case DivIOp:
	case AndIOp:
	case OrIOp:
	case LshIOp:
	case RshIOp:
		if (alter && vmap[val[A_REGNO]].is_const) {
			fold_op(s, val[A_REGNO], K(s->k));
			val[A_REGNO] = K(s->k);
		}
		else
			val[A_REGNO] = F(s->code, val[A_REGNO], K(s->k));
		break;

	case AddXOp:
	case SubXOp:
	case MulXOp:
	case DivXOp:
	case AndXOp:
	case OrXOp:
	case LshXOp:
	case RshXOp:
		if (alter && vmap[val[X_REGNO]].is_const) {
			if (vmap[val[A_REGNO]].is_const) {
				fold_op(s, val[A_REGNO], val[X_REGNO]);
				val[A_REGNO] = K(s->k);
			}
			else {
				s->code = opadjust(s->code, 
						   (int)AddXOp - (int)AddIOp);
				s->k = vmap[val[X_REGNO]].const_val;
				done = 0;
				val[A_REGNO] = 
					F(s->code, val[A_REGNO], K(s->k));
			}
			break;
		}
		/*
		 * Check if we're doing something to an accumulator
		 * that is 0, and simplify.  This may nnot seem like
		 * much of a simplification but it could open up further
		 * optimizations.
		 * XXX We could also check for mul by 1, and -1, etc.
		 */
		if (alter && vmap[val[A_REGNO]].is_const
		    && vmap[val[A_REGNO]].const_val == 0) {
			if (s->code == AddXOp ||
			    s->code == OrXOp ||
			    s->code == LshXOp ||
			    s->code == RshXOp) {
				s->code = TxaOp;
				vstore(s, &val[A_REGNO], val[X_REGNO], alter);
				break;
			}
			else if (s->code == MulXOp ||
				 s->code == DivXOp ||
				 s->code == AndXOp) {
				s->code = LdIOp;
				s->k = 0;
				vstore(s, &val[A_REGNO], K(s->k), alter);
				break;
			}
			else if (s->code == NegOp) {
				s->code = NopOp;
				break;
			}
		}
		val[A_REGNO] = F(s->code, val[A_REGNO], val[X_REGNO]);
		break;

	case TxaOp:
		vstore(s, &val[A_REGNO], val[X_REGNO], alter);
		break;

	case LdmOp:
		v = val[s->k];
		if (alter && vmap[v].is_const) {
			s->code = LdIOp;
			s->k = vmap[v].const_val;
			done = 0;
		}
		vstore(s, &val[A_REGNO], v, alter);
		break;
		
	case TaxOp:
		vstore(s, &val[X_REGNO], val[A_REGNO], alter);
		break;

	case LdmXOp:
		v = val[s->k];
		if (alter && vmap[v].is_const) {
			s->code = LdXIOp;
			s->k = vmap[v].const_val;
			done = 0;
		}
		vstore(s, &val[X_REGNO], v, alter);
		break;

	case StmOp:
		vstore(s, &val[s->k], val[A_REGNO], alter);
		break;

	case StmXOp:
		vstore(s, &val[s->k], val[X_REGNO], alter);
		break;
	}
}

/*
 * Allocation for blist nodes is done cheaply by mallocing the
 * max we will need before the optimization passes.  There is a 
 * linear bound on the number required.
 */
static struct blist *blist_base;
static struct blist *next_blist;

static void
opt_deadstores(b)
	struct block *b;
{
	struct slist *s;
	int i, regno;
	struct stmt *last[N_MEMWORDS];

	bzero((char *)last, sizeof last);

	for (s = b->stmts; s; s = s->next) {
		regno = reguse(&s->s);
		if (regno >= 0) {
			if (regno == AX_REGNO) {
				last[X_REGNO] = 0;
				last[A_REGNO] = 0;
			}
			else
				last[regno] = 0;
		}
		regno = regdef(&s->s);
		if (regno >= 0) {
			if (last[regno]) {
				done = 0;
				last[regno]->code = NopOp;
			}
			last[regno] = &s->s;
		}
	}
	for (i = 0; i < N_MEMWORDS; ++i)
		if (last[i] && !REGELEM(b->out_use, i) && i != A_REGNO) {
			last[i]->code = NopOp;
			done = 0;
		}
}

static void
opt_blk(b, do_stmts)
	struct block *b;
{
	struct slist *s;
	struct blist *p;
	int i;
	long aval;

	/* 
	 * Initialize the register values.
	 * If we have no predecessors, everything is undefined.
	 * Otherwise, we inherent our values from our predecessors.  
	 * If any register has an ambiguous value (i.e. control paths are
	 * merging) give it the undefined value of 0.
	 */
	p = b->pred;
	if (p == 0)
		bzero((char *)b->val, sizeof(b->val));
	else {
		bcopy((char *)p->blk->val, (char *)b->val, sizeof(b->val));
		while (p = p->next) {
			for (i = 0; i < N_MEMWORDS; ++i)
				if (b->val[i] != p->blk->val[i])
					b->val[i] = 0;
		}
	}
	aval = b->val[A_REGNO];
	for (s = b->stmts; s; s = s->next)
		opt_stmt(&s->s, b->val, do_stmts);

	/*
	 * This is a special case: if we don't use anything from this
	 * block, and we load the accumulator with value that is 
	 * already there, eliminate all the statements.
	 */
	if (do_stmts && b->out_use == 0 && aval != 0 &&
	    b->val[A_REGNO] == aval)
		b->stmts = 0;
	else {
		opt_peep(b);
		opt_deadstores(b);
	}
}

/*
 * Return true if any register that is used on exit from 'succ', has
 * an exit value that is different from the corresponding exit value
 * from 'b'.
 */
static int 
use_conflict(b, succ)
	struct block *b, *succ;
{
	int regno;
	regset uset = succ->out_use;

	if (uset == 0)
		return 0;

	for (regno = 0; regno < N_MEMWORDS; ++regno)
		if (REGELEM(uset, regno))
			if (b->val[regno] != succ->val[regno])
				return 1;
	return 0;
}

static void
opt_j(b, which)
	struct block *b;
	int which;
{
	int k;
	struct block **br;
	struct block *target;
	struct block *ancestor;
	struct block *next;
	nodeset dom, closure;
	int sense;

	/*
	 * Don't bother checking if b is a leaf, since we 
	 * are never called that way.
	 */
	if (which) {
		br = &b->jt;
		target = b->jt;
	} else {
		br = &b->jf;
		target = b->jf;
	}
	dom = b->dom;
	closure = b->closure;
 top:
	/*
	 * If we hit a leaf, stop.  Otherwise, 
	 * try to move down another level.
	 */
	if (target->jt == 0) {
		*br = target;
		return;
	}
	if (target->jt == target->jf) {
		/*
		 * Common branch targets can be eliminated, provided
		 * there is no a data dependency.
		 */
		if (!use_conflict(b, target->jt)) {
			done = 0;
			target = target->jt;
			goto top;
		} else {
			*br = target;
			return;
		}
	}
	for (k = 0; k < n_blocks; ++k) {
		if (!NS_MEMBER(dom, k))
			continue;

		ancestor = blocks[k];
		if (target->val[A_REGNO] != ancestor->val[A_REGNO])
			/*
			 * Ignore blocks that do not load the same values.
			 */
			continue;
			
		if (ancestor == b)
			/*
			 * The ancestor happens to be the node whose edge
			 * is being moved (i.e. there are two consective 
			 * nodes that compute the same thing).  The sense
			 * of this branch is given by the branch of the
			 * node we are optimizing.
			 */
			sense = which;

		else if (NS_MEMBER(closure, ancestor->jt->id))
			/*
			 * We can reach the node we are optimizing
			 * along the true branch of the ancestor.
			 * Either we know we came down the true branch,
			 * or we might also be able to reach this node
			 * from the false branch.  In the latter case,
			 * we cannot do anything.
			 */
			if (NS_MEMBER(closure, ancestor->jf->id))
				continue;
			else
				sense = 1;

		else if (NS_MEMBER(closure, ancestor->jf->id))
			/*
			 * The false branch must have been taken.
			 * (We ruled out the true branch above.)
			 */
			sense = 0;
		else
			/* XXX
			 * If we cannot reach the current node from its
			 * dominator, the code is broken.  We can eliminate
			 * the test above.
			 */
			abort();
		
		if (target->s.code == ancestor->s.code) {
			if (target->s.code != EQOp)
				continue;

			if (ancestor->s.k == target->s.k)
				/*
				 * The operands are the same, so the 
				 * result is true if a true branch was
				 * taken to get here, otherwise false.
				 */
				next = sense ? target->jt : target->jf;
			else {
				if (!sense)
					/* 
					 * We do not know the outcome
					 * if we had a false comparison
					 * and the operands are 
					 * different.
					 */
					continue;
				/*
				 * If this is a true comparison but the
				 * operands were different, then the
				 * current must be false.
				 */
				next = target->jf;
			}
			if (use_conflict(b, next))
				/*
				 * There is a data dependency between that
				 * will be invalidated if we move this edge.
				 */
				continue;
			target = next;
			/*
			 * Moved a branch, so do another pass.
			 */
			done = 0;
			goto top;
		}
	}
	/*
	 * Went through all the dominators of the last block
	 * but couldn't move down.
	 */
	*br = target;
}


static void
or_pullup(b)
	struct block *b;
{
	int val, at_top;
	struct block *pull;
	struct block **diffp, **samep;
	struct blist *plist;

	plist = b->pred;
	if (plist == 0)
		return;

	/*
	 * Make sure each predecessor loads the same value.
	 */
	val = plist->blk->val[A_REGNO];
	for (plist = plist->next; plist; plist = plist->next)
		if (val != plist->blk->val[A_REGNO])
			return;

	if (b->pred->blk->jt == b)
		diffp = &b->pred->blk->jt;
	else
		diffp = &b->pred->blk->jf;

	at_top = 1;
	while (1) {
		if (*diffp == 0)
			return;

		if ((*diffp)->jt != b->jt)
			return;

		if (!NS_MEMBER((*diffp)->dom, b->id))
			return;

		if ((*diffp)->val[A_REGNO] != val)
			break;

		diffp = &(*diffp)->jf;
		at_top = 0;
	}
	samep = &(*diffp)->jf;
	while (1) {
		if (*samep == 0)
			return;

		if ((*samep)->jt != b->jt)
			return;

		if (!NS_MEMBER((*samep)->dom, b->id))
			return;

		if ((*samep)->val[A_REGNO] == val)
			break;

		/* XXX Need to check that there are no data dependencies
		   between dp0 and dp1.  Currently, the code generator
		   will not produce such dependencies. */
		samep = &(*samep)->jf;
	}
#ifdef notdef
	/* XXX This doesn't cover everything. */
	for (i = 0; i < N_MEMWORDS; ++i)
		if ((*samep)->val[i] != pred->val[i])
			return;
#endif
	/* Pull up the node. */
	pull = *samep;
	*samep = pull->jf;
	pull->jf = *diffp;
	
	/*
	 * At the top of the chain, each predecessor needs to point at the
	 * pulled up node.  Inside the chain, there is only one predecessor
	 * to worry about.
	 */
	if (at_top) {
		for (plist = b->pred; plist; plist = plist->next) {
			if (plist->blk->jt == b)
				plist->blk->jt = pull;
			else
				plist->blk->jf = pull;
		}
	}
	else
		*diffp = pull;

	done = 0;
}
	
static void
and_pullup(b)
	struct block *b;
{
	int val, at_top;
	struct block *pull;
	struct block **diffp, **samep;
	struct blist *plist;

	plist = b->pred;
	if (plist == 0)
		return;

	/*
	 * Make sure each predecessor loads the same value.
	 */
	val = plist->blk->val[A_REGNO];
	for (plist = plist->next; plist; plist = plist->next)
		if (val != plist->blk->val[A_REGNO])
			return;

	if (b->pred->blk->jt == b)
		diffp = &b->pred->blk->jt;
	else
		diffp = &b->pred->blk->jf;

	at_top = 1;
	while (1) {
		if (*diffp == 0)
			return;

		if ((*diffp)->jf != b->jf)
			return;

		if (!NS_MEMBER((*diffp)->dom, b->id))
			return;

		if ((*diffp)->val[A_REGNO] != val)
			break;

		diffp = &(*diffp)->jt;
		at_top = 0;
	}
	samep = &(*diffp)->jt;
	while (1) {
		if (*samep == 0)
			return;

		if ((*samep)->jf != b->jf)
			return;

		if (!NS_MEMBER((*samep)->dom, b->id))
			return;

		if ((*samep)->val[A_REGNO] == val)
			break;

		/* XXX Need to check that there are no data dependencies
		   between dp0 and dp1.  Currently, the code generator
		   will not produce such dependencies. */
		samep = &(*samep)->jt;
	}
#ifdef notdef
	/* XXX This doesn't cover everything. */
	for (i = 0; i < N_MEMWORDS; ++i)
		if ((*samep)->val[i] != pred->val[i])
			return;
#endif
	/* Pull up the node. */
	pull = *samep;
	*samep = pull->jt;
	pull->jt = *diffp;
	
	/*
	 * At the top of the chain, each predecessor needs to point at the
	 * pulled up node.  Inside the chain, there is only one predecessor
	 * to worry about.
	 */
	if (at_top) {
		for (plist = b->pred; plist; plist = plist->next) {
			if (plist->blk->jt == b)
				plist->blk->jt = pull;
			else
				plist->blk->jf = pull;
		}
	}
	else
		*diffp = pull;

	done = 0;
}

static void
opt_blks(root, do_stmts)
	struct block *root;
{
	int i, maxlevel;
	struct block *p;

	init_val();
	maxlevel = root->level;
	for (i = maxlevel; i >= 0; --i)
		for (p = levels[i]; p; p = p->link)
			opt_blk(p, do_stmts);

	if (do_stmts)
		/* 
		 * No point trying to move branches; it can't possibly
		 * make a difference at this point.
		 */
		return;

	for (i = 1; i <= maxlevel; ++i) {
		for (p = levels[i]; p; p = p->link) {
			opt_j(p, 1);
			opt_j(p, 0);
		}
	}
	for (i = 1; i <= maxlevel; ++i)
		for (p = levels[i]; p; p = p->link) {
			or_pullup(p);
			and_pullup(p);
		}
}

static void
find_pred(root)
	struct block *root;
{
	int i;
	struct block *b;
	struct blist *list;

	next_blist = blist_base;

	for (i = 0; i < n_blocks; ++i)
		blocks[i]->pred = 0;

	/*
	 * Traverse the graph, adding each node to the predecessor
	 * list of its sucessors.  Skip the leaves (i.e. level 0).
	 */
	for (i = root->level; i > 0; --i) {
		for (b = levels[i]; b; b = b->link) {
			list = next_blist++;
			list->blk = b;
			list->next = b->jt->pred;
			b->jt->pred = list;
			list = next_blist++;
			list->blk = b;
			list->next = b->jf->pred;
			b->jf->pred = list;
		}
	}
}

static void
opt_root(b)
	struct block **b;
{
	while (BPF_ISJUMP((*b)->s.code) &&
	       (*b)->jt == (*b)->jf) {
		*b = (*b)->jt;
	}
}

static void
opt_loop(root, do_stmts)
	struct block *root;
	int do_stmts;
{
	do {
		done = 1;

		find_levels(root);
		find_dom(root);
		find_closure(root);
		find_pred(root);
		find_ud(root);

		opt_blks(root, do_stmts);

	} while (!done);
}

/*
 * Optimize the filter code in its dag representation.
 */
void
optimize(rootp)
	struct block **rootp;
{
	struct block *root;

	root = *rootp;

	opt_init(root);
	opt_loop(root, 0);
	opt_loop(root, 1);
	intern_blocks(root);
	opt_root(rootp);
	opt_cleanup();
}

static void
make_marks(p)
	struct block *p;
{
	if (!isMarked(p)) {
		Mark(p);
		if (!BPF_ISLEAF(p->s.code)) {
			make_marks(p->jt);
			make_marks(p->jf);
		}
	}
}

/*
 * Mark code array such that isMarked(i) is true
 * only for nodes that are alive.
 */
static void
mark_code(p)
	struct block *p;
{
	cur_mark += 1;
	make_marks(p);
}

/*
 * True iff the two stmt lists load the same value from the packet into
 * the accumulator.
 */
static int
eq_slist(x, y)
	struct slist *x, *y;
{
	while (1) {
		while (x && x->s.code == NopOp)
			x = x->next;
		while (y && y->s.code == NopOp)
			y = y->next;
		if (x == 0)
			return y == 0;
		if (y == 0)
			return x == 0;
		if (x->s.code != y->s.code ||
		    x->s.k != y->s.k)
			return 0;
		x = x->next;
		y = y->next;
	}
}

static inline int
eq_blk(b0, b1)
	struct block *b0, *b1;
{
	if (b0->s.code == b1->s.code &&
	    b0->s.k == b1->s.k &&
	    b0->jt == b1->jt && b0->jf == b1->jf)
		return eq_slist(b0->stmts, b1->stmts);
	return 0;
}

static void
intern_blocks(root)
	struct block *root;
{
	struct block *p;
	int i, j;
	int done;
 top:
	done = 1;
	for (i = 0; i < n_blocks; ++i)
		blocks[i]->link = 0;

	mark_code(root);

	for (i = n_blocks - 1; --i >= 0; ) {
		if (!isMarked(blocks[i]))
			continue;
		for (j = i + 1; j < n_blocks; ++j) {
			if (!isMarked(blocks[j]))
				continue;
			if (eq_blk(blocks[i], blocks[j])) {
				blocks[i]->link = blocks[j]->link ?
					blocks[j]->link : blocks[j];
				break;
			}
		}
	}
	for (i = 0; i < n_blocks; ++i) {
		p = blocks[i];
		if (p->jt == 0)
			continue;
		if (p->jt->link) {
			done = 0;
			p->jt = p->jt->link;
		}
		if (p->jf->link) {
			done = 0;
			p->jf = p->jf->link;
		}
	}
	if (!done)
		goto top;
}

static void
opt_cleanup()
{
	free((void *)vnode_base);
	free((void *)vmap);
	free((void *)blist_base);
	free((void *)space);
	free((void *)levels);
	free((void *)blocks);
}

/*
 * Return the number of stmts in 's'.
 */
static int
slength(s)
	struct slist *s;
{
	int n = 0;

	for (; s; s = s->next)
		if (s->s.code != NopOp)
			++n;
	return n;
}

/*
 * Return the number of nodes reachable by 'p'.
 * All nodes should be initially unmarked.
 */
static int
count_blocks(p)
	struct block *p;
{
	if (p == 0 || isMarked(p))
		return 0;
	Mark(p);
	return count_blocks(p->jt) + count_blocks(p->jf) + 1;
}	

/*
 * Do a depth first search on the flow graph, numbering the
 * the basic blocks, and entering them into the 'blocks' array.`
 */
static void
number_blks_r(p)
	struct block *p;
{
	int n;

	if (p == 0 || isMarked(p))
		return;

	Mark(p);
	n = n_blocks++;
	p->id = n;
	blocks[n] = p;

	number_blks_r(p->jt);
	number_blks_r(p->jf);
}

/*
 * Return the number of stmts in the flowgraph reachable by 'p'.
 * The nodes should be unmarked before calling.
 */
static int
count_stmts(p)
	struct block *p;
{
	int n;

	if (p == 0 || isMarked(p))
		return 0;
	Mark(p);
	n = count_stmts(p->jt) + count_stmts(p->jf);
	return slength(p->stmts) + n + 1;
}

/*
 * Allocate memory.  All allocation is done before optimization
 * is begun.  A linear bound on the size of all data structures is computed
 * from the total number of blocks and/or statements.
 */
static void
opt_init(root)
	struct block *root;
{
	u_long *p;
	int i, n, max_stmts;

	/*
	 * First, count the blocks, so we can malloc an array to map
	 * block number to block.  Then, put the blocks into the array.
	 */
	unMarkAll();
	n = count_blocks(root);
	blocks = (struct block **)malloc(n * sizeof(*blocks));
	unMarkAll();
	n_blocks = 0;
	number_blks_r(root);

	/*
	 * The number of levels is bounded by the number of nodes.
	 */
	levels = (struct block **)malloc(n * sizeof(*levels));

	ns_nwords = n / (8 * sizeof(u_long)) + 1;
	space = (u_long *)malloc(2 * n * ns_nwords * sizeof(*space));
	p = space;
	all_dom_sets = p;
	for (i = 0; i < n; ++i) {
		blocks[i]->dom = p;
		p += ns_nwords;
	}
	all_closure_sets = p;
	for (i = 0; i < n; ++i) {
		blocks[i]->closure = p;
		p += ns_nwords;
	}
	/*
	 * At most, we can have E = 2N predecessors. 
	 * (E = num edges, N = num blocks)
	 */
	blist_base = (struct blist *)malloc(2 * n * sizeof(*blist_base));

	max_stmts = 0;
	for (i = 0; i < n; ++i)
		max_stmts += slength(blocks[i]->stmts) + 1;
	/*
	 * We allocate at most 3 value numbers per statement,
	 * so this is an upper bound on the number of valnodes
	 * we'll need.
	 */
	maxval = 3 * max_stmts;
	vmap = (struct vmapinfo *)malloc(maxval * sizeof(*vmap));
	vnode_base = (struct valnode *)malloc(maxval * sizeof(*vmap));
}

/*
 * Some pointers used to convert the basic block form of the code,
 * into the array form that BPF requires.  'fstart' will point to
 * the malloc'd array while 'ftail' is used during the recursive traversal.
 */
static struct bpf_insn *fstart;
static struct bpf_insn *ftail;

#ifdef BDEBUG
int bids[1000];
#endif

static void
convert_code_r(p)
	struct block *p;
{
	struct bpf_insn *dst;
	struct slist *src;
	int slen;
	u_int off;

	if (p == 0 || isMarked(p))
		return;
	Mark(p);

	convert_code_r(p->jf);
	convert_code_r(p->jt);

	slen = slength(p->stmts);
	dst = ftail -= slen + 1;

	p->offset = dst - fstart;

	for (src = p->stmts; src; src = src->next) {
		if (src->s.code == NopOp)
			continue;
		dst->code = (u_short)src->s.code;
		dst->k = src->s.k;
		++dst;
	}
#ifdef BDEBUG
	bids[dst - fstart] = p->id + 1;
#endif
	dst->code = (u_short)p->s.code;
	dst->k = p->s.k;
	if (p->jt) {
		off = p->jt->offset - (p->offset + slen);
		if (off >= 256) 
			error("long jumps not supported");
		dst->jt = off;
		off = p->jf->offset - (p->offset + slen);
		if (off >= 256) 
			error("long jumps not supported");
		dst->jf = off;
	}
}
	

/*
 * Convert flowgraph intermediate representation to the
 * BPF array representation.  Set *lenp to the number of instructions.
 */
struct bpf_insn *
icode_to_fcode(root, lenp)
	struct block *root;
	int *lenp;
{
	int n;
	struct bpf_insn *fp;
	
	unMarkAll();
	n = *lenp = count_stmts(root);

	fp = (struct bpf_insn *)malloc(sizeof(*fp) * n);
	bzero((char *)fp, sizeof(*fp) * n);
	fstart = fp;
	ftail = fp + n;

	unMarkAll();
	convert_code_r(root);

	return fp;
}

#ifdef BDEBUG
opt_dump(root)
	struct block *root;
{
	struct bpf_program f;
	
	f.bf_insns = icode_to_fcode(root, &f.bf_len);
	bpf_dump(&f, 1);
	putchar('\n');
	free((char *)f.bf_insns);
}
#endif
