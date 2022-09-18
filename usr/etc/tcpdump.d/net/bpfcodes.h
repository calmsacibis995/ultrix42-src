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
 * SCCSID: @(#)bpfcodes.h	4.1	ULTRIX	1/25/91
 * Based on:
 * @(#) $Header: bpfcodes.h,v 1.4 91/01/08 14:22:43 mccanne Exp $ (LBL)
 */


/*
 * BPF opcode definitions.
 *
 * Define the macro OPDEF(code, str) to enumerate the desired field.
 */

/*
 * The jump instructions must come first.
 */
OPDEF(GEOp, "ge")
OPDEF(GTOp, "gt")
OPDEF(EQOp, "eq")

/*
 * These are ordered such that
 *	(ILdOp - LdOp) == (ILdHOp - LdHOp) == (ILdBOp - LdBOp)
 */
OPDEF(LdOp, "ld")
OPDEF(LdHOp, "ldh")
OPDEF(LdBOp, "ldb")
OPDEF(ILdOp, "ild")
OPDEF(ILdHOp, "ildh")
OPDEF(ILdBOp, "ildb")

OPDEF(LdLenOp, "ldlen")
OPDEF(LdIOp, "ldi")

OPDEF(LdXIOp, "ldxi")
OPDEF(LdxmsOp, "ldxms")

/*
 * These are ordered symetrically too.
 */
OPDEF(AddIOp, "addi")
OPDEF(SubIOp, "subi")
OPDEF(MulIOp, "muli")
OPDEF(DivIOp, "divi")
OPDEF(AndIOp, "andi")
OPDEF(OrIOp, "ori")
OPDEF(LshIOp, "lshi")
OPDEF(RshIOp, "rshi")

OPDEF(AddXOp, "addx")
OPDEF(SubXOp, "subx")
OPDEF(MulXOp, "mulx")
OPDEF(DivXOp, "divx")
OPDEF(AndXOp, "andx")
OPDEF(OrXOp, "orx")
OPDEF(LshXOp, "lshx")
OPDEF(RshXOp, "rshx")
OPDEF(NegOp, "neg")

OPDEF(TxaOp, "txa")
OPDEF(TaxOp, "tax")

OPDEF(LdmOp, "ldm")
OPDEF(LdmXOp, "ldmx")
OPDEF(StmOp, "stm")
OPDEF(StmXOp, "stmx")

OPDEF(NopOp, "nop")
OPDEF(RetOp, "ret")
OPDEF(RetAOp, "reta")
