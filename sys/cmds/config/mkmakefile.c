#ifndef lint
static	char	*sccsid = "@(#)mkmakefile.c	4.3	(ULTRIX)  9/4/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988-1990 by			*
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
 *	Modification History
 *
 * 20-Aug-90	Matt Thomas
 *	Modified mkmakefile to attempt to read in file.<name> for all pseudo
 *	devices.
 *
 * 08-Dec-89 	Pete Keilty
 *	Modified the making of the loadvmunix line by adding NEW ld flag
 *	-B to page align BSS space. SPT.o NEEDS to be the first object
 *	loaded so as to be page aligned. 
 *	VAX SRM requirement and the CIXCD adapter checks.
 *
 * 10-Nov-89	jaw
 *	make file wrong for profiling kernel.
 *
 * 19-July-89	robin
 *	Added vax device support to mips systems
 *
 * 12-Jun-89   gg
 *	Added configurable virtual memory parameters support.
 *	(like vasslop, maxdsiz, maxssiz, maxretry and swapfrag).
 *	Removed virtual memory configurable parameters dmmin and dmmax.
 *
 * 07-March-89  gmm
 *	Fixes for including kdb (for the new path names)
 *
 * 17-Feb-89	map (Mark Parenti)
 *	Make sure swapgeneric.o: section is generated for VAX.
 *
 * 13-Feb-89	map (Mark Parenti)
 *	Change #ifdef's to switches.
 *	Fix generation of swap files for vax.
 *
 * 02-Feb-89	map (Mark Parenti)
 *	Add machine switch for s-file compile line
 *
 * 13-Jun-88	chet
 *	Added configurable buffer cache support.
 *
 * 2-17-87	 Larry C.
 *	Fix bug in read_files that caused a core dump if files.SYSNAME did
 *	not have a duplicate entry that would override and entry in the
 *	default "files" files.
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added portclass support to the system.
 *
 * 16-Apr-87 -- fred (Fred Canter)
 *	Removed the printtext.s kludge (found a better way).
 *	Added a function to make printtext.s.
 *	Added, but not kludged, same function to build QD template
 *	ram .h and .c files.
 *
 * 14-Apr-87 -- prs
 *	Added login to read in the contents of the filesystems file.
 *	Also, verify that at least one file system listed in filesystems
 *	file is configured into kernel.
 *
 * 19-Mar-87 -- fred (Fred Canter)
 *	X in the kernel broke the 150 files limit, raised it to 300.
 *	Added a kludge needed to build one of the monochrome
 *	device dependent X files (printtext.s).
 *
 * 06-Aug-86 -- prs
 *	Added changes to support the merging of the swapboot and
 *	swapgeneric files.
 *
 * 15-Apr-86 -- afd
 *	Removed references to MACHINE_MVAX.
 *
 * 01 Apr 86 -- depp
 *	Added in shared memory configurable items
 *
 * 05-Mar-86 -- bjg
 *	Removed msgbuf.h dependency (for assym.s);
 *	msgbuf removed from kernel
 *
 * 25-Feb-86 -- jrs
 *	Changed to allow multiple "needs" files per files.* line
 *	Changed to support "swap on boot" similar to "swap on generic"
 *	Also changed maxusers check to warning rather than hard bound of 8
 *
 * 05-Feb-86 -- jrs
 *	Moved cpu aliasing out to parser for earlier resolution
 *
 * 08 Aug 85 -- darrell
 *	Added rules for making genassym.c.
 *
 * 19-Jul-85 -- tresvik
 *	Increased maxusers limit from 128 to 256 for larger systems.
 *	Also, force on -DVAX8600 whenever VAX8650 is defined as a cpu.
 *
 * 18 Jun 85 -- reilly
 *      Fix the emulation problem when doing the -s option in
 *      the target directory. 
 *
 * 24 Mar 85 -- depp
 *	Added new config specs "release" and "version".  Since this is
 *	a System V convention, release indicates 1.0, 1.1, 1.2, 2.0, etc.
 *	Version indicates a subrelease 0, 1, 2, ...
 *
 * 11 Apr 85 -- depp
 *	Modified do_cfiles to restrict the size of CFILES.  Additional
 *	files are now placed in CFILES1.
 *
 * 22 Mar 85 -- reilly
 *	Added code for the EMULFLT option ( float point emulation )
 *
 * 06 Mar 85 -- reilly
 *	Modified so that the option BINARY is done correctly.
 *
 * 25 Oct 84 -- rjl
 *	Added support for  MicroVAX binary kits. In line emulation code
 *	and name change to BINARY.machinename format for BINARY directory.
 */

/*
 * Build the makefile for the system, from
 * the information in the files files and the
 * additional files for the machine being compiled to.
 */

#include <stdio.h>
#include <ctype.h>
#include "y.tab.h"
#include "config.h"
#include <machine/param.h>
#include <machine/vmparam.h>

/* largest segment that we currently support */
#define	GIG	1024	/* units of 1 meg */

int	source = 0;
int	kdebug = 0;
static	struct file_list *fcur;
char *tail();

/*
 * Lookup a file, by make.
 */
struct file_list *
fl_lookup(file)
	register char *file;
{
	register struct file_list *fp;

	for (fp = ftab ; fp != 0; fp = fp->f_next) {
		if (eq(fp->f_fn, file))
			return (fp);
	}
	return (0);
}

/*
 * Lookup a file, by final component name.
 */
struct file_list *
fltail_lookup(file)
	register char *file;
{
	register struct file_list *fp;

	for (fp = ftab ; fp != 0; fp = fp->f_next) {
		if (eq(tail(fp->f_fn), tail(file)))
			return (fp);
	}
	return (0);
}

/*
 * Make a new file list entry
 */
struct file_list *
new_fent()
{
	register struct file_list *fp;
	int i;

	fp = (struct file_list *) malloc(sizeof *fp);
	for (i = 0; i < NNEEDS; i++) {
		fp->f_needs[i] = 0;
	}
	fp->f_next = 0;
	fp->f_flags = 0;
	fp->f_type = 0;
	if (fcur == 0)
		fcur = ftab = fp;
	else
		fcur->f_next = fp;
	fcur = fp;
	return (fp);
}

char	*COPTS;
/*
 *  Dont't load if the BINARY option is specified
 */
int dontload = 0;

/*
 * Emulation flag
 */
int emulation_float = 0;

/*
 * Build the makefile from the skeleton
 */
