/*  sccsid  =  @(#)erms.h	4.3   (ULTRIX)   12/20/90  */

/* DEC/CMS REPLACEMENT HISTORY, Element ERMS.H*/
/* *3    12-FEB-1986 10:03:53 ZAREMBA "removed eiliterals.h"*/
/* *2    12-FEB-1986 09:40:06 ZAREMBA "added eiliterals.h"*/
/* *1    16-JAN-1986 11:32:16 ZAREMBA "ERMS header file"*/
/* DEC/CMS REPLACEMENT HISTORY, Element ERMS.H*/
/*
*       .TITLE ERMS.H - ERMS literal, macro and structure definitions
*	.IDENT	/1-001/
*++
*  ABSTRACT:
*	This module contains ERMS specific macros and typdefs.
*	It is used by all of the ERMS modules and by modules that
*       call ERMS functions.
*
*  AUTHOR: Don Zaremba  
*
*  TABLE OF CONTENTS
*
*   ERMS function declarations
*   ES$DECLARE macro   
*   ES$STREAM macro
*   ES$DATE typdef
*   EIS,DIS,SDS,CDS,ADS,SIS typedefs
*   CTXBLK typedef
*   ITEM_INFO typedef
*--
 */

/* Now declare this header */
#ifndef ERMS_H
#define ERMS_H

#include "eims.h"
#include "select.h"

int es$open(),es$close(), es$get(), es$getads();


/*
*	.SBTTL	ERMS_DECLARE - ERMS DECLARE
*++
* FUNCTIONAL DESCRIPTION:		
*
*   This macro declares a segment. It assigns a user supplied
*   segment name to the segment and allocates space for the segment at
*   compile time.
*	
* CALLING SEQUENCE:		ES$DECLARE(..See Below..)
*
* FORMAL PARAMETERS:		
*
*	storage_class	    storage class of the segment  must be
*			    one of: 'extern', 'static', 'auto' or
*			    blank
*			    
*	segment		    must be one of the valid segment typedefs:
*			    EIS,DIS,SDS,CDS,ADS,SIS,CIS
*
*	segment_name	    a name to be assigned to that segment
*
* SIDE EFFECTS: Allocates space in the users area for the segment
*	
*	
*--
*/
/*...  MACRO  ES$DECLARE(storage_class,segment,segment_name) */
#define ES$DECLARE(storage_class,segment,segment_name) \
storage_class segment segment_name

/*				*/
/*...	ENDMACRO ES$DECLARE	*/


/*
*	.SBTTL	ERMS_STREAM - ERMS Stream definition Macro
*++
* FUNCTIONAL DESCRIPTION:		
*
*   This macro defines an ERMS Stream and allocates space for
*   a context block.
*	
* CALLING SEQUENCE:		ES$STREAM(scope,stream_id)
*
* FORMAL PARAMETERS:		
*
*	storage_class	        storage class of the context block.
*				Must be of one the C storage classes
*				except register
*
*	Stream_id		Name of the ERMS stream. This
*				will later be used in all ERMS calls
*
*
* SIDE EFFECTS: Allocates space in the users area for a context block
*	
*	
*--
*/
/*...  MACRO  ES$STREAM(storage_class,stream_id) */
#define ES$STREAM(storage_class, stream_id) storage_class CTXBLK stream_id

/*				*/
/*...	ENDMACRO ES$STREAM	*/


/*
*	.SBTTL	SEGMENTS - ERMS segments
*++
* STRUCTURE DESCRIPTION:		
*
* STRUCTURE TYPE:		
*	
* SCOPE:	Public
*	
* SIDE EFFECTS:			NONE
*
*--
*/
/*...	STRUCTURE segments				    */
/*++							    */


typedef long ES$DATE;


#define segment_header \
	short type; \
        short subtype; \
	short version; \
	short length; \
	char validbits[4]


typedef struct eis_segment {
      segment_header ;
      short eventclass; /* See ERNS Literal Definitions */
      short eventtype; /* See ERMS Literal Definitions */
      long recordnumber;
      short ostype; /* See ERMS Literal Definitions */
      ES$DATE datetime;
      ES$DATE uptime;
      long serialnumber;
      char  *hostname;
      char  hidden[ES$EISIZE]; } EIS;

typedef struct dis_segment {
        segment_header ;
        short devclass; /* See ERMS Literal Definitions */
	short devtype; /* See ERMS Literal Definitions */
	short controller;
	short unitnumber;
	char  *serialid;
        char  *mediaid;
        long coarsesyndrome;
	char  hidden[ES$DISIZE];  } DIS ;

typedef struct sds_segment {
        segment_header ;
        char  validbitarray[ES$SDSVBA];
	char data[ES$SDSIZE];
    } SDS ;

typedef struct cds_segment {
        segment_header ;
        char  validbitarray[ES$CDSVBA];
	char data[ES$CDSIZE];
    } CDS ;

typedef struct ads_segment {
        segment_header ;
        char  validbitarray[ES$ADSVBA];
        char data[ES$ADSIZE];
    } ADS ;

typedef struct sis_segment {
        segment_header ;
        char  validbitarray[ES$SISVBA];
	char data[ES$SISIZE];
    } SIS ;

typedef struct cis_segment {
        segment_header ;
        char  validbitarray[ES$CISVBA];
	char data[ES$CISIZE];
    } CIS ;
/*--							    */


/*...	ENDSTRUCTURE segements					    */


/*
*	.SBTTL	CONTEXT_BLOCK - ERMS context block
*++
* STRUCTURE DESCRIPTION:		
*
*	The context block defines the ERMS working storage area of
*	a user program. It must be passed as the first parameter on
*	every ERMS call. ERMS expects that space for this structure 
*	has already been allocated in the callers program space.
*	
* STRUCTURE TYPE:		CTXBLK
*	
* ACCESS METHODS: This structure is a private (to ERMS) structure. An
*		  application program will not see its internal structure.
*		  It is created by the ES$STREAM macro.
*
* SCOPE:    Private
*	
* SIDE EFFECTS:			NONE
*
*--
*/
/*...	STRUCTURE CTXBLK				    */
/*++							    */
typedef struct context_blk {
    char *file_spec;
    int	 open_mode;
    int	 logical_file;
    SELNODE *selecttree;
    EIS *eis_ptr;
    DIS *dis_ptr;
    SDS *sds_ptr;
    CDS *cds_ptr;
    SIS *sis_ptr;
    CIS *cis_ptr;
    ADS *ads_ptr;
    short eisflg,disflg,sdsflg,cdsflg,sisflg,cisflg,adsflg;
    } CTXBLK;
/*--							    */

/*...	ENDSTRUCTURE CTXBLK					    */

typedef struct
    {
    short       id;
    char        type;
    short       size;
    short       seq_num;
    char        seg_type;
    char        *item_ptr;
    } ITEM_INFO;    

#endif ERMS_H
