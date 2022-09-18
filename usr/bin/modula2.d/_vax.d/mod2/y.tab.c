# define ENDOFFILE 0
# define PLUS 1
# define MINUS 2
# define ASTERISK 3
# define SLASH 4
# define ASSIGN 5
# define AMPERSAND 6
# define DOT 7
# define COMMA 8
# define SEMICOLON 9
# define LPAREN 10
# define LBRACKET 11
# define LBRACE 12
# define UPARROW 13
# define EQUALS 14
# define SHARP 15
# define LESS 16
# define GREATER 17
# define NOTEQUAL 18
# define LSEQUAL 19
# define GREQUAL 20
# define DOTDOT 21
# define COLON 22
# define RPAREN 23
# define RBRACKET 24
# define RBRACE 25
# define BAR 26
# define IDENT 27
# define CARDCONST 28
# define REALCONST 29
# define CHARCONST 30
# define STRCONST 31
# define BOOLCONST 32
# define AND 33
# define ARRAY 34
# define BEGIN 35
# define BY 36
# define CASE 37
# define CONST 38
# define DEFINITION 39
# define DIV 40
# define DO 41
# define ELSE 42
# define ELSIF 43
# define END 44
# define EXIT 45
# define EXPORT 46
# define FOR 47
# define FROM 48
# define IF 49
# define IMPLEMENTATION 50
# define IMPORT 51
# define IN 52
# define LOOP 53
# define MOD 54
# define MODULE 55
# define NOT 56
# define OF 57
# define OR 58
# define POINTER 59
# define PROCEDURE 60
# define QUALIFIED 61
# define RECORD 62
# define REPEAT 63
# define RETURN 64
# define SET 65
# define THEN 66
# define TO 67
# define TYPE 68
# define UNTIL 69
# define VAR 70
# define WHILE 71
# define WITH 72
# define ATSIZE 73
# define ATALIGN 74
# define ATPASCAL 75
# define ATC 76
# define ATNONE 77
# define ATNIL 78
# define ATINCLUDE 79
# define ATNOCOUNT 80
# define ATEXTERNAL 81
# define ATUNQUALIFIED 82
# define ATDYNARRAY 83
# define ATSUBARRAY 84
# define BAD 85

# line 130 "mod2.gram"
#include <stdio.h>
/* standard type pointers, globally defined in symtab */
int anyTypeNode, procTypeNode;
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#ifndef YYSTYPE
#define YYSTYPE int
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256
short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 7,
	256, 7,
	-2, 4,
-1, 123,
	26, 100,
	42, 100,
	43, 100,
	44, 100,
	69, 100,
	-2, 278,
-1, 153,
	256, 48,
	-2, 46,
-1, 165,
	256, 19,
	-2, 16,
-1, 166,
	26, 102,
	42, 102,
	43, 102,
	44, 102,
	69, 102,
	-2, 278,
-1, 167,
	26, 101,
	42, 101,
	43, 101,
	44, 101,
	69, 101,
	-2, 277,
-1, 232,
	26, 103,
	42, 103,
	43, 103,
	44, 103,
	69, 103,
	-2, 277,
-1, 448,
	7, 269,
	-2, 175,
	};
# define YYNPROD 279
# define YYLAST 848
short yyact[]={

 184, 490, 470, 363, 471, 323, 150, 148,  74, 379,
 336, 422, 324, 409, 380, 168,  23, 423, 432,  36,
 178, 221, 251, 260, 400,  39, 121,   8,   8,  18,
 481,  23,  23,  23,  23, 122,  23, 219, 292, 281,
  69,   8,  23,  23,  23,  23,  23,  23,  23,  23,
  23,  23, 152,   8,  73,   8, 381,   8,  16,  13,
  23, 185,  68, 147,  50,   6, 261, 249,  67, 124,
   5, 134,   8, 143, 123, 138, 396,  38,  20, 142,
 461, 211, 263, 420, 223,   8,  60, 369, 438, 141,
 135, 377, 256,  61,  97,  23,  23, 140, 144, 397,
 223, 258, 145,  34, 385, 386, 387, 388, 430,   7,
 472,   8, 413,  12, 378, 163, 151, 171,   8, 463,
  65, 147, 137, 145, 383, 353, 158, 159,  60, 134,
 411, 143, 241, 138, 270, 111, 494, 142, 120, 161,
 464,   9, 145, 137,   8, 145,  66, 141, 135,  79,
  48, 353,  78, 434, 516, 140, 144, 107, 116, 240,
 164, 424, 137, 115,  91, 196, 504, 515, 193, 145,
  66, 213, 214, 101, 343, 344, 102,  47, 446, 410,
  90, 414, 112, 113, 242, 365, 366, 364, 228, 137,
 505, 145, 503, 108,  37, 145, 437, 166, 110, 406,
 343, 344,  49, 368, 191, 315, 103, 210, 104, 311,
 491, 137, 153, 222, 157, 137, 415, 259, 227, 283,
 190, 264, 189, 467, 165, 440, 492, 317, 262, 118,
  99,   8, 341, 342, 241, 243, 285, 289, 233,  48,
 244, 382,   8, 288,  72, 290, 291, 195, 361,   8,
 250, 304, 204, 207, 112, 114, 212, 205, 476, 145,
 112, 240, 225, 145, 226, 112,  47, 229, 276,  93,
 224,   4,  23, 450, 332, 280, 332, 309,  40, 137,
 287, 345, 136, 137, 208, 310, 224, 351, 451, 313,
 267, 303, 268, 319, 320, 309, 347, 352, 246, 247,
 248, 348,  92, 426, 350, 269, 170, 349, 160, 355,
 475, 318, 358, 145, 309, 112, 356, 357, 145, 458,
 250, 307, 306, 425,  22,  37, 447, 112, 359, 308,
 218,  23,  23, 137, 322,  95, 332, 273, 137, 367,
 136, 110, 222,  23, 370, 371,  19, 332, 376, 459,
 271, 460, 393,  94, 160, 351,  62,  63,  64,  15,
 284, 392, 389, 160, 395, 352, 145, 493, 416, 154,
 156, 160, 398, 394, 252, 332, 332, 354, 412, 187,
 407, 408, 401, 417, 332, 346, 137, 305, 160, 112,
 351,  19, 403, 374, 375, 414, 419, 429, 390, 160,
 352, 312, 286,  19,  70, 391,  71, 499, 511, 427,
  60, 332, 332, 275, 443,  59, 441, 332, 186, 428,
 435, 332, 445, 187, 442, 351, 449,  23,  60, 444,
 305, 455, 197, 253, 198, 352, 373, 454, 372, 145,
 431, 112, 452, 436, 282, 112, 279,  23, 473, 160,
  19, 351, 456, 278, 201, 465, 469, 277, 199, 137,
 200, 352, 474, 274, 272, 466, 479, 482, 145,  60,
 487,  60, 232, 197, 477, 198, 495, 167, 483,  94,
 265, 203,  29, 206, 333, 334, 418, 325, 137, 453,
 326, 462,  23, 496, 488,  23, 506, 509, 327, 328,
 500, 501, 507, 508, 384, 145, 329, 421, 209, 146,
 169, 513, 332, 188, 510, 192, 177, 514, 112, 497,
 484, 485, 202, 125, 448, 137, 126, 502, 127, 139,
 517, 512, 238, 239, 234, 235, 128, 241, 112, 210,
 129, 293, 294, 295, 296, 297, 298, 299, 300, 301,
 302, 130, 341, 342, 245,  45,  46,  41,  42, 489,
  48, 254, 257, 439, 240, 132, 194, 112, 112,   8,
 131, 236, 257, 266, 257, 133, 335, 109,  76, 105,
 149, 100, 217, 204, 498, 237, 112,  47, 220, 242,
 155, 216, 215, 106,  43,  14, 112, 112, 433,  75,
  77, 339, 340, 480, 337, 457, 402, 338,  44, 231,
 486, 230,  49,  98, 478, 330, 331, 238, 239, 234,
 235, 399, 241, 162, 117, 343, 344,  96,  17,  11,
  51,  52,  54,  56,  53,  55,  57, 119, 314,  10,
 316,   3,   2,   1,   0,   0,   0,   0,   0, 240,
   0,   0,   0,  21,   0,   0, 236,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  58,   0,
 237,   0,   0,   0, 242,  45,  46,  41,  42,   0,
  48, 360,   0,   0, 362,   0,   0,   0,  51,  52,
  54,  56,  53,  55,  57,  80,  81,  82,  83,  84,
  85,  86,  87,  88,  89,   0,   0,  47, 181, 182,
   0,   0,   0,   0,  43,   0,   0, 179,   0, 183,
 181, 182,   0,   0,   0,   0,  58,   0,  44, 179,
   0, 183,  49, 321,   8, 172, 173, 174, 175, 176,
   0, 404, 255,   0, 405,   0,   8, 172, 173, 174,
 175, 176,   0,  32,  33,   0,   0,   0,   0, 181,
 182,   0,  30, 180,  35,   0, 234, 235, 179, 241,
 183,   0,   0,   0,   0, 180,   0,   0,   0,   8,
  24,  25,  26,  27,  28,   8, 172, 173, 174, 175,
 176,   0,  41,  42,   0,  48, 240,   0,   0,   0,
   0,   0,   0, 236,   0,   0,   0,   0,  31,   0,
   0,   0,   0,   0, 180, 468,   0, 237,   0,   0,
   0, 242,  47,   0,   0,   0,   0,   0,   0,  43,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  44,   0,   0,   0,  49 };
