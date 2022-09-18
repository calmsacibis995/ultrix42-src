/*
 * ultrixboot.c
 */

#ifndef lint
static char *sccsid = "@(#)ultrixboot.c	4.6	ULTRIX	3/6/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
 * 05-Mar-91 -- Joe Szczypek
 *	Suppress generation of contiguous memory warning message with VAXs 
 *	having 512Mb of memory.
 *
 * 06-Nov-90 -- Joe Szczypek
 *	Added support for VAX6000-5x0 Diagnostic Supervisor (emsaa).
 *
 * 10-Oct-90 -- Joe Szczypek
 *	Fixed undefines on VAX side.
 *
 * 23-Aug-90 -- Joe Szczypek
 *      Added new callback interface specified by TURBOchannel ECO.
 *
 * 28-Sep-89 -- alan frechette
 *	Allow booting of VDS on ISIS system. Allow booting of
 *	kernel images from any partition for all MIPS systems.
 *
 * 23-Aug-89 -- tresvik
 *	Support was added for loading VDS on ISIS system.
 *
 * 24-Feb-89 -- cranston
 *      Added support for booting the Vax Diag Supervisor on the 65x0.
 *
 * 30-Dec-88 -- cranston
 *      Added support for booting the Vax Diag Supervisor on the 62x0.
 *
 * 19-Nov-88 -- darrell
 *	The warning for contiguous memory is now printed only if the
 *	difference in real memory size, and the memory size reported
 *	by VMB is greater than 256 pages.
 *
 * 01-Nov-88 -- darrell
 *	The warning for contiguous memory is now printed only if the
 *	difference in real memory size, and the memory size reported
 *	by VMB is greater than 128 pages.
 *
 * 01-Sep-88 -- darrell
 *	The warning for contiguous memory is now printed only if the
 *	difference in real memory size, and the memory size reported
 *	by VMB is greater than 64 pages.
 *
 * 13-Jul-88 -- chet
 *	Removed code that cleared bss; clearing of bss is done by
 *	the start() routine in locore.s. Added check that compares
 *	available memory against image size.
 *
 */

#include "../h/param.h"
#include "../h/reboot.h"
#include "../machine/common/cpuconf.h"

#ifdef mips
#include "../machine/mips/entrypt.h"    /* Contains TURBOchannel defines */
#endif mips

#ifdef vax
#include "vax/vmb.h"
#include "../machine/vax/cpu.h"
#include "../machine/vax/mtpr.h"
#include "../machine/vax/rpb.h"
char    ds730[] = "(x)field/ensaa.exe";
char    ds750[] = "(x)field/ecsaa.exe";
char    ds780[] = "(x)field/essaa.exe";
char    ds6200[] = "(x)field/elsaa.exe";
char	ds5800[] = "()field/elsaa.exe";
char    ds6500[] = "(x)field/ersaa.exe";
char    ds65x0[] = "(x)field/emsaa.exe";
char    ds8200[] = "(x)field/ebsaa.exe";
char    ds8600[] = "(x)field/edsaa.exe";
char    ds8800[] = "(x)field/ezsaa.exe";
char    ds8820[] = "(x)field/ejsaa.exe";
char    vmunix_name[] = "(a)vmunix";
#endif vax

#ifdef mips
char    vmunix_name[] = "vmunix";
char	ds5800[] = "()field/elsaa.exe";
extern int rex_base;		/* Will hold REX callback vector base addr */
extern int rex_magicid;
int prom_io;
unsigned systype;		/* Store the mips "systype" from the PROM */
unsigned cpu;			/* Store the mips cpu from the sid */
int partition = 0;		/* Partition to open imagename from */

#define printf _prom_printf
#define gets _prom_gets
#endif mips

#define INBUFSZ 256
char    ask_name[INBUFSZ];

#define RB_LOADDS RB_INITNAME			/* defined as 0x10 */

