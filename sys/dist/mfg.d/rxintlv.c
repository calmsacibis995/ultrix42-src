/*
 * rxintlv.c
 */
#ifndef lint
static	char	*sccsid = "@(#)rxintlv.c	4.1 (ULTRIX) 7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
/*
 * This utility takes an input image and interleaves sectors on the
 * output file. Its function in intended for use when build RX01
 * floppies which will be read using VMB.EXE's floppy boot driver.
 */
#include <stdio.h>
#include <sys/file.h>
#include <sys/param.h>
#include <a.out.h>
#include <sys/buf.h>

char	buf[128];			/* size of RX01 sector */

main(argc, argv)
	int argc;
	char **argv;
{
	int bc, i, in, out;

	argc--, argv++;
	if (argc < 2) {
		fprintf(stderr, "Usage: rxintlv in_file out_file\n");
		exit(1);
	}
	if ((in = open(argv[0], O_RDONLY)) < 0) {
		fprintf(stderr, "Can't open input: %s\n", argv[0]);
		exit(1);
	}
	if ((out = open(argv[1], O_CREAT|O_RDWR, 0640)) < 0) {
		fprintf(stderr, "Can't open output: %s\n", argv[1]);
		exit(1);
	}
	/*
	 * fill and zero track 0
	 */
	for (i=0; i<26; i++){
		if ((write (out, buf, sizeof buf)) != sizeof buf) {
			fprintf(stderr, "Can't write output: %s\n", argv[1]);
			exit(1);
		}
	}
	i = 0;
	while (1) {
		bc = read (in, buf, 128);
		if (bc > 0) {
			secseek(out, i);
			if ((write (out, buf, 128)) != 128) {
				fprintf(stderr, "Can't write output: %s\n", argv[1]);
				exit(1);
			}
			i++;
			continue;
		}
		if (bc < 0) {
			fprintf(stderr, "Can't read input:  %s\n", argv[0]);
			exit(1);
		}
		break;
	}
	close(in);
	close(out);
}

secseek (file, sector)
	int file, sector;
{
    short
	pcyl,           /*  Physical cylinder number.               */
	psec;           /*  Physical sector number.                 */

    int
	bytoff;         /*  Physical byte offset.                   */

    /*
     *  Compute the uncorrected physical sector and cylinder.
     */
    pcyl = sector / 26;
    psec = sector % 26;         /*  0, 1, 2, ..., 25                */
    /*
     *  Correct for 2:1 interleave.
     */
    psec <<= 1;                 /*  0, 2, 4, ..., 50                */
    if (psec >= 26) {
	psec -= 25;             /*  0, ..., 24, 1, ..., 25          */
    }
    /*
     *  Correct for 6 sector/cylinder skew.
     */
    psec += 6 * pcyl;
    psec %= 26;                 /*  0, ..., 25                      */
    /*
     *  Correct for start at cylinder 1, not 0.
     */
    pcyl ++;
    /*
     *  Correct for wrap around to cylinder 0.
     */
    if (77 == pcyl) {
	pcyl = 0;
    }
    /*
     *  Compute byte offset into device.
     */
    bytoff = (off_t) (26 * pcyl + psec) * (off_t) 128;
    /*
     *  Properly seek.
     */
    if (-1 == lseek (file, bytoff, 0)) {
	fprintf (stderr, "seek to output sector %d failed\n", bytoff/128);
	exit (1);
    }
}
