/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: binasm.h,v 2010.2.1.5 89/11/29 22:38:51 bettina Exp $ */

#include <ansi_compat.h>
#ifdef LANGUAGE_PASCAL
const

/* Traumatic change in 1.30: record becomes larger */
  bin_rec_len = 16;	{16 bytes per record}
/*bin_rec_len = 12;	{12 bytes per record}*/
  symtype_first = first(integer);
  symtype_last  = last(integer);

type

  registers = (
	xr0,	xr1,	xr2,	xr3,	xr4,	xr5,	xr6,	xr7,
	xr8,	xr9,	xr10,	xr11,	xr12,	xr13,	xr14,	xr15,
	xr16,	xr17,	xr18,	xr19,	xr20,	xr21,	xr22,	xr23,
	xr24,	xr25,	xr26,	xr27,	xr28,	xr29,	xr30,	xr31,
	xfr0,	xfr1,	xfr2,	xfr3,	xfr4,	xfr5,	xfr6,	xfr7,
	xfr8,	xfr9,	xfr10,	xfr11,	xfr12,	xfr13,	xfr14,	xfr15,
	xfr16,	xfr17,	xfr18,	xfr19,	xfr20,	xfr21,	xfr22,	xfr23,
	xfr24,	xfr25,	xfr26,	xfr27,	xfr28,	xfr29,	xfr30,	xfr31,
	xnoreg
	);
#ifndef REG_SET
#define REG_SET 1
  aligned_registers = (
    ar0,    ar1,    ar2,    ar3,    ar4,    ar5,    ar6,    ar7,
    ar8,    ar9,    ar10,   ar11,   ar12,   ar13,   ar14,   ar15,
    ar16,   ar17,   ar18,   ar19,   ar20,   ar21,   ar22,   ar23,
    ar24,   ar25,   ar26,   ar27,   ar28,   ar29,   ar30,   ar31,
    afr0,   afr1,   afr2,   afr3,   afr4,   afr5,   afr6,   afr7,
    afr8,   afr9,   afr10,  afr11,  afr12,  afr13,  afr14,  afr15,
    afr16,  afr17,  afr18,  afr19,  afr20,  afr21,  afr22,  afr23,
    afr24,  afr25,  afr26,  afr27,  afr28,  afr29,  afr30,  afr31
    );
  aligned_regset = set of aligned_registers;

