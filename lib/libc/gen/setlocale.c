#ifndef lint
static char Sccsid[] = "@(#)setlocale.c	4.1 (ULTRIX) 7/3/90";
#endif /* lint */

/************************************************************************
 *									*
 *			Copyright (c) 1987,1988,1989 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *		            Bull, France				*
 *			   Siemens AG, FR Germany			*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 *
 *   File name: 	setlocale.c
 *
 *   Source file description:
 *	This file implements the ANSI setlocale() function. This is done
 *	by mapping onto the underlying base functions i_init() and i_ld*().
 *	Setlocale() is also called by the X/OPEN function nl_init().
 *
 *   Functions:
 *	setlocale()
 *
 */

/*
 * Modification history
 * ~~~~~~~~~~~~~~~~~~~~
 *
 * 004  Dan Smith Feb 26, 1990
 *      Defined localeconv function. Removed spurious check for C locale
 *      in setuptable(). Added code to set _pctype_siz which is the
 *      size of the _pctype table. This is used by the multi-byte to
 *      wide character functions.
 *
 * 003  Wendy Rannenberg Jan 25, 1990
 *	- Fixed up pointers to dbheaders so alternate property,
 *	collation, and string tables could be accessed.
 *
 * 002	David Lindner Thu Jan  4 11:15:15 EST 1990
 *	- Modified setlocale so default language definitions could be 
 *	  specified with LANG environment variable.
 *
 * 001	MRH 25-Sep-1987
 *	- Updated to current ANSI & X/OPEN standards.
 *
 * 000	ARG 3-Mar-1987
 *	- Created.
 *
 */

#include <i_defs.h>
#include <locale.h>
#include <limits.h>
#include <ctype.h>		/* for definitions of tables */
#include <langinfo.h>

#define CNULL	(char *)0
#define SEPCH	'\001'
#define SEPSTR	"\001"
#define FALSE	0
#define TRUE	1


/* 
 * setlocale -- set the program's locale.
 *	
 * SYNOPSIS:
 *	char *
 *	setlocale(category, locale)
 *	int   category;
 *	char *locale;
 *
 * DESCRIPTION:
 *	This is the ANSI defined function used to set the programs locale
 *	for the specified category, from <locale.h>. If this call fails
 *	NO changes are made to ANY of the operations of the program.
 *
 * RETURN:
 *	Returns the string specified by the locale for category. Currently
 *	always the LANG string. If the call fails then a NULL pointer is
 *	returned.
 */

struct locale_info {
	char *environ;			/* the related environment var  */
	char setting[2 * NL_LANGMAX + 1];	/* the current setting	*/
};

static struct locale_info cur_locale[] = {
	"LANG", 	"C", 	/* LC_ALL */
	"LC_COLLATE", 	"C",
	"LC_CTYPE",	"C",
	"LC_NUMERIC",	"C",
	"LC_TIME",	"C",
	"LC_MONETARY",	"C"
};

struct new_locale_info {
	char new_setting[2*NL_LANGMAX+1];	/* new setting values	*/
	intl *pi;			/* new values of table pointers	*/
};

static struct new_locale_info new_locale[_LC_MAX + 1];

static col_tab *collate;		/* collation table pointer	*/
static prp_tab *property;		/* property table pointer	*/
static str_tab *numstr, *timestr, *monstr;
				        /* string table pointers	*/
static struct lconv lconv;		/* for localeconv result */

