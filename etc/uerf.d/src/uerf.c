#ifndef lint
static char sccsid[]  =  "@(#)uerf.c	4.3   (ULTRIX)   9/11/90";
#endif  lint

/*
*	.TITLE	UERF - ULTRIX Error Report Formatter (UERF)
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
*	UERF is the ULTRIX Error Report Formatter. (ULTRIX V2.0 +)
*	The process translates binary events in the
*       error log into a human readable report.
*	into a human readable format.
*	
* ENVIRONMENT:	ULTRIX-32 C
*	
* AUTHOR:  Luis Arce,  CREATION DATE:  19-Nov-85
*
* MODIFIED BY:
*
*
*--
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/errlog.h>
#include "uerror.h"
#include "eiliterals.h"
#include "ueliterals.h"
#include "uestruct.h"
#include "btliterals.h"
#include "erms.h"
#include "generic_dsd.h"
#include "std_dsd.h"
#include <signal.h>
#include <ctype.h>
#include <sys/stat.h>


/*
*++
*=
*=
*=MODULE FLOW - uerf.c
*=
*=  a - main(argc,argv)                             Bit to text translator.
*=          getenv()
*=          find_file(UE$BIN_FILE)                          (* d)
*=          parse_cmd(argc,argv)                            (getcmd.c)
*=          valid_tool()                                    (getcmd.c)
*=          dsd_init(bin_file)                              (dsd_access.c)
*=          get_bin_file_ver()                              (dsd_access.c)
*=          valid_gen()                                     (getcmd.c)
*=          if(kernel)
*=              signal(SIGINT && SIGTERM, stop_uerf)
*=          bt$open()                                       (btt.c)
*=          while(es$get(ctx,ES$NEXT))                      (esget.c)
*=              {
*=              if(EI$CORRUPT)
*=                  {
*=                  es$getads(ctx,ads)                      (esget.c)
*=                  dump_raw_rec()                          (* b)
*=                  }
*=              if(bin_output)
*=                  {
*=                  ei_write()                              (eritio.c)
*=                  continue
*=                  }
*=              if(summary)
*=                  {
*=                  es$getads(ctx,ads)                      (esget.c)
*=                  sum_save(eis,sis)                       (summary.c)
*=                  if(UE$OUT_TERSE)
*=                      sum_print(UE$OUT_TERSE)             (summary.c)
*=                  continue
*=                  }
*=              bt$put(eis)                                 (btt.c)
*=              if(UE$OUT_TERSE)
*=                  put_terse(eis,dis)                      (btt.c)
*=              else
*=                  bt$put(dis)                             (btt.c)
*=              bt$put(cds)                                 (btt.c)
*=              if(! UE$OUT_BRIEF)
*=                  {
*=                  bt$put(sds)                             (btt.c)
*=                  while (ads)
*=                      {
*=                      bt$put(sds)                         (btt.c)
*=                      es$getads(ctx,ads)                  (esget.c)
*=                      }
*=                  }
*=              if(UNKNOWN_REC | -Z)
*=                  dump_raw_rec()                          (* b)
*=              }
*=          if(summary && ! TERSE)
*=              sum_print(out_form)                         (summary.c)
*=          bt$close()                                      (btt.c)
*=          es$close(ctx,ES$FORGET)                         (esopen.c)
*=          exit(0)
*=
*=  b - dump_raw_rec()                              Hex dumps raw record.
*=          get_buff_addr()                                 (ulfile.c)
*=          hex_dump(raw_rec,raw_rec,4,0)                   (msgwrt.c)
*=          return()
*=
*=  c - stop_uerf()                                 Asyncronous function
*=                                                  used to proccess ctl-c.
*=          es$close(ctx,ES$FORGET)                         (esopen.c)
*=
*=  d - find_file(file_name)                        Finds full file name path.
*=          return(path)
*=
*
*--
*/


/*
*	.SBTTL	UERF - ULTRIX Error Report Formatter
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  Input command line is parsed and validated.
*	-  DSD table is initialized.
*	-  Input location is selected and opened.
*	-  Selection criteria is verified.
*	-  Output file is opened.
*	-  Raw (binary) records are read from input.
*	-  Raw selection criteria is applied.
*	-  Raw records are transformed to standard ERMS segments.
*	-  Std selection criteria is applied.
*	-  Standard ERMS segments are formatted for output.
*	-  Formatted output is written out to std_out device.
*	-  Close and termination process.
*	
* FORMAL PARAMETERS:		
*
*	argc			Number of command line arguments
*	argv			Command line arguments
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
/*...	PROGRAM UERF (argc,argv)				*/

