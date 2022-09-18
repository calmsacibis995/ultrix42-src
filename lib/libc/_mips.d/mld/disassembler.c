/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: disassembler.c,v 2010.2.1.3 89/11/29 14:29:53 bettina Exp $ */

/*
 * Copyright 1985 by, 1989 MIPS Computer Systems, Inc.
 */

/* MIPS instruction disassembler, callable from either Pascal or C */

#include <stdio.h>
#include "syntax.h"
#ifndef SYSV
#	include <machine/inst.h>
#else
#	include <sys/inst.h>
#endif
#ifndef ftrunc_op
#define ftrunc_op	0x0c
#define fround_op	0x0d
#define ffloor_op	0x0e
#define fceil_op	0x0f
#endif
#include <opnames.h>
#include <disassembler.h>

/* register definitions */
#define ZERO 0

private char *c1fmt_name[16] = {
	"s",	"d",	"e",	"q",
	"w",	"fmt5",	"fmt6",	"fmt7",
	"fmt8",	"fmt9",	"fmta",	"fmtb",
	"fmtc",	"fmtd",	"fmte",	"fmtf"
};

/* public */
/* Three sets of commonly used register names */
/* const */ char *dis_reg_names[3][32] = {
	{	/* compiler names */
		"zero",	"at",	"v0",	"v1",	"a0",	"a1",	"a2",	"a3",
		"t0",	"t1",	"t2",	"t3",	"t4",	"t5",	"t6",	"t7",
		"s0",	"s1",	"s2",	"s3",	"s4",	"s5",	"s6",	"s7",
		"t8",	"t9",	"k0",	"k1",	"gp",	"sp",	"s8",	"ra"
	},
	{	/* hardware names */
		"r0",	"r1",	"r2",	"r3",	"r4",	"r5",	"r6",	"r7",
		"r8",	"r9",	"r10",	"r11",	"r12",	"r13",	"r14",	"r15",
		"r16",	"r17",	"r18",	"r19",	"r20",	"r21",	"r22",	"r23",
		"r24",	"r25",	"r26",	"r27",	"gp",	"sp",	"r30",	"r31"
	},
	{	/* assembler names */
		"$0",	"$at",	"$2",	"$3",	"$4",	"$5",	"$6",	"$7",
		"$8",	"$9",	"$10",	"$11",	"$12",	"$13",	"$14",	"$15",
		"$16",	"$17",	"$18",	"$19",	"$20",	"$21",	"$22",	"$23",
		"$24",	"$25",	"$26",	"$27",	"$gp",	"$sp",	"$30",	"$31"
	}
};

/* Remember the options set by dis_init */
#define ADDR_DEFAULT "%#010x\t"
#define VALUE_DEFAULT "%#010x\t"
#define NAME_DEFAULT COMPILER_NAMES
private struct {
  char *addr_format;
  char *value_format;
  char **reg_names;
  int print_jal_targets;
  } save = {
    ADDR_DEFAULT,
    VALUE_DEFAULT,
    NAME_DEFAULT,
    true
    };

/* public -- see .h file */
void
dis_init(addr_format, value_format, reg_names, print_jal_targets)
  char *addr_format;
  char *value_format;
  char *reg_names[];
  int print_jal_targets;
  {
  if (addr_format) save.addr_format = addr_format;
  else save.addr_format = ADDR_DEFAULT;
  if (value_format) save.value_format = value_format;
  else save.value_format = VALUE_DEFAULT;
  if (reg_names) save.reg_names = reg_names;
  else save.reg_names = NAME_DEFAULT;
  save.print_jal_targets = print_jal_targets;
  }

/* Update regmask to reflect the use of this general-purpose (not fp)
  register, and return its name */
private char *
register_name(ireg, regmask)
  unsigned ireg, *regmask;
  {
  *regmask |= (1 << ireg);
  return save.reg_names[ireg];
  }

private char *
fp_register_name(r)
  unsigned r;
{
  static char *name[32] = {
    "$f0",  "$f1",  "$f2",  "$f3",  "$f4",  "$f5",  "$f6",  "$f7",
    "$f8",  "$f9",  "$f10", "$f11", "$f12", "$f13", "$f14", "$f15",
    "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23",
    "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31" };
  return name[r];
}

