
# ifndef lint
static char *sccsid = "@(#)tar.c	4.1      (ULTRIX)        7/2/90";
# endif not lint

/****************************************************************
 *								*
 *			Copyright (c) 1985, 1988 by		*
 *		Digital Equipment Corporation, Maynard, MA	*
 *			All rights reserved.			*
 *								*
 *   This software is furnished under a license and may be used *
 *   and copied  only  in accordance with the terms of such	*
 *   license and with the  inclusion  of  the  above  copyright *
 *   notice. This software  or	any  other copies thereof may	*
 *   not be provided or otherwise made available to any other	*
 *   person.  No title to and ownership of the software is	*
 *   hereby transferred.					*
 *								*
 *   The information in this software is subject to change	*
 *   without  notice  and should not be construed as a		*
 *   commitment by Digital  Equipment Corporation.		*
 *								*
 *   Digital assumes  no responsibility   for  the use	or	*
 *   reliability of its software on equipment which is not	*
 *   supplied by Digital.					*
 *								*
 ****************************************************************
 *
 *
 *	Modification/Revision history:
 *	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *
 *	revision			comments
 *	--------	-----------------------------------------------
 *	06-Jun-90	kegel
 *			Added code to use valloc() to allocatet aligned
 *			buffers - allows N-buffer IO to function on MIPS.
 *			Incremented revdec to 2 (tar V now prints 20.2).
 *
 *	25-Apr-90	kegel
 *			Added code to readtape.c to fix "append CLD".
 *			Incremented revdec to 1 (tar V now prints 20.1).
 *
 *	02-Mar-90	lambert
 *			Create new signal handler for SIGUSR1 which allows
 *			the user to toggle the vflag via kill(1).
 *
 *	31-Oct-89	rsp
 *			Moved globals to this file. Externs remain in tar.h.
 *
 *	16-May-89	rsp
 *			Added missing fprintf.
 *
 *	31-Aug-88	lambert
 *			Changed include line from "tar.h" to <tar.h> for
 *			POSIX support.
 *
 *	09-Jun-88	map
 *			Changed signal handlers to void.
 *
 *	15-Mar-88	mjk - added posix support
 *
 *	23-Sep-87	fries
 *			Corrected returned values from statchk().
 *
 *	11-Dec-86	fries
 *			Modified code in get_size to use either
 *			device /dev/rra#a or /dev/rra#c to get
 *			default disk information for sizing.
 *
 *	 9-Oct-86	fries
 *			Modified code in statchk to size disk devices
 *
 *	 2-Oct-86	fries
 *			Modified code in statchk to open character file
 *			when block is specified.
 *
 *	15-Sep-86	fries
 *			Modified code in statchk to open normal files
 *
 *	02-Jul-86	lp
 *			N-bufferring now works with eot.
 *
 *	19-Jun-86	lp
 *			Added n-buffered hooks. Did lots of profiling
 *			and found a number of stupid bottlenecks.
 *			Cleanup code & remove non-U32 ifdefs.
 *
 *	20.0		Jeff Fries, 15-May-86
 *			Added device generic ioctl support.
 *
 *	19.1		Ray Glaser, 28-Jan-86
 *			Do not allow non-super-user setting of pflag
 *
 *	19.0		Ray Glaser, Nov-85 - Jan-86
 *			Install multi-archive logic
 *			& /usr/group standard header logic
 *
 *	15.3		Ray Glaser, 24-Sep-85
 *			Correct the "-f" switch logic further. ie.
 *			Preserve the original modes/ownership of a
 *			named output file and do not allow overwrite
 *			of an existing write-protected "-f" output
 *			file. Also remove an extraneous carriage
 *			return on output that caused script problems
 *			and checksum errors when going over the net.
 *
 *	15.2		Ray Glaser, 09-Sep-85
 *			Correct bug in -f switch logic that allows
 *			root to destroy special files. ie. A check
 *			to see if a special file is named vs. a
 *			regular file.
 *
 *	15.1		Ray Glaser, 26-Aug-85
 *			Change output format of several items to
 *			avoid breaking scripts.
 *			1. On extract - print  blocks  vs. block(s)
 *			2. Put the "file" type indicator on modes
 *			   info under the big VERBOSE switch.
 *			   NOTE: This is for Ultrix-32 only. Ultrix-11
 *			   does it the way we have illustrated below.
 *
 *				ie.  drwx------
 *				     crw-------
 *				     brw-------
 *				     -rwx------
 *
 *				will come out as --
 *
 *				     rwx-------  etc..
 *			3. Put print of file size in blocks under
 *			   big VERBOSE switch.
 *
 *				ie..  720/001  for bytes/blocks
 *
 *				will be -
 *
 *				      720
 *
 *			4. Change print of uid/gid to print minimum
 *			   number of characters required.
 *
 *			   ie. It was printing	0/00   when the old
 *			   version had printed	0/0
 *
 *
 *	15.0		Ray Glaser, 29-Jul-85
 *			Fix a "broken pipe" problem when tar'ing out
 *			to stdout and tar'ing back in thru stdin, tar
 *			would "sometimes" exit with a broken pipe error.
 *			Due to a race condition when shutting down the
 *			pipe and the fact that the end of archive detect
 *			logic was not reading "both" eoa (all zeroes)
 *			blocks from the input tar, it would sometimes
 *			break the pipe.
 *
 *	14.0		Ray Glaser, 24-Jul=85
 *			Correct extraneous error message print out
 *			when extracting linked files. Caused by a
 *			small oversite when applying fix for
 *			IPR-00014.
 *
 *	13.0		Ray Glaser, 10-Jul-85
 *			Fixed the bug reported in IPR-00014.
 *			If a tar archive has a symbolic or hard which
 *			had the same name as an existing non-empty
 *			directory, an extraction of the link from the
 *			archive as  root  would unlink the directory from
 *			the system. All files in that directory would
 *			be allocated but unreferenced.
 *			This corrupts the filesystem and  fsck	must be
 *			run to repair it. Tar now outputs an error
 *			message and the directory is not removed.
 *
 *	12.0		Ray Glaser, 10-Jul-85
 *			Fixed a  bug identified by IPR-00006.
 *			If the same linked file was output twice by
 *			the user -
 *
 *			ie.	ln  alpha beta
 *			tar cvf tarfile alpha beta alpha
 *			tar xvf tarfile
 *
 *			Would produce error message that alpha was not
 *			found when the extract was done. The fix ignores
 *			subsequent occurances of alpha & outputs a
 *			message to the effect that the linked file has
 *			already been output & is being skipped.
 *
 *	11.0		Ray Glaser, 01-Jul-85
 *			Allow user to specify both  c and r  switch in
 *			the same command without complaint
 *			 (as it used to be).
 *			Fixed a bug that caused tar to hang in "bread"
 *			routine when zero byte read was encountered.
 *			Zero byte read indicates either eoa  or
 *			that an error occured on the read and no
 *			data is available.
 */
