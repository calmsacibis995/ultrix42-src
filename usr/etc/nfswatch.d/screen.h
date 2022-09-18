/*
 * SCCSID: @(#)screen.h	4.2	ULTRIX	1/25/91
 * Based on:
 * $Header: /sparky/a/davy/system/nfswatch/RCS/screen.h,v 3.0 91/01/23 08:23:26 davy Exp $
 *
 * screen.h - definitions for the display screen.
 *
 * David A. Curry				Jeffrey C. Mogul
 * SRI International				Digital Equipment Corporation
 * 333 Ravenswood Avenue			Western Research Laboratory
 * Menlo Park, CA 94025				100 Hamilton Avenue
 * davy@erg.sri.com				Palo Alto, CA 94301
 *						mogul@decwrl.dec.com
 *
 * $Log:	screen.h,v $
 * Revision 3.0  91/01/23  08:23:26  davy
 * NFSWATCH Version 3.0.
 * 
 * Revision 1.2  90/08/17  15:47:12  davy
 * NFSWATCH Version 2.0.
 * 
 * Revision 1.1  88/11/29  11:20:32  davy
 * NFSWATCH Release 1.0
 * 
 */

#define NONNFSLINES	16		/* non-NFS counter lines	*/
#define NFSLINES	(2 * (LINES-NONNFSLINES)) /* NFS counter lines	*/

#define SCR_MIDDLE	40		/* middle of screen, y coord	*/
#define SCR_PKTLEN	17		/* size of packet name field	*/
#define SCR_NFSLEN	17		/* size of file sys name field	*/

/*
 * X0 is the X location of the field name, X is the coordinate of the
 * field value.  Y is the vertical coordinate of the field name and
 * value.
 */
#define SCR_IF_Y	3
#define SCR_HOST_X	0		/* destination host name	*/
#define SCR_HOST_Y	0
#define SCR_DATE_X	28		/* current date			*/
#define SCR_DATE_Y	0
#define SCR_ELAPS_X0	57		/* elapsed time			*/
#define SCR_ELAPS_X	71
#define SCR_ELAPS_Y	0
#define SCR_PKTINT_X0	0		/* packets this interval	*/
#define SCR_PKTINT_X	19
#define SCR_PKTINT_Y	1
#define SCR_PKTTOT_X0	0		/* total packets received	*/
#define SCR_PKTTOT_X	19
#define SCR_PKTTOT_Y	2
#define SCR_PROMPT_X0	0		/* prompt			*/
#define SCR_PROMPT_X	10
#define SCR_PROMPT_Y	(LINES - 1)

#define SCR_PKT_Y	5		/* start of packet counters	*/
#define SCR_PKTHDR_X	21		/* header coords		*/
#define SCR_PKTHDR_Y	4
#define SCR_PKT_INT_X	19		/* interval counter		*/
#define SCR_PKT_PCT_X	26		/* percentage			*/
#define SCR_PKT_TOT_X	31		/* total counter		*/
#define SCR_PKT_NAME_X	0

#define SCR_NFS_Y	15		/* start of nfs counters	*/
#define SCR_NFSHDR_X	5		/* header coords		*/
#define SCR_NFSHDR_Y	14
#define SCR_NFS_INT_X	19		/* interval counter		*/
#define SCR_NFS_PCT_X	26		/* percentage			*/
#define SCR_NFS_TOT_X	31		/* total counter		*/
#define SCR_NFS_NAME_X	0

/*
 * Screen text items to be displayed.
 */
struct scrtxt {
	short	s_x;			/* x coordinate			*/
	short	s_y;			/* y coordinate			*/
	char	*s_text;		/* text to be displayed		*/
};
