# define SEOS 1
# define SCOMMENT 2
# define SLABEL 3
# define SUNKNOWN 4
# define SHOLLERITH 5
# define SSTRING 6
# define SICON 7
# define SRCON 8
# define SDCON 9
# define SBITCON 10
# define SOCTCON 11
# define SHEXCON 12
# define STRUE 13
# define SFALSE 14
# define SNAME 15
# define SNAMEEQ 16
# define SFIELD 17
# define SSCALE 18
# define SINCLUDE 19
# define SLET 20
# define SASSIGN 21
# define SAUTOMATIC 22
# define SBACKSPACE 23
# define SBLOCK 24
# define SCALL 25
# define SCHARACTER 26
# define SCLOSE 27
# define SCOMMON 28
# define SCOMPLEX 29
# define SCONTINUE 30
# define SDATA 31
# define SDCOMPLEX 32
# define SDIMENSION 33
# define SDO 34
# define SDOUBLE 35
# define SELSE 36
# define SELSEIF 37
# define SEND 38
# define SENDFILE 39
# define SENDIF 40
# define SENTRY 41
# define SEQUIV 42
# define SEXTERNAL 43
# define SFORMAT 44
# define SFUNCTION 45
# define SGOTO 46
# define SASGOTO 47
# define SCOMPGOTO 48
# define SARITHIF 49
# define SLOGIF 50
# define SIMPLICIT 51
# define SINQUIRE 52
# define SINTEGER 53
# define SINTRINSIC 54
# define SLOGICAL 55
# define SNAMELIST 56
# define SOPEN 57
# define SPARAM 58
# define SPAUSE 59
# define SPRINT 60
# define SPROGRAM 61
# define SPUNCH 62
# define SREAD 63
# define SREAL 64
# define SRETURN 65
# define SREWIND 66
# define SSAVE 67
# define SSTATIC 68
# define SSTOP 69
# define SSUBROUTINE 70
# define STHEN 71
# define STO 72
# define SUNDEFINED 73
# define SWRITE 74
# define SLPAR 75
# define SRPAR 76
# define SEQUALS 77
# define SCOLON 78
# define SCOMMA 79
# define SCURRENCY 80
# define SPLUS 81
# define SMINUS 82
# define SSTAR 83
# define SSLASH 84
# define SPOWER 85
# define SCONCAT 86
# define SAND 87
# define SOR 88
# define SNEQV 89
# define SEQV 90
# define SNOT 91
# define SEQ 92
# define SLT 93
# define SGT 94
# define SLE 95
# define SGE 96
# define SNE 97

# line 98 "gram.in"
 
#ifndef lint
static	char	*sccsid = "@(#)gram.head	4.1	(ULTRIX)	7/17/90";
#endif lint

# line 162 "gram.in"
#	include "defs.h"
#	include "data.h"

#ifdef SDB
#	include <a.out.h>

#	ifndef N_SO
#		include <stab.h>
#	endif
#endif

static int equivlisterr;
static int do_name_err;
static int nstars;
static int ndim;
static int vartype;
static ftnint varleng;
static struct { expptr lb, ub; } dims[MAXDIM+1];
static struct Labelblock *labarray[MAXLABLIST];
static int lastwasbranch = NO;
static int thiswasbranch = NO;
extern ftnint yystno;
extern flag intonly;

ftnint convci();
double convcd();
expptr mklogcon(), mkaddcon(), mkrealcon(), mkstrcon(), mkbitcon();
expptr mkcxcon();
struct Listblock *mklist();
struct Listblock *mklist();
struct Impldoblock *mkiodo();
struct Extsym *comblock();


# line 199 "gram.in"
typedef union 	{
	int ival;
	char *charpval;
	chainp chval;
	tagptr tagval;
	expptr expval;
	struct Labelblock *labval;
	struct Nameblock *namval;
	struct Eqvchain *eqvval;
	struct Extsym *extval;
	union  Vexpr *vexpval;
	struct ValList *drvals;
	struct Vlist *dvals;
	union  Delt *deltp;
	struct Rpair *rpairp;
	struct Elist *elistp;
	} YYSTYPE;
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256
short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 20,
	1, 31,
	-2, 255,
-1, 42,
	1, 111,
	-2, 255,
-1, 144,
	1, 271,
	-2, 219,
-1, 164,
	1, 292,
	79, 292,
	-2, 219,
-1, 219,
	78, 210,
	-2, 176,
-1, 242,
	75, 255,
	-2, 252,
-1, 268,
	1, 313,
	-2, 180,
-1, 272,
	1, 322,
	79, 322,
	-2, 182,
-1, 322,
	78, 211,
	-2, 178,
-1, 352,
	1, 294,
	15, 294,
	75, 294,
	79, 294,
	-2, 220,
-1, 430,
	78, 131,
	-2, 128,
-1, 452,
	92, 0,
	93, 0,
	94, 0,
	95, 0,
	96, 0,
	97, 0,
	-2, 190,
-1, 469,
	1, 316,
	79, 316,
	-2, 180,
-1, 471,
	1, 318,
	79, 318,
	-2, 180,
-1, 473,
	1, 320,
	79, 320,
	-2, 180,
-1, 530,
	79, 316,
	-2, 180,
	};
# define YYNPROD 327
# define YYLAST 1275
short yyact[]={

 233, 271, 504, 428, 483, 434, 429, 404, 398, 431,
 403, 390, 482, 351, 389, 350, 218, 418, 302, 528,
 320, 263, 287, 315, 225, 197, 270, 249,  99, 441,
 440, 214, 212, 121, 230, 290,   5,  17, 267, 210,
 192, 199, 113, 201,  94, 173, 186, 190, 437, 105,
 406, 328, 329, 330, 330, 232, 255, 256, 257,  95,
  96,  97, 185, 103, 184, 514, 228, 510, 511, 385,
 497, 128, 114, 257, 174, 496, 151, 299, 151, 112,
 123, 124, 125, 126, 127, 180, 436, 261, 158, 159,
 328, 329, 330, 101, 215, 103, 275, 535, 508, 509,
 158, 159, 328, 329, 330, 336, 335, 334, 333, 332,
 269, 337, 339, 338, 341, 340, 342, 466, 499, 498,
 122, 489, 158, 159, 255, 256, 257, 258, 576, 575,
 310, 149, 185, 149, 184, 309, 226, 236, 236, 554,
 557, 175,  98, 178, 508, 509, 100, 183, 151, 103,
 103, 254, 151, 240, 438, 490, 235, 237, 568, 507,
 432, 433, 151, 289, 574, 205, 203, 506, 209, 477,
 204, 176, 177, 151, 208, 476, 100, 213, 239, 158,
 159, 328, 329, 330, 336, 335, 334, 213, 475, 365,
 337, 339, 338, 341, 340, 342, 158, 159, 255, 256,
 257, 272, 272, 149, 305, 306, 274, 149, 468, 265,
 459, 285, 292, 262, 443, 304, 444, 149, 289, 318,
 143, 322, 284, 294, 324, 439, 559, 327, 149, 462,
 371, 344, 298, 268, 268, 346, 347, 327, 370, 505,
 348, 343, 506, 312, 311, 369, 254, 487, 317, 151,
 488, 300, 308, 283, 356, 151, 151, 151, 151, 151,
 254, 254, 281, 282, 266, 316, 276, 277, 278, 167,
 480, 349, 327, 481, 463, 344, 395, 462, 386, 396,
 254, 387, 110, 130, 379, 364, 109, 380, 562,   4,
 158, 159, 255, 256, 257, 258, 345, 160, 162, 166,
 374, 108, 102, 107, 149, 106, 327, 327, 272, 344,
 149, 149, 149, 149, 149, 400, 430, 104, 353, 327,
 378, 354, 247, 327, 381, 327, 394, 397, 226, 448,
 449, 450, 451, 452, 453, 454, 455, 456, 457, 384,
 372, 426, 217, 425, 409, 327, 565, 327, 327, 327,
 366, 447, 564, 367, 368, 151, 254, 327, 405, 213,
 254, 254, 254, 254, 254, 325, 563, 558, 326, 377,
 219, 313, 562, 555, 345, 561, 375, 376, 102, 102,
 102, 102, 464, 179, 373, 484, 556, 467, 182, 495,
 187, 188, 189, 436, 442, 520, 195, 198, 144, 492,
 164, 215, 427, 479, 272, 272, 272, 187, 206, 207,
 149, 461, 478, 103, 215, 491, 313, 291, 150, 181,
 297, 243, 494, 352, 242, 238, 244, 493, 158, 159,
 255, 256, 257, 258, 501, 503, 469, 471, 473, 222,
 219, 515, 512, 513, 191, 236, 522, 202, 200, 327,
 327, 327, 327, 327, 327, 327, 327, 327, 327, 519,
 523, 438, 102, 517, 521, 288, 254, 500, 502, 161,
 518, 317, 134, 265, 216, 323, 526, 525, 103, 231,
 470, 472, 474, 484, 303, 327, 345, 391, 516, 187,
 467, 293, 527, 327, 534, 531, 532, 533,  30, 241,
 221, 537,  92, 536, 541,   6, 543, 546, 542, 547,
 272, 272, 272, 539, 548, 246, 551, 552, 549, 550,
 553,  81, 236, 327, 129,  80, 524,  79,  78, 560,
 163, 116, 409, 409, 409,  77,  76, 567,  75,  60,
 566,  48, 530, 471, 473,  47, 405, 544, 544, 545,
 545, 415, 423, 424,  44, 148,  32, 148, 570, 569,
 138, 327,  42, 111, 573, 194, 245, 388, 327, 193,
 259, 571, 383, 382, 327, 301, 187, 196, 578, 327,
 260,  26,  25, 393, 198,  24, 470, 472, 474, 577,
  23, 279, 460,  22,  21,   9,   8, 158, 159, 328,
 329, 330, 336, 335, 334, 333, 332, 445, 337, 339,
 338, 341, 340, 342, 158, 159, 255, 256, 257, 258,
 158, 159, 328, 329, 330, 336,   7, 148,   2, 211,
 314, 148, 158, 159, 328, 329, 330, 336, 335, 402,
 435, 148, 264, 337, 339, 338, 341, 340, 342, 407,
 414, 408, 148, 154, 295, 155, 156, 157,  20,  50,
 572, 152, 153, 103, 288, 286, 224, 355, 321, 319,
 392, 296,  91, 359, 360, 361, 362, 363, 458, 252,
 303,  52, 331, 158, 159, 328, 329, 330, 336, 335,
 334, 333, 332,  19, 337, 339, 338, 341, 340, 342,
 446,  54,  36, 220,   3, 158, 159, 328, 329, 330,
 336, 335, 334, 333, 332,   1, 337, 339, 338, 341,
 340, 342,   0, 229,   0,   0,   0,   0, 148, 158,
 159, 357,   0, 358, 148, 148, 148, 148, 148, 234,
   0,   0, 264,   0,   0, 264, 264,   0,   0,   0,
   0,   0, 529, 158, 159, 328, 329, 330, 336, 529,
 529, 529,   0,  12, 337, 339, 338, 341, 340, 342,
   0,   0,   0, 465, 538,   0,   0, 540,  10,  56,
  45,  73,  85,  14,  61,  70,  90,  37,  66,  46,
  55,  68,  72,  31,  67,  34,  33,  11,  87,  35,
  18,  40,  38,  28,  16,  57,  58,  59,  49,  53,
  41,  88,  64,  39,  69,  43,  89,  29,  62,  84,
  13,   0,  82,  65,  51,  86,  27,  74,  63,  15,
 399,   0,  71,  83, 148, 158, 159, 328, 329, 330,
 336, 335, 334, 333, 332, 264, 337, 339, 338, 341,
 340, 342,   0,  93,   0,   0,   0, 158, 159, 328,
 329, 330, 336, 335, 334, 333, 332, 486, 337, 339,
 338, 341, 340, 342, 154,   0, 155, 156, 157,   0,
   0,   0, 152, 153, 103, 115,   0, 118, 119, 120,
   0,   0,   0,   0,   0,   0,   0,   0,   0, 131,
 132,   0,   0, 133,   0, 135, 136, 137,   0,   0,
   0, 139, 140, 141, 154, 142, 155, 156, 157,   0,
   0,   0, 152, 153, 103,   0,   0,   0,   0,   0,
   0,   0,   0,   0, 168, 169, 170, 171, 172,   0,
   0,   0,   0,   0, 229, 223,   0,   0, 264,   0,
 158, 159, 227, 154,   0, 155, 156, 157,   0,   0,
 234, 152, 153, 103, 154,   0, 155, 156, 157,   0,
   0,   0, 152, 153, 103,   0,   0, 154,   0, 155,
 156, 157,   0,   0, 229, 152, 153, 103,   0,   0,
 158, 159, 485,   0, 154,   0, 155, 156, 157,   0,
 234,   0, 152, 153, 103, 154,   0, 155, 156, 157,
   0,   0,   0, 152, 153, 103, 154,   0, 155, 156,
 157,   0,   0, 229, 152, 153, 103, 253,   0, 158,
 159, 227,   0,   0, 229,   0,   0,   0,   0, 234,
 158, 159, 401,   0,   0,   0,   0, 229,   0,   0,
 234,   0,   0, 158, 159,   0,   0, 154,   0, 155,
 156, 157,   0, 234, 273, 152, 153, 103,   0,   0,
 158, 159,   0,   0,   0, 307,   0,   0,   0,   0,
 234, 158, 159,   0,   0,   0, 248,   0,   0,   0,
   0, 234, 158, 159, 250,   0, 251, 412, 413, 415,
 423, 424, 422, 421, 420, 410, 411, 215, 154,   0,
 155, 156, 157,   0,   0,   0, 152, 153, 103,   0,
   0,   0,   0,   0,   0,   0,   0, 147,   0,   0,
   0,   0,   0, 158, 159, 145,   0, 146,   0,   0,
   0,   0,   0,  56,  45,   0,  85,   0,  61,   0,
  90,   0,   0,  46,   0, 154,   0, 155, 156, 157,
   0,   0,  87, 152, 153, 103,   0, 419,   0,  57,
  58,  59,  49, 416, 417,  88,   0,   0, 248,   0,
  89,   0,  62,  84, 158, 159,  82,   0,  51,  86,
   0,   0,  63,   0, 117,   0, 154,  83, 155, 156,
 157,   0,   0,  73, 152, 153, 103,  70,   0,   0,
  66,   0,   0,  68,  72,   0,  67,   0,   0,   0,
   0,   0,   0,   0,   0, 248,   0,   0,   0,   0,
   0, 158, 159, 280,  64,   0,  69,   0,   0,   0,
   0,   0,   0,   0,   0,  65,   0,   0,   0,  74,
   0,   0,   0,   0,  71,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0, 147,   0,   0,   0,
   0,   0, 158, 159, 165 };