char *
setlocale(category, locale)
register int   category;
char *locale;
{
	register int loop;
	register int LC_all_ok;
	register char *nptr, *locptr;
	char nsettings[2*NL_LANGMAX+1];
	char *return_settings(), *getenv();

	/*
	 * check for a query on locale
	 */
	if (locale == CNULL)
		return(return_settings(category));

	/*
	 * special case of setlocale(LC_ALL, setlocale(LC_ALL, NULL))
	 * (ie. when a queried value of LC_ALL is fed back into setlocale).
	 */
	if (category == LC_ALL && *locale == SEPCH) {
		locptr = locale;
		for (loop=LC_ALL; loop <= _LC_MAX; loop++) {
			nptr = nsettings;
			locptr++;
			while (*locptr != SEPCH)
				*nptr++ = *locptr++;
			*nptr = '\0';
			if (setuptable(loop, nsettings) != 0)
				return(CNULL);
		}
		commit_tables(LC_ALL);
		return(locale);
	}

	/*
	 * change locale of a single category
	 */
	if (category != LC_ALL) {
		/*
		 * verify category is within range before attempeting setup
		 */
		if (category > LC_ALL && category <= _LC_MAX && setuptable(category, locale) == 0) {
			commit_tables(category);
			return(new_locale[category].new_setting);
		}
		else
			return(CNULL);
	}

	/*
	 * final case of when a string is not a component.
	 * 
	 * NB: Take care of the special case of setlocale(LC_ALL, "") where
	 * all the LC_* variables are set, but not $LANG! This is still a
	 * legal call.
	 */
	LC_all_ok = FALSE;
	for (loop = LC_ALL; loop <= _LC_MAX; loop++) {
		if (setuptable(loop, locale) != 0) {
			if (loop != LC_ALL)
				return(CNULL);
			else
				LC_all_ok = TRUE;
		}
	}

	if (LC_all_ok == FALSE)
		commit_tables(LC_ALL);
	else
		for (loop = LC_ALL + 1; loop <= _LC_MAX; loop++)
			commit_tables(loop);

	return(return_settings(LC_ALL));
}



/*
 * setuptable -- set up & load a table from category
 *
 * SYNOPSIS
 *	setuptable(cat, locale)
 *	int cat;
 *	char *locale;
 *
 * DESCRIPTION
 *
 *	Sets up a table specified by cat according to the value passed
 *	in locale. The setting of locale can either be a full locale
 *	setting, a table specification or null. If a null locale is
 *	given, setuptables will use the correct environment variable,
 *	or if this is not set, the value of LC_ALL (LANG).
 *
 *	The function loadtables is called to perform the required loading
 *	from the tables. If at any point in the an error is encountered,
 *	setuptables will exit & return an error.
 *
 * RETURNS
 *	0 on success or -1 on error.
 */

static
setuptable(cat, locale)
register int cat;
register char *locale;
{
	char lang[NL_LANGMAX + 1];	/* used to split locale into	*/
	char terr[NL_LANGMAX + 1];	/* required components		*/
	char code[NL_LANGMAX + 1];
	char table[NL_LANGMAX + 1];
	char namebuf[2 * NL_LANGMAX + 1];/* buffers to hold full names	*/
	char dictbuf[2 * NL_LANGMAX + 1];
	char *valptr;			/* used to point to next valid	*/
					/* locale			*/
	char *getenv(), *setname();
	intl *pi;			/* table pointer		*/

	/*
	 * if locale is empty, search appropiate environment variable,
	 * or if not set, $LANG.
	 */
	if (*locale == '\0')
		if ((locale = getenv(cur_locale[cat].environ)) == CNULL || *locale == '\0')
			if ((locale = getenv("LANG")) == CNULL)
				return(-1);

	/* 
 	 * DJL 002
	 * if environment set to C, simply set the name return
 	 */
	if (locale[0] == 'C' && locale[1] == '\0') {
		setname(new_locale[cat].new_setting, "C", CNULL, CNULL, CNULL);
		return (0);
	}

	/*
	 * if just table modifier specified, correct table to prefix to
	 */
	if (*locale == '@') {
		if ((valptr = getenv(cur_locale[cat].environ)) == CNULL || *valptr == '\0' || *valptr == '@')
			if ((valptr = getenv("LANG")) == CNULL || *valptr == '\0' || *valptr == '@')
				return(-1);
		strcpy(dictbuf, valptr);
		strcat(dictbuf, locale);
		locale = dictbuf;
	}

	/*
	 * split locale name into components
	 */
	if (form_lang(locale, lang, terr, code, table) == -1)
		return(-1);

	/*
	 * attempt to open database and return file point on success
	 * or exit on error.
	 */
	setname(namebuf,lang, terr, code, CNULL);
	if ((pi = i_init(namebuf)) == (intl *)0)
		return(-1);
	if (loadtables(cat, pi, table) == -1) {
		/*
		 * we can't call i_end here since databases can be shared
		 * amongst several categories and if we called i_end then
		 * we could free up a table used by another category!!!
		 */
		return(-1);
	}
	setname(new_locale[cat].new_setting, lang, terr, code, table);
	new_locale[cat].pi = pi;
	return(0);
}