/*
 *	Modification history: (cont)
 *	~~~~~~~~~~~~~~~~~~~~
 *
 *	10.0		Ray Glaser, 22-May-85
 *			Fix bug in -p switch logic so that super user
 *			can restore original file ownership.
 *
 *	 09.1		Ray Glaser, xx-Mar-85
 *			Minor debug corrections to new code.
 *
 *	 09		Ray Glaser, 13-Mar-85
 *			Install conditionals and code for Ultrix-11.
 *
 *	 08		Ray Glaser, 15-Feb-85
 *			Install code for MDTAR function.
 *
 *	 07		Ray Glaser, 05-Nov-84
 *			- Replaced the 'k' switch with the 'h' switch.
 *			-or- Put tar back the way it was from Berkley
 *			vis: links....
 *
 *	 06		Ray Lanza,  21-Aug-84
 *			- removed special MicroVAX hacks.
 *
 *	 05		Greg Tarsa, 25-Apr-84
 *			- Fixed flushtape() to check status of final
 *			write to make sure that it succeeds.
 *
 *	 04		Greg Tarsa, 16-Apr-84
 *			- if MVAXI is defined then the default archive
 *			device	will be   /dev/rrx1.
 *
 *	 03		Greg Tarsa, 2-Mar-84
 *			- Changed -h flag to be the -k (keep symbolic
 *			links) flag.
 *
 *	 02		Greg Tarsa, 28-Nov-83
 *			- Fixed a bug that allowed SCCS files to be
 *			  extracted when FF was specified.
 *			- Made F[FF] to work with all command functions.
 *			- Code installed to allow -[cr]FFF to exclude
 *			  executable files.  This is tested, but
 *			  conditionalized out because the  -t & -x
 *			  options need more code to read magic numbers
 *			  from the file on tape.
 *
 *	 01		Greg Tarsa, 18-Nov-83
 *			- Fixed usage message to show proper functions.
 *			- Added units 2,3,6 to program.
 *			- Added miscellaneous comments.
 *			- Reversed sense of the -h flag (symlinks are
 *			  now followed by default)
 */