short yypact[]={

-1000,  33, 504, 759,-1000,-1000,-1000,-1000,-1000,-1000,
 497,-1000,-1000,-1000,-1000,-1000,-1000,  97, 463, 238,
 226, 224, 222, 207, 203,-1000,  -5,-1000,-1000,-1000,
-1000,-1000,1123,-1000,-1000,-1000,  37,-1000,-1000,-1000,
-1000,-1000,  67,-1000, 463,-1000,-1000,-1000,-1000,-1000,
 397,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,1052, 394,1191, 394, 190,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000, 463, 463, 463, 463,-1000, 463,
-1000, 344,-1000,-1000, 463, -22, 463, 463, 463, 369,
-1000,-1000, 463,  48, 373,-1000,-1000,-1000, 372,-1000,
-1000,-1000,-1000,  48, 463, 463, 369,-1000, 399, 265,
 365, 493,-1000, 364, 869, 972, 972, 350, 399, 492,
 463, 346, 463,-1000,-1000,-1000,-1000,1011,-1000,-1000,
 533,1103,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,1011, 134, 185,-1000,-1000, 989, 989,-1000,-1000,
-1000,-1000,1150,-1000,-1000,-1000, 344, 344, 463,-1000,
-1000, 135, 342,  67,-1000, 476,-1000, 342,-1000,-1000,
-1000, 463,-1000, 345,1181,  -7, 172,-1000,-1000,-1000,
 463, 492, 972,1000,-1000, 463,-1000,-1000,-1000,-1000,
-1000,  51,-1000, 341,-1000,-1000, 399, 972,-1000, 972,
 403,-1000, 972,-1000, 289,-1000, 776, 492,-1000, 972,
-1000,-1000,-1000, 972, 972,-1000, 776,-1000, 972,-1000,
-1000,-1000,  67, 492,-1000, 347, 242,-1000,1103,-1000,
-1000,-1000, 648,-1000,1103,1103,1103,1103,1103, -12,
 209, 110, 398,-1000,-1000, 398, 398,-1000, 166, 159,
 151, 776,-1000, 989,-1000,-1000,-1000,-1000,-1000, 533,
-1000,-1000,-1000, 344, 342,-1000, 208,-1000,-1000,-1000,
  37,-1000, 463, -15,-1000, 202,-1000, 472,-1000, 463,
  48, 200,-1000, 250,-1000, 754, 776, 959,-1000,1092,
 399, 327,-1000,  79, 146,-1000, 341,-1000, 776, 318,
 136, 137, 776, 463, 624,-1000, 948, 972, 972, 972,
 972, 972, 972, 972, 972, 972, 972,-1000,-1000,-1000,
-1000,-1000,-1000,-1000, 602, 131, -31, 672, 516, 336,
 198,-1000,-1000,-1000,1011,  41, 776,-1000,-1000, -27,
 -12, -12, -12, 115,-1000, 398, 110, 129, 110, 989,
 989, 989, 109,  96,  90,-1000,-1000,-1000,  37,-1000,
  80,-1000, 194, 909,-1000,-1000,-1000, 463, 171,-1000,
  39,-1000,  76,-1000,-1000,-1000, 463, 972,  67, 351,
-1000, 313,  -9,-1000,-1000,  36,  35,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000, 544, 544,-1000, 972,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,  79, 163,  81,
  17, -16, 386, 386,-1000, -20,-1000,-1000,  79, 399,
 327,-1000, 320, 972, 972,-1000, 492,-1000, -32, -31,
 -31, -31, 539,  98,  98, 551, 672,   7,-1000, 972,
-1000, 492, 492,  67,-1000, 533,-1000,-1000, 398,-1000,
-1000,-1000,-1000,-1000,-1000, 989, 989, 989,-1000,-1000,
-1000, 909,-1000,-1000,  19,-1000,-1000,-1000, 472, 472,
 463,-1000, 776, 463,-1000,-1000,-1000,1092,1092,1092,
-1000,-1000,-1000,-1000,  17,-1000,  79,  79, 386, 386,
 386, 386, -16, -16, 386,  63, 296,-1000,-1000,-1000,
 972, 310, 776,  61, 291, 150,-1000, 972, 299, 295,
 291, 290, 276, 270,-1000, 909,-1000,-1000,-1000,-1000,
 211,-1000,-1000,-1000,-1000,-1000,-1000,  17,  82, -16,
 -16,-1000,-1000,-1000,-1000,  79,-1000, 492,-1000,-1000,
 776,-1000, 972,-1000,-1000,-1000,-1000, 776,-1000,  88,
  17,  50,  49, 776,-1000, 492, 972,-1000, 776 };
short yypgo[]={

   0, 715, 704,  13, 703, 110,  21,  33,  37, 702,
 701, 693,  11,   0, 682, 681, 679, 672, 670, 669,
 668,  19, 666,  85, 665, 660,  87,  38,  26,  16,
 283,  22, 659,  46,  74,  45, 479,   1,  66,  20,
 418, 322,   4,  24,  34,  55, 658,  41, 654,   7,
 651,  17, 650,  50, 649,  48, 640,   5,   9,   2,
   6,  10, 639,   3,  30,  23,  31,  32,  29, 630,
 629, 628, 626, 596, 595, 853,  44, 594, 593, 590,
 585, 582, 581,  42, 577, 575,  28,  35,  43,   8,
  47,  25,  18, 573, 572,  12,  15,  40, 569, 567,
 565,  14, 563, 562, 560,  39, 498, 556, 554, 545,
 541, 539, 538, 220, 536, 535, 530, 528, 527, 525,
  96, 521, 515,  27 };