ES$DECLARE(,EIS,ueis);
ES$DECLARE(,DIS,udis);
ES$DECLARE(,CDS,ucds);
ES$DECLARE(,SDS,usds);
ES$DECLARE(,ADS,uads);
ES$DECLARE(,SIS,usis);
ES$STREAM(,uctx);
short kernel_ctl_c;		/* flag to bypass 1st ctl_c if kernel */
short ctl_c;			/* flag for CTL C	*/
struct in_struc     in_st;	/* allocate in  struct	*/

char  *find_file();
void  dump_raw_rec();
void  ei_write();
char  search_path[UE$XFF];
char  bin_file[UE$XFF];

/***************************************************************/

main (argc,argv)
short argc;
char  *argv[];
{
long  stop_uerf();
long  get_bin_file_ver();
int   status;
short offset;
short i;
static short entno = 0;
static char star[] = "*********************************";
struct stat buf;

printf("\t\t\t\t\t\t %s",UE$VER_NUM);

kernel_ctl_c = FALSE;

if (strlen(argv[0]) > 4)
    {
    strncpy(search_path,argv[0], strlen(argv[0])-5);
    strcat(search_path,":");
    }
strcat(search_path,"/etc:");
strcat(search_path,getenv("PATH"));

strcpy(bin_file,find_file(UE$BIN_FILE));
if (strlen(bin_file) == 0)
    {
    print_err(UE$ERR_NOBIN);
    exit(UE$FAIL);
    }

if ((status = parse_cmd(argc,argv)) != UE$SUCC) /* parse command  */
    {
    if (status == UE$DONE)
	exit(0);		/* successful return code   */
    print_err(status);
    exit(1);			/* unsuccessful return code */
    }

if ((status = valid_tool()) != UE$SUCC) /* validate uerf specific */
    {
    print_err(status);
    exit(1);			/* unsuccessful return code */
    }

if ((status = dsd_init(bin_file)) != DD$SUCCESS)
    {
    print_err(UE$ERR_BADBIN);
    exit(1);			/* unsuccessful return code */
    }

	/* check that we have a matching .bin file */
if( UERF_VERSION != get_bin_file_ver() ) {
	printf("\nFatal Error: mismatched version of uerf data files(%d,%ld)\n",
		UERF_VERSION, get_bin_file_ver());
	exit(1);
}
printf("(%d)\n", get_bin_file_ver());

printf("\n");

if ((status = valid_gen()) != UE$SUCC) /* validate generic input */
    {
    print_err(status);
    exit(1);			/* unsuccessful return code */
    }

if (in_st.kernel)
    {
    kernel_ctl_c = TRUE;
    signal(SIGINT,stop_uerf);
    signal(SIGTERM,stop_uerf);
    }

if ((status = bt$open()) != BT$SUCC)		/* init/open bit to text*/
    {
    print_err(status);
    exit(1);			/* unsuccessful return code */
    }

while (((status = (es$get(&uctx,ES$NEXT)))
	!= ES$EOF) && (ctl_c != UE$STOP))
    {
    if (ueis.eventclass == EI$CORRUPT)
	{
	printf("\n\nCORRUPTED ERROR ENTRY - ENTRY DUMP:\n\n");
	if ((es$getads(&uctx,&uads)) == ES$SUCC)
	    dump_raw_rec();
	}
    else if (status == ES$FAIL)
	{
	print_err(UE$ERR_S_READ);
        exit(1);			/* unsuccessful return code */
	}
    else
	{
        if (in_st.out_file)
            {
            ei_write();                 /* create new log file -b option */
            continue;
            }
        if (in_st.summary)
	    {
	    while (uads.subtype != 0)
		{
		if (uads.subtype == DD$UNKNOWN_REC_ADS)
		    break;
		es$getads(&uctx,&uads);
		}
	    sum_save(&ueis,&usis);      /* save summaries */
            if (in_st.out_form == UE$OUT_TERSE)
                sum_print(UE$OUT_TERSE);
	    continue;
	    }
	if (in_st.out_form != UE$OUT_TERSE)
	    {
	    printf("\n%s ENTRY %5d. %s", star, ++entno, star);
	    bt$put(&ueis);		/* pass EIS to BTT	*/
 	    }
	if (udis.subtype != 0)
	    {
	    if (in_st.out_form == UE$OUT_TERSE)
		put_terse(&ueis,&udis);
	    else
		bt$put(&udis);		/* pass DIS to BTT	*/
	    }
	if (ucds.subtype != 0)
	    bt$put(&ucds);		/* pass CDS to BTT	*/
	if (in_st.out_form != UE$OUT_BRIEF)
	    {
	    if (usds.subtype != 0)
		bt$put(&usds);		/* pass SDS to BTT	*/
	    while (uads.subtype != 0)
		{
		if (uads.subtype == DD$UNKNOWN_REC_ADS)
		    break;
		bt$put(&uads);		/* pass ADS to BTT	*/
		es$getads(&uctx,&uads);
		}
	    }
	}
    if (uads.subtype == DD$UNKNOWN_REC_ADS)
	{
	printf("\n\nUNABLE TO PROPERLY TRANSLATE ENTRY - ENTRY DUMP:");
	dump_raw_rec();
	}
    else if (in_st.dump_rec == TRUE)
	{
	printf("\n\nRECORD ENTRY DUMP:");
	dump_raw_rec();
	}
    if (!in_st.summary)
        printf("\n");
    }
if (in_st.summary)
    if (in_st.out_form != UE$OUT_TERSE)
        sum_print(in_st.out_form);
bt$close();				/* terminate BTT	*/

if (ctl_c != UE$STOP)
    es$close(&uctx,ES$FORGET);		/* terminate ERMS	*/

printf("\n");
exit(0);			/* successful return code */
}
/*...	ENDPROGRAM UERF						*/