private char *
c0_register_name(r)
  unsigned r;
{
  return c0reg_name[r];
}

/* public -- see .h file */
int
disasm(buffer, address, iword, regmask, symbol_value, ls_register)
  char *buffer;
  unsigned address, iword, *regmask, *symbol_value, *ls_register;
{
  int return_value = 0;
  char *bufptr = buffer;
  boolean do_b_displacement = false;
  boolean do_loadstore = false;
  union mips_instruction i;

  i.word = iword;
  *regmask = *symbol_value = *ls_register = 0;

  /* Put out the address and hex value of the instruction,
     leaving bufptr set at the end */
  if (save.addr_format) {
    sprintf(bufptr, save.addr_format, address);
    bufptr += strlen(bufptr);
  }
  if (save.value_format) {
    sprintf(bufptr, save.value_format, iword);
    bufptr += strlen(bufptr);
  }

  switch (i.j_format.opcode) {

  case spec_op:
    if (i.word == 0) {
      strcat(bufptr, "nop");
      bufptr += strlen(bufptr);
      break;
    }
    else if (i.r_format.func == addu_op && i.r_format.rt == ZERO) {
      sprintf(bufptr, "move\t%s,%s",
	      register_name(i.r_format.rd, regmask),
	      register_name(i.r_format.rs, regmask));
      bufptr += strlen(bufptr);
      break;
    }
    strcat(bufptr, spec_name[i.r_format.func]);
    bufptr += strlen(bufptr);

    switch (i.r_format.func) {
    case sll_op:
    case srl_op:
    case sra_op:
      sprintf(bufptr, "\t%s,%s,%d",
	      register_name(i.r_format.rd, regmask),
	      register_name(i.r_format.rt, regmask),
	      i.r_format.re);
      break;
    case sllv_op:
    case srlv_op:
    case srav_op:
      sprintf(bufptr, "\t%s,%s,%s",
	      register_name(i.r_format.rd, regmask),
	      register_name(i.r_format.rt, regmask),
	      register_name(i.r_format.rs, regmask));
      break;
    case mfhi_op:
    case mflo_op:
      sprintf(bufptr, "\t%s",
	      register_name(i.r_format.rd, regmask));
      break;
    case jalr_op:
      return_value = 2;
      sprintf(bufptr, "\t%s,%s",
	      register_name(i.r_format.rd, regmask),
	      register_name(i.r_format.rs, regmask));
      break;
    case jr_op:
      return_value = 2;
      /* fall through */
    case mtlo_op:
    case mthi_op:
      sprintf(bufptr, "\t%s",
	      register_name(i.r_format.rs, regmask));
      break;
    case tge_op:
    case tgeu_op:
    case tlt_op:
    case tltu_op:
    case teq_op:
    case tne_op:
    case mult_op:
    case multu_op:
    case div_op:
    case divu_op:
      sprintf(bufptr, "\t%s,%s",
	      register_name(i.r_format.rs, regmask),
	      register_name(i.r_format.rt, regmask));
      break;
    case syscall_op:
      break;
    case break_op:
      {
	char *format = "\t%d";
	unsigned op2 = i.r_format.rd * 32 + i.r_format.re;
	
	if (op2)
	  format = "\t%d,%d";
	sprintf(bufptr, format, i.r_format.rs*32+i.r_format.rt,
		op2);
      }
      break;
    default:
      sprintf(bufptr, "\t%s,%s,%s",
	      register_name(i.r_format.rd, regmask),
	      register_name(i.r_format.rs, regmask),
	      register_name(i.r_format.rt, regmask));
      break;
    }
    break;
    
  case bcond_op:
    switch (i.i_format.rt) {
    case tgei_op:
    case tgeiu_op:
    case tlti_op:
    case tltiu_op:
    case teqi_op:
    case tnei_op:
      sprintf(bufptr, "%s\t%s,%d",
	      bcond_name[i.i_format.rt],
	      register_name(i.i_format.rs, regmask),
	      i.i_format.simmediate);
      break;
    default:
      sprintf(bufptr, "%s\t%s,",
	      bcond_name[i.i_format.rt],
	      register_name(i.i_format.rs, regmask));
      do_b_displacement = true;
      break;
    }
    break;
  case blez_op:
  case bgtz_op:
  case blezl_op:
  case bgtzl_op:
    sprintf(bufptr, "%s\t%s,", op_name[i.i_format.opcode],
	    register_name(i.i_format.rs, regmask));
    do_b_displacement = true;
    break;
  case beq_op:
    if (i.i_format.rs == ZERO && i.i_format.rt == ZERO) {
      strcat(bufptr, "b\t");
      do_b_displacement = true;
      break;
    }
    /* fall through */
  case bne_op:
  case beql_op:
  case bnel_op:
    sprintf(bufptr, "%s\t%s,%s,", op_name[i.i_format.opcode],
	    register_name(i.i_format.rs, regmask),
	    register_name(i.i_format.rt, regmask));
    do_b_displacement = true;
    break;

  case jal_op:
  case j_op:
    sprintf(bufptr, "%s\t", op_name[i.j_format.opcode]);
    *symbol_value =
      ((address+4)&~((1<<28)-1)) + (i.j_format.target<<2);
    if (save.print_jal_targets) {
      bufptr += strlen(bufptr);
      sprintf(bufptr, "%#x", *symbol_value);
    }
    return_value = 1;
    break;
    
  case swc1_op:
  case sdc1_op:
  case lwc1_op:
  case ldc1_op:
    sprintf(bufptr, "%s\t%s,", op_name[i.i_format.opcode],
	    fp_register_name(i.i_format.rt));
    do_loadstore = true;
    break;

  case swc2_op:
  case sdc2_op:
  case lwc2_op:
  case ldc2_op:
  case swc3_op:
  case sdc3_op:
  case lwc3_op:
  case ldc3_op:
    sprintf(bufptr, "%s\t$%d,", op_name[i.i_format.opcode],
	    i.i_format.rt);
    do_loadstore = true;
    break;
    
  case lb_op:
  case lh_op:
  case lw_op:
  case ld_op:
  case lbu_op:
  case lhu_op:
  case sb_op:
  case sh_op:
  case sw_op:
  case sd_op:
  case swl_op:
  case swr_op:
  case lwl_op:
  case lwr_op:
  case ll_op:
  case sc_op:
    sprintf(bufptr, "%s\t%s,", op_name[i.i_format.opcode],
	    register_name(i.i_format.rt, regmask));
    do_loadstore = true;
    break;
    
  case ori_op:
  case xori_op:
    if (i.u_format.rs == ZERO) {
      sprintf(bufptr, "li\t%s,%d",
	      register_name(i.u_format.rt, regmask),
	      i.u_format.uimmediate);
      break;
    }
    /* fall through */
  case andi_op:
    sprintf(bufptr, "%s\t%s,%s,%#x", op_name[i.u_format.opcode],
	    register_name(i.u_format.rt, regmask),
	    register_name(i.u_format.rs, regmask),
	    i.u_format.uimmediate);
    break;
  case lui_op:
    sprintf(bufptr, "%s\t%s,%#x", op_name[i.u_format.opcode],
	    register_name(i.u_format.rt, regmask),
	    i.u_format.uimmediate);
    break;
  case addi_op:
  case addiu_op:
    if (i.i_format.rs == ZERO) {
      short sign_extender = i.i_format.simmediate;
      sprintf(bufptr, "li\t%s,%d",
	      register_name(i.i_format.rt, regmask),
	      sign_extender);
      break;
    }
    /* fall through */
  default:
    {
      short sign_extender = i.i_format.simmediate;
      sprintf(bufptr, "%s\t%s,%s,%d", op_name[i.i_format.opcode],
	      register_name(i.i_format.rt, regmask),
	      register_name(i.i_format.rs, regmask),
	      sign_extender);
    }
    break;

  case cop0_op:
    switch (i.r_format.rs) {
    case bc_op:
      sprintf(bufptr, "bc0%s\t", bc_name[i.i_format.rt]);
      do_b_displacement = true;
      break;
    case mtc_op:
      sprintf(bufptr, "mtc0\t%s,%s",
	      register_name(i.r_format.rt, regmask),
	      c0_register_name(i.r_format.rd));
      break;
    case mfc_op:
      sprintf(bufptr, "mfc0\t%s,%s",
	      register_name(i.r_format.rt, regmask),
	      c0_register_name(i.r_format.rd));
      break;
    case cfc_op:
      sprintf(bufptr, "cfc0\t%s,$%d",
	      register_name(i.r_format.rt, regmask),
	      i.r_format.rd);
      break;
    case ctc_op:
      sprintf(bufptr, "ctc0\t%s,$%d",
	      register_name(i.r_format.rt, regmask),
	      i.r_format.rd);
      break;
    case cop_op:
      sprintf(bufptr, "c0\t%s", c0func_name[i.r_format.func]);
      break;
    default:
      sprintf(bufptr, "c0rs%d", i.r_format.rs);
      break;
    }
    break;
    
  case cop1_op:
    switch (i.r_format.rs) {
    case bc_op:
      sprintf(bufptr, "bc1%s\t", bc_name[i.i_format.rt]);
      do_b_displacement = true;
      break;
    case mtc_op:
      sprintf(bufptr, "mtc1\t%s,%s",
	      register_name(i.r_format.rt, regmask),
	      fp_register_name(i.r_format.rd));
      break;
    case mfc_op:
      sprintf(bufptr, "mfc1\t%s,%s",
	      register_name(i.r_format.rt, regmask),
	      fp_register_name(i.r_format.rd));
      break;
    case cfc_op:
      sprintf(bufptr, "cfc1\t%s,$%d",
	      register_name(i.r_format.rt, regmask),
	      i.r_format.rd);
      break;
    case ctc_op:
      sprintf(bufptr, "ctc1\t%s,$%d",
	      register_name(i.r_format.rt, regmask),
	      i.r_format.rd);
      break;
    case cop_op+s_fmt:
    case cop_op+d_fmt:
    case cop_op+e_fmt:
    case cop_op+w_fmt:
      sprintf(bufptr, "%s.%s\t",
	      cop1func_name[i.r_format.func],
	      c1fmt_name[i.r_format.rs - cop_op]);
      bufptr += strlen(bufptr);
      switch (i.r_format.func) {
      case fsqrt_op:
      case fabs_op:
      case fmov_op:
      case fcvts_op:
      case fcvtd_op:
      case fcvte_op:
      case fcvtw_op:
      case ftrunc_op:
      case fround_op:
      case ffloor_op:
      case fceil_op:
	sprintf(bufptr, "%s,%s",
		fp_register_name(i.r_format.re),
		fp_register_name(i.r_format.rd));
	break;
      case fcmp_op+0x0:
      case fcmp_op+0x1:
      case fcmp_op+0x2:
      case fcmp_op+0x3:
      case fcmp_op+0x4:
      case fcmp_op+0x5:
      case fcmp_op+0x6:
      case fcmp_op+0x7:
      case fcmp_op+0x8:
      case fcmp_op+0x9:
      case fcmp_op+0xa:
      case fcmp_op+0xb:
      case fcmp_op+0xc:
      case fcmp_op+0xd:
      case fcmp_op+0xe:
      case fcmp_op+0xf:
	sprintf(bufptr, "%s,%s",
		fp_register_name(i.r_format.rd),
		fp_register_name(i.r_format.rt));
	break;
      default:
	sprintf(bufptr, "%s,%s,%s",
		fp_register_name(i.r_format.re),
		fp_register_name(i.r_format.rd),
		fp_register_name(i.r_format.rt));
	break;
      } /* switch on func */
    } /* switch on rs */
    break; /* End of cop1 */
    
  case cop2_op:
  case cop3_op:
    {
      unsigned which_cop = i.j_format.opcode - cop0_op;
      
      switch (i.r_format.rs) {
      case bc_op:
	sprintf(bufptr, "bc%d%c\t", which_cop,
		bc_name[i.r_format.rt]);
	do_b_displacement = true;
	break;
      case mtc_op:
	sprintf(bufptr, "mtc%d\t%s,$%d", which_cop,
		register_name(i.r_format.rt, regmask),
		i.r_format.rd);
	break;
      case mfc_op:
	sprintf(bufptr, "mfc%d\t%s,$%d", which_cop,
		register_name(i.r_format.rt, regmask),
		i.r_format.rd);
	break;
      case cfc_op:
	sprintf(bufptr, "cfc%d\t%s,$%d", which_cop,
		register_name(i.r_format.rt, regmask),
		i.r_format.rd);
	break;
      case ctc_op:
	sprintf(bufptr, "ctc%d\t%s,$%d", which_cop,
		register_name(i.r_format.rt, regmask),
		i.r_format.rd);
	break;
      default:
	sprintf(bufptr, "c%d.%d\t%d", which_cop, i.r_format.rs, i.r_format.func);
	break;
      }
    }
    break;
    
  }
  
  /* Some instructions require more than just registers */
  
  if (do_loadstore) {
    short sign_extender = i.i_format.simmediate;
    *symbol_value = sign_extender;
    *ls_register = i.i_format.rs;
    bufptr += strlen(bufptr);
    sprintf(bufptr, "%d(%s)", sign_extender,
	    register_name(i.i_format.rs, regmask));
    return_value = -1;
  }
  else if (do_b_displacement) {
    short sign_extender = i.i_format.simmediate;
    bufptr += strlen(bufptr);
    sprintf(bufptr, "%#x", address+4+(sign_extender<<2));
    return_value = 2;
  }
  
  return return_value;
}