makefile()
{
	FILE *ifp, *ofp;
	char line[BUFSIZ];
	struct opt *op;
	struct file_sys *fs;
	int found = 0;
	int found_ufs = 0;
	int min_swapfrag; /* min size of swapfrag - min vmem allocation chunk */
	int maxbufcache;	/* used for configurable buffer cache */

	read_files();
	read_filesystems();
	/*
	 * The next two for loops will traverse the option and file
	 * system linked lists, to verify at least one file system
	 * that is listed in the filesystems file is configured.
	 */
	for (op = opt; op; op = op->op_next)
		for (fs = file_sys; fs; fs = fs->fs_next)
			if (!strcmp(op->op_name, fs->fs_name)) {
				if (!strcmp(fs->fs_name, "UFS"))
					found_ufs = 1;
				found = 1;
			}

	/*
	 * If no match was found, report fatal error and return
	 */
	if (!found) {
		fprintf(stderr,"config: One or more entries listed in filesystems file must be specified\n");
		exit(1);
	}
	/*
	 * If UFS was not configured, report warning only
	 */
	if (!found_ufs)
		fprintf(stderr, "Warning: UFS not configured\n");

	strcpy(line, "makefile.");
	(void) strcat(line, machinename);
	ifp = fopen(line, "r");
	if (ifp == 0) {
		perror(line);
		exit(1);
	}
	ofp = fopen(path("makefile"), "w");
	if (ofp == 0) {
		perror(path("makefile"));
		exit(1);
	}
	fprintf(ofp, "IDENT=-I. -I.. -D%s", raise(ident));
	if (profiling)
		fprintf(ofp, " -DGPROF");
	if (machine == MACHINE_VAX)
		if (kdebug)
			fprintf(ofp, " -DKDEBUG");

	if (cputype == 0) {
		printf("cpu type must be specified\n");
		exit(1);
	}

	{ struct cputype *cp, *prevp;
	  for (cp = cputype; cp; cp = cp->cpu_next) {
		for (prevp = cputype; prevp != cp; prevp = prevp->cpu_next) {
			if (strcmp(cp->cpu_name, prevp->cpu_name) == 0) {
				break;
			}
		}
		if (prevp == cp) {
			fprintf(ofp, " -D%s", cp->cpu_name);
		}
	  }
	}

	if (source)
		fprintf(ofp," -DRELEASE='\"'%3.1f'\"' -DVERSION='\"'%d'\"'",
				release,version);

	for (op = opt; op; op = op->op_next)
		if (op->op_value)
			fprintf(ofp, " -D%s=\"%s\"", op->op_name, op->op_value);
		else {
			if (eq(op->op_name,"GFLOAT"))
				fprintf(ofp, " -Mg");

			else if (eq(op->op_name,"BINARY")) 
				/*
			 	 *  BINARY option specified then set
			 	 *  dontload flag
			 	 */
				dontload++;

			else if (eq(op->op_name,"EMULFLT"))
				/* For the floating emulation */
				emulation_float++;

			fprintf(ofp, " -D%s", op->op_name);
		}


	fprintf(ofp, "\n");
	if (hadtz == 0)
		printf("timezone not specified; gmt assumed\n");
	switch (machine) {

	case MACHINE_VAX:
	case MACHINE_MIPS:

	   if (maxusers == 0) {
		printf("maxusers not specified; 24 assumed\n");
		maxusers = 24;
	   } else if (maxusers < 8) {
		printf("maxusers less than 8 not recommended\n");
	   } else if (maxusers > 256) {
		printf("maxusers truncated to 256\n");
		maxusers = 256;
	   }
	   min_swapfrag = (CLSIZE * KLMAX * NBPG)/1024; /* min value of swapfrag in Kbytes*/

	   if(dmmin)
		fprintf(stderr,"Warning: dmmin is no longer supported\n");

	   if(swapfrag) {
		if(min_swapfrag > swapfrag) {
			printf("swapfrag (%d) is too small, set to %d\n", swapfrag, min_swapfrag);
			swapfrag = ctod((min_swapfrag * 1024)/NBPG); /* ultimately it should 
									be in disk blocks 
									for allocation */
	   	}
	   	else if (swapfrag % min_swapfrag != 0) {
			printf("swapfrag (%d) must be a multiple of CLSIZE*KLMAX (%d), swapfrag set to %d\n"
			       , swapfrag , min_swapfrag, min_swapfrag);
			swapfrag = ctod((min_swapfrag * 1024)/NBPG); /* ultimately it should
									be in disk blocks 
									for allocation */
	   	}
		else {
			swapfrag = ctod((swapfrag * 1024)/NBPG); /* convert user given 
								    Kbytes to disk blocks 
								    with portability 
								    aspect in mind */
		}
	   }

	   if(dmmax)
		fprintf(stderr,"Warning: dmmax is no longer supported\n");


	   if (maxuprc == 0 ) {
		printf("maxuprc not specified; 50 assumed\n");
		maxuprc = 50;
	   }
	   if (physmem < MINMEM_MB) {
		printf("physmem not specified or too small; %d megabytes assumed\n", MINMEM_MB);
		physmem = MINMEM_MB;
	   }

	   if(smsmat && smsmat > (GIG << 20)) {
			printf("smsmat too large, reduced to %u bytes\n", 
			    (GIG << 20));
			smsmat = (GIG << 20);
	   }

	   if(maxtsiz && maxtsiz > GIG) {
			printf("maxtsiz (%u) too large, reduced to %d megabytes\n", 
			    maxtsiz, GIG);
			maxtsiz = GIG;
	   }

	if(maxdsiz) {
		/* bytes */
		if(maxdsiz > GIG ){
			printf("maxdsiz (%d) too large, reduced to %d megabytes\n", 
			maxdsiz, GIG);
			maxdsiz = (GIG << 20);
		} else
			maxdsiz <<= 20;
	}
	if(maxssiz) {
		/* bytes */
		if(maxssiz > GIG){
			printf("maxssiz (%d) too large, reduced to %d megabytes\n", 
			maxssiz, GIG);
			maxssiz = (GIG << 20);
		} else
			maxssiz <<= 20;
	}

	   /*
	    * Configurable buffer cache
	    *
	    * Only recognized in a machine specific config file.
	    * This is a  percentage of memory to be allocated for buffer cache.
	    * Assign a default, if necessary, and run sanity tests.
	    * The computation for legal maximum depends upon the MINMEM_MB
	    * value in machine/param.h. This allows the user to use everything
	    * but what's required to boot for the buffer cache.
	    */

#define MINBUFCACHE	10

	   if (source) {	/* BINARY */
	    if (bufcache != 0)
		printf("bufcache value ignored\n");
           } else {	/* machine specific */
	    if (bufcache == 0 ) {
/*
		printf("bufcache not specified; %d%% of memory assumed\n",
		    MINBUFCACHE);
*/
		bufcache = MINBUFCACHE;
            } else if (bufcache < MINBUFCACHE) {
		printf("illegal bufcache value; %d%% of memory used\n",
		   MINBUFCACHE);
		bufcache = MINBUFCACHE;
            } else if (bufcache > 99) {
		bufcache = 99;
	    }
/*
	    maxbufcache = ((float)(physmem - MINMEM_MB) / physmem) * 100;
	    if (bufcache > MINBUFCACHE  &&  bufcache > maxbufcache) {
		printf("bufcache too large; %d%% of memory used\n",
		   maxbufcache);
		bufcache = maxbufcache;
	    }
*/
           }

	   if (processors == 0 ) {
		printf("processors not specified; 1 assumed\n");
		processors = 1;
	   }
	break;
	
	case MACHINE_SUN:
	   if (maxusers == 0) {
		printf("maxusers not specified; 8 assumed\n");
		maxusers = 8;
	   } else if (maxusers < 2) {
		printf("minimum of 2 maxusers assumed\n");
		maxusers = 2;
	   } else if (maxusers > 32) {
		printf("maxusers truncated to 32\n");
		maxusers = 32;
	   }
	break;
	}

	fprintf(ofp, "PARAM=-DTIMEZONE=%d -DDST=%d -DMAXUSERS=%d -DMAXUPRC=%d -DPHYSMEM=%d -DNCPU=%d ",
		timezone, dst, maxusers, maxuprc, physmem,
		processors);
	if (!source)
		fprintf(ofp, "-DBUFCACHE=%d ", bufcache);
	if (smmin)
		fprintf(ofp,"-DSMMIN=%d ",smmin);
	if (smmax)
		fprintf(ofp,"-DSMMAX=%d ",smmax);
	if (smbrk)
		fprintf(ofp,"-DSMBRK=%d ",smbrk);
	if (smseg)
		fprintf(ofp,"-DSMSEG=%d ",smseg);
	if (smsmat)
		fprintf(ofp,"-DSMSMAT=%d ",smsmat);
	if (maxtsiz) 
		fprintf(ofp,"-DMAXTSEG=%u ", maxtsiz);
	if (maxdsiz)
		fprintf(ofp,"-DMAXDSIZ=%u ", maxdsiz);
	if (maxssiz) 
		fprintf(ofp,"-DMAXSSIZ=%u ", maxssiz);
	if (swapfrag)
		fprintf(ofp,"-DSWAPFRAG=%d ", swapfrag);
	if (maxuva)
		fprintf(ofp,"-DMAXUVA=%d ", maxuva);
	if (maxretry)
		fprintf(ofp,"-DMAXRETRY=%d ", maxretry);
	if (vasslop){
		/*
		 * User is giving in terms of Kbytes. Convert it to 
		 * disk blocks and pass
		 * Assume disk block size as 512 for lack of anything better.
		 */
		vasslop = (vasslop * 1024)/512;
		fprintf(ofp,"-DVASSLOP=%d ", vasslop);
	}	

	fprintf(ofp,"\n");
	while (fgets(line, BUFSIZ, ifp) != 0) {
		if (*line == '%')
			goto percent;
		if (profiling && strncmp(line, "COPTS=", 6) == 0) {
			register char *cp;

			fprintf(ofp, 
			    "GPROF.EX=/usr/src/lib/libc/%s/csu/gmon.ex\n",
			    machinename);
			cp = index(line, '\n');
			if (cp)
				*cp = 0;
			cp = line + 6;
			while (*cp && (*cp == ' ' || *cp == '\t'))
				cp++;
			COPTS = malloc((unsigned)(strlen(cp) + 1));
			if (COPTS == 0) {
				printf("config: out of memory\n");
				exit(1);
			}
			strcpy(COPTS, cp);
			fprintf(ofp, "%s -pg\n", line);
			continue;
		}
		fprintf(ofp, "%s", line);
		continue;
	percent:
		if (eq(line, "%OBJS\n"))
			do_objs(ofp);
		else if (eq(line, "%EMULO\n"))
			do_emulo(ofp);
		else if (eq(line, "%EMULS\n"))
			do_emuls(ofp);
		else if (eq(line, "%CFILES\n"))
			do_cfiles(ofp);
		else if (eq(line, "%SFILES\n"))
			do_sfiles(ofp);
		else if (eq(line, "%EMRULES\n"))
			do_emrules(ofp);
		else if (eq(line, "%RULES\n"))
			do_rules(ofp);
		else if (eq(line, "%LOAD\n"))
			do_load(ofp);
		else if(eq(line,"%MACROS\n"))
			do_macros(ofp);
		else
			fprintf(stderr,
			    "Unknown %% construct in generic makefile: %s",
			    line);
	}
	(void) fclose(ifp);
	(void) fclose(ofp);
}

