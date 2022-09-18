/*
 * Copyright (c) 1990 The Regents of the University of California.
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
 * SCCSID: @(#)gencode.h	4.1	ULTRIX	1/25/91
 * Based on:
 * @(#) $Header: gencode.h,v 1.6 91/01/07 16:02:58 mccanne Exp $ (LBL)
 */

/*
 * filter.h must be included before this file.
 */

/* Primary qualifers. */

#define Q_HOST		1
#define Q_NET		2
#define Q_PORT		3
#define Q_GATEWAY	4
#define Q_PROTO		5

/* Protocol qualifiers. */

#define Q_ETHER		1
#define Q_IP		2
#define Q_ARP		3
#define Q_RARP		4
#define Q_TCP		5
#define Q_UDP		6

/* Directional qualifers. */

#define Q_SRC		1
#define Q_DST		2
#define Q_OR		3
#define Q_AND		4

#define Q_DEFAULT	0
#define Q_UNDEF		((unsigned)-1)

struct stmt {
	enum bpf_code code;
	long k;
};

struct slist {
	struct stmt s;
	struct slist *next;
};

struct blist {
	struct block *blk;
	struct blist *next;
};

/* 
 * A bit vector to represent definition sets.  We assume TOT_REGISTERS
 * is smaller than 8*sizeof(regset).
 */
typedef u_long regset;
#define REGMASK(n) (1 << (n))
#define REGELEM(d, n) (d & REGMASK(n))

typedef u_long *nodeset;

/*
 * Total number of registers, including accumulator (A) and index (X).
 * We treat all these guys similarly during flow analysis.
 */
#define N_MEMWORDS (BPF_MEMWORDS+2)

struct block {
	int id;
	struct slist *stmts;	/* side effect stmts */
	struct stmt s;		/* branch stmt */
	int mark;
	int level;
	int offset;
	int sense;
	int n_inedges;
	struct block *jt;
	struct block *jf;
	struct block *head;
	struct block *link;	/* link field used by optimizer */
	nodeset dom;
	nodeset closure;
	struct blist *pred;
	regset def;
	regset in_use;
	regset out_use;
	long val[N_MEMWORDS];
};

struct arth {
	struct block *b;	/* protocol checks */
	struct slist *s;	/* stmt list */
	int regno;		/* virtual register number of result */
};

extern struct arth *gen_loadi();
extern struct arth *gen_load();
extern struct arth *gen_loadlen();
extern struct arth *gen_neg();
extern struct arth *gen_arth();

extern void gen_and();
extern void gen_or();
extern void gen_not();

extern struct block *gen_scode();
extern struct block *gen_ecode();
extern struct block *gen_ncode();
extern struct block *gen_proto_abbrev();
extern struct block *gen_relation();
extern struct block *gen_less();
extern struct block *gen_greater();
extern struct block *gen_byteop();
extern struct block *gen_broadcast();

extern void optimize();

extern void finish_parse();

struct qual {
	unsigned primary : 8;
	unsigned protocol : 8;
	unsigned dir : 8;
	unsigned xxx : 8;
};

