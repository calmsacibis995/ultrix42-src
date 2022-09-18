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
 * SCCSID: @(#)bpf_image.c	4.1	ULTRIX	1/25/91
 * Based on:
 * rcsid[] = "@(#) $Header: bpf_image.c,v 1.9 91/01/08 14:24:27 mccanne Exp $ (LBL)"
 */

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <net/bpf.h>

static char *opstrings[] = {
#define OPDEF(opcode, opstr) opstr , 
#include <net/bpfcodes.h>
#undef OPDEF
};

char *
bpf_image(p, n)
	struct bpf_insn *p;
	int n;
{
	static char image[256];
	char operand[64];

	switch (p->code) {
	case LdxmsOp:
	case LdOp:
	case LdHOp:
	case LdBOp:

	case StmOp:
	case LdmOp:
	case StmXOp:
	case LdmXOp:
		(void)sprintf(operand, "[%d]", p->k);
		break;

	case ILdOp:
	case ILdHOp:
	case ILdBOp:
		(void)sprintf(operand, "[x+%d]", p->k);
		break;
		
	case AddIOp:
	case SubIOp:
	case MulIOp:
	case DivIOp:
	case AndIOp:
	case OrIOp:
	case LshIOp:
	case RshIOp:
	case LdIOp:
	case LdXIOp:
	case GEOp:
	case GTOp:
	case EQOp:
	case RetOp:
		(void)sprintf(operand, "#0x%x", p->k);
		break;

	default:
		*operand = '\0';
		break;
	}
	(void)sprintf(image, BPF_ISJUMP(p->code) ? 
		      "(%03d) %-8s %-16s jt %d\tjf %d" : "(%03d) %-8s %s",
		      n, BPF_VALIDCODE(p->code) ? opstrings[p->code] : "?",
		      operand, n + p->jt, n + p->jf);

	return image;
}
