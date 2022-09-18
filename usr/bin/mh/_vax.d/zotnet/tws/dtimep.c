# include "stdio.h"
static int start_cond = 0;
#define BEGIN start_cond =
struct yysvf { 
	struct yywork *yystoff;
	struct yysvf *yyother;
	int *yystops;};
# define Z 2
#ifndef	lint
static char ident[] = "@(#)$Id: dtimep.lex,v 1.2 90/11/25 19:07:42 sharpe Exp $";
#endif
#include "tws.h"
#include "../h/strings.h"
#include <ctype.h>
#include <sys/types.h>
#ifndef SYS5
#include <sys/timeb.h>
#endif not SYS5

#ifdef SYS5
extern int  daylight;
extern long timezone;
extern char *tzname[];
#endif SYS5

/*
 * Patchable flag that says how to interpret NN/NN/NN dates. When
 * true, we do it European style: DD/MM/YY. When false, we do it
 * American style: MM/DD/YY.  Of course, these are all non-RFC822
 * compliant.
 */
int europeandate = 0;

/*
 * Table to convert month names to numeric month.  We use the
 * fact that the low order 5 bits of the sum of the 2nd & 3rd
 * characters of the name is a hash with no collisions for the 12
 * valid month names.  (The mask to 5 bits maps any combination of
 * upper and lower case into the same hash value).
 */
static	int month_map[] = {
	0,
	6,	/* 1 - Jul */
	3,	/* 2 - Apr */
	5,	/* 3 - Jun */
	0,
	10,	/* 5 - Nov */
	0,
	1,	/* 7 - Feb */
	11,	/* 8 - Dec */
	0,
	0,
	0,
	0,
	0,
	0,
	0,	/*15 - Jan */
	0,
	0,
	0,
	2,	/*19 - Mar */
	0,
	8,	/*21 - Sep */
	0,
	9,	/*23 - Oct */
	0,
	0,
	4,	/*26 - May */
	0,
	7	/*28 - Aug */
};
/*
 * Same trick for day-of-week using the hash function
 *  (c1 & 7) + (c2 & 4)
 */
static	int day_map[] = {
	0,
	0,
	0,
	6,	/* 3 - Sat */
	4,	/* 4 - Thu */
	0,
	5,	/* 6 - Fri */
	0,	/* 7 - Sun */
	2,	/* 8 - Tue */
	1	/* 9 - Mon */,
	0,
	3	/*11 - Wed */
};
#define SETDAY	{ tw.tw_wday= day_map[(cp[0] & 7) + (cp[1] & 4)];\
		tw.tw_flags &= ~TW_SDAY; tw.tw_flags |= TW_SEXP;\
		cp += 2; }
#define SETMONTH { tw.tw_mon = month_map[(cp[0] + cp[1]) & 0x1f]; gotdate++;\
		 cp += 2;\
		 SKIPD;}
#define CVT2	(i=(*cp++ - '0'),isdigit(*cp)? i*10 + (*cp++ - '0') : i)
#define SKIPD	{ while ( !isdigit(*cp++) ) ;  --cp; }
#define EXPZONE	{ tw.tw_flags &= ~TW_SZONE; tw.tw_flags |= TW_SZEXP; }
#define ZONE(x)	{ tw.tw_zone=(x); EXPZONE; }
#define ZONED(x) { ZONE(x); tw.tw_flags |= TW_DST; }
#define	LC(c)	(isupper (c) ? tolower (c) : (c))

#ifdef	DSTXXX
#ifndef	BSD42
#include <time.h>
#else	BSD42
#include <sys/time.h>
#endif	BSD42

