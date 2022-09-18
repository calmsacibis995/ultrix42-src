#ifndef lint
static char Sccsid[] = "@(#)nl_langinfo.c	4.1 (ULTRIX) 7/2/90";
#endif

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
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 * 001	Wendy Rannenberg Fri Feb  2 13:15:49 EST 1990
 *	- Changed the the call to return to check for NULL, and return
 *	  an empty string if true.
 *
 */

#include <i_defs.h>
#include <i_errno.h>
#include <nl_types.h>
#include <locale.h>
#include <langinfo.h>

struct cat_entry {
	char *item;			/* nl_langinfo item	*/
	int   category;			/* category used	*/
	char *C_default;		/* C locale default	*/
};

/*
 * This structure contains the information about which string
 * table is to be accessed depending on which item is requested.
 *
 * If the item is not found in the data structure, then
 * it is assumed to use the LC_TIME category. This is to save
 * on execution time & space.
 */

static struct cat_entry cat_table[] = {

/*
 * NB: Note items are sorted into order for Binary Search !
 *
 *	Item,		Category,	C locale default
 *	=====           =========       ================
 */

	ABDAY_1,	LC_TIME,	"Sun",
	ABDAY_2,	LC_TIME,	"Mon",
	ABDAY_3,	LC_TIME,	"Tue",
	ABDAY_4,	LC_TIME,	"Wed",
	ABDAY_5,	LC_TIME,	"Thu",
	ABDAY_6,	LC_TIME,	"Fri",
	ABDAY_7,	LC_TIME,	"Sat",

	ABMON_1,	LC_TIME,	"Jan",
	ABMON_10,	LC_TIME,	"Oct",
	ABMON_11,	LC_TIME,	"Nov",
	ABMON_12,	LC_TIME,	"Dec",
	ABMON_2,	LC_TIME,	"Feb",
	ABMON_3,	LC_TIME,	"Mar",
	ABMON_4,	LC_TIME,	"Apr",
	ABMON_5,	LC_TIME,	"May",
	ABMON_6,	LC_TIME,	"Jun",
	ABMON_7,	LC_TIME,	"Jul",
	ABMON_8,	LC_TIME,	"Aug",
	ABMON_9,	LC_TIME,	"Sep",

	AM_STR,		LC_TIME,	"AM",
	CRNCYSTR,	LC_MONETARY,	"",

	DAY_1,		LC_TIME,	"Sunday",
	DAY_2,		LC_TIME,	"Monday",
	DAY_3,		LC_TIME,	"Tuesday",
	DAY_4,		LC_TIME,	"Wednesday",
	DAY_5,		LC_TIME,	"Thursday",
	DAY_6,		LC_TIME,	"Friday",
	DAY_7,		LC_TIME,	"Saturday",

	D_FMT,		LC_TIME,	"%m/%d/%y",
	D_T_FMT,	LC_TIME,	"%a %b %d %H:%M:%S %Y",

	EXPL_STR,	LC_NUMERIC,	"e",
	EXPU_STR,	LC_NUMERIC,	"E",

	MON_1,		LC_TIME,	"January",
	MON_10,		LC_TIME,	"October",
	MON_11,		LC_TIME,	"November",
	MON_12,		LC_TIME,	"December",
	MON_2,		LC_TIME,	"February",
	MON_3,		LC_TIME,	"March",
	MON_4,		LC_TIME,	"April",
	MON_5,		LC_TIME,	"May",
	MON_6,		LC_TIME,	"June",
	MON_7,		LC_TIME,	"July",
	MON_8,		LC_TIME,	"August",
	MON_9,		LC_TIME,	"September",

	NOSTR,		LC_ALL,		"no",
	PM_STR,		LC_TIME,	"PM",
	RADIXCHAR,	LC_NUMERIC,	".",
	THOUSEP,	LC_NUMERIC,	"",
	T_FMT,		LC_TIME,	"%H:%M:%S",
	YESSTR,		LC_ALL,		"yes"

};

#define	 NO_OF_ENTRIES	(sizeof(cat_table) / sizeof(struct cat_entry))


/*
 * nl_langinfo -- return information that is culture dependent form the database
 *
 * SYNOPSIS:
 *	char *
 *	nl_langinfo(item)
 *	nl_item item;
 *
 * DESCRIPTION:
 *	This function is found in the X/OPEN guide, section on nl_langinfo(3C).
 *	It is mapped in our system to a call of the i_getstr function.
 *
 * RETURN:
 *	This function returns a null string in case the
 *	information cannot be found.
 */

char *
nl_langinfo(item)
nl_item item;
{
	int  cat, loop;				/* category & loop vars	*/
	int  scmp();				/* string cmp function	*/
	char *result;				/* return result	*/
	char *entry, *bsearch();		/* table entry pointer  */
	str_tab *tblptr;			/* pointer to string tbl*/
	struct cat_entry key;			/* used as key to bsearch */


	/*
	 * Do a binary search into table.
	 */
	key.item = (char *)item;
	entry = bsearch((char *)&key, (char *)cat_table, NO_OF_ENTRIES, sizeof(struct cat_entry), scmp);

	/*
	 * If a non-X/Open specified string is requested, handle that here.
 	 * 001 - changed so it doesn't simply return the value from 
	 * i_getstr, which is NULL if the string is not found.
	 */
	if (entry == (char *)0) {
		result = (i_getstr((char *)item, _lc_strtab[LC_ALL]));
		return(result ? result : "");
	
	}

	/*
	 * now look up string table of selected category for required
	 * item, and check it is not NULL (if so return default).
	 */
	cat = ((struct cat_entry *)entry)->category;
	if (_lc_strtab[cat] == (str_tab *)0) {
		return(((struct cat_entry *)entry)->C_default);
	}
	else
		tblptr = _lc_strtab[cat];

	/*
	 * return required string or null if item is not present
	 */
	result = i_getstr((char *)item, tblptr);

	   return(result ? result : "");
}


/*
 * This function de-references the strings passed to it & calls strcmp()
 * to do the string comparision for the bsearch() library function.
 */

scmp(s1, s2)
struct cat_entry *s1, *s2;
{
	return(strcmp(s1->item, s2->item));
}
