#ifndef lint
static char *sccsid = "@(#)radisk.c	4.1    ULTRIX  7/2/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986-1989 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/***********************************************************************
 *
 *			Modification History
 *
 *	18-Dec-1987	Robin Lewis
 *		made changes to mscp defines to reflect the new "h" file
 *		that was added for port-class drivers.
 *
 *	27-Oct-1989	Tim Burke
 *		Added set and clear of exclusive access mode.
 *
 **********************************************************************/

#define DEBUG


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/param.h>
#include <ufs/fs.h>
#include <sys/dkio.h>
#include <sys/bbr.h>
#include <vaxsysap/mscp_msg.h>
#include <sys/ioctl.h>
#include <ctype.h>
#define BLOCKSIZE 512

/*
 * These definitions control whether to set the unit to exclusive access mode.
 */
#define NO_EXCLUSIVE	0
#define SET_EXCLUSIVE	1
#define CLEAR_EXCLUSIVE 2

char buf[BLOCKSIZE];

/*
 *	14-Sep-1988	Pete Keilty
 *	Correction to do_read subtract part_start from lbn, read from 
 *	starting lbn of the file system.
 *
 *	15-Dec-86	Robin
 *	Made changes to allow the scan option to work on controlers that don't
 *	have the mscp access command.  Also the special file must be the
 *	'c' partition except on the scan function.
 *
 */

/* This is a user level interface which allows a system administrator
 * to access the mscp disk and cause a block to be revectored, scan
 * the disk and report forced errors, clear forced errors, reset
 * the drive write protect software bit, and if I have time to print
 * out the controller type- micro code lev- ant the disk rct.
 *
 * The LBN used is the logical block number starting from the beginning
 * of the disk (512 byte blocks). If the input lbn is -1 then the beginning
 * of the specified partition is used.
 *
 * The length used is the number of blocks to process (512 byte blocks).
 * If the length is -1 then process to the end of the specified partition.
 *
 * The input flags are:
 *	-r	Causes the given lbn to be forced revectored by the controller.
 *		Only one lbn my be specified and the length is always 1.
 *	-s	Causes the controller to scan the disk for bad blocks.
 *	-c	Will clear forced error flags by reading the data ( the
 *		data is in an unknown state) and writing it back to the
 *		same block.  That will cause the data to be writen correctly
 *		or the block to be revectored and the data written in the
 *		new block.
 *	-u	This will reset the software write protect bit.
 *	-p	Will print the controller type, the micro code level and
 *		the rct (IF I HAVE TIME--A BELL)
 *	-e	Set exclusive access mode.
 *	-n	Clear exclusive access mode.
 *
 * The special file input must be a raw device and the lbn input must be
 * in that partition.  If the length specified exceeds the partitions
 * end point then the length will be the partitions end.
 */

long part_size, part_start;


