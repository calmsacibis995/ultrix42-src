/* --------------------------------------------------- */
/* | Copyright (c) 1986 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                            | */
/* --------------------------------------------------- */
/* $Header: inst.h,v 1031.2 88/05/16 14:11:49 bettina Exp $ */
/*
 * inst.h -- instruction format defines
 */

#ifdef MIPSEB
union mips_instruction {
	unsigned word;
	unsigned char byte[4];
	struct {
		unsigned opcode : 6;
		unsigned target : 26;
	} j_format;
	struct {
		unsigned opcode : 6;
		unsigned rs : 5;
		unsigned rt : 5;
		/* REALLY signed, but compiler doesn't handle yet */
		unsigned simmediate : 16;
	} i_format;
	struct {
		unsigned opcode : 6;
		unsigned rs : 5;
		unsigned rt : 5;
		unsigned uimmediate : 16;
	} u_format;
	struct {
		unsigned opcode : 6;
		unsigned rs : 5;
		unsigned rt : 5;
		unsigned rd : 5;
		unsigned re : 5;
		unsigned func : 6;
	} r_format;
	struct {
		unsigned opcode : 6;
		unsigned : 1;
		unsigned fmt : 4;
		unsigned rt : 5;
		unsigned rd : 5;
		unsigned re : 5;
		unsigned func : 6;
	} f_format;
};
#endif

#ifdef MIPSEL
union mips_instruction {
	unsigned word;
	unsigned char byte[4];
	struct {
		unsigned target : 26;
		unsigned opcode : 6;
	} j_format;
	struct {
		/* really should be signed, but compiler doesn't handle */
		unsigned simmediate : 16;
		unsigned rt : 5;
		unsigned rs : 5;
		unsigned opcode : 6;
	} i_format;
	struct {
		unsigned uimmediate : 16;
		unsigned rt : 5;
		unsigned rs : 5;
		unsigned opcode : 6;
	} u_format;
	struct {
		unsigned func : 6;
		unsigned re : 5;
		unsigned rd : 5;
		unsigned rt : 5;
		unsigned rs : 5;
		unsigned opcode : 6;
	} r_format;
	struct {
		unsigned func : 6;
		unsigned re : 5;
		unsigned rd : 5;
		unsigned rt : 5;
		unsigned fmt : 4;
		unsigned : 1;
		unsigned opcode : 6;
	} f_format;
};
#endif

#define	spec_op		0x00
#define	bcond_op	0x01
#define	j_op		0x02
#define	jal_op		0x03

#define	beq_op		0x04
#define	bne_op		0x05
#define	blez_op		0x06
#define	bgtz_op		0x07

#define	addi_op		0x08
#define	addiu_op	0x09
#define	slti_op		0x0A
#define	sltiu_op	0x0B

#define	andi_op		0x0C
#define	ori_op		0x0D
#define	xori_op		0x0E
#define	lui_op		0x0F

#define	lb_op		0x20
#define	lh_op		0x21
#define	lw_op		0x23
#define	lbu_op		0x24
#define	lhu_op		0x25
#define	ld_op		0x27
#define	sb_op		0x28
#define	sh_op		0x29
#define	sw_op		0x2B
#define	sd_op		0x2F
#define	lwl_op		0x22
#define	lwr_op		0x26
#define	swl_op		0x2a
#define	swr_op		0x2e

/* Co-processor sub-opcodes */
#define	bc_op		0x08
#define	mfc_op		0x00
#define cfc_op		0x02
#define	mtc_op		0x04
#define ctc_op		0x06

/* Co-processor 0 opcodes */
#define	cop0_op		0x10
#define	lwc0_op		0x30
#define	ldc0_op		0x34
#define	swc0_op		0x38
#define	sdc0_op		0x3c

/* Co-processor 0 sub-opcodes */
#define	tlbr_op		0x1
#define	tlbwi_op	0x2
#define	tlbwr_op	0x6
#define	tlbp_op		0x8
#define	rfe_op		0x10
 
/* Co-processor 1 opcodes */
#define	cop1_op		0x11
#define	lwc1_op		0x31
#define	ldc1_op		0x35
#define	swc1_op		0x39
#define	sdc1_op		0x3D

/* Co-processor 1 sub-opcodes */
#define	fadd_op		0x00
#define	fsub_op		0x01
#define	fmpy_op		0x02
#define	fdiv_op		0x03
#define	fsqrt_op	0x04
#define	fabs_op		0x05
#define	fmov_op		0x06
#define	fcvts_op	0x20
#define	fcvtd_op	0x21
#define	fcvte_op	0x22
#define	fcvtw_op	0x24
#define	fcmp_op		0x30
#define	s_fmt		0
#define	d_fmt		1
#define	e_fmt		2
#define	w_fmt		4

/* Other coprocessor opcodes */
#define	cop2_op		0x12
#define	lwc2_op		0x32
#define	ldc2_op		0x36
#define	swc2_op		0x3a
#define	sdc2_op		0x3e

#define	cop3_op		0x13
#define	lwc3_op		0x33
#define	ldc3_op		0x37
#define	swc3_op		0x3b
#define	sdc3_op		0x3f


/* bcond subopcodes */
#define	bltz_op		0x00
#define	bgez_op		0x01

/* special subopcodes */
#define	sll_op		0x00
#define	srl_op		0x02
#define	sra_op		0x03
#define	sllv_op		0x04
#define	srlv_op		0x06
#define	srav_op		0x07
#define	jr_op		0x08
#define	jalr_op		0x09
#define	syscall_op	0x0C
#define	break_op	0x0D
#define	vcall_op	0x0E

#define	mfhi_op		0x10
#define	mthi_op		0x11
#define	mflo_op		0x12
#define	mtlo_op		0x13
#define	mult_op		0x18
#define	multu_op	0x19
#define	div_op		0x1A
#define	divu_op		0x1B

#define	add_op		0x20
#define	addu_op		0x21
#define	and_op		0x24
#define	or_op		0x25
#define	xor_op		0x26
#define	nor_op		0x27
#define	sub_op		0x22
#define	subu_op		0x23
#define	slt_op		0x2A
#define	sltu_op		0x2B
