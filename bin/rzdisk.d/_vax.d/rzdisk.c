

#ifndef lint
static char *sccsid = "@(#)rzdisk.c	2.1    ULTRIX  5/12/89";
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
 *	(RZ22, RZ23, RZ55, and RRD40). This utility must be modified
 *	and tested for each new DIGITAL supported disk drive.
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
#include <vaxscsi/rzdisk.h>

int	rzdev = -1;
char	rzdisk[40];
int 	rzcom;
int 	infobyte;
int	asc;
struct 	read_defect_params		rdp;
struct 	format_params			fp;
struct 	reassign_params			rp;
struct 	verify_params			vp;
struct 	defect_descriptors		dd;
struct 	mode_sel_sns_params 		ms;
struct 	mode_sel_sns_params 		msc;
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
	if((rzdev = open(rzdisk, O_RDWR)) == -1) {
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
	struct page_code_37 ms_page37;
	u_char *byteptr;

	bzero((char *)&inq, sizeof(inq));
	execute_rzcmd(SCSI_GET_INQUIRY_DATA, (char *)&inq);
	for(i=0; i<16; i++)
		prodid[i] = inq.prodid[i];
	prodid[i] = NULL;
	/*
	 * FORMAT UNIT not supported on RRD40 disk.
	 */
	if(strncmp(prodid,"RRD40",5) == 0) {
	    	printf("\nFormat Unit unsupported on device (%s).\n", 
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
	printf("\nFORMATTING A DISK DESTROYS ALL DATA!!!\n");
	printstr("\nARE YOU SURE (y/n)? ");
	if(!confirm())
		return;
	printf("\nYOU ARE FORMATTING THE DEVICE (%s)!!!\n",rzdisk);
	printstr("\nIS THIS CORRECT (y/n)? ");
	if(!confirm())
		return;
	/*
	 * Initialize the defect header and the defect list. Note 
 	 * that there is no defect list being sent and therefore 
	 * the defect list length is set to zero (0).
	 */
	bzero((char *)&dd, sizeof(dd));
	fp.fp_format = BLK_FORMAT;
	fp.fp_interleave = 1;
	fp.fp_pattern = 0;
	fp.fp_defects = fmt_defect_lists;
	fp.fp_length = 0;
	dd.dd_header.fu_hdr.vu = 0;
	dd.dd_header.fu_hdr.dcrt = 0;
	dd.dd_header.fu_hdr.dpry = 0;
	fp.fp_addr = (u_char *)&dd;
	/*
	 * Skip MODE SENSE/MODE SELECT for RZ22/RZ23 disks.
	 */
	if(strncmp(prodid,"RZ22",4) == 0 || strncmp(prodid,"RZ23",4) == 0) {
	    	dd.dd_header.fu_hdr.fov = 0;
	    	dd.dd_header.fu_hdr.stpf = 0;
	    	goto start_format;
	}
	/*
	 * Allow MODE SENSE/MODE SELECT for RZ55 disks. Why this
 	 * is done remains to be a mystery. What we do is simply
	 * read the DRIVE PARAMETERS and them write them back
	 * before doing the FORMAT.
	 */
	else {
		dd.dd_header.fu_hdr.fov = 1;
		dd.dd_header.fu_hdr.stpf = 1;
	}
	/*
	 * Read the DRIVE PARAMETERS using the MODE SENSE command.
	 */
	bzero((char *)&ms, sizeof(ms));
	ms.ms_pgcode = 0x3f;
	ms.ms_length = sizeof(ms) - 4;
	ms.ms_pgctrl = SAVED_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&ms) != SUCCESS)
		goto start_format;
	bzero((char *)&msc, sizeof(msc));
	msc.ms_pgcode = 0x3f;
	msc.ms_length = sizeof(ms) - 4;
	msc.ms_pgctrl = CHANGED_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&msc) != SUCCESS)
		goto start_format;
	ms.ms_hdr.sense_len = 0;
	ms.ms_page1.ps = 0;
	ms.ms_page2.ps = 0;
	ms.ms_page3.ps = 0;
	ms.ms_page4.ps = 0;
	ms.ms_page8.ps = 0;
	ms.ms_page37.ps = 0;
	ms.ms_setps = 1;
	ms.ms_length = sizeof(ms) - 4;
	fix_mode_select_params();

	/* Adjust the length if no page 8 */
	if(ms.ms_page8.pgcode != 8) {
		ms.ms_length -= sizeof(ms.ms_page8);
		byteptr = (u_char *)&ms.ms_page8;
		ms_page37 = *((struct page_code_37 *)byteptr);
	}
	else
		ms_page37 = ms.ms_page37;

	/* Adjust the length if no page 37 */
	if(ms_page37.pgcode != 37)
		ms.ms_length -= sizeof(ms_page37);
	/*
	 * Write the DRIVE PARAMETERS using the MODE SELECT command.
	 */
	execute_rzcmd(SCSI_MODE_SELECT, (char *)&ms);
	/*
	 * Start FORMATTING the disk.
	 */