main(argc,argv)
int argc;
char *argv[];
{

struct pt partition;
struct dkacc data;
int ioctlstat;
int part_input;
int fd;
int flag;
/* static char usage[] = "usage: radisk -rcsupen [LBN length] special_file"; */
static char usage[] = "usage: radisk -rcsen [LBN length] special_file";
char *special;
char c_part[265];
long lbn;
int i;
int length = -1;
struct stat statbuf;
int exclusive = NO_EXCLUSIVE;


if( argc < 3)
{
	fprintf(stderr,"%s\n",usage);
	return(-1);
}
special = argv[argc - 1];
if(stat(special,&statbuf) == -1)
{
	fprintf(stderr,"Can't access special file %s\n",special);
	return(-1);
}
/* We have to open the c partition to get the partition table
 * information no matter what the input device partition was
*/
strcpy(c_part,special);
c_part[ strlen(c_part) - 1] = 'c';
argc--;
argv++;
if( argc < 2)
{
	fprintf(stderr,"%s\n",usage);
	return(-1);
}
if(*argv[0] != '-')
{
	fprintf(stderr,"%s\n",usage);
	return(-1);
}
switch (argv[0][1]) 
	{
			/* Toggle the sofwrae write protect bit in the drive */
	case 'u':
/*		data.dk_opcode = ACC_UNPROTECT; */
		return;
		break;
			/* Print out the controller, micro code lev. and rct */
	case 'p':
/*		data.dk_opcode = ACC_PRINT;*/
		return;
		break;
			/* Force clear the forced error bit */
	case 'c':
		data.dk_opcode = ACC_CLEAR;
		argc--;
		argv++;
		for (i=0; i < strlen(*argv); i++)
		{
			if (( isdigit(argv[0][i]) == 0 )&& (argv[0][i] != '-'))
			{
				fprintf(stderr,"lbn must be an integer\n");
				return(-1);
			}
		}
		lbn = atoi(*argv);
		argv++;
		for (i=0; i < strlen(*argv); i++)
		{
			if (( isdigit(argv[0][i]) == 0 )&& (argv[0][i] != '-'))
			{
				fprintf(stderr,"length must be an integer\n");
				return(-1);
			}
		}
		length = atoi(*argv);
		break;
			/* Force re-vector a disk block */
	case 'r':
		data.dk_opcode = ACC_REVEC;
		argc--;
		argv++;
		for (i=0; i < strlen(*argv); i++)
		{
			if (( isdigit(argv[0][i]) == 0 )&& (argv[0][i] != '-'))
			{
				fprintf(stderr,"lbn must be an integer\n");
				return(-1);
			}
		}
		lbn = atoi(*argv);
		break;
			/* Scan a disk for bad blocks, replace and report them */
	case 's':
		data.dk_opcode = ACC_SCAN;
		argc--;
		argv++;
		for (i=0; i < strlen(*argv); i++)
		{
			if (( isdigit(argv[0][i]) == 0 )&& (argv[0][i] != '-'))
			{
				fprintf(stderr,"lbn must be an integer\n");
				return(-1);
			}
		}
		lbn = atoi(*argv);
		argv++;
		for (i=0; i < strlen(*argv); i++)
		{
			if (( isdigit(argv[0][i]) == 0 )&& (argv[0][i] != '-'))
			{
				fprintf(stderr,"length must be an integer\n");
				return(-1);
			}
		}
		length = atoi(*argv);
		break;
	case 'e':	/* Set exclusive access mode */
		exclusive = SET_EXCLUSIVE;
		break;
	case 'n':	/* Clear exclusive access mode */
		exclusive = CLEAR_EXCLUSIVE;
		break;
	}
	if(data.dk_opcode != ACC_SCAN)
	if(special[strlen(special) - 1] != 'c')
	{
		fprintf(stderr,"special file for 'c' partition required\n");
		return;
	}

/* open the c partition to get the partition table */
if((fd=open(c_part,O_RDWR)) < 0)
{
	perror("radisk: open for partition table");
	fprintf(stderr,"can't open %s\n",special);
	return(-1);
}
/* get the partition tabe info */
if(ioctl(fd, DIOCGETPT, (char *) &partition) < 0)
{
	perror("radisk: can't get partition table from driver");
	exit(-1);
}
close(fd);

/* open the partition that the user specified */
if((fd=open(special,O_RDWR)) < 0)
{
	perror("radisk: open");
	fprintf(stderr,"can't open %s\n",special);
	return(-1);
}

/* Exclusive access set and clear doesn't care about partition info; just
 * do the ioctl, give status and then exit.
 */
if (exclusive != NO_EXCLUSIVE) {
	return(set_exclusive(fd, exclusive)); 
}

/* Make sure the LBN is in the specified partition and that the length
 * does not exceed the end of that partition 
 */

part_input=(char) special[strlen(special) - 1];
part_size=partition.pt_part[(special[strlen(special) - 1] - 'a') ].pi_nblocks;
part_start=partition.pt_part[(special[strlen(special) - 1] - 'a') ].pi_blkoff;


if((lbn == -1) || (lbn == 0))
	lbn = part_start;

/* Make sure lbn is in the partition of the special device. */


if(lbn < part_start )
{
	printf("Input lbn %d is not in the %s partition\n",lbn,special);
	exit(-1);
}
if((part_start + part_size) < lbn)
{
	printf("Input lbn %d is not in %s the partition\n",lbn,special);
	exit(-1);
}
/*
calculate the offset from partition of special device input to the lbn,
if lbn is -1 offset = 0 and if length is -1 the go to end of partition.
*/
if ( length == -1)
	length = part_start + part_size - lbn;
if( (part_start + part_size) < (lbn + length) )
{
	length = part_start + part_size - lbn;
	printf("WARNING length input exceeded the partition, the length\n\twas changed to be the partitions end\n");
}

/* load data for ioctl call and then process the ioctl */

data.dk_lbn = lbn;
data.dk_length = length  * 512 ;
data.dk_status = -1;

switch ( data.dk_opcode)
{
	case ACC_CLEAR:
		ioctlstat=do_clear(fd,lbn - part_start,length);
		break;
	case ACC_SCAN:
		fprintf(stderr,"Scan started :\n\n");
		ioctlstat=do_scan(fd,&data);
		fprintf(stderr,"\n");
		break;
	case ACC_REVEC:
		ioctlstat=ioctl(fd, DKIOCACC, (char *) &data);
		break;
/*	
	case ACC_PRINT:
		ioctlstat=ioctl(fd, DKIOCACC, (char *) &data);
		break;
	case ACC_UNPROTECT:
		ioctlstat=ioctl(fd, DKIOCACC, (char *) &data);
		break;
*/
}
	if (ioctlstat == -1) perror("radisk");
}

/*
 * do_clear reads a block of data and writes it back out to the same place on
 * the disk.  The write will force the data to a good block if the block
 * is a bad block or it will clear a forced error flag if it was set.  If
 * the forced error was set the data is in an unknkown state.
 *
 * This is not a good thing to do unless you REALY know what you are up to
 * the file system should be restored after using this because the data
 * will be in an unknown state.
 */

