

#ifndef lint
static char *sccsid = "@(#)rzdisk.c	4.3    ULTRIX  9/11/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,86,87,88,89 by		*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *
 * rzdisk.c	12-Sep-88
 *
 * SCSI Disk Utility Source File
 *
 * Modification history:
 *
 * 20-Aug-90	Robin Miller
 *	Added entries in the floppy format function to support the RX26
 *	floppy drive.  Entries were added for 2.88MB, 1.44MB, & 720KB.
 *
 * 31-Jul-90	Robin Miller
 *	Modified the disk format function to use current parameters
 *	instead of default parameters.  Previously, parameters changed
 *	in pages 3 & 4 by the user were overridden when formatting.
 *
 * 12-Jul-90	Robin Miller
 *	Added entries in the floppy format function to support the RX33
 *	floppy drive.  Entries were added for 1.2MB, 400KB, & 360KB.
 *
 * 24-Jul-89	Fred Canter
 *	Modify rzdisk to know about dynamic BBR. If rzdisk can read the
 *	block without error, then the block is not bad or the driver
 *	has already replaced it (dynamic BBR). Ask user "Are you sure?".
 *
 * 23-Jun-89	Fred Canter
 *	Don't save the pages on mode select prior to formatting a hard disk.
 *	Mode select could fail if current disk format has been corrupted.
 *
 *	Account for page 0 (SMS 3500 controller has one). End of pages was
 *	page code of, now also check page length of 0.
 *
 * 17-Jun-89	Fred Canter
 *	Fixed a bug which caused verification of a HD floppy formatted
 *	as a DD floppy, then reformatted back to HD to fail. Close then
 *	reopen the floppy to force the driver to do read capacity.
 *
 * 15-Jun-89	Fred Canter
 *	Fixed a serious bug in the floppy format check pass (missing lseek).
 *	Fixed more hacks with RRD40 mode select pages 2 and 3.
 *
 * 14-Jun-89	Fred Canter
 *	Added a verify pass to the floppy format command.
 *	Retry floppy format if a unit attention condition exists.
 *	Warn user that some format options are restricted.
 *	Fixed geterror() so its not fooled by no sense data returned.
 *
 * 24-May-89	Fred Canter
 *	Added page 5 for RX23 floppy diskette formatting.
 *	Changed the mode select data structures so an address and
 *	length are passed with the ioctl. This allows pages to be
 *	added to rzdisk without requiring a kernel rebuild.
 *	Make rzdisk much more general (less hardwired knowledge
 *	about devices).
 *
 * 06-Feb-89	Alan Frechette
 *	Added in page 8 support.
 *
 * 09-Jan-89	Fred Canter
 *	Minor comment change.
 *
 * 27-Dec-88	Alan Frechette
 *	Code cleanup changes. Added the routine "fix_mode_select_params()"
 *	to fix up the mode select parameters before issuing the MODE
 *	SELECT command. Fixed the routine "scan_for_bad_blocks()" to 
 *	print out the correct error message when the error code is not
 *	(UNRECOVERABLE, RECOVERABLE with RETRIES, or RECOVERABLE with ECC).
 *	Added comments throughout this code.
 *
 * 20-Dec-88	Alan Frechette
 *	Do not check for errors after issuing the inquiry command.
 *	If the sense key is (SC_NOSENSE) then return the error code
 *	(NO_ERROR) from routine "geterror()". Ignore all (SC_NOSENSE)
 *	errors.
 *
 *	Fixed displaying of CHANGEABLE parameters for RZ23/RZ22 disks.
 *	Fixed some bugs dealing with changing and getting parameters
 *	for the RZ23/RZ22 disks.
 *
 *	If MODE SENSE or MODE SELECT fails when trying to format a
 *	disk then skip it.
 *
 * 13-Dec-88	Alan Frechette
 *	Fixed a bug which caused a core dump when you type "rzdisk"
 *	with no parameters. Changed some error messages. Fixed the
 *	displaying of the menu.
 *
 * 09-Dec-88	Alan Frechette
 *	Reworked getting and changing disk drive parameters. Made 
 *	extensive changes to these routines to get it to work in 
 *	all cases. Added the option of getting the CHANGEABLE
 *	parameters from the disk drive. The CHANGEABLE parameters
 *	returns a bit pattern of the bits that can be changed
 *	within a particular field.
 *
 * 06-Dec-88	Alan Frechette
 *	Fixed "reassign_bad_block()" to retry the read of the bad 
 *	block upto 5 times. Also always write the data back to 
 *	the reassigned block even if the data is bad. The user
 *	may still be able to recover from this by running "fsck".
 *
 *	Fixed a bug in "disk_mounted()" when searching for mounted
 *	file systems.
 *
 * 12-Sep-88	Alan Frechette
 *	Created this utility for the maintainence of scsi disks.
 *
 * IMPORTANT NOTE:
 *	This utility supports only the DIGITAL supplied disks
 *	(RZ22, RZ23, RX23, RZ55, and RRD40). This utility must be tested
 *	and, possibly, modified for each new DIGITAL supported disk drive.
 *	This utility is not guaranteed to work with non DIGITAL
 *	supplied disks, due to vendor dependent SCSI implementations.
 *
 ************************************************************************/
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/errno.h>
#include <ufs/fs.h>
#include <sys/rzdisk.h>

int	rzdev = -1;
char	rzdisk[40];
int 	rzcom;
int 	infobyte;
int	asc;

/*
 * Floppy diskette format types:
 */
#define ED_3_5		1		/* Extra density	2.88MB	*/
#define HD_3_5		2		/* High density		1.44MB	*/
#define DD_3_5		3		/* Double density	 720KB	*/

#define HD_5_25		4		/* High density		 1.2MB	*/
#define DD_5_25		5		/* Double density	 720KB	*/
#define SD_5_25_RX50	6		/* Single density RX50	 400KB	*/
#define SD_5_25		7		/* Signle density	 360KB	*/

#define DISKETTE_OTHER	8		/* Other - Enter custom params.	*/
#define DISKETTE_CANCEL	9		/* Cancel the format operation.	*/


/* A good idea that just didn't see the light of day. */
#ifdef	PAGE_INFO
#define	MAX_PAGES	64
struct	page_header page_info[MAX_PAGES];	/* info about existing pages */
#endif

struct 	read_defect_params		rdp;
struct 	format_params			fp;
struct 	reassign_params			rp;
struct 	verify_params			vp;
struct 	defect_descriptors		dd;
struct 	mode_sel_sns_params 		msp;
struct 	mode_sel_sns_data 		ms;
struct 	mode_sel_sns_params 		mspc;
struct 	mode_sel_sns_data 		msc;
struct	inquiry_info			inq;
#define MSIZE (NMOUNT*sizeof(struct fs_data))
#define NUMTITLES 9
int fields[2*NUMTITLES] = {-12,12,-8,8,-8,8,-8,8,-6,6,-8,8,-8,8,-4,4,-8,8};
int num_fields[2*NUMTITLES] = {-12,12,8,8,8,8,8,8,6,0,8,8,8,8,6,0,-8,8};
	

main(argc,argv)
int argc;
char *argv[];
{

	int lbn,length,i,j;
	u_char defect_format;
	int fmt_defect_lists;
	int page_control;

	if(argc == 1) {
		print_help();
		exit(0);
	}
	if(argv[1][0] != '-') {
		printf("\nNo option specified, type \"rzdisk -h\" for help.\n");
		exit(1);
	}

	switch(argv[1][1]) {
	case 'h':
		if(argc != 2) {
		    printf("\nUsage: rzdisk -h.\n");
		    exit(1);
		}
		print_help();
		break;

	case 'i':
		if(argc != 3) {
		    printf("\nUsage: rzdisk -i special.\n");
		    exit(1);
		}
		open_special_file(argv[2]);
		get_inquiry_info();
		break;

	case 'c':
		if(argc != 3 && argc != 4 || 
			(argc == 4 && strcmp(argv[2],"ask") != 0)) {
		    printf("\nUsage: rzdisk -c [ask] special.\n");
		    exit(1);
		}
		if(argc == 4) {
		    open_special_file(argv[3]);
		    change_drive_parameters(1);
		} 
		else {
		    open_special_file(argv[2]);
		    change_drive_parameters(0);
		}
		break;

	case 'g':
		if(argc != 4) {
		    printf("\nUsage: rzdisk -g (current|saved|default|changeable) special.\n");
		    exit(1);
		}
		if(strcmp(argv[2],"current") == 0)
		    page_control = CURRENT_VALUES;
		else if(strcmp(argv[2],"saved") == 0)
		    page_control = SAVED_VALUES;
		else if(strcmp(argv[2],"default") == 0)
		    page_control = DEFAULT_VALUES;
		else if(strcmp(argv[2],"changeable") == 0)
		    page_control = CHANGED_VALUES;
		else {
		    printf("\nUsage: rzdisk -g (current|saved|default|changeable) special.\n");
		    exit(1);
		}
		open_special_file(argv[3]);
		get_drive_parameters(page_control);
		break;

	case 'f':
		if(argc == 3) {
		    open_special_file(argv[2]);
		    if(rzdisk[strlen(rzdisk) - 1] != 'c') {
		        printf("\nMust specify (c) partition for -f option.\n");
			exit(1);
		    }
		    format_a_floppy();
		    break;
		}
		if(argc != 4) {
		    printf("\nUsage: rzdisk -f (vendor|known) special.\n");
		    exit(1);
		}
		if(strcmp(argv[2],"vendor") == 0)
		    fmt_defect_lists = VENDOR_DEFECTS;
		else if(strcmp(argv[2],"known") == 0)
		    fmt_defect_lists = KNOWN_DEFECTS;
		else {
		    printf("\nUsage: rzdisk -f (vendor|known) special.\n");
		    exit(1);
		}
		open_special_file(argv[3]);
		if(rzdisk[strlen(rzdisk) - 1] != 'c') {
		    printf("\nMust specify (c) partition for -f option.\n");
		    exit(1);
	        }
		format_a_disk(fmt_defect_lists);
		break;

	case 'r':
		if(argc != 4 || !isinteger(argv[2])) {
		    printf("\nUsage: rzdisk -r LBN special.\n");
		    exit(1);
		}
		lbn = atoi(argv[2]);
		open_special_file(argv[3]);
		if(rzdisk[strlen(rzdisk) - 1] != 'c') {
		    printf("\nMust specify (c) partition for -r option.\n");
		    exit(1);
	        }
		reassign_bad_block(lbn);
		break;

	case 's':
		if(argc != 5 || !isinteger(argv[2]) || !isinteger(argv[3])) {
		    printf("\nUsage: rzdisk -s LBN length special.\n");
		    exit(1);
		}
		lbn = atoi(argv[2]);
		length = atoi(argv[3]);
		open_special_file(argv[4]);
		scan_for_bad_blocks(lbn,length);
		break;

	case 'd':
		if(argc != 4) {
		    printf("\nUsage: rzdisk -d (bfi|sector|block) special.\n");
		    exit(1);
		}
		if(strcmp(argv[2],"bfi") == 0)
		    defect_format = BFI_FORMAT;
		else if(strcmp(argv[2],"sector") == 0)
		    defect_format = PHY_FORMAT;
		else if(strcmp(argv[2],"block") == 0)
		    defect_format = BLK_FORMAT;
		else {
		    printf("\nUsage: rzdisk -d (bfi|sector|block) special.\n");
		    exit(1);
		}
		open_special_file(argv[3]);
		read_defects(defect_format);
		break;

	default:
		printf("\nBad option (-%c), type \"rzdisk -h\" for help.\n",
				argv[1][1]);
		exit(1);
		break;
	}
}

open_special_file(special)
char *special;
{
	strcpy(rzdisk,special);
	if(strncmp(special,"/dev/",5) == 0 &&
		strncmp(special,"/dev/rr",7) != 0) {
		printf("\nMust specify raw device special file.\n");
		exit(1);
	}
	if(strncmp(special,"/dev/rrz",8) != 0) {
		printf("\nMust specify special file of the form (/dev/rrz??).\n");
		exit(1);
	}
	if((rzdev = open(rzdisk, O_RDWR|O_NDELAY)) == -1) {
		printf("\nCannot open SCSI device (%s) exiting.\n",rzdisk);
		exit(1);
	}
}

isinteger(string)
char *string;
{

	if(*string == '-' || *string == '+')
		++string;
	while(*string) {
		if((*string >= '0' && *string <= '9') || 
		   	(*string == '-') || (*string == '+'))
			++string;
		else
			return(0);
	}
	return(1);
}

print_help()
{
	printf("\n\n");
	printf("\t\t********************************\n");
	printf("\t\t**** SCSI Disk Utility Menu ****\n");
	printf("\t\t********************************\n");
	printf("\n\tUsage: rzdisk -cdfghirs [LBN|command] [length] special.\n\n");
	printf("rzdisk -f vendor /dev/rrz0c\tFormats disk with VENDOR only defects.\n");
	printf("rzdisk -f known /dev/rrz0c\tFormats disk with all KNOWN defects.\n");
	printf("rzdisk -f /dev/rrz0c\t\tFormats a floppy diskette.\n");
	printf("rzdisk -r 1234 /dev/rrz0c\tReassigns bad block (1234).\n");
	printf("rzdisk -s 0 -1 /dev/rrz0c\tScans the entire disk for bad blocks.\n");
	printf("rzdisk -s 0 -1 /dev/rrz0g\tScans partition (g) for bad blocks.\n");
	printf("rzdisk -i /dev/rrz0c\t\tPrints out the inquiry data info.\n");
	printf("rzdisk -d bfi /dev/rrz0c\tReads defect list in BFI format.\n");
	printf("rzdisk -d sector /dev/rrz0c\tReads defect list in SECTOR format.\n");
	printf("rzdisk -d block /dev/rrz0c\tReads defect list in BLOCK format.\n");
	printf("rzdisk -c /dev/rrz0c\t\tChanges disk parameters to DEFAULT VALUES.\n");
	printf("rzdisk -c ask /dev/rrz0c\tChanges disk parameters interactively.\n");
	printf("rzdisk -g current /dev/rrz0c\tGets CURRENT disk drive parameters.\n");
	printf("rzdisk -g saved /dev/rrz0c\tGets SAVED disk drive parameters.\n");
	printf("rzdisk -g default /dev/rrz0c\tGets DEFAULT disk drive parameters.\n");
	printf("rzdisk -g changeable /dev/rrz0c\tGets CHANGEABLE disk drive parameters.\n");
	printf("rzdisk -h\t\t\tPrints out this help menu.\n");
}

