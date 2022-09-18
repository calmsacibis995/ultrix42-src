#ifndef lint
static char sccsid[]  =  "@(#)erierr.c	4.2   (ULTRIX)   9/11/90";
#endif  lint

/*
*	.TITLE	ERIERR - EIMS module to translate error messages
*	.IDENT	/1-001/
*
* COPYRIGHT (C) 1986 DIGITAL EQUIPMENT CORP.,
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
* FACILITY:		[ EIMS Record Management System ]
*
* ABSTRACT:
*
*	This module translates EIMS error messages to their textual
*	equivalents. 
*	    Version 1.0 initializes a static array at compile
*		        time that contains all the error messages.
*	    Version 2.0 reads error messages from the standard error
*			file, err.dat
*	
* ENVIRONMENT:	VAX/VMS C,  ULTRIX-32 C
*
* AUTHOR:  Don Zaremba,  CREATION DATE:  26-Nov-85
*
* MODIFIED BY:
*
*
*--
*/

#include <stdio.h>
#include "eiliterals.h"

/*
*++
*=
*=
*=MODULE FLOW - erierr.c
*=
*=  a - ei$errtxt(fac,err)                          Translates EIMS errors.
*=          return(error_string)
*=
*
*--
*/

/*
*	.SBTTL	ERRTXT - EIMS ERR to TEXT
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This function returns a pointer to an asciiz string that
*	contains the translation of an EIMS error code.
*	
* CALLING SEQUENCE:		strptr = em$errtxt (fac,err)
*
* FORMAL PARAMETERS:		
*
*	fac			facility code
*	   [long fac]
*	err			error number
*	   [long err] 
*
* IMPLICIT OUTPUTS: Returns a pointer to an asciiz string
*
* EXTERNAL REFERENCES:
*
* FUNCTION VALUE:		string pointer
*
*	
* SIDE EFFECTS:			none
*	
*	
*--
*/
/*...   ROUTINE ei$errtxt (fac, err)  */
char *ei$errtxt (fac,err)
long fac,err;
{

static char *errary[] = {"ERIT - Error in reading file", /*1*/
	    "",
	    "ERIT - Unable to open file", /*2*/
	    "ERIT - Non-existent file",   /*3*/
	    "ERIT - Error in closing file",/*4*/
	    "","","","",
	    "ERIT - UNABLE TO RECOVER FROM ERRORS. ABORTING PROGRAM",/*9*/
	    "ERIT - Unable to complete socket connection", /*10*/
	    "ERIT - Corrupt record encountered in file, attempting recovery", /*9999*/
	    "ERIT - Corrupt record encountered in socket read", /*12*/
	    "ERIT - NOT A VALID ERROR CODE" };

    if (err == 9999)
	return(errary[11]);
    if (err == -1)
       return(errary[1]);
    if ((err < -1) || (err >= sizeof(errary)/sizeof(char *)) || (err == 0 )) 
	return(errary[(sizeof(errary)/sizeof(char *))-1]); 
    return (errary[err]);
/*				*/
/*...	ENDFUNCTION EM$ERRTXT	*/
}

#ifdef STANDALONE
main()
{
    int err;
    char *ei$errtxt();

    printf("Error # ");
    scanf("%d",&err);
    while (err) {
      printf("\n           \"%s\"\n",em$errtxt(EI$,err));
      printf("Error # ");
      scanf("%d",&err);
    }

}
#endif
