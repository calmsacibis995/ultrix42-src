/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: neednop.c,v 2010.2.1.3 89/11/29 14:30:04 bettina Exp $ */

#include <sys/param.h>
#ifdef BSD
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

#define public
#define private static

typedef unsigned boolean;
#define false 0
#define true 1

typedef union mips_instruction mips_inst;

#define S_FMT (0x10+s_fmt)
#define D_FMT (0x10+d_fmt)
#define W_FMT (0x10+w_fmt)

#define zero_reg 0

private boolean
isbr (inst)
    register mips_inst inst;
{
  switch (inst.i_format.opcode) {
  case bcond_op:
    switch (inst.i_format.rt) {
    case spimi_op:
    case tgei_op:
    case tgeiu_op:
    case tlti_op:
    case tltiu_op:
    case teqi_op:
    case tnei_op:
      goto no;
    }
    goto yes;
  case beq_op:
  case bne_op:
  case blez_op:
  case bgtz_op:
  case beql_op:
  case bnel_op:
  case blezl_op:
  case bgtzl_op:
  case jal_op:
  case j_op:
    goto yes;
  case spec_op:
    switch (inst.r_format.func) {
    case jalr_op:
    case jr_op:
      goto yes;
    }
    goto no;
  case cop0_op:
  case cop1_op:
  case cop2_op:
  case cop3_op:
    switch (inst.i_format.rs) {
    case bc_op:
      goto yes;
    }
    goto no;
  }
 no:
  return false;
 yes:
  return true;
}

private boolean
isfbr (inst)
    register mips_inst inst;
{
  switch (inst.i_format.opcode) {
  case cop1_op:
    switch (inst.i_format.rs) {
    case bc_op:
      goto yes;
    }
    goto no;
  }
 no:
  return false;
 yes:
  return true;
}

private boolean
isfop (inst)
    register mips_inst inst;
{
  switch (inst.i_format.opcode) {
  case cop1_op:
    switch (inst.i_format.rs) {
    case S_FMT:
    case D_FMT:
    case W_FMT:
      goto yes;
    }
    goto no;
  }
 no:
  return false;
 yes:
  return true;
}

private boolean
uses (inst, r)
    register mips_inst inst;
    register unsigned r;
{
  register unsigned rt, rs;
  
  if (r == zero_reg) goto no;
  
  rt = inst.i_format.rt;
  rs = inst.i_format.rs;
  switch (inst.i_format.opcode) {
  case addi_op:
  case addiu_op:
  case andi_op:
  case ori_op:
  case xori_op:
  case slti_op:
  case sltiu_op:
  case lui_op:
    if (r == rs) goto yes;
    break;
  case beq_op:
  case bne_op:
  case blez_op:
  case bgtz_op:
  case beql_op:
  case bnel_op:
  case blezl_op:
  case bgtzl_op:
  case sb_op:
  case sh_op:
  case sw_op:
  case swl_op:
  case swr_op:
  case sc_op:
    if (r == rs) goto yes;
    if (r == rt) goto yes;
    break;
  case bcond_op:
    switch (rt) {
    case tgei_op:
    case tgeiu_op:
    case tlti_op:
    case tltiu_op:
    case teqi_op:
    case tnei_op:
    case bltz_op:
    case bgez_op:
    case bltzal_op:
    case bgezal_op:
    case bltzl_op:
    case bgezl_op:
    case bltzall_op:
    case bgezall_op:
      if (r == rs) goto yes;
      break;
    }
    break;
  case jal_op:
  case j_op:
    break;
  case lb_op:
  case lh_op:
  case lw_op:
  case lbu_op:
  case lhu_op:
  case ld_op:
  case lwl_op:
  case lwr_op:
  case ll_op:
  case lwc1_op:
  case ldc1_op:
  case swc1_op:
  case sdc1_op:
    if (r == rs) goto yes;
    break;
  case sd_op:
    if (r == rs) goto yes;
    if (r == rt+0) goto yes;
    if (r == rt+1) goto yes;
    break;
  case spec_op:
    switch (inst.r_format.func) {
    case tge_op:
    case tgeu_op:
    case tlt_op:
    case tltu_op:
    case teq_op:
    case tne_op:
    case add_op:
    case addu_op:
    case sub_op:
    case subu_op:
    case and_op:
    case or_op:
    case xor_op:
    case nor_op:
    case slt_op:
    case sltu_op:
    case sllv_op:
    case srlv_op:
    case srav_op:
    case mult_op:
    case multu_op:
    case div_op:
    case divu_op:
      if (r == rt) goto yes;
      if (r == rs) goto yes;
      break;
    case sll_op:
    case srl_op:
    case sra_op:
      if (r == rt) goto yes;
      break;
    case jr_op:
    case jalr_op:
    case mthi_op:
    case mtlo_op:
      if (r == rs) goto yes;
      break;
    }
    break;
  case cop0_op:
  case cop1_op:
  case cop2_op:
  case cop3_op:
    switch (rs) {
    case ctc_op:
    case mtc_op:
      if (r == rt) goto yes;
      break;
    }
    break;
  }
 no:
  return false;
 yes:
  return true;
}