/*
 *
 *	File name:
 *
 *		tar.c
 *
 *	Source file description:
 *
 *		Contains root code for tar (if overlayed)
 *
 *	Functions:
 *
 *	main		Entry from system. Top level command
 *			processing and user error detection.
 *
 *	onhup,		Routines to catch "signals" and set the
 *	 onintr,	termination flag (term).
 *	  onquit,
 *	   onterm
 *
 *--------------------------
 */

#include <tar.h>
#include <sys/types.h>
#include <sys/fs.h>

struct devget mt_info;
struct stat stat_buf;
struct	DIRE	*Dhead; /* Head pointer to list of directories */
struct	passwd	*getpwnam();
struct	group	*getgrgid();
struct	group	*getgrnam();
struct	group	*gp;
struct linkbuf	*ihead;
struct linkbuf *lp;
struct	devget	mtsts;
struct	mtop	mtops;
struct	stat	stbuf;
int device_open = 0;
int revwhole = 20;
int revdec   =	2; /* Current revision # of tar.
		    * Printed out when the "V" switch is given.
		    */
int get_size();
char	Archive[8] = "archive";
daddr_t bsrch();
union	hblock	cblock;
char	CARCHS[3]  = "1";
int	chksum;
char	DIRECT[13]  = " (directory)";
union	hblock	dblock;
char	eoa[4]	= "EOA";
char	file_name[NAMSIZ+1];
int	found = 0;
daddr_t high;
char	hdir[MAXPATHLEN];
char	iobuf[TBLOCK];
daddr_t low;
char	magtape[1024] = DEFTAPE_RH;
char	mdtar[6] =  "mdtar";
int	njab;
char	NULS[1] = "";
void	onhup();
void	onintr();
void	onquit();
void	onterm();
void	onusr1();
char	*progname;
char	*rindex();
char	*sprintf();
char	*strcat();
char	*strfind();
union	hblock	*tbuf;
time_t	modify_time;
char	tname[15] = "/tmp/tarXXXXXX";
char	unitc;		/* Unit number character. Used so that we can
			 * accept unit numbers and device character
			 * specifiers in any order */
char	*usefile;
char	wdir[MAXPATHLEN];
int	is_posix = 0;
int	is_tape = 0;
FLAG	AFLAG	= 0;
FLAG	Bflag	= 0;
FLAG	bflag	= 0;
FLAG	cflag	= 0;
FLAG	DFLAG	= 0;
FLAG	dflag	= 0;
FLAG	EOTFLAG = 0;	/* End Of Tape flag */
FLAG	EODFLAG = 0;	/* End Of Disk flag */
FLAG	FEOT	= 0;	/* EOT detected in flushtape() */
FLAG	FILE_CONTINUES = 0;
FLAG	Fflag	= 0;
FLAG	fflag	= 0;
FLAG	first	= 0;
FLAG	hdrtype = UMA;	/* Defines the type of tar header block.
			 * See above defines or getdir();
			 */
FLAG	header_flag = 0;/* Normally true (1) when doing a multi-archive
			 * output to cause a continuation header for
			 * the current file to be written on the next
			 * archive. When a file EXACTLY fills the currnt
			 * physical media, this flag is set false (0)
			 * to inhibit the writting of a continuation
			 * header on the next archive as a NEW file will
			 * start fresh on the next archive.
			 */
FLAG	HELP;
FLAG	hflag	= 0;
FLAG	iflag	= 0;
FLAG	lflag	= 0;
FLAG	MDTAR	= 0;	/* Flags whether or not we are invoked
			 * with the name "MDTAR" so that the
			 * code can assume that the output device
			 * is likely to be a diskette.
			 * The code is sensitive to whether the
			 * output device is a tape or diskette.
			 */
FLAG	MFLAG = 0;
FLAG	mflag = 0;
FLAG	MULTI	= 0;
FLAG	new_file;
FLAG	nextvol;
FLAG	NFLAG	= 0;	/* Set -> No multi-archive or file-splitting
			 * across archives functions please.
			 * notset -> Perform multi-archive and
			 * file splitting across archives (default).
			 */
FLAG	NMEM4D	= 0;
FLAG	NMEM4L	= 0;
FLAG	OARCH	= 0;	/* Original archive number the current file
			 * began on. Used for multi-archive file
			 * operations.
			 */