format_a_disk(fmt_defect_lists)
int fmt_defect_lists;
{
	int pid, i;
	long thetime;
	char *ctime();
	char prodid[17];
#ifdef notdef
	u_char *byteptr;
	char *bp_ms, *bp_msc;
	struct page_code_3 *p3;
	struct page_code_3 *q3;
	struct page_code_4 *p4;
	struct page_code_4 *q4;
#endif notdef

	bzero((char *)&inq, sizeof(inq));
	msp.msp_addr = (caddr_t)&inq;
	msp.msp_length = sizeof(inq);
	if(execute_rzcmd(SCSI_GET_INQUIRY_DATA, (char *)&msp) != SUCCESS) {
	    if(geterror() != NO_ERROR)
		return;
	}
	for(i=0; i<16; i++)
	    prodid[i] = inq.prodid[i];
	prodid[i] = NULL;
	/*
	 * FORMAT UNIT only supported on direct access devices.
	 */
	if(inq.perfdt != 0) {
	    printf("\nCANNOT FORMAT %s - not a direct access device.\n",
		rzdisk);
	    return;
	}
	/*
	 * Cannot FORMAT a disk that is mounted.
	 */
	if(disk_mounted())
	    return;
	/*
	 * Inform the user what's going on.
	 */
	printf("\nFORMATTING WILL DESTROY ALL DATA ON DISK (%s)!\n", rzdisk);
	printstr("\nARE YOU SURE (y/n)? ");
	if(!confirm())
	    return;
	/*
	 * Initialize the defect header and the defect list. Note 
 	 * that there is no defect list being sent and therefore 
	 * the defect list length is set to zero (0).
	 */
	bzero((char *)&dd, sizeof(dd));
	fp.fp_format = BLK_FORMAT;
	/* was 1, we now use 0 to specify the drive's default interleave */
	fp.fp_interleave = 0;
	fp.fp_pattern = 0;
	fp.fp_defects = fmt_defect_lists;
	fp.fp_length = 0;
	dd.dd_header.fu_hdr.vu = 0;
	dd.dd_header.fu_hdr.dcrt = 0;
	dd.dd_header.fu_hdr.dpry = 0;
#ifndef	RZ55_STPF
	/* LEDS says setting the STPF bit is not necessary */
	dd.dd_header.fu_hdr.fov = 0;
	dd.dd_header.fu_hdr.stpf = 0;
#endif	RZ55_STPF
	fp.fp_addr = (u_char *)&dd;
#ifdef	RZ55_STPF
	/*
	 * Set stop format on error bit if disk is RZ55.
	 * RZ23/RZ23 (and we assume all other disks) don't
	 * support this bit so we set it to zero.
	 * TODO: how to ask drive if it supports the STPF bit?
	 */
	if(strncmp(prodid,"RZ55",4) == 0) {
	    dd.dd_header.fu_hdr.fov = 1;
	    dd.dd_header.fu_hdr.stpf = 1;
	}
	else {
	    dd.dd_header.fu_hdr.fov = 0;
	    dd.dd_header.fu_hdr.stpf = 0;
	}
#endif	RZ55_STPF
#ifdef notdef
	/*
	 * This code was removed so that current parameters setup by
	 * the user via the change option get used during formatting,
	 * instead of always setting the default parameters.  If the
	 * current parameters are incorrect, the user has the option
	 * to of setting the default parameters before formatting.
	 */

	/*
	 * Set mode select pages 3 and 4 (geometry pages) to
	 * drive's default values (if they are changeable).
	 */
	bzero((char *)&ms, sizeof(ms));
	msp.msp_pgcode = 0x3;
	msp.msp_addr = (caddr_t)&ms;
	msp.msp_length = sizeof(ms);
	msp.msp_pgctrl = DEFAULT_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&msp) != SUCCESS) {
	    if(geterror() != NO_ERROR)
		return;
	}
	bzero((char *)&msc, sizeof(msc));
	mspc.msp_pgcode = 0x3;
	mspc.msp_addr = (caddr_t)&msc;
	mspc.msp_length = sizeof(msc);
	mspc.msp_pgctrl = CHANGED_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&mspc) != SUCCESS) {
	    if(geterror() != NO_ERROR)
		return;
	}
	bp_ms = (char *)&ms;
	bp_ms += sizeof(ms.ms_hdr);		/* skip mode select header */
	bp_msc = (char *)&msc;
	bp_msc += sizeof(ms.ms_hdr);		/* skip mode select header */
	bp_ms += ms.ms_hdr.blk_des_len;		/* point to start of page 1 */
	bp_msc += msc.ms_hdr.blk_des_len;	/* point to start of page 1 */

	p3 = (struct page_code_3 *)bp_ms;
	q3 = (struct page_code_3 *)bp_msc;
	/*
	 * Do not save any pages. If the current disk format is
	 * corrupt (format interrupted by power fail) then there
	 * may not be any pages to save and mode select could fail.
	 */
	msp.msp_setps = 0;
	p3->ps = 0;
	bp_ms += sizeof (struct page_header);
	bp_msc += sizeof (struct page_header);
	if(q3->pglength) {
	    for(i = 0; i < p3->pglength; i++)
		*bp_ms++ &= *bp_msc++;

	    ms.ms_hdr.sense_len = 0;
	    ms.ms_hdr.medium_type = 0;
	    ms.ms_hdr.wp = 0;
	    msp.msp_addr = (caddr_t)&ms;
	    msp.msp_length = sizeof (struct mode_sel_sns_header) +
		ms.ms_hdr.blk_des_len +
			sizeof (struct page_header) +
				p3->pglength;
	    /*
	     * Write the DRIVE PARAMETERS using the MODE SELECT command.
	     */
	    if(execute_rzcmd(SCSI_MODE_SELECT, (char *)&msp) != SUCCESS) {
		if(geterror() != NO_ERROR)
		    return;
	    }
	}

	bzero((char *)&ms, sizeof(ms));
	msp.msp_pgcode = 0x4;
	msp.msp_addr = (caddr_t)&ms;
	msp.msp_length = sizeof(ms);
	msp.msp_pgctrl = DEFAULT_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&msp) != SUCCESS) {
	    if(geterror() != NO_ERROR)
		return;
	}
	bzero((char *)&msc, sizeof(msc));
	mspc.msp_pgcode = 0x4;
	mspc.msp_addr = (caddr_t)&msc;
	mspc.msp_length = sizeof(msc);
	mspc.msp_pgctrl = CHANGED_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&mspc) != SUCCESS) {
	    if(geterror() != NO_ERROR)
		return;
	}
	bp_ms = (char *)&ms;
	bp_ms += sizeof(ms.ms_hdr);		/* skip mode select header */
	bp_msc = (char *)&msc;
	bp_msc += sizeof(ms.ms_hdr);		/* skip mode select header */
	bp_ms += ms.ms_hdr.blk_des_len;		/* point to start of page 1 */
	bp_msc += msc.ms_hdr.blk_des_len;	/* point to start of page 1 */

	p4 = (struct page_code_4 *)bp_ms;
	q4 = (struct page_code_4 *)bp_msc;
	/*
	 * Do not save any pages. If the current disk format is
	 * corrupt (format interrupted by power fail) then there
	 * may not be any pages to save and mode select could fail.
	 */
	msp.msp_setps = 0;
	p4->ps = 0;
	bp_ms += sizeof (struct page_header);
	bp_msc += sizeof (struct page_header);
	if(q4->pglength) {
	    for(i = 0; i < p4->pglength; i++)
		*bp_ms++ &= *bp_msc++;

	    ms.ms_hdr.sense_len = 0;
	    ms.ms_hdr.medium_type = 0;
	    ms.ms_hdr.wp = 0;
	    msp.msp_addr = (caddr_t)&ms;
	    msp.msp_length = sizeof (struct mode_sel_sns_header) +
		ms.ms_hdr.blk_des_len +
			sizeof (struct page_header) +
				p4->pglength;
	    /*
	     * Write the DRIVE PARAMETERS using the MODE SELECT command.
	     */
	    if(execute_rzcmd(SCSI_MODE_SELECT, (char *)&msp) != SUCCESS) {
		if(geterror() != NO_ERROR)
		    return;
	    }
	}
#endif notdef
	/*
	 * Start FORMATTING the disk.
	 */
	printf("\nFormatting device (%s).\n",rzdisk);
	if((pid = fork()) == 0) {
	 	for(;;) {
		    time(&thetime);
		    printf("\tworking ..... %s",ctime(&thetime));
		    sleep(120);
	    	}
	}
	sleep(2);
	if(execute_rzcmd(SCSI_FORMAT_UNIT, (char *)&fp) != SUCCESS) {
		if(geterror() != NO_ERROR) {
			kill(pid, SIGKILL);
			return;
		}
	}
	kill(pid, SIGKILL);
	printf("Done formatting device (%s).\n",rzdisk);
}

format_a_floppy()
{

	char *malloc();
	int pid;
	long thetime;
	int setps;
	int value,i; 
	char *ptr;
	int num_sp;
	struct page_header *pp;
	struct page_header *pp_ms, *pp_msc;
	char *bp_ms, *bp_msc;
	struct page_code_3 *p3;
	struct page_code_3 *q3;
	struct page_code_5 *p5;
	struct page_code_5 *q5;
	int sec_trk, db_sec, num_head, num_cyl, sp_cyl, xfer_rate;
	int err, hd, cyl, bcnt;
	char *rbuf;
	int ret, doing_retry;

	/*
	 * Cannot FORMAT a disk that is mounted.
	 */
	if(disk_mounted())
		return;
	/*
	 * Don't allow format if device does not have
	 * flexible disk parameters page (page 5).
	 */
	msp.msp_pgcode = 0x5;
	msp.msp_addr = (caddr_t)&ms;
	msp.msp_length = sizeof(ms);
	msp.msp_pgctrl = DEFAULT_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&msp) != SUCCESS) {
	    printf("\nCANNOT FORMAT %s - not a floppy disk (no page 5).\n",
		rzdisk);
	    return;
	}
	/*
	 * Inform the user what's going on.
	 */
	printf("\nFORMATTING WILL DESTROY ALL DATA ON DISKETTE (%s)!\n",
		rzdisk);
	/*
	 * Ask the user which format to use.
	 */
	while (1) {
	    printf("\nPlease enter one of the following format options:\n");
	    printf("\n    %d - 2.88MB 3.5\" ED DISKETTE %s\n", ED_3_5,
		"(36 sector, 80 cylinder, 2 head)");
	    printf("\n    %d - 1.44MB 3.5\" HD DISKETTE %s\n", HD_3_5,
		"(18 sector, 80 cylinder, 2 head)");
	    printf("\n    %d - 720KB  3.5\" DD DISKETTE %s\n", DD_3_5,
		"( 9 sector, 80 cylinder, 2 head)");
	    printf("\n    %d - 1.2MB  5.25\" HD DISKETTE %s\n", HD_5_25,
		"(15 sector, 80 cylinder, 2 head)");
	    printf("\n    %d - 720KB  5.25\" DD DISKETTE %s\n", DD_5_25,
		"( 9 sector, 80 cylinder, 2 head)");
	    printf("\n    %d - 400KB  5.25\" SD DISKETTE %s\n", SD_5_25_RX50,
		"(10 sector, 80 cylinder, 1 head)");
	    printf("\n    %d - 360KB  5.25\" SD DISKETTE %s\n", SD_5_25,
		"( 9 sector, 40 cylinder, 2 head)");
	    printf("\n    %d - OTHER FORMAT TYPE       %s\n", DISKETTE_OTHER,
		"( you supply format parameters )");
	    printf("\n    %d - CANCEL FORMAT OPERATION\n", DISKETTE_CANCEL);
	    printstr("\nFormat Type: ");
	    i = getinteger();
	    switch(i) {

	    case ED_3_5:
		sec_trk = 36;
		db_sec = 512;
		num_head = 2;
		num_cyl = 80;
		sp_cyl = 0;
		xfer_rate = 1000;
		break;

	    case HD_3_5:
		sec_trk = 18;
		db_sec = 512;
		num_head = 2;
		num_cyl = 80;
		sp_cyl = 0;
		xfer_rate = 500;
		break;

	    case DD_3_5:
		sec_trk = 9;
		db_sec = 512;
		num_head = 2;
		num_cyl = 80;
		sp_cyl = 0;
		xfer_rate = 250;
		break;

	    case HD_5_25:
		sec_trk = 15;
		db_sec = 512;
		num_head = 2;
		num_cyl = 80;
		sp_cyl = 0;
		xfer_rate = 500;
		break;

	    case DD_5_25:
		sec_trk = 9;
		db_sec = 512;
		num_head = 2;
		num_cyl = 80;
		sp_cyl = 0;
		xfer_rate = 250;
		break;

	    case SD_5_25_RX50:
		sec_trk = 10;
		db_sec = 512;
		num_head = 1;
		num_cyl = 80;
		sp_cyl = 0;
		xfer_rate = 250;
		break;

	    case SD_5_25:
		sec_trk = 9;
		db_sec = 512;
		num_head = 2;
		num_cyl = 40;
		sp_cyl = 0;
		xfer_rate = 250;
		break;

	    case DISKETTE_OTHER:
		printstr("\nNumber of sectors per track? ");
		sec_trk = getinteger();
		db_sec = 512;
		printstr("Number of heads? ");
		num_head = getinteger();
		printstr("Number of Cylinders? ");
		num_cyl = getinteger();
		printstr("Number of step pulses per cylinder? ");
		sp_cyl = getinteger();
		printstr("Transfer rate (kbit/second)? ");
		xfer_rate = getinteger();
		break;

	    case DISKETTE_CANCEL:
		return;
		break;		/* NOTREACHED */

	    default:
		printf("\nInvalid format type (<CTRL/C> to abort)\n");
		continue;
	    }
	    if( (i == DD_3_5) || (i == DD_5_25) ||
		(i == SD_5_25_RX50) || (i == SD_5_25) ||
		(i == DISKETTE_OTHER) ) {
		printf("\nRESTRICTED FORMAT: %s\n",
		    "see rzdisk(8) reference page for details.");
		printstr("\nARE YOU SURE (y/n)? ");
		if(!confirm())
		    continue;
	    }
	    if(i == ED_3_5)
		ptr = " Double Sided ED ";
	    if( (i == HD_3_5) || (i == HD_5_25) )
		ptr = " Double Sided HD ";
	    else if( (i == DD_3_5) || (i == DD_5_25) )
		ptr = " Double Sided DD ";
	    else if(i == SD_5_25_RX50)
		ptr = " Single Sided SD ";
	    else if(i == SD_5_25)
		ptr = " Double Sided SD ";
	    else
		ptr = " ";
	    printf("\nInsert a%sdiskette into the drive.\n", ptr);
	    printstr("\nReady to begin format (y/n)? ");
	    if(!confirm())
		continue;
	    break;
	}
	doing_retry = 0;
fmt_retry:

	/*
	 * Tell the drive how to format the floppy by
	 * setting mode select pages 3 and 5.
	 * NOTE: we get the current values instead of the defaults
	 *	 because some of the values (that we don't set) are
	 *	 changeable. Hopefully, current = default will be true.
	 */
	bzero((char *)&ms, sizeof(ms));
	msp.msp_pgcode = 0x3;
	msp.msp_addr = (caddr_t)&ms;
	msp.msp_length = sizeof(ms);
	msp.msp_pgctrl = CURRENT_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&msp) != SUCCESS) {
	    if(geterror() != NO_ERROR)
		return;
	}
	bzero((char *)&msc, sizeof(msc));
	mspc.msp_pgcode = 0x3;
	mspc.msp_addr = (caddr_t)&msc;
	mspc.msp_length = sizeof(msc);
	mspc.msp_pgctrl = CHANGED_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&mspc) != SUCCESS) {
	    if(geterror() != NO_ERROR)
		return;
	}
	bp_ms = (char *)&ms;
	bp_ms += sizeof(ms.ms_hdr);		/* skip mode select header */
	bp_msc = (char *)&msc;
	bp_msc += sizeof(ms.ms_hdr);		/* skip mode select header */
	bp_ms += ms.ms_hdr.blk_des_len;		/* point to start of page 1 */
	bp_msc += msc.ms_hdr.blk_des_len;	/* point to start of page 1 */
	/*
	 * Make sure block descriptor is correct.
	 * Force LBN size to 512 bytes.
	 */
	if(ms.ms_hdr.blk_des_len == 8) {
	    ms.ms_desc.density_code &= msc.ms_desc.density_code;
	    ms.ms_desc.nblks2 &= msc.ms_desc.nblks2;
	    ms.ms_desc.nblks1 &= msc.ms_desc.nblks1;
	    ms.ms_desc.nblks0 &= msc.ms_desc.nblks0;
	    ms.ms_desc.blklen2 = ((512 >> 16) & 0xff);
	    ms.ms_desc.blklen1 = ((512 >> 8) & 0xff);
	    ms.ms_desc.blklen0 = (512 & 0xff);
	}

	p3 = (struct page_code_3 *)bp_ms;
	q3 = (struct page_code_3 *)bp_msc;
/*	msp.msp_setps = p3->ps;	OLDWAY */
	msp.msp_setps = 0;
	p3->ps = 0;

	p3->spt1 = ((sec_trk >> 8) & 0xff);
	p3->spt0 = ((sec_trk >> 0) & 0xff);

	p3->bps1 = ((db_sec >> 8) & 0xff);
	p3->bps0 = ((db_sec >> 0) & 0xff);