#endif

  itype = (
	ilabel,     isym,       iglobal,    iabort,     ialign,
	iascii,     iasciiz,    ibyte,      icomm,      ilcomm,
	idata,      idouble,    ifile,      ifloat,     ihalf,
	iline,      idead,      inop,       ireserved1, ireserved2,
	ispace,     itext,      iword,      iocode,     iend,
	isdata,     irdata,     ient,       iloc,       ibgnb,
	iendb,      iasm0,      iset,       iorg,       irep,
	iendrep,    ilab,       ivreg,      imask,      ifmask,
	ierr,       iglobabs,   iverstamp,  iframe,     iextended,
	iextern,    iaent,      ioption,    inoalias,   ialias,
	ifp1,       ifp2,       ifpboth,    ilivereg, 	igjaldef,
	igjallive,  igjrlive, ishift_addr	);

  {---------------------------------------------------------------------------------}
  { asmcodes specify the assembler instructions 			      }
  {---------------------------------------------------------------------------------}

  asmcodes = (
	zabs,       zadd,       zaddu,      zand,       zb,        {000-004}
	zbc0f,      zbc0t,      zbc1f,      zbc1t,      zbc2f,     {005-009}
	zbc2t,      zbc3f,      zbc3t,      zbeq,       zbge,      {010-014}
	zbgeu,      zbgez,      zbgt,       zbgtu,      zbgtz,     {015-019}
	zble,       zbleu,      zblez,      zblt,       zbltu,     {020-024}
	zbltz,      zbne,       zbreak,     zc0,        zc1,       {025-029}
	zc2,        zc3,        zdiv,       zdivu,      zj,        {030-034}
	zjal,       zla,        zlb,        zlbu,       zlh,       {035-039}
	zlhu,       zli,        zlw,        z43,        zlwc1,     {040-044}
	zlwc2,      zlwc3,      zmfhi,      zmflo,      zmove,     {045-049}
	z50,        zswc1,      zswc2,      zswc3,      zmthi,     {050-054}
	zmtlo,      zmul,       zmulo,      zmulou,     zmult,     {055-059}
	zmultu,     zneg,       znop,       znor,       zor,       {060-064}
	zrem,       zremu,      zrfe,       zrol,       zror,      {065-069}
	zsb,        zseq,       zsge,       zsgeu,      zsgt,      {070-074}
	zsgtu,      zsh,        zsle,       zsleu,      zsll,      {075-079}
	zslt,       zsltu,      zsne,       zsra,       zsrl,      {080-084}
	zsub,       zsubu,      zsw,        zsyscall,   zxor,      {085-089}
	znot,       zlwl,       zlwr,       zswl,       zswr,      {090-094}
	zvcall,     zmfc0,      zmfc1,      zmfc2,      zmfc3,     {095-099}
	zmtc0,      zmtc1,      zmtc2,      zmtc3,      ztlbr,     {100-104}
	ztlbwi,     ztlbwr,     ztlbp,      zld,        zsd,       {106-109}
	z110,       zldc1,      zldc2,      zldc3,      z114,      {110-114}
	zsdc1,      zsdc2,      zsdc3,				   {115-117}
	fl_s,		fl_d,		fl_e,			   {118-120}
	fs_s,		fs_d,		fs_e,			   {121-123}
 	fadd_s,		fadd_d,		fadd_e,			   {124-126}
 	fsub_s,		fsub_d,		fsub_e,			   {127-129}
 	fmul_s,		fmul_d,		fmul_e,			   {130-132}
 	fdiv_s,		fdiv_d,		fdiv_e,			   {133-135}
 	fsqrt_s,	fsqrt_d,	fsqrt_e,		   {136-138}
 	fmov_s,		fmov_d,		fmov_e,			   {139-141}
 	fabs_s,		fabs_d,		fabs_e,			   {142-144}
        		fcvt_s_d,	fcvt_s_e,	fcvt_s_w,  {145-147}
 	fcvt_d_s,        		fcvt_d_e,	fcvt_d_w,  {148-150}
 	fcvt_e_s,	fcvt_e_d,        		fcvt_e_w,  {151-153}
 	fcvt_w_s,	fcvt_w_d,	fcvt_w_e,		   {154-156}
 	fc_f_s,		fc_f_d,		fc_f_e,			   {157-159}
 	fc_un_s,	fc_un_d,	fc_un_e,		   {160-162}
 	fc_eq_s,	fc_eq_d,	fc_eq_e,		   {163-165}
 	fc_ueq_s,	fc_ueq_d,	fc_ueq_e,		   {166-168}
 	fc_olt_s,	fc_olt_d,	fc_olt_e,		   {169-171}
 	fc_ult_s,	fc_ult_d,	fc_ult_e,		   {172-174}
	fc_ole_s,	fc_ole_d,	fc_ole_e,		   {175-177}
 	fc_ule_s,	fc_ule_d,	fc_ule_e,		   {178-180}
 	fc_sf_s,	fc_sf_d,	fc_sf_e,		   {181-183}
 	fc_ngle_s,	fc_ngle_d,	fc_ngle_e,		   {184-186}
 	fc_seq_s,	fc_seq_d,	fc_seq_e,		   {187-189}
 	fc_ngl_s,	fc_ngl_d,	fc_ngl_e,		   {190-192}
 	fc_lt_s,	fc_lt_d,	fc_lt_e,		   {193-195}
 	fc_nge_s,	fc_nge_d,	fc_nge_e,		   {196-198}
 	fc_le_s,	fc_le_d,	fc_le_e,		   {199-201}
 	fc_ngt_s,	fc_ngt_d,	fc_ngt_e,		   {202-204}
	zlui,		zulw,		zulh,		zulhu,	   {205-208}
	zusw,		zush,		zaddi,		zaddiu,	   {209-212}
	zslti,		zsltiu,		zandi,		zori,	   {213-216}
	zxori,		z218,		znegu,		zbeqz,	   {217-220}
	zbnez,							   {221}
	fneg_s,		fneg_d,		fneg_e,			   {222-224}
	zcfc1,		zctc1,		zbal,		zbgezal,   {225-228}
	zbltzal,	zmtc1_d,	zmfc1_d,		   {229-231}
	ztrunc_w_s,	ztrunc_w_d,	ztrunc_w_e,		   {232-234}
	zround_w_s,	zround_w_d,	zround_w_e,		   {235-237}
	zaddou,		zsubou,					   {238-239}
	ztruncu_w_s,	ztruncu_w_d,	ztruncu_w_e,		   {240-242}
	zroundu_w_s,	zroundu_w_d,	zroundu_w_e,		   {243-245}
	zcfc0,		zcfc2,		zcfc3,		zctc0,	   {246-249}
	zctc2,		zctc3,					   {250-251}
        fli_s,		fli_d,		fli_e,			   {252-254}
	ztlt,		ztltu,		ztge,		ztgeu,	   {255-258}
	zteq,		ztne,		zll,		zsc,	   {259-262}
	zceil_w_s,	zceil_w_d,	zceil_w_e,		   {263-265}
	zceilu_w_s,	zceilu_w_d,	zceilu_w_e,		   {266-268}
	zfloor_w_s,	zfloor_w_d,	zfloor_w_e,		   {269-271}
	zflooru_w_s,	zflooru_w_d,	zflooru_w_e,		   {272-274}
	zbeql,          zbeqzl,         zbnel,          zbnezl,    {275-278}
	zblel,          zbleul,         zblezl,                    {279-281}
	zbgtl,          zbgtul,         zbgtzl,                    {282-284}
	zbltl,          zbltul,         zbltzl,         zbltzall,  {285-288}
	zbgel,          zbgeul,         zbgezl,         zbgezall,  {289-292}
	zbc0fl,         zbc0tl,         zbc1fl,         zbc1tl,    {293-296}
	zbc2fl,         zbc2tl,         zbc3fl,         zbc3tl,    {297-300}
	{ 9/26/89 new op code added for mips-3 arch. all 27 of them }
	zldl,           zldr,           zlld,         	zlwu,	   {301-304}
	zsdl,           zsdr,           zscd,	        	   {305-307}
	zdaddi,         zdaddiu,        zdadd,	        zdaddu,	   {308-311}
	zdsub,          zdsubu,         			   {312-313}
	zdsll,          zdsrl,          zdsra,         		   {314-316}
	zdsllv,         zdsrlv,         zdsrav,         	   {317-319}
	zdmult,         zdmultu,        zddiv,         	zddivu,    {320-323}
	zlsc1,         	zssc1,        	zdmtc1,         zdmfc1,    {324-327}
	zdli,							   {328}
	zbad
	);

  kind = (
	chars, instruction
	);

  symtype = symtype_first .. symtype_last;

  format = (
	frob,				{ reg, offset(+/-32k), base	      }
	fra,				{ reg, [sym]+offset		      }
	fri,				{ reg, immed (32 bit)		      }
	frrr,				{ reg, reg, reg 		      }
	frri,				{ reg, reg, immed (32 bit)	      }
	frr,				{ reg, reg			      }
	fa,				{ [sym]+offset [+(base)]	      }
	fr,				{ reg				      }
	frrl,				{ reg, reg, sym 		      }
	frl,				{ reg, sym			      }
	fl,				{ sym				      }
	forrr,				{ co processor if required?	      }
	fril, 				{ reg, immed, label                   }
	fi				{ immed				      }
#if 0	
	,flrr				{ sym, reg, reg			      }
#endif	
	);

  set_value = (
	set_undefined,
	set_reorder,
	set_noreorder,
	set_macro,
	set_nomacro,
	set_at,
	set_noat,
	set_move,
	set_nomove,
	set_bopt,
	set_nobopt,
	set_volatile,
	set_novolatile
	);

  opt_type = (
	o_undefined,
	o_optimize,
	o_other
	);

  opt_arg_type = (
	opt_none,
	opt_int,
	opt_float,
	opt_string
	);

  binasm = packed record
    case kind of
      chars : (
	data : packed array[1 .. bin_rec_len] of char
	);

      instruction : (
	symno : symtype;		{ 32 bits			      }
	fill0a : 0..1023;		{ 10 bits to word boundary	      }
	case instr : itype of 	{ 6 bits			      }
	    ierr, idead : ();		{ ????? 			      }

	    iabort, idata, iend, iglobal, iasm0, iendrep, ilabel, ireserved1,
		inop, ireserved2, itext, isdata, irdata, ilab, ibgnb,
		iendb : ();		{ symno has the info		      }

	    ient, iaent : (
	      lexlevel : integer);

	    iframe : (
	      frameoffset : integer;
	      framereg : registers;
	      pcreg : registers);

	    imask, ifmask : (
	      regmask,			{ 32 bits			      }
	      regoffset : integer);	{ 32 bits			      }

	    iverstamp : (
	      majornumber,		{ 32 bits			      }
	      minornumber : integer);	{ 32 bits			      }

	    iloc : (
	      filenumber,		{ 32 bits			      }
	      linenumber : integer);	{ 32 bits			      }

	    { Add "ishift_addr" . Gili 10/13/89			}
	    ialign, iascii, iasciiz, icomm, ilcomm, isym, ifloat, idouble,
		iextended, iorg, irep, iset, ispace, ifile, iline,
		iglobabs, iextern, ishift_addr : (
	      length : integer;		{ 32 bits			      }
	      rep : cardinal);		{ 32 bits - only for ifloat, idouble, }
					{   iextended			      }
	    ilivereg : (
	      gpmask,			{ 32 bits			      }
	      fpmask : cardinal);	{ 32 bits			      }

	    { 9/21/89 add ".gjaldef", ".gjallive", ".gjrlive" directive }
	    igjaldef, igjallive, igjrlive : (
	      gjmask : aligned_regset);{ 64 bits			      }

	    ibyte, ihalf, iword : (
	      expression : integer;	{ 32 bits			      }
	      replicate : cardinal);	{ 32 bits			      }

	    iocode, ivreg : (
	      fill03 : 0..63;		{  6 bits to half boundary	      }
	      op : asmcodes;		{  9 bits, 16 bits due to alignment   }
	      reg1 : registers; 	{  7 bits			      }
	      reg2 : registers; 	{  7 bits			      }
	      case form : format of	{  4 bits			      }
		frrr : (		{ reg1, reg2, reg3		      }
		  reg3 : registers);	{  7 bits			      }
		fa,			{ [sym]+offset			      }
		frob,			{ reg1, offset(+/-32k), reg2	      }
		fra,			{ reg1, [sym]+offset		      }
		fri,			{ reg1, immed (32 bit)		      }
		fril,			{ reg1, immed (32 bit), label	      }
		frri : (		{ reg1, reg2, immed (32 bit)	      }
		  immediate : integer); { 32 bits			      }
		fr,			{ reg1				      }
		frr,			{ reg1, reg2			      }
		frl,			{ reg1, sym			      }
		fl : ();		{ sym				      }
		forrr : ();		{ co processor if required?	      }
		fi : (			{ immed				      }
		  imm : integer);	{ 32 bits 			      }
#if 0		
		frrl,			{ reg1, reg2, sym		      }
		flrr : ();		{ sym, reg1, reg2		      }
#else		
		frrl : ();		{ reg1, reg2, sym		      }
#endif		
	      );
	    ioption: (
	      option: opt_type;		{ which option (e.g. "O" for "-O3")   }
	      fill04: 0 .. 16#3fffffff; { pad to 32-bit boundary	      }
	      case opt_arg_type of	{ associated arg (e.g. "3" for "-O3") }
		opt_none: ();
		opt_int: (opt_int_value: integer);
					{ integer value of argument           }
		opt_float,
		opt_string: (opt_len: integer);
					{ length in bytes of string arg which }
					{   appears in future binasm records, }
					{   representing either fp or string  }
	      );
	    inoalias,			{ no aliasing till reversed by ialias }
	    ialias: (
	      basereg1: registers;
	      basereg2: registers;
	      );
	);
      end;  {record}