short yypact[]={

  15,-1000,-1000,-1000,-1000, 215,  86,-1000,-1000, 215,
 348,-198,-1000, 382,-1000, 752, 150, 348,-1000,-1000,
 254, 674,-1000, 403,-1000,-1000,-1000,-1000,-1000,-1000,
 752, 752, 752, 752,-1000, 752, 397, 217, 382, 101,
-1000, 752, 752, 752, 752, 752, 752, 752, 752, 752,
 752,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000, 752,
 215, 279,-1000,-1000,-1000, 244, 471,-1000, 314,-1000,
-1000,-1000,-1000,-1000, 150, 191, 138,-1000, 215, 215,
 144, 144, 144, 144, 789, 789, 233, 233, 144, 554,
 230,-1000,-1000,-1000, 752, 752, 190,-1000,-1000,  83,
-1000,  26,-1000,-1000,-1000,-1000,-1000,-204, 215, 360,
  45, 441,-1000,  88,-1000,-1000,-1000,-1000,  60, 397,
 215,-1000,  84, 468,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000, 758,-1000, 413, 758, 178,
 758,  26,-1000, 215, 215, 462, 447, 758,   1,  28,
   0, 150, 150,-1000,-1000, 308,  14,-1000, 215,-1000,
 215, 215,-1000, 215,-1000,-1000, 463,-1000,  84,-1000,
 616,-1000,-1000,-1000,-1000,-1000,-1000,-1000, 369, 758,
 758, 758, 758, 758, 421,-1000, 758, 719,  35,-1000,
  26, 758, 187,  13,  26, 475,-1000, 758,-1000, 758,
-1000, 215,  77,-1000, 336, 455,-1000, 323, 454,-1000,
 391, 215, 448, 444, 437, 348,-217, 435, 215,-1000,
 337,-1000, 380, 215,-1000,-1000,-1000, 441, 101, 382,
 382,-218,-1000,-1000, 758, 758, 758, 758, 758, 758,
 758, 758, 758, 758,-1000, 268,-1000,-1000,-1000, 226,
 422,-1000, 301, 758,-1000,-1000, 306,-1000,  26, 165,
-1000, 379,  26, 758, 161, 758, 186, 287, 269,-1000,
 707, 752,-1000, 542,-1000, 542, 363,-1000,-1000,-1000,
 382, 150,-1000, 464,-1000,  30, 117, 355,-1000, 150,
-1000,-1000, 150, 126, 126, 126, 126, 763, 763, 228,
 228, 126, 531,-1000,-1000, 758, 758, 223,-1000, 758,
 143,-1000,  26, 159,-1000,-1000,  20,  26,-1000,-1000,
-1000, 758, 429, 427,-1000,-1000,-1000,-1000,-1000,-1000,
 752, 752, 464,-1000,-1000, 222,  34, 204,  67,  29,
 352, 215, 752,-1000,-1000, 382, 542,-1000, 382,-1000,
-1000, 464,  19,-1000, 117,-1000, 101, 101,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,  26, 758,-1000,-1000, 758,
 155,-1000,-1000,-1000, 542, 542, 122, 215,  55, 172,
-1000, 346, 215, 222,  16,-1000,-1000,-1000,-1000,  91,
 300, 282,-1000, 382, 101,-1000, 215,  51,-1000, 397,
 107, 107, 397, 152,  22, 189,-1000,-1000,-1000,-1000,
 542, 222, 464, 215, 204,-1000, 542, 121, 304,-1000,
 542, 265,-1000,-1000, 117,-1000, 752,-1000, 107, 464,
 215,-1000, 281,-1000,  58, 281,-1000,-1000,  26, 182,
 758,-1000, 122, 464,-1000,-1000, 752, 215,-1000,-1000,
  91, 288,-1000, 234, 150, 464,-1000,-1000,-1000,-226,
-1000,-1000, 441, 215, 215,-1000, 143,  26,-1000,-1000,
 184,-1000, 345,  79,-1000, 215,-1000, 382, 397,   1,
 215, 398,   0,-1000, 441, 441, 397,-1000, 148, 125,
 146, 752, 204, 204, 752, 464,-1000,-1000, 394,-1000,
-1000,-1000,-1000,-1000,  26,-1000,-1000, 386, 386, 184,
-1000, 542, 123, 110, 382,-1000,-1000,-1000 };
short yypgo[]={

   0, 643, 642, 641, 103, 639,  59,  15,  25,   8,
  19, 637,  40, 629, 628, 627, 624, 623, 621,  24,
  18, 614, 613, 611, 610, 609, 606, 605,   7, 603,
   6,  80,   5, 600,  56, 599, 598, 595,  62, 593,
 592, 591,  17,   0,  10,  21, 590, 588, 582, 581,
 580, 579, 578,  26, 577, 575,  20, 573, 570, 566,
 565, 374, 563, 559, 551, 540,  23,  66, 536, 529,
 528,   3,  74,  69, 526,  61, 523,  92, 117, 516,
 306,  64, 509, 508, 507,  11, 506, 504, 499, 498,
  12,  68, 110,  22,   4,   9,  14,   2,   1, 490,
 487,  13, 485, 484, 483, 482, 120,  67, 324, 653,
 481 };