/*
 * get next word from stream and error out if EOF
 */

char *
next_word(fp, fname)
FILE *fp;
char *fname;
{
	register char *wd;

	if ((wd = get_word(fp)) == (char *) EOF) {
		printf("%s: Unexpected end of file\n", fname);
		exit(1);
	}
	return(wd);
}

/*
 * Read in information about existing file systems.
 */
read_filesystems()
{
	FILE *fp;
	char fsname[32], *wd, *name;
	struct file_sys *fs;

	fp = fopen("filesystems", "r");
	if (fp == NULL) {
		perror("filesystems");
		exit(1);
	}

	/* process each line */

	while ((wd = get_word(fp)) != (char *)EOF) {
		if (wd == NULL)
			continue;

		/* record each file system name */

		name = ns(wd);
		fs = (struct file_sys *)malloc(sizeof (struct file_sys));

		fs->fs_name = name;
		fs->fs_next = file_sys;
		file_sys = fs;
	}
	(void) fclose(fp);
}

/*
 * Read in the information about files used in making the system.
 * Store it in the ftab linked list.
 */
read_files()
{
	FILE *fp;
	register struct file_list *tp;
	register struct device *dp, *dp2;
	int sysfile, required, doopts, override, skipopts, negate;
	int noneed, nexneed, failing;
	char fname[32], *wd, *module;

	/*
	 * filename	[ standard | optional ] [ config-dependent ]
	 *	[ dev* [ or dev* ] | profiling-routine ] [ device-driver] 
	 *	[Binary | Notbinary] [ Unsupported ] 
	 */

	ftab = NULL;
	required = 1;
	override = 0;

	/* loop for all file lists we have to process */

	for (sysfile = 0, dp2 = dtab; sysfile < 4; sysfile++) {
		if (dp2 && sysfile == 2)
			sysfile -= 1;
		/* select appropriate file */

		switch (sysfile) {

			case 0:
				strcpy(fname, "files");
				break;

			case 1:
				for (; dp2; dp2 = dp2->d_next) {
					if (dp2->d_type == PSEUDO_DEVICE
							&& dp2->d_unit != -1)
						break;
				}
				if (!dp2)
					continue;
				(void) sprintf(fname, "files.%s", dp2->d_name);
				dp2 = dp2->d_next;
				required = 0;
				override = 1;
				break;

			case 2:
				(void) sprintf(fname, "files.%s", machinename);
				required = 1;
				override = 0;
				break;

			case 3:
				(void) sprintf(fname, "files.%s",
						raise(ident));
				required = 0;
				override = 1;
				break;
		}
		fp = fopen(fname, "r");
		if (fp == NULL) {
			if (required == 0) {
				continue;
			} else {
				perror(fname);
				exit(1);
			}
		}

		/* process each line */

		while ((wd = get_word(fp)) != (char *)EOF) {
			if (wd == NULL) {
				continue;
			}

			/* record and check module name */

			module = ns(wd);
			if (fl_lookup(module)) {
				printf("%s: Duplicate file %s.\n", fname,
					module);
				exit(1);
			}
			if (override != 0) {
				if ((tp = fltail_lookup(module)) != NULL) {
					printf("%s: Local file %s overrides %s.\n",
						fname, module, tp->f_fn);
				} else
				     tp = new_fent(); 
			} else {
				tp = new_fent();
			}
			tp->f_fn = module;
			tp->f_type = 0;
			tp->f_flags = 0;
			for (nexneed = 0; nexneed < NNEEDS; nexneed++) {
				tp->f_needs[nexneed] = NULL;
			}
			nexneed = -1;

			/* process optional or standard specification */

			wd = get_word(fp);
			if (eq(wd, "optional")) {
				doopts = 1;
			} else if (eq(wd, "standard")) {
				doopts = 0;
			} else {
				printf("%s: %s must be optional or standard\n",
					fname, module);
			}
			wd = next_word(fp, fname);

			/* process config-dependent specification */

			if (eq(wd, "config-dependent")) {
				tp->f_flags |= CONFIGDEP;
				wd = next_word(fp, fname);
			}

			/* process optional specifications */

			failing = 0;
			if (doopts == 1) {
				noneed = 0;
				skipopts = 0;
				negate = 0;
				while (wd != NULL) {
					if (eq(wd, "device-driver")
						|| eq(wd, "profiling-routine")
						|| eq(wd, "Binary")
						|| eq(wd, "Unsupported")
						|| eq(wd, "Notbinary")) {
						break;
					}

					/* if this is or, dependency is met */

					if (eq(wd, "or")) {
						if (negate) {
							printf("%s: %s has unspecified negate\n",
								fname, module);
							exit(1);
						}
						if (failing == 0) {
							skipopts = 1;
						} else {
							skipopts = 0;
							failing = 0;
						}
						wd = next_word(fp, fname);
						continue;
					}

					/* if this is not, next dependency
					   is negated */

					if (eq(wd, "not")) {
						negate = 1;
						wd = next_word(fp, fname);
						continue;
					}

					/* if dependent on cpu, do
					   not build header file */

					if (eq(wd, "cpu")) {
						noneed = 1;
						wd = next_word(fp, fname);
						continue;
					}

					/* if dependent on bus recognize
					   keyword, does nothing now */

					if (eq(wd, "bus")) {
						wd = next_word(fp, fname);
						continue;
					}

					/* process normal dependency */

					if (nexneed < 0) {
						nexneed = 0;
					}
					if (noneed == 0) {
						if (nexneed >= NNEEDS) {
							printf("%s: %s has too many dependencies",
								fname, module);
							exit(1);
						}
						tp->f_needs[nexneed++] =
							ns(wd);
					}
					noneed = 0;

					/* if dependency met, wait for field
					   terminator */

					if (skipopts != 0) {
						wd = next_word(fp, fname);
						continue;
					}

					/* see if dependency is satisfied */

					dp = dtab;
					while (dp != NULL && !eq(wd, dp->d_name)) {
						dp = dp->d_next;
					}
					if ((dp == NULL && negate == 0) ||
						(dp != NULL && negate != 0)) {

						/* flush rest of this line
						   or until we find an "or" */

						failing = 1;
						skipopts = 1;
					}
					wd = next_word(fp, fname);
					negate = 0;
				}

				/* finished with dependencies, error if none
				   or hanging not */

				if (negate) {
					printf("%s: %s has unspecified negate\n",
						fname, module);
					exit(1);
				}
				if (nexneed < 0
					&& !eq(wd, "profiling-routine")) {
					printf("%s: what is %s optional on?\n",
						fname, module);
					exit(1);
				}
			}

			/* is this module to be included? */

			if (failing != 0) {
				tp->f_flags |= INVISIBLE;
			}

			/* handle profiling or device driver spec */

			if (eq(wd, "profiling-routine")) {
				tp->f_type = PROFILING;
				if (profiling == 0) {
					tp->f_flags |= INVISIBLE;
				}
				wd = next_word(fp, fname);
			} else if (eq(wd, "device-driver")) {
				tp->f_type = DRIVER;
				wd = next_word(fp, fname);
			} else {
				tp->f_type = NORMAL;
			}

			/* handle binary or not binary spec */

			if (eq(wd, "Binary")) {
				if (!source) {
					tp->f_flags |= OBJS_ONLY;
				}
				wd = next_word(fp, fname);
			} else if (eq(wd, "Notbinary")) {
				if (source) {
					tp->f_flags |= NOTBINARY;
				}
				wd = next_word(fp, fname);
			}

			/* handle unsupported spec */

			if (eq(wd, "Unsupported")) {
				if (!source) {
					tp->f_flags |= UNSUPPORTED;
				}
				wd = next_word(fp, fname);
			}

			/* if anything left, its a syntax error */

			if (wd != NULL) {
				printf("%s: syntax error describing %s found %s type %d flags 0x%x\n",
					fname, module, wd, tp->f_type,
					tp->f_flags);
				exit(1);
			}
		}
	}
}

