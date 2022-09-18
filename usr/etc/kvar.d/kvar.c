#ifndef lint
static	char	*sccsid = "@(#)kvar.c	4.2	(ULTRIX)	3/7/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1991 by			*
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


/*
 * mips-only kernel variable patch utility
 */

#ifdef mips

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/file.h>
#include	<ar.h>
#include	<symconst.h>
#include	<sym.h>
#include	<cmplrs/stsupport.h>
#include	<filehdr.h>
#include	<scnhdr.h>
#include	<ldfcn.h>
#include	<stamp.h>

char *myname;
LDFILE	*ldptr;
int kflag = 0;
int symbol = -1;
int rw = -1;
int vflag = 0;
extern char *optarg;
extern int optind, opterr;
int errflg = 0;
int size = -1;
int localsyms = 0;

fcheck(flag)
{
	if (flag != -1) usage();
}

usage()
{
	fprintf(stderr, "%s usage:\n",myname);
	fprintf(stderr,
 "    -s name -(r|w)(b|w|l) [-v value] [-o offset] [-l] a.out\n");
	fprintf(stderr,
 "    -a address -(r|w)(b|w|l) [-v value] [-o offset] [-l] a.out\n");
	fprintf(stderr,
 "    -k -s name -(r|w)(b|w|l) [-v value] [-o offset] [-l] vmunix\n");
	fprintf(stderr,
 "    -k -a address -(r|w)(b|w|l) [-v value] [-o offset] [-l] vmunix\n");
	fprintf(stderr,
 "where:\n");
	fprintf(stderr,
 "    r = read, w = write; b = 1 byte, w = 2 bytes, l = 4 bytes;\n");
	fprintf(stderr,
 "    value, address, and offset are positive decimal or hex constants.\n");
	fprintf(stderr,
 "use -k for in-memory copy of vmunix.\n");
	fprintf(stderr,
 "use -l for local symbol search (global is default).\n");
	(void) exit(1);
}

#define	DATATYPES	(5)
int sctypes[DATATYPES] = {scSData, scRData, scData, scBss, scSBss};

main(argc,argv)
	char **argv;
{
    	SYMR	sym;
	char *patch, *file;
	SCNHDR secthead;
	int fd, c;
	long value, location;
	long position, dot, oldvalue = 0;
	long getnum();
	
	myname = argv[0];
	while((c = getopt(argc, argv, "ls:a:r:w:kv:o:")) != EOF) switch(c) {
	case 'l' :
		localsyms = 1;
		break;
	case 'r' :
		fcheck(rw);
		rw = 1;
		goto getsize;
	case 'w' :
		fcheck(rw);
		rw = 0;
getsize:
		switch (optarg[0]) {
		case 'b' :
			size = 1;
			break;
		case 'w' :
			size = 2;
			break;
		case 'l' :
			size = 4;
			break;
		default : usage();
		}
		break;
	case 'a' :
		fcheck(symbol);
		location = getnum(optarg);
		symbol = 0;
		break;
	case 'v' :
		value = getnum(optarg);
		vflag++;
		break;
	case 'k' :
		kflag = 1;
		break;
	case 's' :	
		fcheck(symbol);
		symbol = 1;
		patch = optarg;
		break;
	case 'o' :
		dot = getnum(optarg);
		break;
	case '?' :
		usage();
	}

	if (((optind + 1)  != argc) || (rw == -1) || (size == -1) ||
		(symbol == -1) || (!rw && !vflag)) usage();
	else file = argv[optind];

	if ((ldptr = ldopen (file, NULL)) == NULL) {
		fprintf (stderr, "%s: Error: cannot open %s\n", 
			myname, file);
		return -1;
	} 
	else if (symbol && process(file,&sym,patch)) return;

	if (symbol) location = sym.value;
	dot = location + dot;

	if (symbol) {
		if (range(&secthead,sym.sc,dot)) {
badadd:
			fprintf(stderr,"%s: address 0x%x is illegal\n",
				myname,dot);
			(void) exit(1);
		}
	}
	else {
		register int itype;
	
		for (itype = 0; itype < DATATYPES; itype++)
			if (!range(&secthead,sctypes[itype],dot)) break;
		if (itype == DATATYPES) goto badadd;
	}
	
	ldclose(ldptr);

	file = kflag ? "/dev/kmem" : file;

	if ((fd = open(file,rw ? O_RDONLY : O_RDWR)) < 0) {
		(void) perror(file);
		(void) exit(1);
	}
	else if (!kflag && lseek(fd, secthead.s_scnptr, SEEK_SET) < 0) {
		(void) perror(file);
		(void) exit(1);
	}

	if (!kflag) position = dot - secthead.s_vaddr;
	else position = dot;

	if (!kflag && lseek(fd, (long) position, SEEK_CUR) == -1){
		(void) perror(file);
		(void) exit(1);
	}
	else if (kflag && lseek(fd, (long) position, SEEK_SET) == -1) {
		(void) perror(file);
		(void) exit(1);
	}

	if (!rw) {
		oldvalue = 0;
 		if (read(fd, &oldvalue, size) != size) {
			(void) perror(file);
			(void) exit(1);
		}

		(void) lseek(fd, (long)-size, SEEK_CUR);

		if (write(fd, &value, size) < 0) {
			(void) perror(file);
			(void) exit(1);
		}
		(void) lseek(fd, (long)-size, SEEK_CUR);
	}

	value = 0;
	if (read(fd, &value, size) != size) {
		(void) perror(file);
		(void) exit(1);
	}
	fprintf(stdout,"0x%x = 0x%x", dot, value);

	if (!rw)
		fprintf(stdout,", was 0x%x", oldvalue);

	fprintf(stdout, "\n");

	close(fd);
}