#ifdef vax
#define MAXMEMSIZE	1024*1024*512
#define BISPACE		(1024*1024)/2
char	ucodedone=0;				/* mark one time ucode
						   load for 11/750 (successful
						   or unsuccessful) */
extern	int      mode, cpu;
extern	struct	vmb_info *vmbinfo;
#endif vax

char   *imagename;

/*
 * Functional Discription:
 *	main of `ultrixboot' program ... 
 *
 * Inputs:
 *	none although R10 and R11 are preserved on VAX
 *	arg list is preserved on MIPS
 *
 * Outputs:
 *	none
 *
 */
#ifdef vax
main ()
#endif vax

#ifdef mips
int ub_argc;
char **ub_argv;
char **ub_envp;
char **ub_vector;
int gub_argc;
char **gub_argv;

char *lub_argv[INBUFSZ];

main (argc,argv,envp,vector)
int argc;
char **argv, **envp, **vector;
#endif mips
{
	register howto, devtype;		/* howto=r11, devtype=r10 */
#ifdef vax
	union cpusid sid; 
#endif vax
#ifdef mips
	register char *arg, *cp;
	char    tmp_name[INBUFSZ];
	int	i,j;
#endif mips
	int	io;
	char	fs;
	char	*boot;
	extern char *version;

#ifdef mips
	rex_magicid = (int)envp;
	rex_base = (int)vector;
#endif mips

	printf("\nUltrixboot - %s\n\n", version);
	/*
	 * If the amount of memory found by VMB exceeds the amount of
	 * contiguous memory sized for Ultrix by 256 pages, warn the
	 * user, otherwise let it slide.  Rememeber, MVAX II reserves two
	 * pages at the top for the console. Firefox uses more high memory
	 * than previous systems - in that it has a CTSIA and CCA there.
	 * Also, leave some slop for the way the bit map is counted
	 * (by byte rather than by bit).
	 * This warning is intended to find large holes in memory and
	 * and answer the question 'Why is Ultrix only seeing x of the y
	 * Meg of memory?'
	 */
#ifdef vax
	if ((((((struct rpb *)(vmbinfo->rpbbas))->pfncnt) - vmbinfo->memsiz/512) > 256) && (vmbinfo->memsiz != MAXMEMSIZE-BISPACE)) 
		printf("\nWARNING:\n\tA total of %d bytes of memory were found,\n\tof which %d are contiguous.  Ultrix will\n\tonly use the contiguous memory.\n\n",
		(((struct rpb *)(vmbinfo->rpbbas))->pfncnt)*512, 
		vmbinfo->memsiz);
	fs = ((howto & 0x70000000) >> 28) + 'a';
#endif vax
#ifdef mips
	ub_argc = argc;		/* save prom args for kernel */
	ub_argv = argv;		/* save prom args for kernel */
	gub_argc = argc;	/* global save of arg */
	gub_argv = argv;	/* global save of arg */
	ub_envp = envp;
	ub_vector = vector;     /* save prom args for kernel */
	howto = 0;

	if((int)envp == REX_MAGIC) {
	  rex_base = (int)vector;
	  systype = rex_getsystype();
	}
	else {
	  rex_base = 0;
	  systype = xtob(prom_getenv("systype"));
	}
	cpu = GETSYSTYPE(systype);

	if(rex_base) {
		for(i=1;i<ub_argc;i++) {
			if(ub_argv[i][0] != '-' && ub_argv[i][0] != NULL)
				if(ub_argv[i][1] == '/')
	  				imagename = argv[i];
		}
	  	if(rex_bootinit() < 0) {
	    		printf("bootinit failed\n");
	    		stop();
	  	}
	}
	else {
	  imagename = (char *)prom_getenv("boot");
	  if ((prom_io = _prom_open(imagename, 0)) < 0 ) {
	    printf("can't open channel to boot driver\n");
	    stop();
	  }
        }

	/*
	 * Evaluate the args passed to see if this '-d' exists to
	 * indicate a diagnostic boot for those machines for which
	 * support exists.
	 */
	while (argc > 1 && argv[1][0] == '-') {
		arg = *++argv;
		argc--;
		while (*++arg != '\0')
			switch (*arg) {
			case 'd':
				howto |= RB_LOADDS;
				break;
			default:
				break;
			}
	}
#endif mips
	io = -1;				/* set to start loop */
	while (io < 0) {
	/* 
	 * If we are loading the supervisor, imagename is
	 * already set.  If we are not, then check for RB_ASKNAME.
	 * If RB_ASKNAME is not set, then assume the default 
	 * 'vmunix' name, otherwise ask for a unix file name to
	 * load.  Entering a diagnostic supervisor name will not
	 * work as it is not a unix image.
	 */
		if (howto & RB_ASKNAME) {
			printf ("Enter %s name: ",
				howto & RB_LOADDS ? "supervisor" : "image");
#ifdef mips
			if(rex_base) {
				ub_argv = lub_argv;
				ub_argv[1] = tmp_name;
				rex_gets(ub_argv[1]);

				i=0;
				ask_name[i++] = 'b';
				ask_name[i++] = 'o';
				ask_name[i++] = 'o';
				ask_name[i++] = 't';
				ask_name[i++] = ' ';
				j=0;
				while(ub_argv[1][j] != NULL)
				    ask_name[i++] = ub_argv[1][j++];
				ask_name[i++] = ' ';
				ask_name[i++] = '-';
				ask_name[i++] = 'N';
				ask_name[i++] = 'o';
				ask_name[i++] = 'b';
				ask_name[i++] = 'o';
				ask_name[i++] = 'o';
				ask_name[i++] = 't';
				ask_name[i++] = '\0';

				*(int *)(rex_base + 0x54) = 0;
				rex_execute_cmd(ask_name);
				if(*(int *)(rex_base + 0x54) == 0) {
				        rex_printf("Channel failed\n");
					rex_rex('h');
				}
				if(rex_bootinit() < 0) {
				        rex_printf("Channel failed\n");
					rex_rex('h');
				}

				i=1;
				ub_argc=1;
				cp = ub_argv[i++];
				while(*cp++ != NULL) {
					if(*(cp-1) == ' ') {
						*(cp-1) = '\0';
						ub_argc++;
						ub_argv[i] = cp;
						i++;
					} 
					        
				}
				ub_argc++;
/*
				for(i=1;i<ub_argc;i++)
				        printf("argv[%x]=%s\n",i,ub_argv[i]);
*/
				cp = ub_argv[1];
				i=0;
				while(*cp != NULL) 
					ask_name[i++] = *cp++;
				ask_name[i]= *cp;	
				if(ask_name[0] == 0)
					continue;
				imagename = ask_name;
/*
				printf("new string = %s\n",imagename);
*/
			}	
			else {
#endif mips
				gets (ask_name);
				if (ask_name[0] == 0)
					continue;
				imagename = ask_name;
#ifdef mips
			}
#endif mips
		}
		else
			if (howto & RB_LOADDS) {/* Are we loading the DS? */
				switch (cpu) {	/* Which cpu is this? */
#ifdef vax
				case VAX_730:	/* point to 'ensaa' */
					imagename = ds730;
					break;
				case VAX_750: 	/* point to 'ecsaa' */
					imagename = ds750;
					break;
				case VAX_780: 	/* point to 'essaa' */
					imagename = ds780;
					break;
				case VAX_8200:	/* point to 'ebsaa' */
					imagename = ds8200;
					break;
				case VAX_8600:	/* point to 'edsaa' */
					imagename = ds8600;
					break;
				case VAX_8800:	/* point to 'ezsaa' */
					imagename = ds8800;
					break;
				case CVAX_CPU:	/* point to 'elsaa' */
					imagename = ds6200;
					break;
				case 11:	/* point to 'ersaa' */
					imagename = ds6500;
					break;
				case 17:	/* polarstar */
					imagename = ds8820;/*point to ejsaa*/
					break;
				case 18:	/* point to 'emsaa' */
					imagename = ds65x0;
					break;
#endif vax
#ifdef mips
				case ST_DS5800:	/* ISIS */
					imagename = ds5800;
					break;
#endif mips
				default: 
				/* 
				 * Only the above CPU's are supported.
				 * Remember that 11/780 = 11/785
				 *           	 11/730 = 11/725
				 *		 8600	= 8650
				 *		 8800	= 85xx and 87xx
				 */
					printf ("No DS load support for cpu type %d\n", cpu);
					stop();
				}
			} 
#ifdef vax
			else	{
				imagename = vmunix_name;
			}

		if ((howto & RB_ASKNAME) == 0) {
			imagename[1] = fs;
		}
		/*
		 * Try only once to load the pcs ucode for a 11/750 if
		 * the rev level indicates rev 95.  If it is greater
		 * than 95 then don't attempt a redundant load.
		 */
		if ((cpu == VAX_750) && (!ucodedone)) {	
			sid.cpusid = mfpr(SID);	/* read to see any changes */
			if (sid.cpu750.cp_urev == 95) {
				printf ("Updating 11/750 microcode ...\n");
				load_pcs();	
				ucodedone++;
			}
		}
#endif vax
#ifdef mips
		if(howto & RB_ASKNAME) {
			strcpy(tmp_name, imagename);
			if(rex_base) {
				cp = ub_argv[1];
				for(i=1;i<ub_argc;i++) {
					if(ub_argv[i][0] == '-') {
						cp = &ub_argv[i][1];
						switch (*cp++) {
						case 'b':
						   i++;
						   while(ub_argv[i][0] == NULL) {
							i++;	
							if(i >= ub_argc)
							  rex_rex('h');
						      }
						   if(ub_argv[i][0] >= 'a' && ub_argv[i][0] <= 'h'){
							partition = ub_argv[i][0]-'a';
							ub_argv[i][0] = '0';
							i = ub_argc;
							break;
						   } else {
						        break;
						   }
						}
					}
				 }
			}
			else {	
				cp = tmp_name;
				while(*cp != ')' && *cp != NULL)
					cp++;
				while(*cp != ',' && *cp != NULL && cp != &tmp_name[0])
					cp--;
				if((*cp++ == ',') && (*cp >= 'a') && 
					(*cp <= 'h') && (*(cp+1) == ')')) {
					partition = *cp - 'a';
					if((cpu == ST_DS3100) || (cpu == ST_DS5400))
						*cp = 'c'; /* make it 'c' partition */
					else
						*cp = '0'; /* give it a zero offset */
				}
			}
			if (!rex_base) {
			  _prom_close(prom_io);
			  if((prom_io = _prom_open(tmp_name, 0)) < 0 ) {
			    printf("can't open channel to boot driver\n");
			    continue;
			  }
			}
		}
#endif mips
		if ((io = open (imagename, 0)) < 0) {
			printf ("can't open %s\n", imagename);
			howto |= RB_ASKNAME;
			continue;
		}
		printf ("Loading %s ...\n", imagename);
		/* 
	 	 * If we are loading the supervisor, call load_ds which
	 	 * is a special load routine, just for the supervisor.
	 	 * load_image will not load the diagnostic supervisor.
	 	 */
		if (howto & RB_LOADDS) {
			if (load_ds(io)) {
				printf("Starting VAX Diagnostic Supervisor ...\n");
				start_ds();
			}
		} else
#ifdef vax
			load_image (howto, devtype, io);
#endif vax
#ifdef mips
			load_image (io);
#endif mips
		howto |= RB_ASKNAME;
		close (io);
		io = -1;
	}
}

