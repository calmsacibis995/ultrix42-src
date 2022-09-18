#ifndef lint
static char sccsid[]  =  "@(#)makerr.c	4.1   (ULTRIX)   7/2/90";
#endif  lint

/*
*	.TITLE	MAKERR - Program used to make an error file
*			 from an .h file.
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
*	This program is used to create an error file
*	from a .h file.
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
#include "uerror.h"
#include "uestruct.h"
#include <stdio.h>
#include <ctype.h>
#include <math.h>



/*
*	.SBTTL	MAKERR - Program to make an error file.
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  This program reads an .h file and creates an error file
*	   that is later used by other functions to print error
*	   messages.
*
* FORMAL PARAMETERS:		Name of input .h file.
*
* IMPLICIT INPUTS:		Output file name is the logical
*				UE$ERR_FILE
*
* IMPLICIT OUTPUTS:		Output file
*
* COMPLETION STATUS:		NONE
*
* SIDE EFFECTS:			NONE
*
*--
*/
/*...	FUNCTION MAKERR(argc,infile)			*/

main(argc,infile)
short argc;
char  *infile[];

{
struct err_cd err;
FILE *ifpt;
FILE *ofpt;
char line[UE$XFF];
char *lptr;
short code;

if (argc <= 1)
    {
    printf("Input .h file not given.\n");
    exit();
    }
if ((ifpt = fopen(infile[1], "r")) == NULL)
    printf("Unable to open input file - %s.\n",infile[1]);
else
    {
    strcpy(line,UE$ERR_FILE);
    if ((ofpt = fopen(line, "w")) == NULL)
	printf("Unable to open output file - %s.\n",line);
    else
	{
	printf("Output file = [%s]\n",line);
	while ((lptr = fgets(line,UE$XFF,ifpt)) != NULL)
	    {
	    if (strncmp(lptr = (char *)strchr(lptr,'#'),"#define",7) != NULL)
		continue;
	    if (strncmp(lptr = (char *)strchr(lptr,'$'),"$ERR_",5) != NULL)
		continue;
	    while (!isspace(*lptr++))
		;
	    code = atoi(lptr);
	    if (strncmp(lptr = (char *)strchr(lptr,'/'),"/*",2) != NULL)
		continue;
	    lptr += 2;
fprintf(ofpt,"%4d %.*s\n",code, strcspn(lptr,"*"), lptr);
	    }
	}
    }
}
/*...   ENDFUNCTION MAKERR()			*/