FLAG	OFLAG = 0;
FLAG	oflag = 0;
FLAG	pflag	= 0;
FLAG	pipein	= 0;	/* Flag set if input from pipe. */
FLAG	PUTE	= 0;	/* Set when putting end of archive (empty)
			 * blocks out at end of all data
			 */
FLAG	rflag	= 0;
FLAG	SFLAG	= 0;	/* Flags that we want to output User Group
			 * standard archive format
			 */
FLAG	sflag	= 0;
FLAG	term	= 0;
FLAG	tflag	= 0;
FLAG	unitflag = 0;	/* Flags that a unit # has been given */
FLAG	VFLAG	= 0;	/* Big VERBOSE	requested flag */
FLAG	vflag	= 0;	/* Little verbose requested flag */
FLAG	Rflag	= 0;	/* The named files contain lists of files which are to
			 * be put into or extracted from the archive.
			 */
FLAG	volann	= -1;	/* Flags wether or not tar has announced the
			 * format / archive # of the current archive.
			 */
FLAG	VOLCHK	= 0;
FLAG	wflag	= 0;
FLAG	xflag	= 0;
SIZE_I	blocks;
SIZE_I	MAXAR = MAXARCHIVE;
SIZE_I	nblock	= DEF_NBLK; /* Number of blocks user desires in a single
			   * read/write system call.  */
SIZE_L	blocks_used = 0L;
SIZE_L	bytes;
SIZE_L	chctrs_in_this_chunk;
SIZE_L	cmtime;
SIZE_L	corgsize;
SIZE_L	cremain;
SIZE_L	extracted_size = 0L;/* Size of a file extracted/tabled so far.
			     * Used when dealing
			     * with files split across archives.
			     */
SIZE_L	original_size = 0L;/* Original size of a file if split
			    * across archives
			    */
SIZE_L	remaining_chctrs;/* Number of chctrs in the file that have not
			  * yet been placed on the output archive.
			  */
SIZE_L	written = 0L;	 /* Number of chctrs of the file written
			  */
SIZE_L	size_of_media[MAXARCHIVE+1];/* Default # of blocks on the device
				 * Zero assumes unlimited space.
				 */
COUNT_INDEX	CARCH = 1;	/* Current archive number reading/writting. */
COUNTER dcount1 = 0;
COUNTER dcount2 = 0;
COUNTER dcount3 = 0;
COUNTER lcount1 = 0;
COUNTER lcount2 = 0;
COUNTER start_archive = 1;	/* Number of the archive to start with
				 * on output. Primarily intended to be
				 * used for error recovery when dumping
				 * a lot of files across many media.
				 */
STRING_POINTER	getcwd();
STRING_POINTER	getwd();
FILE_D	mt;
INDEX	recno	= 0;
FILE	*tfile;

main(argc, argv)
	int	argc;
	char	*argv[];
{

	/*	Go parse the command
	 */
	chksum = parse(argc,argv);
	argv += chksum;

	if (rflag) {
		/* Go process an output to archive function.
		*/
		while (dorep(argv) == A_WRITE_ERR)
			;/*NOP*/

		done(SUCCEED);
	}

	/* Does user want to extract files from archive ?
	 */
	if (xflag)
		doxtract(argv); /*-yes- Go do the extract */
	else
		dotable();	/*-no-	Must be a table function */

	done(SUCCEED);

}


void
onhup()
{
	signal(SIGHUP, SIG_IGN);
	term++;
}

void
onintr()
{
	signal(SIGINT, SIG_IGN);
	term++;
	done(FAIL);
}

void
onquit()
{
	signal(SIGQUIT, SIG_IGN);
	term++;
	done(FAIL);
}

void
onterm()
{
	signal(SIGTERM, SIG_IGN);
	term++;
	done(FAIL);
}

void
onusr1()
{
	signal(SIGUSR1, onusr1);
	if (vflag) vflag = 0;
	else vflag++;
}

