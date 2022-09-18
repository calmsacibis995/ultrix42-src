#ifndef lint
static char sccsid[]  =  "@(#)ulfile.c	4.2   (ULTRIX)   9/11/90";
#endif  lint

/*
*	.TITLE	ULFILE - Raw ULTRIX event file handler
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
* FACILITY:		[ FMA Software Tools - Detail Design ]
*
* ABSTRACT:
*
*	This module contains the functions to 
open, close and read 
*	either the error log file or a socket on an ULTRIX system.
*	
*	
* ENVIRONMENT:	ULTRIX-32 C
*	
* AUTHOR:  Bob Winant,  CREATION DATE:  30-Jan-1986
*
* MODIFIED BY:
*
*--
*/
#include "eiliterals.h"		/* Erit specific literals */
#include "eims.h"
#include "erms.h"
#include "select.h"
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/errlog.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <elwindow.h>

/*
*++
*=
*=
*=MODULE FLOW - ulfile.c
*=
*=  a - get_buff_addr()                             Gets address of buffer
*=                                                  of raw rec
*=          return(rec_buff)
*=
*=  b - open_file(filename, mode)                   Opens file or socket
*=          if(EI$MAILBOX)
*=              {
*=              bld_socket()                                (* j)
*=              return(EI$OPN)
*=              }
*=          if(EI$APPEND)
*=              {
*=              open(out_file,WRONLY|CREAT|APPEND,0644)
*=              return(EI$OPN)
*=              }
*=          open(in_file,RDONLY,0)
*=          return(EI$OPN)
*=
*=  c - close_file(mode)                            Closes file or socket.
*=          if(socket_flag)
*=              close_wndw(in_file,params)
*=          else
*=              close(in_file)
*=          return(status)
*=
*=  d - read_file(status)                           Reads next raw record
*=                                                  into buffer.
*=          if(socket_flag)
*=              soc_read(status)                            (* k)
*=          else
*=              {
*=              if(new_file)
*=                  init_file()                             (* i)
*=              get_next_rec(status)                        (* e)
*=          validate_entry(elrp)                            (* g)
*=          return(elrp)
*=
*=  e - get_next_rec(status)                        Gets the next record.
*=          if(! reverse_flag)
*=              {
*=              lseek(in_file,offset,L_SET)
*=              read(in_file,rec_buff,EL_MAXRECSIZE)
*=              }
*=          if(reverse_flag)
*=              {
*=              lseek(in_file,offset-EL_MAXRECSIZE,L_SET)
*=              read(in_file,rec_buff,EL_MAXRECSIZE)
*=              "back up checking for trailer"
*=              }
*=          return(rec_buff)
*=
*=  f - write_rec(elrp)                             Writes log record.
*=          write(out_file,elrp,reclen)
*=          return(EI$SUCC)
*=
*=  g - validate_entry(elrp)                        Validates record trailer.
*=          "check for valid trailer"
*=          to_new_ver()                                    (* h)
*=          "check valid flag"
*=          return(EI$CORRUPT | EI$SUCC)
*=
*=  h - to_new_ver()                                Changes elrp format from
*=                                                  old versions to latest.
*=
*=  i - init_file()                                 Initializes input file
*=                                                  for 1st read.
*=          if(reverse_flag)
*=              "setup for reverse"
*=          while(between start time and end time)
*=              {
*=              lseek(in_file,offset,L_SET)
*=              read(in_file,rec_buff,EL_MAXRECSIZE)
*=              validate_entry(elrp)                        (* g)
*=              es$eval(end_date_tree)                      (select.c)
*=              es$eval(start_date_tree)                    (select.c)
*=              }
*=          return()
*=
*=  j - bld_socket()                                Opens errlog socket.
*=          open_wndw(opts,params)
*=          return(socket_id)
*=
*=  k - soc_read()                                  Reads socket and places
*=                                                  raw record in buffer
*=          read_wndw(in_file,rec_buff,EL_MAXRECSIZE)
*=          return(rec_buff)
*=
*
*--
*/

extern SELNODE *startree;
extern SELNODE *endtree;

extern char *ei$errtxt();    		/* error messages rtn */

/****************** Declare module wide globals ******************/

long in_file         = 0;		/* Input file descriptor */
long out_file        = 0;		/* Output file descriptor */
char rec_buff[EL_MAXRECSIZE];  		/* location to put record */
char   *trailer_id;			/* Pointer to check for valid trailer */

long   file_offset       =  0;
long   total_bytes       =  0;
long   rec_len           =  0;
struct el_rec *elrp;			/* to directly access info in event */