short yyr1[]={

   0,   1,   1,  71,  71,  71,  71,  71,  71,  71,
   2,  72,  72,  72,  72,  72,  72,  76,  34,  30,
  35,  35,  23,  23,  23,  24,  24,  31,  31,  17,
  73,  73,  73,  73,  73,  73,  73,  73,  73,  73,
  73,  73,  11,  11,   8,   9,  10,  10,  10,  10,
  10,  10,  10,  10,  10,  10,  10,   7,   7,   7,
  46,  46,  46,  46,  47,  47,  77,  77,  78,  78,
  79,  79,  90,  48,  48,  84,  84,  91,  91,  85,
  85,  92,  33,  87,  87,  94,  93,  93,  95,  95,
  42,  42,  96,  96,   3,  80,  80,  97, 100,  98,
  99,  99, 101, 101,  12,  82,  82, 102,  18,  18,
  83,  81, 103, 103, 104, 105,  70,  70,  62,  62,
  67,  67,  67,  67,  67,  55,  64,  68,  63,  63,
  60,  60,  66,  69,  69,  65,  65,  65,  65,  61,
  61,  61,  49,  49,  49,  49,  49,  49,  49,  49,
  49,  54,  54,  54,  53,  50,  50,  50,  51,  51,
  52,  52,  52,  59,  59,  59,  59,  59,  58,  58,
  58,  57,  57,  56,  56,  56,  19,  19,  20,  20,
  37,  37,  37,  38,  38,  38,  38,  38,  38,  38,
  38,  38,  38,  38,  38,  38,  38,  13,  13,  14,
  14,  14,  14,  14,  14,  36,  36,  36,  36,  29,
  39,  39,  45,  45,  45,  45,  45,  45,  44,  40,
  40,  41,  41,  41,  41,  41,  41,  41,  41,  74,
  74,  74,  74,  74,  74,  74, 107,  21, 106, 106,
 106, 106, 106, 106, 106, 106, 106, 106, 106,   4,
 108, 109, 109, 109, 109,  86,  86,  32,  22,  22,
  43,  43,  15,  15,  25,  25,  75,  88,  89, 110,
 111, 111, 111, 111, 111, 111, 111, 111, 111, 111,
 111, 111, 111, 111, 112, 119, 119, 119, 114, 121,
 121, 121, 116, 116, 113, 113, 122, 122, 123, 123,
 123, 123, 123, 123,  16, 115, 117, 118, 118,  26,
  26,   6,   6,  27,  27,  27,  28,  28,  28,  28,
  28,  28,   5,   5,   5,   5, 120 };
short yyr2[]={

   0,   0,   3,   2,   2,   2,   3,   3,   2,   1,
   1,   3,   3,   4,   4,   5,   3,   0,   1,   1,
   0,   1,   0,   2,   3,   1,   3,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   2,   3,
   1,   5,   6,   5,   2,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   0,   4,   6,
   3,   4,   5,   3,   1,   3,   3,   3,   3,   3,
   3,   3,   3,   1,   3,   1,   3,   1,   1,   1,
   3,   3,   2,   0,   3,   0,   2,   3,   1,   3,
   1,   1,   1,   3,   1,   3,   3,   4,   0,   2,
   1,   3,   1,   3,   1,   1,   2,   4,   1,   3,
   0,   1,   3,   3,   0,   4,   1,   3,   1,   3,
   1,   2,   2,   3,   1,   1,   3,   5,   1,   3,
   0,   1,   7,   1,   3,   2,   2,   3,   1,   1,
   3,   3,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   2,   2,   1,   1,   2,   2,   1,   1,
   1,   1,   1,   1,   2,   2,   3,   3,   1,   3,
   3,   1,   3,   1,   1,   3,   0,   1,   1,   3,
   1,   3,   1,   1,   1,   3,   3,   3,   3,   2,
   3,   3,   3,   3,   3,   2,   3,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   2,   4,   5,   5,
   0,   1,   1,   1,   1,   1,   1,   1,   5,   1,
   3,   1,   1,   3,   3,   3,   3,   2,   3,   1,
   7,   2,   2,   6,   2,   2,   5,   3,   4,   5,
   2,   1,   1,  10,   1,   3,   4,   3,   3,   1,
   1,   3,   3,   7,   7,   0,   1,   3,   1,   3,
   1,   2,   1,   1,   1,   3,   0,   0,   0,   1,
   2,   2,   2,   2,   2,   2,   2,   3,   4,   4,
   2,   3,   1,   3,   3,   1,   1,   1,   3,   1,
   1,   1,   1,   1,   3,   3,   1,   3,   1,   1,
   1,   2,   2,   2,   1,   3,   3,   4,   4,   1,
   3,   1,   5,   1,   1,   1,   3,   3,   3,   3,
   3,   3,   1,   5,   5,   5,   0 };
short yychk[]={

-1000,  -1, -71,  -2, 256,   3,   1, -72, -73, -74,
  19,  38,   4,  61,  24,  70,  45,  -8,  41, -11,
 -46, -77, -78, -79, -80, -81, -82,  67,  44,  58,
-106,  34,-107,  37,  36,  40,  -9,  28,  43,  54,
  42,  51,-103,  56,-108,  21,  30,-109,-110,  49,
 -32,  65, -15,  50, -10,  31,  20,  46,  47,  48,
-111,  25,  59,  69,  53,  64,  29,  35,  32,  55,
  26,  73,  33,  22,  68,-112,-114,-115,-117,-118,
-119,-121,  63,  74,  60,  23,  66,  39,  52,  57,
  27, -17,   5, -75, -76, -76, -76, -76,  45, -86,
  79, -34, -30,  15,  79, -86,  79,  79,  79,  79,
  79,-102,  84, -83, -83, -75,-106,  71, -75, -75,
 -75,  -7,  83, -83, -83, -83, -83, -83, -86, -36,
 -30, -75, -75, -75,  75, -75, -75, -75,-104, -75,
 -75, -75, -75,-113, -41,  83,  85,  75, -36, -45,
 -40, -13,  13,  14,   5,   7,   8,   9,  81,  82,
-113,  75,-113,-116, -41,  83,-113,  79, -75, -75,
 -75, -75, -75, -35, -34, -35, -34, -34, -76, -30,
 -23,  75, -30, -47,  86,  84, -33, -30, -30, -30,
 -90,  75, -97, -98,-100, -30, -84, -91, -30, -47,
  75, -88,  75, -88, -33, -47, -30, -30, -90, -97,
-105, -70, -67, -55, -66,  15,  75,  77, -29,  75,
  -4,   7,  75,  76, -22, -43, -37,  83, -38,  75,
 -44, -36, -45, -13,  91, -39, -37, -39,  75,-105,
  -3,   7, -30,  75, -30, -40,-122, -41,  75,-123,
  83,  85, -16,  16, -13,  83,  84,  85,  86, -40,
 -40, -26,  79,  -6, -36,  75,  79, -27, -38,  -5,
 -28, -37, -44,  75, -27,-120,-120,-120,-120, -40,
  83, -23, -23, -34, -83,  76, -24, -31, -30,  83,
 -87,  75, -86,  15, -87, -48, -36,  75,  -8,  84,
  79, -85, -92, -30,  -3, -37, -37,  75, -33,  84,
  79, -64, -68,  75, -69, -65, -55, -66, -37, -19,
 -39, -20, -37,  72, -37,  76,  79, -13,  83,  84,
  85, -14,  90,  89,  88,  87,  86,  92,  94,  93,
  96,  95,  97,  -3, -37, -38, -37, -37, -37, -86,
 -96,  -3,  76,  76,  79, -40, -37,  83,  85, -40,
 -40, -40, -40, -40,  76,  79, -26, -26, -26,  79,
  79,  79, -38,  -5, -28,-120,-120, -23, -87,  76,
  79,  -7, -93, -94, -33,  84,  76,  79, -99,-101,
 -12,  15, -18, -30, -91,  76,  79,  77, -89,  76,
 -89,  83, -62, -61, -49, -55, -53, -54, -50, -44,
  13,  14,   5,   6, -52,   7,  81,  82, -51,  75,
  12,  11,  10,   8,   9, -67, -68,  75, -63, -60,
 -59, -58,  81,  82, -57, -56,   7, -55,  75,  79,
 -64, -68,  76,  78,  79, -30,  76, -43, -37, -37,
 -37, -37, -37, -37, -37, -37, -37, -37,  76,  79,
  76,  75,  79,  76,-123, -40,  76,  -6,  79, -38,
  -5, -38,  -5, -38,  -5,  79,  79,  79,  -7, -31,
  76,  79, -95, -42, -37,  83, -36,  76,  79,  82,
  79, -92, -37, -86,  71,  76,  84,  79,  83,  83,
 -53, -51, -53, -51, -59,  76,  79,  78,  81,  82,
  83,  84, -58, -58,  85, -59, -55, -65, -68, -29,
  75, -39, -37,  -3, -38, -96,  -3, -86, -21, -30,
 -38, -21, -21, -21, -95,  78,-101, -12, -30, -21,
 -30, -89, -61, -49, -55, -53, -49, -59, -60, -58,
 -58, -57, -57, -57,  76,  77,  76,  79,  76,  76,
 -37,  76,  77,  76,  76,  76, -42, -37,  76, -63,
 -59,  -3, -25, -37,  76,  79,  79,  -3, -37 };