/*
 * This routine handles the MicroVAX emulation code object modules. These 
 * routines must be processed in a particular order. This is necessary so
 * that the space they occupy can be mapped as user read.
 */
do_emulo(fp)
	FILE *fp;
{
	static char *objects[]={ 
	" vaxarith.o  vaxcvtpl.o vaxeditpc.o vaxhandlr.o \\\n",
	"	vaxashp.o vaxcvtlp.o vaxdeciml.o vaxemulat.o vaxstring.o \\\n",
	"	vaxconvrt.o ",
	0 };

	char **cptr;
	if ( emulation_instr || emulation_float )
		fprintf(fp, "EMULO=\tvaxbegin.o ");
	else
		return;

	if ( emulation_instr ) {
		for( cptr = objects ; *cptr ; cptr++ )
			fputs(*cptr, fp);
	}

	if ( emulation_float )
		fprintf(fp, "vaxfloat.o ");

	fprintf(fp,"vaxexception.o vaxend.o\n");
}
do_objs(fp)
	FILE *fp;
{
	register struct file_list *tp, *fl;
	register int lpos, len;
	register char *cp, och, *sp;
	char swapname[32];

	fprintf(fp, "OBJS=\t");
	lpos = 8;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if ((tp->f_flags & INVISIBLE) != 0)
			continue;
		if ((tp->f_flags & NOTBINARY) && source)
		/* do not load object in the BINARY directory */
			continue;
		sp = tail(tp->f_fn);
		for (fl = conf_list; fl; fl = fl->f_next) {
			if (fl->f_type != SWAPSPEC)
				continue;
			if (eq(fl->f_fn, "generic") || eq(fl->f_fn, "boot"))
				sprintf(swapname, "swap.c", fl->f_fn);
			else
				sprintf(swapname, "swap%s.c", fl->f_fn);
			if (eq(sp, swapname))
				goto cont;
		}
		cp = sp + (len = strlen(sp)) - 1;
		och = *cp;
		*cp = 'o';
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "%s ", sp);
		if (tp->f_flags & UNSUPPORTED)
			printf("Warning this device is not supported by DIGITAL %s\n",sp);
		lpos += len + 1;
		*cp = och;
