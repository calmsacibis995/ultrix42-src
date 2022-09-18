/*  sccsid  =  @(#)ueliterals.h	4.3   (ULTRIX)   10/16/90  */

/*
*	.TITLE	UELITERALS - Literal definitions for UERF
*	.IDENT	/1-001/
*
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
*	This header file contains the literals used by UERF
*	
*	
* ENVIRONMENT:	ULTRIX-32 C
*	
* AUTHOR:  Luis Arce,  CREATION DATE:  17-Feb-86
*
* MODIFIED BY:
*
*--
*/

#ifndef _UELITERALS_
#define _UELITERALS_

/************************  LITERALS     ************************/

/************************	Do not change the version number
				this is done automaticaly by the
				script Maint.			*/

#define  UE$VER_NUM		" uerf version 4.2-011 "

/****************************************************************/

#define  UE$ERR_FILE		"uerf.err"
#define  UE$HLP_FILE		"uerf.hlp"
#define  UE$BIN_FILE		"uerf.bin"

#define  UE$FILE_CONF		"/etc/elcsd.conf"
#define  UE$HELP_HDR		"\n\n\n\n\t\t\t U E R F   H E L P \n\n"

#ifndef  TRUE
#define  TRUE		(1 == 1)
#define  FALSE		(0 == 1)
#endif   TRUE

#define  UE$NULL	 0		/* NULL literal		*/
#define  UE$HST_LEN	32
#define  UE$XFF		255
#define  UE$OUT_BRIEF	 0		/* brief output flag	*/
#define  UE$OUT_FULL	 1		/* full output flag	*/
#define  UE$OUT_TERSE 	 2		/* terse output flag	*/

#define  UE$GOTONE	 1		/* multi request	*/

/************************  RETURN CODES  ************************/

#define  UE$FAIL	-1		/* Failure		*/
#define  UE$SUCC	 1		/* Success		*/
#define  UE$DONE	 2              /* Terminate processing	*/
#define  UE$STOP	 3		/* Stop request by user	*/

/* other return codes are same as error codes 			*/

/************************  FLAG VALUES   ************************/

#define  UE$FLG_h	'h'		/* Help			*/
#define  UE$FLG_f	'f'		/* Input file		*/
#define  UE$FLG_n	'n'		/* Buffer input		*/
#define  UE$FLG_x	'x'		/* Exclude		*/
#define  UE$FLG_c	'c'		/* Class		*/
#define  UE$FLG_D	'D'		/* Disk			*/
#define  UE$FLG_T	'T'		/* Tape			*/
#define  UE$FLG_u	'u'		/* Unit 		*/
#define  UE$FLG_e	'e'		/* Error		*/
#define  UE$FLG_O	'O'		/* Operating system	*/
#define  UE$FLG_M	'M'		/* Main			*/
#define  UE$FLG_A	'A'		/* Adapter		*/
#define  UE$FLG_H	'H'		/* Host			*/
#define  UE$FLG_t	't'		/* Time			*/
#define  UE$FLG_o	'o'		/* Output		*/
#define  UE$FLG_r	'r'		/* record type   	*/
#define  UE$FLG_s	's'		/* sequence num		*/
#define  UE$FLG_R	'R'		/* Reverse read of file */
#define  UE$FLG_Z	'Z'		/* Dump the file	*/
#define  UE$FLG_S	'S'		/* Summaries    	*/
#define  UE$FLG_b	'b'		/* create binary file   */

#endif _UELITERALS_