short yydef[]={

   1,  -2,   0,   0,   9,  10,   2,   3,   4,   5,
   0, 266,   8,  17,  17,  17,  17, 255,   0,  30,
  -2,  32,  33,  34,  35,  36,  37, 110,  40, 110,
 229, 266,   0, 266, 266, 266,  57, 110, 110, 110,
 110, 110,  -2, 105,   0, 266, 266, 241, 242, 266,
 244, 266, 266, 266,  45, 114, 250, 266, 266, 266,
 269, 266, 262, 263,  46,  47,  48,  49,  50,  51,
  52,  53,  54,  55,  56,   0,   0,   0,   0, 282,
 266, 266, 266, 266, 266, 285, 286, 287, 289, 290,
 291,   6,  29,   7,  20,  20,   0,   0,  17,   0,
 256,  22,  18,  19,   0,   0, 256,   0,   0,   0,
  98, 106,   0,  38,   0, 267, 231, 232,   0, 234,
 235,  44, 267,   0,   0,   0,   0,  98,   0,   0,
 205,   0, 240,   0,   0, 210, 210,   0,   0,   0,
   0,   0,   0, 270,  -2, 272, 273,   0, 221, 222,
   0,   0, 212, 213, 214, 215, 216, 217, 197, 198,
 274,   0, 275, 276,  -2, 293, 280,   0, 326, 326,
 326, 326,   0,  11,  21,  12,  22,  22,   0, 110,
  16,   0,  83, 255,  64,   0,  63,  83,  67,  69,
  71,   0,  96,   0,   0,   0,  39,  75,  77,  78,
   0,   0,   0,   0,  60,   0,  66,  68,  70,  95,
 113,   0, 116, 120, 124, 125,   0,   0, 206,  -2,
   0, 249,   0, 245,   0, 258, 260,   0, 180,   0,
 182, 183, 184,   0,   0, 247, 211, 248,   0, 112,
 251,  94,  -2,   0, 257, 298,   0, 219,   0, 296,
 299, 300,   0, 304,   0,   0,   0,   0,   0, 227,
 298, 277,   0, 309, 311,   0,   0, 281,  -2, 314,
 315,   0,  -2,   0, 283, 284, 288, 305, 306, 326,
 326,  13,  14,  22,  83,  23,   0,  25,  27,  28,
  57,  85,   0,   0,  82,   0,  73,   0,  99,   0,
   0,   0,  79,   0, 268,   0, 268,   0,  61,   0,
   0, 121, 122, 130,   0, 133,   0, 138, 238,   0,
   0, 177,  -2,   0,   0, 246,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0, 199, 200, 201,
 202, 203, 204, 261,   0, 180, 189, 195,   0,   0,
   0,  92,  -2, 295,   0,   0, 301, 302, 303, 223,
 224, 225, 226, 228, 294,   0, 279,   0, 278,   0,
   0,   0, 180,   0,   0, 307, 308,  15,  57,  24,
   0,  43,   0,   0,  62,  65,  72,   0,   0, 100,
 102, 104, 107, 108,  76,  41,   0,   0, 255,   0,
  58,   0,   0, 118, 139, 142, 151, 143, 144, 145,
 146, 147, 148, 149, 150, 154,   0,   0, 155,   0,
 160, 161, 162, 158, 159, 117, 123, 130,   0,   0,
  -2, 163,   0,   0, 168, 171, 173, 174,   0,   0,
 135, 136, 207, 210,   0, 239,   0, 259, 185, 186,
 187, 188,  -2, 191, 192, 193, 194, 196, 181,   0,
 236,   0,   0, 255, 297, 298, 220, 310,   0,  -2,
 317,  -2, 319,  -2, 321,   0,   0,   0,  42,  26,
  84,   0,  86,  88,  91,  90,  74,  97,   0,   0,
   0,  80,  81,   0, 233, 268, 115,   0,   0,   0,
 152, 156, 153, 157, 131, 126,   0, 130,   0,   0,
   0,   0, 164, 165,   0,   0,   0, 134, 137, 208,
 210,   0, 179,   0, 180,   0,  93,   0,   0, 205,
  -2,   0,   0,   0,  87,   0, 101, 103, 109, 230,
   0,  59, 119, 140, 142, 151, 141, 129,   0, 166,
 167, 169, 170, 172, 175,   0, 209,   0, 218, 253,
 254, 312,   0, 323, 324, 325,  89,  91, 127,   0,
 128,   0, 237, 264, 132,   0,   0, 243, 265 };
#ifndef lint
static char yaccpar_sccsid[] = "@(#)yaccpar	4.1	(Berkeley)	2/11/83";
#endif

#
# define YYFLAG -1000
# define YYERROR goto yyerrlab
# define YYACCEPT return(0)
# define YYABORT return(1)

/*	parser for yacc output	*/

#ifdef YYDEBUG
int yydebug = 0; /* 1 for debugging */
#endif
YYSTYPE yyv[YYMAXDEPTH]; /* where the values are stored */
int yychar = -1; /* current input token number */
int yynerrs = 0;  /* number of errors */
short yyerrflag = 0;  /* error recovery flag */