cont:
		;
	}
	if (lpos != 8)
		putc('\n', fp);
}

/*
 *	This routine outputs the emulation source file names
 */
do_emuls(fp)
	FILE *fp;
{
	static char *sources[]={
	"../machine/emul/vaxarith.s ../machine/emul/vaxashp.s \\\n",
	"	../machine/emul/vaxconvrt.s ../machine/emul/vaxcvtlp.s\\\n",
	"	../machine/emul/vaxcvtpl.s ../machine/emul/vaxdeciml.s\\\n",
	"	../machine/emul/vaxeditpc.s ../machine/emul/vaxemulat.s \\\n",
	"	../machine/emul/vaxhandlr.s ../machine/emul/vaxstring.s \\\n",
	0};

	char **cptr;

	if( !source ) 
		return;

	if (emulation_instr || emulation_float)
		fprintf(fp,"EMULS=\t../machine/emul/vaxbegin.s ");
	else
		return;

	if (emulation_instr) {
		for( cptr=sources ; *cptr ; cptr++ )
			fputs(*cptr, fp);
	}

	fprintf(fp,"\t");
	if (emulation_float)
		fprintf(fp,"../machine/emul/vaxfloat.s ");

	fprintf(fp,"../machine/emul/vaxexception.s ../machine/emul/vaxend.s\n");
}

#define MAXFILES	150	/* maximum number of files in CFILES */
do_cfiles(fp)
	FILE *fp;
{
	register struct file_list *tp;
	register int lpos, len, count;
	int	cfiles;
	char buf[1024];
	char cstr[20];

	bzero(buf,1024);
	strcpy(buf,"\nALLCFILES=\t${CFILES}");
	fprintf(fp, "CFILES=\t");
	lpos = 8;
	count = 0;
	cfiles = 1;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if ((tp->f_flags & INVISIBLE) != 0)
			continue;
		if (tp->f_flags & OBJS_ONLY)
			continue;
		if ((tp->f_flags & NOTBINARY) && source)
		/* do not compile in the BINARY directory */
			continue;
		if (tp->f_fn[strlen(tp->f_fn)-1] != 'c')
			continue;
		if ((len = 3 + strlen(tp->f_fn)) + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "../../%s ", tp->f_fn);
		lpos += len + 1;
		if (++count == MAXFILES) {
			fprintf(fp, "\n\n");
			sprintf(cstr,"CFILES%d",cfiles);
			fprintf(fp,"%s=", cstr);
			lpos = 8;
			count = 0;
			strcat(buf," ${");
			strcat(buf,cstr);
			strcat(buf,"}");
			cfiles++;
		}
	}
	if (source)
		fprintf(fp, "\\\n\t../machine/genassym.c");
	if (lpos != 8)
		putc('\n', fp);
	strcat(buf,"\n");
	fprintf(fp,buf);
}

do_sfiles(fp)
	FILE *fp;
{
	register struct file_list *tp;
	register int lpos, len;

	fprintf(fp, "SFILES=");
	lpos = 8;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if ((tp->f_flags & INVISIBLE) != 0)
			continue;
		if (tp->f_flags & OBJS_ONLY)
			continue;
		if((tp->f_flags & NOTBINARY) && source)
			continue;
		if (tp->f_fn[strlen(tp->f_fn)-1] != 's')
			continue;
		if ((len = 3 + strlen(tp->f_fn)) + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "../../%s ", tp->f_fn);
		lpos += len + 1;
	}
	if (lpos != 8)
		putc('\n', fp);
}

char *
tail(fn)
	char *fn;
{
	register char *cp;

	cp = rindex(fn, '/');
	if (cp == 0)
		return (fn);
	return (cp+1);
}

/*
 * Output the emulation code rules. This is really a hack but considering
 * the fact that the code and rules are not optional for microvax and that
 * the rules are special and do not fit with the general case we do it this
 * way.
 */
do_emrules( fp )
	FILE *fp;
{
	char **cptr;
	/*
	 * This is the list of files that make up the emulation code.
	 * Some of the code is dependant on the placement and order
	 * of the first three names.
	 */
	static char *emuls[]={
		"vaxbegin", "vaxend",  "vaxfloat", "vaxarith", "vaxcvtpl",
		"vaxeditpc","vaxhandlr", "vaxashp", "vaxcvtlp", "vaxdeciml",
		"vaxemulat", "vaxstring", "vaxconvrt", "vaxexception",0
		};

	cptr = emuls;
	if( !source ){
		for( ; *cptr ; cptr++ ){
			fprintf(fp, "%s.o: ../BINARY/%s.o\n",
				*cptr, *cptr);
			fprintf(fp, "\t@${LN} -s ../BINARY/%s.o %s.o\n\n",
				*cptr, *cptr);
		}
	} else {
		/*
		 * do begin and end
		 */
		fprintf(fp, "%s.o: ../machine/emul/%s.s\n\t${CC} -c ../machine/emul/%s.s\n\n",
			*cptr, *cptr, *cptr);
		cptr++;
		fprintf(fp, "%s.o: ../machine/emul/%s.s\n\t${CC} -c ../machine/emul/%s.s\n\n",
			*cptr, *cptr, *cptr);
		cptr++;
		/*
		 * do float
		 */
		fprintf(fp, "%s.o: ../machine/emul/%s.s ../machine/emul/%s.awk\n",
			*cptr, *cptr, *cptr);
		fprintf(fp, "\t${AWK} -f ../machine/emul/%s.awk ../machine/emul/%s.s | \\\n\t\t${CPP} -I. | ${AS} -o %s.o -\n",
			*cptr, *cptr, *cptr);
		cptr++;
		/*
		 * Do the rest
		 */
		for( ; *cptr ; cptr++ ) {
			fprintf(fp, "%s.o: ../machine/emul/%s.s\n", *cptr, *cptr);
			fprintf(fp, "\t${CPP} -I. ");
			fprintf(fp, "../machine/emul/%s.s | ${AS} -o %s.o -\n\n",
				*cptr, *cptr);
		}
	}
}