start_format:
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
		    printf("\nTHE DEVICE (%s) HAS MOUNTED FILESYSTEMS!!!\n",
				rzdisk);
		    printf("\nCANNOT FORMAT A DISK THAT'S MOUNTED!!!\n");
		    printf("\nUNMOUNT ALL FILESYSTEMS BEFORE FORMATTING!!!\n");
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
	execute_rzcmd(SCSI_GET_INQUIRY_DATA, (char *)&inq);
	for(i=0; i<16; i++)
		prodid[i] = inq.prodid[i];
	prodid[i] = NULL;
	/*
	 * REASSIGN BLOCK not supported on RRD40 disk.
	 */
	if(strncmp(prodid,"RRD40",5) == 0) {
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
	if(write(rzdev, buffer, 512) == -1)
		goto reassign;
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
	execute_rzcmd(SCSI_GET_INQUIRY_DATA, (char *)&inq);
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
	 * READ DEFECT DATA not supported on RRD40 disk.
	 */
	if(strncmp(prodid,"RRD40",5) == 0) {
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
	char *ptr;
	int rz22_rz23_disk = 0;
	int rrd40_disk = 0;
	int rz55_disk = 0;
	struct page_code_37 ms_page37;
	u_char *byteptr;

	bzero((char *)&inq, sizeof(inq));
	execute_rzcmd(SCSI_GET_INQUIRY_DATA, (char *)&inq);
	for(i=0; i<16; i++)
		prodid[i] = inq.prodid[i];
	prodid[i] = NULL;
	/*
	 * Can only change pages (1 and 37) for RZ22/RZ23 disks.
	 */
	if(strncmp(prodid,"RZ22",4) == 0 ||
		strncmp(prodid,"RZ23",4) == 0)
		rz22_rz23_disk = 1;

	/*
	 * Can only change page (1) for the RRD40 disk.
	 */
	if(strncmp(prodid,"RRD40",5) == 0)
		rrd40_disk = 1;

	/*
	 * Can change all pages (1,2,3,4,8,37) for the RZ55 disk.
	 */
	if(strncmp(prodid,"RZ55",5) == 0)
		rz55_disk = 1;
	/*
	 * User wants to enter new parameters interactively.
	 */
	if(ask)
	    	goto interactive;
	/*
	 * Inform the user what's going on.
	 */
	printf("\nCHANGING DISK DRIVE PARAMETERS TO DEFAULT VALUES!!!\n");
	printstr("\nARE YOU SURE (y/n)? ");
	if(!confirm())
		return;
	/*
	 * Get the DEFAULT disk drive parameters.
	 */
	bzero((char *)&ms, sizeof(ms));
	ms.ms_pgcode = 0x3f;
	ms.ms_length = sizeof(ms) - 4;
	ms.ms_pgctrl = DEFAULT_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&ms) != SUCCESS) {
		if(geterror() != NO_ERROR)
			return;
	}
	bzero((char *)&msc, sizeof(msc));
	msc.ms_pgcode = 0x3f;
	msc.ms_length = sizeof(ms) - 4;
	msc.ms_pgctrl = CHANGED_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&msc) != SUCCESS) {
		if(geterror() != NO_ERROR)
			return;
	}
	/*
	 * Cannot SAVE the parameters for the RRD40 disk.
	 */
	if(rrd40_disk)
		setps = 0;
	/*
	 * Can SAVE the parameters for the RZ22,RZ23,RZ55 disks.
	 */
	else {
		printstr("\nSAVE THE DEFAULT VALUES ON DISK (y/n)? ");
		if(confirm())
			setps = 1;
		else
			setps = 0;
	}
	ms.ms_hdr.sense_len = 0;
	ms.ms_hdr.wp = 0;
	ms.ms_page1.ps = 0;
	ms.ms_page2.ps = 0;
	ms.ms_page3.ps = 0;
	ms.ms_page4.ps = 0;
	ms.ms_page8.ps = 0;
	ms.ms_page37.ps = 0;
	ms.ms_setps = setps;
	ms.ms_length = sizeof(ms) - 4;
	fix_mode_select_params();
	/* Adjust the length if no page 8 */
	if(ms.ms_page8.pgcode != 8) {
		ms.ms_length -= sizeof(ms.ms_page8);
		byteptr = (u_char *)&ms.ms_page8;
		ms_page37 = *((struct page_code_37 *)byteptr);
	}
	else
		ms_page37 = ms.ms_page37;
	/* Adjust the length if no DEC unique page 37 */
	if(ms_page37.pgcode != 37)
		ms.ms_length -= sizeof(ms_page37);
	/* Adjust the length for the RZ22/RZ23 disks */
	if(rz22_rz23_disk) {
		ms.ms_length = ms.ms_length - sizeof(ms.ms_page2) -
			       sizeof(ms.ms_page3) - sizeof(ms.ms_page4);
		if(ms_page37.pgcode == 37) {
			byteptr = (u_char *)&ms.ms_page2;
			bcopy((char *)&ms_page37, byteptr, 
						sizeof(ms_page37));
		}
	}
	/* Adjust the length for the RRD40 disk */
	if(rrd40_disk) {
		ms.ms_length = ms.ms_length - sizeof(ms.ms_page2) -
				sizeof(ms.ms_page3) - sizeof(ms.ms_page4);
	}
	/*
	 * Change the disk drive parameters to their DEFAULT values.
	 */
	printf("\nChanging disk drive parameters for device (%s).\n",
		rzdisk);
	if(execute_rzcmd(SCSI_MODE_SELECT, (char *)&ms) != SUCCESS) {
		if(geterror() != NO_ERROR)
			return;
	}
	return;
