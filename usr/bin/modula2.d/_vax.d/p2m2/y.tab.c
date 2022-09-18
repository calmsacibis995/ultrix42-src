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
# define NUMBER 28
# define UNUSED 29
# define CHARCONST 30
# define STRCONST 31
# define BOOLCONST 32
# define AND 33
# define ARRAY 34
# define BEGIN 35
# define BY 36
# define CASE 37
# define CONST 38
# define LABEL 39
# define DIV 40
# define DO 41
# define ELSE 42
# define GOTO 43
# define END 44
# define PACKED 45
# define FORWARD 46
# define FOR 47
# define FROM 48
# define IF 49
# define FUNCTION 50
# define EXTERNAL 51
# define IN 52
# define DOWNTO 53
# define MOD 54
# define PROGRAM 55
# define NOT 56
# define OF 57
# define OR 58
# define POINTER 59
# define PROCEDURE 60
# define PFILE 61
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
# define INCLUDE 73
# define BAD 74
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

# line 941 "pascal.gram"


PrintEND()
{
	PrintSemi();
	PrintKeyword("END");
	PrintSemi();
}
short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 137,
	23, 166,
	-2, 168,
-1, 178,
	21, 78,
	-2, 287,
-1, 252,
	9, 164,
	42, 164,
	44, 164,
	69, 164,
	-2, 221,
-1, 432,
	44, 119,
	-2, 125,
	};
# define YYNPROD 298
# define YYLAST 566
short yyact[]={

  86, 404, 417, 271, 228, 407,  71,  69, 339, 287,
  30,  30,  30, 433, 160,  82, 273, 159, 201, 166,
 354, 150, 405, 203,  14, 133, 274,  68,  70,  43,
  14,  31,  77,  76,  38,  73, 154,  26,  72,  14,
 237,  30,  30,  30, 452, 373, 151,  16,  15, 415,
  48, 374, 345,  16,  15,  46, 153,  30,  30,  23,
  49,  46,  16,  15, 424,  23,  44, 128, 129,  24,
  46, 270,  74,  75,  23,  24, 269,  17, 423,  18,
 419,  83,  85,  17,  24,  18, 436, 350, 266,  14,
  93, 102,  17,  14,  18, 237,  90, 132,  27,  78,
 376,  79,  80, 418, 130,  32, 174, 429, 174,  30,
 158,  34,  16,  15, 179, 180,  16,  15, 131, 122,
 123, 375,  28, 175,  23, 182, 169,  84,  23,   5,
 342, 314,   6,  92,  24, 214, 109,  25,  24,  29,
 178, 176,  17, 177,  18, 195,  17,  88,  18, 205,
  33,  36, 186, 210,  89, 187,  27, 386, 181, 289,
 190, 188, 189,  87,  27,  27,  83,  85, 197, 202,
 225, 223, 196, 290, 171, 173, 124, 235, 172, 209,
  25,  33,  36,  27,  78, 211,  79,  80, 382, 212,
 457, 284, 227, 230, 234, 265,  98,  99, 229, 231,
 286, 233, 136, 378, 381, 252, 134, 283, 135, 210,
 317,  52,  84, 226, 267, 285, 403, 157, 210, 210,
 251, 289, 293, 279,  95, 281, 236, 207, 111, 324,
  27, 240, 156, 284,  55, 263, 379, 265, 260, 141,
 254, 206, 448, 360, 361, 275, 259, 428, 183, 265,
 257, 316, 253, 114, 323, 116, 118, 115, 117, 119,
  63, 322,  61, 264, 321,  13, 256, 179, 180,  27,
 355,  13, 357, 358, 255, 258, 175, 278, 295, 303,
  13,  66, 277, 202, 305, 276, 112, 310, 453, 328,
 137, 120,  59, 178, 176,  65, 177, 459, 327,  54,
  53, 338, 338, 333, 334, 335, 450, 174, 344, 341,
 315, 174, 174, 174, 330, 331, 337, 340, 336, 359,
 359, 359, 202,  50,  53, 347, 343, 349, 304, 348,
  13, 238, 368, 369,  13, 237, 371, 200, 370, 372,
 362, 363, 377, 294, 136, 136, 108, 265, 134, 134,
 135, 135, 383, 210, 174, 136, 199, 387, 137, 134,
 147, 135, 145, 142, 110, 359, 359, 359, 359, 384,
 106, 385, 104, 100,  97,  94,  91,  60,  37, 392,
 111, 174, 308,  47,   4, 388, 389, 390, 391, 307,
 101, 356, 126, 125, 184, 185, 121, 409, 400, 338,
 127, 413, 174, 410, 411, 198, 140, 412, 329, 288,
 326, 325, 420, 421, 340, 229, 414, 426, 113, 192,
 191, 139, 138,  81, 302, 399, 398, 301, 431, 430,
 439, 422, 397, 300, 396, 299, 395, 440, 441, 445,
 446, 447, 443, 298, 438, 408, 442, 406, 394, 297,
 437, 449, 454, 444, 210, 435, 456, 455, 427, 458,
 393, 296, 232, 194, 193, 332, 249, 248, 247, 246,
 245, 244, 243, 242, 241, 239, 204, 250, 292, 291,
 306, 152, 208, 262, 312, 216, 451, 434, 416, 432,
 425, 402, 351, 353, 352, 272, 313, 217, 311, 215,
 401, 346, 380, 309, 268, 170, 213, 367, 282, 365,
 280, 366, 224, 364, 222, 320, 221, 319, 220, 318,
 219, 218, 168, 167, 165, 164, 163, 162, 161, 149,
  96, 261, 148,  51,  19,  56,  21,  20,  57,  22,
 107,  35,  64, 144,  42, 105,  62, 143,  41, 103,
  40,  39,  67,  12,  11,  10,   9,   8,   7, 146,
  45, 155,  58,   3,   2,   1 };