/* public - see .h file */
void
dis_regs(buffer, regmask, reg_values)
  char *buffer;
  unsigned regmask;
  unsigned reg_values[];
  {
  boolean first = true;
  unsigned i;

  buffer += strlen(buffer);
  for (i = 0; regmask; i++, regmask >>= 1)
    {
    if (regmask & 1)
      {
      sprintf(buffer, "%s%s=%#x",
	first ? "\t<" : ",",
	save.reg_names[i],
	reg_values[i]);
      buffer += strlen(buffer);
      first = false;
      }
    }
  if (!first)
    strcat(buffer, ">");
  }

#include <errno.h>
extern int errno;

/* public - see .h file */
int
disassembler(iadr, regstyle, get_symname, get_regvalue, get_bytes, print_header)
  unsigned iadr;
  int regstyle;
  char *(*get_symname)();
  int (*get_regvalue)();
  long (*get_bytes)();
  void (*print_header)();
  {
  unsigned old_iadr = iadr;

  if (!get_bytes)
    {
    errno = EINVAL;
    return -1;
    }

  /* Don't print address, value, or (if we have get_symname) jal targets */
  dis_init("", "", regstyle ? HARDWARE_NAMES : COMPILER_NAMES,
    ! (int) get_symname);

  for (; ; iadr += 4)
    {
    int which;
    unsigned instr, regmask, symbol_value, ls_register;
    char buffer[1024], *symname;

    instr = (unsigned) get_bytes();
    if (print_header)
      print_header(iadr, instr);
    which = disasm(buffer, iadr, instr, &regmask, &symbol_value, &ls_register);

    /* For jal, must print the target either via get_symname or numerically */
    if ((which == 1) && get_symname)
      if (symname = get_symname(symbol_value))
	strcat(buffer, symname);
      else
	sprintf(buffer + strlen(buffer), "%#x", symbol_value);

    /* For load/store, if we have get_regvalue, must print the effective addr */
    else if ((which < 0) && get_regvalue)
      sprintf(buffer + strlen(buffer),
	" <0x%x>", get_regvalue(ls_register) + symbol_value);

    /* If we have get_regvalue, we must print the registers */
    if (regmask && get_regvalue)
      {
      unsigned reg_values[32], reg_cnt, regtemp;

      for (reg_cnt = 0, regtemp = regmask; regtemp; reg_cnt++, regtemp >>= 1)
	if (regtemp & 1)
	  reg_values[reg_cnt] = get_regvalue(reg_cnt);
      dis_regs(buffer + strlen(buffer), regmask, reg_values);
      }

    puts(buffer);

    /* Quit unless we have a jump/call/branch delay slot */
    if (which <= 0)
      return iadr - old_iadr + 4;
    }
  }

