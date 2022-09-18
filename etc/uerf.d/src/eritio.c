#ifndef lint
static char sccsid[]  =  "@(#)eritio.c	4.3   (ULTRIX)   10/16/90";
#endif  lint

/*
*	.TITLE	ERITIO - I/O for Event Record Input Transformer
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
* DEC ASSUMES NO RESPONSIBILITY FOR THE USE OR RELIABILITY OF{* ITS SOFTWARE ON EQUIPMENT WHICH IS NOT SUPPLIED BY DEC.
*
*++
*
* FACILITY:		[ FMA Software Tools - Detail Design ]
*
* ABSTRACT:
*
*	This module contains the functions to open, close and read 
*	raw system events from any operating system.{*	
*	
* ENVIRONMENT:	ULTRIX-32 C
*	
* AUTHOR:  Luis Arce,  CREATION DATE:  19-Nov-85
*
* MODIFIED BY:
*		Bob Winant  12-31-85
*
*--
*/

#include <stdio.h>
#include "erms.h"
#include "eims.h"
#include "eiliterals.h"
#include "select.h"

/*
*++
*=
*=
*=MODULE FLOW - eritio.c
*=
*=  a - ei$open(file_name, mode)
*=          open_file(file_name, mode)                      (ulfile.c)
*=          return(status)
*=
*=  b - ei$close(mode)
*=          close_file                                      (ulfile.c)
*=          return(status)
*=
*=  c - ei$get(seg_ptrs...)                         Gets std segs filled.
*=          for(;;)
*=              {
*=              read_file                                   (ulfile.c)
*=              if(! EI$SUCC)
*=                  {
*=                  bld_corrupt_eis                         (eribld.c)
*=                  return(EI$CORRUPT)
*=                  }
*=              if (check_selection() == ES$SUCC)           (erisel.c)
*=                  {
*=                  zero_out(std_segs...,len)               (* d)
*=                  ei$bld(seg_ptrs...,os_rec_ptr)          (eribld.c)
*=                  return(EI$SUCC)
*=                  }
*=              if (EOF)
*=                  return(EI$EOF)
*=              }
*=
*=  d - zero_out(ptr,len)                           Area (seg) is zeroed out.
*=
*=  e - ei$ads_get(ads_ptr)                         Fills ads with record info.
*=          zero_out(std_segs...,len)                       (* d)
*=          ads_bld(ads_ptr,os_rec_ptr)                     (eribld.c)
*=          return(status)
*=
*=  f - ei_write()                                  Writes os_record to a file.
*=          write_rec(os_rec_ptr)                           (ulfile.c)
*=          return(EI$SUCC)
*=
*--
*/
/************ Processing routine ***************/

long check_selection();
char *read_file();
ei$bld();
void bld_corrupt_eis();
void zero_out();
long ads_bld();
long write_rec();

#define SEGHEADSIZ 8

extern short corrupt_flag;		/* value from ulfile.c */
extern long  rec_len;			/* value from ulfile.c */

static char *event_ptr = NULL;

