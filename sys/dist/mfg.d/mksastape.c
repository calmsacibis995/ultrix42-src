/*
 * mksastape.c
 */
#ifndef lint
static	char	*sccsid = "@(#)mksastape.c	4.1 (ULTRIX) 7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 * This program builds a bootable image which can be copied to tape.  It
 * is for use on the PMAX workstation.  Block 0 is a bootblk which
 * describes the appended image.  Block sizes must be 512 bytes.
 */
#ifdef mips
#include <stdio.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/exec.h>
#include <sys/buf.h>
#include "../../sas/mips/bootblk.h"

#define BLKSZ 512

struct header {
	struct exec ex;
	struct scnhdr sh;
};

#endif mips
main(argc, argv)
	int argc;
	char **argv;
{
#ifdef mips
	struct header hdr;
	int i, io_in, io_out;
	char buf[BLKSZ];
	struct bootblock bblk;

	argc--, argv++;

	if (argc < 2) {
		fprintf(stderr, "Usage: mksas kernel_image tape_image\n");
		exit(1);
	}
	if ((io_in = open(argv[0], O_RDONLY)) < 0) {
		fprintf(stderr, "Can't open %s\n", argv[0]);
		exit(1);
	}
	if ((i = read(io_in, (char *)&hdr, sizeof hdr)) != sizeof hdr) {
		fprintf(stderr, "Can't read %s\n", argv[1]);
		exit(1);
	}
	if (hdr.ex.ex_o.magic != OMAGIC) {
		fprintf(stderr, "Only OMAGIC supported\n");
		exit(1);
	}
	if ((lseek(io_in, hdr.sh.s_scnptr, 0)) < 0) {
		fprintf(stderr, "can't lseek\n");
		exit(1);
	}
	if ((io_out = open(argv[1], O_CREAT|O_WRONLY, 0644)) < 0) {
		fprintf(stderr, "Can't open %s\n", argv[1]);
		exit(1);
	}

	/* 
	 * build a bootblk and write it to the output 
	 */
	bzero((char*)&bblk, sizeof bblk);
	bzero(buf, BLKSZ);
	bblk.magic = BB_MAGIC;
	bblk.type = BB_CONTIGUOUS;
	bblk.bb.bb0.ladr = hdr.ex.ex_o.entry;
	bblk.bb.bb0.sadr = hdr.ex.ex_o.entry;
	bblk.bb.bb0.bcnt = 
	    (hdr.ex.ex_o.tsize + hdr.ex.ex_o.dsize + BLKSZ-1) / BLKSZ;
	bblk.bb.bb0.sblk = 1;
	bcopy((char*)&bblk, buf, sizeof bblk);

	if ((write (io_out, buf, BLKSZ)) != BLKSZ) {
		fprintf(stderr, "%s: write failed\n", argv[1]);
		exit(1);
	}

	/* 
	 * Now, append the rest of the text/data to the bootblk
	 * in the output file
	 */
	for (i = 0 ; i < bblk.bb.bb0.bcnt; i++) {
		if ((read (io_in, buf, BLKSZ)) <= 0) {
			fprintf(stderr, "%s: read failed\n", argv[0]);
			exit(1);
		}
		if ((write (io_out, buf, BLKSZ)) != BLKSZ) {
			fprintf(stderr, "%s: write failed\n", argv[1]);
			exit(1);
		}
	}
	close(io_out);
	close(io_in);
#endif mips
}
