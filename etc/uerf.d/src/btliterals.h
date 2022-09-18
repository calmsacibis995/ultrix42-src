/*  sccsid  =  @(#)btliterals.h	4.1   (ULTRIX)   7/2/90  */

/*
*	.TITLE	BTLITERALS - Literal definitions for BTT 
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
*	This header file contains the literals used by BTT
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

#ifndef _BTLITERALS_
#define _BTLITERALS_

/************************  LITERALS     ************************/

#define		BT$SUCC		 1	/* Successful 		*/
#define		BT$NEW_SEG	 1	/* New segment flag	*/
#define		BT$SPACE	" "	/* space for printf	*/
#define		BT$INV_ITEM	"Undefined"
#define		BT$NOT_AVAIL	"Unavailable"

#define		BT$F1_LEN	20	/* len of 1st out field	*/
#define		BT$F2_LEN	15	/* len of 2nd out field	*/
#define		BT$F3_LEN	 5	/* len of 3rd out field	*/
#define		BT$F4_LEN	38	/* len of 4th out field	*/
#define		BT$F4A_LEN	20	/* part of 4th field	*/
#define		BT$F4B_LEN	 2	/* Spaces for part of 4	*/
#define		BT$F4C_LEN	16	/* part of 4th field	*/

#endif _BTLITERALS_