/*
 * loadtables -- load the appropriate tables from category
 *
 * SYNOPSIS
 *	loadtables(cat, pi, table)
 *	int cat;
 *	intl *pi;
 *	char *table;
 *
 * DESCRIPTION
 *	Load the tables as specified by cat. Once all have been loaded
 *	successfully switch the environment. If at any point we encounter 
 *	a problem, stop and return an error.
 *
 * RETURNS
 *	0 on success and -1 on error
 */


static
loadtables(cat, pi, table)
register int cat;
register intl *pi;
char *table;
{
	switch (cat) {
	case LC_COLLATE:
		/*
	 	 * if a table given try to load it otherwise use the
		 * default (load it if necessary). 
		 */
		if (*table)
			pi->in_cldflt = collate = i_ldcol(table, pi);
		else if (pi->in_cldflt == (col_tab *)0) {
			if ((collate = i_ldcol(COL_DEF, pi)) == (col_tab *)0)
				return -1;
			pi->in_cldflt = collate;
		} else
		   	collate = pi->in_cldflt;

		/*
		 * could not load the requested table so giveup now
		 */
		if (collate == (col_tab *)0)
			return -1;

		break;

	case LC_CTYPE:
		/* 
		 * if a table given try to load it otherwise use the 
		 * default (load it if necessary)
		 */
		if (*table)
			pi -> in_prdflt = property = i_ldprp(table, pi);
		else if (pi->in_prdflt == (prp_tab *)0) {
			if ((property = i_ldprp(PRP_DEF, pi)) == (prp_tab *)0)
				return -1;
			pi->in_prdflt = property;
		} else
		   	property = pi->in_prdflt;

		/*
		 * could not load the requested table so giveup now
		 */
		if (property == (prp_tab *)0)
			return -1;

		/* 
		 * load the conversion tables
		 */
		if (pi->in_ilower == (cnv_tab *)0 && 
		   (pi->in_ilower = i_ldcnv("tolower", pi)) == (cnv_tab *)0)
			return -1;

		if (pi->in_iupper == (cnv_tab *)0 && 
		   (pi->in_iupper = i_ldcnv("toupper", pi)) == (cnv_tab *)0)
			return -1;

		break;

	case LC_NUMERIC:

		/* 
		 * if a table given try to load it otherwise use the 
		 * default (load it if necessary)
		 */
		if (*table)
			pi->in_sgdflt = numstr = i_ldstr(table, pi);
		else {
			if (pi->in_sgdflt == (str_tab*)0) {
			   if (	(numstr = i_ldstr(FRM_DEF, pi)) == (str_tab*)0)
				return -1;
		           pi->in_sgdflt = numstr;
		        } else
			
			   numstr = pi->in_sgdflt;
		}

		/*
		 * could not load the requested table so giveup now
		 */
		if (numstr == (str_tab *)0) {
			return -1;
		}

		break;

	case LC_TIME:

		/* 
		 * if a table given try to load it otherwise use the 
		 * default (load it if necessary)
		 */
		if (*table)
			pi->in_sgdflt = timestr = i_ldstr(table, pi);


		else {
			if (pi->in_sgdflt == (str_tab*)0) {
			   if (	(timestr = i_ldstr(FRM_DEF, pi)) == (str_tab*)0)
				return -1;
		           pi->in_sgdflt = timestr;
		        } else
			
			   timestr = pi->in_sgdflt;
		}

		/*
		 * could not load the requested table so giveup now
		 */
		if (timestr == (str_tab *)0) {
			return -1;
		}

		break;

	case LC_MONETARY:
		/* 
		 * if a table given try to load it otherwise use the 
		 * default (load it if necessary)
		 */
		if (*table)
			pi->in_sgdflt = monstr = i_ldstr(table, pi);
		else {
			if (pi->in_sgdflt == (str_tab*)0) {
			   if (	(monstr = i_ldstr(FRM_DEF, pi)) == (str_tab*)0)
				return -1;
		           pi->in_sgdflt = monstr;
		        } else
			
			   monstr = pi->in_sgdflt;
		}

		/*
		 * could not load the requested table so giveup now
		 */
		if (monstr == (str_tab *)0) {
			return -1;
		}

		break;
	}
	return(0);
}