/*
 * SCSI-II specs says this field is ignored.

	p3->interleave1 = ((value >> 8) & 0xff);
	p3->interleave0 = ((value >> 0) & 0xff);
*/

	bp_ms += sizeof (struct page_header);
	bp_msc += sizeof (struct page_header);
	/* if length is zero the page is not changeable */
	if(q3->pglength) {
	    for(i = 0; i < p3->pglength; i++)
		*bp_ms++ &= *bp_msc++;

	    ms.ms_hdr.sense_len = 0;
	    ms.ms_hdr.medium_type = 0;
	    ms.ms_hdr.wp = 0;
	    msp.msp_addr = (caddr_t)&ms;
	    msp.msp_length = sizeof (struct mode_sel_sns_header) +
		ms.ms_hdr.blk_des_len +
			sizeof (struct page_header) +
				p3->pglength;
	    /*
	     * Write the DRIVE PARAMETERS using the MODE SELECT command.
	     */
	    if(execute_rzcmd(SCSI_MODE_SELECT, (char *)&msp) != SUCCESS) {
		if(geterror() != NO_ERROR)
		    return;
	    }
	}

	bzero((char *)&ms, sizeof(ms));
	msp.msp_pgcode = 0x5;
	msp.msp_addr = (caddr_t)&ms;
	msp.msp_length = sizeof(ms);
	msp.msp_pgctrl = CURRENT_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&msp) != SUCCESS) {
	    if(geterror() != NO_ERROR)
		return;
	}
	bzero((char *)&msc, sizeof(msc));
	mspc.msp_pgcode = 0x5;
	mspc.msp_addr = (caddr_t)&msc;
	mspc.msp_length = sizeof(msc);
	mspc.msp_pgctrl = CHANGED_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&mspc) != SUCCESS) {
	    if(geterror() != NO_ERROR)
		return;
	}
	bp_ms = (char *)&ms;
	bp_ms += sizeof(ms.ms_hdr);		/* skip mode select header */
	bp_msc = (char *)&msc;
	bp_msc += sizeof(ms.ms_hdr);		/* skip mode select header */
	bp_ms += ms.ms_hdr.blk_des_len;		/* point to start of page 1 */
	bp_msc += msc.ms_hdr.blk_des_len;	/* point to start of page 1 */
	/*
	 * Make sure block descriptor is correct.
	 * Force LBN size to 512 bytes.
	 */
	if(ms.ms_hdr.blk_des_len == 8) {
	    ms.ms_desc.density_code &= msc.ms_desc.density_code;
	    ms.ms_desc.nblks2 &= msc.ms_desc.nblks2;
	    ms.ms_desc.nblks1 &= msc.ms_desc.nblks1;
	    ms.ms_desc.nblks0 &= msc.ms_desc.nblks0;
	    ms.ms_desc.blklen2 = ((512 >> 16) & 0xff);
	    ms.ms_desc.blklen1 = ((512 >> 8) & 0xff);
	    ms.ms_desc.blklen0 = (512 & 0xff);
	}

	p5 = (struct page_code_5 *)bp_ms;
	q5 = (struct page_code_5 *)bp_msc;
/*	msp.msp_setps = p5->ps;	OLDWAY */
	msp.msp_setps = 0;
	p5->ps = 0;

	p5->xfer_rate1 = ((xfer_rate >> 8) & 0xff);
	p5->xfer_rate0 = (xfer_rate & 0xff);

	p5->num_heads = num_head;

	p5->sec_per_trk = sec_trk;

	p5->db_per_physec1 = ((db_sec >> 8) & 0xff);
	p5->db_per_physec0 = (db_sec & 0xff);

	p5->num_cyl1 = ((num_cyl >> 8) & 0xff);
	p5->num_cyl0 = (num_cyl & 0xff);

	p5->sp_cyl = sp_cyl;

	bp_ms += sizeof (struct page_header);
	bp_msc += sizeof (struct page_header);
	if(q5->pglength) {
	    for(i = 0; i < p5->pglength; i++)
		*bp_ms++ &= *bp_msc++;

	    ms.ms_hdr.sense_len = 0;
	    ms.ms_hdr.medium_type = 0;
	    ms.ms_hdr.wp = 0;
	    msp.msp_addr = (caddr_t)&ms;
	    msp.msp_length = sizeof (struct mode_sel_sns_header) +
		ms.ms_hdr.blk_des_len +
			sizeof (struct page_header) +
				p5->pglength;
	    /*
	     * Write the DRIVE PARAMETERS using the MODE SELECT command.
	     */
	    if(execute_rzcmd(SCSI_MODE_SELECT, (char *)&msp) != SUCCESS) {
		if(geterror() != NO_ERROR)
		    return;
	    }
	}
	else {
	    printf("\nCANNOT FORMAT %s - flexible disk page not changeable.\n",
		rzdisk);
	    return;
	}
	if (!doing_retry) {
	    printf("\nFormatting device (%s).\n",rzdisk);
	    if((pid = fork()) == 0) {
		for(;;) {
		    time(&thetime);
		    printf("\tworking ..... %s",ctime(&thetime));
		    sleep(120);
		}
	    }
	    sleep(2);
	}

	fp.fp_format = BLK_FORMAT;
	fp.fp_interleave = 0;
	fp.fp_pattern = 0;
	fp.fp_length = 0;
	fp.fp_defects = NO_DEFECTS;
	fp.fp_addr = 0;
	/*
	 * We retry the format command once a unit attention
	 * condition exits for the floppy drive. This will happen
	 * if the user does not insert the floppy before starting
	 * the rzdisk utility.
	 */
	if(execute_rzcmd(SCSI_FORMAT_UNIT, (char *)&fp) != SUCCESS) {
	    ret = geterror();
	    if(ret != NO_ERROR) {
		if((ret == ATTN_ERROR) && (!doing_retry)) {
		    doing_retry = 1;
		    goto fmt_retry;
		}
		kill(pid, SIGKILL);
		return;
	    }
	}
	kill(pid, SIGKILL);
	printf("Done formatting device (%s).\n",rzdisk);
	/*
	 * Do a check pass to verify the format succeeded.
	 * Media compatibility is not guaranteed between HD
	 * and DD diskettes.
	 */
	printf("\nVerifying format of device (%s).\n",rzdisk);
	if((pid = fork()) == 0) {
	    for(;;) {
		time(&thetime);
		printf("\tworking ..... %s",ctime(&thetime));
		sleep(120);
	    }
	}
	sleep(2);

	/*
	 * Close then reopen the floppy so we are sure
	 * the driver has the correct size information.
	 */
	close(rzdev);
	if((rzdev = open(rzdisk, O_RDWR)) == -1) {
	    printf("Cannot reopen floppy drive (%s).\n", rzdisk);
	    kill(pid, SIGKILL);
	    return;
	}
	bcnt = 512 * sec_trk;
	rbuf = (char *)malloc(bcnt);
	err = 0;
	lseek(rzdev, 0, 0);
	for(hd = 0 ; hd < num_head; hd++) {
	    if(err)
		break;
	    for(cyl = 0; cyl < num_cyl; cyl++) {
		if(err)
		    break;
		if(read(rzdev, rbuf, bcnt) != bcnt) {
		    printf("\nVerify failed: %s head %d cylinder %d.\n",
			"read error on", hd, cyl);
		    err = 1;
		}
	    }
	}

	kill(pid, SIGKILL);
	if(err == 0)
	    printf("Done verifying device (%s).\n",rzdisk);
}

disk_mounted()
{
	int i,ret;
	struct	fs_data *fd;
	char *malloc();
	int loc;
	int big;
	struct	fs_data *mountbuffer;
	char temp[40];
	char *p;
	
	mountbuffer = (struct fs_data *) malloc(MSIZE);
	if(mountbuffer == NULL) {
		printf("\nUnable to get system memory.\n");
		return(-1);
	}
	loc = 0;
	ret = getmnt(&loc,mountbuffer,MSIZE,STAT_MANY,0);
	if(ret < 0) {
		printf("\nUnable to get mounted file system info.\n");
		return(-1);
	}
	big = 0;
	for(fd=mountbuffer; fd < &mountbuffer[ret]; fd++) {
		i = strlen(fd->fd_devname);
		if(i > big) big=i;
	}
	fields[0] = -big;
	fields[1] = big;
	num_fields[0] = -big;
	num_fields[1] = big;
	strcpy(temp,"/dev/r");
	p = &rzdisk[strlen("/dev/rr")];
	i = 6;
	while(*p)
		temp[i++] = *p++;
	temp[--i] = NULL;
	for(fd=mountbuffer; fd < &mountbuffer[ret]; fd++) {
		if(strncmp(temp,fd->fd_devname,strlen(temp)) == 0) {
		    printf("\nTHE DEVICE (%s) HAS MOUNTED FILESYSTEMS!\n",
				rzdisk);
		    printf("\nCANNOT FORMAT A DISK THAT'S MOUNTED!\n");
		    printf("\nUNMOUNT ALL FILESYSTEMS BEFORE FORMATTING!\n");
		    return(1);
		}
	}
	return(0);
}

reassign_bad_block(lbn)
int lbn;
{
	char buffer[512];
	int baddata;
	off_t offset;
	int retries = 5;
	int i;
	char prodid[17];

	bzero((char *)&inq, sizeof(inq));
	msp.msp_addr = (caddr_t)&inq;
	msp.msp_length = sizeof(inq);
	execute_rzcmd(SCSI_GET_INQUIRY_DATA, (char *)&msp);
	for(i=0; i<16; i++)
		prodid[i] = inq.prodid[i];
	prodid[i] = NULL;
	/*
	 * REASSIGN BLOCK not supported on some disks.
	 */
	if((strncmp(prodid,"RRD40",5) == 0) ||
	   (strncmp(prodid,"RX23",4) == 0) ||
	   (strncmp(prodid,"RX33",4) == 0)) {
	    	printf("\nReassign Block unsupported on device (%s).\n",rzdisk);
	    	return;
	}
	/*
	 * Read the data from the bad block before doing
     	 * the reassign and save it in a temporary buffer.
	 * Set "baddata" flag if we could not read the data
 	 * in the bad block.
	 */
	bzero(buffer, 512);
	offset = lbn * 512;
	while(retries--) {
	    lseek(rzdev, offset, 0);
	    if(read(rzdev, buffer, 512) == -1)
		baddata = 1;
	    else {
		baddata = 0;
		break;
	    }
	}
	/*
	 * If we were able to read the bad block, then
	 * the block is not really bad or the driver has
	 * already reassigned it (via dynamic BBR).
	 * Ask if user really wants to reassign this block.
	 * This is not the best method, but it should
	 * prevent double reassign of blocks with ECC errors.
	 * STRATEGY:
	 *	Unrecoverable error (ASC=11), reassign block.
	 *	ECC recovered error (ASC=18), driver reassigns block (DBBR).
	 *	Retry recovered error (ASC=17), don't reassign block,
	 *	    user can force reassign (Are you sure? yes).
	 */
	if (baddata == 0) {
	    printf("\nRead good data from block %d.", lbn);
	    printf(" Either the block is not\nbad or the driver");
	    printf(" already reassigned it via dynamic BBR.\n");
	    printf("Refer to \"Bad Block Replacement on SCSI ");
	    printf("disks\" for assistance.\n");
	    printstr("\nCANCEL REASSIGN BLOCK (y/n)? ");
	    if(confirm())
		return;
	}
	/*
	 * Initialize the defect header and the defect list. Note 
 	 * that there is only 1 block that is reassigned at a time
 	 * so the defect list length is set to four (4).
	 */
	bzero((char *)&rp, sizeof(rp));
	rp.rp_lbn3 = ((lbn >> 24) & 0xff);
	rp.rp_lbn2 = ((lbn >> 16) & 0xff);
	rp.rp_lbn1 = ((lbn >> 8) & 0xff);
	rp.rp_lbn0 = ((lbn >> 0) & 0xff);
	rp.rp_header.defect_len1 = 0;
	rp.rp_header.defect_len0 = 4;
	retries = 0;
reassign:
	printf("\nReassigning bad block (%d) on device (%s).\n",lbn,rzdisk);
	if(execute_rzcmd(SCSI_REASSIGN_BLOCK, (char *)&rp) != SUCCESS) {
	    	if(geterror() != NO_ERROR)
			return;
	}
	/*
	 * Write the saved data from the temporary buffer
     	 * back to the newly reassigned block. 
	 */
	lseek(rzdev, offset, 0);
	if(write(rzdev, buffer, 512) == -1) {
	    printf("\nWrite of data to replacement block failed.\n");
	    if(++retries <= 2)
		goto reassign;
	    else
		return;
	}
	/*
	 * Inform user if the data is (GOOD) or (BAD).
	 */
	if(baddata)
		printf("\nThe data in the reassigned block is (BAD).\n");
	else
		printf("\nThe data in the reassigned block is (GOOD).\n");
}

scan_for_bad_blocks(lbn,length)
int lbn;
int length;
{

	int modval,i;
	int lastblk,part,badlbn;
	char cpart;
	struct pt pt;
	char *print_error_code();
	int err;

	bzero((char *)&vp, sizeof(vp));
	/* 
	 * Get the partition table for the disk being scanned.
	 */
	if(execute_rzcmd(DIOCGETPT, (char *)&pt) != SUCCESS) {
		printf("\nCannot get partition table from device (%s).\n",
				rzdisk);
		return;
	}
	cpart = rzdisk[strlen(rzdisk) - 1];
	part = cpart - 'a';
	lastblk = pt.pt_part[part].pi_blkoff + pt.pt_part[part].pi_nblocks;
	/* 
	 * Check for a valid (LBN) entered.
	 */
	if(lbn < 0 || lbn > lastblk) {
		printf("\nInvalid LBN (%d) entered for device (%s).\n",
			lbn, rzdisk);
		printf("\nValid LBN range for device (%s) is [%d-%d].\n",
			rzdisk, pt.pt_part[part].pi_blkoff,
				pt.pt_part[part].pi_nblocks);
		return;
	}
	lbn = lbn + pt.pt_part[part].pi_blkoff;
	/* 
	 * Check for a valid (length) entered.
	 */
	if(length == -1)
		length = lastblk - lbn;
	else if((length + lbn) > lastblk) {
		printf("\nInvalid length (%d) entered for device (%s).\n",
			length, rzdisk);
		printf("\nLength plus LBN cannot exceed (%d) for device (%s).\n",
			lastblk, rzdisk);
		return;
	}
	printf("\nScanning for bad blocks on device (%s).\n",rzdisk);
	printf("\nBlock range being scanned is [%d-%d].\n\n",
			lbn, lbn+length-1);
	/* 
	 * Begin scanning 30000 blocks at a time.
	 */
	while(length != 0) {
		vp.vp_lbn = (u_long)lbn;
		modval = lbn % 30000;
		if(length > 30000) {
			vp.vp_length = (u_short) (30000 - modval);
			length -= (30000 - modval);
			lbn += (30000 - modval);
		}
		else {
			vp.vp_length = (u_short) length;
			length -= length;
			lbn += length;
		}
		printf("Scanning blocks [%d-%d]\n",
			vp.vp_lbn,(vp.vp_length+vp.vp_lbn-1));
restart_scan:
		if(execute_rzcmd(SCSI_VERIFY_DATA, (char *)&vp) != SUCCESS) {
			err = geterror();
			badlbn = infobyte;
			/* 
	 		 * Handle a FATAL ERROR.
	 		 */
			if(err == FATAL_ERROR)
				break;
			/* 
	 		 * Handle a RECOVERABLE ERROR.
	 		 */
			else if(err == SOFT_ERROR) {
			    if(asc == 0x18)
			        printf("\tRecoverable read error with ECC");
			    else if(asc == 0x17)
			        printf("\tRecoverable read error with RETRIES");
			    else
				printf("\tRecoverable read error: (%s)",
					print_error_code(asc));
			    printf(" at (LBN = %d)\n", badlbn);
			}
			/* 
	 		 * Handle a UNRECOVERABLE ERROR.
	 		 */
			else if(err == HARD_ERROR) {
			    if(asc == 0x11)
			        printf("\tUnrecoverable read error");
			    else
				printf("\tUnrecoverable read error: (%s)",
					print_error_code(asc));
			    printf(" at (LBN = %d)\n", badlbn);
			}
			/* 
	 		 * Handle NO ERROR.
	 		 */
			else if(err == NO_ERROR) {
			    continue;
			}
			vp.vp_length = (vp.vp_length + vp.vp_lbn - badlbn - 1);
			vp.vp_lbn = badlbn + 1;
			goto restart_scan;
		}
	}
	printf("\nDone scanning for bad blocks on device (%s).\n",rzdisk);
}

