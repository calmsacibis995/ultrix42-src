/*
 * loadimage.c.c
 *
 * Revision History:
 *
 * Oct 09, 1990 -- Joe Szczypek
 *	Added TURBOchannel ROM support.
 */

#ifndef lint
static char *sccsid = "@(#)loadimage.c	4.3	(ULTRIX)	11/14/90";
#endif lint

#include "../h/param.h"
#include <a.out.h>
#include "../machine/mips/cpu.h"
#include "../machine/mips/entrypt.h"

#define printf _prom_printf

/*
 * format of bootable a.out file headers
 */
struct execinfo {
	struct filehdr fh;
	AOUTHDR ah;
};

extern int prom_io;
extern int ub_argc;
extern char **ub_argv;
extern int gub_argc;
extern char **gub_argv;
extern char **ub_envp;
extern char **ub_vector;
extern int rex_base;
extern int rex_magicid;

/*
 * getxfile -- load binary image
 */
load_image(io)
int io;
{
	struct execinfo ei;
	extern unsigned client_pc;

	if (read(io, &ei, sizeof(ei)) != sizeof(ei)) {
		printf("bad a.out format\n");
		goto bad;
	}
#ifdef notdef
	if (N_BADBO(ei.fh)) {
		printf("inappropriate byte order\n");
		goto bad;
	}
#endif notdef
	if (N_BADMAG(ei.ah)) {
		printf("bad magic number\n");
		goto bad;
	}
	lseek(io, N_TXTOFF(ei.fh, ei.ah), 0);
#ifdef SECONDARY
	printf ("\nSizes:\ntext = %d\n", ei.ah.tsize);
#endif SECONDARY
#ifdef notdef
	if (range_check(ei.ah.text_start, ei.ah.tsize))
		goto bad;
#endif notdef
	if (read(io, ei.ah.text_start, ei.ah.tsize)
	    != ei.ah.tsize) {
		printf("short read\n");
		goto bad;
	}
#ifdef SECONDARY
	printf ("data = %d\n", ei.ah.dsize);
#endif SECONDARY
#ifdef notdef
	if (range_check(ei.ah.data_start, ei.ah.dsize))
		goto bad;
#endif notdef
	if (read(io, ei.ah.data_start, ei.ah.dsize)
	    != ei.ah.dsize) {
		printf("short read\n");
		goto bad;
	}
#ifdef SECONDARY
	printf ("bss  = %d\n", ei.ah.bsize);
#endif SECONDARY
#ifdef notdef
	if (range_check(ei.ah.bss_start, ei.ah.bsize)) {
		/*
		 * minor hack: set client_pc and print intended entry point
		 * to make downloading lowprom easier (since downloading
		 * lowprom always fails on a range_check)
		 */
		client_pc = ei.ah.entry;
		printf("intended entry point: 0x%x\n", ei.ah.entry);
		goto bad;
	}
	bzero(ei.ah.bss_start, ei.ah.bsize);
#endif notdef
	if(!rex_base)
   	        _prom_close(prom_io);
#ifdef SECONDARY
	printf ("Starting at 0x%x\n\n", ei.ah.entry);
	if(rex_base) {
	        ub_argc = gub_argc;
	        ub_argv = gub_argv;
	}
#endif SECONDARY
	(*((int (*) ()) ei.ah.entry)) (ub_argc,ub_argv,ub_envp,ub_vector);
bad:
	return(-1);
}