#endif /* LANGUAGE_PASCAL */


#ifdef __LANGUAGE_C

typedef unsigned asmopcode;
typedef unsigned asmformat;
typedef unsigned asmreg;
typedef int asmint;
typedef unsigned asmuint;
typedef int asmlabel;
typedef int asmsym;

#define zero 0
#define float_register 32
#define xnoreg 64

#define ilabel		 0
/*#define isym		 1*/
#define iglobal		 2
#define iabort		 3
#define ialign		 4
#define iascii		 5
#define iasciiz		 6
#define ibyte		 7
#define icomm		 8
#define ilcomm		 9
#define idata		10
#define idouble		11
#define ifile		12
#define ifloat		13
#define ihalf		14
/*#define iline		15*/
#define idead		16
#define inop		17
#define ireserved1	18
#define ireserved2	19
#define ispace		20
#define itext		21
#define iword		22
#define iocode		23
#define iend		24
#define isdata		25
#define irdata		26
#define ient		27
#define iloc		28
#define ibgnb		29
#define iendb		30
#define iasm0		31
#define iset		32
#define iorg		33
#define irep		34
#define iendrep		35
#define ilab		36
#define ivreg		37
#define imask		38
#define ifmask		39
#define ierr		40
#define iglobabs	41
#define iverstamp	42
#define iframe		43
#define iextended	44
#define iextern		45
#define iaent		46
#define ioption		47
#define inoalias	48
#define ialias		49
#define ifp1		50
#define ifp2		51
#define ifpboth		52
#define ilivereg	53
#define igjaldef	54
#define igjallive	55
#define igjrlive	56
#define ishift_addr	57