read_defects(defect_format)
u_char defect_format;
{
	int value,i; 
	char prodid[17];

	bzero((char *)&inq, sizeof(inq));
	msp.msp_addr = (caddr_t)&inq;
	msp.msp_length = sizeof(inq);
	execute_rzcmd(SCSI_GET_INQUIRY_DATA, (char *)&msp);
	for(i=0; i<16; i++)
		prodid[i] = inq.prodid[i];
	prodid[i] = NULL;
	/*
	 * BLOCK format not supported on RZ22/RZ23 disks.
	 */
	if((strncmp(prodid,"RZ22",4) == 0 ||
		strncmp(prodid,"RZ23",4) == 0) && 
			defect_format == BLK_FORMAT) {
	    	printf("\nBLOCK format unsupported on device (%s).\n", 
			rzdisk);
	    	return;
	}
	/*
	 * READ DEFECT DATA not supported on some disks.
	 */
	if((strncmp(prodid,"RRD40",5) == 0) ||
	   (strncmp(prodid,"RX23",4) == 0) ||
	   (strncmp(prodid,"RX33",4) == 0)) {
	    	printf("\nRead Defect Data unsupported on device (%s).\n",
			rzdisk);
	    	return;
	}
	bzero((char *)&dd, sizeof(dd));
	rdp.rdp_format = defect_format;
	rdp.rdp_alclen = sizeof(dd);
	rdp.rdp_addr = (u_char *)&dd;
	printf("\nReading defects from device (%s).\n",rzdisk);
	if(execute_rzcmd(SCSI_READ_DEFECT_DATA, (char *)&rdp) != SUCCESS) {
		if(geterror() != NO_ERROR)
			return;
	}
	print_defects();
}

print_defects()
{
	int i, j, defect_length, ndefects, linecnt;
	int cylinder, bfi, sector, lba;

        printf("\nDefect list header:\n");
        printf("  Format 0x%02x", dd.dd_header.rdd_hdr.format);
        if(dd.dd_header.rdd_hdr.mdl)
    	    	printf(" MDL");
        if(dd.dd_header.rdd_hdr.gdl)
    	    	printf(" GDL");
        switch(dd.dd_header.rdd_hdr.format) {
        	case 4: printf(" BYTES\n"); break;
        	case 5: printf(" SECTOR\n"); break;
        	case 6: printf(" VENDOR\n"); break;
        	case 7: printf(" RESERVED\n"); break;
        	default:printf(" BLOCK\n"); break;
        }
	defect_length = (dd.dd_header.rdd_hdr.defect_len0 & 0x00ff) +
			((dd.dd_header.rdd_hdr.defect_len1 << 8) & 0xff00);
	if(dd.dd_header.rdd_hdr.format == BLK_FORMAT)
		ndefects = defect_length / 4;
	else
		ndefects = defect_length / 8;
        printf("  Defect list length %d number of defects %d\n",
			defect_length, ndefects);
        i = 0;
        j = 0;
	linecnt = 0;
        while(i < defect_length) {
        	switch (dd.dd_header.rdd_hdr.format) {
        	case 0:
        	case 1:
        	case 2:
        	case 3:
		    lba = ((dd.BLK[j].lba3 << 24) & 0xff000000) +
			  ((dd.BLK[j].lba2 << 16) & 0xff0000) +
			  ((dd.BLK[j].lba1 << 8) & 0xff00) +
			  ((dd.BLK[j].lba0 << 0) & 0xff);
        	    printf(" Block %8d\n", lba);
        	    i += 4;
        	    break;
    
        	case 4:
		    cylinder = ((dd.BFI[j].cyl2 << 16) & 0xff0000) +
			       ((dd.BFI[j].cyl1 << 8) & 0xff00) +
			       ((dd.BFI[j].cyl0 << 0) & 0xff);
		    bfi = ((dd.BFI[j].bfi3 << 24) & 0xff000000) +
			  ((dd.BFI[j].bfi2 << 16) & 0xff0000) +
			  ((dd.BFI[j].bfi1 << 8) & 0xff00) +
			  ((dd.BFI[j].bfi0 << 0) & 0xff);
        	    printf(" Cylinder %6d  Head %2d  Byte %6d\n",
				cylinder, dd.BFI[j].head, bfi);
        	    i += 8;
        	    break;
    
        	case 5:
		    cylinder = ((dd.PHY[j].cyl2 << 16) & 0xff0000) +
			       ((dd.PHY[j].cyl1 << 8) & 0xff00) +
			       ((dd.PHY[j].cyl0 << 0) & 0xff);
		    sector = ((dd.PHY[j].sector3 << 24) & 0xff000000) +
			     ((dd.PHY[j].sector2 << 16) & 0xff0000) +
			     ((dd.PHY[j].sector1 << 8) & 0xff00) +
			     ((dd.PHY[j].sector0 << 0) & 0xff);
        	    printf(" Cylinder %6d  Head %2d  Sector %4d\n",
				cylinder, dd.PHY[j].head, sector);
        	    i += 8;
        	    break;
    
        	case 6:
        	case 7:
        	    return;
        	}
		++j;
        	if(++linecnt == 20) {
		    linecnt = 0;
        	    if((i < defect_length) && !moreoutput())
        	    	return;
        	}
        }
}

change_drive_parameters(ask)
int ask;
{

	int setps;
	int value,i; 
	char prodid[17];
	char *ptr, *p;
	int num_sp, sum;
	struct page_header *pp;
	struct page_header *pp_ms, *pp_msc;
	char *bp_ms, *bp_msc;
	struct page_code_1 *p1;
	struct page_code_1 *q1;
	struct page_code_2 *p2;
	struct page_code_2 *q2;
	struct page_code_3 *p3;
	struct page_code_3 *q3;
	struct page_code_4 *p4;
	struct page_code_4 *q4;
	struct page_code_5 *p5;
	struct page_code_5 *q5;
	struct page_code_8 *p8;
	struct page_code_8 *q8;
	struct page_code_37 *p37;
	struct page_code_37 *q37;

	bzero((char *)&inq, sizeof(inq));
	msp.msp_addr = (caddr_t)&inq;
	msp.msp_length = sizeof(inq);
	if(execute_rzcmd(SCSI_GET_INQUIRY_DATA, (char *)&msp) != SUCCESS) {
	    if(geterror() != NO_ERROR)
		return;
	}
	for(i=0; i<16; i++)
	    prodid[i] = inq.prodid[i];
	prodid[i] = NULL;
	/*
	 * User wants to enter new parameters interactively.
	 */
	if(ask)
	    	goto interactive;
	/*
	 * Inform the user what's going on.
	 */
	printf("\nCHANGING DISK DRIVE PARAMETERS TO DEFAULT VALUES!\n");
	printstr("\nARE YOU SURE (y/n)? ");
	if(!confirm())
		return;
	/*
	 * Get the DEFAULT disk drive parameters.
	 */
	bzero((char *)&ms, sizeof(ms));
	msp.msp_pgcode = 0x3f;
	msp.msp_length = sizeof(ms);
	msp.msp_addr = (caddr_t)&ms;
	msp.msp_pgctrl = DEFAULT_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&msp) != SUCCESS) {
		if(geterror() != NO_ERROR)
			return;
	}
	/*
	 * Fill in the page information structure.
	 * Tells which pages this device supports and
	 * which of those pages can be saved.
	 */
#ifdef	PAGE_INFO
	for(i=0; i<MAX_PAGES; i++) {
		page_info[i].pgcode = 0;
		page_info[i].pglength = 0;
	}
#endif	PAGE_INFO
	num_sp = 0;
	ptr = (char *)&ms;
	ptr += sizeof(ms.ms_hdr);		/* skip mode select header */
	ptr += ms.ms_hdr.blk_des_len;		/* point to start of page 1 */
	while(1) {
		pp = (struct page_header *)ptr;
		if ((pp->pgcode == 0) && (pp->pglength == 0))
			break;
		if (pp->ps) {
#ifdef	PAGE_INFO
			page_info[pp->pgcode].ps = 1;
#endif	PAGE_INFO
			num_sp++;
			pp->ps = 0;	/* must be zero for mode select */
		}
#ifdef	PAGE_INFO
		page_info[pp->pgcode].pgcode = pp->pgcode;
		page_info[pp->pgcode].pglength = pp->pglength;
#endif	PAGE_INFO
		ptr += sizeof(struct page_header) + pp->pglength;
	}
	/*
	 * Get the CHANGEABLE disk drive parameters.
	 */
	bzero((char *)&msc, sizeof(msc));
	mspc.msp_pgcode = 0x3f;
	mspc.msp_length = sizeof(msc);
	mspc.msp_addr = (caddr_t)&msc;
	mspc.msp_pgctrl = CHANGED_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&mspc) != SUCCESS) {
		if(geterror() != NO_ERROR)
			return;
	}
	/*
	 * If there are any saveable pages, ask the user
	 * whether or not to save them.
	 */
	if(num_sp == 0)
		setps = 0;
	else {
		printstr("\nSAVE THE DEFAULT VALUES ON DISK (y/n)? ");
		if(confirm())
			setps = 1;
		else
			setps = 0;
	}
	msp.msp_setps = setps;
	msp.msp_length = ms.ms_hdr.sense_len + 1;
	ms.ms_hdr.sense_len = 0;
	ms.ms_hdr.medium_type = 0;
	ms.ms_hdr.wp = 0;
	/*
	 * For each data byte of each page and the block descriptor,
	 * and it with the same data byte in the changeable page.
	 * This makes sure we don't attempt to set any non-
	 * changeable bits to one (which would cause a check
	 * condition on the mode select).
	 */
	bp_ms = (char *)&ms;
	bp_ms += sizeof(ms.ms_hdr);		/* skip mode select header */
	bp_msc = (char *)&msc;
	bp_msc += sizeof(ms.ms_hdr);		/* skip mode select header */
	bp_ms += ms.ms_hdr.blk_des_len;		/* point to start of page 1 */
	bp_msc += msc.ms_hdr.blk_des_len;	/* point to start of page 1 */
	/*
	 * Make sure block descriptor is correct.
	 * Force LBN size to 512 bytes.
	 */
	if(ms.ms_hdr.blk_des_len == 8) {
	    ms.ms_desc.density_code &= msc.ms_desc.density_code;
	    ms.ms_desc.nblks2 &= msc.ms_desc.nblks2;
	    ms.ms_desc.nblks1 &= msc.ms_desc.nblks1;
	    ms.ms_desc.nblks0 &= msc.ms_desc.nblks0;
	    ms.ms_desc.blklen2 = ((512 >> 16) & 0xff);
	    ms.ms_desc.blklen1 = ((512 >> 8) & 0xff);
	    ms.ms_desc.blklen0 = (512 & 0xff);
	}
	while(1) {
		pp_ms = (struct page_header *)bp_ms;
		pp_msc = (struct page_header *)bp_msc;
		if ((pp_ms->pgcode == 0) && (pp_ms->pglength == 0))
			break;
		if (pp_ms->pgcode != pp_msc->pgcode) {
		    printf("change_drive_parameters: %s\n",
			"ms.pgcode does not match msc.pgcode!");
		    return;
		}
		bp_ms += sizeof (struct page_header);
		bp_msc += sizeof (struct page_header);
		/*
		 * If the page has zero length or all the data
		 * bytes are zero, then the page is not changeable.
		 * So don't send it (RRD40 fails if we send page 2).
		 * The RRD40 says page 3 is changeable but its not,
		 * so we zap the bytes per physical sector field.
		 */
		sum=0;
		if(pp_msc->pglength) {
			ptr = (char *)bp_msc;
			if((strncmp(prodid,"RRD40",5) == 0) &&
			   (pp_msc->pgcode == 3)) {
				q3 = (struct page_code_3 *)(bp_msc -
					sizeof(struct page_header));
				q3->bps1 = 0;
				q3->bps0 = 0;
			}
			for(i = 0; i < pp_msc->pglength; i++)
				sum += *ptr++;
		}
		if(pp_msc->pglength && sum) {
			for(i = 0; i < pp_ms->pglength; i++)
				*bp_ms++ &= *bp_msc++;
		}
		else {
			bp_msc += pp_msc->pglength;	/* could be zero */
			bp_ms -= sizeof(struct page_header);
			msp.msp_length -= sizeof(struct page_header);
			msp.msp_length -= pp_ms->pglength;
			p = bp_ms;
			ptr = bp_ms + sizeof(struct page_header) +
				pp_ms->pglength;
			while(ptr < ((char *)&ms + sizeof(ms)))
				*p++ = *ptr++;
			while(p < ((char *)&ms + sizeof(ms)))
				*p++ = 0;
		}
	}
	/*
	 * Change the disk drive parameters to their DEFAULT values.
	 */
	printf("\nChanging disk drive parameters for device (%s).\n",
		rzdisk);
	if(execute_rzcmd(SCSI_MODE_SELECT, (char *)&msp) != SUCCESS) {
		if(geterror() != NO_ERROR)
			return;
	}
	return;
interactive:
	/*
	 * Inform the user what's going on.
	 */
	printf("\nCHANGING DISK DRIVE PARAMETERS INTERACTIVELY!\n");
	printstr("\nARE YOU SURE (y/n)? ");
	if(!confirm())
		return;
	/*
	 * Get the CURRENT disk drive parameters.
	 */
	bzero((char *)&ms, sizeof(ms));
	msp.msp_pgcode = 0x3f;
	msp.msp_length = sizeof(ms);
	msp.msp_addr = (caddr_t)&ms;
	msp.msp_pgctrl = CURRENT_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&msp) != SUCCESS) {
		if(geterror() != NO_ERROR)
			return;
	}
	bzero((char *)&msc, sizeof(msc));
	mspc.msp_pgcode = 0x3f;
	mspc.msp_length = sizeof(msc);
	mspc.msp_addr = (caddr_t)&msc;
	mspc.msp_pgctrl = CHANGED_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&mspc) != SUCCESS) {
		if(geterror() != NO_ERROR)
			return;
	}
	/*
 	 * This section asks the user which parameters they want to
	 * change. The utility displays one page at a time and asks
	 * asks the user if they want to change any parameters in
	 * that given page. This goes on until all pages have been
 	 * displayed to the user.
	 */
	printf("\nBlock descriptor:\n");
	printf("  Density code\t\t\t\t%d\n", ms.ms_desc.density_code);
	printf("  Number of blocks\t\t\t%d\n", 
			((ms.ms_desc.nblks2<<16) + 
				(ms.ms_desc.nblks1<<8) + ms.ms_desc.nblks0));
	printf("  Block length\t\t\t\t%d\n", 
			((ms.ms_desc.blklen2<<16) + 
				(ms.ms_desc.blklen1<<8) + ms.ms_desc.blklen0));
