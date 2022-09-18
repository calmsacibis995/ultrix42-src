/************************************************************************
 *									*
 *		      Copyright (c) 1987,1988,1989 by			*
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
 * @(#)langinfo.h	4.1	(ULTRIX)	7/2/90
 * definition of langinfo constants
 */

/*
 * Default strings used to format: date and time, date, time.
 *	e.g. Sunday, August 24 21:08:38 MET 1986
 *	     24/08/86
 *	     21:08
 */
#define	D_T_FMT	  "D_T_FMT"	/* string for formatting date and time */
#define D_FMT	  "D_FMT"	/* date */
#define T_FMT	  "T_FMT"	/* time */

/*
 * Strings for AM and PM 
 */
#define AM_STR	  "AM_STR"	/* AM */
#define PM_STR	  "PM_STR"	/* PM */

/*
 * The seven days of the week in their full beauty
 */
#define	DAY_1	  "DAY_1"	/* sunday */
#define	DAY_2	  "DAY_2"	/* monday */
#define	DAY_3	  "DAY_3"	/* tuesday */
#define	DAY_4	  "DAY_4"	/* wednesday */
#define	DAY_5	  "DAY_5"	/* thursday */
#define	DAY_6	  "DAY_6"	/* friday */
#define	DAY_7	  "DAY_7"	/* saturday */

/*
 * The abbreviated seven days of the week
 */
#define	ABDAY_1	  "ABDAY_1"	/* sun */
#define	ABDAY_2	  "ABDAY_2"	/* mon */
#define	ABDAY_3	  "ABDAY_3"	/* tue */
#define	ABDAY_4	  "ABDAY_4"	/* wed */
#define	ABDAY_5	  "ABDAY_5"	/* thu */
#define	ABDAY_6	  "ABDAY_6"	/* fri */
#define	ABDAY_7	  "ABDAY_7"	/* sat */

/*
 * The full names of the twelve months...
 */
#define	MON_1	  "MON_1"	/* january */
#define	MON_2	  "MON_2"	/* february */
#define	MON_3	  "MON_3"	/* march */
#define	MON_4	  "MON_4"	/* april */
#define	MON_5	  "MON_5"	/* may */
#define	MON_6	  "MON_6"	/* june */
#define	MON_7	  "MON_7"	/* july */
#define	MON_8	  "MON_8"	/* august */
#define	MON_9	  "MON_9"	/* september */
#define	MON_10	  "MON_10"	/* october */
#define	MON_11	  "MON_11"	/* november */
#define	MON_12	  "MON_12"	/* december */

/*
 * ... and their abbreviated form
 */
#define	ABMON_1	  "ABMON_1"	/* jan */
#define	ABMON_2	  "ABMON_2"	/* feb */
#define	ABMON_3	  "ABMON_3"	/* mar */
#define	ABMON_4	  "ABMON_4"	/* apr */
#define	ABMON_5	  "ABMON_5"	/* may */
#define	ABMON_6	  "ABMON_6"	/* jun */
#define	ABMON_7	  "ABMON_7"	/* jul */
#define	ABMON_8	  "ABMON_8"	/* aug */
#define	ABMON_9	  "ABMON_9"	/* sep */
#define	ABMON_10  "ABMON_10"	/* oct */
#define	ABMON_11  "ABMON_11"	/* nov */
#define	ABMON_12  "ABMON_12"	/* dec */

/*
 * plus some special strings you might need to know
 */
#define	RADIXCHAR "RADIXCHAR"	/* radix character */
#define	THOUSEP	  "THOUSEP"	/* separator for thousand */
#define	YESSTR	  "YESSTR"	/* affirmative response for yes/no queries */
#define	NOSTR	  "NOSTR"	/* negative response for yes/no queries */
#define CRNCYSTR  "CRNCYSTR"	/* currency symbol */
#define EXPL_STR  "EXPL_STR"	/* lower case exponent character	*/
#define EXPU_STR  "EXPU_STR"	/* upper case exponent character	*/

/*
 * and the definition of the nl_langinfo function
 */
extern char   *nl_langinfo();	/* get a string from the database	*/