static	zonehack (tw)
register struct tws *tw;
{
    register struct tm *tm;

    if (twclock (tw) == -1L)
	return;

    tm = localtime (&tw -> tw_clock);
    if (tm -> tm_isdst) {
	tw -> tw_flags |= TW_DST;
	tw -> tw_zone -= 60;
    }
}
#endif	DSTXXX
struct tws *dparsetime (str)
	char *str;
{
	register int i;
	static struct tws tw;
	register char *cp;
	register int gotdate = 0;
#ifndef SYS5
	struct timeb	tb;
#endif not SYS5

	start_cond = 0;

	/* Zero out the struct. */
	bzero( (char *) &tw, sizeof tw);

	/* Set default time zone. */
#ifndef SYS5
	ftime( &tb );
	tw.tw_zone = -tb.timezone;
#else SYS5
	tzset( );
	tw.tw_zone = -(timezone / 60);
#endif SYS5

	while (isspace(*str))
		str++;
	while ( 1 )
		switch (cp = str, *cp ? lex_string( &str, start_cond) : 0) {

		case -1:
			if (!gotdate || tw.tw_year == 0)
				return 0;
			/* fall through */
		case 0:
			return &tw;

case 1:
			SETDAY;
break;
case 2:
		{
					cp++;
					SETDAY;
					}
break;
case 3:
	{
					if (europeandate) {
						/* European: DD/MM/YY */
						tw.tw_mday = CVT2;
						cp++;
						tw.tw_mon  = CVT2 - 1;
					} else {
						/* American: MM/DD/YY */
						tw.tw_mon  = CVT2 - 1;
						cp++;
						tw.tw_mday = CVT2;
					}
					cp++;
					for (i = 0; isdigit(*cp); )
						i = i*10 + (*cp++ - '0');
					tw.tw_year = i % 100;
					}
break;
case 4:
{
					tw.tw_mday = CVT2;
					while ( !isalpha(*cp++) )
						;
					SETMONTH;
					for (i = 0; isdigit(*cp); )
						i = i*10 + (*cp++ - '0');
					tw.tw_year = i % 100;
					}
break;
case 5:
	{
					cp++;
					SETMONTH;
					tw.tw_mday = CVT2;
					SKIPD;
					for (i = 0; isdigit(*cp); )
						i = i*10 + (*cp++ - '0');
					tw.tw_year = i % 100;
					}
break;
case 6:
		{
					cp++;
					SETMONTH;
					tw.tw_mday = CVT2;
					}
break;
case 7:
	{	/* hack: ctime w/o TZ */
					tw.tw_hour = CVT2; cp++;
					tw.tw_min  = CVT2; cp++;
					tw.tw_sec  = CVT2;
					while( !isdigit(*cp++) )
						;
					cp++;
					tw.tw_year = CVT2;
					}
break;
case 8:
			{
					tw.tw_hour = CVT2; cp++;
					tw.tw_min  = CVT2; cp++;
					tw.tw_sec  = CVT2;
					BEGIN Z;
					}
break;
case 9:
			{
					tw.tw_hour = CVT2; cp++;
					tw.tw_min = CVT2;
					BEGIN Z;
					}
break;
case 10:
			{
					tw.tw_hour = CVT2; cp++;
					if (tw.tw_hour == 12)
						tw.tw_hour = 0;
					tw.tw_min  = CVT2;
					BEGIN Z;
					}
break;
case 11:
			{
					tw.tw_hour = CVT2; cp++;
					if (tw.tw_hour != 12)
						tw.tw_hour += 12;
					tw.tw_min  = CVT2;
					BEGIN Z;
					}
break;
case 12:
		{
					tw.tw_hour = CVT2;
					tw.tw_min  = CVT2;
					tw.tw_sec  = CVT2;
					BEGIN Z;
					}
break;
case 13:
			{
					/*
					 * Luckly, 4 digit times in the range
					 * 1960-1999 aren't legal as hour
					 * and minutes.
					 */
					cp += 2;
					tw.tw_year = CVT2;
					}
break;
case 14:
		{
					if (tw.tw_hour) {
					    cp += 2;
					    tw.tw_year = CVT2;
					    tw.tw_zone = 0;
					} else {
					    tw.tw_hour = CVT2;
					    tw.tw_min  = CVT2;
					    BEGIN Z;
					}
					}
break;
case 15:
			ZONE(0 * 60);
break;
case 16:
			ZONE(0 * 60);
break;
case 17:
			ZONE(2 * 60);
break;
case 18:
			ZONED(2 * 60);
break;
case 19:
			ZONE(-5 * 60);
break;
case 20:
			ZONED(-5 * 60);
break;
case 21:
			ZONE(-6 * 60);
break;
case 22:
			ZONED(-6 * 60);
break;
case 23:
			ZONE(-7 * 60);
break;
case 24:
			ZONED(-7 * 60);
break;
case 25:
			ZONE(-8 * 60);
break;
case 26:
			ZONED(-8 * 60);
break;
case 27:
			ZONE(-(3 * 60 + 30));
break;
case 28:
			ZONE(-4 * 60);
break;
case 29:
			ZONED(-4 * 60);
break;
case 30:
			ZONE(-9 * 60);
break;
case 31:
			ZONED(-9 * 60);
break;
case 32:
			ZONE(-10 * 60);
break;
case 33:
			ZONED(-10 * 60);
break;
case 34:
			ZONED(0 * 60);
break;
case 35:
			{
					tw.tw_zone = 60 * (('a'-1) - LC(*cp));
					EXPZONE; 
					}
break;
case 36:
			{
					tw.tw_zone = 60 * ('a' - LC(*cp));
					EXPZONE; 
					}
break;
case 37:
			{
					tw.tw_zone = 60 * (LC(*cp) - 'm');
					EXPZONE; 
					}
break;
case 38:
		{
					cp++;
					tw.tw_zone = ((cp[0] * 10 + cp[1])
						     -('0' * 10   + '0'))*60
						    +((cp[2] * 10 + cp[3])
						     -('0' * 10   + '0'));
					EXPZONE;
#ifdef	DSTXXX
					zonehack (&tw);
#endif	DSTXXX
					cp += 4;
					}
break;
case 39:
		{
					cp++;
					tw.tw_zone = (('0' * 10   + '0')
						     -(cp[0] * 10 + cp[1]))*60
						    +(('0' * 10   + '0')
						     -(cp[2] * 10 + cp[3]));
					EXPZONE;
#ifdef	DSTXXX
					zonehack (&tw);
#endif	DSTXXX
					cp += 4;
					}
break;
case 40:
		{
					while( !isdigit(*cp++) )
						;
					cp++;
					tw.tw_year = CVT2;
					}
break;
case 41:
case 42:
;
break;
default: return(0);
} }
/* end of yylex */
int yyvstop[] ={
0,

42,
0,

41,
0,

42,
0,

35,
0,

35,
0,

35,
0,

35,
0,

35,
0,

35,
0,

35,
0,

35,
0,

35,
0,

36,
0,

36,
0,

37,
0,

37,
0,

37,
0,

37,
0,

37,
0,

37,
0,

37,
0,

37,
0,

37,
0,

15,
0,

9,
0,

1,
0,

1,
0,

1,
0,

1,
0,

1,
0,

1,
0,

1,
0,

29,
0,

28,
0,

34,
0,

22,
0,

21,
0,

20,
0,

19,
0,

16,
0,

33,
0,

32,
0,

18,
0,

17,
0,

24,
0,

23,
0,

27,
0,

26,
0,

25,
0,

31,
0,

30,
0,

14,
0,

9,
0,

9,
0,

13,
14,
0,

1,
0,

2,
0,

14,
0,

8,
0,

10,
0,

11,
0,

4,
0,

4,
0,

13,
14,
0,

6,
0,

40,
0,

38,
0,

39,
0,

2,
0,

3,
0,

3,
0,

12,
0,

8,
0,

8,
0,

4,
0,

4,
0,

4,
0,

6,
0,

6,
0,

1,
0,

4,
0,

5,
0,

5,
0,

5,
0,

5,
0,

7,
0,
0};
# define YYTYPE int
struct yywork { YYTYPE verify, advance; } yycrank[] ={
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	1,5,	1,6,	
5,5,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	21,21,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	1,5,	0,0,	5,5,	
3,21,	3,6,	0,0,	0,0,	
0,0,	1,7,	0,0,	0,0,	
21,21,	0,0,	0,0,	0,0,	
0,0,	1,8,	1,9,	1,8,	
1,10,	1,10,	1,10,	1,10,	
1,10,	1,10,	1,10,	3,21,	
9,63,	21,82,	22,83,	22,83,	
0,0,	0,0,	0,0,	3,7,	
0,0,	0,0,	3,22,	0,0,	
3,23,	0,0,	0,0,	3,8,	
3,9,	3,8,	3,10,	3,10,	
3,10,	3,10,	3,10,	3,10,	
3,10,	10,64,	10,64,	10,64,	
10,64,	10,64,	10,64,	10,64,	
10,64,	10,64,	10,64,	0,0,	
0,0,	0,0,	1,11,	15,72,	
0,0,	1,12,	14,70,	1,13,	
12,67,	13,68,	17,75,	1,14,	
19,79,	20,81,	1,15,	1,16,	
1,17,	15,73,	11,65,	16,74,	
1,18,	1,19,	13,69,	11,66,	
1,20,	19,80,	14,71,	25,99,	
3,24,	3,25,	3,26,	3,27,	
3,28,	3,29,	3,30,	3,31,	
3,32,	3,33,	3,34,	3,34,	
3,35,	3,36,	3,37,	3,38,	
3,39,	3,39,	3,40,	3,41,	
3,42,	3,39,	3,43,	3,39,	
3,44,	7,45,	8,50,	18,76,	
23,84,	23,84,	30,104,	18,77,	
7,46,	24,97,	42,114,	45,117,	
26,100,	28,102,	7,47,	7,48,	
31,105,	33,70,	7,49,	38,112,	
33,107,	24,65,	46,118,	18,78,	
24,98,	8,50,	24,66,	26,101,	
28,103,	36,74,	49,123,	31,106,	
56,129,	36,111,	38,113,	33,108,	
47,119,	33,71,	8,51,	57,130,	
8,52,	8,53,	8,53,	8,53,	
8,53,	8,53,	8,53,	8,53,	
8,53,	8,53,	8,53,	8,54,	
44,115,	23,85,	23,86,	23,87,	
47,120,	23,88,	35,72,	23,89,	
23,90,	35,109,	23,91,	59,133,	
50,50,	23,92,	23,93,	44,116,	
23,94,	48,121,	55,127,	58,131,	
35,73,	23,95,	60,134,	55,128,	
35,110,	23,96,	61,135,	62,136,	
65,138,	64,52,	48,122,	51,51,	
66,139,	67,140,	68,141,	50,50,	
69,142,	70,143,	8,55,	58,132,	
64,54,	8,56,	71,144,	8,57,	
71,145,	73,148,	72,146,	8,58,	
50,51,	74,149,	8,59,	8,60,	
8,61,	72,147,	51,51,	75,150,	
8,62,	52,124,	52,124,	52,124,	
52,124,	52,124,	52,124,	52,124,	
52,124,	52,124,	52,124,	53,125,	
53,125,	53,125,	53,125,	53,125,	
53,125,	53,125,	53,125,	53,125,	
53,125,	54,126,	54,126,	54,126,	
54,126,	54,126,	54,126,	54,126,	
54,126,	54,126,	54,126,	63,125,	
63,125,	63,125,	63,125,	63,125,	
63,125,	63,137,	63,137,	63,137,	
63,137,	76,151,	77,152,	78,153,	
50,55,	79,154,	80,155,	50,56,	
81,156,	50,57,	82,157,	93,111,	
85,97,	50,58,	91,107,	92,109,	
50,59,	50,60,	50,61,	97,160,	
98,161,	99,162,	50,62,	51,55,	
100,163,	101,164,	51,56,	85,98,	
51,57,	91,108,	92,110,	102,165,	
51,58,	103,166,	104,167,	51,59,	
51,60,	51,61,	105,168,	106,169,	
107,170,	51,62,	83,158,	83,158,	
83,158,	83,158,	83,158,	83,158,	
83,158,	83,158,	83,158,	83,158,	
84,159,	84,159,	84,159,	84,159,	
84,159,	84,159,	84,159,	84,159,	
84,159,	84,159,	108,171,	109,172,	
110,173,	111,174,	112,175,	113,176,	
115,177,	116,178,	117,179,	118,180,	
119,181,	120,182,	121,183,	122,184,	
123,185,	124,186,	124,187,	124,187,	
124,187,	124,187,	124,187,	124,187,	
124,187,	124,187,	124,187,	124,187,	
125,188,	125,188,	125,188,	125,188,	
125,188,	125,188,	125,188,	125,188,	
125,188,	125,188,	126,189,	127,194,	
128,195,	129,196,	130,197,	131,198,	
132,199,	133,201,	132,200,	134,203,	
135,204,	136,205,	138,207,	139,207,	
133,202,	140,207,	143,214,	145,147,	
148,216,	149,217,	150,218,	152,220,	
153,221,	126,189,	137,206,	137,206,	
137,206,	137,206,	137,206,	137,206,	
137,206,	137,206,	137,206,	137,206,	
141,207,	138,207,	139,207,	142,212,	
140,207,	126,190,	126,190,	126,190,	
126,190,	126,190,	126,190,	126,190,	
126,190,	126,190,	126,190,	126,191,	
144,207,	146,207,	147,207,	180,230,	
154,212,	151,212,	156,212,	141,207,	
155,212,	182,232,	142,212,	157,225,	
157,225,	157,225,	157,225,	179,228,	
181,228,	183,228,	187,186,	184,228,	
190,191,	192,241,	142,212,	144,207,	
146,207,	147,207,	185,228,	154,212,	
151,212,	156,212,	193,242,	155,212,	
198,250,	200,202,	203,252,	204,253,	
205,254,	208,147,	126,192,	154,212,	
151,212,	156,212,	209,257,	155,212,	
210,258,	211,259,	206,255,	212,212,	
213,260,	214,261,	215,147,	216,262,	
217,263,	126,193,	218,264,	219,265,	
220,266,	140,210,	138,208,	158,226,	
158,226,	158,226,	158,226,	158,226,	
158,226,	158,226,	158,226,	158,226,	
158,226,	206,255,	212,212,	139,209,	
221,267,	222,268,	179,229,	223,269,	
224,270,	228,274,	142,213,	159,227,	
159,227,	159,227,	159,227,	159,227,	
159,227,	159,227,	159,227,	159,227,	
159,227,	141,211,	183,233,	146,215,	
181,231,	184,234,	188,238,	185,235,	
186,236,	186,237,	186,236,	186,236,	
186,236,	186,236,	186,236,	186,236,	
186,236,	186,236,	189,189,	156,224,	
194,202,	154,222,	229,275,	230,276,	
144,147,	151,219,	155,223,	231,277,	
232,278,	188,238,	191,240,	191,240,	
191,240,	191,240,	191,240,	191,240,	
191,240,	191,240,	191,240,	191,240,	
233,279,	189,189,	234,280,	194,202,	
235,281,	188,239,	188,239,	188,239,	
188,239,	188,239,	188,239,	188,239,	
188,239,	188,239,	188,239,	195,202,	
194,243,	237,283,	238,238,	194,244,	
194,245,	194,244,	194,244,	194,244,	
194,244,	194,244,	194,244,	194,244,	
194,244,	241,241,	242,242,	245,290,	
246,202,	247,291,	248,292,	249,293,	
250,294,	251,202,	195,202,	252,295,	
253,296,	238,238,	254,297,	257,147,	
196,202,	255,255,	258,301,	259,302,	
260,303,	261,304,	263,305,	195,243,	
241,241,	242,242,	195,244,	195,245,	
195,244,	195,244,	195,244,	195,244,	
195,244,	195,244,	195,244,	195,244,	
264,306,	265,307,	189,192,	196,202,	
255,255,	266,308,	197,202,	268,309,	
269,310,	270,311,	275,312,	277,313,	
194,246,	279,314,	280,315,	281,316,	
196,243,	189,193,	282,282,	196,244,	
196,245,	196,244,	196,244,	196,244,	
196,244,	196,244,	196,244,	196,244,	
196,244,	197,202,	289,318,	291,202,	
284,284,	286,285,	199,202,	288,288,	
292,319,	293,320,	294,321,	295,322,	
296,323,	282,282,	197,243,	297,324,	
283,282,	197,244,	197,245,	197,244,	
197,244,	197,244,	197,244,	197,244,	
197,244,	197,244,	197,244,	284,284,	
286,285,	199,202,	288,288,	195,247,	
201,202,	300,299,	298,298,	301,326,	
299,325,	302,327,	303,212,	283,282,	
304,147,	305,328,	199,243,	306,147,	
196,248,	199,244,	199,245,	199,244,	
199,244,	199,244,	199,244,	199,244,	
199,244,	199,244,	199,244,	201,202,	
283,236,	298,298,	202,202,	299,325,	
307,329,	308,330,	309,331,	311,332,	
312,228,	313,333,	314,334,	285,285,	
201,243,	207,207,	316,335,	201,244,	
201,245,	201,244,	201,244,	201,244,	
201,244,	201,244,	201,244,	201,244,	
201,244,	202,202,	317,336,	197,249,	
319,338,	320,339,	321,202,	322,340,	
323,202,	324,341,	285,285,	330,344,	
207,207,	332,345,	202,243,	335,346,	
318,337,	202,244,	202,245,	202,244,	
202,244,	202,244,	202,244,	202,244,	
202,244,	202,244,	202,244,	285,317,	
207,256,	207,256,	207,256,	207,256,	
207,256,	207,256,	207,256,	207,256,	
207,256,	207,256,	199,202,	318,337,	
341,348,	343,350,	201,251,	225,271,	
225,271,	225,271,	225,271,	225,271,	
225,271,	225,271,	225,271,	225,271,	
225,271,	226,272,	226,272,	226,272,	
226,272,	226,272,	226,272,	226,272,	
226,272,	226,272,	226,272,	227,273,	
227,273,	227,273,	227,273,	227,273,	
227,273,	227,273,	227,273,	227,273,	
227,273,	236,282,	236,282,	236,282,	
236,282,	236,282,	236,282,	236,282,	
236,282,	236,282,	236,282,	239,284,	
239,284,	239,284,	239,284,	239,284,	
239,284,	239,284,	239,284,	239,284,	
239,284,	240,285,	349,349,	287,287,	
336,347,	336,347,	336,347,	336,347,	
0,0,	0,0,	0,0,	243,243,	
347,351,	347,351,	347,351,	347,351,	
347,351,	347,351,	347,351,	347,351,	
347,351,	347,351,	0,0,	0,0,	
240,285,	349,349,	287,287,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	243,243,	0,0,	
0,0,	244,287,	287,288,	0,0,	
240,286,	240,286,	240,286,	240,286,	
240,286,	240,286,	240,286,	240,286,	
240,286,	240,286,	243,244,	243,245,	
243,244,	243,244,	243,244,	243,244,	
243,244,	243,244,	243,244,	243,244,	
244,287,	0,0,	0,0,	256,298,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
244,288,	0,0,	325,325,	0,0,	
244,287,	244,287,	244,287,	244,287,	
244,287,	244,287,	244,287,	244,287,	
244,287,	244,287,	256,298,	0,0,	
0,0,	290,287,	0,0,	287,289,	
0,0,	0,0,	0,0,	0,0,	
0,0,	325,325,	256,299,	0,0,	
342,349,	0,0,	256,300,	256,300,	
256,300,	256,300,	256,300,	256,300,	
256,300,	256,300,	256,300,	256,300,	
290,287,	325,342,	325,343,	325,342,	
325,342,	325,342,	325,342,	325,342,	
325,342,	325,342,	325,342,	342,349,	
290,288,	244,289,	350,349,	0,0,	
0,0,	290,244,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	342,349,	
342,349,	342,349,	342,349,	342,349,	
342,349,	342,349,	342,349,	342,349,	
342,349,	350,349,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	350,342,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	290,289,	0,0,	0,0,	
0,0};
struct yysvf yysvec[] ={
0,	0,	0,
yycrank+1,	0,		0,	
yycrank+0,	yysvec+1,	0,	
yycrank+27,	0,		0,	
yycrank+0,	yysvec+3,	0,	
yycrank+3,	0,		yyvstop+1,
yycrank+0,	0,		yyvstop+3,
yycrank+47,	0,		0,	
yycrank+141,	0,		0,	
yycrank+3,	yysvec+8,	0,	
yycrank+37,	yysvec+8,	0,	
yycrank+2,	0,		0,	
yycrank+3,	0,		0,	
yycrank+4,	0,		0,	
yycrank+5,	0,		0,	
yycrank+2,	0,		0,	
yycrank+4,	0,		0,	
yycrank+7,	0,		0,	
yycrank+54,	0,		0,	
yycrank+4,	0,		0,	
yycrank+8,	0,		0,	
yycrank+12,	0,		yyvstop+5,
yycrank+14,	0,		0,	
yycrank+104,	0,		0,	
yycrank+57,	0,		yyvstop+7,
yycrank+8,	0,		yyvstop+9,
yycrank+60,	0,		yyvstop+11,
yycrank+0,	yysvec+12,	yyvstop+13,
yycrank+61,	0,		yyvstop+15,
yycrank+0,	yysvec+13,	yyvstop+17,
yycrank+45,	0,		yyvstop+19,
yycrank+64,	0,		yyvstop+21,
yycrank+0,	0,		yyvstop+23,
yycrank+68,	0,		0,	
yycrank+0,	0,		yyvstop+25,
yycrank+109,	0,		yyvstop+27,
yycrank+66,	0,		yyvstop+29,
yycrank+0,	yysvec+17,	yyvstop+31,
yycrank+67,	0,		yyvstop+33,
yycrank+0,	0,		yyvstop+35,
yycrank+0,	yysvec+18,	yyvstop+37,
yycrank+0,	yysvec+19,	yyvstop+39,
yycrank+42,	0,		yyvstop+41,
yycrank+0,	yysvec+20,	yyvstop+43,
yycrank+100,	0,		yyvstop+45,
yycrank+45,	0,		0,	
yycrank+59,	0,		0,	
yycrank+87,	0,		0,	
yycrank+113,	0,		0,	
yycrank+77,	0,		0,	
yycrank+203,	0,		0,	
yycrank+222,	0,		0,	
yycrank+209,	0,		0,	
yycrank+219,	yysvec+8,	0,	
yycrank+229,	0,		0,	
yycrank+106,	0,		0,	
yycrank+79,	0,		0,	
yycrank+86,	0,		0,	
yycrank+122,	0,		0,	
yycrank+114,	0,		0,	
yycrank+111,	0,		0,	
yycrank+127,	0,		0,	
yycrank+126,	0,		0,	
yycrank+239,	yysvec+8,	0,	
yycrank+182,	yysvec+50,	0,	
yycrank+114,	0,		0,	
yycrank+129,	0,		0,	
yycrank+134,	0,		0,	
yycrank+136,	0,		0,	
yycrank+131,	0,		0,	
yycrank+127,	0,		0,	
yycrank+134,	0,		0,	
yycrank+132,	0,		0,	
yycrank+135,	0,		0,	
yycrank+131,	0,		0,	
yycrank+139,	0,		0,	
yycrank+181,	0,		0,	
yycrank+186,	0,		0,	
yycrank+189,	0,		0,	
yycrank+184,	0,		0,	
yycrank+201,	0,		0,	
yycrank+204,	0,		0,	
yycrank+249,	0,		0,	
yycrank+290,	0,		0,	
yycrank+300,	0,		0,	
yycrank+208,	0,		0,	
yycrank+0,	yysvec+25,	0,	
yycrank+0,	yysvec+26,	0,	
yycrank+0,	yysvec+28,	0,	
yycrank+0,	yysvec+30,	0,	
yycrank+0,	yysvec+31,	0,	
yycrank+210,	0,		0,	
yycrank+211,	0,		0,	
yycrank+192,	0,		0,	
yycrank+0,	yysvec+38,	0,	
yycrank+0,	yysvec+42,	0,	
yycrank+0,	yysvec+44,	0,	
yycrank+199,	0,		0,	
yycrank+200,	0,		0,	
yycrank+201,	0,		0,	
yycrank+204,	0,		0,	
yycrank+205,	0,		0,	
yycrank+211,	0,		0,	
yycrank+213,	0,		0,	
yycrank+214,	0,		0,	
yycrank+218,	0,		0,	
yycrank+219,	0,		0,	
yycrank+220,	0,		0,	
yycrank+242,	0,		0,	
yycrank+243,	0,		0,	
yycrank+244,	0,		0,	
yycrank+245,	0,		0,	
yycrank+246,	0,		0,	
yycrank+247,	0,		0,	
yycrank+0,	0,		yyvstop+47,
yycrank+248,	0,		0,	
yycrank+249,	0,		0,	
yycrank+261,	0,		0,	
yycrank+257,	0,		0,	
yycrank+252,	0,		0,	
yycrank+259,	0,		0,	
yycrank+253,	0,		0,	
yycrank+270,	0,		0,	
yycrank+272,	0,		0,	
yycrank+326,	0,		0,	
yycrank+336,	0,		0,	
yycrank+385,	0,		yyvstop+49,
yycrank+281,	0,		0,	
yycrank+293,	0,		0,	
yycrank+298,	0,		0,	
yycrank+300,	0,		0,	
yycrank+289,	0,		0,	
yycrank+292,	0,		0,	
yycrank+287,	0,		0,	
yycrank+285,	0,		0,	
yycrank+288,	0,		0,	
yycrank+293,	0,		0,	
yycrank+370,	0,		0,	
yycrank+397,	0,		0,	
yycrank+398,	0,		0,	
yycrank+400,	0,		0,	
yycrank+419,	0,		0,	
yycrank+422,	0,		yyvstop+51,
yycrank+293,	yysvec+139,	0,	
yycrank+435,	0,		0,	
yycrank+310,	yysvec+140,	0,	
yycrank+436,	0,		0,	
yycrank+437,	0,		0,	
yycrank+312,	yysvec+142,	yyvstop+53,
yycrank+312,	yysvec+140,	0,	
yycrank+303,	yysvec+147,	0,	
yycrank+440,	0,		yyvstop+55,
yycrank+299,	yysvec+147,	0,	
yycrank+316,	yysvec+142,	yyvstop+57,
yycrank+439,	0,		yyvstop+59,
yycrank+443,	0,		yyvstop+61,
yycrank+441,	0,		yyvstop+63,
yycrank+401,	0,		0,	
yycrank+455,	0,		0,	
yycrank+475,	0,		0,	
yycrank+0,	0,		yyvstop+65,
yycrank+0,	0,		yyvstop+67,
yycrank+0,	0,		yyvstop+69,
yycrank+0,	0,		yyvstop+71,
yycrank+0,	0,		yyvstop+73,
yycrank+0,	0,		yyvstop+75,
yycrank+0,	0,		yyvstop+77,
yycrank+0,	0,		yyvstop+79,
yycrank+0,	0,		yyvstop+81,
yycrank+0,	0,		yyvstop+83,
yycrank+0,	0,		yyvstop+85,
yycrank+0,	0,		yyvstop+87,
yycrank+0,	0,		yyvstop+89,
yycrank+0,	0,		yyvstop+91,
yycrank+0,	0,		yyvstop+93,
yycrank+0,	0,		yyvstop+95,
yycrank+0,	0,		yyvstop+97,
yycrank+0,	0,		yyvstop+99,
yycrank+0,	0,		yyvstop+101,
yycrank+418,	0,		0,	
yycrank+347,	yysvec+179,	0,	
yycrank+419,	0,		0,	
yycrank+353,	yysvec+179,	0,	
yycrank+420,	0,		0,	
yycrank+422,	0,		0,	
yycrank+429,	0,		0,	
yycrank+492,	0,		0,	
yycrank+415,	0,		0,	
yycrank+529,	0,		yyvstop+103,
yycrank+541,	0,		yyvstop+105,
yycrank+406,	yysvec+189,	yyvstop+107,
yycrank+514,	0,		0,	
yycrank+356,	0,		0,	
yycrank+365,	0,		0,	
yycrank+543,	0,		0,	
yycrank+578,	0,		0,	
yycrank+607,	0,		0,	
yycrank+633,	0,		0,	
yycrank+359,	yysvec+195,	0,	
yycrank+661,	0,		0,	
yycrank+376,	yysvec+196,	0,	
yycrank+687,	0,		0,	
yycrank+713,	0,		0,	
yycrank+377,	yysvec+196,	0,	
yycrank+368,	yysvec+202,	0,	
yycrank+364,	yysvec+202,	0,	
yycrank+481,	yysvec+188,	yyvstop+109,
yycrank+724,	0,		0,	
yycrank+373,	0,		0,	
yycrank+371,	0,		0,	
yycrank+379,	0,		0,	
yycrank+372,	0,		0,	
yycrank+482,	0,		yyvstop+112,
yycrank+395,	0,		0,	
yycrank+396,	0,		0,	
yycrank+390,	0,		0,	
yycrank+398,	0,		0,	
yycrank+387,	0,		0,	
yycrank+400,	0,		0,	
yycrank+385,	0,		0,	
yycrank+399,	0,		0,	
yycrank+419,	0,		0,	
yycrank+402,	0,		0,	
yycrank+419,	0,		0,	
yycrank+419,	0,		0,	
yycrank+739,	0,		0,	
yycrank+749,	0,		0,	
yycrank+759,	0,		0,	
yycrank+477,	0,		yyvstop+114,
yycrank+457,	0,		0,	
yycrank+458,	0,		0,	
yycrank+445,	0,		0,	
yycrank+463,	0,		0,	
yycrank+457,	0,		0,	
yycrank+474,	0,		0,	
yycrank+475,	0,		0,	
yycrank+769,	0,		0,	
yycrank+532,	yysvec+236,	0,	
yycrank+581,	0,		yyvstop+116,
yycrank+779,	0,		0,	
yycrank+828,	0,		yyvstop+118,
yycrank+592,	0,		yyvstop+120,
yycrank+593,	0,		yyvstop+122,
yycrank+838,	0,		0,	
yycrank+864,	0,		yyvstop+124,
yycrank+546,	yysvec+244,	yyvstop+126,
yycrank+496,	0,		0,	
yycrank+490,	0,		0,	
yycrank+497,	0,		0,	
yycrank+490,	0,		0,	
yycrank+511,	0,		0,	
yycrank+505,	0,		0,	
yycrank+502,	0,		0,	
yycrank+514,	0,		0,	
yycrank+513,	0,		0,	
yycrank+608,	0,		yyvstop+128,
yycrank+890,	0,		yyvstop+131,
yycrank+499,	0,		0,	
yycrank+520,	0,		0,	
yycrank+522,	0,		0,	
yycrank+499,	0,		0,	
yycrank+507,	0,		0,	
yycrank+0,	yysvec+260,	0,	
yycrank+524,	0,		0,	
yycrank+535,	0,		0,	
yycrank+537,	0,		0,	
yycrank+532,	0,		0,	
yycrank+0,	yysvec+260,	0,	
yycrank+543,	0,		0,	
yycrank+547,	0,		0,	
yycrank+530,	0,		0,	
yycrank+0,	0,		yyvstop+133,
yycrank+0,	0,		yyvstop+135,
yycrank+0,	0,		yyvstop+137,
yycrank+0,	0,		yyvstop+139,
yycrank+525,	0,		0,	
yycrank+0,	yysvec+275,	0,	
yycrank+547,	0,		0,	
yycrank+0,	yysvec+275,	0,	
yycrank+549,	0,		0,	
yycrank+553,	0,		0,	
yycrank+536,	0,		0,	
yycrank+645,	0,		yyvstop+141,
yycrank+671,	yysvec+186,	yyvstop+143,
yycrank+659,	0,		yyvstop+145,
yycrank+722,	0,		yyvstop+147,
yycrank+660,	0,		yyvstop+149,
yycrank+830,	0,		yyvstop+151,
yycrank+662,	0,		yyvstop+153,
yycrank+550,	0,		0,	
yycrank+916,	yysvec+243,	yyvstop+155,
yycrank+551,	0,		0,	
yycrank+574,	0,		0,	
yycrank+576,	0,		0,	
yycrank+560,	0,		0,	
yycrank+577,	0,		0,	
yycrank+575,	0,		0,	
yycrank+570,	0,		0,	
yycrank+689,	0,		yyvstop+157,
yycrank+691,	0,		0,	
yycrank+653,	yysvec+298,	yyvstop+159,
yycrank+598,	0,		0,	
yycrank+587,	0,		0,	
yycrank+658,	yysvec+212,	yyvstop+161,
yycrank+583,	0,		0,	
yycrank+604,	0,		0,	
yycrank+593,	0,		0,	
yycrank+627,	0,		0,	
yycrank+627,	0,		0,	
yycrank+629,	0,		0,	
yycrank+0,	yysvec+260,	0,	
yycrank+627,	0,		0,	
yycrank+687,	0,		0,	
yycrank+632,	0,		0,	
yycrank+633,	0,		0,	
yycrank+0,	yysvec+275,	0,	
yycrank+634,	0,		0,	
yycrank+689,	0,		0,	
yycrank+751,	0,		0,	
yycrank+647,	0,		0,	
yycrank+635,	0,		0,	
yycrank+629,	0,		0,	
yycrank+650,	0,		0,	
yycrank+638,	0,		0,	
yycrank+655,	0,		0,	
yycrank+901,	0,		0,	
yycrank+0,	yysvec+306,	0,	
yycrank+0,	yysvec+304,	0,	
yycrank+0,	yysvec+306,	0,	
yycrank+0,	yysvec+260,	0,	
yycrank+654,	0,		0,	
yycrank+0,	yysvec+260,	0,	
yycrank+660,	0,		0,	
yycrank+0,	yysvec+275,	0,	
yycrank+0,	yysvec+275,	0,	
yycrank+662,	0,		0,	
yycrank+786,	0,		0,	
yycrank+0,	yysvec+318,	yyvstop+163,
yycrank+0,	yysvec+323,	0,	
yycrank+0,	yysvec+321,	0,	
yycrank+0,	yysvec+323,	0,	
yycrank+683,	0,		0,	
yycrank+927,	0,		yyvstop+165,
yycrank+728,	yysvec+342,	yyvstop+167,
yycrank+0,	yysvec+306,	0,	
yycrank+0,	yysvec+260,	0,	
yycrank+0,	yysvec+275,	0,	
yycrank+800,	0,		0,	
yycrank+0,	yysvec+323,	0,	
yycrank+829,	0,		yyvstop+169,
yycrank+953,	yysvec+325,	yyvstop+171,
yycrank+0,	0,		yyvstop+173,
0,	0,	0};
struct yywork *yytop = yycrank+1013;
struct yysvf *yybgin = yysvec+1;
char yymatch[] ={
00  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,011 ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
011 ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
'0' ,'0' ,'2' ,'3' ,'3' ,'3' ,'6' ,'6' ,
'6' ,'6' ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,'a' ,'a' ,'a' ,'a' ,'a' ,'a' ,'a' ,
'a' ,'a' ,01  ,'k' ,'k' ,'k' ,'n' ,'n' ,
'n' ,'n' ,'n' ,'n' ,'n' ,'n' ,'n' ,'n' ,
'n' ,'n' ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
0};
char yyextra[] ={
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0};
/*	ncform	4.1	83/08/11	*/