/*
	printstr("\nDo you want to change the block length (y/n)? ");
	if(confirm()) {
		printstr("Enter the new block length? ");
		value = getinteger();
		ms.ms_desc.blklen2 = ((value >> 16) & 0xff);
		ms.ms_desc.blklen1 = ((value >> 8) & 0xff);
		ms.ms_desc.blklen0 = ((value >> 0) & 0xff);
	}
*/
	printf("\nBlock descriptor changes not allowed (%s).\n",
		"block length must be 512");

	bp_ms = (char *)&ms;
	bp_ms += sizeof(ms.ms_hdr);		/* skip mode select header */
	bp_msc = (char *)&msc;
	bp_msc += sizeof(ms.ms_hdr);		/* skip mode select header */
	bp_ms += ms.ms_hdr.blk_des_len;		/* point to start of page 1 */
	bp_msc += msc.ms_hdr.blk_des_len;	/* point to start of page 1 */
	num_sp = 0;
	while(1) {
	    pp_ms = (struct page_header *)bp_ms;
	    pp_msc = (struct page_header *)bp_msc;
	    if ((pp_ms->pgcode == 0) && (pp_ms->pglength == 0))
		break;
	    /*
	     * If the page has zero length or all the data
	     * bytes are zero, then the page is not changeable.
	     * So don't send it (RRD40 fails if we send page 2).
	     * The RRD40 says page 3 is changeable but its not,
	     * so we zap the bytes per physical sector field.
	     */
	    if (pp_msc->pglength == 0) {
		bp_ms += sizeof(struct page_header) + pp_ms->pglength;
		bp_msc += sizeof(struct page_header);
		continue;
	    }
	    sum=0;
	    if(pp_msc->pglength) {
		ptr = (char *)bp_msc + sizeof(struct page_header);
		if((strncmp(prodid,"RRD40",5) == 0) &&
		   (pp_msc->pgcode == 3)) {
			q3 = (struct page_code_3 *)bp_msc;
			q3->bps1 = 0;
			q3->bps0 = 0;
		}
		for(i = 0; i < pp_msc->pglength; i++)
			sum += *ptr++;
	    }
	    if(pp_msc->pglength && (sum == 0)) {
		bp_ms += sizeof(struct page_header) + pp_ms->pglength;
		bp_msc += sizeof(struct page_header) + pp_msc->pglength;
		continue;
	    }
	    if (pp_ms->pgcode != pp_msc->pgcode) {
		printf("change_drive_parameters: %s\n",
		    "ms.pgcode does not match msc.pgcode!");
		return;
	    }
	    if(pp_ms->ps)
		num_sp++;	/* remember number of saveable pages */
	    pp_ms->ps = 0;	/* must be zero for mode select */
	    switch(pp_ms->pgcode) {
	    case 1:					/* Page 1 */
		p1 = (struct page_code_1 *)bp_ms;
		q1 = (struct page_code_1 *)bp_msc;
		printf("\nPage 1 - error recovery parameters:\n");
		printf("  PS\t\t\t\t\t%d\n", p1->ps);
		printf("  Page code\t\t\t\t%d\n", p1->pgcode);
		printf("  Page length\t\t\t\t%d\n", p1->pglength);
		printf("  Flags\t\t\t\t\t0x%02x\n", p1->flags);
		printf("  Retry count\t\t\t\t%d\n", p1->retry_count);
		printf("  Correction span\t\t\t%d\n", p1->correct_span);
		printf("  Head offset count\t\t\t%d\n", p1->head_offset);
		printf("  Data strobe offset count\t\t%d\n", p1->data_strobe);
		printf("  Recovery time limit\t\t\t%d\n", p1->recovery_time);
		printstr("\nDo you want to change any fields in this page (y/n)? ");
		if(!confirm())
		    break;
		if(q1->flags) {
		    printstr("Enter the new flags in (HEX)? ");
		    value = gethexnum();
		    p1->flags = value;
		}
		if(q1->retry_count) {
		    printstr("Enter the new retry count? ");
		    value = getinteger();
		    p1->retry_count = value;
		}
		if(q1->correct_span) {
		    printstr("Enter the new correction span? ");
		    value = getinteger();
		    p1->correct_span = value;
		}
		if(q1->head_offset) {
		    printstr("Enter the new head offset count? ");
		    value = getinteger();
		    p1->head_offset = value;
		}
		if(q1->data_strobe) {
		    printstr("Enter the new data strobe offset count? ");
		    value = getinteger();
		    p1->data_strobe = value;
		}
		if(q1->recovery_time) {
		    printstr("Enter the new recovery time limit? ");
		    value = getinteger();
		    p1->recovery_time = value;
		}
		break;

	    case 2:					/* Page 2 */
		p2 = (struct page_code_2 *)bp_ms;
		q2 = (struct page_code_2 *)bp_msc;
		printf("\nPage 2 - disconnect/reconnect parameters:\n");
		printf("  PS\t\t\t\t\t%d\n", p2->ps);
		printf("  Page code\t\t\t\t%d\n", p2->pgcode);
		printf("  Page length\t\t\t\t%d\n", p2->pglength);
		printf("  Buffer full ratio\t\t\t%d\n", p2->bus_fratio);
		printf("  Buffer empty ratio\t\t\t%d\n", p2->bus_eratio);
		printf("  Bus inactivity limit\t\t\t%d\n", 
				((p2->bus_inactive1<<8) +
					p2->bus_inactive0));
		printf("  Disconnect time limit\t\t\t%d\n",
				((p2->disconn_time1<<8) +
					p2->disconn_time0));
		printf("  Connect time limit\t\t\t%d\n", 
			((p2->conn_time1<<8) +
				p2->conn_time0));
		printstr("\nDo you want to change any fields in this page (y/n)? ");
		if(!confirm())
		    break;
		if(q2->bus_fratio) {
		    printstr("Enter the new bus full ratio? ");
		    value = getinteger();
		    p2->bus_fratio = value;
		}
		if(q2->bus_eratio) {
		    printstr("Enter the new bus empty ratio? ");
		    value = getinteger();
		    p2->bus_eratio = value;
		}
		if(q2->bus_inactive1 || q2->bus_inactive0) {
		    printstr("Enter the new bus inactivity limit? ");
		    value = getinteger();
		    p2->bus_inactive1 = ((value >> 8) & 0xff);
		    p2->bus_inactive0 = ((value >> 0) & 0xff);
		}
		if(q2->disconn_time1 || q2->disconn_time0) {
		    printstr("Enter the new disconnect time limit? ");
		    value = getinteger();
		    p2->disconn_time1 = ((value >> 8) & 0xff);
		    p2->disconn_time0 = ((value >> 0) & 0xff);
		}
		if(q2->conn_time1 || q2->conn_time0) {
		    printstr("Enter the new connect time limit? ");
		    value = getinteger();
		    p2->conn_time1 = ((value >> 8) & 0xff);
		    p2->conn_time0 = ((value >> 0) & 0xff);
		}
		break;

	    case 3:					/* Page 3 */
		p3 = (struct page_code_3 *)bp_ms;
		q3 = (struct page_code_3 *)bp_msc;
		printf("\nPage 3 - direct-access device format parameters:\n");
		printf("  PS\t\t\t\t\t%d\n", p3->ps);
		printf("  Page code\t\t\t\t%d\n", p3->pgcode);
		printf("  Page length\t\t\t\t%d\n", p3->pglength);
		printf("  Tracks per zone\t\t\t%d\n", 
				((p3->tpz1<<8) + p3->tpz0));
		printf("  Alternate sectors per zone\t\t%d\n", 
				((p3->aspz1<<8) + p3->aspz0));
		printf("  Alternate tracks per zone\t\t%d\n", 
				((p3->atpz1<<8) + p3->atpz0));
		printf("  Alternate tracks per volume\t\t%d\n", 
				((p3->atpv1<<8) + p3->atpv0));
		printf("  Sectors per track\t\t\t%d\n", 
				((p3->spt1<<8) + p3->spt0));
		printf("  Data bytes per phy sector\t\t%d\n", 
				((p3->bps1<<8) + p3->bps0));
		printf("  Interleave\t\t\t\t%d\n", 
				((p3->interleave1<<8) + 
					p3->interleave0));
		printf("  Track skew\t\t\t\t%d\n", 
				((p3->track_skew1<<8) + 
					p3->track_skew0));
		printf("  Cylinder skew\t\t\t\t%d\n", 
				((p3->cylinder_skew1<<8) + 
					p3->cylinder_skew0));
		printf("  Flags\t\t\t\t\t0x%x\n", p3->flags);
		printstr("\nDo you want to change any fields in this page (y/n)? ");
		if(!confirm())
		    break;
		if(q3->tpz1 || q3->tpz0) {
		    printstr("Enter the new tracks per zone? ");
		    value = getinteger();
		    p3->tpz1 = ((value >> 8) & 0xff);
		    p3->tpz0 = ((value >> 0) & 0xff);
		}
		if(q3->aspz1 || q3->aspz0) {
		    printstr("Enter the new alternate sectors per zone? ");
		    value = getinteger();
		    p3->aspz1 = ((value >> 8) & 0xff);
		    p3->aspz0 = ((value >> 0) & 0xff);
		}
		if(q3->atpz1 || q3->atpz0) {
		    printstr("Enter the new alternate tracks per zone? ");
		    value = getinteger();
		    p3->atpz1 = ((value >> 8) & 0xff);
		    p3->atpz0 = ((value >> 0) & 0xff);
		}
		if(q3->atpv1 || q3->atpv0) {
		    printstr("Enter the new alternate tracks per volume? ");
		    value = getinteger();
		    p3->atpv1 = ((value >> 8) & 0xff);
		    p3->atpv0 = ((value >> 0) & 0xff);
		}
		if(q3->spt1 || q3->spt0) {
		    printstr("Enter the new sectors per track? ");
		    value = getinteger();
		    p3->spt1 = ((value >> 8) & 0xff);
		    p3->spt0 = ((value >> 0) & 0xff);
		}
		if(q3->bps1 || q3->bps0) {
		    printstr("Enter the new bytes per sector? ");
		    value = getinteger();
		    p3->bps1 = ((value >> 8) & 0xff);
		    p3->bps0 = ((value >> 0) & 0xff);
		}
		if(q3->interleave1 || q3->interleave0) {
		    printstr("Enter the new interleave factor? ");
		    value = getinteger();
		    p3->interleave1 = ((value >> 8) & 0xff);
		    p3->interleave0 = ((value >> 0) & 0xff);
		}
		if(q3->track_skew1 || q3->track_skew0) {
		    printstr("Enter the new track skew? ");
		    value = getinteger();
		    p3->track_skew1 = ((value >> 8) & 0xff);
		    p3->track_skew0 = ((value >> 0) & 0xff);
		}
		if(q3->cylinder_skew1 || q3->cylinder_skew0) {
		    printstr("Enter the new cylinder skew? ");
		    value = getinteger();
		    p3->cylinder_skew1 = ((value >> 8) & 0xff);
		    p3->cylinder_skew0 = ((value >> 0) & 0xff);
		}
		if(q3->flags) {
		    printstr("Enter the new flags in (HEX)? ");
		    value = gethexnum();
		    p3->flags = value;
		}
		break;

	    case 4:					/* Page 4 */
		p4 = (struct page_code_4 *)bp_ms;
		q4 = (struct page_code_4 *)bp_msc;
		printf("\nPage 4 - rigid disk drive geometry parameters:\n");
		printf("  PS\t\t\t\t\t%d\n", p4->ps);
		printf("  Page code\t\t\t\t%d\n", p4->pgcode);
		printf("  Page length\t\t\t\t%d\n", p4->pglength);
		printf("  Maximum number of cylinders\t\t%d\n", 
				((p4->ncyl2<<16) +
					(p4->ncyl1<<8) +
						p4->ncyl0));
		printf("  Maximum number of heads\t\t%d\n", p4->nheads);
		printf("  Write precompensation start\t\t%d\n", 
				((p4->wprecomp2<<16) +
					(p4->wprecomp1<<8) +
						p4->wprecomp0));
		printf("  Reduced write current start\t\t%d\n", 
				((p4->rwc2<<16) +
					(p4->rwc1<<8) +
						p4->rwc0));
		printf("  Drive step rate\t\t\t%d\n", 
				((p4->dsr1<<8) + p4->dsr0));
		printf("  Landing zone cylinder\t\t\t%d\n", 
				((p4->lzc2<<16) +
					(p4->lzc1<<8) +
						p4->lzc0));
		printstr("\nDo you want to change any fields in this page (y/n)? ");
		if(!confirm())
		    break;
		if(q4->ncyl2 || 
			q4->ncyl1 || q4->ncyl0) {
		    printstr("Enter the number of cylinders? ");
		    value = getinteger();
		    p4->ncyl2 = ((value >> 16) & 0xff);
		    p4->ncyl1 = ((value >> 8) & 0xff);
		    p4->ncyl0 = ((value >> 0) & 0xff);
		}
		if(q4->nheads) {
		    printstr("Enter the number of heads? ");
		    value = getinteger();
		    p4->nheads = value;
		}
		if(q4->wprecomp2 || 
			q4->wprecomp1 || q4->wprecomp0) {
		    printstr("Enter the write precompensation start? ");
		    value = getinteger();
		    p4->wprecomp2 = ((value >> 16) & 0xff);
		    p4->wprecomp1 = ((value >> 8) & 0xff);
		    p4->wprecomp0 = ((value >> 0) & 0xff);
		}
		if(q4->rwc2 || 
			q4->rwc1 || q4->rwc0) {
		    printstr("Enter the reduced write current start? ");
		    value = getinteger();
		    p4->rwc2 = ((value >> 16) & 0xff);
		    p4->rwc1 = ((value >> 8) & 0xff);
		    p4->rwc0 = ((value >> 0) & 0xff);
		}
		if(q4->dsr1 || q4->dsr0) {
		    printstr("Enter the drive step rate? ");
		    value = getinteger();
		    p4->dsr1 = ((value >> 8) & 0xff);
		    p4->dsr0 = ((value >> 0) & 0xff);
		}
		if(q4->lzc1 || q4->lzc0) {
		    printstr("Enter the landing zone cylinder? ");
		    value = getinteger();
		    p4->lzc1 = ((value >> 8) & 0xff);
		    p4->lzc0 = ((value >> 0) & 0xff);
		}
		break;

	    case 5:					/* Page 5 */
		p5 = (struct page_code_5 *)bp_ms;
		q5 = (struct page_code_5 *)bp_msc;
		printf("\nPage 5 - flexible disk drive parameters:\n");
	    	printf("  PS\t\t\t\t\t%d\n", p5->ps);
		printf("  Page code\t\t\t\t%d\n", p5->pgcode);
		printf("  Page length\t\t\t\t%d\n", p5->pglength);
		printf("  Transfer rate\t\t\t\t%d\n",
			((p5->xfer_rate1<<8) +
				p5->xfer_rate0));
		printf("  Number of heads\t\t\t%d\n",  p5->num_heads);
		printf("  Sectors per track\t\t\t%d\n", p5->sec_per_trk);
		printf("  Data bytes per physical sector\t%d\n",
			((p5->db_per_physec1<<8) +
				p5->db_per_physec0));
		printf("  Number of Cylinders\t\t\t%d\n",
			((p5->num_cyl1<<8) +
				p5->num_cyl0));
		printf("  Starting write precompensation cyl\t%d\n",
			((p5->swpc1<<8) +
				p5->swpc0));
		printf("  Starting reduced write current cyl\t%d\n",
			((p5->srwcc1<<8) +
				p5->srwcc0));
		printf("  Drive step rate\t\t\t%d\n",
			((p5->drv_stp_rate1<<8) +
				p5->drv_stp_rate0));
		printf("  Drive step pulse width\t\t%d\n", p5->drv_sp_width);
		printf("  Head settle delay\t\t\t%d\n",
			((p5->hd_stl_del1<<8) +
				p5->hd_stl_del0));
		printf("  Motor on delay\t\t\t%d\n", p5->mtr_on_del);
		printf("  Motor off delay\t\t\t%d\n", p5->mtr_off_del);
		if(!moreoutput())
			return;
		printf("\n");
		printf("  Motor on (MO)\t\t\t\t%d\n", p5->mo);
		printf("  Start sector number (SSN)\t\t%d\n", p5->ssn);
		printf("  True ready (TRDY)\t\t\t%d\n", p5->trdy);
		printf("  Step pulses per cylinder (SPC)\t%d\n", p5->sp_cyl);
		printf("  Write precompensation level\t\t%d\n", p5->wpc_lvl);
		printf("  Head load delay\t\t\t%d\n", p5->hl_del);
		printf("  Head unload delay\t\t\t%d\n", p5->hul_del);
		printf("  Pin 2 definition\t\t\t%d\n", p5->p2_def);
		printf("  Pin 34 definition\t\t\t%d\n", p5->p34_def);
		printf("  Pin 1 definition\t\t\t%d\n", p5->p1_def);
		printf("  Pin 4 definition\t\t\t%d\n", p5->p4_def);
		printf("  Medium rotation rate\t\t\t%d\n",
			((p5->med_rr1<<8) +
				p5->med_rr0));
		printstr("\nDo you want to change any fields in this page (y/n)? ");
		if(!confirm())
		    break;
		if(q5->xfer_rate0 || q5->xfer_rate1) {
		    printstr("Enter the transfer rate? ");
		    value = getinteger();
		    p5->xfer_rate1 = ((value >> 8) & 0xff);
		    p5->xfer_rate0 = (value & 0xff);
		}
		if(q5->num_heads) {
		    printstr("Enter the number of heads? ");
		    p5->num_heads = getinteger() & 0xff;
		}
		if(q5->sec_per_trk) {
		     printstr("Enter the sectors per track? ");
		     p5->sec_per_trk = getinteger() & 0xff;
		}
		if(q5->db_per_physec0 || q5->db_per_physec1) {
		    printstr("Enter the data bytes per physical sector? ");
		    value = getinteger();
		    p5->db_per_physec1 = ((value >> 8) & 0xff);
		    p5->db_per_physec0 = (value & 0xff);
		}
		if(q5->num_cyl0 || q5->num_cyl1) {
		    printstr("Enter the number of cylinders? ");
		    value = getinteger();
		    p5->num_cyl1 = ((value >> 8) & 0xff);
		    p5->num_cyl0 = (value & 0xff);
		}
		if(q5->swpc0 || q5->swpc1) {
		    printstr("Enter the write precompensation start cyl? ");
		    value = getinteger();
		    p5->swpc1 = ((value >> 8) & 0xff);
		    p5->swpc0 = (value & 0xff);
		}
		if(q5->srwcc0 || q5->srwcc1) {
		    printstr("Enter the reduced write current start cyl? ");
		    value = getinteger();
		    p5->srwcc1 = ((value >> 8) & 0xff);
		    p5->srwcc0 = (value & 0xff);
		}
		if(q5->drv_stp_rate0 || q5->drv_stp_rate1) {
		    printstr("Enter the drive step rate? ");
		    value = getinteger();
		    p5->drv_stp_rate1 = ((value >> 8) & 0xff);
		    p5->drv_stp_rate0 = (value & 0xff);
		}
		if(q5->drv_sp_width) {
		    printstr("Enter the drive step pulse width? ");
		    p5->drv_sp_width = getinteger() & 0xff;
		}
		if(q5->hd_stl_del0 || q5->hd_stl_del1) {
		    printstr("Enter the head settle delay? ");
		    value = getinteger();
		    p5->hd_stl_del1 = ((value >> 8) & 0xff);
		    p5->hd_stl_del0 = (value & 0xff);
		}
		if(q5->mtr_on_del) {
		    printstr("Enter the motor on delay? ");
		    p5->mtr_on_del = getinteger() & 0xff;
		}
		if(q5->mtr_off_del) {
		    printstr("Enter the motor off delay? ");
		    p5->mtr_off_del = getinteger() & 0xff;
		}
		if(q5->mo) {
		    printstr("Enter the motor on (MO) bit? ");
		    p5->mo = getinteger() & 0x1;
		}
		if(q5->ssn) {
		    printstr("Enter the start sector number (SSN) bit? ");
		    p5->ssn = getinteger() & 0x1;
		}
		if(q5->trdy) {
		    printstr("Enter the true ready (TRDY) bit? ");
		    p5->trdy = getinteger() & 0x1;
		}
		if(q5->sp_cyl) {
		    printstr("Enter the step pulses per cylinder (SPC)? ");
		    p5->sp_cyl = getinteger() & 0xf;
		}
		if(q5->wpc_lvl) {
		    printstr("Enter the write precompensation level? ");
		    p5->wpc_lvl = getinteger() & 0xff;
		}
		if(q5->hl_del) {
		    printstr("Enter the head load delay? ");
		    p5->hl_del = getinteger() & 0xff;
		}
		if(q5->hul_del) {
		    printstr("Enter the head unload delay? ");
		    p5->hul_del = getinteger() & 0xff;
		}
		if(q5->p2_def) {
		    printstr("Enter the pin 2 definition? ");
		    p5->p2_def = getinteger() & 0xf;
		}
		if(q5->p34_def) {
		    printstr("Enter the pin 34 definition? ");
		    p5->p34_def = getinteger() & 0xf;
		}
		if(q5->p1_def) {
		    printstr("Enter the pin 1 definition? ");
		    p5->p1_def = getinteger() & 0xf;
		}
		if(q5->p4_def) {
		    printstr("Enter the pin 4 definition? ");
		    p5->p4_def = getinteger() & 0xf;
		}
		if(q5->med_rr0 || q5->med_rr1) {
		    printstr("Enter the medium rotation rate? ");
		    value = getinteger();
		    p5->med_rr1 = ((value >> 8) & 0xff);
		    p5->med_rr0 = (value & 0xff);
		}
		break;

	    case 8:					/* Page 8 */
		p8 = (struct page_code_8 *)bp_ms;
		q8 = (struct page_code_8 *)bp_msc;
    		printf("\nPage 8 - caching parameters:\n");
    		printf("  PS\t\t\t\t\t%d\n", p8->ps);
    		printf("  Page code\t\t\t\t%d\n", p8->pgcode);
    		printf("  Page length\t\t\t\t%d\n", p8->pglength);
		printf("  Read cache disable bit\t\t0x%x\n",
					p8->rc);
		printf("  Multiple selection bit\t\t0x%x\n",
					p8->ms);
		printf("  Write cache enable bit\t\t0x%x\n",
					p8->wce);
		printf("  Write retention priority\t\t%d\n",
					p8->wrp);
		printf("  Demand read retention priority\t%d\n",
					p8->drrp);
		printf("  Disable prefetch transfer length\t%d\n",
				((p8->dpftl1<<8) + p8->dpftl0));
		printf("  Minimum prefetch\t\t\t%d\n",
				((p8->minpf1<<8) + p8->minpf0));
		printf("  Maximum prefetch\t\t\t%d\n",
				((p8->maxpf1<<8) + p8->maxpf0));
		printf("  Maximum prefetch ceiling\t\t%d\n",
				((p8->maxpfc1<<8) + p8->maxpfc0));
		printstr("\nDo you want to change any fields in this page (y/n)? ");
		if(!confirm())
		    break;
		if(q8->rc) {
		    printstr("Set read cache disable (RC) bit (y/n)? ");
		    if(confirm())
			p8->rc = 1;
		    else
			p8->rc = 0;
		}
		if(q8->ms) {
		    printstr("Set mutiple selection (MS) bit (y/n)? ");
		    if(confirm())
			p8->ms = 1;
		    else
			p8->ms = 0;
		}
		if(q8->wce) {
		    printstr("Set write cache enable (WCE) bit (y/n)? ");
		    if(confirm())
			p8->wce = 1;
		    else
			p8->wce = 0;
		}
		if(q8->wrp) {
		    printstr("Enter the write retention priority? ");
		    value = getinteger();
		    p8->wrp = value;
		}
		if(q8->drrp) {
		    printstr("Enter the demand read retention priority? ");
		    value = getinteger();
		    p8->drrp = value;
		}
		if(q8->dpftl1 || q8->dpftl0) {
		    printstr("Enter disable pre-fetch transfer length? ");
		    value = getinteger();
		    p8->dpftl1 = ((value >> 8) & 0xff);
		    p8->dpftl0 = ((value >> 0) & 0xff);
		}
		if(q8->minpf1 || q8->minpf0) {
		    printstr("Enter minimum pre-fetch value? ");
		    value = getinteger();
		    p8->minpf1 = ((value >> 8) & 0xff);
		    p8->minpf0 = ((value >> 0) & 0xff);
		}
		if(q8->maxpf1 || q8->maxpf0) {
		    printstr("Enter maximum pre-fetch value? ");
		    value = getinteger();
		    p8->maxpf1 = ((value >> 8) & 0xff);
		    p8->maxpf0 = ((value >> 0) & 0xff);
		}
		if(q8->maxpfc1 || q8->maxpfc0) {
		    printstr("Enter maximum pre-fetch ceiling value? ");
		    value = getinteger();
		    p8->maxpfc1 = ((value >> 8) & 0xff);
		    p8->maxpfc0 = ((value >> 0) & 0xff);
		}
		break;

	    case 37:					/* Page 37 */
		p37 = (struct page_code_37 *)bp_ms;
		q37 = (struct page_code_37 *)bp_msc;
		printf("\nPage 37 - DEC unique parameters:\n");
		printf("  PS\t\t\t\t\t%d\n", p37->ps);
		printf("  Page code\t\t\t\t%d\n", p37->pgcode);
		printf("  Page length\t\t\t\t%d\n", p37->pglength);
		printf("  Spinup on power up\t\t\t%d\n", p37->spinup);
		printstr("\nDo you want to change any fields in this page (y/n)? ");
		if(!confirm())
		    break;
		printstr("Have drive spinup on power on (y/n)? ");
		if(confirm())
		    p37->spinup = 0;
		else
		    p37->spinup = 1;
		break;

	    default:
		/*
		 * TODO: would be nice to allow the user to
		 *	 change bytes in unknown pages.
		 */
		printf("\nPage %d - unknown or unsupported page:\n",
			pp_ms->pgcode);
		printf("  PS\t\t\t\t\t%d\n", pp_ms->ps);
		printf("  Page code\t\t\t\t%d\n", pp_ms->pgcode);
		printf("  Page length\t\t\t\t%d\n", pp_ms->pglength);
		ptr = bp_ms + sizeof (struct page_header);
		for(i = 0; i < pp_ms->pglength; i++) {
		    if(i && ((i % 16) == 0)) {
			if(!moreoutput())
			    break;
			printf("\n");
		    }
		    printf("  Byte %d\t\t\t\t0x%x\n", i, *ptr++ & 0xff);
		}
		break;

	    }	/* switch */

	    bp_ms += sizeof (struct page_header) + pp_ms->pglength;
	    bp_msc += sizeof (struct page_header) + pp_msc->pglength;

	}	/* while */
	/*
	 * Ask user which pages to save if there are
	 * any saveable pages for this device.
	 */
	if(num_sp == 0)
		setps = 0;
	else {
		printstr("\nSAVE THE CHANGED VALUES ON DISK (y/n)? ");
		if(confirm())
			setps = 1;
		else
			setps = 0;
	}

	msp.msp_setps = setps;
	msp.msp_length = ms.ms_hdr.sense_len + 1;
	ms.ms_hdr.sense_len = 0;
	ms.ms_hdr.medium_type = 0;
	ms.ms_hdr.wp = 0;
	/*
	 * For each data byte of each page and the block descriptor,
	 * and it with the same data byte in the changeable page.
	 * This makes sure we don't attempt to set any non-
	 * changeable bits to one (which would cause a check
	 * condition on the mode select).
	 */
	bp_ms = (char *)&ms;
	bp_ms += sizeof(ms.ms_hdr);		/* skip mode select header */
	bp_msc = (char *)&msc;
	bp_msc += sizeof(ms.ms_hdr);		/* skip mode select header */
	bp_ms += ms.ms_hdr.blk_des_len;		/* point to start of page 1 */
	bp_msc += msc.ms_hdr.blk_des_len;	/* point to start of page 1 */
	/*
	 * Make sure block descriptor is correct.
	 * Force LBN size to 512 bytes.
	 */
	if(ms.ms_hdr.blk_des_len == 8) {
	    ms.ms_desc.density_code &= msc.ms_desc.density_code;
	    ms.ms_desc.nblks2 &= msc.ms_desc.nblks2;
	    ms.ms_desc.nblks1 &= msc.ms_desc.nblks1;
	    ms.ms_desc.nblks0 &= msc.ms_desc.nblks0;
	    ms.ms_desc.blklen2 = ((512 >> 16) & 0xff);
	    ms.ms_desc.blklen1 = ((512 >> 8) & 0xff);
	    ms.ms_desc.blklen0 = (512 & 0xff);
	}
	while(1) {
		pp_ms = (struct page_header *)bp_ms;
		pp_msc = (struct page_header *)bp_msc;
		if ((pp_ms->pgcode == 0) && (pp_ms->pglength == 0))
			break;
		if (pp_ms->pgcode != pp_msc->pgcode) {
		    printf("change_drive_parameters: %s\n",
			"ms.pgcode does not match msc.pgcode!");
		    return;
		}
		bp_ms += sizeof (struct page_header);
		bp_msc += sizeof (struct page_header);
		/*
		 * If the page has zero length or all the data
		 * bytes are zero, then the page is not changeable.
		 * So don't send it (RRD40 fails if we send page 2).
		 * The RRD40 says page 3 is changeable but its not,
		 * so we zap the bytes per physical sector field.
		 */
		sum=0;
		if(pp_msc->pglength) {
			ptr = (char *)bp_msc;
			if((strncmp(prodid,"RRD40",5) == 0) &&
			   (pp_msc->pgcode == 3)) {
				q3 = (struct page_code_3 *)(bp_msc -
					sizeof(struct page_header));
				q3->bps1 = 0;
				q3->bps0 = 0;
			}
			for(i = 0; i < pp_msc->pglength; i++)
				sum += *ptr++;
		}
		if(pp_msc->pglength && sum) {
			for(i = 0; i < pp_ms->pglength; i++)
				*bp_ms++ &= *bp_msc++;
		}
		else {
			bp_msc += pp_msc->pglength;	/* could be zero */
			bp_ms -= sizeof(struct page_header);
			msp.msp_length -= sizeof(struct page_header);
			msp.msp_length -= pp_ms->pglength;
			p = bp_ms;
			ptr = bp_ms + sizeof(struct page_header) +
				pp_ms->pglength;
			while(ptr < ((char *)&ms + sizeof(ms)))
				*p++ = *ptr++;
			while(p < ((char *)&ms + sizeof(ms)))
				*p++ = 0;
		}
	}
	/*
	 * Change the disk drive parameters to their CHANGED values.
	 */
	printf("\nChanging disk drive parameters for device (%s).\n",
		rzdisk);
	if(execute_rzcmd(SCSI_MODE_SELECT, (char *)&msp) != SUCCESS) {
		if(geterror() != NO_ERROR)
			return;
	}
}