short yyr1[]={

   0,   1,   1,   1,   5,  11,   2,  13,   2,  14,
  15,  17,  18,   3,  21,  16,  23,  24,  22,  25,
  26,  22,  27,  27,  27,  27,  29,  29,  29,  29,
  20,  20,  33,  33,  35,  35,   8,   8,  36,  36,
  36,  19,  19,  37,   6,   6,  40,  39,  41,  39,
  42,  42,  42,  45,  45,  45,  46,  46,  48,  48,
  47,  47,  49,  49,  49,  49,  49,  52,  52,   9,
   9,  54,  54,  31,  31,  51,  51,  57,  55,  59,
  58,  62,  60,  63,  60,  64,  65,  66,  68,  68,
  69,  69,  69,  69,  70,  71,  71,  71,  53,  53,
  53,  53,  53,  53,  72,  72,  74,  74,  76,  73,
  73,  73,  73,  73,  73,  73,  73,  73,  73,  73,
  73,  73,  75,  75,  78,  78,  78,  78,  78,  78,
  78,  78,  78,  78,  78,  78,  80,  80,  80,  80,
  80,  80,  80,  80,  80,  80,  61,  61,  77,  77,
  82,  82,  82,  82,  82,  56,  56,  83,  83,  83,
  30,  30,  84,  84,  84,  85,  85,  86,  86,  86,
  87,  87,  87,  87,  87,  88,  88,  89,  91,  91,
  92,  92,  93,  93,  67,  67,  94,  94,  96,  96,
  96,  96,  97,  97,  98,  98,  95,  95,  99,  44,
  44,  44, 100, 100, 100, 101, 101, 102,  34,  34,
 103,  90,  90,  90,  32,  32,  32,  32,  32,  32,
  32,  32, 104, 104,  50,  50, 105, 105, 106, 106,
  79,  79, 107, 107, 108, 108, 108, 108, 108, 108,
 108, 108, 108, 108, 108, 109, 109, 109, 109, 109,
 109, 109, 109, 109, 109,  81,  81,  81,  81,  81,
  81,  81,  81,  38,  38, 110, 110,  28,  28,  43,
  43,   4,  10,  10,  12,  12,  12,   7,   7 };
short yyr2[]={

   0,   1,   1,   1,   0,   0,  10,   0,   6,   0,
   0,   0,   0,  14,   0,  10,   0,   0,  11,   0,
   0,   8,   2,   2,   2,   1,   0,   3,   5,   2,
   0,   2,   3,   5,   1,   2,   0,   2,   3,   4,
   4,   0,   2,   3,   0,   1,   0,  10,   0,   6,
   1,   3,   4,   3,   4,   1,   2,   3,   0,   2,
   1,   3,   2,   2,   2,   1,   1,   0,   2,   1,
   3,   2,   3,   2,   4,   4,   4,   0,   6,   0,
   4,   0,  10,   0,  12,   4,   5,   3,   2,   4,
   4,   5,   3,   2,   5,   1,   3,   5,   0,   1,
   1,   2,   2,   3,   1,   3,   1,   2,   3,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   2,   1,   2,   3,   1,   1,   1,   1,   1,   1,
   1,   2,   3,   2,   2,   2,   1,   3,   3,   3,
   3,   3,   3,   3,   3,   3,   1,   3,   1,   3,
   4,   4,   2,   2,   3,   1,   1,   4,   5,   2,
   0,   2,   0,   1,   3,   1,   2,   1,   4,   6,
   0,   1,   1,   1,   1,   4,   4,   3,   1,   3,
   1,   3,   1,   3,   1,   3,   0,   3,   0,   3,
   6,   8,   1,   3,   0,   2,   1,   3,   3,   1,
   1,   1,   3,   3,   4,   2,   3,   5,   1,   3,
   3,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   3,   3,   4,   2,   0,   2,   3,   4,   0,   1,
   3,   4,   0,   1,   1,   1,   1,   1,   1,   1,
   1,   3,   2,   2,   2,   1,   3,   3,   3,   3,
   3,   3,   3,   3,   3,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   3,   4,   2,   0,   2,   1,
   3,   1,   2,   1,   1,   0,   1,   1,   0 };