/*
 * Create the makerules for each file
 * which is part of the system.
 * Devices are processed with the special c2 option -i
 * which avoids any problem areas with i/o addressing
 * (e.g. for the VAX); assembler files are processed by as.
 */
do_rules(f)
	FILE *f;
{
	register char *cp, *np, och, *tp;
	register struct file_list *ftp;
	char *extras;

	if(machine == MACHINE_VAX) {	 
	if(source){
		fprintf(f,"locore_bin.o: assym.s ${AHEADS} ../machine/rpb.s");
		fprintf(f,"\t../machine/scb.s ../machine/lock.s ../machine/locore.s \\\n");
		fprintf(f,"\t../machine/mtpr.h ../machine/trap.h ../machine/psl.h \\\n");
		fprintf(f,"\t../machine/pte.h ../machine/cpu.h mba.h\n");
		fprintf(f,"\t${CAT} assym.s ../machine/locore.s > locore.c\n");
		fprintf(f,"\t${CC} -E -I. -DLOCORE ${COPTS} locore.c > locore.i\n");
		fprintf(f,"\t${AS} -o locore_bin.o ${AHEADS} locore.i\n");
		fprintf(f,"\t${RM} locore.i locore.c\n\n");
		fprintf(f,"scb.o: assym.s  ../machine/rpb.s ../machine/scb.s ../machine/lock.s\n");
		fprintf(f,"\t${CAT} assym.s ../machine/rpb.s ../machine/scb.s ../machine/lock.s > scb.c\n");
		fprintf(f,"\t${CC} -E -I. -DLOCORE ${COPTS} scb.c > scb.i\n");
		fprintf(f,"\t${AS} -o scb.o ${AHEADS} scb.i\n");
		fprintf(f,"\t${RM} scb.i scb.c\n\n");
		fprintf(f,"spt.o: assym.s  ../machine/spt.s\n");
		fprintf(f,"\t${CAT} assym.s ../machine/spt.s > spt.c\n");
		fprintf(f,"\t${CC} -E -I. -DLOCORE ${COPTS} ${PARAM} spt.c | \\\n\t\t$(MAKESPT) > spt.i\n");
		fprintf(f,"\t${AS} -o spt.o ${AHEADS} spt.i\n");
		fprintf(f,"\t${RM} spt.i spt.c\n\n");
		fprintf(f,"genassym: genassym.o\n");
		fprintf(f,"\t${CC} -o genassym ${CFLAGS} ${PARAM} genassym.o\n\n");
		fprintf(f,"genassym.o: ../machine/genassym.c\n");
		fprintf(f,"\t${CC} -c ${CFLAGS} ${PARAM} ../machine/genassym.c\n\n");
		fprintf(f,"assym.s: ../../h/param.h ../machine/pte.h ../../h/buf.h ../../h/vmparam.h \\\n");
		fprintf(f,"\t../../h/vmmeter.h ../../h/dir.h ../../h/cmap.h ../../h/map.h ../../io/uba/ubavar.h \\\n");
		fprintf(f,"\t../../h/proc.h makefile genassym\n");
		fprintf(f,"\t./genassym %d %d %d >assym.s\n\n",
			maxusers, physmem, bufcache);  /*RR - needed -DKERNEL*/
					     /*RR - IDENT changed to CFLAGS*/
		fprintf(f,"${INLINE}: ../machine/inline/inline.h \\\n");
		fprintf(f,"\t../machine/inline/langpats.c ../machine/inline/libcpats.c \\\n");
		fprintf(f,"\t../machine/inline/machdep.c ../machine/inline/machpats.c \\\n");
		fprintf(f,"\t../machine/inline/main.c\n");
		fprintf(f,"\tcd ../machine/inline; ${MAKE}\n\n");
	} else {
		fprintf(f,"scb.o: assym.s  ../machine/rpb.s ../machine/scb.s ../machine/lock.s\n");
		fprintf(f,"\t${CAT} assym.s ../machine/rpb.s ../machine/scb.s ../machine/lock.s > scb.c\n");
		fprintf(f,"\t${CC} -E -I. -DLOCORE ${COPTS}  scb.c > scb.i\n");
		fprintf(f,"\t${AS} -o scb.o ${AHEADS} scb.i\n");
		fprintf(f,"\t${RM} scb.i scb.c\n\n");
		fprintf(f,"spt.o: assym.s  ../machine/spt.s\n");
		fprintf(f,"\t${CAT} assym.s ../machine/spt.s > spt.c\n");
		fprintf(f,"\t${CC} -E -I. -DLOCORE ${COPTS} ${PARAM} spt.c | $(MAKESPT) > spt.i\n");
		fprintf(f,"\t${AS} -o spt.o ${AHEADS} spt.i\n");
		fprintf(f,"\t${RM} spt.i spt.c\n\n");
		fprintf(f,"locore_bin.o:\n");
		fprintf(f,"\t@${LN} -s ../BINARY/locore_bin.o locore_bin.o\n\n");
		fprintf(f,"assym.s: makefile\n\t../BINARY/genassym %d %d %d > assym.s\n\n", maxusers, physmem, bufcache);
	}
	} /* End of MACHINE_VAX */
		

for (ftp = ftab; ftp != 0; ftp = ftp->f_next) {
	if ((ftp->f_flags & INVISIBLE) != 0)
		continue;
	if ((ftp->f_flags & NOTBINARY) && source)
		continue;
	cp = (np = ftp->f_fn) + strlen(ftp->f_fn) - 1;
	och = *cp;
	*cp = '\0';
	if (ftp->f_flags & OBJS_ONLY) {
	  fprintf(f,"%so: ../BINARY/%so\n\t@${LN} -s ../BINARY/%so %so\n\n"
			,tail(np),tail(np),tail(np),tail(np));
		continue;
	}
	fprintf(f, "%so: ../../%s%c\n", tail(np), np, och);
	tp = tail(np);
	if (och == 's') {
		switch (machine) {

		case MACHINE_MIPS:
		    fprintf(f, "\t${CC} ${CCASFLAGS} -o %so ../../%ss\n\n", tp, np);
		    break;

		case MACHINE_VAX:
		default:
		    fprintf(f, "\t${AS} -o %so ../../%ss\n\n", tp, np);
		    break;
		}
		continue;
	}
	if (ftp->f_flags & CONFIGDEP)
		extras = "${PARAM} ";
	else
		extras = "";
	switch (ftp->f_type) {

	case NORMAL:
		switch (machine) {

		case MACHINE_VAX:
			fprintf(f, "\t${CC} -I. -c -S ${COPTS} ${PARAM} %s../../%sc\n",
				extras, np);
			fprintf(f, "\t${C2} %ss | ${INLINE} |", tp);
			fprintf(f, " ${AS} -o %so\n", tp);
			fprintf(f, "\t${RM} -f %ss\n\n", tp);
			break;

		case MACHINE_MIPS:
			fprintf(f, "\t${CC} ${CCNFLAGS} %s../../%sc\n\n",
			     extras, np);
			break;

		case MACHINE_SUN:
			fprintf(f, "\t${CC} -I. -c -O ${COPTS} %s../%sc\n\n",
				extras, np);
			break;
		}
		break;

	case DRIVER:
		switch (machine) {

		case MACHINE_VAX:
			fprintf(f, "\t${CC} -I. -c -S ${COPTS} ${PARAM} %s../../%sc\n",
				extras, np);
			fprintf(f,"\t${C2} -i %ss | ${INLINE} |", tp);
			fprintf(f, " ${AS} -o %so\n", tp);
			fprintf(f, "\t${RM} -f %ss\n\n", tp);
			break;

		case MACHINE_MIPS:
			fprintf(f, "\t${CC} ${CCDFLAGS} %s../../%sc\n\n",
			     extras, np);
			break;

		case MACHINE_SUN:
			fprintf(f, "\t${CC} -I. -c -O ${COPTS} %s../%sc\n\n",
				extras, np);
		}
		break;

	case PROFILING:
		if (!profiling)
			continue;
		if (COPTS == 0) {
			fprintf(stderr,
			    "config: COPTS undefined in generic makefile");
			COPTS = "";
		}
		switch (machine) {

		case MACHINE_VAX:
			fprintf(f, "\t${CC} -I. -c -S ${PARAM} %s %s../../%sc\n",
				COPTS, extras, np);
			fprintf(f, "\t${EX} - %ss < ${GPROF.EX}\n", tp);
			fprintf(f, "\t${INLINE} %ss | ${AS} -o %so\n",
				tp, tp);
			fprintf(f, "\t${RM} -f %ss\n\n", tp);
			break;
		
		case MACHINE_MIPS:
			if(!profiling)
				continue;
			fprintf(f, "\t${RM} -f %so\n", tp);
			fprintf(f, "\t${CC} ${CCPFLAGS} %s../../%sc\n\n",
			    extras, np);
		        break;


		case MACHINE_SUN:
			fprintf(stderr,
			    "config: don't know how to profile kernel on sun\n");
			break;
		}
		break;

	default:
		printf("Don't know rules for %s\n", np);
		break;
	}
	*cp = och;
}
}