short socket_flag   = EI$FALSE;
short reverse_flag  = EI$FALSE;
short corrupt_flag  = EI$FALSE;
short newfile_flag  = EI$TRUE;

struct options opts;		/* Options to be read for socket conn */
struct s_params params;		/* Parameters for socket connection */
struct sockaddr_un soc_from;  	/* Socket structure that info comes from */
long   socfrmlen;		/* Length of info returned from a socket read */

/***************************  FUNCTIONS  ***************************/

long  init_file();
long  validate_entry();
void  to_new_ver();
long  bld_socket();
struct el_rec  *soc_read();
struct el_rec  *get_next_rec();

/**********************  GET_BUFF_ADDR  ****************************/

struct el_rec *get_buff_addr()

{
return ((struct el_rec *)rec_buff);
}

/******************************************************************/




/*
*	.SBTTL	OPEN_FILE - Obtains access to raw error information
*++
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
/*...	ROUTINE OPEN_FILE (filename, mode)				*/
long open_file (filename, mode)

char *filename;
short mode;

{

if (mode == EI$MAILBOX)
    {
    socket_flag = EI$TRUE;
    if ((in_file = bld_socket()) != EI$NSKT)
        return (EI$OPN);
    else
        {
	printf("\n%s\n", ei$errtxt(EI$, EI$NSKT));
	return (EI$FNX);
        }
    }
else
    {
    if (mode == EI$REVERSE)
        reverse_flag = EI$TRUE;
    if (strcmp(filename, "") != EI$FALSE)
        {
        if (mode == EI$APPEND)
            {
            if ((out_file = open(filename, O_WRONLY | O_CREAT | O_APPEND,
                                         0644)) >= 0)
                return (EI$OPN);
            else
                {
                printf("\n%s: %s\n", ei$errtxt(EI$,EI$NOP), filename);
                return (EI$NOP);
                }
            }
        if ((in_file = open(filename, O_RDONLY, 0)) >= 0)
            return (EI$OPN);
        else
            {
	    printf("\n%s: %s\n", ei$errtxt(EI$,EI$NOP), filename);
            return (EI$NOP);
            }
        }
    else
        {
	printf ("\n%s: %s\n", ei$errtxt(EI$,EI$FNX), filename);
	return (EI$FNX);
        }
    }
}
/*...	ENDROUTINE OPEN_FILE					*/

/*
*	.SBTTL	CLOSE_FILE - Closes the error log file or socket
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  Closes the input file or socket
*	-  Does necessary clean-up
*	
* FORMAL PARAMETERS:		
*
*	socket_flag		flag to specify socket
*
* IMPLICIT INPUTS:		Input file or socket in fcb_struc
*
* IMPLICIT OUTPUTS:		NONE
*
* COMPLETION STATUS:		EI$FCL (file or socket closed - success)
*				EI$FNC (file or socket not closed)
*
* SIDE EFFECTS:			File or socket will be closed and
*				become unavailable for processing.
*
*--
*/
/*...	ROUTINE CLOSE_FILE (mode)					*/
long close_file(mode)

long mode;

{
long status;

if (socket_flag)
    {
    status = close_wndw(in_file, &params);
    if (status != -1)
	status = 0;
    }
else
    status = close(in_file);

if (!status)
    return (EI$FCL);
else
    {
    printf("\n%s\n", ei$errtxt(EI$,EI$FNC));
    return (EI$FNC);
    }
}
/*...	ENDROUTINE CLOSE_FILE					*/


/*
*	.SBTTL	READ_FILE - reads an entry from the raw event file
*++
* FUNCTIONAL DESCRIPTION:		
*
*	Reads the specified input file or socket one record at a time
*	
* FORMAL PARAMETERS:		elrp - A pointer to be assigned to the
*					    read-in buffer.
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		A raw system event record
*
* COMPLETION STATUS:		EI$SUCC - Everything's cool
*				EI$RDR - error encountered
*					during read.
*				EI$EOF - End of file encountered
*				EI$FAIL - File somehow inaccessible
*				EI$CORRUPT - corrupt record found in
*					     raw event file.
*				
*
* SIDE EFFECTS:			NONE
*
*--
*/
/*...	ROUTINE READ_FILE ()				*/

struct el_rec *read_file(status)
long *status;

