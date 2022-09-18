/*   static char sccsid[]="@(#)etoa.h	4.1		(ULTRIX)   	7/2/90";   */

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*	Conversion table for EBCDIC to 8-bit ASCII	*/

	000 ,001 ,002 ,003 ,0234,011 ,0206,0177,0227,0215,0216,013 ,014 ,015 ,016,017 ,
	020 ,021 ,022 ,023 ,0235,0205,010 ,0207,030 ,031 ,0222,0217,034 ,035 ,036 ,037 ,
	0200,0201,0202,0203,0204,012 ,027 ,027 ,0210,0211,0212,0213,0214,005 ,'^' ,007 ,
	0220,0221,026 ,0223,0224,0225,0226,004,0230,0231,0232,0233,024 ,025 ,0236,032 ,
	' ' ,0240,0241,0242,0243,0244,0245,0246,0247,0250,0325,'.' ,'<' ,'(' ,'+' ,'|' ,
	'&' ,0251,0252,0253,0254,0255,0256,0257,0260,0261,'!' ,'$' ,'*' ,')' ,';' ,'~' ,
	'-' ,'/' ,0262,0263,0264,0265,0266,0267,0270,0271,0313,',' ,'%' ,'_' ,'>' ,'?' ,
	0272,0273,0274,0275,0276,0277,0300,0301,0302,'`' ,':' ,'#' ,'@' ,'\'' ,'=' ,'"' ,
	0303,'a' ,'b' ,'c' ,'d' ,'e' ,'f' ,'g' ,'h' ,'i' ,0304,0305,0306,0307,0310,0311,
	0312,'j' ,'k' ,'l' ,'m' ,'n' ,'o' ,'p' ,'q' ,'r' ,'^' ,0314,0315,0316,0317,0320,
	0321,0345,'s' ,'t' ,'u' ,'v' ,'w' ,'x' ,'y' ,'z' ,0322,0323,0324,'[' ,0326,0327,
	0330,0331,0332,0333,0334,0335,0336,0337,0340,0341,0342,0343,0344,']' ,0346,0347,
	'{' ,'A' ,'B' ,'C' ,'D' ,'E' ,'F' ,'G' ,'H' ,'I' ,0350,0351,0352,0353,0354,0355,
	'}' ,'J' ,'K' ,'L' ,'M' ,'N' ,'O' ,'P' ,'Q' ,'R' ,0356,0357,0360,0361,0362,0363,
	'\\' ,0237,'S' ,'T' ,'U' ,'V' ,'W' ,'X' ,'Y' ,'Z' ,0364,0365,0366,0367,0370,0371,
	'0' ,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,'7' ,'8' ,'9' ,0372,0373,0374,0375,0376,0377
