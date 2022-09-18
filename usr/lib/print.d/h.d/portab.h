#ifdef ultrix
#ifdef lint
static char *sccsid = "@(#)portab.h	4.1	(ULTRIX)	7/2/90";
#endif lint
#endif ultrix
/************************************************************************
 *	PORTAB.H - Include file to provide defines for data 		*
 *	declarations.  This version for VAX-11 C.			*
 ************************************************************************/
/***
 * Edit:	gh	 11-MAR-1986 10:39:51 created 
 *		gh	 10-APR-1986 13:07:00 Changed VOID declaration, O_RAW
 ***/

#define BOOLEAN short
#define BYTE	char
#define UBYTE	unsigned char
#define WORD	short
#define UWORD	unsigned short
#define LONG	long
#define DWORD	long
#define ULONG	unsigned long
#define DEFAULT	int			/* Default integer size */

#define REG	register
#define LOCAL	auto
#define EXTERNAL extern
#define MLOCAL	static
#define GLOBAL	/**/
#define VOID	void
#define CONST	readonly 		/* For ansi compatability */
#define NOSHARE noshare			/* For multi-user applications */
#define READONLY readonly 		/* For multi-user applications */


/*----------------------------------------------------------------------*
 *	Miscellaneous Definitions to eliminate need for stdio.h		*
 *----------------------------------------------------------------------*/ 
#define FAILURE (-1)
#define ERROR (-1)
#define SUCCESS (0)
#define NULL 0
#define NULLPTR ((BYTE *)0)		/* Value for a NULL pointer */
#define FALSE 0
#define TRUE (1)

#define MIN(x,y)	(((x) < (y)) ? (x) : (y))
#define MAX(x,y)	(((x) > (y)) ? (x) : (y))

#define FOREVER while (TRUE) {
#define END_FOREVER }

#define O_RAW (0)		/* Needed for inter'C' open() compatability */
