/************************************************************************
 *									*
 *			Copyright (c) 1987,1988 by			*
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
 * @(#)i_errno.h	4.1	(ULTRIX)	7/2/90
 */

#ifdef I_ERRNO_GLOB
int i_errno;
#else
extern int	 i_errno;	/* internationalization error global	*/
#endif

/*
 * definitions for the i_errno variable
 */
#define I_ERROR		-1		/* general error return used	*/
#define	I_EBACC		 1		/* inaccessible data base	*/
#define	I_EBADF		 2		/* corrupted data base/msgcat.	*/
#define	I_ENMEM		 3		/* not enough memory		*/
#define	I_EINTL		 4		/* illegal database pointer	*/
#define I_EIPRP		 5		/* illegal property table	*/
#define	I_EICOL		 6		/* illegal collation table	*/
#define I_EICNV		 7		/* illegal conversion table	*/
#define I_EISTI		 8		/* illegal string index table	*/
#define I_EISTR		 9		/* illegal string name		*/
#define	I_EICOD		10		/* illegal code in the database	*/
#define	I_EICTM		11		/* illegal format in i_ctime	*/
#define	I_EICPR		12		/* illegal conversion in doprnt	*/
#define	I_EICSC		13		/* illegal conversion in doscan	*/
#define	I_EIBNV		14		/* bad environment		*/
#define I_EREAD		15		/* read error in message cat.	*/
#define I_ENMSG		16		/* no message available		*/
#define I_EBADC		17		/* bad call of function		*/
#define I_ENOOP		18		/* cannot open message cat.	*/