short yypact[]={

  74,-1000,-1000,-1000,  78, 138,-1000,-1000,-1000, 138,
 138, 138,-1000, 369, -39,-1000,-1000,-1000,-1000,  15,
   9, 314, 290,-1000,-1000,-1000, 282,-1000, 368, 248,
-1000,-1000,-1000, 246,-1000, 273,-1000,-1000,-1000,  71,
 138, 138, 138,  52, 367,  98,-1000,  78,  46, 366,
-1000, 202,-1000,-1000,-1000, 365, 138, 138, 364, 138,
-1000,-1000, 363,-1000, 361,-1000,-1000, 105, 355, 372,
-1000, 265, 239, 118,-1000,-1000,  64,-1000,-1000,-1000,
-1000,-1000, 348,-1000,-1000,-1000, 227, 354,-1000,-1000,
 353,-1000,-1000, 351,-1000,-1000, -14,-1000,-1000,-1000,
-1000, 209,-1000,  71,-1000, 113,-1000, 113, 138,-1000,
-1000,-1000,-1000,  71,-1000,-1000,-1000,-1000,-1000,-1000,
-1000, 156,-1000,-1000,-1000, 156, 156, 156,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,  71, 156,
  71,-1000,-1000, 347, 328,-1000,-233,-1000, 138, 218,
-1000,-1000, 138,-1000,-1000,  24,-1000, 138,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
 101,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000, 143,
 142,-1000,-1000,-1000,  71,  71,-1000,  64, 118, 118,
-1000,  71, 138, 176,  71, 171,-1000, 153,  71,-1000,
-1000, 326,-1000, 322, 203,-1000,-1000,-1000, 138, 241,
-1000,  44,-1000, 138,-1000,  19,  14, 138, 138, 264,
 261, 256, 138,-1000, 138,-1000,-1000,-1000, 183,-1000,
-1000,-1000, 192, 199,-1000,-1000, 148,-1000,-1000,-1000,
 200,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000, 338, 280,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-233, 319, -14, 339,-1000,-1000, 375,-1000, 276,-1000,
-1000,  94, 301,-1000, 229, 187,-1000,-1000,-1000, 243,
 240, 233, 208,-1000,-1000,-1000,-1000,-1000, 137,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,  71,  71,  71,-233,
 138, 138,  71,  86,-1000,-1000, 138, 138,-1000,  -5,
 266, 113, 266,  43,-1000,-1000,-1000,-1000, 242, 242,
 242,-1000,-1000,-1000,-1000,  71,  71,-1000, 199,  71,
-1000,-1000,  71, -21,  -6,  80,  31, 337,-1000,-1000,
 195,-1000,-1000,-1000,-1000,-1000, 180,-1000,-1000,-1000,
-1000, 138, 138, 113,-1000,-1000, 129,-1000,-1000,-1000,
-1000,-1000,-1000,-1000, 242, 242, 242, 242,-1000, 199,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
 113,-1000,-1000, 194,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,  71,  71, 138,-1000,
-1000, 266,  -8,-1000,  61,-1000,  36,-1000,  71,-1000,
-1000,  11,-1000,-1000,-1000,-1000, 138,-1000,-1000,-1000,
-1000, 225,  66,-1000,-1000,-1000,-1000,  37,-1000,-1000,
  71,  71,-1000,-1000,  71,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000, 220,  71,-1000, 297,-1000,-1000, -22,
-1000, 278,-1000, 138,-1000, 167,  61, 288,-1000,-1000 };
short yypgo[]={

   0, 565, 564, 563, 383,  31, 562, 561,  29, 560,
 559,  18, 132, 558, 557, 556, 555, 554, 553, 552,
 551,  27, 550, 122, 139, 549,   6, 548, 105, 547,
 546, 545,  17, 544, 111, 543, 542, 541, 540, 539,
 538, 537, 211, 536, 535, 534, 533, 532,   0, 531,
 530, 529,  14, 528, 527, 526, 525, 524,  19, 523,
 522, 521,  26, 520, 519,  20, 518, 517, 516, 515,
 514, 513, 512, 511, 510, 509, 508, 507, 506, 505,
 504, 503, 502, 501, 500, 499, 498, 497,   3, 496,
 495,  16, 494, 493, 492, 491, 490, 489, 488,  13,
 487,   7, 486, 485, 484,  21, 483, 482, 481, 480,
  22, 479, 478,   1, 477, 476, 475, 474, 473, 472,
 471, 470, 469, 468, 467, 466,  15, 465,  25, 464,
 463, 462, 461, 460,   2, 458, 455, 453, 452, 450,
 449, 448, 447,   5, 445,   4, 444, 443, 436, 435,
 434, 433, 432, 431, 430, 429, 428, 427,   8, 426,
 425, 424,  32, 423, 422, 421, 420, 419,  38, 418,
 411,   9, 410, 409, 408, 406, 405, 400, 396,  28,
 395, 394,  33,  35, 393, 392, 391, 390, 389, 346 };