short yychk[]={

-1000,  -1,  -2,  -3, 256,  55,  50,  -4,  27,  55,
  -5, -13,  -4,  -6, -37,  11, 256, -14,  -7,   9,
 -38,-109,-108, -43,  28,  29,  30,  31,  32,-105,
  10,  56,   1,   2,  -4,  12, -10,  44,  -6,  -8,
  24,   3,   4,  40,  54,   1,   2,  33,   6,  58,
 -81,  14,  15,  18,  16,  19,  17,  20,  52,  12,
   7, -38,-108,-108,-108,-106, -92, -91, -38, -12,
   7,   9,  27,  -7,  -9, -35, -52, -33,  51,  48,
-109,-109,-109,-109,-109,-109,-109,-109,-109,-109,
-106,  -4,  23,  25,   8,  21, -15, -10, -22,  39,
 -49,  35,  38,  68,  70, -51, -39, -31,  55, -54,
  60, -34,  -4,  -4,  25, -91, -38, -16,  39, -11,
  55, -53,   9, -72, -73, -76, -74, -70, -68, -65,
 -64, -58, -60, -55,  45,  64, 256, -56,  49, -69,
  71,  63,  53,  47,  72, -43, -82,  37, -28, -50,
 -30,  -9, 256,  -4,   9, -46,  10,  -4,  81,  -7,
   8,  51, -17,  55, -12,  -4, -72,   9,  -7, -61,
 -80, -78,  28,  29,  30,  31,  32, -79, -56,  10,
  56,   1,   2,  12, -43, -75,   5,  10, -61,  44,
  42,  26, -61, -53, -59,  -4, -56,  11,  13,  11,
  13,   7, -61,-110,  -4, 256,-104,  -4, 256, -83,
 -34,  81, 256, -10, -10, -40, -41, -48,  22,  23,
 -47, -45, -34,  70, 256,  -4,  -4, -34,  -8,  -4,
 -23, -25,   9, -73,   3,   4,  40,  54,   1,   2,
  33,   6,  58, -81, -75, -61, -78, -78, -78,-107,
 -67, -93, -61,  12, -61,  23, -77, -61,  66, -53,
 -66, -67,  41,  69, -53,   5, -57, -77, -77,  -4,
  57,  14,   9,  14,   9,  22, -34,   9,   9,   9,
  -6, 256,   9, -43,  23,  -7,  22, -34,  -7,  -9,
  -7,  -7, 256, -80, -80, -80, -80, -80, -80, -80,
 -80, -80, -80,  23,  25,   8,  21,-107,  23,   8,
 -53,  44,  22, -53, -61,  44, -61,  41,  24,  24,
 -66,  26, -38, -32, -90,-100, -99, -89, -88, -86,
  73,  74, -43,-103,-102,  34, -44,  62,  65,  59,
  60,  10,  11,  83,  84, -32,  22,  -7, -10, -45,
 -42, -43, -44,  34,  22, -10,  -8,  -8, -10, -93,
 -61,  25, -61, -71,  44,  42,  43, -53,  44,  67,
 -53, -66,   9,   9, -38, -38, -90,  57,  80, -95,
 -96, -34,  37,  57, -87,  75,  76,  77,  78,  10,
 -34, -38,  -7, -32,  -8,  -7,  57,  80, -42, -18,
 -19, -19, -26, -53, -61, -61,  44, -32, -32,-101,
  57,   8, -43,  57,   9,  44,  22, -43,  -4, -90,
  67, -84, -85, -42,  70,  23,  21,  -7, -19, -43,
  57, -12, -20, -36,  46, -20, -12,  44,  66, -62,
  36, -32, -90, -43, -96, -32,  57,  22,  -4, -32,
   8,  23, -42, -38,  -9, -43, -10, -27,  38,  68,
  70, -31, -34,  61,  82, -10, -53,  41, -61,-101,
 -97, -94, -92, -43, -85,  22,  24, -10, -21, -28,
 -29, 256, -30,  -7, -34, -34, -24, -71, -53, -63,
 -98,  26,  42,  22,  57, -43,  -7, -12,  -4,   9,
  -7,  -7, -12,  44,  41,  44, -94, -95, -95, -97,
  -7,  14, -53, -98, -32,  44,  44,  -7 };
short yydef[]={

   0,  -2,   1,   2,   3,   0,   0,  -2, 271,   0,
  44,   0,   9, 278,  45,   0,   0,  44,  36, 277,
   0, 263, 245, 234, 235, 236, 237, 238, 239, 240,
   0,   0,   0,   0, 269, 228, 275, 273, 278,  67,
  43,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0, 255, 256, 257, 258, 259, 260, 261, 262, 228,
   0,   0, 242, 243, 244,   0, 229, 180, 178,   8,
 274, 276, 272,  10,   0,  37,  69,  34,   0,   0,
 246, 247, 248, 249, 250, 251, 252, 253, 254, 264,
   0, 270, 241, 226,   0,   0,   0,   5,  35,   0,
  68,  98, 267, 224, 160,  65,  66,  67,   0,   0,
   0, 278, 208,   0, 227, 181, 179,  11,   0, 275,
   0,  70,  99,  -2, 104, 109, 110, 111, 112, 113,
 114, 115, 116, 117, 118, 119, 121, 106,   0,   0,
   0,  98,  79,   0,   0, 155, 156,   0,  62,  63,
  64,   0,   0,  -2,  73,  58,   0,  71,   0,  32,
   0,   0,  36,   0,   6,  -2,  -2,  -2,   0, 120,
 146, 136, 124, 125, 126, 127, 128, 129, 130,   0,
   0,   0,   0, 232, 155, 107,   0,   0,   0,  88,
  98,  93,   0,   0,  98,   0,  77,   0, 152,   0,
 153,   0,   0, 268,   0,   0, 225,   0,   0, 161,
   0,   0,   0,   0,   0,  44,   0,   0,   0,  56,
 278,  60,   0,   0,  55,  72, 209, 278,  67, 278,
 278,   0,  -2, 105,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0, 131,   0, 133, 134, 135,   0,
 233, 184, 182, 232, 108, 122,   0, 148,  98,   0,
  92,   0,  98,   0,   0,   0,   0,   0,   0, 154,
   0,   0, 266,   0, 223,   0,   0, 159,  75,  76,
 278,   0,  74,  59,  57,   0,   0,   0,  33,   0,
  36,  36,   0, 137, 138, 139, 140, 141, 142, 143,
 144, 145, 147, 132, 230,   0,   0,   0, 123,   0,
   0,  89,  98,   0,  85,  80,   0,  98, 150, 151,
  90,   0,   0,   0, 214, 215, 216, 217, 218, 219,
   0,   0, 211, 212, 213, 199,   0, 188,   0, 170,
 167,   0,   0, 200, 201, 278,   0,  36, 278,  61,
  53,  50,   0, 199,   0,  12,  41,  41,  20, 185,
 183, 231, 149,  94,  95,  98,   0,  87,  86,   0,
   0,  91, 265, 222,   0,   0,   0,   0,   0,   0,
 196,   0,   0,   0,   0, 171, 172, 173, 174, 162,
   0,   0, 157, 278,  41,  49,   0,   0,  54, 275,
  30,  30, 275,   0,   0,  81,  78, 220, 221, 202,
   0,   0, 203,   0, 188, 198,   0,   0, 269, 177,
   0,   0, 163, 165,   0, 210,   0, 158,  67,  51,
   0,  13,   0,  42,   0,   0,  21,  96,  98,   0,
   0, 205,   0, 204, 197, 189, 186,   0,  -2, 176,
   0, 168, 166,   0,   0,  52,  14,  31, 267,  26,
 160,  25, 278,   0,   0,  17,   0,  98,  83, 206,
 194, 192,   0,   0, 164,   0, 207, 278, 275,  22,
  23,   0,  24,  38, 278, 278, 275,  97,   0,   0,
   0, 186, 188, 188, 186, 169,  47,  15, 278,  29,
  39,  40,  18,  82,  98, 190, 193, 195, 187, 194,
  27,   0,   0,   0, 278,  84, 191,  28 };
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
# line 154 "mod2.gram"
{ printf("Fatal error, cannot proceed\n");
				    exit(1); } break;
