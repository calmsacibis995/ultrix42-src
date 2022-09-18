/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: opnames.c,v 2010.2.1.3 89/11/29 14:30:02 bettina Exp $ */

char *op_name[64] = {
  "spec",  "bcond", "j",     "jal",   "beq",   "bne",   "blez",  "bgtz",
  "addi",  "addiu", "slti",  "sltiu", "andi",  "ori",   "xori",  "lui",
  "cop0",  "cop1",  "cop2",  "cop3",  "beql",  "bnel",  "blezl", "bgtzl",
  "op18",  "op19",  "op1a",  "op1b",  "op1c",  "op1d",  "op1e",  "op1f",
  "lb",    "lh",    "lwl",   "lw",    "lbu",   "lhu",   "lwr",   "ld",
  "sb",    "sh",    "swl",   "sw",    "op2c",  "op2d",  "swr",   "sd",
  "ll",    "lwc1",  "lwc2",  "lwc3",  "op34",  "ldc1",  "ldc2",  "ldc3",
  "sc",    "swc1",  "swc2",  "swc3",  "op3c",  "sdc1",  "sdc2",  "sdc3" };

char *spec_name[64] = {
  "sll",   "spec01","srl",   "sra",   "sllv",  "spec05","srlv",  "srav",
  "jr",    "jalr",  "spec0c","spec0d","syscall","break","spim",  "spec0f",
  "mfhi",  "mthi",  "mflo",  "mtlo",  "spec14","spec15","spec16","spec17",
  "mult",  "multu", "div",   "divu",  "spec1c","spec1d","spec1e","spec1f",
  "add",   "addu",  "sub",   "subu",  "and",   "or",    "xor",   "nor",
  "spec28","spec29","slt",   "sltu",  "spec2c","spec2d","spec2e","spec2f",
  "tge",   "tgeu",  "tlt",   "tltu",  "teq",   "spec35","tne",   "spec37",
  "spec38","spec39","spec3a","spec3b","spec3c","spec3d","spec3e","spec3f" };

char *bcond_name[32] = {
  "bltz",    "bgez",    "bltzl",   "bgezl",
  "spimi",   "bcond05", "bcond06", "bcond07",
  "tgei",    "tgeiu",   "tlti",    "tltiu",
  "teqi",    "bcond0d", "tnei",    "bcond0f",
  "bltzal",  "bgezal",  "bltzall", "bgezall",
  "bcond14", "bcond15", "bcond16", "bcond17",
  "bcond18", "bcond19", "bcond1a", "bcond1b",
  "bcond1c", "bcond1d", "bcond1e", "bcond1f" };

char *cop1func_name[64] = {
  "add",   "sub",   "mul",   "div",   "sqrt",  "abs",   "mov",   "neg",
  "fop08", "fop09", "fop0a", "fop0b", "round.w","trunc.w","ceil.w","floor.w",
  "fop10", "fop11", "fop12", "fop13", "fop14", "fop15",	"fop16", "fop17",
  "fop18", "fop19", "fop1a", "fop1b", "fop1c", "fop1d",	"fop1e", "fop1f",
  "cvt.s", "cvt.d", "cvt.e", "fop23", "cvt.w", "fop25",	"fop26", "fop27",
  "fop28", "fop29", "fop2a", "fop2b", "fop2c", "fop2d",	"fop2e", "fop2f",
  "c.f",   "c.un",  "c.eq",  "c.ueq", "c.olt", "c.ult",	"c.ole", "c.ule",
  "c.sf",  "c.ngle","c.seq", "c.ngl", "c.lt",  "c.nge",	"c.le",  "c.ngt" };

char *bc_name[32] = {
  "f",   "t",   "fl",  "tl",
  "x04", "x05", "x06", "x07",
  "x08", "x09", "x0a", "x0b",
  "x0c", "x0d", "x0e", "x0f",
  "x10", "x11", "x12", "x13",
  "x14", "x15", "x16", "x17",
  "x18", "x19", "x1a", "x1b",
  "x1c", "x1d", "x1e", "x1f" };

char *c0func_name[64] = {
  "op00", "tlbr", "tlbwi","op03", "op04", "op05", "tlbwr","op07",
  "tlbp", "op9",  "op10", "op11", "op12", "op13", "op14", "op15",
  "rfe",  "op17", "op18", "op19", "op20", "op21", "op22", "op23",
  "op24", "op25", "op26", "op27", "op28", "op29", "op30", "op31",
  "op32", "op33", "op34", "op35", "op36", "op37", "op38", "op39",
  "op40", "op41", "op42", "op43", "op44", "op45", "op46", "op47",
  "op48", "op49", "op50", "op51", "op52", "op53", "op54", "op55",
  "op56", "op57", "op58", "op59", "op60", "op61", "op62", "op63"
};

char *c0reg_name[32] = {
  "index","random","tlblo","c0r3","context","c0r5","c0r6","c0r7",
  "badvaddr","c0r9","tlbhi","c0r11","sr", "cause","epc",  "c0r15",
  "c0r16","c0r17","c0r18","c0r19","c0r20","c0r21","c0r22","c0r23",
  "c0r24","c0r25","c0r26","c0r27","c0r28","c0r29","c0r30","c0r31"
};