get_inquiry_info()
{
    
    char hold[20];
    int i;

	printf("\nGetting inquiry data info from device (%s).\n", rzdisk);
	bzero((char *)&inq, sizeof(inq));
	msp.msp_addr = (caddr_t)&inq;
	msp.msp_length = sizeof(inq);
	if(execute_rzcmd(SCSI_GET_INQUIRY_DATA, (char *)&msp) != SUCCESS) {
		if(geterror() != NO_ERROR)
			return;
	}
    	printf("\nDump of Inquiry Data:\n");
    	printf("  Peripheral Device Type\t\t0x%x\n",inq.perfdt);
    	printf("  Device Type Qualifier\t\t\t0x%x\n",inq.devtq);
    	printf("  Removable Medium\t\t\t0x%x\n",inq.rmb);
    	printf("  Version\t\t\t\t0x%x\n",inq.version);
    	for(i=0; i<8; i++)
		hold[i] = inq.vndrid[i];
    	hold[i] = '\0';
    	printf("  Vendor Identificaton\t\t\t%s\n", hold);
    	for(i=0; i<16; i++)
		hold[i] = inq.prodid[i];
    	hold[i] = '\0';
    	printf("  Product Identification\t\t%s\n", hold);
    	for(i=0; i<4; i++)
		hold[i] = inq.revlvl[i];
    	hold[i] = '\0';
    	printf("  Revision Level\t\t\t%s\n", hold);
}