case 4:
# line 162 "mod2.gram"
{ yyval = DefineModule(yypvt[-0],MODULE); } break;
case 5:
# line 163 "mod2.gram"
{ ScanEofOK(); } break;
case 6:
# line 164 "mod2.gram"
{ EndFile(); EndModule(yypvt[-7],yypvt[-3],yypvt[-2]); } break;
case 7:
# line 165 "mod2.gram"
{ yyval = DefineModule(yypvt[-0],MODULE); } break;
case 8:
# line 167 "mod2.gram"
{ EndFile(); EndModule(yypvt[-3],0,yypvt[-1]); } break;
case 9:
# line 172 "mod2.gram"
{ yyval = DefineModule(yypvt[-0],IMPLEMENTATION); } break;
case 10:
# line 173 "mod2.gram"
{ GetDefinitionModule(yypvt[-2]);} break;
case 11:
# line 174 "mod2.gram"
{ ProcessExports(yypvt[-4]); } break;
case 12:
# line 175 "mod2.gram"
{ ScanEofOK(); } break;
case 13:
# line 176 "mod2.gram"
{ EndFile(); EndModule(yypvt[-10],yypvt[-3],yypvt[-2]); } break;
case 14:
# line 182 "mod2.gram"
{ ScanEofOK(); } break;
case 15:
# line 182 "mod2.gram"
{ EndFile(); } break;
case 16:
# line 187 "mod2.gram"
{ yyval = DefineModule(yypvt[-0],DEFINITION); } break;
case 17:
# line 189 "mod2.gram"
{ ScanEofOK(); } break;
case 18:
# line 190 "mod2.gram"
{ EndFile(); EndModule(yypvt[-7],0,yypvt[-2]); } break;
case 19:
# line 191 "mod2.gram"
{ yyval = DefineModule(yypvt[-0],DEFINITION); } break;
case 20:
# line 192 "mod2.gram"
{ ScanEofOK(); } break;
case 21:
# line 193 "mod2.gram"
{ EndFile(); EndModule(yypvt[-4],0,yypvt[-2]); } break;
case 25:
# line 204 "mod2.gram"
{ EndProc(yypvt[-0],0,0); } break;
case 27:
# line 211 "mod2.gram"
{ DefineType(yypvt[-1],0); } break;
case 28:
# line 213 "mod2.gram"
{ DefineType(yypvt[-3],yypvt[-1]); } break;
case 32:
# line 227 "mod2.gram"
{ yyval = Import(0,yypvt[-1]); } break;
case 33:
# line 229 "mod2.gram"
{ yyval = Import(yypvt[-3],yypvt[-1]); } break;
case 34:
# line 234 "mod2.gram"
{ yyval = ReadImport(yypvt[-0]); } break;
case 35:
# line 236 "mod2.gram"
{ yyval = ReadImport(yypvt[-1]); } break;
case 37:
# line 243 "mod2.gram"
{ ProcessImport(yypvt[-0],0); } break;
case 38:
# line 248 "mod2.gram"
{ Export(yypvt[-1],EXPORT); } break;
case 39:
# line 250 "mod2.gram"
{ Export(yypvt[-1],QUALIFIED); } break;
case 40:
# line 252 "mod2.gram"
{ Export(yypvt[-1],ATUNQUALIFIED); } break;
case 43:
# line 264 "mod2.gram"
{ yyval = yypvt[-1]; } break;
case 46:
# line 275 "mod2.gram"
{ yyval = DefineModule(yypvt[-0],MODULE); } break;
case 47:
# line 277 "mod2.gram"
{ EndModule(yypvt[-7],yypvt[-2],yypvt[-1]); } break;
case 48:
# line 278 "mod2.gram"
{ yyval = DefineModule(yypvt[-0],MODULE); } break;
case 49:
# line 280 "mod2.gram"
{ EndModule(yypvt[-3],0,yypvt[-1]); } break;
case 50:
# line 285 "mod2.gram"
{ yyval = TypeOf(yypvt[-0]); } break;
case 51:
# line 287 "mod2.gram"
{ yyval = ArrayType(0,TypeOf(yypvt[-0]),yypvt[-2],0); } break;
case 52:
# line 289 "mod2.gram"
{ yyval = ArrayType(0,TypeOf(yypvt[-0]),yypvt[-3],ATNOCOUNT); } break;
case 53:
# line 294 "mod2.gram"
{ yyval = MakeParamList(0,yypvt[-2],yypvt[-0]); } break;
case 54:
# line 296 "mod2.gram"
{ yyval = MakeParamList(VAR,yypvt[-2],yypvt[-0]); } break;
case 55:
# line 299 "mod2.gram"
{ yyval = MakeParamList(0,0,anyTypeNode); } break;
case 56:
# line 304 "mod2.gram"
{ yyval = 0; } break;
case 57:
# line 306 "mod2.gram"
{ yyval = yypvt[-1]; } break;
case 58:
# line 311 "mod2.gram"
{ yyval = 0; } break;
case 59:
# line 313 "mod2.gram"
{ yyval = TypeOf(yypvt[-0]); } break;
case 61:
# line 321 "mod2.gram"
{ yyval = AppendParamList(yypvt[-2],yypvt[-0]); } break;
case 69:
# line 346 "mod2.gram"
{ yyval = AddToStmtList(0,0); } break;
case 70:
# line 348 "mod2.gram"
{ yyval = yypvt[-0]; } break;
case 71:
# line 354 "mod2.gram"
{ yyval = DefineProc(yypvt[-0],PROCEDURE); } break;
case 72:
# line 356 "mod2.gram"
{ yyval = DefineProc(yypvt[-0],ATEXTERNAL); } break;
case 73:
# line 361 "mod2.gram"
{ yyval = AddTypeToProc(yypvt[-1],procTypeNode); } break;
case 74:
# line 363 "mod2.gram"
{ yyval = AddTypeToProc(yypvt[-3],ProcType(yypvt[-2],yypvt[-1])); } break;
case 75:
# line 368 "mod2.gram"
{ EndProc(yypvt[-3],yypvt[-2],yypvt[-1]); } break;
case 76:
# line 370 "mod2.gram"
{ EndProc(yypvt[-3],0,yypvt[-1]); } break;
case 77:
# line 373 "mod2.gram"
{ yyval = StartWith(yypvt[-0]); } break;
case 78:
# line 374 "mod2.gram"
{ yyval = WithStmtNode(yypvt[-3],yypvt[-1]); } break;
case 79:
# line 378 "mod2.gram"
{ yyval = StartLoop(); } break;
case 80:
# line 379 "mod2.gram"
{ yyval = LoopStmtNode(yypvt[-2],yypvt[-1]); } break;
case 81:
# line 384 "mod2.gram"
{ yyval = StartFor(yypvt[-4],yypvt[-2],yypvt[-0],0); } break;
case 82:
# line 386 "mod2.gram"
{ yyval = ForStmtNode(yypvt[-3],yypvt[-1]); } break;
case 83:
# line 388 "mod2.gram"
{ yyval = StartFor(yypvt[-6],yypvt[-4],yypvt[-2],yypvt[-0]); } break;
case 84:
# line 389 "mod2.gram"
{ yyval = ForStmtNode(yypvt[-3],yypvt[-1]); } break;
case 85:
# line 394 "mod2.gram"
{ yyval = RepeatStmtNode(yypvt[-2],yypvt[-0]); } break;
case 86:
# line 399 "mod2.gram"
{ yyval = WhileStmtNode(yypvt[-3],yypvt[-1]); } break;
case 87:
# line 404 "mod2.gram"
{ yyval = MakeCase(yypvt[-2],yypvt[-0]); } break;
case 89:
# line 412 "mod2.gram"
{ yyval = AddCaseElse(yypvt[-3],yypvt[-1]); } break;
case 90:
# line 417 "mod2.gram"
{ yyval = AddCase(CaseStmtNode(yypvt[-2]),yypvt[-0]); } break;
case 91:
# line 420 "mod2.gram"
{ yyval = AddCase(CaseStmtNode(yypvt[-3]),yypvt[-0]); } break;
case 92:
# line 423 "mod2.gram"
{ yyval = AddCase(yypvt[-2],yypvt[-0]); } break;
case 93:
# line 426 "mod2.gram"
{ yyval = yypvt[-1]; } break;
case 94:
# line 431 "mod2.gram"
{ yyval = IfStmtNode(yypvt[-3],yypvt[-1],yypvt[-0]); } break;
case 95:
# line 436 "mod2.gram"
{ yyval = AddToStmtList(0,0); } break;
case 96:
# line 438 "mod2.gram"
{ yyval = yypvt[-1]; } break;
case 97:
# line 440 "mod2.gram"
{ yyval = AddToStmtList(0,IfStmtNode(yypvt[-3],yypvt[-1],yypvt[-0])); } break;
case 98:
# line 445 "mod2.gram"
{ yyval = AddToStmtList(0,0); } break;
case 99:
# line 447 "mod2.gram"
{ yyval = AddToStmtList(0,0); } break;
case 102:
# line 453 "mod2.gram"
{ yyval = yypvt[-0]; } break;
case 103:
# line 455 "mod2.gram"
{ yyval = yypvt[-1]; } break;
case 104:
# line 460 "mod2.gram"
{ yyval = AddToStmtList(0,yypvt[-0]); } break;
case 105:
# line 462 "mod2.gram"
{ yyval = AddToStmtList(yypvt[-2],yypvt[-0]); } break;
case 106:
# line 467 "mod2.gram"
{ yyval = ProcStmtNode(yypvt[-0],0); } break;
case 107:
# line 469 "mod2.gram"
{ yyval = ProcStmtNode(yypvt[-1],yypvt[-0]); } break;
case 108:
# line 474 "mod2.gram"
{ yyval = AssignStmtNode(yypvt[-2],yypvt[-0]); } break;
case 118:
# line 497 "mod2.gram"
{ yyval = ExitStmtNode(); } break;
case 119:
# line 502 "mod2.gram"
{ yyval = ReturnStmtNode(0); } break;
case 120:
# line 504 "mod2.gram"
{ yyval = ReturnStmtNode(yypvt[-0]); } break;
case 121:
# line 506 "mod2.gram"
{ yyval = 0; } break;
case 122:
# line 511 "mod2.gram"
{ yyval = AddToExprList(0,0); } break;
case 123:
# line 513 "mod2.gram"
{ yyval = yypvt[-1]; } break;
case 124:
# line 518 "mod2.gram"
{ yyval = ConstExprNode(yypvt[-0]); } break;
case 125:
# line 520 "mod2.gram"
{ yyval = ConstExprNode(yypvt[-0]); } break;
case 126:
# line 522 "mod2.gram"
{ yyval = ConstExprNode(yypvt[-0]); } break;
case 127:
# line 524 "mod2.gram"
{ yyval = ConstExprNode(yypvt[-0]); } break;
case 128:
# line 526 "mod2.gram"
{ yyval = ConstExprNode(yypvt[-0]); } break;
case 131:
# line 532 "mod2.gram"
{ yyval = FuncExprNode(yypvt[-1],yypvt[-0]); } break;
case 132:
# line 534 "mod2.gram"
{ yyval = yypvt[-1]; } break;
case 133:
# line 536 "mod2.gram"
{ yyval = UnOpExprNode(NOT,yypvt[-0]); } break;
case 134:
# line 538 "mod2.gram"
{ yyval = UnOpExprNode(PLUS,yypvt[-0]); } break;
case 135:
# line 540 "mod2.gram"
{ yyval = UnOpExprNode(MINUS,yypvt[-0]); } break;
case 137:
# line 547 "mod2.gram"
{ yyval = BinOpExprNode(ASTERISK,yypvt[-2],yypvt[-0]); } break;
case 138:
# line 549 "mod2.gram"
{ yyval = BinOpExprNode(SLASH,yypvt[-2],yypvt[-0]); } break;
case 139:
# line 551 "mod2.gram"
{ yyval = BinOpExprNode(DIV,yypvt[-2],yypvt[-0]); } break;
case 140:
# line 553 "mod2.gram"
{ yyval = BinOpExprNode(MOD,yypvt[-2],yypvt[-0]); } break;
case 141:
# line 555 "mod2.gram"
{ yyval = BinOpExprNode(PLUS,yypvt[-2],yypvt[-0]); } break;
case 142:
# line 557 "mod2.gram"
{ yyval = BinOpExprNode(MINUS,yypvt[-2],yypvt[-0]); } break;
case 143:
# line 559 "mod2.gram"
{ yyval = BinOpExprNode(AND,yypvt[-2],yypvt[-0]); } break;
case 144:
# line 561 "mod2.gram"
{ yyval = BinOpExprNode(AMPERSAND,yypvt[-2],yypvt[-0]); } break;
case 145:
# line 563 "mod2.gram"
{ yyval = BinOpExprNode(OR,yypvt[-2],yypvt[-0]); } break;
case 147:
# line 571 "mod2.gram"
{ yyval = BinOpExprNode(yypvt[-1],yypvt[-2],yypvt[-0]); } break;
case 148:
# line 576 "mod2.gram"
{ yyval = AddToExprList(0,yypvt[-0]); } break;
case 149:
# line 578 "mod2.gram"
{ yyval = AddToExprList(yypvt[-2],yypvt[-0]); } break;
case 150:
# line 583 "mod2.gram"
{ yyval = SubscriptExprNode(SymExprNode(yypvt[-3]),yypvt[-1]); } break;
case 151:
# line 585 "mod2.gram"
{ yyval = SubscriptExprNode(yypvt[-3],yypvt[-1]); } break;
case 152:
# line 587 "mod2.gram"
{ yyval = DerefExprNode(SymExprNode(yypvt[-1])); } break;
case 153:
# line 589 "mod2.gram"
{ yyval = DerefExprNode(yypvt[-1]); } break;
case 154:
# line 591 "mod2.gram"
{ yyval = DotExprNode(yypvt[-2],yypvt[-0]); } break;
case 155:
# line 596 "mod2.gram"
{ yyval = SymExprNode(yypvt[-0]); } break;
case 157:
# line 605 "mod2.gram"
{ DefineVarList(yypvt[-3],yypvt[-1],VAR); } break;
case 158:
# line 607 "mod2.gram"
{ DefineVarList(yypvt[-3],yypvt[-1],ATEXTERNAL); } break;
case 162:
# line 622 "mod2.gram"
{ yyval = 0; } break;
case 164:
# line 626 "mod2.gram"
{ yyval = AppendParamList(yypvt[-2],yypvt[-0]); } break;
case 165:
# line 631 "mod2.gram"
{ yyval = MakeParamList(0,0,yypvt[-0]); } break;
case 166:
# line 633 "mod2.gram"
{ yyval = MakeParamList(VAR,0,yypvt[-0]); } break;
case 167:
# line 638 "mod2.gram"
{ yyval = procTypeNode; } break;
case 168:
# line 640 "mod2.gram"
{ yyval = ProcType(yypvt[-1],0); } break;
case 169:
# line 642 "mod2.gram"
{ yyval = ProcType(yypvt[-3],TypeOf(yypvt[-0])); } break;
case 170:
# line 646 "mod2.gram"
{ yyval = POINTER; } break;
case 171:
# line 648 "mod2.gram"
{ yyval = ATPASCAL; } break;
case 172:
# line 650 "mod2.gram"
{ yyval = ATC; } break;
case 173:
# line 652 "mod2.gram"
{ yyval = ATNONE; } break;
case 174:
# line 654 "mod2.gram"
{ yyval = ATNIL; } break;
case 175:
# line 664 "mod2.gram"
{ yyval = PointerForwardType(yypvt[-0],yypvt[-2]); } break;
case 176:
# line 666 "mod2.gram"
{ yyval = PointerType(yypvt[-0],yypvt[-2]); } break;
case 177:
# line 671 "mod2.gram"
{ yyval = SetType(yypvt[-0]); } break;
case 178:
# line 676 "mod2.gram"
{ yyval = MakeConstSet(yypvt[-0],0); } break;
case 179:
# line 678 "mod2.gram"
{ yyval = MakeConstSet(yypvt[-2],yypvt[-0]); } break;
case 180:
# line 683 "mod2.gram"
{ yyval = AddToConstSetList(0,yypvt[-0]); } break;
case 181:
# line 685 "mod2.gram"
{ yyval = AddToConstSetList(yypvt[-2],yypvt[-0]); } break;
case 182:
# line 690 "mod2.gram"
{ yyval = MakeExprSet(yypvt[-0],0); } break;
case 183:
# line 692 "mod2.gram"
{ yyval = MakeExprSet(yypvt[-2],yypvt[-0]); } break;
case 184:
# line 697 "mod2.gram"
{ yyval = AddToExprSetList(0,yypvt[-0]); } break;
case 185:
# line 699 "mod2.gram"
{ yyval = AddToExprSetList(yypvt[-2],yypvt[-0]); } break;
case 186:
# line 704 "mod2.gram"
{ yyval = 0; } break;
case 187:
# line 706 "mod2.gram"
{ yyval = MakeVariant(yypvt[-2],yypvt[-0]); } break;
case 188:
# line 711 "mod2.gram"
{ yyval = AddToFieldList(0,0); } break;
case 189:
# line 713 "mod2.gram"
{ yyval = MakeFieldList(yypvt[-2],yypvt[-0]); } break;
case 190:
# line 715 "mod2.gram"
{ yyval = MakeVariantField(0,TypeOf(yypvt[-4]),yypvt[-2],yypvt[-1]); } break;
case 191:
# line 717 "mod2.gram"
{ yyval = MakeVariantField(yypvt[-6],TypeOf(yypvt[-4]),yypvt[-2],yypvt[-1]); } break;
case 192:
# line 722 "mod2.gram"
{ yyval = AddToVariantList(0,yypvt[-0]); } break;
case 193:
# line 724 "mod2.gram"
{ yyval = AddToVariantList(yypvt[-2],yypvt[-0]); } break;
case 194:
# line 729 "mod2.gram"
{ yyval = 0; } break;
case 195:
# line 731 "mod2.gram"
{ yyval = MakeVariant(0,yypvt[-0]); } break;
case 197:
# line 738 "mod2.gram"
{ yyval = AppendFieldList(yypvt[-2],yypvt[-0]); } break;
case 198:
# line 743 "mod2.gram"
{ yyval = RecordType(yypvt[-1]); } break;
case 199:
# line 748 "mod2.gram"
{ yyval = ARRAY; } break;
case 200:
# line 750 "mod2.gram"
{ yyval = ATDYNARRAY; } break;
case 201:
# line 752 "mod2.gram"
{ yyval = ATSUBARRAY; } break;
case 202:
# line 757 "mod2.gram"
{ yyval = ArrayType(yypvt[-1],yypvt[-0],ARRAY,0); } break;
case 203:
# line 759 "mod2.gram"
{ yyval = ArrayType(0,TypeOf(yypvt[-0]),yypvt[-2],0); } break;
case 204:
# line 761 "mod2.gram"
{ yyval = ArrayType(0,TypeOf(yypvt[-0]),yypvt[-3],ATNOCOUNT); } break;
case 205:
# line 766 "mod2.gram"
{ yyval = yypvt[-0]; } break;
case 206:
# line 769 "mod2.gram"
{ yyval = ArrayType(yypvt[-1],yypvt[-0],ARRAY,0); } break;
case 207:
# line 774 "mod2.gram"
{ yyval = SubrangeType(yypvt[-3],yypvt[-1]); } break;
case 208:
# line 779 "mod2.gram"
{ yyval = AddToIdentList(0,MakeIdent(yypvt[-0])); } break;
case 209:
# line 781 "mod2.gram"
{ yyval = AddToIdentList(yypvt[-2],MakeIdent(yypvt[-0])); } break;
case 210:
# line 786 "mod2.gram"
{ yyval =  EnumerationType(yypvt[-1]); } break;
case 211:
# line 791 "mod2.gram"
{ yyval = TypeOf(yypvt[-0]); } break;
case 220:
# line 812 "mod2.gram"
{ yyval = TypeWithSize(yypvt[-0],yypvt[-1]); } break;
case 221:
# line 814 "mod2.gram"
{ yyval = TypeWithAlign(yypvt[-0],yypvt[-1]); } break;
case 222:
# line 819 "mod2.gram"
{ DefineType(yypvt[-3],yypvt[-1]); } break;
case 226:
# line 834 "mod2.gram"
{ yyval = SetConst(yypvt[-1],0); } break;
case 227:
# line 836 "mod2.gram"
{ yyval = SetConst(yypvt[-1],TypeOf(yypvt[-3])); } break;
case 228:
# line 841 "mod2.gram"
{ yyval = 0; } break;
case 230:
# line 848 "mod2.gram"
{ yyval = SetExprNode(yypvt[-1],0); } break;
case 231:
# line 850 "mod2.gram"
{ yyval = SetExprNode(yypvt[-1],yypvt[-3]); } break;
case 232:
# line 855 "mod2.gram"
{ yyval = 0; } break;
case 234:
# line 862 "mod2.gram"
{ yyval = SymConst(yypvt[-0]); } break;
case 241:
# line 876 "mod2.gram"
{ yyval = yypvt[-1]; } break;
case 242:
# line 878 "mod2.gram"
{ yyval = UnOpConst(NOT,yypvt[-0]); } break;
case 243:
# line 880 "mod2.gram"
{ yyval = UnOpConst(PLUS,yypvt[-0]); } break;
case 244:
# line 882 "mod2.gram"
{ yyval = UnOpConst(MINUS,yypvt[-0]); } break;
case 246:
# line 890 "mod2.gram"
{ yyval = BinOpConst(ASTERISK,yypvt[-2],yypvt[-0],0); } break;
case 247:
# line 892 "mod2.gram"
{ yyval = BinOpConst(SLASH,yypvt[-2],yypvt[-0],0); } break;
case 248:
# line 894 "mod2.gram"
{ yyval = BinOpConst(DIV,yypvt[-2],yypvt[-0],0); } break;
case 249:
# line 896 "mod2.gram"
{ yyval = BinOpConst(MOD,yypvt[-2],yypvt[-0],0); } break;
case 250:
# line 898 "mod2.gram"
{ yyval = BinOpConst(PLUS,yypvt[-2],yypvt[-0],0); } break;
case 251:
# line 900 "mod2.gram"
{ yyval = BinOpConst(MINUS,yypvt[-2],yypvt[-0],0); } break;
case 252:
# line 902 "mod2.gram"
{ yyval = BinOpConst(AND,yypvt[-2],yypvt[-0],0); } break;
case 253:
# line 904 "mod2.gram"
{ yyval = BinOpConst(AMPERSAND,yypvt[-2],yypvt[-0],0); } break;
case 254:
# line 906 "mod2.gram"
{ yyval = BinOpConst(OR,yypvt[-2],yypvt[-0],0); } break;
case 255:
# line 912 "mod2.gram"
{ yyval = EQUALS; } break;
case 256:
# line 914 "mod2.gram"
{ yyval = SHARP; } break;
case 257:
# line 916 "mod2.gram"
{ yyval = NOTEQUAL; } break;
case 258:
# line 918 "mod2.gram"
{ yyval = LESS; } break;
case 259:
# line 920 "mod2.gram"
{ yyval = LSEQUAL; } break;
case 260:
# line 922 "mod2.gram"
{ yyval = GREATER; } break;
case 261:
# line 924 "mod2.gram"
{ yyval = GREQUAL; } break;
case 262:
# line 926 "mod2.gram"
{ yyval = IN; } break;
case 264:
# line 934 "mod2.gram"
{ yyval = BinOpConst(yypvt[-1],yypvt[-2],yypvt[-0],0); } break;
case 265:
# line 939 "mod2.gram"
{ DefineConst(yypvt[-3],yypvt[-1]); } break;
case 269:
# line 954 "mod2.gram"
{ yyval = AddToIdentList(0,MakeIdent(yypvt[-0])); } break;
case 270:
# line 956 "mod2.gram"
{ yyval = AddToIdentList(yypvt[-2],MakeIdent(yypvt[-0])); } break;
case 272:
# line 968 "mod2.gram"
{ yyval = yypvt[-0]; } break;
case 273:
# line 971 "mod2.gram"
{ ErrorMissingIdent(); yyval = 0; } break;
case 275:
# line 978 "mod2.gram"
{ ErrorModuleDot(); } break;
case 276:
# line 981 "mod2.gram"
{ ErrorModuleDot(); } break;
case 278:
# line 988 "mod2.gram"
{ ErrorMissingSemicolon(); } break; 
		}
		goto yystack;  /* stack new state and value */

	}