interactive:
	/*
	 * Inform the user what's going on.
	 */
	printf("\nCHANGING DISK DRIVE PARAMETERS INTERACTIVELY!!!\n");
	printstr("\nARE YOU SURE (y/n)? ");
	if(!confirm())
		return;
	/*
	 * Get the CURRENT disk drive parameters.
	 */
	bzero((char *)&ms, sizeof(ms));
	ms.ms_pgcode = 0x3f;
	ms.ms_length = sizeof(ms) - 4;
	ms.ms_pgctrl = CURRENT_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&ms) != SUCCESS) {
		if(geterror() != NO_ERROR)
			return;
	}
	bzero((char *)&msc, sizeof(msc));
	msc.ms_pgcode = 0x3f;
	msc.ms_length = sizeof(ms) - 4;
	msc.ms_pgctrl = CHANGED_VALUES;
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&msc) != SUCCESS) {
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
	printstr("\nDo you want to change the block length (y/n)? ");
	if(confirm()) {
		printstr("Enter the new block length? ");
		value = getinteger();
		ms.ms_desc.blklen2 = ((value >> 16) & 0xff);
		ms.ms_desc.blklen1 = ((value >> 8) & 0xff);
		ms.ms_desc.blklen0 = ((value >> 0) & 0xff);
	}
page1_ask:
	printf("\nPage 1 error recovery parameters:\n");
	printf("  PS\t\t\t\t\t%d\n", ms.ms_page1.ps);
	printf("  Page code\t\t\t\t%d\n", ms.ms_page1.pgcode);
	printf("  Page length\t\t\t\t%d\n", ms.ms_page1.pglength);
	printf("  Flags\t\t\t\t\t0x%02x\n", ms.ms_page1.flags);
	printf("  Retry count\t\t\t\t%d\n", ms.ms_page1.retry_count);
	printf("  Correction span\t\t\t%d\n", ms.ms_page1.correct_span);
	printf("  Head offset count\t\t\t%d\n", ms.ms_page1.head_offset);
	printf("  Data strobe offset count\t\t%d\n", ms.ms_page1.data_strobe);
	printf("  Recovery time limit\t\t\t%d\n", ms.ms_page1.recovery_time);
	printstr("\nDo you want to change any fields in this page (y/n)? ");
	if(confirm()) {
		if(msc.ms_page1.flags) {
		    printstr("Enter the new flags in (HEX)? ");
		    value = gethexnum();
		    ms.ms_page1.flags = value;
		}
		if(msc.ms_page1.retry_count) {
		    printstr("Enter the new retry count? ");
		    value = getinteger();
		    ms.ms_page1.retry_count = value;
		}
		if(msc.ms_page1.correct_span) {
		    printstr("Enter the new correction span? ");
		    value = getinteger();
		    ms.ms_page1.correct_span = value;
		}
		if(msc.ms_page1.head_offset) {
		    printstr("Enter the new head offset count? ");
		    value = getinteger();
		    ms.ms_page1.head_offset = value;
		}
		if(msc.ms_page1.data_strobe) {
		    printstr("Enter the new data strobe offset count? ");
		    value = getinteger();
		    ms.ms_page1.data_strobe = value;
		}
		if(msc.ms_page1.recovery_time) {
		    printstr("Enter the new recovery time limit? ");
		    value = getinteger();
		    ms.ms_page1.recovery_time = value;
		}
	}
page2_ask:
	if(rrd40_disk)
		goto done_asking;
	if(rz22_rz23_disk) {
		byteptr = (u_char *)&ms.ms_page8;
		ms_page37 = *((struct page_code_37 *)byteptr);
		goto page37_ask;
	}
	printf("\nPage 2 disconnect/reconnect parameters:\n");
	printf("  PS\t\t\t\t\t%d\n", ms.ms_page2.ps);
	printf("  Page code\t\t\t\t%d\n", ms.ms_page2.pgcode);
	printf("  Page length\t\t\t\t%d\n", ms.ms_page2.pglength);
	printf("  Buffer full ratio\t\t\t%d\n", ms.ms_page2.bus_fratio);
	printf("  Buffer empty ratio\t\t\t%d\n", ms.ms_page2.bus_eratio);
	printf("  Bus inactivity limit\t\t\t%d\n", 
			((ms.ms_page2.bus_inactive1<<8) +
				ms.ms_page2.bus_inactive0));
	printf("  Disconnect time limit\t\t\t%d\n",
			((ms.ms_page2.disconn_time1<<8) +
				ms.ms_page2.disconn_time0));
	printf("  Connect time limit\t\t\t%d\n", 
			((ms.ms_page2.conn_time1<<8) +
				ms.ms_page2.conn_time0));
	printstr("\nDo you want to change any fields in this page (y/n)? ");
	if(confirm()) {
		if(msc.ms_page2.bus_fratio) {
		    printstr("Enter the new bus full ratio? ");
		    value = getinteger();
		    ms.ms_page2.bus_fratio = value;
		}
		if(msc.ms_page2.bus_eratio) {
		    printstr("Enter the new bus empty ratio? ");
		    value = getinteger();
		    ms.ms_page2.bus_eratio = value;
		}
		if(msc.ms_page2.bus_inactive1 || msc.ms_page2.bus_inactive0) {
		    printstr("Enter the new bus inactivity limit? ");
		    value = getinteger();
		    ms.ms_page2.bus_inactive1 = ((value >> 8) & 0xff);
		    ms.ms_page2.bus_inactive0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page2.disconn_time1 || msc.ms_page2.disconn_time0) {
		    printstr("Enter the new disconnect time limit? ");
		    value = getinteger();
		    ms.ms_page2.disconn_time1 = ((value >> 8) & 0xff);
		    ms.ms_page2.disconn_time0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page2.conn_time1 || msc.ms_page2.conn_time0) {
		    printstr("Enter the new connect time limit? ");
		    value = getinteger();
		    ms.ms_page2.conn_time1 = ((value >> 8) & 0xff);
		    ms.ms_page2.conn_time0 = ((value >> 0) & 0xff);
		}
	}
page3_ask:
	printf("\nPage 3 direct-access device format parameters:\n");
	printf("  PS\t\t\t\t\t%d\n", ms.ms_page3.ps);
	printf("  Page code\t\t\t\t%d\n", ms.ms_page3.pgcode);
	printf("  Page length\t\t\t\t%d\n", ms.ms_page3.pglength);
	printf("  Tracks per zone\t\t\t%d\n", 
			((ms.ms_page3.tpz1<<8) + ms.ms_page3.tpz0));
	printf("  Alternate sectors per zone\t\t%d\n", 
			((ms.ms_page3.aspz1<<8) + ms.ms_page3.aspz0));
	printf("  Alternate tracks per zone\t\t%d\n", 
			((ms.ms_page3.atpz1<<8) + ms.ms_page3.atpz0));
	printf("  Alternate tracks per volume\t\t%d\n", 
			((ms.ms_page3.atpv1<<8) + ms.ms_page3.atpv0));
	printf("  Sectors per track\t\t\t%d\n", 
			((ms.ms_page3.spt1<<8) + ms.ms_page3.spt0));
	printf("  Data bytes per phy sector\t\t%d\n", 
			((ms.ms_page3.bps1<<8) + ms.ms_page3.bps0));
	printf("  Interleave\t\t\t\t%d\n", 
			((ms.ms_page3.interleave1<<8) + 
				ms.ms_page3.interleave0));
	printf("  Track skew\t\t\t\t%d\n", 
			((ms.ms_page3.track_skew1<<8) + 
				ms.ms_page3.track_skew0));
	printf("  Cylinder skew\t\t\t\t%d\n", 
			((ms.ms_page3.cylinder_skew1<<8) + 
				ms.ms_page3.cylinder_skew0));
	printf("  Flags\t\t\t\t\t0x%02x\n", ms.ms_page1.flags);
	printstr("\nDo you want to change any fields in this page (y/n)? ");
	if(confirm()) {
		if(msc.ms_page3.tpz1 || msc.ms_page3.tpz0) {
		    printstr("Enter the new tracks per zone? ");
		    value = getinteger();
		    ms.ms_page3.tpz1 = ((value >> 8) & 0xff);
		    ms.ms_page3.tpz0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page3.aspz1 || msc.ms_page3.aspz0) {
		    printstr("Enter the new alternate sectors per zone? ");
		    value = getinteger();
		    ms.ms_page3.aspz1 = ((value >> 8) & 0xff);
		    ms.ms_page3.aspz0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page3.atpz1 || msc.ms_page3.atpz0) {
		    printstr("Enter the new alternate tracks per zone? ");
		    value = getinteger();
		    ms.ms_page3.atpz1 = ((value >> 8) & 0xff);
		    ms.ms_page3.atpz0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page3.atpv1 || msc.ms_page3.atpv0) {
		    printstr("Enter the new alternate tracks per volume? ");
		    value = getinteger();
		    ms.ms_page3.atpv1 = ((value >> 8) & 0xff);
		    ms.ms_page3.atpv0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page3.spt1 || msc.ms_page3.spt0) {
		    printstr("Enter the new sectors per track? ");
		    value = getinteger();
		    ms.ms_page3.spt1 = ((value >> 8) & 0xff);
		    ms.ms_page3.spt0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page3.bps1 || msc.ms_page3.bps0) {
		    printstr("Enter the new bytes per sector? ");
		    value = getinteger();
		    ms.ms_page3.bps1 = ((value >> 8) & 0xff);
		    ms.ms_page3.bps0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page3.interleave1 || msc.ms_page3.interleave0) {
		    printstr("Enter the new interleave factor? ");
		    value = getinteger();
		    ms.ms_page3.interleave1 = ((value >> 8) & 0xff);
		    ms.ms_page3.interleave0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page3.track_skew1 || msc.ms_page3.track_skew0) {
		    printstr("Enter the new track skew? ");
		    value = getinteger();
		    ms.ms_page3.track_skew1 = ((value >> 8) & 0xff);
		    ms.ms_page3.track_skew0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page3.cylinder_skew1 || msc.ms_page3.cylinder_skew0) {
		    printstr("Enter the new cylinder skew? ");
		    value = getinteger();
		    ms.ms_page3.cylinder_skew1 = ((value >> 8) & 0xff);
		    ms.ms_page3.cylinder_skew0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page3.flags) {
		    printstr("Enter the new flags in (HEX)? ");
		    value = gethexnum();
		    ms.ms_page3.flags = value;
		}
	}
page4_ask:
	printf("\nPage 4 rigid disk drive geometry parameters:\n");
	printf("  PS\t\t\t\t\t%d\n", ms.ms_page4.ps);
	printf("  Page code\t\t\t\t%d\n", ms.ms_page4.pgcode);
	printf("  Page length\t\t\t\t%d\n", ms.ms_page4.pglength);
	printf("  Maximum number of cylinders\t\t%d\n", 
			((ms.ms_page4.ncyl2<<16) +
				(ms.ms_page4.ncyl1<<8) +
					ms.ms_page4.ncyl0));
	printf("  Maximum number of heads\t\t%d\n", ms.ms_page4.nheads);
	printf("  Write precompensation start\t\t%d\n", 
			((ms.ms_page4.wprecomp2<<16) +
				(ms.ms_page4.wprecomp1<<8) +
					ms.ms_page4.wprecomp0));
	printf("  Reduced write current start\t\t%d\n", 
			((ms.ms_page4.rwc2<<16) +
				(ms.ms_page4.rwc1<<8) +
					ms.ms_page4.rwc0));
	printf("  Drive step rate\t\t\t%d\n", 
			((ms.ms_page4.dsr1<<8) + ms.ms_page4.dsr0));
	printf("  Landing zone cylinder\t\t\t%d\n", 
			((ms.ms_page4.lzc2<<16) +
				(ms.ms_page4.lzc1<<8) +
					ms.ms_page4.lzc0));
	printstr("\nDo you want to change any fields in this page (y/n)? ");
	if(confirm()) {
		if(msc.ms_page4.ncyl2 || 
			msc.ms_page4.ncyl1 || msc.ms_page4.ncyl0) {
		    printstr("Enter the number of cylinders? ");
		    value = getinteger();
		    ms.ms_page4.ncyl2 = ((value >> 16) & 0xff);
		    ms.ms_page4.ncyl1 = ((value >> 8) & 0xff);
		    ms.ms_page4.ncyl0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page4.nheads) {
		    printstr("Enter the number of heads? ");
		    value = getinteger();
		    ms.ms_page4.nheads = value;
		}
		if(msc.ms_page4.wprecomp2 || 
			msc.ms_page4.wprecomp1 || msc.ms_page4.wprecomp0) {
		    printstr("Enter the write precompensation start? ");
		    value = getinteger();
		    ms.ms_page4.wprecomp2 = ((value >> 16) & 0xff);
		    ms.ms_page4.wprecomp1 = ((value >> 8) & 0xff);
		    ms.ms_page4.wprecomp0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page4.rwc2 || 
			msc.ms_page4.rwc1 || msc.ms_page4.rwc0) {
		    printstr("Enter the reduced write current start? ");
		    value = getinteger();
		    ms.ms_page4.rwc2 = ((value >> 16) & 0xff);
		    ms.ms_page4.rwc1 = ((value >> 8) & 0xff);
		    ms.ms_page4.rwc0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page4.dsr1 || msc.ms_page4.dsr0) {
		    printstr("Enter the drive step rate? ");
		    value = getinteger();
		    ms.ms_page4.dsr1 = ((value >> 8) & 0xff);
		    ms.ms_page4.dsr0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page4.lzc1 || msc.ms_page4.lzc0) {
		    printstr("Enter the landing zone cylinder? ");
		    value = getinteger();
		    ms.ms_page4.lzc1 = ((value >> 8) & 0xff);
		    ms.ms_page4.lzc0 = ((value >> 0) & 0xff);
		}
	}
page8_ask:
	if(ms.ms_page8.pgcode != 8) {
		byteptr = (u_char *)&ms.ms_page8;
		ms_page37 = *((struct page_code_37 *)byteptr);
		goto page37_ask;
	}
	else
		ms_page37 = ms.ms_page37;
    	printf("\nPage 8 caching parameters:\n");
    	printf("  PS\t\t\t\t\t%d\n", ms.ms_page8.ps);
    	printf("  Page code\t\t\t\t%d\n", ms.ms_page8.pgcode);
    	printf("  Page length\t\t\t\t%d\n", ms.ms_page8.pglength);
	printf("  Read cache disable bit\t\t0x%x\n",
				ms.ms_page8.rc);
	printf("  Multiple selection bit\t\t0x%x\n",
				ms.ms_page8.ms);
	printf("  Write cache enable bit\t\t0x%x\n",
				ms.ms_page8.wce);
	printf("  Write retention priority\t\t%d\n",
				ms.ms_page8.wrp);
	printf("  Demand read retention priority\t%d\n",
				ms.ms_page8.drrp);
	printf("  Disable prefetch transfer length\t%d\n",
			((ms.ms_page8.dpftl1<<8) + ms.ms_page8.dpftl0));
	printf("  Minimum prefetch\t\t\t%d\n",
			((ms.ms_page8.minpf1<<8) + ms.ms_page8.minpf0));
	printf("  Maximum prefetch\t\t\t%d\n",
			((ms.ms_page8.maxpf1<<8) + ms.ms_page8.maxpf0));
	printf("  Maximum prefetch ceiling\t\t%d\n",
			((ms.ms_page8.maxpfc1<<8) + ms.ms_page8.maxpfc0));
	printstr("\nDo you want to change any fields in this page (y/n)? ");
	if(confirm()) {
		if(msc.ms_page8.rc) {
		    printstr("Set read cache disable (RC) bit (y/n)? ");
		    if(confirm())
			ms.ms_page8.rc = 1;
		    else
			ms.ms_page8.rc = 0;
		}
		if(msc.ms_page8.ms) {
		    printstr("Set mutiple selection (MS) bit (y/n)? ");
		    if(confirm())
			ms.ms_page8.ms = 1;
		    else
			ms.ms_page8.ms = 0;
		}
		if(msc.ms_page8.wce) {
		    printstr("Set write cache enable (WCE) bit (y/n)? ");
		    if(confirm())
			ms.ms_page8.wce = 1;
		    else
			ms.ms_page8.wce = 0;
		}
		if(msc.ms_page8.wrp) {
		    printstr("Enter the write retention priority? ");
		    value = getinteger();
		    ms.ms_page8.wrp = value;
		}
		if(msc.ms_page8.drrp) {
		    printstr("Enter the demand read retention priority? ");
		    value = getinteger();
		    ms.ms_page8.drrp = value;
		}
		if(msc.ms_page8.dpftl1 || msc.ms_page8.dpftl0) {
		    printstr("Enter disable pre-fetch transfer length? ");
		    value = getinteger();
		    ms.ms_page8.dpftl1 = ((value >> 8) & 0xff);
		    ms.ms_page8.dpftl0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page8.minpf1 || msc.ms_page8.minpf0) {
		    printstr("Enter minimum pre-fetch value? ");
		    value = getinteger();
		    ms.ms_page8.minpf1 = ((value >> 8) & 0xff);
		    ms.ms_page8.minpf0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page8.maxpf1 || msc.ms_page8.maxpf0) {
		    printstr("Enter maximum pre-fetch value? ");
		    value = getinteger();
		    ms.ms_page8.maxpf1 = ((value >> 8) & 0xff);
		    ms.ms_page8.maxpf0 = ((value >> 0) & 0xff);
		}
		if(msc.ms_page8.maxpfc1 || msc.ms_page8.maxpfc0) {
		    printstr("Enter maximum pre-fetch ceiling value? ");
		    value = getinteger();
		    ms.ms_page8.maxpfc1 = ((value >> 8) & 0xff);
		    ms.ms_page8.maxpfc0 = ((value >> 0) & 0xff);
		}
	}
page37_ask:
	if(ms_page37.pgcode != 37)
		goto done_asking;
	printf("\nPage 37 DEC unique parameters:\n");
	printf("  PS\t\t\t\t\t%d\n", ms_page37.ps);
	printf("  Page code\t\t\t\t%d\n", ms_page37.pgcode);
	printf("  Page length\t\t\t\t%d\n", ms_page37.pglength);
	printf("  Spinup on power up\t\t\t%d\n", ms_page37.spinup);
	printstr("\nDo you want to change any fields in this page (y/n)? ");
	if(confirm()) {
		printstr("Have drive spinup on power on (y/n)? ");
		if(confirm())
		   	ms_page37.spinup = 0;
		else
		   	ms_page37.spinup = 1;
	}
done_asking:
	/*
	 * Cannot SAVE the parameters for the RRD40 disk.
	 */
	if(rrd40_disk)
		setps = 0;
	/*
	 * Can SAVE the parameters for the RZ22,RZ23,RZ55 disks.
	 */
	else {
		printstr("\nSAVE THE CHANGED VALUES ON DISK (y/n)? ");
		if(confirm())
			setps = 1;
		else
			setps = 0;
	}
	ms.ms_hdr.sense_len = 0;
	ms.ms_hdr.wp = 0;
	ms.ms_page1.ps = 0;
	ms.ms_page2.ps = 0;
	ms.ms_page3.ps = 0;
	ms.ms_page4.ps = 0;
	ms.ms_page8.ps = 0;
	ms.ms_page37.ps = 0;
	ms_page37.ps = 0;
	ms.ms_setps = setps;
	ms.ms_length = sizeof(ms) - 4;
	fix_mode_select_params();
	/* Adjust the length if no page 8 */
	if(ms.ms_page8.pgcode != 8) {
		ms.ms_length -= sizeof(ms.ms_page8);
		byteptr = (u_char *)&ms.ms_page8;
		bcopy((u_char *)&ms_page37, byteptr, sizeof(ms_page37));
	}
	else {
		ms.ms_page37 = ms_page37;
	}
	/* Adjust the length if no DEC unique page 37 */
	if(ms_page37.pgcode != 37)
		ms.ms_length -= sizeof(ms_page37);
	/* Adjust the length for the RZ22/RZ23 disks */
	if(rz22_rz23_disk) {
		ms.ms_length = ms.ms_length - sizeof(ms.ms_page2) -
			       sizeof(ms.ms_page3) - sizeof(ms.ms_page4);
		if(ms_page37.pgcode == 37) {
			byteptr = (u_char *)&ms.ms_page2;
			bcopy((char *)&ms_page37, byteptr, 
						sizeof(ms_page37));
		}
	}
	/* Adjust the length for the RRD40 disk */
	if(rrd40_disk) {
		ms.ms_length = ms.ms_length - sizeof(ms.ms_page2) -
				sizeof(ms.ms_page3) - sizeof(ms.ms_page4);
	}
	/*
	 * Change the disk drive parameters to their CHANGED values.
	 */
	printf("\nChanging disk drive parameters for device (%s).\n",
		rzdisk);
	if(execute_rzcmd(SCSI_MODE_SELECT, (char *)&ms) != SUCCESS) {
		if(geterror() != NO_ERROR)
			return;
	}
}