short yyr1[]={

   0,   1,   2,   2,   7,   3,  10,   8,   8,   9,
   9,   4,   4,  12,  12,  12,  12,  12,  12,  12,
  19,  13,  20,  14,  22,  15,  15,  25,  23,  27,
  29,  16,  30,  16,  31,  28,  33,  35,  17,  36,
  17,  38,  34,  40,  39,  41,  41,  44,  43,  45,
  47,  49,  45,  18,  18,  18,  18,  46,  46,  50,
  42,  32,  32,  32,  32,  32,  32,  52,  52,  52,
  61,  59,  63,  64,  60,  66,  67,  60,  68,  69,
  60,  70,  71,  60,  72,  73,  60,  74,  75,  60,
  76,  77,  60,  78,  53,  80,  82,  54,  81,  83,
  84,  83,  79,  79,  85,  86,  55,  87,  57,  88,
  90,  92,  90,  91,  93,  91,  89,  94,  96,  89,
  95,  98,  95,  97,  97, 100, 102,  99, 103, 104,
  56,  51, 106,  51, 107, 105, 109, 105, 108, 108,
 108,  11, 111,  11, 112,  11, 113, 114, 110, 115,
 115, 116, 116, 116, 116, 116, 116, 116, 116, 116,
 116, 116, 127, 117, 118, 118, 129, 128, 130, 128,
 132, 133, 119, 134, 135, 134, 137, 138, 136, 139,
 136, 140, 141, 120, 142, 142, 144, 146, 143, 147,
 148, 121, 149, 150, 122, 151, 152, 154, 123, 155,
 153, 156, 153, 157, 124, 159, 158, 160, 158, 161,
 125, 162, 162, 162, 162, 162, 162, 164, 162, 165,
 162, 126, 166, 126, 126, 167, 126,  26,  26, 145,
 170, 145, 131, 172, 131, 171, 171, 171, 174, 173,
 175, 163, 176, 163,  21,  21, 177, 177, 177, 177,
 177, 178, 178, 178, 169, 169, 169, 169, 169, 169,
 169, 179, 180, 179, 101, 181, 101, 182, 182, 168,
 184, 168, 185, 168, 183, 183, 186, 186,  58,  48,
   6,   6,  62, 188,  62, 187, 187,   5,  65,  65,
  65,  65,  65,  65,  24,  37, 189,  37 };
short yyr2[]={

   0,   1,   1,   1,   0,   8,   0,   4,   1,   0,
   1,   1,   2,   1,   1,   1,   1,   1,   1,   2,
   0,   4,   0,   4,   0,   4,   3,   0,   4,   0,
   0,   5,   0,   4,   0,   4,   0,   0,   5,   0,
   4,   0,   4,   0,   3,   2,   3,   0,   3,   2,
   0,   0,   7,   4,   4,   3,   3,   0,   1,   0,
   4,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   0,   4,   0,   0,   5,   0,   0,   5,   0,   0,
   5,   0,   0,   6,   0,   0,   6,   0,   0,   6,
   0,   0,   6,   0,   3,   0,   0,   7,   3,   1,
   0,   4,   0,   1,   0,   0,   5,   0,   5,   1,
   1,   0,   4,   0,   0,   4,   0,   0,   0,   7,
   0,   0,   3,   1,   2,   0,   0,   8,   0,   0,
   5,   1,   0,   4,   0,   3,   0,   5,   0,   1,
   1,   1,   0,   4,   0,   4,   1,   3,   2,   0,
   3,   0,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   0,   4,   1,   2,   0,   3,   0,   4,
   0,   0,   7,   0,   0,   3,   0,   0,   7,   0,
   2,   0,   0,   7,   1,   2,   0,   0,   6,   0,
   0,   6,   0,   0,   6,   0,   0,   0,  10,   0,
   3,   0,   3,   0,   3,   0,   4,   0,   4,   0,
   3,   1,   1,   1,   1,   1,   2,   0,   4,   0,
   3,   1,   0,   5,   2,   0,   4,   1,   3,   1,
   0,   4,   2,   0,   5,   0,   2,   3,   0,   3,
   0,   4,   0,   5,   0,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   0,   4,   1,   0,   4,   1,   3,   1,
   0,   3,   0,   3,   1,   3,   1,   1,   1,   1,
   0,   3,   1,   0,   4,   1,   3,   1,   1,   2,
   1,   1,   2,   1,   1,   1,   0,   4 };