/*
 * Create the load strings
 */
do_load(f)
	register FILE *f;
{
	register struct file_list *fl;
	int first = 1;
	struct file_list *do_systemspec();

	fl = conf_list;
	while (fl) {
		if (fl->f_type != SYSTEMSPEC) {
			fl = fl->f_next;
			continue;
		}
		fl = do_systemspec(f, fl, first);
		if (first)
			first = 0;
	}
	fprintf(f, "all:");
	for (fl = conf_list; fl != 0; fl = fl->f_next)
		if (fl->f_type == SYSTEMSPEC)
			fprintf(f, " %s", fl->f_needs[0]);
	fprintf(f, "\n");
}

do_macros(f)
	register FILE *f;
{
	register struct opt *op;

	for (op = mkopt; op; op = op->op_next)
		fprintf(f, "%s=%s\n", op->op_name, op->op_value);
}

struct file_list *
do_systemspec(f, fl, first)
	FILE *f;
	register struct file_list *fl;
	int first;
{
	int swaptype;
	char *depends;

	if(machine == MACHINE_MIPS)
		if (fl == NULL) return(fl);
	if ( emulation_instr || emulation_float )
		depends = "${OBJS} ${EMULO}";
	else
		depends = "${OBJS}";

	fprintf(f, "%s: makefile", fl->f_needs[0]);
	if (machine == MACHINE_VAX) {
		if (source) {
		    fprintf(f, " ");
		    fprintf(f, "${INLINE}");
		}
		fprintf(f, " spt.o scb.o locore_bin.o ubglue.o \\\n");
		fprintf(f, "\t%s param.o", depends);
		if (source)
			fprintf(f, " ioconf.o swap.o");
		else
			fprintf(f, " ioconf.o swapgen%s.o", fl->f_needs[0]);
	}
        if(machine == MACHINE_MIPS)
		fprintf(f, " ${LOBJS} ${OBJS} ");

	switch (machine) {

	case MACHINE_MIPS:
	   if (!source) {
		if (!eq(fl->f_fn, "generic") && !eq(fl->f_fn, "boot"))
			fprintf(f, " swap%s.o", fl->f_fn);  /*RR*/
		else
                	fprintf(f, " swapgeneric.o");  /*RSP*/	
	   }
	   break;

	case MACHINE_VAX:
	default:
	   if (!eq(fl->f_fn, "generic") && !eq(fl->f_fn, "boot"))
		fprintf(f, " swap%s.o", fl->f_fn);  /*RR*/
	   break;
	}

	if (dontload) {
		/*
		 *	Don't load if the BINARY option has been specified
		 */
		switch (machine) {
		case MACHINE_VAX:
		  	if(kdebug) 
			  	fprintf(f,"\n\t(cd ../machine/kdb; make)\n");
			break;
		default:
			break;		
		
		}
		fprintf(f, "\n");   /*RR*/
		fprintf(f, "\t@${ECHO} Binary build done\n\n");
	}
	else {	/* we are loading */
		/* first define a load tag on the end of vmunix */
		/* and define that symbol as the actual load commands */
		/* now we can say "make vmunix" to get the old style loads */
		/* with all the dependancy checking */
		/* or we can say "make load" and we execute the load commands */
		if (strcmp(fl->f_needs[0],"vmunix")==0) {
			/*for vmunix we allow "make load" or "make loadvmunix"*/
			fprintf(f, " load%s\n\nload load%s:\n",
				fl->f_needs[0], fl->f_needs[0]);   /*RR*/
		}
		else {
			fprintf(f, " load%s\n\nload%s:\n",
				fl->f_needs[0], fl->f_needs[0]);   /*RR*/
		}
		fprintf(f, "\t${RM} -f %s\n", fl->f_needs[0]);
		fprintf(f, "\t${SH} ${CONF}/newvers.sh\n");/*RR*/
		switch (machine) {
		case MACHINE_VAX:
			fprintf(f, "\t${CC} $(CFLAGS) -c vers.c\n");
			break;
		case MACHINE_MIPS:
			fprintf(f, "\t${CC} $(CCNFLAGS) -c vers.c\n");
			break;
		default:
			break;
		}
		fprintf(f, "\t@${ECHO} loading %s\n", fl->f_needs[0]);

		switch (machine) {
		case MACHINE_VAX:
			/* -B flag for page alignment of spt in BSS. */
			fprintf(f,"\t@${LD} -n -o %s -e start -x -T 80000000 -B ",
				fl->f_needs[0]);
			/* spt.o MUST be first module for load!!! */
			fprintf(f, "spt.o scb.o \\\n\t\t");
			fprintf(f, "locore_bin.o ubglue.o ");
			fprintf(f, "%s \\\n\t\t",depends);
			fprintf(f, "ioconf.o param.o swap.o ");
			if (kdebug) fprintf(f,"../machine/kdb/kdb.o -lc ");	
			break;

		case MACHINE_MIPS:
			fprintf(f, "\t@${LD} ${LDFLAGS} -o %s ${LOBJS} ${OBJS} ",
			fl->f_needs[0]);
			break;

		case MACHINE_SUN:
			fprintf(f, "\t@${LD} -o %s -e start -x -T 4000 ",
				fl->f_needs[0]);
			break;
		}
		fprintf(f, "vers.o ", fl->f_fn);

		switch (machine) {
		case MACHINE_MIPS:

		   if (!source) {
			if (!eq(fl->f_fn, "generic") && !eq(fl->f_fn, "boot"))
				fprintf(f, "swap%s.o\n", fl->f_fn);
			else
                		fprintf(f, " swapgeneric.o\n");  /*RSP*/	
		   }
		   break;

		case MACHINE_VAX:

		   if (!eq(fl->f_fn, "generic") && !eq(fl->f_fn, "boot"))
			fprintf(f, "swap%s.o\n", fl->f_fn);
		   else
			fprintf(f, "\n");
		   break;
		}

		fprintf(f, "\t@echo rearranging symbols\n");
		fprintf(f, "\t@-${SYMORDER} ../machine/symbols.sort %s\n",
			fl->f_needs[0]);
		fprintf(f, "\t@${SIZE} %s\n", fl->f_needs[0]);
		if (machine == MACHINE_VAX) {
			if (kdebug){
				fprintf(f, "\t(cd ../machine/kdb; make kdbload)\n");
				fprintf(f, "\t../machine/kdb/kdbload %s \n",fl->f_needs[0]);
				fprintf(f, "\t@${SIZE} %s\n", fl->f_needs[0]);
			}
		}
		fprintf(f, "\t@${CHMOD} 755 %s\n\n", fl->f_needs[0]);
	}

	swaptype = 0;
	if (eq(fl->f_fn, "generic"))
		swaptype = 1;
	else if (eq(fl->f_fn, "boot"))
			swaptype = 2;
	if (!source) {
	   switch (machine) {

	   case MACHINE_VAX:
		fprintf(f, "swapgen%s.o:\n", fl->f_needs[0]);
		fprintf(f, "\t${CC} -I. -c -S ${COPTS} -DSWAPTYPE=%d ../machine/swap.c\n", swaptype);
		fprintf(f, "\t${C2} swap.s | ${INLINE}");
		fprintf(f, " | ${AS} -o swap.o\n");
		fprintf(f, "\t${RM} -f swap.s\n\n");
		break;

	   case MACHINE_MIPS:
		if (swaptype) {
		   fprintf(f, "swapgeneric.o:\n");
		   fprintf(f, "\t${RM} -f swapgeneric.o\n");
		   fprintf(f, "\t${CC} ${CCSFLAGS} -DSWAPTYPE=%d ../machine/swapgeneric.c\n", swaptype);
		   fprintf(f, "\n");
		}
		break;
	   }
	}
	do_swapspec(f, fl->f_fn);
	for (fl = fl->f_next; fl && fl->f_type == SWAPSPEC; fl = fl->f_next)
		;
	return (fl);
}