/*
*	.SBTTL	DUMP_RAW_REC - Function to display the raw record in hex
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  this function is used when the hex dump of the raw record
*	   is needed. Record is corrupted, or unsupported, etc.
*
* FORMAL PARAMETERS:		
*
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
/*...	FUNCTION DUMP_RAW_REC       			*/

void  dump_raw_rec()
{

struct el_rec *get_buff_addr();
struct el_rec *raw_rec;

raw_rec = get_buff_addr();

if ((raw_rec->elrhdr.rhdr_reclen < 1) ||
    (raw_rec->elrhdr.rhdr_reclen > sizeof(struct el_rec)))
	raw_rec->elrhdr.rhdr_reclen = sizeof(struct el_rec);
printf("\n");
hex_dump(raw_rec,raw_rec->elrhdr.rhdr_reclen,4,0);
printf("\n");
}
/*...   ENDFUNCTION DUMP_RAW_REC			*/


/*
*	.SBTTL	STOP_UERF - function used when ctl c is entered
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  this is an asyncronous function defined by "signal".
*	   it is entered on the signals SIGINT or SIGTERM.
*
* FORMAL PARAMETERS:		
*
*
* IMPLICIT INPUTS:		NONE
*
* IMPLICIT OUTPUTS:		ctl_c is set to UE$STOP
*
* COMPLETION STATUS:		NONE
*
* SIDE EFFECTS:			NONE
*
*--
*/
/*...	FUNCTION STOP_UERF       			*/

long  stop_uerf()
{

extern short kernel_ctl_c;

es$close(&uctx,ES$FORGET);
ctl_c = UE$STOP;

}
/*...   ENDFUNCTION STOP_UERF       			*/


/*
*	.SBTTL	FIND_FILE - function used to find bin, hlp or err files
*++
* FUNCTIONAL DESCRIPTION:		
*
*	-  this function is used to search the designated search_path
*          for the file name that was passed as a parameter.
*
* FORMAL PARAMETERS:		
*
*
* IMPLICIT INPUTS:		file_name
*
* IMPLICIT OUTPUTS:		full file name or null string
*
* COMPLETION STATUS:		NONE
*
* SIDE EFFECTS:			NONE
*
*--
*/
/*...	FUNCTION FIND_FILE (file_name)		*/

char *find_file(file_name)
char *file_name;
{
short	i;
short	offset;
long	status;
char	path[UE$XFF];
char    nul = '\0';
struct	stat buf;

for (offset = 0, i = 0; offset < strlen(search_path); offset++)
    {
    if (search_path[offset] != ':')
	{
	path[i++] = search_path[offset];
	}
    else
	{
	path[i++] = '/';
	path[i] = nul;
	strcat(path,file_name);
	if ((status = stat(path,&buf)) == 0)
	    return path;
	i = 0;
	}
    }
path[0] = nul;
return path;
}
/*...   ENDFUNCTION FIND_FILE   			*/