{
short i;

corrupt_flag = EI$FALSE;
		
if (socket_flag == EI$TRUE)		/* check for socket read  */
    elrp = soc_read(status);
else
    {
    if (newfile_flag == EI$TRUE)
	{
	init_file();
        rec_len = 0;			/* initialize rec len	*/
	}
    elrp = get_next_rec(status);
    }
    
if (*status != EI$SUCC)
    return (NULL);

if ((*status = validate_entry(elrp)) == EI$FAIL)
    return (NULL);

for (i = elrp->elrhdr.rhdr_reclen; i < EL_MAXRECSIZE; i++)
    rec_buff[i] = '\0';		/* zero out remainder of buffer */

if (*status == EI$CORRUPT)
    corrupt_flag = EI$TRUE;

return (elrp);
}

/*...	END ROUTINE READ_FILE()				*/



/*
*	.SBTTL	GET_NEXT_REC   reads entry from the raw event file
*++
* FUNCTIONAL DESCRIPTION:		
*
*	Reads the specified input file for next record.
*	
* FORMAL PARAMETERS:
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		A raw system event record
*
* COMPLETION STATUS:		EI$SUCC - Everything's cool
*				EI$RDR - error encountered
*					during read.
*				EI$EOF - End of file encountered
*				EI$FAIL - File somehow inaccessible
*				EI$CORRUPT - corrupt record found in
*					     raw event file.
*				
*
* SIDE EFFECTS:			NONE
*
*--
*/
/*...	ROUTINE GET_NEXT_REC()				*/

struct el_rec *get_next_rec(status)
long *status;

{
short i, j;
long len;

if (reverse_flag != EI$TRUE)

/*******************  FORWARD READ OF FILE ******************/

    {
    file_offset += rec_len;
    lseek(in_file,file_offset,L_SET);
    if (read(in_file,rec_buff,EL_MAXRECSIZE) == 0)
	{
	*status = EI$EOF;
	return(NULL);
	}
    }
else
    
/***********************  REVERSE READ OF FILE ******************/
    
    {
    if (file_offset <= 0)
	{
	*status = EI$EOF;
	return(NULL);
	}
    if (file_offset >= EL_MAXRECSIZE)
	{
	len          = EL_MAXRECSIZE;
	file_offset -= EL_MAXRECSIZE;
	}
    else
	{
	len         = file_offset;
	file_offset = 0;
	}
    lseek(in_file, file_offset, L_SET);
    if (read(in_file, rec_buff, len) != len)
	{
	*status = EI$RDR;
	return(NULL);
	}
    if (strncmp(rec_buff+(len - EL_TRAILERSIZE),trailer,EL_TRAILERSIZE) != 0)
	{
	*status = EI$RDR;
	return(NULL);
	}
    for(i = len - (EL_TRAILERSIZE + 1); i > 0; i--)
	{
        if (strncmp(rec_buff+i,trailer,EL_TRAILERSIZE) == 0)
	    break;
	}
    if (i == 0)
	{
	if (file_offset != 0)	/* cannot find start of rec */
	    {
	    *status = EI$FAIL;
	    return(NULL);
	    }
	}
    else
	{
	i += EL_TRAILERSIZE;
	len -= i;			/* calculated rec len	*/
	for (j = 0; j < len; j++)
	    {
	    rec_buff[j] = rec_buff[i+j];
	    }
	}
    file_offset += i;
    }
*status = EI$SUCC;
return ((struct el_rec *)rec_buff);
}

/*... END ROUTINE GET_NEXT_REC 				*/




/*
*	.SBTTL	WRITE_REC - Writes raw record to new log file
*++
* FUNCTIONAL DESCRIPTION:		
*
*	Writes raw record from given buffer to new output log file
*	
* FORMAL PARAMETERS:		elrp - A pointer to buffer
*                                      (raw sytem event record). 
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		A raw system event record
*
* COMPLETION STATUS:		EI$SUCC - Everything's cool
*
* SIDE EFFECTS:			NONE
*
*--
*/
/*...	ROUTINE WRITE_REC(elrp)				*/

long write_rec(elrp)
struct el_rec *elrp;
{
write(out_file,elrp,elrp->elrhdr.rhdr_reclen);
return (EI$SUCC);
}

/*...	END ROUTINE PUT_REC()				*/

/*
*	.SBTTL	VALIDATE_ENTRY - Validates the current raw record
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine checks the expected trailer on the end of the
*	raw record to see if a valid entry exists.  It also checks the
*	validation location for the record to see if that is also valid.
*	
* FORMAL PARAMETERS:		
*
*	elrp		A pointer to current raw ULTRIX event.
*
* IMPLICIT INPUTS:		A raw event record
*
* IMPLICIT OUTPUTS:		NONE
*
* COMPLETION STATUS:		EI$TRUE (good record)
*				EI$CORRUPT (record has valid trailer
*					but invalid value in rhdr_valid)
*				EI$FAIL (invalid record, and unable
*					to resync)
*
* SIDE EFFECTS:			Return value used by calling
*				routine to decide what to do next
*
*--
*/
/*...	ROUTINE VALIDATE_ENTRY(elrp)				*/