fix_mode_select_params()
{

	ms.ms_desc.density_code &= msc.ms_desc.density_code;
	ms.ms_desc.nblks2 &= msc.ms_desc.nblks2;
	ms.ms_desc.nblks1 &= msc.ms_desc.nblks1;
	ms.ms_desc.nblks0 &= msc.ms_desc.nblks0;
	ms.ms_page1.flags &= msc.ms_page1.flags;
	ms.ms_page1.retry_count &= msc.ms_page1.retry_count;
	ms.ms_page1.correct_span &= msc.ms_page1.correct_span;
	ms.ms_page1.head_offset &= msc.ms_page1.head_offset;
	ms.ms_page1.data_strobe &= msc.ms_page1.data_strobe;
	ms.ms_page1.recovery_time &= msc.ms_page1.recovery_time;
	ms.ms_page2.bus_fratio &= msc.ms_page2.bus_fratio;
	ms.ms_page2.bus_eratio &= msc.ms_page2.bus_eratio;
	ms.ms_page2.bus_inactive1 &= msc.ms_page2.bus_inactive1;
	ms.ms_page2.bus_inactive0 &= msc.ms_page2.bus_inactive0;
	ms.ms_page2.disconn_time1 &= msc.ms_page2.disconn_time1;
	ms.ms_page2.disconn_time0 &= msc.ms_page2.disconn_time0;
	ms.ms_page2.conn_time1 &= msc.ms_page2.conn_time1;
	ms.ms_page2.conn_time0 &= msc.ms_page2.conn_time0;
	ms.ms_page3.tpz1 &= msc.ms_page3.tpz1;
	ms.ms_page3.tpz0 &= msc.ms_page3.tpz0;
	ms.ms_page3.aspz1 &= msc.ms_page3.aspz1;
	ms.ms_page3.aspz0 &= msc.ms_page3.aspz0;
	ms.ms_page3.atpz1 &= msc.ms_page3.atpz1;
	ms.ms_page3.atpz0 &= msc.ms_page3.atpz0;
	ms.ms_page3.atpv1 &= msc.ms_page3.atpv1;
	ms.ms_page3.atpv0 &= msc.ms_page3.atpv0;
	ms.ms_page3.spt1 &= msc.ms_page3.spt1;
	ms.ms_page3.spt0 &= msc.ms_page3.spt0;
	ms.ms_page3.bps1 &= msc.ms_page3.bps1;
	ms.ms_page3.bps0 &= msc.ms_page3.bps0;
	ms.ms_page3.interleave1 &= msc.ms_page3.interleave1;
	ms.ms_page3.interleave0 &= msc.ms_page3.interleave0;
	ms.ms_page3.track_skew1 &= msc.ms_page3.track_skew1;
	ms.ms_page3.track_skew0 &= msc.ms_page3.track_skew0;
	ms.ms_page3.cylinder_skew1 &= msc.ms_page3.cylinder_skew1;
	ms.ms_page3.cylinder_skew0 &= msc.ms_page3.cylinder_skew0;
	ms.ms_page3.flags &= msc.ms_page3.flags;
	ms.ms_page4.ncyl2 &= msc.ms_page4.ncyl2;
	ms.ms_page4.ncyl1 &= msc.ms_page4.ncyl1;
	ms.ms_page4.ncyl0 &= msc.ms_page4.ncyl0;
	ms.ms_page4.nheads &= msc.ms_page4.nheads;
	ms.ms_page4.wprecomp2 &= msc.ms_page4.wprecomp2;
	ms.ms_page4.wprecomp1 &= msc.ms_page4.wprecomp1;
	ms.ms_page4.wprecomp0 &= msc.ms_page4.wprecomp0;
	ms.ms_page4.rwc2 &= msc.ms_page4.rwc2;
	ms.ms_page4.rwc1 &= msc.ms_page4.rwc1;
	ms.ms_page4.rwc0 &= msc.ms_page4.rwc0;
	ms.ms_page4.dsr1 &= msc.ms_page4.dsr1;
	ms.ms_page4.dsr0 &= msc.ms_page4.dsr0;
	ms.ms_page4.lzc1 &= msc.ms_page4.lzc1;
	ms.ms_page4.lzc0 &= msc.ms_page4.lzc0;
	ms.ms_page8.rc &= msc.ms_page8.rc;
	ms.ms_page8.ms &= msc.ms_page8.ms;
	ms.ms_page8.wce &= msc.ms_page8.wce;
	ms.ms_page8.wrp &= msc.ms_page8.wrp;
	ms.ms_page8.drrp &= msc.ms_page8.drrp;
	ms.ms_page8.dpftl1 &= msc.ms_page8.dpftl1;
	ms.ms_page8.dpftl0 &= msc.ms_page8.dpftl0;
	ms.ms_page8.minpf1 &= msc.ms_page8.minpf1;
	ms.ms_page8.minpf0 &= msc.ms_page8.minpf0;
	ms.ms_page8.maxpf1 &= msc.ms_page8.maxpf1;
	ms.ms_page8.maxpf0 &= msc.ms_page8.maxpf0;
	ms.ms_page8.maxpfc1 &= msc.ms_page8.maxpfc1;
	ms.ms_page8.maxpfc0 &= msc.ms_page8.maxpfc0;
}