#define zabs		  0
#define zadd		  1
#define zaddu		  2
#define zand		  3
#define zb		  4
#define zbc0f		  5
#define zbc0t		  6
#define zbc1f		  7
#define zbc1t		  8
#define zbc2f		  9
#define zbc2t		 10
#define zbc3f		 11
#define zbc3t		 12
#define zbeq		 13
#define zbge		 14
#define zbgeu		 15
#define zbgez		 16
#define zbgt		 17
#define zbgtu		 18
#define zbgtz		 19
#define zble		 20
#define zbleu		 21
#define zblez		 22
#define zblt		 23
#define zbltu		 24
#define zbltz		 25
#define zbne		 26
#define zbreak		 27
#define zc0		 28
#define zc1		 29
#define zc2		 30
#define zc3		 31
#define zdiv		 32
#define zdivu		 33
#define zj		 34
#define zjal		 35
#define zla		 36
#define zlb		 37
#define zlbu		 38
#define zlh		 39
#define zlhu		 40
#define zli		 41
#define zlw		 42
#define z43		 43
#define zlwc1		 44
#define zlwc2		 45
#define zlwc3		 46
#define zmfhi		 47
#define zmflo		 48
#define zmove		 49
#define z50		 50
#define zswc1		 51
#define zswc2		 52
#define zswc3		 53
#define zmthi		 54
#define zmtlo		 55
#define zmul		 56
#define zmulo		 57
#define zmulou		 58
#define zmult		 59
#define zmultu		 60
#define zneg		 61
#define znop		 62
#define znor		 63
#define zor		 64
#define zrem		 65
#define zremu		 66
#define zrfe		 67
#define zrol		 68
#define zror		 69
#define zsb		 70
#define zseq		 71
#define zsge		 72
#define zsgeu		 73
#define zsgt		 74
#define zsgtu		 75
#define zsh		 76
#define zsle		 77
#define zsleu		 78
#define zsll		 79
#define zslt		 80
#define zsltu		 81
#define zsne		 82
#define zsra		 83
#define zsrl		 84
#define zsub		 85
#define zsubu		 86
#define zsw		 87
#define zsyscall	 88
#define zxor		 89
#define znot		 90
#define zlwl		 91
#define zlwr		 92
#define zswl		 93
#define zswr		 94
#define zvcall		 95
#define zmfc0		 96
#define zmfc1		 97
#define zmfc2		 98
#define zmfc3		 99
#define zmtc0		100
#define zmtc1		101
#define zmtc2		102
#define zmtc3		103
#define ztlbr		104
#define ztlbwi		105
#define ztlbwr		106
#define ztlbp		107
#define zld		108
#define zsd		109
#define z110		110
#define zldc1		111
#define zldc2		112
#define zldc3		113
#define z114		114
#define zsdc1		115
#define zsdc2		116
#define zsdc3		117
#define fl_s		118
#define fl_d		119
#define fl_e		120
#define fs_s		121
#define fs_d		122
#define fs_e		123
#define fadd_s		124
#define fadd_d		125
#define fadd_e		126
#define fsub_s		127
#define fsub_d		128
#define fsub_e		129
#define fmul_s		130
#define fmul_d		131
#define fmul_e		132
#define fdiv_s		133
#define fdiv_d		134
#define fdiv_e		135
#define fsqrt_s		136
#define fsqrt_d		137
#define fsqrt_e		138
#define fmov_s		139
#define fmov_d		140
#define fmov_e		141
#define fabs_s		142
#define fabs_d		143
#define fabs_e		144
#define fcvt_s_d	145
#define fcvt_s_e	146
#define fcvt_s_w	147
#define fcvt_d_s	148
#define fcvt_d_e	149
#define fcvt_d_w	150
#define fcvt_e_s	151
#define fcvt_e_d	152
#define fcvt_e_w	153
#define fcvt_w_s	154
#define fcvt_w_d	155
#define fcvt_w_e	156
#define fc_f_s		157
#define fc_f_d		158
#define fc_f_e		159
#define fc_un_s		160
#define fc_un_d		161
#define fc_un_e		162
#define fc_eq_s		163
#define fc_eq_d		164
#define fc_eq_e		165
#define fc_ueq_s	166
#define fc_ueq_d	167
#define fc_ueq_e	168
#define fc_olt_s	169
#define fc_olt_d	170
#define fc_olt_e	171
#define fc_ult_s	172
#define fc_ult_d	173
#define fc_ult_e	174
#define fc_ole_s	175
#define fc_ole_d	176
#define fc_ole_e	177
#define fc_ule_s	178
#define fc_ule_d	179
#define fc_ule_e	180
#define fc_sf_s		181
#define fc_sf_d		182
#define fc_sf_e		183
#define fc_ngle_s	184
#define fc_ngle_d	185
#define fc_ngle_e	186
#define fc_seq_s	187
#define fc_seq_d	188
#define fc_seq_e	189
#define fc_ngl_s	190
#define fc_ngl_d	191
#define fc_ngl_e	192
#define fc_lt_s		193
#define fc_lt_d		194
#define fc_lt_e		195
#define fc_nge_s	196
#define fc_nge_d	197
#define fc_nge_e	198
#define fc_le_s		199
#define fc_le_d		200
#define fc_le_e		201
#define fc_ngt_s	202
#define fc_ngt_d	203
#define fc_ngt_e	204
#define zlui		205
#define zulw		206
#define zulh		207
#define zulhu		208
#define zusw		209
#define zush		210
#define zaddi		211
#define zaddiu		212
#define zslti		213
#define zsltiu		214
#define zandi		215
#define zori		216
#define zxori		217
#define z218		218
#define znegu		219
#define zbeqz		220
#define zbnez		221
#define fneg_s		222
#define fneg_d		223
#define fneg_e		224
#define zcfc1		225
#define zctc1		226
#define zbal		227
#define zbgezal		228
#define zbltzal		229
#define zmtc1_d		230
#define zmfc1_d		231
#define ztrunc_w_s	232
#define ztrunc_w_d	233
#define ztrunc_w_e	234
#define zround_w_s	235
#define zround_w_d	236
#define zround_w_e	237
#define zaddou		238
#define zsubou		239
#define ztruncu_w_s	240
#define ztruncu_w_d	241
#define ztruncu_w_e	242
#define zroundu_w_s	243
#define zroundu_w_d	244
#define zroundu_w_e	245
#define zcfc0		246
#define zcfc2		247
#define zcfc3		248
#define zctc0		249
#define zctc2		250
#define zctc3		251
#define fli_s		252
#define fli_d		253
#define fli_e		254
#define ztlt		255
#define ztltu		256
#define ztge		257
#define ztgeu		258
#define zteq		259
#define ztne		260
#define zll		261
#define zsc		262
#define zceil_w_s	263
#define zceil_w_d	264
#define zceil_w_e	265
#define zceilu_w_s	266
#define zceilu_w_d	267
#define zceilu_w_e	268
#define zfloor_w_s	269
#define zfloor_w_d	270
#define zfloor_w_e	271
#define zflooru_w_s	272
#define zflooru_w_d	273
#define zflooru_w_e	274
#define	zbeql		275
#define	zbeqzl		276
#define zbnel		277
#define zbnezl		278
#define	zblel		279
#define	zbleul		280
#define zblezl		281
#define	zbgtl		282
#define zbgtul		283
#define zbgtzl		284
#define	zbltl		285
#define zbltul		286
#define zbltzl		287
#define zbltzall	288
#define zbgel		289
#define zbgeul		290
#define zbgezl		291
#define zbgezall	292
#define zbc0fl		293
#define zbc0tl		294
#define zbc1fl		295
#define zbc1tl		296
#define zbc2fl		297
#define zbc2tl		298
#define zbc3fl		299
#define zbc3tl		300
#define zldl            301
#define zldr            302
#define zlld         	303  
#define zlwu         	304  
#define zsdl            305
#define zsdr            306
#define zscd	        307
#define zdaddi          308
#define zdaddiu       	309
#define zdadd	       	310
#define zdaddu		311
#define zdsub           312
#define zdsubu       	312  		3
#define zdsll         	314
#define zdsrl           315
#define zdsra           316
#define zdsllv          317
#define zdsrlv          318
#define zdsrav       	319
#define zdmult        	320
#define zdmultu        	321
#define zddiv         	322
#define zddivu    	323
#define zlsc1         	324
#define zssc1        	325
#define zdmtc1         	326
#define zdmfc1   	327
#define zbad		329
#define n_asmcodes	329

