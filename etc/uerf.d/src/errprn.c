#ifndef lint
static char sccsid[]  =  "@(#)errprn.c	4.2   (ULTRIX)   9/11/90";
#endif  lint

/*
*	.TITLE	ERRPRN - Program used to print an error
*
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
*	This program is used to print the error message given
*	a error code.  It uses file from lit ERR_MSG_FILE
*	
* ENVIRONMENT:	ULTRIX-32 C
*	
* AUTHOR:  Luis Arce,  CREATION DATE:  12-Feb-86
*
* MODIFIED BY:
*
*
*--
*/

#include "ueliterals.h"
#include <stdio.h>
#include <ctype.h>
/*
*++
*=
*=
*=MODULE FLOW - errprn.c
*=
*=  a - print_err(err_code)                         Translates errcode to test.
*=          find_file(UE$ERR_FILE)                          (uerf.c)
*=          fopen(err_file)
*=          fprint(error)
*=          return()
*=
*
*--
*/

/*
*	.SBTTL	ERRPRN - Function to print error messages.
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  This program reads the error file designated by ERR_MSG_FILE
*	   and prints out the corresponding message.
*
* FORMAL PARAMETERS:		print_err(err_code)
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		NONE
*
* COMPLETION STATUS:		NONE
*
* SIDE EFFECTS:			NONE
*
*--
*/
/*...	FUNCTION print_err(err_code)			*/

long print_err(err_code)
long  err_code;

{
FILE *fpt;
char line[UE$XFF];

extern char  search_path[];
char  *find_file();
char  err_file[UE$XFF];

printf("\n");
strcpy(err_file,find_file(UE$ERR_FILE));
if (strlen(err_file) == 0)
    {
    fprintf(stderr,"Unable to find ERROR MESSAGE FILE");
    return(UE$FAIL);
    }

if ((fpt = fopen(err_file, "r")) == NULL)
    {
    fprintf(stderr,"Unable to open ERROR MESSAGE FILE");
    return;
    }
while (fgets(line,UE$XFF,fpt) != NULL)
    {
    if (err_code == atoi(line))
	break;
    }
fprintf(stderr,"\n%s", line+6);
return;
}
/*...   ENDFUNCTION print_err()				*/