do_clear(fd,lbn,length)
int fd;
long lbn, length;
{
	int charcnt;
	for (;length > 0;length--)
	{
		if((lseek(fd,(lbn*512),L_SET)) == -1)
		{
			perror("radisk: seek error, process aborted \n");
			exit(-1);
		}
		charcnt=read(fd, buf, BLOCKSIZE);
		if((lseek(fd,(lbn * 512),L_SET)) == -1)
		{
			perror("radisk: seek error, process aborted \n");
			return(-1);
		}
		if((charcnt=write(fd, buf, BLOCKSIZE)) == -1)
		{
			perror("radisk: write error, process aborted \n");
			return(-1);
		}
		lbn++;
	}
return(0);
}

/* Send the mscp_scan command to the controller.  If it finds a bad block
 * it will return a bad block reported after it has revectored the block.
 * We should restart the scan, this will continue until the disk is compleatly
 * checked.  The scan starts up where the use indicated and not where
 * the error was found becaus we want to end with an error free scan of
 * the entire disk section. I think errors should be far and few between
 * so the compleat restart will not cause large delays.  If thats not
 * true we may want to change the logic to restart near the found bad block.
 */

do_scan(fd,data)
int fd;
struct dkacc *data;
{
	int ioctlstat;
	struct dkacc localdata;
	localdata.dk_opcode = data->dk_opcode;
	localdata.dk_lbn = data->dk_lbn;
	localdata.dk_flags = data->dk_flags;
	localdata.dk_status = data->dk_status;
	localdata.dk_length = data->dk_length;

	ioctlstat=ioctl(fd, DKIOCACC, &localdata);
	if(ioctlstat == -1) 
		return(ioctlstat);
	if(localdata.dk_status == NO_ACCESS)
	{
		do_read(fd,data->dk_lbn,data->dk_length);
		return;
	}
	if(localdata.dk_status != MSCP_ST_SUCC)
	{
		/* If a bad block is found, print it's LBN and restart the scan. */
		if(localdata.dk_flags & MSCP_EF_BBLKR)
		{
			fprintf(stderr,"%d replaced\n",localdata.dk_lbn);
			do_scan(fd,data);
		}
		/* On forced error report that it was found, skip it and keep going*/
		if(localdata.dk_status & (MSCP_ST_DATA | (MSCP_SC_FRCER << MSCP_ST_SBBIT)))
		{
		/* compute the lbn that had the error from the byte count returned */
			localdata.dk_lbn = data->dk_lbn + (long) (localdata.dk_length / BLOCKSIZE);
			fprintf(stderr,"forced error set at LBN %d \n",localdata.dk_lbn);
			localdata.dk_opcode = data->dk_opcode;
			localdata.dk_lbn++;
			localdata.dk_flags = data->dk_flags;
			localdata.dk_status = data->dk_status;
			localdata.dk_length = data->dk_length - (localdata.dk_length) - BLOCKSIZE;
			do_scan(fd,&localdata);
		}
		else
		{
			fprintf(stderr,"Aborted mscp status is o%o flags are o%o\n",
				localdata.dk_status,
				localdata.dk_flags);
		}
	}
	return(ioctlstat);
}

/* do_read is a function that simulates the scan option for disks that
   do not support the mscp access function.  Just read everything and let the
   driver report any problems.
*/

do_read(fd,lbn,length)
int fd;
long lbn, length;
{
	char buffer[BLOCKSIZE];
	int charcnt;
	/* change length in bytes to length in sectors */
	length = (length/512);
	/* convert lbn to fs block number */
	lbn -= part_start;
	for (;length > 0;length--)
	{
		if((lseek(fd,(lbn*512),L_SET)) == -1)
		{
			perror("radisk: seek error, process aborted \n");
			exit(-1);
		}
		if((read(fd, buffer, BLOCKSIZE)) == -1)
		{
			perror("radisk: read error, process aborted \n");
			exit(-1);
		}
		lbn++;
	}
}
/* set_exclusive is used to set and clear the exclusive access mode.  This
 * routine returns 0 on success and -1 on failure.
 */
int
set_exclusive(fd, exclusive)
	int fd, exclusive;
{
	int ioctl_val = 0;
	if (fd < 0) {		/* paranoia */
		fprintf(stderr,"Bad fd in set_exclusive, %d", fd);
		return(-1);
	}
	switch (exclusive) {
		case SET_EXCLUSIVE:	ioctl_val = 1; break;
		case CLEAR_EXCLUSIVE:	ioctl_val = 0; break;
		default:		
			fprintf(stderr,"Bad exclusive operation, %d",exclusive);
			return(-1);
	}
	if (ioctl(fd, DKIOCEXCL, &ioctl_val) < 0) {
		perror("radisk: DKIOCEXCL failed");
		fprintf(stderr,"Exclusive access mode change failure.\n");
		return(-1);
	}
	return(0);
}