private boolean
usesf (inst, r)
    register mips_inst inst;
    register unsigned r;
{
  register unsigned rt, rs;
  
  rt = inst.i_format.rt;
  rs = inst.i_format.rs;
  switch (inst.i_format.opcode) {
  case swc1_op:
    if (r == rt) goto yes;
    break;
  case sdc1_op:
    if (r == rt+0) goto yes;
    if (r == rt+1) goto yes;
    break;
  case cop1_op:
    switch (rs) {
    case mfc_op:
      if (r == inst.f_format.rd) goto yes;
      break;
    case S_FMT:
    case D_FMT:
    case W_FMT:
      switch (inst.f_format.func) {
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
      case fadd_op:
      case fsub_op:
      case fmul_op:
      case fdiv_op:
	if (r == rt) goto yes;
	if (rs == D_FMT && r == rt+1) goto yes;
	/* fall through */
      case fmov_op:
      case fneg_op:
      case fabs_op:
      case fsqrt_op:
      case fcvts_op:
      case fcvtd_op:
      case fcvtw_op:
      case fround_op:
      case ftrunc_op:
      case fceil_op:
      case ffloor_op:
	if (r == inst.f_format.rd) goto yes;
	if (rs == D_FMT && r == inst.f_format.rd+1) goto yes;
	break;
      }
      break;
    }
    break;
  }
 no:
  return false;
 yes:
  return true;
}


private boolean
defineshilo (inst)
    register mips_inst inst;
{
  switch (inst.i_format.opcode) {
  case spec_op:
    switch (inst.r_format.func) {
    case multu_op:
    case mult_op:
    case divu_op:
    case div_op:
    case mthi_op:
    case mtlo_op:
      goto yes;
    }
    break;
  }
 no:
  return false;
 yes:
  return true;
}

private boolean
defines (inst, r)
    register mips_inst inst;
    register unsigned r;
{
  register unsigned rt, rs;
  
  if (r == zero_reg) goto no;
  
  rt = inst.i_format.rt;
  rs = inst.i_format.rs;
  switch (inst.i_format.opcode) {
  case addi_op:
  case addiu_op:
  case andi_op:
  case ori_op:
  case xori_op:
  case slti_op:
  case sltiu_op:
  case lui_op:
    if (r == rt) goto yes;
    break;
  case bcond_op:
    switch (rt) {
    case bltzal_op:
    case bgezal_op:
    case bltzall_op:
    case bgezall_op:
      if (r == 31) goto yes;
      break;
    }
    break;
  case jal_op:
    if (r == 31) goto yes;
    break;
  case lb_op:
  case lh_op:
  case lw_op:
  case lbu_op:
  case lhu_op:
  case lwl_op:
  case lwr_op:
  case ll_op:
  case sc_op:
    if (r == rt) goto yes;
    break;
  case ld_op:
    if (r == rt+0) goto yes;
    if (r == rt+1) goto yes;
    break;
  case spec_op:
    switch (inst.r_format.func) {
    case add_op:
    case addu_op:
    case sub_op:
    case subu_op:
    case and_op:
    case or_op:
    case xor_op:
    case nor_op:
    case slt_op:
    case sltu_op:
    case sllv_op:
    case srlv_op:
    case srav_op:
    case sll_op:
    case srl_op:
    case sra_op:
    case jalr_op:
    case mfhi_op:
    case mflo_op:
      if (r == inst.r_format.rd) goto yes;
      break;
    }
    break;
  case cop0_op:
  case cop1_op:
  case cop2_op:
  case cop3_op:
    switch (rs) {
    case mfc_op:
    case cfc_op:
      if (r == rt) goto yes;
      break;
    }
    break;
  }
 no:
  return false;
 yes:
  return true;
}

/* return number of nops needed between i1 and i2, where i0 is instruction
   before i1. */
int
need_nop (i0, i1, i2)
    mips_inst i0, i1, i2;
{
  register unsigned rt, rs, rd;
  
  /* check for 2-cycle hazards from i0 to i2 */
  switch (i0.i_format.opcode) {
  case spec_op:
    switch (i0.r_format.func) {
    case mfhi_op:
    case mflo_op:
      if (defineshilo(i2)) goto ret1;
      break;
    }
    break;
  case cop1_op:
    switch (i0.r_format.rs) {
    case ctc_op:
      if (isfbr(i2)) goto ret1;
      break;
    }
    break;
  }

  /* check for 1 and 2-cycle hazards from i1 to i2 */
  rt = i1.i_format.rt;
  rs = i1.i_format.rs;
  switch (i1.i_format.opcode) {
  case lb_op:
  case lh_op:
  case lw_op:
  case lbu_op:
  case lhu_op:
  case lwl_op:
  case lwr_op:
  case ll_op:
    if (uses(i2, rt)) goto ret1;
    break;
  case ld_op:
    if (uses(i2, rt+0) || uses(i2, rt+1)) goto ret1;
    break;
  case lwc1_op:
    if (usesf(i2, rt)) goto ret1;
    break;
  case ldc1_op:
    if (usesf(i2, rt+0) || usesf(i2, rt+1)) goto ret1;
    break;
  case spec_op:
    rd = i1.r_format.rd;
    switch (i1.r_format.func) {
    case mflo_op:
    case mfhi_op:
      if (defineshilo(i2)) goto ret2;
      break;
    }
    break;
  case cop1_op:
    rd = i1.r_format.rd;
    switch (rs) {
    case mtc_op:
      if (usesf(i2, rd)) goto ret1;
      break;
    case ctc_op:
      if (isfbr(i2)) goto ret2;
      if (isfop(i2)) goto ret1;
      break;
    case cfc_op:
    case mfc_op:
      if (uses(i2, rt)) goto ret1;
      break;
    case S_FMT:
    case D_FMT:
    case W_FMT:
      switch (i1.f_format.func) {
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
	if (isfbr(i2)) goto ret1;
	break;
      }
      break;
    }
    break;
  }
 ret0:
  return 0;
 ret1:
  return 1;
 ret2:
  return 2;
}