get_inquiry_info()
{
    
    char hold[20];
    int i;

	printf("\nGetting inquiry data info from device (%s).\n", rzdisk);
	bzero((char *)&inq, sizeof(inq));
	if(execute_rzcmd(SCSI_GET_INQUIRY_DATA, (char *)&inq) != SUCCESS) {
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
	struct page_code_1 ms_page1;
	struct page_code_2 ms_page2;
	struct page_code_3 ms_page3;
	struct page_code_4 ms_page4;
	struct page_code_8 ms_page8;
	struct page_code_37 ms_page37;
	u_char *byteptr;
	int i;

	bzero((char *)&ms, sizeof(ms));
	ms.ms_pgcode = 0x3f;
	ms.ms_length = sizeof(ms) - 4;
	ms.ms_pgctrl = page_control;
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
	if(execute_rzcmd(SCSI_MODE_SENSE, (char *)&ms) != SUCCESS) {
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
	printf("  WP\t\t\t\t\t0x%02x\n", ms.ms_hdr.wp);
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
page1_display:
	byteptr = (u_char *)&ms.ms_page1;
	ms_page1 = *((struct page_code_1 *)byteptr);
	if(ms_page1.pgcode == 1) {
		if(ms_page1.pglength == 0) {
			byteptr += 2;
			goto page2_display;
		}
		if(!moreoutput())
			return;
	    	printf("\nPage 1 error recovery parameters:\n");
	    	printf("  PS\t\t\t\t\t%d\n", ms_page1.ps);
	    	printf("  Page code\t\t\t\t%d\n", ms_page1.pgcode);
	    	printf("  Page length\t\t\t\t%d\n", ms_page1.pglength);
		if(page_control == CHANGED_VALUES) {
	    		printf("  Flags\t\t\t\t\t0x%x\n", ms_page1.flags);
	    		printf("  Retry count\t\t\t\t0x%x\n", 
					ms_page1.retry_count);
	    		printf("  Correction span\t\t\t0x%x\n", 
					ms_page1.correct_span);
	    		printf("  Head offset count\t\t\t0x%x\n", 
					ms_page1.head_offset);
	    		printf("  Data strobe offset count\t\t0x%x\n", 
					ms_page1.data_strobe);
	    		printf("  Recovery time limit\t\t\t0x%x\n", 
					ms_page1.recovery_time);
		}
		else {
	    		printf("  Flags\t\t\t\t\t0x%02x\n", ms_page1.flags);
	    		printf("  Retry count\t\t\t\t%d\n", 
					ms_page1.retry_count);
	    		printf("  Correction span\t\t\t%d\n", 
					ms_page1.correct_span);
	    		printf("  Head offset count\t\t\t%d\n", 
					ms_page1.head_offset);
	    		printf("  Data strobe offset count\t\t%d\n", 
					ms_page1.data_strobe);
	    		printf("  Recovery time limit\t\t\t%d\n", 
					ms_page1.recovery_time);
		}
		byteptr += sizeof(ms_page1);
	}
	
page2_display:
	ms_page2 = *((struct page_code_2 *)byteptr);
	if(ms_page2.pgcode == 2) {
		if(ms_page2.pglength == 0) {
			byteptr += 2;
			goto page3_display;
		}
		if(!moreoutput())
			return;
		printf("\nPage 2 disconnect/reconnect parameters:\n");
		printf("  PS\t\t\t\t\t%d\n", ms_page2.ps);
		printf("  Page code\t\t\t\t%d\n", ms_page2.pgcode);
		printf("  Page length\t\t\t\t%d\n", ms_page2.pglength);
		if(page_control == CHANGED_VALUES) {
			printf("  Buffer full ratio\t\t\t0x%x\n", 
				ms_page2.bus_fratio);
			printf("  Buffer empty ratio\t\t\t0x%x\n", 
				ms_page2.bus_eratio);
			printf("  Bus inactivity limit\t\t\t0x%x\n", 
				((ms_page2.bus_inactive1<<8) +
					ms_page2.bus_inactive0));
			printf("  Disconnect time limit\t\t\t0x%x\n",
				((ms_page2.disconn_time1<<8) +
					ms_page2.disconn_time0));
			printf("  Connect time limit\t\t\t0x%x\n", 
				((ms_page2.conn_time1<<8) +
					ms_page2.conn_time0));
		}
		else {
			printf("  Buffer full ratio\t\t\t%d\n", 
				ms_page2.bus_fratio);
			printf("  Buffer empty ratio\t\t\t%d\n", 
				ms_page2.bus_eratio);
			printf("  Bus inactivity limit\t\t\t%d\n", 
				((ms_page2.bus_inactive1<<8) +
					ms_page2.bus_inactive0));
			printf("  Disconnect time limit\t\t\t%d\n",
				((ms_page2.disconn_time1<<8) +
					ms_page2.disconn_time0));
			printf("  Connect time limit\t\t\t%d\n", 
				((ms_page2.conn_time1<<8) +
					ms_page2.conn_time0));
		}
		byteptr += sizeof(ms_page2);
	}
	
page3_display:
	ms_page3 = *((struct page_code_3 *)byteptr);
	if(ms_page3.pgcode == 3) {
		if(ms_page3.pglength == 0) {
			byteptr += 2;
			goto page4_display;
		}
		if(!moreoutput())
			return;
		printf("\nPage 3 direct-access device format parameters:\n");
		printf("  PS\t\t\t\t\t%d\n", ms_page3.ps);
		printf("  Page code\t\t\t\t%d\n", ms_page3.pgcode);
		printf("  Page length\t\t\t\t%d\n", ms_page3.pglength);
		if(page_control == CHANGED_VALUES) {
			printf("  Tracks per zone\t\t\t0x%x\n", 
				((ms_page3.tpz1<<8) + ms_page3.tpz0));
			printf("  Alternate sectors per zone\t\t0x%x\n", 
				((ms_page3.aspz1<<8) + ms_page3.aspz0));
			printf("  Alternate tracks per zone\t\t0x%x\n", 
				((ms_page3.atpz1<<8) + ms_page3.atpz0));
			printf("  Alternate tracks per volume\t\t0x%x\n", 
				((ms_page3.atpv1<<8) + ms_page3.atpv0));
			printf("  Sectors per track\t\t\t0x%x\n", 
				((ms_page3.spt1<<8) + ms_page3.spt0));
			printf("  Data bytes per phy sector\t\t0x%x\n", 
				((ms_page3.bps1<<8) + ms_page3.bps0));
			printf("  Interleave\t\t\t\t0x%x\n", 
				((ms_page3.interleave1<<8) + 
					ms_page3.interleave0));
			printf("  Track skew\t\t\t\t0x%x\n", 
				((ms_page3.track_skew1<<8) + 
					ms_page3.track_skew0));
			printf("  Cylinder skew\t\t\t\t0x%x\n", 
				((ms_page3.cylinder_skew1<<8) + 
					ms_page3.cylinder_skew0));
			printf("  Flags\t\t\t\t\t0x%x\n", ms_page3.flags);
		}
		else {
			printf("  Tracks per zone\t\t\t%d\n", 
				((ms_page3.tpz1<<8) + ms_page3.tpz0));
			printf("  Alternate sectors per zone\t\t%d\n", 
				((ms_page3.aspz1<<8) + ms_page3.aspz0));
			printf("  Alternate tracks per zone\t\t%d\n", 
				((ms_page3.atpz1<<8) + ms_page3.atpz0));
			printf("  Alternate tracks per volume\t\t%d\n", 
				((ms_page3.atpv1<<8) + ms_page3.atpv0));
			printf("  Sectors per track\t\t\t%d\n", 
				((ms_page3.spt1<<8) + ms_page3.spt0));
			printf("  Data bytes per phy sector\t\t%d\n", 
				((ms_page3.bps1<<8) + ms_page3.bps0));
			printf("  Interleave\t\t\t\t%d\n", 
				((ms_page3.interleave1<<8) + 
					ms_page3.interleave0));
			printf("  Track skew\t\t\t\t%d\n", 
				((ms_page3.track_skew1<<8) + 
					ms_page3.track_skew0));
			printf("  Cylinder skew\t\t\t\t%d\n", 
				((ms_page3.cylinder_skew1<<8) + 
					ms_page3.cylinder_skew0));
			printf("  Flags\t\t\t\t\t0x%02x\n", ms_page3.flags);
		}
		byteptr += sizeof(ms_page3);
	}
	
page4_display:
	ms_page4 = *((struct page_code_4 *)byteptr);
	if(ms_page4.pgcode == 4) {
		if(ms_page4.pglength == 0) {
			byteptr += 2;
			goto page8_display;
		}
		if(!moreoutput())
			return;
		printf("\nPage 4 rigid disk drive geometry parameters:\n");
		printf("  PS\t\t\t\t\t%d\n", ms_page4.ps);
		printf("  Page code\t\t\t\t%d\n", ms_page4.pgcode);
		printf("  Page length\t\t\t\t%d\n", ms_page4.pglength);
		if(page_control == CHANGED_VALUES) {
			printf("  Maximum number of cylinders\t\t0x%x\n", 
				((ms_page4.ncyl2<<16) +
					(ms_page4.ncyl1<<8) +
						ms_page4.ncyl0));
			printf("  Maximum number of heads\t\t0x%x\n", 
				ms_page4.nheads);
			printf("  Write precompensation start\t\t0x%x\n", 
				((ms_page4.wprecomp2<<16) +
					(ms_page4.wprecomp1<<8) +
						ms_page4.wprecomp0));
			printf("  Reduced write current start\t\t0x%x\n", 
				((ms_page4.rwc2<<16) +
					(ms_page4.rwc1<<8) +
						ms_page4.rwc0));
			printf("  Drive step rate\t\t\t0x%x\n", 
				((ms_page4.dsr1<<8) + ms_page4.dsr0));
			printf("  Landing zone cylinder\t\t\t0x%x\n", 
				((ms_page4.lzc2<<16) +
					(ms_page4.lzc1<<8) +
						ms_page4.lzc0));
		}
		else {
			printf("  Maximum number of cylinders\t\t%d\n", 
				((ms_page4.ncyl2<<16) +
					(ms_page4.ncyl1<<8) +
						ms_page4.ncyl0));
			printf("  Maximum number of heads\t\t%d\n", 
				ms_page4.nheads);
			printf("  Write precompensation start\t\t%d\n", 
				((ms_page4.wprecomp2<<16) +
					(ms_page4.wprecomp1<<8) +
						ms_page4.wprecomp0));
			printf("  Reduced write current start\t\t%d\n", 
				((ms_page4.rwc2<<16) +
					(ms_page4.rwc1<<8) +
						ms_page4.rwc0));
			printf("  Drive step rate\t\t\t%d\n", 
				((ms_page4.dsr1<<8) + ms_page4.dsr0));
			printf("  Landing zone cylinder\t\t\t%d\n", 
				((ms_page4.lzc2<<16) +
					(ms_page4.lzc1<<8) +
						ms_page4.lzc0));
		}
		byteptr += sizeof(ms_page4);
	}
	
page8_display:
	ms_page8 = *((struct page_code_8 *)byteptr);
	if(ms_page8.pgcode == 8) {
		if(ms_page8.pglength == 0) {
			byteptr += 2;
			goto page37_display;
		}
		if(!moreoutput())
			return;
	    	printf("\nPage 8 caching parameters:\n");
	    	printf("  PS\t\t\t\t\t%d\n", ms_page8.ps);
	    	printf("  Page code\t\t\t\t%d\n", ms_page8.pgcode);
	    	printf("  Page length\t\t\t\t%d\n", ms_page8.pglength);
		if(page_control == CHANGED_VALUES) {
			printf("  Read cache disable bit\t\t0x%x\n",
						ms_page8.rc);
			printf("  Multiple selection bit\t\t0x%x\n",
						ms_page8.ms);
			printf("  Write cache enable bit\t\t0x%x\n",
						ms_page8.wce);
			printf("  Write retention priority\t\t0x%x\n",
						ms_page8.wrp);
			printf("  Demand read retention priority\t0x%x\n",
						ms_page8.drrp);
			printf("  Disable prefetch transfer length\t0x%x\n",
				((ms_page8.dpftl1<<8) + ms_page8.dpftl0));
			printf("  Minimum prefetch\t\t\t0x%x\n",
				((ms_page8.minpf1<<8) + ms_page8.minpf0));
			printf("  Maximum prefetch\t\t\t0x%x\n",
				((ms_page8.maxpf1<<8) + ms_page8.maxpf0));
			printf("  Maximum prefetch ceiling\t\t0x%x\n",
				((ms_page8.maxpfc1<<8) + ms_page8.maxpfc0));
		}
		else {
			printf("  Read cache disable bit\t\t0x%x\n",
						ms_page8.rc);
			printf("  Multiple selection bit\t\t0x%x\n",
						ms_page8.ms);
			printf("  Write cache enable bit\t\t0x%x\n",
						ms_page8.wce);
			printf("  Write retention priority\t\t%d\n",
						ms_page8.wrp);
			printf("  Demand read retention priority\t%d\n",
						ms_page8.drrp);
			printf("  Disable prefetch transfer length\t%d\n",
				((ms_page8.dpftl1<<8) + ms_page8.dpftl0));
			printf("  Minimum prefetch\t\t\t%d\n",
				((ms_page8.minpf1<<8) + ms_page8.minpf0));
			printf("  Maximum prefetch\t\t\t%d\n",
				((ms_page8.maxpf1<<8) + ms_page8.maxpf0));
			printf("  Maximum prefetch ceiling\t\t%d\n",
				((ms_page8.maxpfc1<<8) + ms_page8.maxpfc0));
		}
		byteptr += sizeof(ms_page8);
	}

page37_display:
	ms_page37 = *((struct page_code_37 *)byteptr);
	if(ms_page37.pgcode == 37) {
		if(ms_page37.pglength == 0) {
			byteptr += 2;
			printf("\n");
			return;
		}
		if(!moreoutput())
			return;
		printf("\nPage 37 DEC unique parameters:\n");
		printf("  PS\t\t\t\t\t%d\n", ms_page37.ps);
		printf("  Page code\t\t\t\t%d\n", ms_page37.pgcode);
		printf("  Page length\t\t\t\t%d\n", ms_page37.pglength);
		if(page_control == CHANGED_VALUES) {
			printf("  Spinup on power up\t\t\t0x%x\n", 
				ms_page37.spinup);
		}
		else {
			printf("  Spinup on power up\t\t\t%d\n", 
				ms_page37.spinup);
		}
		printf("\n");
	}
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
	"soft error",			/* 0x01 */
	"not ready",			/* 0x02 */
	"medium error",			/* 0x03 */
	"hardware error",		/* 0x04 */
	"illegal request",		/* 0x05 */
	"unit attention",		/* 0x06 */
	"write protected",		/* 0x07 */
	"blank check",			/* 0x08 */
	"vendor unique",		/* 0x09 */
	"copy aborted",			/* 0x0a */
	"aborted command",		/* 0x0b */
	"equal error",			/* 0x0c */
	"volume overflow",		/* 0x0d */
	"miscompare error",		/* 0x0e */
	"reserved",			/* 0x0f */
	0
};

#define MAX_SENSE_KEY_STR \
	(sizeof(rz_sense_key_str)/sizeof(rz_sense_key_str[0]))

char *rz_error_code_str[] = {
	"no sense",			/* 0x00 */
	"",				/* 0x01 */
	"no seek",			/* 0x02 */
	"write fault",			/* 0x03 */
	"drive not ready",		/* 0x04 */
	"drive not selected",		/* 0x05 */
	"no track zero found",		/* 0x06 */
	"",				/* 0x07 */
	"",				/* 0x08 */
	"",				/* 0x09 */
	"",				/* 0x0a */
	"",				/* 0x0b */
	"",				/* 0x0c */
	"",				/* 0x0d */
	"",				/* 0x0e */
	"",				/* 0x0f */
	"ID CRC or ECC error",		/* 0x10 */
	"unrecoverable read error",	/* 0x11 */
	"no address mark",		/* 0x12 */
	"no data field address mark",	/* 0x13 */
	"block not found",		/* 0x14 */
	"seek error",			/* 0x15 */
	"",				/* 0x16 */
	"recoverable read error",	/* 0x17 */
	"soft data error",		/* 0x18 */
	"defect list error",		/* 0x19 */
	"parameter overrun",		/* 0x1a */
	"",				/* 0x1b */
	"primary defect list not found",/* 0x1c */
	"compare error",		/* 0x1d */
	"",				/* 0x1e */
	"",				/* 0x1f */
	"invalid command",		/* 0x20 */
	"illegal block address",	/* 0x21 */
	"illegal command",		/* 0x22 */
	"",				/* 0x23 */
	"invalid cdb",			/* 0x24 */
	"invalid lun",			/* 0x25 */
	"invalid field in param list",	/* 0x26 */
	"write protected",		/* 0x27 */
	"",				/* 0x28 */
	"reset",			/* 0x29 */
	"mode select changed",		/* 0x2a */
	"",				/* 0x2b */
	"",				/* 0x2c */
	"",				/* 0x2d */
	"",				/* 0x2e */
	"",				/* 0x2f */
	"",				/* 0x30 */
	"medium format corrupted",	/* 0x31 */
	"no defect spare location",	/* 0x32 */
	"",				/* 0x33 */
	"",				/* 0x34 */
	"",				/* 0x35 */
	"",				/* 0x36 */
	"",				/* 0x37 */
	"",				/* 0x38 */
	"",				/* 0x39 */
	"",				/* 0x3a */
	"",				/* 0x3b */
	"",				/* 0x3c */
	"",				/* 0x3d */
	"",				/* 0x3e */
	"",				/* 0x3f */
	"RAM failure",			/* 0x40 */
	"",				/* 0x41 */
	"",				/* 0x42 */
	"message reject error",		/* 0x43 */
	"internal controller error",	/* 0x44 */
	"select/reselect failed",	/* 0x45 */
	"",				/* 0x46 */
	"scsi interface parity error",	/* 0x47 */
	"initiator detected error",	/* 0x48 */
	"inappropriate illegal message",/* 0x49 */
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
	int i;

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

	if(sense.snskey == SC_NOSENSE)
		return(NO_ERROR);

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

	printf("\n*** ERROR OCCURRED DURING COMMAND (%s) ***\n\n",rzcomstr);
	printf("(%s) error: sense key (0x%x): %s.\n",
		rzdisk, sense.snskey, print_sense_key(sense.snskey));
	printf("(%s) error: error code (0x%x): %s.\n",
		rzdisk, sense.asc, print_error_code(sense.asc));
	printf("(%s) error: sense = ", rzdisk);
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
