/*  sccsid  =  @(#)eiliterals.h	4.1   (ULTRIX)   7/2/90  */

/*
*	.TITLE	EILITERALS - Literal definitions for ERIT
*	.IDENT	/1-001/
*
* COPYRIGHT (C) 1985 DIGITAL EQUIPMENT CORP.,
* CSSE SOFTWARE ENGINEERING
* MARLBOROUGH, MASSACHUSETTS
*
* THIS SOFTWARE IS FURNISHED UNDER A LICENSE FOR USE ONLY ON A 
* SINGLE COMPUTER SYSTEM AND MAY BE COPIED ONLY WITH THE INCLUSION
* OF THE ABOVE COPYRIGHT NOTICE.  THIS SOFTWARE,  OR ANY OTHER
* COPIES THEREOF, MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE
* TO ANY OTHER PERSON EXCEPT FOR USE ON SUCH SYSTEM AND TO ONE WHO
* AGREES TO THESE LICENSE TERMS.  TITLE TO AND OWNERSHIP OF THE
* SOFTWARE SHALL AT ALL TIMES REMAIN IN DEC.
*
* THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT
* NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL
* EQUIPMENT CORPORATION.
*
* DEC ASSUMES NO RESPONSIBILITY FOR THE USE OR RELIABILITY OF
* ITS SOFTWARE ON EQUIPMENT WHICH IS NOT SUPPLIED BY DEC.
*
*++
*
* FACILITY:		[ FMA Software Tools - Detail Design ]
*
* ABSTRACT:
*
*	This header file contains the literals used by ERIT for error
*	checking and return values to its callers
*	
*	
* ENVIRONMENT:	ULTRIX-32 C
*	
* AUTHOR:  Bob Winant,  CREATION DATE:  28-Jan-86
*
* MODIFIED BY:
*
*--
*/

/* top level facility code */
#define EI$ 100

/* error codes */

#define EI$RDR  -1		/* file read error */
#define EI$FALSE 0
#define EI$NOP   2		/* file not open */
#define EI$FNX   3		/* file doesn't exist */
#define EI$FNC   4		/* file not closed */
#define EI$RND   5		/* Incomplete record */
#define EI$RINV  6		/* Record invalid */
#define EI$NULL  7		/* NULL value */
#define EI$EOF   8		/* end_of_file flag */
#define EI$FAIL  9		/* Everything failed! */
#define EI$NSKT  10		/* no socket available */
#define EI$CORRUPT 9999		/* corrupt record identifier */
#define EI$SCR   12	 	/* corrupt record encountered in socket */
/* good return codes */

#define EI$OPN  1		/* file open */
#define EI$FCL  1		/* file closed */
#define EI$RCG  3		/* EIMS record ok */
#define EI$SUCC 1
#define EI$TRUE EI$SUCC

/* Identification codes */

#define EI$MAILBOX 101		/* User requested direct with OS */
#define EI$REVERSE 102		/* User request for reverse read */
#define EI$APPEND  103		/* User request for write file */

/******************* MSCP FORMAT CODES ***************************/

#define EI$MSCP_CNT_ERR   0	/* Controler error		*/
#define EI$MSCP_BUS_ADR   1	/* Bus address    		*/
#define EI$MSCP_DSK_TRN   2	/* Disk transfer errors		*/
#define EI$MSCP_SDI_ERR   3	/* SDI errors     		*/
#define EI$MSCP_SML_DSK   4	/* Small disk error		*/
#define EI$MSCP_TPE_TRN   5	/* Tape transfer  		*/
#define EI$MSCP_STI_ERR   6	/* STI communications		*/
#define EI$MSCP_STI_DRV   7	/* STI drive error		*/
#define EI$MSCP_STI_FRM   8	/* STI formatter error		*/
#define EI$MSCP_REPLACE   9	/* Bad block replace attempt	*/