/*
 * Functional Discription:
 *	This routine loads the diagnostic supervisor at address 0xfe00
 *	for VAX and 0xa010fe00 for MIPS.  It always assumes good memory.
 *
 * Inputs:
 *	io channel
 *
 * Outputs:
 *	0 = error loading
 *	1 = good load
 *
 */
load_ds (io)
	int     io;
{
	int     cnt, i;
#ifdef vax
	char	*addr = (char *) 0xfe00;
#endif vax
#ifdef mips
	char	*addr = (char *) 0xa010fe00;
#endif mips

	for (i = 0;; i += 10240) {
		if ((cnt = read (io, addr + i, 10240)) < 0)
			return(0);		/* bad load */
		if (!cnt)
			break;
	}
	printf ("\n");
	return(1);
}

#ifdef vax
char    pcs_ucode[] = "(a)pcs750.bin";

/*
 * Functional Discription:
 *	This routine is called only once if ucode is out of date on an
 *	11/750.  It returns nothing since is only called once.
 *
 * Inputs:
 *	none
 *
 * Outputs:
 *	none
 *
 */
load_pcs () {
	int     pcs_io;
	int     cnt;

	/*
	 * If the open of pcs750.bin fails it will tell us and we will want
	 * to return.
	 */
	pcs_io = open (pcs_ucode, 0);
	if (pcs_io < 0)
		return;
	/*
	 * Read in the file starting at memory location 0
	 * We read a little more than the actual file size.
	 * This way we can check to make sure we read exactly
	 * the number of bytes we expected to.
	 */
	cnt = read (pcs_io, (char *) 0, 23 * 512);
	/*
	 * Make sure the read was successful. If not, report it and return.
	 * Allow Unix to boot anyway
	 */
	if (cnt != 22 * 512)
		printf ("Load pcs750 failed: Error reading %s\n", pcs_ucode);
	else {
		pcsloadpatch ();			/* Load patchbits */
		pcsloadpcs ();				/* Load pcs code */
	}
	close(pcs_io);
	return;
}
#endif vax