long  validate_entry(elrp)

struct el_rec *elrp;

{
long  i;				/* loop ctr */

/**************** First check for a valid trailer *****************/

rec_len = elrp->elrhdr.rhdr_reclen;   /*save len for next read */

if (strncmp(((char *)elrp)+(rec_len-EL_TRAILERSIZE),trailer,EL_TRAILERSIZE) != 0)
    {					/* no trailer found */
    for (i = 0; i < EL_MAXRECSIZE; ++i)	/* search for next */
        {
        if (strncmp(rec_buff+i,trailer,EL_TRAILERSIZE) == 0)
            {
            rec_len = i+EL_TRAILERSIZE;
	    return (EI$CORRUPT);	/* found another trailer */
	    break;
            }
        }
    return (EI$FAIL);			/* cannot resync */
    }

to_new_ver();

/************************ now check for valid flag ***************/

if (elrp->elrhdr.rhdr_valid != EL_VALID)
    return (EI$CORRUPT);		/* no valid flag */

return(EI$SUCC);
}
/*...	ENDROUTINE VALIDATE_ENTRY				*/



/*
*	.SBTTL	TO_NEW_VER - Changes the errlog file to a new version
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine is used to change an errlog file from an old version
*	to that of the latest version.
*	
* FORMAL PARAMETERS:		
*
*
* IMPLICIT INPUTS:		A raw event record
*
* IMPLICIT OUTPUTS:		NONE
*
* COMPLETION STATUS:		NONE
*
* SIDE EFFECTS:			The errlog entry will now look like
*				the latest errlog.h
*
*--
*/
/*...	ROUTINE TO_NEW_VER()					*/

void to_new_ver()
{

long  i;				/* loop ctr */
long  new_rhdr_len;
long  new_body_len;
long  old_rhdr_len;
long  old_body_len;
char  temp_buff[EL_MAXRECSIZE];

/********************** DEFINE OLD HEADERS ************************/

struct v1_el_rhdr {			/* errlog header for v1 */
	u_short rhdr_reclen;		/* errlog record length */
	u_short rhdr_seqnum; 		/* seq. number */
	u_long rhdr_time;		/* time in sec */
	u_long rhdr_sid;		/* system id, filled in by elcs */
	u_char rhdr_valid;		/* valid error record */
	u_char rhdr_pri;		/* priority hi - low */
	u_char rhdr_elver[2];		/* errlog version,filled in by elcs */
	char rhdr_hname[EL_SIZE12];	/* host name, filled in by elcs */
};

if (elrp->elrhdr.rhdr_elver == 1)		/* ver 1 entry */

/************************ change ver 1 to ver 2 *************************/
    {
    old_rhdr_len  = sizeof(struct v1_el_rhdr);
    old_body_len  = rec_len - old_rhdr_len;

    new_rhdr_len  = sizeof(struct el_rhdr);
    new_body_len  = old_body_len;

    for (i = 0; i < rec_len; i++) 	/* copy rec to temp */
	temp_buff[i] = rec_buff[i];

    for (i = 0; i < (new_rhdr_len - old_rhdr_len); i++)
        rec_buff[old_rhdr_len + i] = '\0';	/* zero out new area */

					/* move body */
    for (i = 0; i < old_body_len; i++)
        rec_buff[new_rhdr_len + i] = temp_buff[old_rhdr_len + i];

    elrp->elrhdr.rhdr_reclen = new_rhdr_len + new_body_len;
    elrp->elrhdr.rhdr_elver  = 2;

    }
/******************** end change ver 1 to ver 2 *************************/


}
/*...	TO_NEW_VER				*/



/*
*	.SBTTL	INIT_FILE       Initialize pointers for read by date.
*++
* FUNCTIONAL DESCRIPTION:		
*
*	Returns the file_offset to the first record (forward or rev)
*	
* FORMAL PARAMETERS:		NONE
*
* IMPLICIT INPUTS:		Module wide declarations for rec_buff,
*				flags, and other integers necessary
*				for keeping track of the possition in
*				the file and direction.
*
* IMPLICIT OUTPUTS:		file-offset
*
* COMPLETION STATUS:		NONE
*
* SIDE EFFECTS:			NONE
*
*--
*/

/*...  ROUTINE  INIT_FILE()				*/

long  init_file()