short yychk[]={

-1000,  -1,  -2,  -3,  -4,  55, -12, -13, -14, -15,
 -16, -17, -18, 256,  15,  39,  38,  68,  70, -45,
 -41, -43, -39,  50,  60, -12,  -5,  27, -23, -24,
 -48,  -5, -28, -24, -34, -37, -24,   9,  73, -20,
 -22, -27, -33,  -8,  51,  -9,  46,  -4,  -8,  51,
   9, -46, -42,  10,   9, -42, -44, -40,  -6,  10,
   9,  14, -30,  14, -36,  22,   8, -19, -21,-101,
-179, -26,-168,-183,   1,   2,-182,-162,  28,  30,
  31,-163,-126,  10,  56,  11, -48, -23, -28, -34,
  44,   9,  35,  44,   9,  22, -50,   9, -24, -24,
   9,-187,  -5, -25,   9, -31,   9, -38,-189,  31,
   9,   8,  21,-169,  14,  18,  16,  19,  17,  20,
  52,-178,   1,   2,  58,-184,-185,-177,   3,   4,
  40,  54,  33,-128,  11,  13,   7,  10,-164,-165,
-175,  12,   9, -29, -35,   9, -10,   9, -47, -51,
-105,  60,-108,  70,  50,  -7,  23,   8, -26, -32,
 -52, -53, -54, -55, -56, -57, -58, -59, -60,  13,
 -79,  61,  65,  62, -48,  10,  28,  30,  27,   1,
   2,  45, -32, -24,-181,-180,-168,-182,-183,-183,
-162,-166,-167,-129,-130, -26,-162, -21,-176,   9,
   9, -11,-110, 256,-115, -48,  23,   9,-107, -62,
 -48,  -8,  -5, -78,  34, -85,-103, -87, -61, -63,
 -66, -68, -70,  28, -72,  28,-179, -26,-145, -26,
 -48,  23,-131, -26,  23,  24, -21,   9,   9,-116,
  28,-117,-118,-119,-120,-121,-122,-123,-124,-125,
-114,-126, -48,  49,  37,  71,  63,  47,  72,  43,
  35, -49,-106, -62,  22,   8,  44, -48, -80,  57,
  57, -88, -90, -91, -62, -62,  21,  21,  21, -48,
 -74, -48, -76,  24,   8,  23,   8,-171,-173,  22,
  25,-111,-112,  22,   5,-128,-132,-140,-147,-149,
-151,-157,-161, -11,   9,-105,-109,-188,   7, -81,
  11, -86,-104, -89,  37,   9,  22,  23, -64, -67,
 -69,  21,  21,  21,  21,-170,-172,-171, -48,-174,
-110,-110,-127, -26, -26, -26, -11,-126, -48,-158,
-126, -26,  44, -58, -48,  57, -83, -52, -32, -52,
  44, -94, -92, -93, -65,  28,-186,  30,  31, -48,
   1,   2, -65, -65, -71, -75, -73, -77, -26, -26,
-171, -26, -26,  66,  57,  41,  69,   5,   8,  41,
 -82,  24,   8, -48, -91, -32,  28, -48, -65, -65,
 -65, -65,-171,-133,-141,-148,-150,-152,-159,-160,
 -32, -84, -95,  22,-113,-110,-142,-143,-144,-113,
 -26, -26,-158,-113, -52,  57, -98,-134,  42,  44,
-143,-145,-153,  67,  53, -96, -48,-135,  22,  41,
-155,-156, -97, -99,-100,-136,  49,-139,-146,-154,
 -26, -26, -99,-101,-137,-113,-113,-113,  22, -26,
   9,-102,  66,  10,-138, -88,-113,  23,-134,   9 };