yyparse() {

	short yys[YYMAXDEPTH];
	short yyj, yym;
	register YYSTYPE *yypvt;
	register short yystate, *yyps, yyn;
	register YYSTYPE *yypv;
	register short *yyxi;

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyps= &yys[-1];
	yypv= &yyv[-1];

 yystack:    /* put a state and value onto the stack */

#ifdef YYDEBUG
	if( yydebug  ) printf( "state %d, char 0%o\n", yystate, yychar );
#endif
		if( ++yyps> &yys[YYMAXDEPTH] ) { yyerror( "yacc stack overflow" ); return(1); }
		*yyps = yystate;
		++yypv;
		*yypv = yyval;

 yynewstate:

	yyn = yypact[yystate];

	if( yyn<= YYFLAG ) goto yydefault; /* simple state */

	if( yychar<0 ) if( (yychar=yylex())<0 ) yychar=0;
	if( (yyn += yychar)<0 || yyn >= YYLAST ) goto yydefault;

	if( yychk[ yyn=yyact[ yyn ] ] == yychar ){ /* valid shift */
		yychar = -1;
		yyval = yylval;
		yystate = yyn;
		if( yyerrflag > 0 ) --yyerrflag;
		goto yystack;
		}

 yydefault:
	/* default state action */

	if( (yyn=yydef[yystate]) == -2 ) {
		if( yychar<0 ) if( (yychar=yylex())<0 ) yychar = 0;
		/* look through exception table */

		for( yyxi=yyexca; (*yyxi!= (-1)) || (yyxi[1]!=yystate) ; yyxi += 2 ) ; /* VOID */

		while( *(yyxi+=2) >= 0 ){
			if( *yyxi == yychar ) break;
			}
		if( (yyn = yyxi[1]) < 0 ) return(0);   /* accept */
		}

	if( yyn == 0 ){ /* error */
		/* error ... attempt to resume parsing */

		switch( yyerrflag ){

		case 0:   /* brand new error */

			yyerror( "syntax error" );
		yyerrlab:
			++yynerrs;

		case 1:
		case 2: /* incompletely recovered error ... try again */

			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */

			while ( yyps >= yys ) {
			   yyn = yypact[*yyps] + YYERRCODE;
			   if( yyn>= 0 && yyn < YYLAST && yychk[yyact[yyn]] == YYERRCODE ){
			      yystate = yyact[yyn];  /* simulate a shift of "error" */
			      goto yystack;
			      }
			   yyn = yypact[*yyps];

			   /* the current yyps has no shift onn "error", pop stack */

#ifdef YYDEBUG
			   if( yydebug ) printf( "error recovery pops state %d, uncovers %d\n", *yyps, yyps[-1] );
#endif
			   --yyps;
			   --yypv;
			   }

			/* there is no state on the stack with an error shift ... abort */

	yyabort:
			return(1);


		case 3:  /* no shift yet; clobber input char */

#ifdef YYDEBUG
			if( yydebug ) printf( "error recovery discards char %d\n", yychar );
#endif

			if( yychar == 0 ) goto yyabort; /* don't discard EOF, quit */
			yychar = -1;
			goto yynewstate;   /* try again in the same state */

			}

		}

	/* reduction by production yyn */

#ifdef YYDEBUG
		if( yydebug ) printf("reduce %d\n",yyn);
#endif
		yyps -= yyr2[yyn];
		yypvt = yypv;
		yypv -= yyr2[yyn];
		yyval = yypv[1];
		yym=yyn;
			/* consult goto table to find next state */
		yyn = yyr1[yyn];
		yyj = yypgo[yyn] + *yyps + 1;
		if( yyj>=YYLAST || yychk[ yystate = yyact[yyj] ] != -yyn ) yystate = yyact[yypgo[yyn]];
		switch(yym){
			
case 3:
# line 259 "gram.in"
{ lastwasbranch = NO; } break;
case 5:
# line 262 "gram.in"
{ if(yypvt[-1].labval && (yypvt[-1].labval->labelno==dorange))
			enddo(yypvt[-1].labval->labelno);
		  if(lastwasbranch && thislabel==NULL)
			warn("statement cannot be reached");
		  lastwasbranch = thiswasbranch;
		  thiswasbranch = NO;
		  if(yypvt[-1].labval)
			{
			if(yypvt[-1].labval->labtype == LABFORMAT)
				err("label already that of a format");
			else
				yypvt[-1].labval->labtype = LABEXEC;
			}
		  if(!optimflag)
			{
			argtemplist = hookup(argtemplist, activearglist);
			activearglist = CHNULL;
			}
		} break;
case 6:
# line 282 "gram.in"
{ doinclude( yypvt[-0].charpval ); } break;
case 7:
# line 284 "gram.in"
{ lastwasbranch = NO;  endproc(); } break;
case 8:
# line 286 "gram.in"
{ execerr("unclassifiable statement", CNULL);  flline(); } break;
case 9:
# line 288 "gram.in"
{ flline();  needkwd = NO;  inioctl = NO; 
		  yyerrok; yyclearin; } break;
case 10:
# line 293 "gram.in"
{
#ifdef SDB
		if( sdbflag )
			{
			linenostab(lineno);
			}
#endif

		if(yystno != 0)
			{
			yyval.labval = thislabel =  mklabel(yystno);
			if(thislabel->labdefined)
				execerr("label %s already defined",
					convic(thislabel->stateno) );
			else	{
				if(thislabel->blklevel!=0 && thislabel->blklevel<blklevel
				    && thislabel->labtype!=LABFORMAT)
					warn1("there is a branch to label %s from outside block",
					      convic( (ftnint) (thislabel->stateno) ) );
				thislabel->blklevel = blklevel;
				thislabel->labdefined = YES;
				}
			}
		else    yyval.labval = thislabel = NULL;
		} break;
case 11:
# line 321 "gram.in"
{startproc(yypvt[-0].namval, CLMAIN); } break;
case 12:
# line 323 "gram.in"
{ if(yypvt[-0].namval) NO66("named BLOCKDATA");
		  startproc(yypvt[-0].namval, CLBLOCK); } break;
case 13:
# line 326 "gram.in"
{ entrypt(CLPROC, TYSUBR, (ftnint) 0,  yypvt[-1].namval, yypvt[-0].chval); } break;
case 14:
# line 328 "gram.in"
{ entrypt(CLPROC, TYUNKNOWN, (ftnint) 0, yypvt[-1].namval, yypvt[-0].chval); } break;
case 15:
# line 330 "gram.in"
{ entrypt(CLPROC, yypvt[-4].ival, varleng, yypvt[-1].namval, yypvt[-0].chval); } break;
case 16:
# line 332 "gram.in"
{ if(parstate==OUTSIDE || procclass==CLMAIN
			|| procclass==CLBLOCK)
				execerr("misplaced entry statement", CNULL);
			entrypt(CLENTRY, 0, (ftnint) 0, yypvt[-1].namval, yypvt[-0].chval);
		} break;
case 17:
# line 340 "gram.in"
{ newproc(); } break;
case 19:
# line 347 "gram.in"
{ yyval.namval = mkname(toklen, token); } break;
case 20:
# line 350 "gram.in"
{ yyval.namval = NULL; } break;
case 22:
# line 355 "gram.in"
{ yyval.chval = 0; } break;
case 23:
# line 357 "gram.in"
{ NO66(" () argument list");
		  yyval.chval = 0; } break;
case 24:
# line 360 "gram.in"
{yyval.chval = yypvt[-1].chval; } break;
case 25:
# line 364 "gram.in"
{ yyval.chval = (yypvt[-0].namval ? mkchain(yypvt[-0].namval,CHNULL) : CHNULL ); } break;
case 26:
# line 366 "gram.in"
{ if(yypvt[-0].namval) yypvt[-2].chval = yyval.chval = hookup(yypvt[-2].chval, mkchain(yypvt[-0].namval,CHNULL)); } break;
case 27:
# line 370 "gram.in"
{ if((yypvt[-0].namval->vstg!=STGUNKNOWN && yypvt[-0].namval->vstg!=STGARG)
				|| (yypvt[-0].namval->vclass == CLPARAM) ) {
			dclerr("name declared as argument after use", yypvt[-0].namval);
			yyval.namval = NULL;
		  } else
			yypvt[-0].namval->vstg = STGARG;
		} break;
case 28:
# line 378 "gram.in"
{ NO66("altenate return argument");
		  yyval.namval = 0;  substars = YES; } break;
case 29:
# line 385 "gram.in"
{
		char *s;
		s = copyn(toklen+1, token);
		s[toklen] = '\0';
		yyval.charpval = s;
		} break;
case 38:
# line 472 "gram.in"
{ NO66("SAVE statement");
		  saveall = YES; } break;
case 39:
# line 475 "gram.in"
{ NO66("SAVE statement"); } break;
case 40:
# line 477 "gram.in"
{
		if (parstate == OUTSIDE)
			{
			newproc();
			startproc(PNULL, CLMAIN);
			parstate = INSIDE;
			}
		if (parstate < INDCL)
			parstate = INDCL;
		fmtstmt(thislabel);
		setfmt(thislabel);
		} break;
case 41:
# line 490 "gram.in"
{ NO66("PARAMETER statement"); } break;
case 42:
# line 494 "gram.in"
{ settype(yypvt[-3].namval, yypvt[-5].ival, yypvt[-0].ival);
		  if(ndim>0) setbound(yypvt[-3].namval,ndim,dims);
		} break;
case 43:
# line 498 "gram.in"
{ settype(yypvt[-2].namval, yypvt[-4].ival, yypvt[-0].ival);
		  if(ndim>0) setbound(yypvt[-2].namval,ndim,dims);
		} break;
case 44:
# line 504 "gram.in"
{ varleng = yypvt[-0].ival; } break;
case 45:
# line 508 "gram.in"
{ varleng = (yypvt[-0].ival<0 || yypvt[-0].ival==TYLONG ? 0 : typesize[yypvt[-0].ival]);
		  vartype = yypvt[-0].ival;
		} break;
case 46:
# line 513 "gram.in"
{ yyval.ival = TYLONG; } break;
case 47:
# line 514 "gram.in"
{ yyval.ival = dblflag ? TYDREAL : TYREAL; } break;
case 48:
# line 515 "gram.in"
{ yyval.ival = dblflag ? TYDCOMPLEX : TYCOMPLEX; } break;
case 49:
# line 516 "gram.in"
{ yyval.ival = TYDREAL; } break;
case 50:
# line 517 "gram.in"
{ NOEXT("DOUBLE COMPLEX statement"); yyval.ival = TYDCOMPLEX; } break;
case 51:
# line 518 "gram.in"
{ yyval.ival = TYLOGICAL; } break;
case 52:
# line 519 "gram.in"
{ NO66("CHARACTER statement"); yyval.ival = TYCHAR; } break;
case 53:
# line 520 "gram.in"
{ yyval.ival = TYUNKNOWN; } break;
case 54:
# line 521 "gram.in"
{ yyval.ival = TYUNKNOWN; } break;
case 55:
# line 522 "gram.in"
{ NOEXT("AUTOMATIC statement"); yyval.ival = - STGAUTO; } break;
case 56:
# line 523 "gram.in"
{ NOEXT("STATIC statement"); yyval.ival = - STGBSS; } break;
case 57:
# line 527 "gram.in"
{ yyval.ival = varleng; } break;
case 58:
# line 529 "gram.in"
{
		expptr p;
		int typlen;
		
		p = yypvt[-1].expval;
		NO66("length specification *n");
		if( ! ISICON(p) || p->constblock.const.ci<0 )
			{
			yyval.ival = 0;
			dclerr("- length must be a positive integer value",
				PNULL);
			}
		else if( dblflag )
			{
			typlen = p->constblock.const.ci;
			if( vartype == TYDREAL && typlen == 4 ) yyval.ival = 8;
			else if( vartype == TYDCOMPLEX && typlen == 8 ) yyval.ival = 16;
			else yyval.ival = typlen;
			}
		else
			yyval.ival = p->constblock.const.ci;
		} break;
case 59:
# line 552 "gram.in"
{ NO66("length specification *(*)"); yyval.ival = -1; } break;
case 60:
# line 556 "gram.in"
{ incomm( yyval.extval = comblock(0, CNULL) , yypvt[-0].namval ); } break;
case 61:
# line 558 "gram.in"
{ yyval.extval = yypvt[-1].extval;  incomm(yypvt[-1].extval, yypvt[-0].namval); } break;
case 62:
# line 560 "gram.in"
{ yyval.extval = yypvt[-2].extval;  incomm(yypvt[-2].extval, yypvt[-0].namval); } break;
case 63:
# line 562 "gram.in"
{ incomm(yypvt[-2].extval, yypvt[-0].namval); } break;
case 64:
# line 566 "gram.in"
{ yyval.extval = comblock(0, CNULL); } break;
case 65:
# line 568 "gram.in"
{ yyval.extval = comblock(toklen, token); } break;
case 66:
# line 572 "gram.in"
{ setext(yypvt[-0].namval); } break;
case 67:
# line 574 "gram.in"
{ setext(yypvt[-0].namval); } break;
case 68:
# line 578 "gram.in"
{ NO66("INTRINSIC statement"); setintr(yypvt[-0].namval); } break;
case 69:
# line 580 "gram.in"
{ setintr(yypvt[-0].namval); } break;
case 72:
# line 588 "gram.in"
{
		struct Equivblock *p;
		if(nequiv >= maxequiv)
			many("equivalences", 'q');
		if( !equivlisterr ) {
		   p  =  & eqvclass[nequiv++];
		   p->eqvinit = NO;
		   p->eqvbottom = 0;
		   p->eqvtop = 0;
		   p->equivs = yypvt[-1].eqvval;
		   p->init = NO;
		   p->initoffset = 0;
		   }
		} break;
case 73:
# line 605 "gram.in"
{ yyval.eqvval=ALLOC(Eqvchain);
		  equivlisterr = 0;
		  if( yypvt[-0].expval->tag == TCONST ) {
			equivlisterr = 1;
			dclerr( "- constant in equivalence", NULL );
		  }
		  yyval.eqvval->eqvitem.eqvlhs = (struct Primblock *)yypvt[-0].expval;
		} break;
case 74:
# line 614 "gram.in"
{ yyval.eqvval=ALLOC(Eqvchain);
		  if( yypvt[-0].expval->tag == TCONST ) {
			equivlisterr = 1;
			dclerr( "constant in equivalence", NULL );
		  }
		  yyval.eqvval->eqvitem.eqvlhs = (struct Primblock *) yypvt[-0].expval;
		  yyval.eqvval->eqvnextp = yypvt[-2].eqvval;
		} break;
case 77:
# line 630 "gram.in"
{ int k;
		  yypvt[-0].namval->vsave = YES;
		  k = yypvt[-0].namval->vstg;
		if( ! ONEOF(k, M(STGUNKNOWN)|M(STGBSS)|M(STGINIT)) 
				|| (yypvt[-0].namval->vclass == CLPARAM) )
			dclerr("can only save static variables", yypvt[-0].namval);
		} break;
case 78:
# line 638 "gram.in"
{ yypvt[-0].extval->extsave = 1; } break;
case 81:
# line 646 "gram.in"
{ paramset( yypvt[-2].namval, yypvt[-0].expval ); } break;
case 82:
# line 650 "gram.in"
{ if(ndim>0) setbound(yypvt[-1].namval, ndim, dims); } break;
case 83:
# line 655 "gram.in"
{ ndim = 0; } break;
case 85:
# line 659 "gram.in"
{ ndim = 0; } break;
case 88:
# line 664 "gram.in"
{ if(ndim == maxdim)
			err("too many dimensions");
		  else if(ndim < maxdim)
			{ dims[ndim].lb = 0;
			  dims[ndim].ub = yypvt[-0].expval;
			}
		  ++ndim;
		} break;
case 89:
# line 673 "gram.in"
{ if(ndim == maxdim)
			err("too many dimensions");
		  else if(ndim < maxdim)
			{ dims[ndim].lb = yypvt[-2].expval;
			  dims[ndim].ub = yypvt[-0].expval;
			}
		  ++ndim;
		} break;
case 90:
# line 684 "gram.in"
{ yyval.expval = 0; } break;
case 92:
# line 689 "gram.in"
{ nstars = 1; labarray[0] = yypvt[-0].labval; } break;
case 93:
# line 691 "gram.in"
{ if(nstars < MAXLABLIST)  labarray[nstars++] = yypvt[-0].labval; } break;
case 94:
# line 695 "gram.in"
{ yyval.labval = execlab( convci(toklen, token) ); } break;
case 95:
# line 699 "gram.in"
{ NO66("IMPLICIT statement"); } break;
case 98:
# line 706 "gram.in"
{ needkwd = 1; } break;
case 99:
# line 707 "gram.in"
{ vartype = yypvt[-0].ival; } break;
case 102:
# line 715 "gram.in"
{ setimpl(vartype, varleng, yypvt[-0].ival, yypvt[-0].ival); } break;
case 103:
# line 717 "gram.in"
{ setimpl(vartype, varleng, yypvt[-2].ival, yypvt[-0].ival); } break;
case 104:
# line 721 "gram.in"
{ if(toklen!=1 || token[0]<'a' || token[0]>'z')
			{
			dclerr("implicit item must be single letter", PNULL);
			yyval.ival = 0;
			}
		  else yyval.ival = token[0];
		} break;
case 107:
# line 735 "gram.in"
{
		if(yypvt[-2].namval->vclass == CLUNKNOWN)
			{
			yypvt[-2].namval->vclass = CLNAMELIST;
			yypvt[-2].namval->vtype = TYINT;
			yypvt[-2].namval->vstg = STGINIT;
			yypvt[-2].namval->varxptr.namelist = yypvt[-0].chval;
			yypvt[-2].namval->vardesc.varno = ++lastvarno;
			}
		else dclerr("cannot be a namelist name", yypvt[-2].namval);
		} break;
case 108:
# line 749 "gram.in"
{ yyval.chval = mkchain(yypvt[-0].namval, CHNULL); } break;
case 109:
# line 751 "gram.in"
{ yyval.chval = hookup(yypvt[-2].chval, mkchain(yypvt[-0].namval, CHNULL)); } break;
case 110:
# line 755 "gram.in"
{ switch(parstate)	
			{
			case OUTSIDE:	newproc();
					startproc(PNULL, CLMAIN);
			case INSIDE:	parstate = INDCL;
			case INDCL:	break;

			default:
				dclerr("declaration among executables", PNULL);
			}
		} break;
case 111:
# line 769 "gram.in"
{
	  if (overlapflag == YES)
	    warn("overlapping initializations");
	} break;
case 114:
# line 779 "gram.in"
{ if(parstate == OUTSIDE)
			{
			newproc();
			startproc(PNULL, CLMAIN);
			}
		  if(parstate < INDATA)
			{
			enddcl();
			parstate = INDATA;
			}
		  overlapflag = NO;
		} break;
case 115:
# line 794 "gram.in"
{ savedata(yypvt[-3].elistp, yypvt[-1].drvals); } break;
case 116:
# line 798 "gram.in"
{ yyval.elistp = preplval(NULL, yypvt[-0].deltp); } break;
case 117:
# line 800 "gram.in"
{ yyval.elistp = preplval(yypvt[-2].elistp, yypvt[-0].deltp); } break;
case 119:
# line 805 "gram.in"
{
			  yypvt[-0].drvals->next = yypvt[-2].drvals;
			  yyval.drvals = yypvt[-0].drvals;
			} break;
case 120:
# line 812 "gram.in"
{ yyval.deltp = mkdlval(yypvt[-0].vexpval, NULL, NULL); } break;
case 121:
# line 814 "gram.in"
{ yyval.deltp = mkdlval(yypvt[-1].vexpval, yypvt[-0].dvals, NULL); } break;
case 122:
# line 816 "gram.in"
{ yyval.deltp = mkdlval(yypvt[-1].vexpval, NULL, yypvt[-0].rpairp); } break;
case 123:
# line 818 "gram.in"
{ yyval.deltp = mkdlval(yypvt[-2].vexpval, yypvt[-1].dvals, yypvt[-0].rpairp); } break;
case 125:
# line 822 "gram.in"
{ yyval.vexpval = mkdname(toklen, token); } break;
case 126:
# line 826 "gram.in"
{ yyval.dvals = revvlist(yypvt[-1].dvals); } break;
case 127:
# line 830 "gram.in"
{ yyval.rpairp = mkdrange(yypvt[-3].vexpval, yypvt[-1].vexpval); } break;
case 128:
# line 834 "gram.in"
{
			  yyval.dvals = prepvexpr(NULL, yypvt[-0].vexpval);
			} break;
case 129:
# line 838 "gram.in"
{
			  yyval.dvals = prepvexpr(yypvt[-2].dvals, yypvt[-0].vexpval);
			} break;
case 130:
# line 843 "gram.in"
{ yyval.vexpval = NULL; } break;
case 131:
# line 844 "gram.in"
{ yyval.vexpval = yypvt[-0].vexpval; } break;
case 132:
# line 848 "gram.in"
{ yyval.deltp = mkdatado(yypvt[-5].elistp, yypvt[-3].vexpval, yypvt[-1].dvals); } break;
case 133:
# line 852 "gram.in"
{ yyval.elistp = preplval(NULL, yypvt[-0].deltp); } break;
case 134:
# line 854 "gram.in"
{ yyval.elistp = preplval(yypvt[-2].elistp, yypvt[-0].deltp); } break;
case 135:
# line 858 "gram.in"
{ yyval.deltp = mkdlval(yypvt[-1].vexpval, yypvt[-0].dvals, NULL); } break;
case 136:
# line 860 "gram.in"
{ yyval.deltp = mkdlval(yypvt[-1].vexpval, NULL, yypvt[-0].rpairp); } break;
case 137:
# line 862 "gram.in"
{ yyval.deltp = mkdlval(yypvt[-2].vexpval, yypvt[-1].dvals, yypvt[-0].rpairp); } break;
case 139:
# line 867 "gram.in"
{
			  static dvalue one = { DVALUE, NORMAL, 1 };

			  yyval.drvals = mkdrval(&one, yypvt[-0].expval);
			} break;
case 140:
# line 873 "gram.in"
{
			  yyval.drvals = mkdrval(yypvt[-2].vexpval, yypvt[-0].expval);
			  frvexpr(yypvt[-2].vexpval);
			} break;
case 141:
# line 878 "gram.in"
{
			  yyval.drvals = mkdrval(yypvt[-2].vexpval, yypvt[-0].expval);
			  frvexpr(yypvt[-2].vexpval);
			} break;
case 142:
# line 885 "gram.in"
{
			  yyval.expval = evparam(yypvt[-0].vexpval);
			  free((char *) yypvt[-0].vexpval);
			} break;
case 143:
# line 890 "gram.in"
{
			  yyval.expval = ivaltoicon(yypvt[-0].vexpval);
			  frvexpr(yypvt[-0].vexpval);
			} break;
case 146:
# line 897 "gram.in"
{ yyval.expval = mklogcon(1); } break;
case 147:
# line 898 "gram.in"
{ yyval.expval = mklogcon(0); } break;
case 148:
# line 899 "gram.in"
{ yyval.expval = mkstrcon(toklen, token); } break;
case 149:
# line 900 "gram.in"
{ yyval.expval = mkstrcon(toklen, token); } break;
case 152:
# line 906 "gram.in"
{ yyval.vexpval = yypvt[-0].vexpval; } break;
case 153:
# line 908 "gram.in"
{
			  yyval.vexpval = negival(yypvt[-0].vexpval);
			  frvexpr(yypvt[-0].vexpval);
			} break;
case 154:
# line 915 "gram.in"
{ yyval.vexpval = evicon(toklen, token); } break;
case 156:
# line 920 "gram.in"
{ yyval.expval = yypvt[-0].expval; } break;
case 157:
# line 922 "gram.in"
{
			  consnegop(yypvt[-0].expval);
			  yyval.expval = yypvt[-0].expval;
			} break;
case 158:
# line 928 "gram.in"
{ yyval.expval = mkrealcon(TYREAL, convcd(toklen, token)); } break;
case 159:
# line 929 "gram.in"
{ yyval.expval = mkrealcon(TYDREAL, convcd(toklen, token)); } break;
case 160:
# line 932 "gram.in"
{ yyval.expval = mkbitcon(4, toklen, token); } break;
case 161:
# line 933 "gram.in"
{ yyval.expval = mkbitcon(3, toklen, token); } break;
case 162:
# line 934 "gram.in"
{ yyval.expval = mkbitcon(1, toklen, token); } break;
case 164:
# line 939 "gram.in"
{ yyval.vexpval = yypvt[-0].vexpval; } break;
case 165:
# line 941 "gram.in"
{ yyval.vexpval = mkdexpr(OPNEG, NULL, yypvt[-0].vexpval); } break;
case 166:
# line 943 "gram.in"
{ yyval.vexpval = mkdexpr(OPPLUS, yypvt[-2].vexpval, yypvt[-0].vexpval); } break;
case 167:
# line 945 "gram.in"
{ yyval.vexpval = mkdexpr(OPMINUS, yypvt[-2].vexpval, yypvt[-0].vexpval); } break;
case 169:
# line 950 "gram.in"
{ yyval.vexpval = mkdexpr(OPSTAR, yypvt[-2].vexpval, yypvt[-0].vexpval); } break;
case 170:
# line 952 "gram.in"
{ yyval.vexpval = mkdexpr(OPSLASH, yypvt[-2].vexpval, yypvt[-0].vexpval); } break;
case 172:
# line 957 "gram.in"
{ yyval.vexpval = mkdexpr(OPPOWER, yypvt[-2].vexpval, yypvt[-0].vexpval); } break;
case 173:
# line 961 "gram.in"
{ yyval.vexpval = evicon(toklen, token); } break;
case 175:
# line 964 "gram.in"
{ yyval.vexpval = yypvt[-1].vexpval; } break;
case 176:
# line 1033 "gram.in"
{ yyval.chval = 0; } break;
case 178:
# line 1038 "gram.in"
{ yyval.chval = mkchain(yypvt[-0].expval, CHNULL); } break;
case 179:
# line 1040 "gram.in"
{ yyval.chval = hookup(yypvt[-2].chval, mkchain(yypvt[-0].expval,CHNULL) ); } break;
case 181:
# line 1046 "gram.in"
{ if (parstate != INDCL)
			yyval.expval = mkexpr(OPPAREN, yypvt[-1].expval, ENULL);
		  else yyval.expval = yypvt[-1].expval;
		} break;
case 185:
# line 1056 "gram.in"
{ yyval.expval = mkexpr(yypvt[-1].ival, yypvt[-2].expval, yypvt[-0].expval); } break;
case 186:
# line 1058 "gram.in"
{ yyval.expval = mkexpr(OPSTAR, yypvt[-2].expval, yypvt[-0].expval); } break;
case 187:
# line 1060 "gram.in"
{ yyval.expval = mkexpr(OPSLASH, yypvt[-2].expval, yypvt[-0].expval); } break;
case 188:
# line 1062 "gram.in"
{ yyval.expval = mkexpr(OPPOWER, yypvt[-2].expval, yypvt[-0].expval); } break;
case 189:
# line 1064 "gram.in"
{ if(yypvt[-1].ival == OPMINUS)
			yyval.expval = mkexpr(OPNEG, yypvt[-0].expval, ENULL);
		  else 	yyval.expval = yypvt[-0].expval;
		} break;
case 190:
# line 1069 "gram.in"
{ yyval.expval = mkexpr(yypvt[-1].ival, yypvt[-2].expval, yypvt[-0].expval); } break;
case 191:
# line 1071 "gram.in"
{ NO66(".EQV. operator");
		  yyval.expval = mkexpr(OPEQV, yypvt[-2].expval,yypvt[-0].expval); } break;
case 192:
# line 1074 "gram.in"
{ NO66(".NEQV. operator");
		  yyval.expval = mkexpr(OPNEQV, yypvt[-2].expval, yypvt[-0].expval); } break;
case 193:
# line 1077 "gram.in"
{ yyval.expval = mkexpr(OPOR, yypvt[-2].expval, yypvt[-0].expval); } break;
case 194:
# line 1079 "gram.in"
{ yyval.expval = mkexpr(OPAND, yypvt[-2].expval, yypvt[-0].expval); } break;
case 195:
# line 1081 "gram.in"
{ yyval.expval = mkexpr(OPNOT, yypvt[-0].expval, ENULL); } break;
case 196:
# line 1083 "gram.in"
{ NO66("concatenation operator //");
		  yyval.expval = mkexpr(OPCONCAT, yypvt[-2].expval, yypvt[-0].expval); } break;
case 197:
# line 1087 "gram.in"
{ yyval.ival = OPPLUS; } break;
case 198:
# line 1088 "gram.in"
{ yyval.ival = OPMINUS; } break;
case 199:
# line 1091 "gram.in"
{ yyval.ival = OPEQ; } break;
case 200:
# line 1092 "gram.in"
{ yyval.ival = OPGT; } break;
case 201:
# line 1093 "gram.in"
{ yyval.ival = OPLT; } break;
case 202:
# line 1094 "gram.in"
{ yyval.ival = OPGE; } break;
case 203:
# line 1095 "gram.in"
{ yyval.ival = OPLE; } break;
case 204:
# line 1096 "gram.in"
{ yyval.ival = OPNE; } break;
case 205:
# line 1100 "gram.in"
{ yyval.expval = mkprim(yypvt[-0].namval, PNULL, CHNULL); } break;
case 206:
# line 1102 "gram.in"
{ NO66("substring operator :");
		  if( yypvt[-1].namval->vclass != CLPARAM ) {
		  	yyval.expval = mkprim(yypvt[-1].namval, PNULL, yypvt[-0].chval);
		  } else {
			errstr("substring of parameter %s",
				varstr(VL,yypvt[-1].namval->varname) );
			YYERROR ;
		  }
		} break;
case 207:
# line 1112 "gram.in"
{ if( yypvt[-3].namval->vclass != CLPARAM ) {
		  	yyval.expval = mkprim(yypvt[-3].namval, mklist(yypvt[-1].chval), CHNULL);
		  } else {
			errstr("can not subscript parameter %s",
				varstr(VL,yypvt[-3].namval->varname) );
			YYERROR ;
		  }
		} break;
case 208:
# line 1121 "gram.in"
{ if( yypvt[-4].namval->vclass != CLPARAM ) {
		  	NO66("substring operator :");
		  	yyval.expval = mkprim(yypvt[-4].namval, mklist(yypvt[-2].chval), yypvt[-0].chval);
		  } else {
			errstr("can not subscript parameter %s",
				varstr(VL,yypvt[-4].namval->varname) );
			YYERROR ;
		  }
		} break;
case 209:
# line 1133 "gram.in"
{ yyval.chval = mkchain(yypvt[-3].expval, mkchain(yypvt[-1].expval,CHNULL)); } break;
case 210:
# line 1137 "gram.in"
{ yyval.expval = 0; } break;
case 212:
# line 1142 "gram.in"
{ yyval.expval = mklogcon(1); } break;
case 213:
# line 1143 "gram.in"
{ yyval.expval = mklogcon(0); } break;
case 214:
# line 1144 "gram.in"
{ yyval.expval = mkstrcon(toklen, token); } break;
case 215:
# line 1145 "gram.in"
 { yyval.expval = mkintcon( convci(toklen, token) ); } break;
case 216:
# line 1146 "gram.in"
 { yyval.expval = mkrealcon(TYREAL, convcd(toklen, token)); } break;
case 217:
# line 1147 "gram.in"
 { yyval.expval = mkrealcon(TYDREAL, convcd(toklen, token)); } break;
case 218:
# line 1151 "gram.in"
{ yyval.expval = mkcxcon(yypvt[-3].expval,yypvt[-1].expval); } break;
case 220:
# line 1157 "gram.in"
{ if (optimflag && parstate != INDCL)
			yyval.expval = mkexpr(OPPAREN, yypvt[-1].expval, ENULL);
		  else yyval.expval = yypvt[-1].expval;
		} break;
case 223:
# line 1166 "gram.in"
{ yyval.expval = mkexpr(yypvt[-1].ival, yypvt[-2].expval, yypvt[-0].expval); } break;
case 224:
# line 1168 "gram.in"
{ yyval.expval = mkexpr(OPSTAR, yypvt[-2].expval, yypvt[-0].expval); } break;
case 225:
# line 1170 "gram.in"
{ yyval.expval = mkexpr(OPSLASH, yypvt[-2].expval, yypvt[-0].expval); } break;
case 226:
# line 1172 "gram.in"
{ yyval.expval = mkexpr(OPPOWER, yypvt[-2].expval, yypvt[-0].expval); } break;
case 227:
# line 1174 "gram.in"
{ if(yypvt[-1].ival == OPMINUS)
			yyval.expval = mkexpr(OPNEG, yypvt[-0].expval, ENULL);
		  else	yyval.expval = yypvt[-0].expval;
		} break;
case 228:
# line 1179 "gram.in"
{ NO66("concatenation operator //");
		  yyval.expval = mkexpr(OPCONCAT, yypvt[-2].expval, yypvt[-0].expval); } break;
case 230:
# line 1234 "gram.in"
{
		if( !do_name_err ) {
		   if(yypvt[-3].labval->labdefined)
			execerr("no backward DO loops", CNULL);
		   yypvt[-3].labval->blklevel = blklevel+1;
		   exdo(yypvt[-3].labval->labelno, yypvt[-0].chval);
		   }
		} break;
case 231:
# line 1243 "gram.in"
{ exendif();  thiswasbranch = NO; } break;
case 233:
# line 1246 "gram.in"
{ exelif(yypvt[-2].expval); lastwasbranch = NO; } break;
case 234:
# line 1248 "gram.in"
{ exelse(); lastwasbranch = NO; } break;
case 235:
# line 1250 "gram.in"
{ exendif(); lastwasbranch = NO; } break;
case 236:
# line 1254 "gram.in"
{ exif(yypvt[-1].expval); } break;
case 237:
# line 1258 "gram.in"
{ if( yypvt[-2].namval->vclass != CLPARAM ) {
			yyval.chval = mkchain(yypvt[-2].namval, yypvt[-0].chval);
			do_name_err = 0;
		  } else {
			err("symbolic constant not allowed as DO variable");
		 	do_name_err = 1;
		  }
		} break;
case 238:
# line 1269 "gram.in"
{ exequals(yypvt[-2].expval, yypvt[-0].expval); } break;
case 239:
# line 1271 "gram.in"
{ if( yypvt[-0].namval->vclass != CLPARAM ) {
			exassign(yypvt[-0].namval, yypvt[-2].labval);
		  } else {
			err("can only assign to a variable");
		  }
		} break;
case 242:
# line 1280 "gram.in"
{ inioctl = NO; } break;
case 243:
# line 1282 "gram.in"
{ exarif(yypvt[-6].expval, yypvt[-4].labval, yypvt[-2].labval, yypvt[-0].labval);  thiswasbranch = YES; } break;
case 244:
# line 1284 "gram.in"
{ excall(yypvt[-0].namval, PNULL, 0, labarray); } break;
case 245:
# line 1286 "gram.in"
{ excall(yypvt[-2].namval, PNULL, 0, labarray); } break;
case 246:
# line 1288 "gram.in"
{ if(nstars < MAXLABLIST)
			excall(yypvt[-3].namval, mklist(yypvt[-1].chval), nstars, labarray);
		  else
			err("too many alternate returns");
		} break;
case 247:
# line 1294 "gram.in"
{ exreturn(yypvt[-0].expval);  thiswasbranch = YES; } break;
case 248:
# line 1296 "gram.in"
{ exstop(yypvt[-2].ival, yypvt[-0].expval);  thiswasbranch = yypvt[-2].ival; } break;
case 249:
# line 1300 "gram.in"
{ yyval.labval = mklabel( convci(toklen, token) ); } break;
case 250:
# line 1304 "gram.in"
{ if(parstate == OUTSIDE)
			{
			newproc();
			startproc(PNULL, CLMAIN);
			}
		  if( yystno != 0 && thislabel->labtype != LABFORMAT)
			if (optimflag)
				optbuff (SKLABEL, 0, thislabel->labelno, 1);
			else
				putlabel(thislabel->labelno);
		} break;
case 251:
# line 1318 "gram.in"
{ exgoto(yypvt[-0].labval);  thiswasbranch = YES; } break;
case 252:
# line 1320 "gram.in"
{ if( yypvt[-0].namval->vclass != CLPARAM ) {
			exasgoto(yypvt[-0].namval);  thiswasbranch = YES;
		  } else {
			err("must go to label or assigned variable");
		  }
		} break;
case 253:
# line 1327 "gram.in"
{ if( yypvt[-4].namval->vclass != CLPARAM ) {
			exasgoto(yypvt[-4].namval);  thiswasbranch = YES;
		  } else {
			err("must go to label or assigned variable");
		  }
		} break;
case 254:
# line 1334 "gram.in"
{ if(nstars < MAXLABLIST)
			if (optimflag)
			    optbuff (SKCMGOTO, fixtype(yypvt[-0].expval), nstars, labarray);
			else
			    putcmgo (fixtype(yypvt[-0].expval), nstars, labarray);
		  else
			err("computed GOTO list too long");
		} break;
case 257:
# line 1349 "gram.in"
{ nstars = 0; yyval.namval = yypvt[-0].namval; } break;
case 258:
# line 1353 "gram.in"
{ yyval.chval = (yypvt[-0].expval ? mkchain(yypvt[-0].expval,CHNULL) : CHNULL); } break;
case 259:
# line 1355 "gram.in"
{ if(yypvt[-0].expval)
			if(yypvt[-2].chval) yyval.chval = hookup(yypvt[-2].chval, mkchain(yypvt[-0].expval,CHNULL));
			else yyval.chval = mkchain(yypvt[-0].expval,CHNULL);
		  else
			yyval.chval = yypvt[-2].chval;
		} break;
case 261:
# line 1365 "gram.in"
{ if(nstars<MAXLABLIST) labarray[nstars++] = yypvt[-0].labval; yyval.expval = 0; } break;
case 262:
# line 1369 "gram.in"
{ yyval.ival = 0; } break;
case 263:
# line 1371 "gram.in"
{ yyval.ival = 1; } break;
case 264:
# line 1375 "gram.in"
{ yyval.chval = mkchain(yypvt[-0].expval, CHNULL); } break;
case 265:
# line 1377 "gram.in"
{ yyval.chval = hookup(yypvt[-2].chval, mkchain(yypvt[-0].expval,CHNULL) ); } break;
case 266:
# line 1381 "gram.in"
{ if(parstate == OUTSIDE)
			{
			newproc();
			startproc(PNULL, CLMAIN);
			}
		  if(parstate < INDATA) enddcl();
		  if( yystno != 0 && thislabel->labtype != LABFORMAT)
			if (optimflag)
				optbuff (SKLABEL, 0, thislabel->labelno, 1);
			else
				putlabel(thislabel->labelno);
		  yystno = 0;
		} break;
case 267:
# line 1397 "gram.in"
{ intonly = YES; } break;
case 268:
# line 1401 "gram.in"
{ intonly = NO; } break;
case 269:
# line 1406 "gram.in"
{ endio(); } break;
case 271:
# line 1411 "gram.in"
{ ioclause(IOSUNIT, yypvt[-0].expval); endioctl(); } break;
case 272:
# line 1413 "gram.in"
{ ioclause(IOSUNIT, PNULL); endioctl(); } break;
case 273:
# line 1415 "gram.in"
{ ioclause(IOSUNIT, IOSTDERR); endioctl(); } break;
case 275:
# line 1418 "gram.in"
{ doio(PNULL); } break;
case 276:
# line 1420 "gram.in"
{ doio(PNULL); } break;
case 277:
# line 1422 "gram.in"
{ doio(yypvt[-0].chval); } break;
case 278:
# line 1424 "gram.in"
{ doio(yypvt[-0].chval); } break;
case 279:
# line 1426 "gram.in"
{ doio(yypvt[-0].chval); } break;
case 280:
# line 1428 "gram.in"
{ doio(PNULL); } break;
case 281:
# line 1430 "gram.in"
{ doio(yypvt[-0].chval); } break;
case 282:
# line 1432 "gram.in"
{ doio(PNULL); } break;
case 283:
# line 1434 "gram.in"
{ doio(yypvt[-0].chval); } break;
case 285:
# line 1441 "gram.in"
{ iostmt = IOBACKSPACE; } break;
case 286:
# line 1443 "gram.in"
{ iostmt = IOREWIND; } break;
case 287:
# line 1445 "gram.in"
{ iostmt = IOENDFILE; } break;
case 289:
# line 1452 "gram.in"
{ iostmt = IOINQUIRE; } break;
case 290:
# line 1454 "gram.in"
{ iostmt = IOOPEN; } break;
case 291:
# line 1456 "gram.in"
{ iostmt = IOCLOSE; } break;
case 292:
# line 1460 "gram.in"
{
		ioclause(IOSUNIT, PNULL);
		ioclause(IOSFMT, yypvt[-0].expval);
		endioctl();
		} break;
case 293:
# line 1466 "gram.in"
{
		ioclause(IOSUNIT, PNULL);
		ioclause(IOSFMT, PNULL);
		endioctl();
		} break;
case 294:
# line 1474 "gram.in"
{ if(yypvt[-1].expval->headblock.vtype == TYCHAR)
			{
			ioclause(IOSUNIT, PNULL);
			ioclause(IOSFMT, yypvt[-1].expval);
			}
		  else
			ioclause(IOSUNIT, yypvt[-1].expval);
		  endioctl();
		} break;
case 295:
# line 1484 "gram.in"
{ endioctl(); } break;
case 298:
# line 1492 "gram.in"
{ ioclause(IOSPOSITIONAL, yypvt[-0].expval); } break;
case 299:
# line 1494 "gram.in"
{ ioclause(IOSPOSITIONAL, PNULL); } break;
case 300:
# line 1496 "gram.in"
{ ioclause(IOSPOSITIONAL, IOSTDERR); } break;
case 301:
# line 1498 "gram.in"
{ ioclause(yypvt[-1].ival, yypvt[-0].expval); } break;
case 302:
# line 1500 "gram.in"
{ ioclause(yypvt[-1].ival, PNULL); } break;
case 303:
# line 1502 "gram.in"
{ ioclause(yypvt[-1].ival, IOSTDERR); } break;
case 304:
# line 1506 "gram.in"
{ yyval.ival = iocname(); } break;
case 305:
# line 1510 "gram.in"
{ iostmt = IOREAD; } break;
case 306:
# line 1514 "gram.in"
{ iostmt = IOWRITE; } break;
case 307:
# line 1518 "gram.in"
{
		iostmt = IOWRITE;
		ioclause(IOSUNIT, PNULL);
		ioclause(IOSFMT, yypvt[-1].expval);
		endioctl();
		} break;
case 308:
# line 1525 "gram.in"
{
		iostmt = IOWRITE;
		ioclause(IOSUNIT, PNULL);
		ioclause(IOSFMT, PNULL);
		endioctl();
		} break;
case 309:
# line 1534 "gram.in"
{ yyval.chval = mkchain(yypvt[-0].tagval, CHNULL); } break;
case 310:
# line 1536 "gram.in"
{ yyval.chval = hookup(yypvt[-2].chval, mkchain(yypvt[-0].tagval, CHNULL)); } break;
case 311:
# line 1540 "gram.in"
{ yyval.tagval = (tagptr) yypvt[-0].expval; } break;
case 312:
# line 1542 "gram.in"
{ yyval.tagval = (tagptr) mkiodo(yypvt[-1].chval,yypvt[-3].chval); } break;
case 313:
# line 1546 "gram.in"
{ yyval.chval = mkchain(yypvt[-0].expval, CHNULL); } break;
case 314:
# line 1548 "gram.in"
{ yyval.chval = mkchain(yypvt[-0].tagval, CHNULL); } break;
case 316:
# line 1553 "gram.in"
{ yyval.chval = mkchain(yypvt[-2].expval, mkchain(yypvt[-0].expval, CHNULL) ); } break;
case 317:
# line 1555 "gram.in"
{ yyval.chval = mkchain(yypvt[-2].expval, mkchain(yypvt[-0].tagval, CHNULL) ); } break;
case 318:
# line 1557 "gram.in"
{ yyval.chval = mkchain(yypvt[-2].tagval, mkchain(yypvt[-0].expval, CHNULL) ); } break;
case 319:
# line 1559 "gram.in"
{ yyval.chval = mkchain(yypvt[-2].tagval, mkchain(yypvt[-0].tagval, CHNULL) ); } break;
case 320:
# line 1561 "gram.in"
{ yyval.chval = hookup(yypvt[-2].chval, mkchain(yypvt[-0].expval, CHNULL) ); } break;
case 321:
# line 1563 "gram.in"
{ yyval.chval = hookup(yypvt[-2].chval, mkchain(yypvt[-0].tagval, CHNULL) ); } break;
case 322:
# line 1567 "gram.in"
{ yyval.tagval = (tagptr) yypvt[-0].expval; } break;
case 323:
# line 1569 "gram.in"
{ yyval.tagval = (tagptr) mkiodo(yypvt[-1].chval, mkchain(yypvt[-3].expval, CHNULL) ); } break;
case 324:
# line 1571 "gram.in"
{ yyval.tagval = (tagptr) mkiodo(yypvt[-1].chval, mkchain(yypvt[-3].tagval, CHNULL) ); } break;
case 325:
# line 1573 "gram.in"
{ yyval.tagval = (tagptr) mkiodo(yypvt[-1].chval, yypvt[-3].chval); } break;
case 326:
# line 1577 "gram.in"
{ startioctl(); } break; 
		}
		goto yystack;  /* stack new state and value */

	}
