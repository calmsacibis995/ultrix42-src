
#ifndef lint
static	char	*sccsid = "@(#)snapcopy.c	2.1	(ULTRIX)	4/20/89";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/************************************************************************
 *									*
 *		VAX 8600/8650 Snapshot File Copy			*
 *									*
 *	usage:  snapcopy directory  					*
 *									*
 *		where <directory> is the directory to copy the 		*
 *		snapshot files to.					*
 *									*
 *		usage and debug messages > stdout			*
 *		error messages  > stderr				*
 *									*
 *		The snapshot files are placed in the specified		*
 *		directory and given the name:				*
 *									*
 *			hr:min:sec-snap1.dat				*
 *									*
 *			hr:min:sec-snap2.dat				*
 *									*
 *		An ASCII message is put into the error log that		*
 *		an snapshot file was logged				*
 *									*
 *	debug mode:  If 'DEBUG' is defined then the snap status		*
 *		     returned from the front end is printed.       	*
 *									*
 *									*
 *	Modification History:						*
 *									*
 *		28-Jan-88	Add -d command line option to invalidate*
 *				the snap shot files after they are      *
 *                              copied from the frontend. The default   *
 *                              will be to NOT delete.             swc  *
 ************************************************************************/

#include <stdio.h>
#include <sgtty.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/errlog.h>

#define COPY_1 "/etc/arff xmf /dev/crl snap1.dat"
#define COPY_2 "/etc/arff xmf /dev/crl snap2.dat"
#define GETSTAT 0x30		/* console command for snap file status */
#define INVAL_1 0x31		/* console command to invalidate snap1.dat */
#define INVAL_2 0x32		/* console command to invalidate snap2.dat */
#define ERROR -1
#define SUCCESS 0


struct timeval tv;
struct timezone tz;
struct tm *localtime();
struct tm *tp;
char rename_1[80];		/* These 2 arrays will hold the 2 command */
char rename_2[80];		/* strings to rename the files */
char message_1[80];		/* Error logger message for snap1.dat */
char message_2[80];		/* Error logger message for snap2.dat */
FILE *dev_fp;   		/* file pointer for virural term */
int  delete;			/* flag to delete snapfile, -d option */


struct sgttyb tty;		/* basic tty ioctl structure */



void copy_1() 			/* copy snap1 then invalidate it */
{
#ifdef DEBUG
	printf("copying snap1.dat\n");
#endif
	if ((system(COPY_1)) != 0)
	    fprintf(stderr,"snapcopy: arff can't copy snap1.dat\n");
	else
	    {
	     if (logerr(ELMSGT_SNAP8600, message_1) == ERROR)
		fprintf(stderr,"snapcopy: can't write to error log\n");
	     if (delete)
	         putc(INVAL_1,dev_fp); 	/* invalidate snap1 if -d option*/
	     system(rename_1);
	    }
}




void copy_2()			/* copy snap2 then invalidate it */
{
#ifdef DEBUG
	printf("copying snap2.dat\n");
#endif
	if ((system(COPY_2)) != 0)
	    fprintf(stderr,"snapcopy: arff can't copy snap2.dat\n");
	else
	    {
	     if (logerr(ELMSGT_SNAP8600, message_2) == ERROR)
		fprintf(stderr,"snapcopy: can't write to error log\n");
	     if (delete)
	        putc(INVAL_2,dev_fp);	/* invalidate snap2 if -d option*/
	     system(rename_2);
	    }
}


main(argc, argv)
int  argc;
char *argv[];