#define frob	 0
#define fra	 1
#define fri	 2
#define frrr	 3
#define frri	 4
#define frr	 5
#define fa	 6
#define fr	 7
#define frrl	 8
#define frl	 9
#define fl	10
#define forrr	11
#define fril	12
#define fi	13
#if 0
#define flrr	14
#endif

#define set_undefined	 0
#define set_reorder	 1
#define set_noreorder	 2
#define set_macro	 3
#define set_nomacro	 4
#define set_at		 5
#define set_noat	 6
#define set_move	 7
#define set_nomove	 8
#define set_bopt	 9
#define set_nobopt	10

#define o_undefined	 0
#define o_optimize	 1
#define o_other		 2

#define opt_none	 0
#define opt_int		 1
#define opt_float	 2
#define opt_string	 3

#define binasm_record_length 16

#ifndef REG_SET_C
#define REG_SET_C 1
typedef unsigned regset[2];
#endif /* REG_SET_C */

typedef union {
  char data[binasm_record_length];
  struct {
    asmsym symno;
    unsigned fill0a : 10;
    unsigned asmtype : 6;
  } common;
  struct {
    asmsym symno;
    unsigned fill0a : 10;
    unsigned asmtype : 6;
    unsigned lexlev;
  } ent;
  struct {
    asmsym symno;
    unsigned fill0a : 10;
    unsigned asmtype : 6;
    asmint frameoffset;
    asmreg framereg : 7;
    asmreg pcreg : 7;
  } frame;
  struct {
    asmsym symno;
    unsigned fill0a : 10;
    unsigned asmtype : 6;
    unsigned regmask;
    asmint regoffset;
  } mask;
  /* 9/21/89 add ".gjaldef", ".gjallive", ".gjrlive" directive */
  struct {
    asmsym symno;
    unsigned fill0a : 10;
    unsigned asmtype : 6;
    regset gjmask;
  } gmask;
  struct {
    asmsym symno;
    unsigned fill0a : 10;
    unsigned asmtype : 6;
    int majornumber;
    int minornumber;
  } verstamp;
  struct {
    asmsym symno;
    unsigned fill0a : 10;
    unsigned asmtype : 6;
    unsigned filenumber;
    unsigned linenumber;
  } loc;
  struct {
    asmsym symno;
    unsigned fill0a : 10;
    unsigned asmtype : 6;
    unsigned length;
    unsigned short repeat;
  } chars;
  struct {
    asmsym symno;
    unsigned fill0a : 10;
    unsigned asmtype : 6;
    asmint expression;
    unsigned short repeat;
  } value;
  struct {
    asmsym symno;
    unsigned fill0a : 10;
    unsigned asmtype : 6;
    /*** fill in ***/
  } option;
  struct {
    asmsym symno;
    unsigned fill0a : 10;
    unsigned asmtype : 6;
    asmreg basereg1 : 7;
    asmreg basereg2 : 7;
  } alias;
  struct {
    asmsym symno;
    unsigned fill0a : 10;
    unsigned asmtype : 6;
    unsigned fill03 : 6;
    asmopcode op : 16;		/* 9 bits plus alignment in pascal */
    asmreg reg1 : 7;
    asmreg reg2 : 7;
    asmformat form : 4;
    asmreg reg3 : 7;
  } rinst;
  struct {
    asmsym symno;
    unsigned fill0a : 10;
    unsigned asmtype : 6;
    unsigned fill03 : 6;
    asmopcode op : 16;		/* 9 bits plus alignment in pascal */
    asmreg reg1 : 7;
    asmreg reg2 : 7;
    asmformat form : 4;
    asmint immediate;
  } iinst;
} binasm;

#endif /* __LANGUAGE_C */
