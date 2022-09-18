#ifndef lint
static	char	*sccsid = "@(#)kdbload.c	4.1	ULTRIX	7/2/90";
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

/************************************************************************
 *
 *	Revision History
 *
 *	  03-24-88: gmm
 *	  Exit with value 1 if the symbol table or strings table not big
 *	  enough to hold everything in the kernel
 *
 ************************************************************************/
#include <a.out.h>
#include <fcntl.h>
#include "kdb.h"
struct exec header;
struct nlist nl[6];
#define STRINGS	0
#define SYMBOLS	1
#define ETEXT	2
#define FILHDR	3
#define STACK	4
#define NULL	5
extern errno;

get_kdb_offsets()
{
	int phys_offset;
	/*
	 * get offsets for kdb_strings and kdb_symbol_table
	 */

	nl[STRINGS].n_un.n_name = "_kdb_strings";
	nl[SYMBOLS].n_un.n_name = "_kdb_symbol_table";
	nl[ETEXT].n_un.n_name   = "_etext";
	nl[FILHDR].n_un.n_name   = "_kdb_filhdr";
	nl[STACK].n_un.n_name   = "_kdb_stack";

	nlist ("vmunix", nl);
	if (nl[STRINGS].n_type == 0) {
		printf("no namelist\n");
		exit(1);
	}

	/*
	 * since we are not dealing with a running image,
	 * mask off the high bit
	 */
	nl[STRINGS].n_value &= 0x7fffffff;
	nl[SYMBOLS].n_value &= 0x7fffffff;
	nl[FILHDR].n_value  &= 0x7fffffff;
	nl[STACK].n_value  &= 0x7fffffff;

	/*
	 * text and data space must be multiples of 1024
	 * bytes, so for data, need to offset value in 
	 * file by (1024-(etext&0xff)) padding in text segment
	 * This is added to the offset of the header at
	 * the beginning of the file
	 */
	phys_offset = sizeof (struct exec) 
		- (0x400 - (nl[ETEXT].n_value & 0x3ff));
	nl[STRINGS].n_value += phys_offset;
	nl[SYMBOLS].n_value += phys_offset;
	nl[FILHDR].n_value  += phys_offset;
	nl[STACK].n_value  += phys_offset;

	printf("strings 0x%x \tsymbols 0x%x \tfilhdr \t0x%x stack 0x%x\n",
	    nl[STRINGS].n_value, nl[SYMBOLS].n_value,
	    nl[FILHDR].n_value, nl[STACK].n_value);
}

get_header(fd)
{
	/*
	 * gets the header information from the executable
	 */
	if (read(fd, &header, sizeof header) != sizeof header)
		perror("read_exec"), exit(errno);

/*
 *	printf("magic is %o\t", header.a_magic);
 *	printf("syms is %d\n", header.a_syms);
 *	printf("text is %d\t", header.a_text);
 *	printf("data is %d\t", header.a_data);
 *	printf("bss is %d\n", header.a_bss);
 */
}

open_file(which)
	char * which;
{
	int fd;
	fd = open(which, O_RDWR);
	if (fd == -1) 
		perror("open"), exit(errno);
	return(fd);
}

struct nlist *
get_symtab(fd)
{
	/*
	 * allocate space, and read in the symbol table
	 * from the file at fd
	 * Return a pointer to the allocated space
	 */
	struct nlist *symtab;
	if(header.a_syms > MAXSYMBOLS*sizeof(struct nlist)) {
		printf("Increase kdb symbol table: current = %d, required = %d\n",header.a_syms,MAXSYMBOLS*(sizeof(struct nlist)));
		exit(1);
	}
	symtab = (struct nlist *)malloc(header.a_syms);
	if (symtab == (struct nlist *) 0)
		perror("malloc_sym"), exit(errno);
	if (lseek(fd, N_SYMOFF(header), 0) == -1)
		perror("lseek_sym"), exit(errno);
	if (read(fd, symtab, header.a_syms) != header.a_syms)
		perror("read_sym"), exit(errno);
	return(symtab);
}