/*
 * commit_tables -- switch to new locale setting(s).
 *
 * SYNOPSIS
 *	commit_tables(cat)
 *	int cat;
 * 
 * DESCRIPTION
 *	Only once all the specified new locales have be validated, can
 *	the value of the locales be changed (as an error must leave the
 *	international environment unchanged).
 *
 *	Once all locale values to be changed have been successfully setup
 *	with setuptable, commit_tables should be called to switch the
 *	international environment to the new setting(s).
 *
 * RETURNS
 *	None.
 *
 */


/* 
 * a useful macro used below to get strings from the string table
 */
#define getstr(string, def) ((cp = i_getstr(string, _lc_strtab[cat])) ? *cp : def)

/*
 * another useful macro to load in string table for that category
 */
#define ldstrtab(LC_CATEGORY) i_ldstr(FRM_DEF, new_locale[LC_CATEGORY].pi)

/*
 * yet another useful macro to test to see if a category is set to the
 * C locale.
 */
#define isclocale(CAT) (new_locale[CAT].new_setting[0] == 'C' && new_locale[CAT].new_setting[1] == '\0')

static
commit_tables(cat)
register int cat;
{
        char *cp;
        intl *pi;
	extern	unsigned long _pctype_siz;

	switch (cat) {

	case LC_ALL:
		/*
		 * load up string table for LC_ALL
		 */
		_lc_strtab[LC_ALL] = (isclocale(LC_ALL)) ? (str_tab *) 0 : ldstrtab(LC_ALL);
		strcpy(cur_locale[LC_ALL].setting, new_locale[LC_ALL].new_setting);
		/* drop through */

	case LC_COLLATE:
		if (isclocale(LC_COLLATE)) {
			_lc_cldflt = (col_tab *)0;
			_lc_strtab[LC_COLLATE] = (str_tab *)0;
		} else {
			_lc_cldflt = collate;
			_lc_strtab[LC_COLLATE] = ldstrtab(LC_COLLATE);
		}
		strcpy(cur_locale[LC_COLLATE].setting, new_locale[LC_COLLATE].new_setting);

		/*
		 * if all locales to be committed, then drop through switch
		 * instruction. This has been done purely to minimise
		 * execution time.
		 */
		if (cat != LC_ALL)
			break;
		/* drop through */

	case LC_CTYPE:
		if (isclocale(LC_CTYPE)) {
			_pctype = _ctype__;
			_pctype_siz = 255; 	/* 004	*/
			_lc_prdflt = (prp_tab *)0;
			_lc_tolower = (cnv_tab *)0;
			_lc_toupper = (cnv_tab *)0;
			_lc_strtab[LC_CTYPE] = (str_tab *)0;
		} else {
			pi = new_locale[LC_CTYPE].pi;
			/*
			 * minus one to take account of EOF in ctype macros
			 */
			_pctype = pi->in_prdflt->prp_tbl - 1;
			_pctype_siz = pi->in_prdflt->prp_nbspl; /* 004 */
			_lc_prdflt = property;
			_lc_tolower = pi->in_ilower;
			_lc_toupper = pi->in_iupper;
			_lc_strtab[LC_CTYPE] = ldstrtab(LC_CTYPE);
		}
		strcpy(cur_locale[LC_CTYPE].setting, new_locale[LC_CTYPE].new_setting);
		if (cat != LC_ALL)
			break;
		/* drop through */

	case LC_NUMERIC:
		if (isclocale(LC_NUMERIC)) {
			_lc_thosep = ',';
			_lc_radix  = '.';
			_lc_exl    = 'e';	
			_lc_exu    = 'E';
			_lc_strtab[LC_NUMERIC] = (str_tab *)0;
		} else {
			pi = new_locale[LC_NUMERIC].pi;
			/*
			 * setup string table now as it is used by getstr
			 */
			_lc_strtab[LC_NUMERIC] = numstr;
			_lc_thosep = getstr(THOUSEP, ',');
			_lc_radix  = getstr(RADIXCHAR, '.');
			_lc_exl    = getstr(EXPL_STR, 'e');
			_lc_exu    = getstr(EXPU_STR, 'E');
		}
		strcpy(cur_locale[LC_NUMERIC].setting, new_locale[LC_NUMERIC].new_setting);
		if (cat != LC_ALL)
			break;
		/* drop through */

	case LC_TIME:
		if (isclocale(LC_TIME)) 	
		 	_lc_strtab[LC_TIME] = (str_tab *)0;
		else
			_lc_strtab[LC_TIME] = timestr;

		strcpy(cur_locale[LC_TIME].setting, new_locale[LC_TIME].new_setting);
		if (cat != LC_ALL)
			break;
		/* drop through */

	case LC_MONETARY:
		if (isclocale(LC_MONETARY))
			_lc_strtab[LC_MONETARY] = (str_tab *)0;
		else
			_lc_strtab[LC_MONETARY] = monstr;

		strcpy(cur_locale[LC_MONETARY].setting, new_locale[LC_MONETARY].new_setting);
		if (cat != LC_ALL)
			break;
		/* drop though */
	}
}