#ifdef mips
/*
 * Functional Discription:
 *	This routine is called to start the VAX DS on an ISIS system.
 *	The ISIS prom is called to give control to and start the
 *	onboard CVAX.
 *
 * Inputs:
 *	none
 *
 * Outputs:
 *	none
 *
 */
start_ds()
{
	char *bootdev;
	char *cca;
	char *addr = (char *) 0xa010fd00;

	bootdev = (char *)prom_getenv("boot");	/* store the boot descriptor */
	cca = (char *)prom_getenv("cca");	/* store the cca descriptor */
	/* save the bootdev and cca consecutively at 'addr' */
	bcopy(bootdev, addr, strlen(bootdev)+1);
	bcopy(cca, addr+strlen(bootdev)+1, strlen(cca)+1);
	_prom_close(prom_io);			/* close the device fd */
	_prom_startcvax(0x110000);		/* start the CVAX */
}

/*
 * Functional Discription:
 * 	Convert an ASCII string of hex characters to a binary number.
 *
 * Inputs:
 *	pointer to string
 *
 * Outputs:
 *	value (int)
 *
 */
xtob(str)
	char *str;
{
	register int hexnum;
	if (str == NULL || *str == NULL)
		return(0);
	hexnum = 0;
	if (str[0] == '0' && str[1] == 'x')
		str = str + 2;
	for ( ; *str; str++) {
		if (*str >= '0' && *str <= '9')
			hexnum = hexnum * 16 + (*str - 48);
		else if (*str >= 'a' && *str <= 'f')
			hexnum = hexnum * 16 + (*str - 87);
		else if (*str >= 'A' && *str <= 'F')
			hexnum = hexnum * 16 + (*str - 55);
	}
	return (hexnum);
}

stop()
{
        if(!rex_base) {
	  _prom_close(prom_io);
	  _prom_restart();
	}
	else {
	  rex_rex('h');
	}
}
#endif mips