put_symtab(fd, symtab)
	struct nlist *symtab;
{
	/*
	 * take the symbol table from *symtab and put it
	 * into the file at fd, starting at location nl[SYMBOLS].n_value
	 */
	if (nl[SYMBOLS].n_value == 0) {
		printf("why is nl[SYMBOLS].n_value 0?\n");
		exit(1);
	}
	if (lseek(fd, nl[SYMBOLS].n_value, 0) < 0)
		perror("lseek-symbols"), exit(errno);
	if (write(fd, symtab, header.a_syms) != header.a_syms)
		perror("write-symtab"), exit(errno);
}

char *
get_strings(fd)
{
	/*
	 * allocate space, and read in the strings from fd
	 * Return a pointer to the allocated space
	 * Have to read in the string table size from 1st integer
	 * The first value in the strings table must be the size
	 * (The size includes the four bytes for the size
	 */
	int strsiz;
	char *strings;

	if (lseek(fd, N_SYMOFF(header)+header.a_syms, 0) < 0)
		perror("lseek-strsize"), exit(errno);
	if (read(fd, &strsiz, sizeof strsiz) != sizeof strsiz)
		perror("read_strsiz"), exit(errno);

	if(strsiz > MAXSYMBOLS * SYMBOL_SIZE) {
		printf("Increase kdb_strings size: current = %d, required = %d\n",MAXSYMBOLS*SYMBOL_SIZE, strsiz);
		exit(1);
	}
	strings = (char *)malloc(strsiz);
	if (strings == (char *) 0)
		perror("malloc_str"), exit(errno);
	*(int *)strings = strsiz;
	if (read(fd, (char *)((int)strings+4), strsiz-4) != strsiz-4)
		perror("read_str"), exit(errno);
	return(strings);
}

put_strings(fd, strings)
	char *strings;
{
	/*
	 * take the symbol table from *strings and put it
	 * into the file at fd, starting at location nl[STRINGS].n_value
	 */
	int strsiz = *(int *)strings;
	if (nl[STRINGS].n_value == 0) {
		printf("why is nl[STRINGS].n_value 0?\n");
		exit(1);
	}
	if (lseek(fd, nl[STRINGS].n_value, 0) < 0)
		perror("lseek-strings"), exit(errno);
	if (write(fd, strings, strsiz) != strsiz)
		perror("write-strings"), exit(errno);
}

put_filhdr(fd)
{
	int offset;
	/*
	 * the header information is used by the debugger
	 * The header is from 'header'
	 * It goes to nl[FILHDR].n_value
	 */
	if (nl[FILHDR].n_value == 0) {
		printf("why is nl[FILHDR].n_value 0?\n");
		exit(1);
	}
	offset = nl[FILHDR].n_value;
	if (lseek(fd, nl[FILHDR].n_value, 0) < 0)
		perror("lseek-filhdr"), exit(errno);
	if (write(fd, &header, sizeof header) != sizeof header)
		perror("write-header"), exit(errno);
}

char loadedbuf[] = "kdbloaded";

indicate_kdbloaded(fd)
{
	if (lseek(fd, nl[STACK].n_value, 0) < 0)
		perror("lseek-filhdr"), exit(errno);
	if (write(fd, loadedbuf, sizeof loadedbuf) != sizeof loadedbuf)
		perror("write-loadedbuf"), exit(errno);
}

main(argc,argv)
char *argv[];
{
	
	int fd;
	struct nlist *symtab;
	char *strings;
	
	if (argc>1) fd = open_file(argv[1]);
	else fd = open_file("vmunix");
	get_header(fd);
	get_kdb_offsets();

	symtab = get_symtab(fd);
	put_symtab(fd, symtab);

	strings = get_strings(fd);
	put_strings(fd, strings);

	put_filhdr(fd);
	indicate_kdbloaded(fd);
	exit(0);
}
