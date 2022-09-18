#ifndef lint
static char sccsid[]  =  "@(#)esget.c	4.2   (ULTRIX)   9/11/90";
#endif  lint

/* DEC/CMS REPLACEMENT HISTORY, Element ESGET.C*/
/* *6     6-Mar-1987          ARCE    "added ads to ei$get parms */
/* *5    11-MAR-1986 12:11:20 ZAREMBA "added call to selection"*/
/* *4    12-FEB-1986 13:01:25 ZAREMBA "new version of erit"*/
/* *3    20-JAN-1986 16:44:58 ZAREMBA "remove _L from literals"*/
/* *2    17-JAN-1986 17:11:00 ZAREMBA """added calls to erit"*/
/* *1    16-JAN-1986 11:32:41 ZAREMBA "ERMS get function"*/
/* DEC/CMS REPLACEMENT HISTORY, Element ESGET.C*/
/*
*	.TITLE	ESGET - ERMS get function 
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
* FACILITY:		[ ERMS - EIMS Record Management System ]
*
* ABSTRACT:
*
*	This module is the ERMS call interface to the GET function.
*	It can be called directly from a C program or as the
*	call from the pre-prosessor. The only entry point to this
*	module is the ERMS_GET function.
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
#include "erms.h"
#include "eiliterals.h"

/*
*++
*=
*=
*=MODULE FLOW - esget.c
*=
*=  a - es$get(ctx,option)                          Updates ctx with record
*=                                                  that matches selection
*=                                                  criteria
*=          while(!EOF && !accepted)
*=              {
*=              ei$get(seg_ptrs...)                         (eritio.c)
*=              es$eval(select_tree)                        (select.c)
*=              }
*=          return(ES$SUCC)
*=
*=  b - es$getads(ctx,ads_ptr)                      Updates ctx with next ads
*=          ei$ads_get(ads_ptr)                             (eritio.c)
*=          return(ES$SUCC | ES$NOADS | ES$FAIL)
*=
*
*--
*/


/*
*	.SBTTL	ERMS_GET - ERMS GET function
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This function finds and returns a logical record in the
*	current stream. Only logical records that match the selection
*	criteria are returned.
*	
* CALLING SEQUENCE:		status = es$get (..See Below..)
*
* FORMAL PARAMETERS:		
*
*	stream_id		stream identifier
*	    [CTXBLK *stream_id]
*
*	option			must be one of the following:
*	    [int option]	    ES$NEXT -- (default) get the next 
*					record in the stream
*				    ES$FIRST -- get the first record in the
*				        stream, without changing record 
*					pointer.
*				    ES$RESTART -- reset pointer and get
*					first record in stream
*
*
*
* IMPLICIT OUTPUTS: Data in the segment buffers is updated at the
*		    completion of a successful read.
*
* EXTERNAL REFERNENCES: 
*		ERIT_GET
*
* FUNCTION VALUE:		integer value
*
*	ES$SUCC - GET completed successful
*	ES$STRM - undeclared stream id
*	ES$EOF - END-OF-FILE 
*	ES$FAIL - failed for unknown reason
*	
* SIDE EFFECTS:			
*	If a logical record is found then segment buffers are filled
*	with the data from the file. The pointer to the next record is
*	updated, except when the FIRST$ option is requested.
*       If no record is found, for whatever reason, then the content
*	of the segment buffers is undefined, it may or may not have been
*	modified. Record pointer is not updated.
*	
*	
*	
*--
*/
/*DEFFUN*/
/*...   FUNCTION es$get(stream_id,option) */
es$get (str,option)
CTXBLK *str;
int option;
{
    static long answer;
    static long accepted;


    if (str->selecttree != NULL)  /* Test for selection */
      {
	answer = EI$TRUE;
	accepted = ES$FAIL;
        while ((answer == EI$TRUE) && (accepted == ES$FAIL))
          {
	    answer = ei$get(str->eis_ptr,
			    str->dis_ptr,
			    str->cds_ptr,
			    str->sds_ptr,
			    str->ads_ptr,
			    str->sis_ptr);
	    accepted = es$eval(str->selecttree); /* check this record */
	  }
      }
    else
	answer = ei$get(    str->eis_ptr,
			    str->dis_ptr,
			    str->cds_ptr,
			    str->sds_ptr,
			    str->ads_ptr,
			    str->sis_ptr);
		     /* no selection */

    if (answer == EI$TRUE) answer = ES$SUCC;
    else if (answer == EI$EOF) answer = ES$EOF;
    else answer = ES$FAIL;
    return(answer);
/*				*/
/*...	ENDFUNCTION ERMS_GET	*/
}



/*
*	.SBTTL	ERMS_GETADS - ERMS GET next ADS function
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This function returns the next ADS segement. The proper proceedure
*	for reading all ADS segments is defined below.
*
*  while (es$get (..) != ES$EOF) { 
*	... code to process non_ADS segments ...
*      while (es$getads(..) != ES$NOADS) {
*	    ... code to process each ADS segment ... 
*	    }
*	}
*
*
*	
* CALLING SEQUENCE:		status = es$getads (..See Below..)
*
* FORMAL PARAMETERS:		
*
*	stream_id		stream identifier
*	    [CTXBLK *stream_id]
*
*	ads_location		pointer to ADS buffer (Optional)
*			        If not present uses those passed with
*				the ERMS OPEN function.
*	    [ADS *ads_location]
*
* IMPLICIT INPUTS: Pointers to user ads buffer
*
* IMPLICIT OUTPUTS: Data in the ads segment buffers is updated at the
*		    completion of a successful read.
*
* EXTERNAL REFERNENCES: 
*
* FUNCTION VALUE:		integer value
*
*	ES$SUCC - GET completed successful
*	ES$STRM - undeclared stream id
*	ES$NOADS - no more ADS segements
*	ES$FAIL - failed for unknown reason
*	
* SIDE EFFECTS:			
*	If a logical record is found then segment buffers are filled
*	with the data from the file. The pointer to the next ads record 
*	is updated.
*       If no record is found, for whatever reason, then the content
*	of the segment buffers is undefined, it may or may not have been
*	modified. Record pointer is not updated.
*	
*	
*	
*--
*/
/*...   FUNCTION es$getads (stream_id,ads_location) */
es$getads (str,ads)
CTXBLK *str;
ADS *ads;
{
    long answer;
   
    answer = ei$ads_get(((ads) ? ads : str->ads_ptr )); 
    if (answer == EI$TRUE) return(ES$SUCC);
    if (answer == EI$FAIL) return(ES$NOADS);
    return(ES$FAIL);

/*				*/
/*...	ENDFUNCTION ERMS_getads	*/
}
