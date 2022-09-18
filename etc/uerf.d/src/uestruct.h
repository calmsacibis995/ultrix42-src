/*  sccsid  =  @(#)uestruct.h	4.1   (ULTRIX)   7/2/90  */

/*
*	.TITLE	UESTRUCT - Structure definitions for UERF
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
*	This header file contains the structures used by UERF
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

#ifndef _UESTRUCT_
#define _UESTRUCT_

/************************  ERROR CODES STRUCTURE  ***************/

struct err_cd
    {
    short code;
    char  *errmsg;
    };

/***********************  INPUT STRUCTURE  **********************/

struct in_struc
    {
    struct parse_tree *root;
    struct parse_tree *last;
    short  out_form;			/* brief or full output	*/
    short  out_file;			/* output bin file flag */
    short  kernel;			/* true if kernel buf read */
    short  dump_rec;			/* flag to dump rec        */
    short  summary;			/* flag for summaries	*/
    };

/*******************  PARSE TREE STRUCTURE  *********************/

struct parse_tree
    {
    char   flag[1];			/* flag value		*/
    struct parse_tree *next_tree;	/* next tree entry	*/
    struct parm_struc *parm_list;	/* pointer to parms	*/
    };

/*******************  PARAMETER CHAIN STRUCTURE  ****************/

struct parm_struc
    {
    short  kind;			/* kind of chain	*/
    struct parm_struc *next_parm;	/* next parameter	*/
    char   *parm_string;		/* parameter string	*/
    };

#endif _UESTRUCT_

