
#ifndef lint
static	char	*sccsid = "@(#)itc.c	4.1  (ULTRIX)        7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 * itc - image tape copy, tape to disk and back out
 *
 *	itc i /dev/nrmt##{l,m,h} > file
 *	itc o /dev/nrmt##{l,m,h} < file
 *
 * This program serves as an example of how to use the Ultrix N-buffered
 * I/O mechanism to buffer I/O requests to and from a tape device under
 * Ultrix. The user interface for the N-buffered I/O mechanism is
 * documented under nbuf(4) and works for any character device provided
 * that the device driver for the device has all the same routines
 * implemented and specified in the kernel file conf.c as do all the
 * Ultrix tape drivers. The symbol "##" above represents a tape drive
 * device unit number and the {l,m,h} is the density selection of
 * "l"ow, "m"edium, or "h"igh where applicable.
 *
 * Digital Equipment Corporation supplies this software example on
 * an "as-is" basis for general customer use.  Note that Digital
 * does not offer any support for it, nor is it covered under any
 * of Digital's support contracts.
 *
 */

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/devio.h>
#include <sys/mtio.h>

#define BUFAMT 128*512-1		/* Zero based 64 KB	*/
#define NBUFAMT 8			/* Number of N-buffers	*/
extern int errno;			/* Error number		*/

main(argc, argv)
int argc;
char **argv;
{
	int opt;			/* Options variable	*/
	int fd;				/* File descriptor	*/
	int cnt = NBUFAMT;		/* N-buffer count	*/
	int n;				/* Amount for I/O	*/
	char *buf[NBUFAMT];		/* N-buffer array	*/
	int eof;			/* End-of-file flag	*/
	int cnter = 0;			/* Counter		*/

	/* Check to be sure option was given */
	if( argc < 3 ) {
		fprintf( stderr,
			"Usage:\t itc i /dev/nrmt##{l,m,h} > file\n\t itc o /dev/nrmt##{l,m,h} < file\n" );
		exit( 1 );
	}

	/* Save the option */
	opt = argv[1][0];

	/* Check to see which option and open device */
	if(( fd = open( argv[2], opt == 'i' ? 0 : 1 )) < 0 ) {
		perror( "itc: tape open error" );
		exit( 1 );
	}

	/* Determine if we can use n-buffered I/O */
	if( ioctl( fd, FIONBUF, &cnt ) < 0 ) {
		cnt = 0;
	}

	/* Allocate buffers if N-buffered I/O is possible */
	if( cnt > 0 )
		for( n=0; n < cnt; n++ )
			buf[n] = (char *)malloc( BUFAMT+1024 );
		else
			buf[0] = (char *)malloc( BUFAMT );

	/* Determine which way */
	if( opt == 'i' ) {

		/* Copying from tape to a file */

		eof = 0;
		for( ;; ) {
loop:
		    n = read( fd, buf[cnter], BUFAMT );
		    if( cnt ) {
			int didread;

			if( ++cnter == cnt ) {
				cnter = 0;
			}

			errno = 0;
			didread = ioctl( fd, FIONBDONE, &buf[cnter] );

			if( errno != EINVAL ) {
				n = didread;
			} else {
				goto loop;
			}
		    }

		    if( n < 0 ) {
			perror( "itc: tape read error" );
			exit( 1 );
		    }

		    /* We encountered a tape mark */
		    /* Reset to next tape file */
		    if( n == 0 ) {
			close( fd );

			if( eof ) {
				break;
			} else {
				eof = 1;
			}

			/* Skip to next tape file */
			if(( fd = open( argv[2], opt == 'i' ? 0 : 1 )) < 0 ) {
				perror( "itc: tape open error" );
				exit( 1 );
			}

			if( cnt && ioctl( fd, FIONBUF, &cnt ) < 0 ) {
				cnt = 0;
			}
			cnter = 0;
		    } else {
			eof = 0;
		    }

		    write( 1, (char *)&n, sizeof( n ) );
		    if( n ) {
			write( 1, buf[cnter], n );
		    }
		}

	} else {
		int read_len;

		/* Copying from file to tape */

		while( read( 0, (char *)&n, sizeof( n )) == sizeof( n )) {

		    if( n == 0 ) {
			close( fd );

			fd = open( argv[2], 1 );

			if( cnt && ioctl( fd, FIONBUF, &cnt ) < 0 ) {
				cnt = 0;
			}

			cnter = 0;

		    } else {
			int err;

			if(( read_len = read( 0, buf[cnter], n )) != n ) {
				perror( "itc: file format error" );
				exit( 1 );
			}

			err = write( fd, buf[cnter], n );

			if( cnt && ( err >= 0 )) {
				int didwrite;

				if( ++cnter == cnt ) {
					cnter = 0;
				}

				errno = 0;
				didwrite = ioctl( fd, FIONBDONE, &buf[cnter] );

				if ( errno != EINVAL ) {
					err = didwrite;
				}
			}

			if( err < 0 ) {
				perror( "itc: tape write error" );
				exit( 1 );
			}
		    }
		}
		close( fd );
	}
}