/*
*	.SBTTL	EI$OPEN - Obtains access to raw error information
*++
*
*
* FUNCTIONAL DESCRIPTION:		
*
*	-  Selects user's input location (file or mailbox).
*	-  Opens file or mailbox.
*	
* FORMAL PARAMETERS:		
*
*	filename		Name of the file to be opened
*	mode			Mode to open the file (used as a flag to
*				open mailbox).
*		  
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		A raw system event file
*
* COMPLETION STATUS:		EI$OPN (file or socket open - success)
*				EI$NOP (file or socket not opened - failure)
*				EI$FNX (file doesn't exist - failure)
*
* SIDE EFFECTS:			Raw data will be accessible
*
*--
*/
/*...	ROUTINE ei$open (filename, mode)				*/
long ei$open (filename, mode)
   char *filename;
   short mode;
{
   return open_file(filename, mode);
}
/*...	ENDROUTINE EI$OPEN					*/
/*
*	.SBTTL	EI$CLOSE - Closes the error log file or socket
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  Closes the input file or socket
*	-  Does necessary clean-up
*	
* FORMAL PARAMETERS:		
*
*	mode			flag specifying whether using a mailbox
*				or not
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		NONE
*
* COMPLETION STATUS:		EI$FCL (file or socket closed - success)
*				EI$FNC (file or socket not closed)
*
* SIDE EFFECTS:			Access to raw data ended
*
*--
*/
/*...	ROUTINE EI$CLOSE (mode)				*/
long ei$close (mode)
short mode;
{
   return close_file(mode);
}
/*...	ENDROUTINE EI$CLOSE					*/
/*
*	.SBTTL	EI$GET - Gets a record from the file or socket
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  Reads a record from file or socket.
*	-  Transforms raw record to standard record
*	-  Updates File Control Block
*	-  Returns standard record or EOF
*	
* FORMAL PARAMETERS:		
*				Pointer to the context block
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		NONE
*
* COMPLETION STATUS:		EI$SUCC - std buffers filled (success)
*				EI$RDR - error encountered
*					during read.
*				EI$EOF - end-of-file encountered (success)
*				EI$CORRUPT - corrupted/invalid entry
*					     found in raw event file
*				EI$FAIL - failure
*				EI$NULL - All ptrs point to null (failure)
*
* SIDE EFFECTS:			An EIMS standard event (minus any ADS's)
*				record will be created.
*
*--
*
/*...	ROUTINE ei$get (eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr)	*/
long ei$get (eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr)
EIS *eis_ptr;
DIS *dis_ptr;
CDS *cds_ptr;
SDS *sds_ptr;
ADS *ads_ptr;
SIS *sis_ptr;
{

long status;

if (	(eis_ptr == NULL) &&
	(dis_ptr == NULL) &&
	(cds_ptr == NULL) &&
	(sds_ptr == NULL) &&
	(ads_ptr == NULL) &&
	(sis_ptr == NULL))
    return (EI$NULL);

for (;;)
    {
    event_ptr = read_file(&status);
    if (status != EI$SUCC)
	{
	if (status != EI$CORRUPT)
	    return(status);
	bld_corrupt_eis(eis_ptr);	/* build eis only           */
	return (EI$CORRUPT);
        }

    if ((status = check_selection()) == ES$SUCC)
        {				/* good rec - build segs   */
	zero_out(eis_ptr,sizeof(EIS));
	zero_out(dis_ptr,sizeof(DIS));
	zero_out(cds_ptr,sizeof(CDS));
	zero_out(sds_ptr,sizeof(SDS));
	zero_out(ads_ptr,sizeof(ADS));
	zero_out(sis_ptr,sizeof(SIS));
	status = 
          ei$bld(eis_ptr,dis_ptr,cds_ptr,sds_ptr,ads_ptr,sis_ptr, event_ptr);
	if(status == EI$FAIL) {
		continue;
	}
        return (EI$SUCC);
        }
    else if ((status == ES$EOF) || (status == EI$EOF))
	return (EI$EOF);
    }
}
/*...	ENDROUTINE EI$GET					*/

void zero_out (loc,len)
char   *loc;
short   len;
{

short i;
for (i = 0; i < len; i++)
    loc[i] = '\0';
}


/*
*	.SBTTL	EI$ADS_GET - Produces an ads from current event info
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine fills in the buffer space allocated by the caller
*	with any additional information contained in the event record
*	currently being processed.  If the raw event record being processed
*	was found to be corrupt, the adsptr is simply pointed to the actual
*	data, as there is nothing more to do.  Otherwise, the regular
*	segment build process is done.
*	
* FORMAL PARAMETERS:		
*
*	adsptr			Pointer to buffer space to load the
*				information into.
*
* IMPLICIT INPUTS:		A raw system event from which to
*				extract the information.
*
*				EIMS table definition(s) for the ADS
*				associated with the event.
*
* IMPLICIT OUTPUTS:		A complete EIMS standard event.
*
* COMPLETION STATUS:		EI$SUCC - Success always returned
*
* SIDE EFFECTS:			EIMS standard event record created.
*
*--
*/
/*...	ROUTINE EI$ADS_GET (adsptr)				*/
long ei$ads_get(adsptr)
ADS *adsptr;

{

long    i;
long    j;
long    stat;
short   subtype;
char    *adstmp;

adstmp = (char *)adsptr;
adsptr->type = ES$ADS;
if (corrupt_flag == EI$TRUE)
    {
    adsptr->length = rec_len + ES$ADSVBA + SEGHEADSIZ;
    for (j = 0, i = (ES$ADSVBA + SEGHEADSIZ); i < rec_len; ++i, ++j)
        adstmp[i] = event_ptr[j];
    return(EI$SUCC);
    }
else
    {
    subtype = adsptr->subtype;
    zero_out(adsptr,sizeof(ADS));
    adsptr->type    = ES$ADS;
    adsptr->subtype = subtype;
    adsptr->version = 1;
    stat = (ads_bld(adsptr, event_ptr));   
    }
return (stat);
}
/*...	ENDROUTINE EI$ADS_GET					*/


/*
*	.SBTTL	EI_WRITE - Writes a log entry record to a log file
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine is used to create another log file with selected
*       records. -b option.
*	
* FORMAL PARAMETERS:		
*
* IMPLICIT INPUTS:		A raw system event to write
*
* IMPLICIT OUTPUTS:		A raw system event to new log file.
*
* COMPLETION STATUS:		EI$SUCC - Success always returned
*
* SIDE EFFECTS:			NONE.
*
*--
*/
/*...	ROUTINE EI_WRITE()				*/
long ei_write()

{
extern long ads_bld();

write_rec(event_ptr);
return (EI$SUCC);
}
/*...	ENDROUTINE EI_WRITE                       */
