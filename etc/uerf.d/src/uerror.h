/*  sccsid  =  @(#)uerror.h	4.1   (ULTRIX)   7/2/90  */

/*
*	.TITLE	UERROR.H - Literal definitions for UERF errors.
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
*	This header file contains the literals used for all errors in
*	UERF.  The program mk_err_fil will create a error file from
*	this .h file which is used by UERF to print out any errors.
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

#ifndef _UERRORS_
#define _UERRORS_


/****************************  ERROR  CODES  ****************************/
/*
 *  Restrictions for this .h file so that makerr can process it.
 *
 *  #define has to start in collumn 1
 *  Logical name must contain $ERR_
 *  value must be numeric
 *  Comments will be used as the error message.
 *
 */

/************* reserve  100 - 199  UERF     ************/
#define  UE$ERR_NOBIN   101	/* Unable to find binary file.		*/
#define  UE$ERR_BADBIN  102	/* Unable to load binary file.		*/
#define  UE$ERR_NOCONF  103     /* Unable to open system config file.	*/
#define  UE$ERR_CONFIG  104     /* Error reading system config file.	*/
#define  UE$ERR_NOHLP   105	/* Unable to find UERF help file.       */

/************* reserve  200 - 299  GETCMD   ************/
#define	 UE$ERR_INVFLG	201	/* Invalid option flag.			*/
#define  UE$ERR_DUPFLG	202	/* Duplicate option flag.		*/
#define  UE$ERR_INVPAR	203	/* Invalid parameter.			*/
#define  UE$ERR_INCOMP	204	/* Incompatible flag combination.	*/
#define  UE$ERR_INCPAR	205	/* Incompatible parameter combination.	*/
#define  UE$ERR_DEVTBL  206	/* Error searching device table.	*/
#define  UE$ERR_H_OPEN  216	/* Error opening help file.		*/

/************* reserve  300 - 399  ERITIO   ************/
#define  UE$ERR_S_OPEN	301	/* Error opening syserr file.		*/
#define  UE$ERR_K_OPEN	302	/* Error opening socket.		*/
#define  UE$ERR_S_READ  303	/* Error reading syserr file.		*/

/************* reserve  400 - 499  BTT      ************/
#define  UE$ERR_DECODE  401	/* Unable to decode field.	*/

/************* reserve  500 - 599  MSGWRT   ************/

/************* reserve  600 - 699  XFORM    ************/

#define  UE$ERR_UNDEFN    0     /* Invalid error code # */

#endif _UERRORS_