/*
 * setname -- set the name for a LANG type variable
 * 
 * SYNOPSIS
 *	char *
 * 	setname(name, lang, terr, code, table)
 *	char *name, *lang, *terr, *code, *table;
 *
 * DESCRIPTION
 *	Concatenates all the components (if set) into the name field which
 *	is then returned. This call cannot fail name MUST be long enough
 *	for the result which will be no longer than 2 * NL_LANGMAX + 1.
 *	NOTE also that lang MUST have a non-null value.
 *
 * RETURNS
 *	The concatenated string.
 */

static
char *
setname(name, lang, terr, code, table)
register char *name, *lang, *terr, *code, *table;
{	
	strcpy(name, lang);
	if (terr && *terr) {
		strcat(name, "_");
		strcat(name, terr);
	}
	if (code && *code) {
		strcat(name, ".");
		strcat(name, code);
	}
	if (table && *table) {
		strcat(name, "@");
		strcat(name, table);
	}
	return name;
}



/*
 * return_settings -- return current locale settings
 *
 * SYNOPSIS
 *	char *
 *	return_settings(cat)
 *	int cat;
 *
 * DESCRIPTION
 *	Returns a pointer to the locale setting of the specified category.
 *	If LC_ALL is inquired, then a composite string is return which
 *	can be passed back into setlocale to restore a locale setting.
 *
 * RETURNS
 *	Pointer to buffer containing required locale settings.
 *	NULL if the category is out of range
 */

static
char *
return_settings(cat)
int cat;
{
	int i;
	static char resultbuf[(2 * NL_LANGMAX + 1) * (_LC_MAX + 1)];

	if (cat == LC_ALL) {
		strcpy(resultbuf, SEPSTR);
		for (i=LC_ALL; i <= _LC_MAX; i++) {
			strcat(resultbuf, cur_locale[i].setting);
			strcat(resultbuf, SEPSTR);
		}
		return(resultbuf);
	}
	if (cat > LC_ALL && cat <= _LC_MAX)
		return(cur_locale[cat].setting);
	else
		return (CNULL);
}

/* 
 * macro to get the string corresponing to  symbol from the strings table
 * of locale cat. The string is assigned to the field of lconv.
 * If the string returned was null, then that field of lconv gets
 * assigned to default.
 *
*/

#define SETLCONV(symbol,cat,field,default)\
if ((lconv.field = i_getstr(symbol, _lc_strtab[cat]))\
     		== (char *) 0) lconv.field = default

/*
 * This macro is similar to the one above except that the
 * category is always LC_MONETARY
 *
*/

#define SETLCONV_MON(symbol,field,default)\
if ((lconv.field = i_getstr(symbol, _lc_strtab[LC_MONETARY]))\
     		== (char *) 0) lconv.field = default

/*
 * This macro is similar to the one above except that 
 * the first character of the string returned is used.
 *
*/


#define SETLCONV_MON_CHAR(symbol,field,default)\
if ((cp  = i_getstr(symbol, _lc_strtab[LC_MONETARY]))\
     		== (char *) 0) lconv.field = default;\
else\
	lconv.field = cp[0]