short yydef[]={

   0,  -2,   1,   2,   3,   0,  11,  13,  14,  15,
  16,  17,  18,   0,   0,  22,  24,  29,  36,   9,
   9,  57,   0,  47,  43,  12, 280, 287,   0,   0,
 294, 279,  32,   0,  39,   0, 295,  19,  20, 244,
   0,   0,   0,   0,   0,   0,   8,  10,   0,   0,
  49,   0,  58,  59,  45,   0,   0,   0,   0,   0,
  26,  27,   0,  34,   0,  41, 296,   0,   0, 245,
 264, 261, 227, 269, 270, 272, 274, 267, 211, 212,
 213, 214, 215, 217, 219, 240, 221,   0,  30,  37,
   0,  55,   6,   0,  56,  50, 138,  46,  48,  44,
   4,   0, 285,   0,  33, 102,  40, 102,   0,  21,
  23, 265, 262,   0, 254, 255, 256, 257, 258, 259,
 260,   0, 251, 252, 253,   0,   0,   0, 246, 247,
 248, 249, 250, 216, 222, 224, 225,  -2,   0,   0,
 244, 242,  25,   0,   0,  53, 149,  54,   0,   0,
 131, 134,   0, 139, 140,   9, 281,   0,  28,  35,
  61,  62,  63,  64,  65,  66,  67,  68,  69,  93,
   0, 104, 128, 107, 278,  70,  72,  75,  -2,  81,
  84, 103,  42, 297,   0,   0, 228, 275, 271, 273,
 268,   0,   0,   0,   0,   0, 220,   0, 244,  31,
  38,   7, 141,   0, 151,  51,  60, 132,   0,   0,
 282,   0, 286,   0,  95,   0,   0, 113,   0,   0,
   0,   0,   0,  87,   0,  90, 266, 263,   0, 229,
 226, 167,   0, 235, 218, 241,   0, 142, 144, 148,
   0, 152, 153, 154, 155, 156, 157, 158, 159, 160,
 161,   0,  -2, 170, 181, 189, 192, 195, 203, 209,
 149,   0, 138, 135, 136, 283,   0,  94,   0, 105,
 129, 116, 109, 110,   0,   0,  73,  76,  79,   0,
   0,   0,   0, 223, 230, 169, 233, 232, 235, 238,
 243, 149, 149, 150, 162, 165,   0,   0,   0, 149,
   0,   0,   0,   0,  52, 133,   0,   0,   5,   0,
   0, 102,   0,   0, 117, 111, 114,  71,   0,   0,
   0,  82,  88,  85,  91,   0,   0, 236, 235,   0,
 143, 145,   0,   0,   0,   0,   0,   0, 221, 204,
   0, 210, 147, 137, 284,  96,   0,  99, 106, 130,
 108,   0, 113, 102,  74, 288,   0, 290, 291, 293,
 276, 277,  77,  80,   0,   0,   0,   0, 231, 235,
 237, 239, 163, 171, 182, 190, 193, 196, 205, 207,
 102,  98, 100, 120, 112, 115, 289, 292,  83,  89,
  86,  92, 234, 149, 186, 149,   0,   0,   0, 149,
  97,   0,   0, 121, 173, 146, 186, 184,   0, 191,
 194,   0, 206, 208, 101, 118,   0, 172, 174, 183,
 185,   0,   0, 199, 201, 125, 122, 179, 187, 197,
   0,   0,  -2, 123,   0, 175, 176, 149, 149, 149,
 200, 202, 124,   0,   0, 180,   0, 198, 126,   0,
 188,   0, 177, 113, 149,   0, 173,   0, 178, 127 };
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
			
case 4:
# line 135 "pascal.gram"
{ SetProgFile(); } break;
case 6:
# line 141 "pascal.gram"
{ PrintKeyword("BEGIN"); } break;
case 8:
# line 145 "pascal.gram"
{ SourceError("forward does not exist"); } break;
case 19:
# line 172 "pascal.gram"
{ SourceError("declaration error"); } break;
case 20:
# line 176 "pascal.gram"
{ PrintString("(* #"); PrintKeyword("INCLUDE");} break;
case 21:
# line 178 "pascal.gram"
{ PrintStringConst(yypvt[-0]); PrintString(" *)");
			ProcessInclude(yypvt[-0]); } break;