do_swapspec(f, name)
	FILE *f;
	register char *name;
{
	if ((eq(name, "generic") || eq(name, "boot")) && source) {
		switch (machine) {

		case MACHINE_VAX:
			fprintf(f, "swap.o: ../machine/swap.c\n");
			fprintf(f, "\t${CC} -I. -c -S ${COPTS} ");
			fprintf(f, "../machine/swap.c\n");
			fprintf(f, "\t${C2} swap.s | ${INLINE}");
			fprintf(f, " | ${AS} -o swap.o\n");
			fprintf(f, "\t${RM} -f swap.s\n\n");
			break;

		case MACHINE_MIPS:
			break;

		case MACHINE_SUN:
			fprintf(f, "\t${CC} -I. -c -O ${COPTS} ");
			fprintf(f, "../%s/swap.c\n\n", machinename);
			break;
		}
	} else
	  if (!eq(name, "generic") && !eq(name, "boot")) {
		switch (machine) {
		case MACHINE_VAX:
			fprintf(f, "swap%s.o: swap%s.c\n", name, name);
			fprintf(f, "\t${CC} -I. -c -O ${COPTS} swap%s.c\n\n", name);
			break;
		case MACHINE_MIPS:
			fprintf(f, "swap%s.o: swap%s.c\n", name, name);
			fprintf(f, "\t${RM} -f swap%s.o\n", name);
			fprintf(f, "\t${CC} ${CCSFLAGS} swap%s.c\n\n", name);
			break;
		}	
		}
}

char *
raise(str)
	register char *str;
{
	register char *cp = str;

	while (*str) {
		if (islower(*str))
			*str = toupper(*str);
		str++;
	}
	return (cp);
}



isconfigured(device)
	register char *device;
{
	register struct device *dp;

	for (dp = dtab; dp != 0; dp = dp->d_next)
		if (dp->d_unit != -1 && eq(dp->d_name, device)) 
			return(1);
	return(0);
}

#ifdef mips
eq(a,b)
register char *a,*b;
{
	if (a && b) {	/* survive those null pointers...rr */
		return(!strcmp(a,b));
	}
	return(0);
}
#endif mips