/*
 *
 * SYNOPSIS:
 *	struct lconv *
 *      localeconv(void)
 *
 * DESCRIPTION:
 *    	This is the ANSI defined function used to set the components
 *      of static struct lconv with values appropriate for the 
 *      formatting of numeric quantities according to the rules 
 *	of the current locale. The semantics are defined in 4.4.2.1
 *      of the ANSI Draft.
 *
 *      Following identifiers have to be defined in the international-
 *      ization database so that localeconv is aware of them. 
 *
 *	ic identifier  ic example	category    affected field    default
 *
 *	RADIXCHAR	= ".";		LC_NUMERIC   decimal_point 	"."
 *	THOUSEP  	= "";		LC_NUMERIC   thousands_sep 	""
 *	GROUPING 	= "";		LC_NUMERIC   grouping  		""
 *
 *      Monetary example for Italy follows 
 *
 *      INT_CURR_SYMB  = "ITL.";	LC_MONETARY  int_curr_symbol 	"" 
 *      CRNCYSTR       = "L.";		LC_MONETARY  currency_symbol 	""
 *	MON_DEC_PT     = "";		LC_MONETARY  mon_decimal_point 	""
 *	MON_THOUSEP    = ".";		LC_MONETARY  mon_thousands_sep 	""
 *	MON_GROUPING   = etx, '\0';	LC_MONETARY  mon_grouping 	""
 *	POSITIVE_SIGN  = "";		LC_MONETARY  positive_sign  	""
 *	NEGATIVE_SIGN  = "-";		LC_MONETARY  negative_sign 	""
 *	INT_FRAC_DIG   = '0';		LC_MONETARY  int_frac_digits 	CHAR_MAX
 *	FRAC_DIGITS    = '0';		LC_MONETARY  frac_digits 	CHAR_MAX
 *	P_CS_PRECEDES  = '1';		LC_MONETARY  p_cs_precedes 	CHAR_MAX
 *	P_SEP_BY_SPAC  = '0';		LC_MONETARY  p_sep_by_space 	CHAR_MAX
 *	N_CS_PRECEDES  = '1';		LC_MONETARY  n_cs_precedes 	CHAR_MAX
 *	N_SEP_BY_SPAC  = '0';		LC_MONETARY  n_sep_by_space 	CHAR_MAX
 *	P_SIGN_POSN    = '1';		LC_MONETARY  p_sign_posn 	CHAR_MAX
 *	N_SIGN_POSN    = '1';		LC_MONETARY  n_sign_posn 	CHAR_MAX
 *
 *
 *
 * RETURN:
 *     Returns the address of the static struct lconv.
 *
 *
 */


#if __STDC__ == 1
struct lconv *localeconv(void)
#else
struct lconv *localeconv()
#endif
{
	char *cp;

/* 	Since the C Locale values of lconv are the same as the
 *	values returned if a value is not available, no special
 *	check for the C Locale is made (for all cases).
 *	For C Locale the string table is null always, i.e
 *      i_getstr always returns 0.
 */

	SETLCONV("RADIXCHAR",LC_NUMERIC,decimal_point,".");
	SETLCONV("THOUSEP",  LC_NUMERIC,thousands_sep,"");
	SETLCONV("GROUPING", LC_NUMERIC,grouping,     "");

	SETLCONV_MON("INT_CURR_SYMB",     int_curr_symbol,   ""); 
	SETLCONV_MON("CRNCYSTR",          currency_symbol,   "");
	SETLCONV_MON("MON_DEC_PT",        mon_decimal_point, "");
	SETLCONV_MON("MON_THOUSEP",       mon_thousands_sep, "");
	SETLCONV_MON("MON_GROUPING",      mon_grouping,      "");
	SETLCONV_MON("POSITIVE_SIGN",     positive_sign,     "");
	SETLCONV_MON("NEGATIVE_SIGN",     negative_sign,     "");

	SETLCONV_MON_CHAR("INT_FRAC_DIG",      int_frac_digits,   CHAR_MAX);
	SETLCONV_MON_CHAR("FRAC_DIGITS",       frac_digits,       CHAR_MAX);
	SETLCONV_MON_CHAR("P_CS_PRECEDES",     p_cs_precedes,     CHAR_MAX);
	SETLCONV_MON_CHAR("P_SEP_BY_SPAC",     p_sep_by_space,    CHAR_MAX);
	SETLCONV_MON_CHAR("N_CS_PRECEDES",     n_cs_precedes,     CHAR_MAX);
	SETLCONV_MON_CHAR("N_SEP_BY_SPAC",     n_sep_by_space,    CHAR_MAX);
	SETLCONV_MON_CHAR("P_SIGN_POSN",       p_sign_posn,       CHAR_MAX);
	SETLCONV_MON_CHAR("N_SIGN_POSN",       n_sign_posn,       CHAR_MAX);
	return(&lconv);
}
