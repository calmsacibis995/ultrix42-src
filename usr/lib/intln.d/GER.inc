/*
 * @(#)GER.inc	4.1
 *
 * This is the second part of the International UNIX data base for
 *
 * 	GERMAN
 *
 */


COLLATION :
	/*
	 * dictionary ordering of characters. (Duden is used as reference)
	 */
	PRIMARY: nul - space; PRIMARY: dot; PRIMARY: A, DIA_A, a, dia_a;
	PRIMARY: B, b; PRIMARY: C, c; PRIMARY: D, d; PRIMARY: E, e;
	PRIMARY: F, f; PRIMARY: G, g; PRIMARY: H, h; PRIMARY: I, i;
	PRIMARY: J, j; PRIMARY: K, k; PRIMARY: L, l; PRIMARY: M, m;
	PRIMARY: N, n; PRIMARY: O, DIA_O, o, dia_o;
	PRIMARY: P, p; PRIMARY: Q, q; PRIMARY: R, r;
	PRIMARY: S, s; PRIMARY: T, t; PRIMARY: U, DIA_U, u, dia_u;
	PRIMARY: V, v; PRIMARY: W, w; PRIMARY: X, x; PRIMARY: Y, y;
	PRIMARY: Z, z;

	sharp_s	= (s, s);

	PRIMARY: REST;
END.

/*
 * The following collation needs a different property table from the one above.
 */
COLLATION TELEPHONE :
	PROPERTY: P_TELEPHONE;
	/*
	 * telephone book ordering of characters.
	 *	- sort umlauts as x+e
	 *	- non letters ignored.
	 */
	PRIMARY: a, A; PRIMARY: b, B; PRIMARY: c, C; PRIMARY: d, D;
	PRIMARY: e, E; PRIMARY: f, F; PRIMARY: g, G; PRIMARY: h, H;
	PRIMARY: i, I; PRIMARY: j, J; PRIMARY: k, K; PRIMARY: l, L;
	PRIMARY: m, M; PRIMARY: n, N; PRIMARY: o, O; PRIMARY: p, P;
	PRIMARY: q, Q; PRIMARY: r, R; PRIMARY: s, S; PRIMARY: t, T;
	PRIMARY: u, U; PRIMARY: v, V; PRIMARY: w, W; PRIMARY: x, X;
	PRIMARY: y, Y; PRIMARY: z, Z;
	sharp_s	= (s, s);
	DIA_A	= (A, e);	/* ASSUMES DIA_A MARKED AS DIPHTONG ! */
	DIA_O	= (O, e);	/* ASSUMES DIA_O MARKED AS DIPHTONG ! */
	DIA_U	= (U, e);	/* ASSUMES DIA_U MARKED AS DIPHTONG ! */
	dia_a	= (a, e);	/* ASSUMES dia_a MARKED AS DIPHTONG ! */
	dia_o	= (o, e);	/* ASSUMES dia_o MARKED AS DIPHTONG ! */
	dia_u	= (u, e);	/* ASSUMES dia_u MARKED AS DIPHTONG ! */
END.

STRINGTABLE :
	NOSTR		= "nein"; 
	EXPL_STR	= 'e';
	EXPU_STR	= 'E';
	RADIXCHAR	= comma;
	THOUSEP		= dot;
	YESSTR		= "ja";
	CRNCYSTR	= "+DM";

	D_T_FMT		= "%a, %d. %b %Y %H:%M:%S" ;
	D_FMT		= "%a, %d. %b %Y";
	T_FMT		= "%H:%M:%S";
	AM_STR		= "AM";
	PM_STR		= "PM";

	DAY_1		= "Sonntag"; 		DAY_2		= "Montag";
	DAY_3		= "Dienstag"; 		DAY_4		= "Mittwoch";
	DAY_5		= "Donnerstag"; 	DAY_6		= "Freitag";
	DAY_7		= "Samstag";

	ABDAY_1		= "So"; 		ABDAY_2		= "Mo";
	ABDAY_3		= "Di"; 		ABDAY_4		= "Mi";
	ABDAY_5		= "Do"; 		ABDAY_6		= "Fr";
	ABDAY_7		= "Sa";

	MON_1		= "Januar";		MON_2		= "Februar";
	MON_3		= M, dia_a, "rz";	MON_4		= "April";
	MON_5		= "Mai";		MON_6		= "Juni";
	MON_7		= "Juli";		MON_8		= "August";
	MON_9		= "September";		MON_10		= "Oktober";
	MON_11		= "November";		MON_12		= "Dezember";

	ABMON_1		= "Jan";		ABMON_2		= "Feb";
	ABMON_3	 	= M, dia_a, r;		ABMON_4		= "Apr";
	ABMON_5	   	= "Mai";		ABMON_6		= "Jun"	;
	ABMON_7		= "Jul";		ABMON_8		= "Aug";
	ABMON_9		= "Sep";		ABMON_10	= "Okt";
	ABMON_11	= "Nov";		ABMON_12	= "Dez";
END.


CONVERSION tolower :
	DEFAULT -> SAME;
	A - Z	 -> a - z;
	DIA_A	-> dia_a;	DIA_O	-> dia_o;
	DIA_U	-> dia_u;
END.

CONVERSION toupper :
	DEFAULT -> SAME;
	a - z	 -> A - Z;
	dia_a	-> DIA_A;	dia_o	-> DIA_O;
	dia_u	-> DIA_U;
END.