case 22:
# line 182 "pascal.gram"
{ PrintKeyword("LABEL"); } break;
case 23:
# line 184 "pascal.gram"
{ PrintSemi(); SourceError("no gotos or labels"); } break;
case 24:
# line 188 "pascal.gram"
{ PrintKeyword("CONST"); } break;
case 25:
# line 190 "pascal.gram"
{ PrintSemi(); } break;
case 26:
# line 192 "pascal.gram"
{ PrintSemi(); } break;
case 27:
# line 196 "pascal.gram"
{ PrintString("="); } break;
case 29:
# line 201 "pascal.gram"
{ PrintKeyword("TYPE"); } break;
case 30:
# line 202 "pascal.gram"
{ PrintSemi(); } break;
case 32:
# line 204 "pascal.gram"
{ PrintSemi(); } break;
case 34:
# line 208 "pascal.gram"
{ PrintString("="); } break;
case 36:
# line 212 "pascal.gram"
{ PrintKeyword("VAR"); } break;
case 37:
# line 212 "pascal.gram"
{ PrintSemi(); } break;
case 39:
# line 214 "pascal.gram"
{ PrintSemi(); } break;
case 41:
# line 218 "pascal.gram"
{ PrintString(":"); } break;
case 43:
# line 222 "pascal.gram"
{ PrintKeyword("PROCEDURE"); } break;
case 44:
# line 223 "pascal.gram"
{ DefineFunction(yypvt[-0]); yyval = yypvt[-0]; } break;
case 45:
# line 228 "pascal.gram"
{ DefineParameters(0); PrintSemi(); yyval=yypvt[-1]; } break;
case 46:
# line 230 "pascal.gram"
{ DefineParameters(1); PrintSemi(); yyval=yypvt[-2]; } break;
case 47:
# line 234 "pascal.gram"
{ PrintKeyword("PROCEDURE"); } break;
case 48:
# line 235 "pascal.gram"
{ DefineFunction(yypvt[-0]); yyval = yypvt[-0]; } break;
case 49:
# line 239 "pascal.gram"
{ DefineParameters(0); PrintSemi(); yyval=yypvt[-1]; } break;
case 50:
# line 241 "pascal.gram"
{ PrintString(":"); } break;
case 51:
# line 242 "pascal.gram"
{ DefineParameters(1); } break;
case 52:
# line 243 "pascal.gram"
{ PrintSemi(); yyval=yypvt[-6]; } break;
case 53:
# line 247 "pascal.gram"
{ PrintKeyword("END"); PrintIdent(yypvt[-3]); EndFunction();
			PrintSemi(); } break;
case 54:
# line 251 "pascal.gram"
{ PrintKeyword("END"); PrintIdent(yypvt[-3]); EndFunction();
			PrintSemi(); } break;
case 55:
# line 255 "pascal.gram"
{ EndFunction(); PrintSemi(); } break;
case 56:
# line 258 "pascal.gram"
{ EndFunction(); PrintSemi(); } break;
case 57:
# line 263 "pascal.gram"
{ PrintString("()"); } break;
case 59:
# line 268 "pascal.gram"
{ PrintString("("); } break;
case 60:
# line 270 "pascal.gram"
{ PrintString(")"); } break;
case 70:
# line 302 "pascal.gram"
{ PrintString("("); } break;
case 71:
# line 304 "pascal.gram"
{ PrintString(")"); } break;
case 72:
# line 308 "pascal.gram"
{ PrintString("[");
			    PrintConst(yypvt[-0]); } break;
case 73:
# line 310 "pascal.gram"
{ PrintString(".."); } break;
case 74:
# line 312 "pascal.gram"
{ PrintString("]"); } break;
case 75:
# line 314 "pascal.gram"
{ PrintString("[");
			    PrintStringConst(yypvt[-0]); } break;
case 76:
# line 316 "pascal.gram"
{ PrintString(".."); } break;
case 77:
# line 318 "pascal.gram"
{ PrintString("]"); } break;
case 78:
# line 320 "pascal.gram"
{ PrintString("[");
			PrintIdent(yypvt[-0]); } break;
case 79:
# line 322 "pascal.gram"
{ PrintString(".."); } break;
case 80:
# line 324 "pascal.gram"
{ PrintString("]"); } break;
case 81:
# line 326 "pascal.gram"
{ PrintString("[+"); } break;
case 82:
# line 328 "pascal.gram"
{ PrintString(".."); } break;
case 83:
# line 330 "pascal.gram"
{ PrintString("]"); } break;
case 84:
# line 332 "pascal.gram"
{ PrintString("[-"); } break;
case 85:
# line 334 "pascal.gram"
{ PrintString(".."); } break;
case 86:
# line 336 "pascal.gram"
{ PrintString("]"); } break;
case 87:
# line 339 "pascal.gram"
{ PrintString("[+");
		PrintConst(yypvt[-0]); } break;
case 88:
# line 341 "pascal.gram"
{ PrintString(".."); } break;
case 89:
# line 343 "pascal.gram"
{ PrintString("]"); } break;
case 90:
# line 346 "pascal.gram"
{ PrintString("[-");
		PrintConst(yypvt[-0]); } break;