{

short  i;
long   lo;
long   hi;
long   offset;

newfile_flag = EI$FALSE;

lo = 0;
hi = lseek(in_file, 0, L_XTND);		/* get file size	*/

if (reverse_flag == EI$TRUE)
    {
    file_offset = hi;
    if (endtree == NULL)
	return;
    }
else
    {
    file_offset = lo;
    if (startree == NULL)
	return;
    }

/*****************  DO BINARY SEARCH TO START  ******************/

while ( ((hi - lo)/2) > EL_MAXRECSIZE)
    {
    offset = lo + ((hi - lo)/2);
    lseek(in_file, offset, L_SET);	/* move ptr to mid range	*/
    if (read(in_file, rec_buff, EL_MAXRECSIZE) != EL_MAXRECSIZE)
        return;			/* cannot split any smaller     */

    for (i = 0; i < EL_MAXRECSIZE; i++, offset++)
        {
        if (strncmp(rec_buff+i,trailer,EL_TRAILERSIZE) == 0)
	    break;
        }

    if (i == EL_MAXRECSIZE)
        return;			/* cannot split any smaller     */
    i      += EL_TRAILERSIZE;
    offset += EL_TRAILERSIZE;

    lseek(in_file, offset, L_SET);	/* move ptr to rec start */
    if (read(in_file, rec_buff, EL_MAXRECSIZE) != EL_MAXRECSIZE)
        return;			/* cannot split any smaller     */

    elrp = (struct el_rec *)rec_buff;

    if (validate_entry(elrp) != EI$SUCC)
        return;			/* cannot split any smaller     */

    if (reverse_flag == EI$TRUE)
        {
        if (es$eval(endtree) == ES$SUCC)
            {
	    lo = offset;
	    }
        else
	    {
	    hi = offset;
	    file_offset = offset;
	    }
        }
    else
        {
        if (es$eval(startree) == ES$SUCC)
            {
	    hi = offset;
	    }
        else
	    {
	    lo = offset;
	    file_offset = offset;
	    }
        }
    }

/*****************  FINISHED BINARY SEARCH  *********************/

return;
}

/*...  END ROUTINE  INIT_FILE()			*/



/*
*	.SBTTL	BLD_SOCKET - Creates socket path to OS
*++
* FUNCTIONAL DESCRIPTION:		
*
*	Opens a path to the OS buffer for a constant read and
*	translation on wakeup
*	
* FORMAL PARAMETERS:		NONE
*
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		NONE
*
* COMPLETION STATUS:		sock_id - socket descriptor
*				EI$NSKT - failed to open socket
*
* SIDE EFFECTS:			Path to OS opened, opens processing
*				directly from os, continues until user
*				stops it.
*
*--
*/
/*...	ROUTINE BLD_SOCKET ()				*/
long bld_socket()

{
long sock_id;		/* Receives return value from socket open */

/******** Set up options to ALL * (defined in window.h by UEG) ****/
 
opts.class	= ALL;
opts.type	= ALL;
opts.ctldevtyp	= ALL;
opts.num	= ALL;
opts.unitnum	= ALL;

sock_id = open_wndw(&opts, &params);		/* Open socket connection */
if (sock_id != EI$RDR)
    return (sock_id);
else
    return (EI$NSKT);
}
/*...	ENDROUTINE BLD_SOCKET				*/


/*
*	.SBTTL	SOC_READ - reads an entry from a socket
*++
* FUNCTIONAL DESCRIPTION:		
*
*	Reads the specified socket one record at a time
*	
* FORMAL PARAMETERS:		NONE
*
*
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		A raw system event record
*
* COMPLETION STATUS:		EI$TRUE - Everything's cool
*				EI$RDR - error encountered
*					during read.
*				EI$CORRUPT - corrupt record found
*				
*
* SIDE EFFECTS:			Process goes into a wait state
*				until a response is received from
*				the called socket.
*
*--
*/
/*...	ROUTINE SOC_READ ()					*/
struct el_rec  *soc_read(status)
long *status;

{
long   bytes_read;
int    done = 0;

/********** Get a record from the socket connector ***************/

while (!done)
    {
    bytes_read = read_wndw(in_file, rec_buff, EL_MAXRECSIZE);

    if (bytes_read == 0)		/* 0 => exit; */
	{
	*status = EI$FAIL;
        return(NULL);
	}
    if (bytes_read == EI$RDR)
        {
        if (errno != EINTR)
	    {
	    *status = EI$FAIL;
            return (NULL);		/* return if not */
	    }
        else 				/* otherwise, try again */
            {
	    errno = 0;
	    continue;
	    }
        }
    done = 1;
    }
*status = EI$SUCC;
return((struct el_rec *)rec_buff);
}  
/*...	ENDROUTINE SOC_READ					*/