/* Routine to obtain generic device status */
statchk(tape,mode)
	char	*tape;
	int	mode;
{
	int to;
	int error = 0;
	char *cp;
	char tname[80];

	/* Determine if DEVICE or FILE */
	if((stat(tape,&stat_buf) >= 0) && (((stat_buf.st_mode & S_IFMT)
	    == S_IFCHR) || ((stat_buf.st_mode & S_IFMT) ==  S_IFBLK))) {

		/* Open device to obtain status */
		if(!device_open){

			/* Preset size of the media to 800 blocks */
			if(MDTAR && !sflag)
				set_size(800L);

			/* DISK BLOCK DEVICE SPECIFIED		   */
			/* If a Block Special, then modify name to */
			/* a Character Special file		     */
			if ((stat_buf.st_mode & S_IFMT) == S_IFBLK){
				cp = (char *)tape;
				strncpy(tname,tape,5);
				tname[5] = 'r';
				tname[6] = '\0';
				strcat(tname,cp+5);
			}
			else
				strcpy(tname,tape);

			to = open(tname,mode|O_NDELAY);

			/* If open error, then try to open the */
			/* block device and proceed	       */
			if (to < 0){
				to = open(tape,mode);
				if(to > 0)
					device_open = 1;
				return(to);
			}
		}

		device_open = 1;

		/* Get generic device status */
		if (ioctl(to,DEVIOCGET,(char *)&mt_info) < 0){
			if ((stat_buf.st_mode & S_IFMT) == S_IFBLK){
				close(to);
				to = open(tape,mode);
			}
			if(!sflag)
				set_size(get_size(tape));
			return(to);
		}

		if(mt_info.category == DEV_DISK)
			MDTAR = TRUE;
		else MDTAR = FALSE;

		is_tape = (mt_info.category == DEV_TAPE);
		/* Check for device on line */
		if(mt_info.stat & DEV_OFFLINE){
			fprintf(stderr,"\7\n%s: Place %s device unit #%u ONLINE\n",progname,mt_info.device,mt_info.unit_num);
			close(to);
			device_open = 0;
			error = 1;
			errno = 0;
			return(-1);
		} else

		/* Check for device write locked when in write mode */

			if((mt_info.stat & DEV_WRTLCK) && (mode != O_RDONLY)){
				fprintf(stderr,"\7\n%s: WRITE ENABLE %s device unit #%u\n",progname,mt_info.device,mt_info.unit_num);
				close(to);
				device_open = 0;
				error = 1;
				errno = 0;
				return(-1);
			}
		if ((stat_buf.st_mode & S_IFMT) == S_IFBLK){
			close(to);
			to = open(tape,mode);
		}
	} else {
		to = open(tape,mode);
		if ((to < 0) && (errno == ENOENT) && (mode != O_RDONLY) &&
		    cflag)
			to = open(tape,mode|O_CREAT,0666);
		return(to);
	}

	/* Re-Open as user requested */
	if(error)
		return(-1);

	if(!sflag)
		set_size(get_size(tape));
	return(to);
}


/* PROCEDURE: this does disk sizing based on the given */
/* partition. It uses the default disk partition table */
/* and indexes into it to get the size.		       */
long get_size(device)
	char *device;
{
	char *dev_name, tname[80];
	struct pt part;
	extern struct devget mt_info;
	int i,dsk_fd;

	if(!MDTAR)
	return(0L);

	/* If Block Device, then make it char device */
	if ((stat_buf.st_mode & S_IFMT) == S_IFBLK){
		if(dev_name = rindex(device,'/'))dev_name++;
		if(dev_name = rindex(device,'/'))dev_name++;

		strcpy(tname,"/dev/");
		strcat(tname,"r");
		strcat(tname,dev_name);
	} else
		strcpy(tname,device);

	/* Open the /dev/rra#a device to get */
	/* the default partition information */
	tname[strlen(tname)-1] = 'a';

	/* Templorarily open device to get device size */
	if((dsk_fd = open(tname,O_NDELAY|O_RDONLY)) < 0){

		/* If the 'a' device not available, then */
		/* try the 'c' device...		 */
		tname[strlen(tname)-1] = 'c';

		if((dsk_fd = open(tname,O_NDELAY|O_RDONLY)) < 0)
			goto err_out;

	}

	/* Get partition information */
	if (ioctl(dsk_fd,DIOCDGTPT,(char *)&part) < 0){
err_out:
		fprintf(stderr, "%s: ", progname);
		fprintf(stderr, "Unable to open device %s",tname);
		fprintf(stderr, " to get default parition info.\n");
		fprintf(stderr, "Setting disk device size to 800 blocks\n");

		/* Close Device */
		close(dsk_fd);

		return(800L);
	}

	close(dsk_fd);

	/* Calculate partition index */
	i = device[strlen(device)-1] - 'a';

	/* Return size of the partition */
	return(part.pt_part[i].pi_nblocks);
}
