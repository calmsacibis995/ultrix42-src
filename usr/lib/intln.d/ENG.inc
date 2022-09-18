/*
 * @(#)ENG.inc	4.1
 */

COLLATION :
	PRIMARY: nul - space;
	PRIMARY: dot;
	PRIMARY: a, A; 	PRIMARY: b, B; 	PRIMARY: c, C;
	PRIMARY: d, D; 	PRIMARY: e, E; 	PRIMARY: f, F;
	PRIMARY: g, G; 	PRIMARY: h, H; 	PRIMARY: i, I;
	PRIMARY: j, J; 	PRIMARY: k, K; 	PRIMARY: l, L;
	PRIMARY: m, M; 	PRIMARY: n, N;  PRIMARY: o, O;
	PRIMARY: p, P; 	PRIMARY: q, Q; 	PRIMARY: r, R;
	PRIMARY: s, S; 	PRIMARY: t, T; 	PRIMARY: u, U;
	PRIMARY: v, V; 	PRIMARY: w, W; 	PRIMARY: x, X;
	PRIMARY: y, Y; 	PRIMARY: z, Z;
	PRIMARY: REST;
END.

STRINGTABLE :
	EXPL_STR	= "e";
	EXPU_STR	= "E";
	NOSTR		= "no";
	RADIXCHAR	= dot;
	THOUSEP		= comma;
	YESSTR		= "yes";
	CRNCYSTR	= "+", pound;

	D_T_FMT		= "%a %b %d %H:%M:%S %Z %Y";
	D_FMT		= "%m/%d/%y";
	T_FMT		= "%H:%M:%S";
	AM_STR		= "AM";
	PM_STR		= "PM";

	DAY_1		= "Sunday";		DAY_2		= "Monday";
	DAY_3		= "Tuesday";		DAY_4		= "Wednesday";
	DAY_5		= "Thursday";		DAY_6		= "Friday";
	DAY_7		= "Saturday";

	ABDAY_1		= "Sun";		ABDAY_2		= "Mon";
	ABDAY_3		= "Tue";		ABDAY_4		= "Wed";
	ABDAY_5		= "Thu";		ABDAY_6		= "Fri";
	ABDAY_7		= "Sat";

	MON_1		= "January";		MON_2		= "February";
	MON_3		= "March";		MON_4		= "April";
	MON_5		= "May";		MON_6		= "June";
	MON_7		= "July";		MON_8		= "August";
	MON_9		= "September";		MON_10		= "October";
	MON_11		= "November";		MON_12		= "December";

	ABMON_1		= "Jan";		ABMON_2		= "Feb";
	ABMON_3		= "Mar";		ABMON_4		= "Apr";
	ABMON_5		= "May";		ABMON_6		= "Jun";
	ABMON_7		= "Jul";		ABMON_8		= "Aug";
	ABMON_9		= "Sep";		ABMON_10	= "Oct";
	ABMON_11	= "Nov";		ABMON_12	= "Dec";
END.

#include "toupper.cnv"
#include "tolower.cnv"