long
getnum(str)
	char *str;
{
	long val;

	if (!isdigit(*str)) usage();
	else if (str[0] == '0' && str[1] == 'x') {
		if (sscanf(&str[2], "%x", &val) == 0) usage();
	} else val = atoi(str);

	return (val);
}

range(scp,sc,dot)
	SCNHDR *scp;
	int sc,dot;
{
	char *name;

	switch (sc) {
	case scSData:
		name = ".sdata";
		break;
	case scRData:
		name = ".rdata";
		break;
	case scData: 
		name = ".data";
		break;
	case scSBss:
		if (!kflag) return -1;
		name = ".sbss";
		break;
	case scBss:
		if (!kflag) return -1;
		name = ".bss";
		break;
	default:
		fprintf(stderr,"%s: fatal error not possible\n",myname);
		(void) exit(1);
	}
	if (ldnshread(ldptr, name, scp) < 0) {
		fprintf(stderr,"%s: %s section", myname, name);
		(void) exit(1);
	}
	if (dot >= (scp->s_vaddr + scp->s_size) ||
		dot < scp->s_vaddr) {	
		return -1;
	}
	else return 0;
}

process (filename,sym,patch)
	char	*filename;
    	pSYMR	sym;
	char *patch;
{
	int isym, imax;
	char *name;
	extern char *ldgetname();

	if (PSYMTAB(ldptr) == 0) {
		fprintf (stderr, "%s: Error: no symbol table in %s\n",
			myname, filename);
		return -1;
	}

    	imax = SYMHEADER(ldptr).isymMax + SYMHEADER(ldptr).iextMax;
	isym = localsyms ? 0 : SYMHEADER(ldptr).isymMax;

	for (;isym < imax; isym++) {
		if (ldtbread (ldptr, isym, sym) != SUCCESS) {
			fprintf (stderr, 
			"%s: Error: cannot read the %d symbol in %s\n", 
			myname, isym, filename);
			return -1;
		}
		else if (sym->sc == scData || sym->sc == scSData ||
				sym->sc == scRData || (kflag && 
			(sym->sc == scBss || sym->sc == scSBss)));
		else continue;
		if ((name = ldgetname(ldptr, sym)) == NULL) {
			fprintf (stderr, 
			"%s: Error: cannot read the %d symbol name in %s\n", 
			myname, isym, filename);
			return -1;
		}
		else if (strcmp(name,patch) == 0) return 0;
	} 
	fprintf(stderr,"%s: symbol (%s) not found or not data\n",
		myname,patch);
	return -1;
} 

#endif /* mips */

#ifdef vax

main()
{
	printf("no vax support, use adb\n");
	exit(0);
}

#endif /* vax */