case 91:
# line 348 "pascal.gram"
{ PrintString(".."); } break;
case 92:
# line 350 "pascal.gram"
{ PrintString("]"); } break;
case 93:
# line 354 "pascal.gram"
{PrintKeyword("POINTER TO"); EnsureSpace(); } break;
case 95:
# line 360 "pascal.gram"
{ PrintKeyword("ARRAY"); EnsureSpace(); } break;
case 96:
# line 362 "pascal.gram"
{ PrintKeyword("OF"); } break;
case 100:
# line 376 "pascal.gram"
{ PrintString(","); } break;
case 104:
# line 386 "pascal.gram"
{ PrintKeyword("FILE"); } break;
case 105:
# line 387 "pascal.gram"
{ PrintKeyword("OF"); } break;
case 106:
# line 389 "pascal.gram"
{ SourceError("use io module for files"); } break;
case 107:
# line 395 "pascal.gram"
{ PrintKeyword("RECORD"); } break;
case 108:
# line 399 "pascal.gram"
{ PrintEND(); } break;
case 111:
# line 410 "pascal.gram"
{ PrintSemi(); } break;
case 114:
# line 418 "pascal.gram"
{ PrintString(":"); } break;
case 117:
# line 425 "pascal.gram"
{ PrintKeyword("CASE"); } break;
case 118:
# line 427 "pascal.gram"
{ PrintKeyword("OF"); AdvanceSpace(); } break;
case 119:
# line 429 "pascal.gram"
{ PrintEND(); } break;
case 121:
# line 434 "pascal.gram"
{ PrintString(":");} break;
case 125:
# line 444 "pascal.gram"
{ PrintString("| "); } break;
case 126:
# line 446 "pascal.gram"
{ PrintString(":"); } break;
case 127:
# line 450 "pascal.gram"
{ PrintSemi(); } break;
case 128:
# line 454 "pascal.gram"
{ PrintKeyword("SET"); } break;
case 129:
# line 455 "pascal.gram"
{ PrintKeyword("OF"); } break;
case 132:
# line 467 "pascal.gram"
{ PrintSemi(); } break;
case 134:
# line 472 "pascal.gram"
{ PrintKeyword("PROCEDURE"); } break;
case 135:
# line 474 "pascal.gram"
{ SourceError("use procedure type for procedure parameters"); } break;
case 136:
# line 478 "pascal.gram"
{ PrintString(":"); } break;
case 139:
# line 485 "pascal.gram"
{ PrintKeyword("VAR"); } break;
case 140:
# line 487 "pascal.gram"
{ PrintKeyword("FUNCTION");
		SourceError("use procedure type for function parameters"); } break;
case 142:
# line 499 "pascal.gram"
{PrintSemi();} break;
case 144:
# line 502 "pascal.gram"
{ SourceError("statement error"); } break;
case 147:
# line 514 "pascal.gram"
{ EatSpace(); } break;
case 150:
# line 525 "pascal.gram"
{ PrintConst(yypvt[-1]); PrintString(":");
		SourceError("no gotos or labels"); } break;