get_drive_parameters(page_control)
{

	char strtype[20];
	int i;
	char *ptr;
	struct page_header *pp_ms;
	char *bp_ms;
	struct page_code_1 *p1;
	struct page_code_2 *p2;
	struct page_code_3 *p3;
	struct page_code_4 *p4;
	struct page_code_5 *p5;
	struct page_code_8 *p8;
	struct page_code_37 *p37;

	bzero((char *)&ms, sizeof(ms));
	msp.msp_pgcode = 0x3f;
	msp.msp_length = sizeof(ms);
	msp.msp_pgctrl = page_control;
	msp.msp_addr = (caddr_t)&ms;
	/*
	 * Get the CURRENT, DEFAULT, SAVED, or CHANGEABLE disk drive
	 * parameters based on the page control value.
	 */
	if(page_control == CURRENT_VALUES)
	    strcpy(strtype,"CURRENT");
	else if(page_control == DEFAULT_VALUES)
	    strcpy(strtype,"DEFAULT");
	else if(page_control == SAVED_VALUES)
	    strcpy(strtype,"SAVED");
	else if(page_control == CHANGED_VALUES)
	    strcpy(strtype,"CHANGEABLE");
	printf("\nGetting %s disk drive parameters from device (%s).\n",
			strtype,rzdisk);
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&msp) != SUCCESS) {
		if(geterror() != NO_ERROR)
			return;
	}
	/*
	 * Display the parameters to the user. All parameters are
	 * displayed in (Decimal), all flags in (Hex). If the page
	 * code is CHANGEABLE then all fields are displayed in (Hex)
	 * to show the bit pattern of all the bits that can be changed.
	 */
	printf("\nMode sense header:\n");
	printf("  Sense data length\t\t\t%d\n", ms.ms_hdr.sense_len);
	printf("  Medium type\t\t\t\t%d\n", ms.ms_hdr.medium_type);
	printf("  WP\t\t\t\t\t0x%x\n", ms.ms_hdr.wp);
	printf("  Block descriptor length\t\t%d\n", ms.ms_hdr.blk_des_len);

	printf("\nBlock descriptor:\n");
	printf("  Density code\t\t\t\t%d\n", ms.ms_desc.density_code);
	printf("  Number of blocks\t\t\t%d\n", 
			((ms.ms_desc.nblks2<<16) + 
				(ms.ms_desc.nblks1<<8) + ms.ms_desc.nblks0));
	if(page_control == CHANGED_VALUES && ms.ms_desc.blklen2 == 0xff &&
		ms.ms_desc.blklen1 == 0xff && ms.ms_desc.blklen0 == 0xff) {
		printf("  Block length\t\t\t\t0x%x\n", 
			((ms.ms_desc.blklen2<<16) + 
				(ms.ms_desc.blklen1<<8) + ms.ms_desc.blklen0));
	}
	else {
		printf("  Block length\t\t\t\t%d\n", 
			((ms.ms_desc.blklen2<<16) + 
				(ms.ms_desc.blklen1<<8) + ms.ms_desc.blklen0));
	}
	bp_ms = (char *)&ms;
	bp_ms += sizeof(ms.ms_hdr);		/* skip mode select header */
	bp_ms += ms.ms_hdr.blk_des_len;		/* point to start of page 1 */
	while(1) {
	    pp_ms = (struct page_header *)bp_ms;
	    if ((pp_ms->pgcode == 0) && (pp_ms->pglength == 0))
		break;
	    if (pp_ms->pglength == 0) {	/* no data bytes in this page */
		bp_ms += sizeof(struct page_header);
		continue;
	    }
	    switch(pp_ms->pgcode) {
	    case 1:					/* Page 1 */
		p1 = (struct page_code_1 *)bp_ms;

		if(!moreoutput())
			return;
	    	printf("\nPage 1 - error recovery parameters:\n");
	    	printf("  PS\t\t\t\t\t%d\n", p1->ps);
	    	printf("  Page code\t\t\t\t%d\n", p1->pgcode);
	    	printf("  Page length\t\t\t\t%d\n", p1->pglength);
		if(page_control == CHANGED_VALUES) {
	    		printf("  Flags\t\t\t\t\t0x%02x\n", p1->flags);
	    		printf("  Retry count\t\t\t\t0x%x\n", 
					p1->retry_count);
	    		printf("  Correction span\t\t\t0x%x\n", 
					p1->correct_span);
	    		printf("  Head offset count\t\t\t0x%x\n", 
					p1->head_offset);
	    		printf("  Data strobe offset count\t\t0x%x\n", 
					p1->data_strobe);
	    		printf("  Recovery time limit\t\t\t0x%x\n", 
					p1->recovery_time);
		}
		else {
	    		printf("  Flags\t\t\t\t\t0x%02x\n", p1->flags);
	    		printf("  Retry count\t\t\t\t%d\n", 
					p1->retry_count);
	    		printf("  Correction span\t\t\t%d\n", 
					p1->correct_span);
	    		printf("  Head offset count\t\t\t%d\n", 
					p1->head_offset);
	    		printf("  Data strobe offset count\t\t%d\n", 
					p1->data_strobe);
	    		printf("  Recovery time limit\t\t\t%d\n", 
					p1->recovery_time);
		}
		break;
	
	    case 2:					/* Page 2 */
		p2 = (struct page_code_2 *)bp_ms;

		if(!moreoutput())
			return;
		printf("\nPage 2 - disconnect/reconnect parameters:\n");
		printf("  PS\t\t\t\t\t%d\n", p2->ps);
		printf("  Page code\t\t\t\t%d\n", p2->pgcode);
		printf("  Page length\t\t\t\t%d\n", p2->pglength);
		if(page_control == CHANGED_VALUES) {
			printf("  Buffer full ratio\t\t\t0x%x\n", 
				p2->bus_fratio);
			printf("  Buffer empty ratio\t\t\t0x%x\n", 
				p2->bus_eratio);
			printf("  Bus inactivity limit\t\t\t0x%x\n", 
				((p2->bus_inactive1<<8) +
					p2->bus_inactive0));
			printf("  Disconnect time limit\t\t\t0x%x\n",
				((p2->disconn_time1<<8) +
					p2->disconn_time0));
			printf("  Connect time limit\t\t\t0x%x\n", 
				((p2->conn_time1<<8) +
					p2->conn_time0));
		}
		else {
			printf("  Buffer full ratio\t\t\t%d\n", 
				p2->bus_fratio);
			printf("  Buffer empty ratio\t\t\t%d\n", 
				p2->bus_eratio);
			printf("  Bus inactivity limit\t\t\t%d\n", 
				((p2->bus_inactive1<<8) +
					p2->bus_inactive0));
			printf("  Disconnect time limit\t\t\t%d\n",
				((p2->disconn_time1<<8) +
					p2->disconn_time0));
			printf("  Connect time limit\t\t\t%d\n", 
				((p2->conn_time1<<8) +
					p2->conn_time0));
		}
		break;

	    case 3:					/* Page 3 */
		p3 = (struct page_code_3 *)bp_ms;

		if(!moreoutput())
			return;
		printf("\nPage 3 - direct-access device format parameters:\n");
		printf("  PS\t\t\t\t\t%d\n", p3->ps);
		printf("  Page code\t\t\t\t%d\n", p3->pgcode);
		printf("  Page length\t\t\t\t%d\n", p3->pglength);
		if(page_control == CHANGED_VALUES) {
			printf("  Tracks per zone\t\t\t0x%x\n", 
				((p3->tpz1<<8) + p3->tpz0));
			printf("  Alternate sectors per zone\t\t0x%x\n", 
				((p3->aspz1<<8) + p3->aspz0));
			printf("  Alternate tracks per zone\t\t0x%x\n", 
				((p3->atpz1<<8) + p3->atpz0));
			printf("  Alternate tracks per volume\t\t0x%x\n", 
				((p3->atpv1<<8) + p3->atpv0));
			printf("  Sectors per track\t\t\t0x%x\n", 
				((p3->spt1<<8) + p3->spt0));
			printf("  Data bytes per phy sector\t\t0x%x\n", 
				((p3->bps1<<8) + p3->bps0));
			printf("  Interleave\t\t\t\t0x%x\n", 
				((p3->interleave1<<8) + 
					p3->interleave0));
			printf("  Track skew\t\t\t\t0x%x\n", 
				((p3->track_skew1<<8) + 
					p3->track_skew0));
			printf("  Cylinder skew\t\t\t\t0x%x\n", 
				((p3->cylinder_skew1<<8) + 
					p3->cylinder_skew0));
			printf("  Flags\t\t\t\t\t0x%x\n", p3->flags);
		}
		else {
			printf("  Tracks per zone\t\t\t%d\n", 
				((p3->tpz1<<8) + p3->tpz0));
			printf("  Alternate sectors per zone\t\t%d\n", 
				((p3->aspz1<<8) + p3->aspz0));
			printf("  Alternate tracks per zone\t\t%d\n", 
				((p3->atpz1<<8) + p3->atpz0));
			printf("  Alternate tracks per volume\t\t%d\n", 
				((p3->atpv1<<8) + p3->atpv0));
			printf("  Sectors per track\t\t\t%d\n", 
				((p3->spt1<<8) + p3->spt0));
			printf("  Data bytes per phy sector\t\t%d\n", 
				((p3->bps1<<8) + p3->bps0));
			printf("  Interleave\t\t\t\t%d\n", 
				((p3->interleave1<<8) + 
					p3->interleave0));
			printf("  Track skew\t\t\t\t%d\n", 
				((p3->track_skew1<<8) + 
					p3->track_skew0));
			printf("  Cylinder skew\t\t\t\t%d\n", 
				((p3->cylinder_skew1<<8) + 
					p3->cylinder_skew0));
			printf("  Flags\t\t\t\t\t0x%x\n", p3->flags);
		}
		break;
	
	    case 4:					/* Page 4 */
		p4 = (struct page_code_4 *)bp_ms;

		if(!moreoutput())
			return;
		printf("\nPage 4 - rigid disk drive geometry parameters:\n");
		printf("  PS\t\t\t\t\t%d\n", p4->ps);
		printf("  Page code\t\t\t\t%d\n", p4->pgcode);
		printf("  Page length\t\t\t\t%d\n", p4->pglength);
		if(page_control == CHANGED_VALUES) {
			printf("  Maximum number of cylinders\t\t0x%x\n", 
				((p4->ncyl2<<16) +
					(p4->ncyl1<<8) +
						p4->ncyl0));
			printf("  Maximum number of heads\t\t0x%x\n", 
				p4->nheads);
			printf("  Write precompensation start\t\t0x%x\n", 
				((p4->wprecomp2<<16) +
					(p4->wprecomp1<<8) +
						p4->wprecomp0));
			printf("  Reduced write current start\t\t0x%x\n", 
				((p4->rwc2<<16) +
					(p4->rwc1<<8) +
						p4->rwc0));
			printf("  Drive step rate\t\t\t0x%x\n", 
				((p4->dsr1<<8) + p4->dsr0));
			printf("  Landing zone cylinder\t\t\t0x%x\n", 
				((p4->lzc2<<16) +
					(p4->lzc1<<8) +
						p4->lzc0));
		}
		else {
			printf("  Maximum number of cylinders\t\t%d\n", 
				((p4->ncyl2<<16) +
					(p4->ncyl1<<8) +
						p4->ncyl0));
			printf("  Maximum number of heads\t\t%d\n", 
				p4->nheads);
			printf("  Write precompensation start\t\t%d\n", 
				((p4->wprecomp2<<16) +
					(p4->wprecomp1<<8) +
						p4->wprecomp0));
			printf("  Reduced write current start\t\t%d\n", 
				((p4->rwc2<<16) +
					(p4->rwc1<<8) +
						p4->rwc0));
			printf("  Drive step rate\t\t\t%d\n", 
				((p4->dsr1<<8) + p4->dsr0));
			printf("  Landing zone cylinder\t\t\t%d\n", 
				((p4->lzc2<<16) +
					(p4->lzc1<<8) +
						p4->lzc0));
		}
		break;
	
	    case 5:					/* Page 5 */
		p5 = (struct page_code_5 *)bp_ms;

		if(!moreoutput())
			return;
		printf("\nPage 5 - flexible disk drive parameters:\n");
	    	printf("  PS\t\t\t\t\t%d\n", p5->ps);
		printf("  Page code\t\t\t\t%d\n", p5->pgcode);
		printf("  Page length\t\t\t\t%d\n", p5->pglength);
		if(page_control == CHANGED_VALUES) {
			printf("  Transfer rate\t\t\t\t0x%x\n",
				((p5->xfer_rate1<<8) +
					p5->xfer_rate0));
			printf("  Number of heads\t\t\t0x%x\n", 
				p5->num_heads);
			printf("  Sectors per track\t\t\t0x%x\n",
				p5->sec_per_trk);
			printf("  Data bytes per physical sector\t0x%x\n",
				((p5->db_per_physec1<<8) +
					p5->db_per_physec0));
			printf("  Number of Cylinders\t\t\t0x%x\n",
				((p5->num_cyl1<<8) +
					p5->num_cyl0));
			printf("  Starting write precompensation cyl\t0x%x\n",
				((p5->swpc1<<8) +
					p5->swpc0));
			printf("  Starting reduced write current cyl\t0x%x\n",
				((p5->srwcc1<<8) +
					p5->srwcc0));
			printf("  Drive step rate\t\t\t0x%x\n",
				((p5->drv_stp_rate1<<8) +
					p5->drv_stp_rate0));
			printf("  Drive step pulse width\t\t0x%x\n",
				p5->drv_sp_width);
			printf("  Head settle delay\t\t\t0x%x\n",
				((p5->hd_stl_del1<<8) +
					p5->hd_stl_del0));
			printf("  Motor on delay\t\t\t0x%x\n",
				p5->mtr_on_del);
			printf("  Motor off delay\t\t\t0x%x\n",
				p5->mtr_off_del);
			if(!moreoutput())
				return;
			printf("\n");
			printf("  Motor on (MO)\t\t\t\t0x%x\n",
				p5->mo);
			printf("  Start sector number (SSN)\t\t0x%x\n",
				p5->ssn);
			printf("  True ready (TRDY)\t\t\t0x%x\n",
				p5->trdy);
			printf("  Step pulses per cylinder (SPC)\t0x%x\n",
				p5->sp_cyl);
			printf("  Write precompensation level\t\t0x%x\n",
				p5->wpc_lvl);
			printf("  Head load delay\t\t\t0x%x\n",
				p5->hl_del);
			printf("  Head unload delay\t\t\t0x%x\n",
				p5->hul_del);
			printf("  Pin 2 definition\t\t\t0x%x\n",
				p5->p2_def);
			printf("  Pin 34 definition\t\t\t0x%x\n",
				p5->p34_def);
			printf("  Pin 1 definition\t\t\t0x%x\n",
				p5->p1_def);
			printf("  Pin 4 definition\t\t\t0x%x\n",
				p5->p4_def);
			printf("  Medium rotation rate\t\t\t0x%x\n",
				((p5->med_rr1<<8) +
					p5->med_rr0));
		}
		else {
			printf("  Transfer rate\t\t\t\t%d\n",
				((p5->xfer_rate1<<8) +
					p5->xfer_rate0));
			printf("  Number of heads\t\t\t%d\n", 
				p5->num_heads);
			printf("  Sectors per track\t\t\t%d\n",
				p5->sec_per_trk);
			printf("  Data bytes per physical sector\t%d\n",
				((p5->db_per_physec1<<8) +
					p5->db_per_physec0));
			printf("  Number of Cylinders\t\t\t%d\n",
				((p5->num_cyl1<<8) +
					p5->num_cyl0));
			printf("  Starting write precompensation cyl\t%d\n",
				((p5->swpc1<<8) +
					p5->swpc0));
			printf("  Starting reduced write current cyl\t%d\n",
				((p5->srwcc1<<8) +
					p5->srwcc0));
			printf("  Drive step rate\t\t\t%d\n",
				((p5->drv_stp_rate1<<8) +
					p5->drv_stp_rate0));
			printf("  Drive step pulse width\t\t%d\n",
				p5->drv_sp_width);
			printf("  Head settle delay\t\t\t%d\n",
				((p5->hd_stl_del1<<8) +
					p5->hd_stl_del0));
			printf("  Motor on delay\t\t\t%d\n",
				p5->mtr_on_del);
			printf("  Motor off delay\t\t\t%d\n",
				p5->mtr_off_del);
			if(!moreoutput())
				return;
			printf("\n");
			printf("  Motor on (MO)\t\t\t\t%d\n",
				p5->mo);
			printf("  Start sector number (SSN)\t\t%d\n",
				p5->ssn);
			printf("  True ready (TRDY)\t\t\t%d\n",
				p5->trdy);
			printf("  Step pulses per cylinder (SPC)\t%d\n",
				p5->sp_cyl);
			printf("  Write precompensation level\t\t%d\n",
				p5->wpc_lvl);
			printf("  Head load delay\t\t\t%d\n",
				p5->hl_del);
			printf("  Head unload delay\t\t\t%d\n",
				p5->hul_del);
			printf("  Pin 2 definition\t\t\t%d\n",
				p5->p2_def);
			printf("  Pin 34 definition\t\t\t%d\n",
				p5->p34_def);
			printf("  Pin 1 definition\t\t\t%d\n",
				p5->p1_def);
			printf("  Pin 4 definition\t\t\t%d\n",
				p5->p4_def);
			printf("  Medium rotation rate\t\t\t%d\n",
				((p5->med_rr1<<8) +
					p5->med_rr0));
		}
		break;
	
	    case 8:					/* Page 8 */
		p8 = (struct page_code_8 *)bp_ms;

		if(!moreoutput())
			return;
	    	printf("\nPage 8 - caching parameters:\n");
	    	printf("  PS\t\t\t\t\t%d\n", p8->ps);
	    	printf("  Page code\t\t\t\t%d\n", p8->pgcode);
	    	printf("  Page length\t\t\t\t%d\n", p8->pglength);
		if(page_control == CHANGED_VALUES) {
			printf("  Read cache disable bit\t\t0x%x\n",
						p8->rc);
			printf("  Multiple selection bit\t\t0x%x\n",
						p8->ms);
			printf("  Write cache enable bit\t\t0x%x\n",
						p8->wce);
			printf("  Write retention priority\t\t0x%x\n",
						p8->wrp);
			printf("  Demand read retention priority\t0x%x\n",
						p8->drrp);
			printf("  Disable prefetch transfer length\t0x%x\n",
				((p8->dpftl1<<8) + p8->dpftl0));
			printf("  Minimum prefetch\t\t\t0x%x\n",
				((p8->minpf1<<8) + p8->minpf0));
			printf("  Maximum prefetch\t\t\t0x%x\n",
				((p8->maxpf1<<8) + p8->maxpf0));
			printf("  Maximum prefetch ceiling\t\t0x%x\n",
				((p8->maxpfc1<<8) + p8->maxpfc0));
		}
		else {
			printf("  Read cache disable bit\t\t0x%x\n",
						p8->rc);
			printf("  Multiple selection bit\t\t0x%x\n",
						p8->ms);
			printf("  Write cache enable bit\t\t0x%x\n",
						p8->wce);
			printf("  Write retention priority\t\t%d\n",
						p8->wrp);
			printf("  Demand read retention priority\t%d\n",
						p8->drrp);
			printf("  Disable prefetch transfer length\t%d\n",
				((p8->dpftl1<<8) + p8->dpftl0));
			printf("  Minimum prefetch\t\t\t%d\n",
				((p8->minpf1<<8) + p8->minpf0));
			printf("  Maximum prefetch\t\t\t%d\n",
				((p8->maxpf1<<8) + p8->maxpf0));
			printf("  Maximum prefetch ceiling\t\t%d\n",
				((p8->maxpfc1<<8) + p8->maxpfc0));
		}
		break;

	    case 37:					/* Page 37 */
		p37 = (struct page_code_37 *)bp_ms;

		if(!moreoutput())
			return;
		printf("\nPage 37 - DEC unique parameters:\n");
		printf("  PS\t\t\t\t\t%d\n", p37->ps);
		printf("  Page code\t\t\t\t%d\n", p37->pgcode);
		printf("  Page length\t\t\t\t%d\n", p37->pglength);
		if(page_control == CHANGED_VALUES) {
			printf("  Spinup on power up\t\t\t0x%x\n", 
				p37->spinup);
		}
		else {
			printf("  Spinup on power up\t\t\t%d\n", 
				p37->spinup);
		}
		break;

	    default:
		if(!moreoutput())
			return;
		printf("\nPage %d - unknown or unsupported page:\n",
			pp_ms->pgcode);
		printf("  PS\t\t\t\t\t%d\n", pp_ms->ps);
		printf("  Page code\t\t\t\t%d\n", pp_ms->pgcode);
		printf("  Page length\t\t\t\t%d\n", pp_ms->pglength);
		ptr = bp_ms + sizeof (struct page_header);
		for(i = 0; i < pp_ms->pglength; i++) {
		    if(i && ((i % 16) == 0)) {
			if(!moreoutput())
			    break;
			printf("\n");
		    }
		    printf("  Byte %d\t\t\t\t0x%x\n", i, *ptr++ & 0xff);
		}
		break;

	    }	/* switch */

	    bp_ms += sizeof (struct page_header) + pp_ms->pglength;

	}	/* while */
}