{
	static char v_term[] = "/dev/ttyc3";  /* console virtural terminal */
	int	snap_stat;	/* returned status from front end */
	int	dev_fd;		/* console virtural terminal file descriptor */
	int	arg;		/* argv array index */
	struct  stat stat_buf;  /* file status info sturcture */
	short	saved_flags;    /* copy of original terminal flags */

	snap_stat = 0;	/* initialize some stuff */
	arg = 1;
	delete = 0;

	/* A unique file name must be given to the files in the likely event
	 * that some snap files already exist, so that we don't loose the
	 * old ones. Make a file name from the current system time. Then we
	 * have to 'mv' them to this new name. The following builds
	 * a command string to do this, to be used with 'system()' later on.
	 */

	gettimeofday(&tv , &tz);
	tp = localtime(&tv.tv_sec);
	sprintf(rename_1,"/bin/mv snap1.dat %02d:%02d:%02d-snap1.dat",
		tp->tm_hour, tp->tm_min, tp->tm_sec);
	sprintf(rename_2,"/bin/mv snap2.dat %02d:%02d:%02d-snap2.dat",
		tp->tm_hour, tp->tm_min, tp->tm_sec);


	/* Process the arg list */

	if (argc < 2 || argc >3)     /* check for correct arg count */
		{
		 printf("\nUsage: snapcopy [-d] directory\n");
		 exit(ERROR);
	        }

	if ( argc > 2)
	   {
	     if ( strcmp(argv[arg], "-d") == 0)
	        {
	          delete = 1;	/* set delete frontend snap files flag */
	          arg = 2;      /* use 2nd comand line arg as directoy spec*/
	        }
	     else
	        {
	          printf("\nUsage: snapcopy [-d] directory\n");
	          exit(ERROR);
	        }
	    }


	/*
	 * Check accessability of path name given as the calling arguement
	 *
	 *		- does it exist
	 *		- is it a directory
	 *		- we can write to it
	 *
	 * If the above is ok then make it the current working directory.
	 * This will be the place where the snap files end up at.
	 */

	if ((stat(argv[arg],&stat_buf)) != 0)  /* is there such a place */
	       {
		fprintf(stderr,"snapcopy: %s does not exist\n",argv[arg]);
		exit(ERROR);
	       }
	if ((stat_buf.st_mode & S_IFDIR) != S_IFDIR)
	       {
		fprintf(stderr,"snapcopy: %s not a directory\n",argv[arg]);
		exit(ERROR);
	       }
	if ((stat_buf.st_mode & S_IWRITE) != S_IWRITE)
	   {
	    fprintf(stderr,"snapcopy: no write permission for %s\n",argv[arg]);
	    exit(ERROR);
	      }
	if (chdir(argv[arg]) != 0)
	       {
		fprintf(stderr,"snapcopy: could not chdir to %s\n",argv[arg]);
		exit(ERROR);
	       }
#ifdef DEBUG
	printf("directory checks done\n");
#endif

	/* Create the messages for the error logger */

	sprintf(message_1,"%02d:%02d:%02d-snap1.dat created in %s",
		tp->tm_hour, tp->tm_min, tp->tm_sec, argv[arg]);
	sprintf(message_2,"%02d:%02d:%02d-snap2.dat created in %s",
		tp->tm_hour, tp->tm_min, tp->tm_sec, argv[arg]);


	/* Check the accessability of the console virtural terminal 
	 *
	 *		- does it exist
	 *		- is it a character special file
	 *		- do we have write permission
	 *		- do we have read permission
	 *
	 * If all is ok then open it up for stream I/O.
	 */

	if ((stat(v_term,&stat_buf)) != 0)	/* is there such a place */
	       {
		fprintf(stderr,"snapcopy: %s does not exist\n",v_term);
		exit(ERROR);
	       }
	if ((stat_buf.st_mode & S_IFCHR) != S_IFCHR)
	       {
		fprintf(stderr,"snapcopy: %s not a character special file\n",
			v_term);
		exit(ERROR);
	       }
	if ((stat_buf.st_mode & S_IWRITE) != S_IWRITE)
	       {
		fprintf(stderr,"snapcopy: no write permission for %s\n",v_term);
		exit(ERROR);
	       }
	if ((stat_buf.st_mode & S_IREAD) != S_IREAD)
	       {
		fprintf(stderr,"snapcopy:no read permission for %s\n",v_term);
		exit(ERROR);
	       }

	/* open the virtural terminal for read/write */

	if ((dev_fp = fopen(v_term, "r+")) == NULL)  
		{
		 fprintf(stderr,"snapcopy: cant open %s\n",v_term);
		 exit(ERROR);
		}

	/* set the virtual terminal line to RAW mode */

	dev_fd = fileno(dev_fp);
	ioctl(dev_fd , TIOCGETP, &tty);	
	saved_flags = tty.sg_flags;
	tty.sg_flags = (tty.sg_flags& ~ECHO) | RAW; 
	ioctl(dev_fd,TIOCSETP, &tty);	
#ifdef DEBUG
	printf("/dev/ttyc3 opened\n");
#endif
	/*
	 * Tell the front end we want the status of the snap files.
	 * The front end responds with two bytes:
	 *       byte #1 = 0x30 data packet header 
	 *	 byte #2 = bit 0 (0) = snap1.dat invalid
	 *			 (1) = snap1.dat valid
	 *		   bit 1 (0) = snap2.dat invalid
	 *			 (1) = snap2.dat valid
	 */


#ifdef DEBUG
	printf("requesting status from frontend\n");
#endif
	putc(GETSTAT,dev_fp);	
	rewind(dev_fp);
	snap_stat = getc(dev_fp); 
	if (snap_stat != GETSTAT)
	    {
	     fprintf(stderr,"snapcopy: bad status header received: %x\n", 
		snap_stat);
	     exit(ERROR);
	    }
#ifdef DEBUG
	printf("first status byte: %x\n",snap_stat);
#endif
	snap_stat = getc(dev_fp); 
#ifdef DEBUG
	printf("second status byte: %x\n",snap_stat);
#endif
	rewind(dev_fp);

	/*
	 * If there are any valid snap files then 'arff' them
	 * to the path specified in the calling arguement, then
	 * tell the front end to invalidate them.
	 */

		
	switch (snap_stat)	
	      {
	       case 0 : break;  /* no valid snap files present */

	       case 1 : copy_1(); /* only snap1.dat valid */
			break;

	       case 2 : copy_2(); /* only snap2.dat valid */
			break;

	       case 3 : copy_1(); /* both valid */
		        copy_2();
			break;

	       default : fprintf(stderr,"snapcopy: invalid snap file status\n");
			 break;
	      }


	/* restore original mode to the virtual terminal line */

	tty.sg_flags = saved_flags;	
	ioctl(dev_fd,TIOCSETP,&tty);
	exit(SUCCESS);
}