case 162:
# line 556 "pascal.gram"
{ PrintString(":="); CheckFunction(yypvt[-1]); } break;
case 166:
# line 567 "pascal.gram"
{ PrintString("("); } break;
case 167:
# line 568 "pascal.gram"
{ PrintString(")"); } break;
case 168:
# line 570 "pascal.gram"
{ PrintString("("); } break;
case 169:
# line 572 "pascal.gram"
{ PrintString(")"); } break;
case 170:
# line 577 "pascal.gram"
{ PrintKeyword("IF");} break;
case 171:
# line 578 "pascal.gram"
{ PrintKeyword("THEN"); EatSpace();} break;
case 172:
# line 580 "pascal.gram"
{ PrintEND(); } break;
case 174:
# line 585 "pascal.gram"
{ EatSpace(); } break;
case 176:
# line 589 "pascal.gram"
{ PrintKeyword("ELSIF"); } break;
case 177:
# line 590 "pascal.gram"
{ PrintKeyword("THEN"); EatSpace(); } break;
case 179:
# line 593 "pascal.gram"
{ PrintKeyword("ELSE"); } break;
case 181:
# line 598 "pascal.gram"
{ PrintKeyword("CASE"); } break;
case 182:
# line 600 "pascal.gram"
{ PrintKeyword("OF"); AdvanceSpace(); } break;
case 183:
# line 603 "pascal.gram"
{ PrintEND(); } break;
case 186:
# line 613 "pascal.gram"
{ PrintString("| "); } break;
case 187:
# line 615 "pascal.gram"
{ PrintString(":"); EatSpace(); } break;
case 189:
# line 622 "pascal.gram"
{ PrintKeyword("WHILE"); } break;
case 190:
# line 624 "pascal.gram"
{ PrintKeyword("DO"); EatSpace(); } break;
case 191:
# line 626 "pascal.gram"
{ PrintEND(); } break;
case 192:
# line 630 "pascal.gram"
{ PrintKeyword("REPEAT"); } break;
case 193:
# line 632 "pascal.gram"
{ PrintKeyword("UNTIL"); } break;
case 195:
# line 638 "pascal.gram"
{ PrintKeyword("FOR"); } break;
case 196:
# line 640 "pascal.gram"
{ PrintString(":="); } break;
case 197:
# line 643 "pascal.gram"
{ PrintKeyword("DO"); EatSpace(); } break;
case 198:
# line 645 "pascal.gram"
{ PrintEND(); } break;
case 199:
# line 649 "pascal.gram"
{ PrintKeyword("TO"); } break;
case 201:
# line 652 "pascal.gram"
{ PrintKeyword("TO"); } break;
case 202:
# line 654 "pascal.gram"
{ PrintKeyword("BY -1"); EnsureSpace(); } break;
case 203:
# line 659 "pascal.gram"
{ PrintKeyword("WITH"); } break;
case 205:
# line 664 "pascal.gram"
{ PrintString(","); } break;
case 206:
# line 665 "pascal.gram"
{ PrintEND(); } break;
case 207:
# line 667 "pascal.gram"
{ PrintKeyword("DO"); EatSpace(); } break;
case 208:
# line 668 "pascal.gram"
{ PrintEND(); } break;
case 209:
# line 672 "pascal.gram"
{ PrintKeyword("GOTO"); } break;
case 210:
# line 674 "pascal.gram"
{ SourceError("no gotos or labels"); } break;
case 211:
# line 680 "pascal.gram"
{ PrintConst(yypvt[-0]); } break;
case 212:
# line 682 "pascal.gram"
{ PrintStringConst(yypvt[-0]); } break;
case 213:
# line 684 "pascal.gram"
{ PrintStringConst(yypvt[-0]); } break;
case 217:
# line 692 "pascal.gram"
{ PrintString("("); } break;
case 218:
# line 694 "pascal.gram"
{ PrintString(")"); } break;
case 219:
# line 696 "pascal.gram"
{ PrintKeyword("NOT"); } break;
case 222:
# line 709 "pascal.gram"
{ PrintString("["); } break;
case 223:
# line 711 "pascal.gram"
{ PrintString("]"); } break;
case 224:
# line 713 "pascal.gram"
{ PrintString("^"); } break;
case 225:
# line 715 "pascal.gram"
{ PrintString("."); } break;
case 230:
# line 730 "pascal.gram"
{ PrintString(","); } break;
case 233:
# line 740 "pascal.gram"
{ PrintString(","); } break;
case 238:
# line 753 "pascal.gram"
{ PrintString(":"); } break;
case 240:
# line 757 "pascal.gram"
{ PrintString("{"); } break;
case 241:
# line 759 "pascal.gram"
{ PrintString("}"); } break;
case 242:
# line 762 "pascal.gram"
{ PrintString("{"); } break;
case 243:
# line 764 "pascal.gram"
{ PrintString("}"); } break;
case 246:
# line 777 "pascal.gram"
{ PrintString("*"); } break;
case 247:
# line 779 "pascal.gram"
{ PrintString("/"); } break;
case 248:
# line 781 "pascal.gram"
{ PrintKeyword("DIV"); } break;
case 249:
# line 783 "pascal.gram"
{ PrintKeyword("MOD"); } break;
case 250:
# line 785 "pascal.gram"
{ PrintKeyword("AND"); } break;
case 251:
# line 791 "pascal.gram"
{ PrintString("+"); } break;
case 252:
# line 793 "pascal.gram"
{ PrintString("-"); } break;
case 253:
# line 795 "pascal.gram"
{ PrintKeyword("OR"); } break;
case 254:
# line 801 "pascal.gram"
{ PrintString("="); } break;
case 255:
# line 803 "pascal.gram"
{ PrintString("<>"); } break;
case 256:
# line 805 "pascal.gram"
{ PrintString("<"); } break;
case 257:
# line 807 "pascal.gram"
{ PrintString("<="); } break;
case 258:
# line 809 "pascal.gram"
{ PrintString(">"); } break;
case 259:
# line 811 "pascal.gram"
{ PrintString(">="); } break;
case 260:
# line 813 "pascal.gram"
{ PrintKeyword("IN"); } break;
case 262:
# line 821 "pascal.gram"
{ PrintString(".."); } break;
case 265:
# line 830 "pascal.gram"
{ PrintString(","); } break;
case 270:
# line 845 "pascal.gram"
{ PrintString("+"); } break;
case 272:
# line 848 "pascal.gram"
{ PrintString("-"); } break;
case 276:
# line 861 "pascal.gram"
{ PrintString("+"); } break;
case 277:
# line 863 "pascal.gram"
{ PrintString("-"); } break;
case 279:
# line 876 "pascal.gram"
{ PrintIdent(yypvt[-0]);} break;
case 283:
# line 888 "pascal.gram"
{ PrintString(","); } break;
case 288:
# line 908 "pascal.gram"
{ PrintConst(yypvt[-0]); } break;
case 289:
# line 911 "pascal.gram"
{ PrintConst(yypvt[-0]); } break;
case 290:
# line 913 "pascal.gram"
{ PrintStringConst(yypvt[-0]); } break;
case 291:
# line 915 "pascal.gram"
{ PrintStringConst(yypvt[-0]); } break;
case 294:
# line 929 "pascal.gram"
{ CheckExport(yypvt[-0]); } break;
case 296:
# line 936 "pascal.gram"
{ PrintString(","); } break; 
		}
		goto yystack;  /* stack new state and value */

	}