execute_rzcmd(command, data)
int command;
char *data;
{

	int i;

	rzcom = command;
	if(ioctl(rzdev, command, data) == -1)
	    return(ERROR);
	else
	    return(SUCCESS);
}

char *rz_sense_key_str[] = {
	"no sense",			/* 0x00 */
	"recovered error",		/* 0x01 */
	"not ready",			/* 0x02 */
	"medium error",			/* 0x03 */
	"hardware error",		/* 0x04 */
	"illegal request",		/* 0x05 */
	"unit attention",		/* 0x06 */
	"data protect",			/* 0x07 */
	"blank check",			/* 0x08 */
	"vendor specific",		/* 0x09 */
	"copy aborted",			/* 0x0a */
	"aborted command",		/* 0x0b */
	"equal",			/* 0x0c */
	"volume overflow",		/* 0x0d */
	"miscompare",			/* 0x0e */
	"reserved",			/* 0x0f */
	0
};

#define MAX_SENSE_KEY_STR \
	(sizeof(rz_sense_key_str)/sizeof(rz_sense_key_str[0]))

char *rz_error_code_str[] = {
	"no additional sense information",			/* 0x00 */
	"no index/sector signal",				/* 0x01 */
	"no seek complete",					/* 0x02 */
	"peripheral device write fault",			/* 0x03 */
	"logical unit not ready, cause not reportable",		/* 0x04 */
	"logical unit does not respond to selection",		/* 0x05 */
	"no reference position found",				/* 0x06 */
	"multiple peripheral devices selected",			/* 0x07 */
	"logical unit communication failure",			/* 0x08 */
	"track following error",				/* 0x09 */
	"error log overflow",					/* 0x0a */
	"",							/* 0x0b */
	"write error",						/* 0x0c */
	"",							/* 0x0d */
	"",							/* 0x0e */
	"",							/* 0x0f */
	"ID CRC or ECC error",					/* 0x10 */
	"unrecovered read error",				/* 0x11 */
	"address mark not found in id field",			/* 0x12 */
	"address mark not found in data field",			/* 0x13 */
	"recorded entry not found",				/* 0x14 */
	"random positioning error",				/* 0x15 */
	"data synchronization mark error",			/* 0x16 */
	"recovered data with retries",				/* 0x17 */
	"recovered data with error correction applied",		/* 0x18 */
	"defect list error",					/* 0x19 */
	"parameter list length error",				/* 0x1a */
	"synchronous data transfer error",			/* 0x1b */
	"defect list not found",				/* 0x1c */
	"miscompare during verify operation",			/* 0x1d */
	"recovered id with ecc correction",			/* 0x1e */
	"",							/* 0x1f */
	"invalid command operation mode",			/* 0x20 */
	"logical block address out of range",			/* 0x21 */
	"illegal function for device type",			/* 0x22 */
	"",							/* 0x23 */
	"invalid field in cdb",					/* 0x24 */
	"logical unit not supported",				/* 0x25 */
	"invalid field in parameter list",			/* 0x26 */
	"write protected",					/* 0x27 */
	"not ready to ready transition",			/* 0x28 */
	"power on, reset, or bus device reset occurred",	/* 0x29 */
	"parameters changed",					/* 0x2a */
	"copy cannot execute since host cannot disconnect",	/* 0x2b */
	"command sequence error",				/* 0x2c */
	"overwrite error on update in place",			/* 0x2d */
	"",							/* 0x2e */
	"tagged commands cleared by another initiator",		/* 0x2f */
	"incompatible medium installed",			/* 0x30 */
	"medium format corrupted",				/* 0x31 */
	"no defect spare location available",			/* 0x32 */
	"tape length error",					/* 0x33 */
	"",							/* 0x34 */
	"",							/* 0x35 */
	"ribbon/ink/toner failure",				/* 0x36 */
	"rounded parameter",					/* 0x37 */
	"sequential positioning error",				/* 0x38 */
	"saving parameters not supported",			/* 0x39 */
	"medium not present",					/* 0x3a */
	"",							/* 0x3b */
	"",							/* 0x3c */
	"invalid bits in identify message",			/* 0x3d */
	"logical unit has not self-configured yet",		/* 0x3e */
	"target operating conditions have changed",		/* 0x3f */
	"ram failure",						/* 0x40 */
	"data path failure",					/* 0x41 */
	"power-on or self-test failure",			/* 0x42 */
	"message error",					/* 0x43 */
	"internal target failure",				/* 0x44 */
	"select/reselect failure",				/* 0x45 */
	"unsuccessful soft reset",				/* 0x46 */
	"scsi parity error",					/* 0x47 */
	"initiator detected error message received",		/* 0x48 */
	"invalid message error",				/* 0x49 */
	"command phase error",					/* 0x4a */
	"data phase error",					/* 0x4b */
	"logical unit failed self-configuration",		/* 0x4c */
	"",							/* 0x4d */
	"overlapped commands attemtped",			/* 0x4e */
	"",							/* 0x4f */
	"write append error",					/* 0x50 */
	"erase failure",					/* 0x51 */
	"cartridge fault",					/* 0x52 */
	"media load/eject failed",				/* 0x53 */
	"scsi to host system interface failure",		/* 0x54 */
	"system resource failure",				/* 0x55 */
	"",							/* 0x56 */
	"unable to recover table-of-contents",			/* 0x57 */
	"generation does not exist",				/* 0x58 */
	"updated block read",					/* 0x59 */
	"operator request or state change input (unspec)",	/* 0x5a */
	"log exception",					/* 0x5b */
	"rpl status change",					/* 0x5c */
	"",							/* 0x5d */
	"",							/* 0x5e */
	"",							/* 0x5f */
	"lamp failure",						/* 0x60 */
	"video acquisition error",				/* 0x61 */
	"scan head positioning error",				/* 0x62 */
	"end of user area encountered on this track",		/* 0x63 */
	"illegal mode for this track",				/* 0x64 */
	0
};

#define MAX_ERROR_CODE_STR \
	(sizeof(rz_error_code_str)/sizeof(rz_error_code_str[0]))


char *unknown_key = "unknown key";
char *unknown_code = "unknown code";


geterror()
{

	struct extended_sense sense;
	char *print_sense_key();
	char *print_error_code();
	u_char *p;
	char rzcomstr[40];
	int i, sum;

	rzcomstr[0] = NULL;
	switch(rzcom) {
	case SCSI_FORMAT_UNIT:
		strcat(rzcomstr,"Format Unit");
		break;
	case SCSI_READ_DEFECT_DATA:
		strcat(rzcomstr,"Read Defect Data");
		break;
	case SCSI_REASSIGN_BLOCK:
		strcat(rzcomstr,"Reassign Block");
		break;
	case SCSI_VERIFY_DATA:
		strcat(rzcomstr,"Verify Data");
		break;
	case SCSI_MODE_SENSE:
		strcat(rzcomstr,"Mode Sense");
		break;
	case SCSI_MODE_SELECT:
		strcat(rzcomstr,"Mode Select");
		break;
	case SCSI_GET_INQUIRY_DATA:
		strcat(rzcomstr,"Inquiry");
		break;
	case DIOCGETPT:
		strcat(rzcomstr,"Get Partition Table");
		break;
	default:
		strcat(rzcomstr,"Unknown Command");
		break;
	}

	if(errno != EIO) {
	    printf("\n*** ERROR OCCURRED DURING COMMAND (%s) ***\n\n",
			rzcomstr);
	    perror(rzdisk);
	    return(FATAL_ERROR);
	}

	if(ioctl(rzdev, SCSI_GET_SENSE, (char *)&sense) == -1) {
	    printf("\n*** ERROR OCCURRED DURING COMMAND (%s) ***\n\n",
			rzcomstr);
	    printf("\nCannot get sense info from device (%s).\n",rzdisk);
	    return(FATAL_ERROR);
	}

	/*
	 * So we don't get fooled by ne sense data returned.
	 */
	p = (u_char *)&sense;
	sum = 0;
	for(i=0; i<sizeof(sense); i++)
	    sum += *p++;
	if(sum == 0) {
	    printf("\n*** ERROR OCCURRED DURING COMMAND (%s) ***\n\n",
			rzcomstr);
	    printf("\nCannot get sense info from device (%s).\n",rzdisk);
	    return(FATAL_ERROR);
	}

	if(sense.snskey == SC_NOSENSE)
		return(NO_ERROR);

	/*
	 * We need to klnow about unit attention
	 * when formatting a floppy.
	 */
	if((rzcom == SCSI_FORMAT_UNIT) && (sense.snskey == SC_UNITATTEN))
		return(ATTN_ERROR);

	infobyte =  ((sense.infobyte3 << 24) & 0xff000000);
	infobyte += ((sense.infobyte2 << 16) & 0xff0000);
	infobyte += ((sense.infobyte1 << 8) & 0xff00);
	infobyte += ((sense.infobyte0 << 0) & 0xff);
	if(rzcom == SCSI_VERIFY_DATA) {
	    	if(sense.snskey == SC_RECOVERR) {
		    asc = sense.asc;
		    return(SOFT_ERROR);
		}
	    	else if(sense.snskey == SC_MEDIUMERR) {
		    asc = sense.asc;
		    return(HARD_ERROR);
		}
	}

	printf("\n*** ERROR OCCURRED on %s DURING COMMAND (%s) ***\n\n",
	    rzdisk, rzcomstr);
	printf("Sense Key (0x%x): %s.\n",
		sense.snskey, print_sense_key(sense.snskey));
	printf("Additional Sense Code (0x%x): %s.\n",
		sense.asc, print_error_code(sense.asc));
	printf("Sense Data: ");
	p = (u_char *)&sense;
	for(i=0; i<sizeof(sense); i++)
	    printf("%x ", *p++);
	printf("\n");
	return(FATAL_ERROR);
}

char *print_sense_key(key)
int key;
{

	if((key > MAX_SENSE_KEY_STR -1) || rz_sense_key_str[key] == NULL)
		return(unknown_key);
	else
		return(rz_sense_key_str[key]);
}

char *print_error_code(code)
int code;
{

	if((code > MAX_ERROR_CODE_STR -1) || rz_error_code_str[code] == NULL)
		return(unknown_code);
	else
		return(rz_error_code_str[code]);
}

moreoutput()
{
    	char buf[8];

    	printstr("\nMore output (y/n)? ");
	if(confirm())
		return(1);
	else 
		return(0);
}

confirm()
{

	char answer[20];
	int nread;

doconfirm:
	nread = read(0, answer, 20);
	if(answer[nread-1] != '\n')
		while(getchar() != '\n');
	if(answer[0] == 'y' || answer[0] == 'Y' || answer[0] == '\n')
		return(1);
	else if(answer[0] == 'n' || answer[0] == 'N')
		return(0);
	else {
		printstr("\n***** Bad input please enter (y/n)? ");
		goto doconfirm;
	}
}

printstr(s)
char *s;
{

	write(1, s, strlen(s));
}

getinteger()
{

	int value;

getint:
	if(scanf("%d", &value) <= 0) {
		while(getchar() != '\n');
		printstr("\n***** Bad input please enter (INTEGER NUMBER)? ");
		goto getint;
	}
	while(getchar() != '\n');
	return(value);
}

gethexnum()
{

	int value;

gethex:
	if(scanf("%x", &value) <= 0) {
		while(getchar() != '\n');
		printstr("\n***** Bad input please enter (HEX NUMBER)? ");
		goto gethex;
	}
	while(getchar() != '\n');
	return(value);
}
