# ifndef lint
static	char	*sccsid = "@(#)cpustat.c	4.5	(ULTRIX)	12/20/90";
# endif not lint

/************************************************************************
 *									*
 *			Copyright (c) 1988, 1990 by			*
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

#include <curses.h>
#include <signal.h>
#include <nlist.h>
#include <utmp.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/smp_lock.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/cpudata.h>
#ifdef mips
#include <sys/fixpoint.h>
#endif mips
#ifdef vax
#include <machine/vectors.h>
#endif /* vax */

/**************************************************************************
 * A program to display per cpu information 				  *
 *
 * to be compiled with curses library
 * cc -o cpustat cpustat.c -lcurses -ltermcap
 *
 *
 * This program is designed to display different sets of information 
 * about each cpu. In the default mode, the intent is to display as  
 * much as possible on a full screen. But the user can select specific 
 * flags for specific displays ('s' for statistics and 'c' for cpu  
 * states). If the program is to be modified to display new sets of 
 * information, the following need to be done:
 * 	1. If a new data structure is to be accessed in the kernel, 
 *	   update the namelist structure and modify the update_info() 
 *	   routine to extract the new information.
 *	2. Add a new flag to the command line and modify the get_opts()
 *	   routine to set the corresponding flag on.
 *	3. display() is the central routine controlling the display.
 *	   Wherever a test is done for the current optional flags
 *	   (sflg or cflg), add the test for the new flag. If the new
 *	   flag is set, call the corresponding routine to display the
 *	   header or the updated information.
 *	4. Add a new switch condition for the new flag (i.e. a one 
 *	   letter command is to be added to the commands available from
 *	   the full screen.). Do things similar to what is currently
 *	   being done for 'c' or 's' commands.
 *	5. Write two new routines to set up the header for the new set
 *	   of information (similar to setup_state()) and display the
 *	   new information (similar to upd_statistics_scrn()).
 *	6. Write a new routine to print the new information on the
 *	   screen (similar to pr_state()).
 *	7. Special things may have to be done depending on the format
 *	   in which the new information is to be displayed (like
 *	   format_state()).
 *	8. Update the help messages help_stats[], help_state[] and
 *	   help_info[].
 *	9. Update the string *cmd for the new command.
 *	10. Update the man page.
 *
 **************************************************************************/

/**************************************************************************
 * Modification History:
 *
 * 20 dec 90 - dlh
 *	corrected output of pid field
 *	changed display of vp state
 *
 * 14 Nov 90 - paradis
 *	Add support for displaying vector-processor statistics
 *	(only on vector-capable VAX architectures)
 *
 * 08 Dec 89 - jaw 
 *	add error check for select call for interrupted system call.
 * 09 Jun 89 - gmm
 *	Fixes to print load average correctly on mips.
 * 15 Jun 88 - George Mathew
 *	Changes to conform to the new way cpudata allocated
 * 01 Jun 88 - George Mathew
 *	Fixed some display alignment problems
 * 22 Feb 88 - George Mathew
 *	Created this progam.
 *
 ***************************************************************************/

struct nlist namelist[] = {
#define C_HZ		0
	{ "_hz" },
#define	C_CPUDATA	1
	{ "_cpudata" },
#define C_AVENRUN	2
	{ "_avenrun" },
#ifdef vax
#define C_VPTOTAL	3
	{ "_vptotal" },
#define C_VPMASK	4
	{ "_vpmask" },
#endif /* vax */
	{ "" }
};

#define	HOSTLEN	255
char	hname[HOSTLEN];
#define	DEF_INTERVAL	10		/* default interval between displays */
/* #define MAXCPU	32	 max. no of cpus/slots (now defined in cpudata.h) */

/* X-Y positions on the window */
#define XZERO 	0
#define YZERO 	0
#define	YTWO	2
#define YTHREE	3


int	activecpu;			/* mask of active cpus */
int	fflg,cflg,sflg,errflg;		/* command line options */
int	vflg = 0;			/* vector option */
int	vptotal = 0;			/* # vector processors */
unsigned vpmask = 0;			/* mask of installed VPs */
int	kmem;				/* file descriptor for kmem file */
int	deflt = 1;				/* default flag */
int	interval,iter;
char	*kernfile = "/vmunix";
char	*kmemfile = "/dev/kmem";
int	total_cpus,cpudata_size;
struct	cpudata *pcpudata, *cpudata_p1, *cpudata_p0;
#ifdef vax
struct  vpdata *vpdata_p1, *vpdata_p0;
#endif /* vax */
char	*state_hdr = "cpu      state      ipi-mask    proc    pid";
char	*statistics_hdr = "cpu  us%%  ni%%  sy%%  id%%    csw     sys     trap    intr     ipi   ttyin   ttyout";
#ifdef vax
char	*vp_hdr = "cpu     vp state    vp owner    chp cxsw    exp cxsw    succ req   failed req"; 
#endif /* vax */
char	printbuf[100];		/* buffer to hold the format string for printing*/
char *st_format = "       %s         %s     %c    ";
#ifdef vax
double	loadav[3];
#endif vax
#ifdef mips
fix	loadav[3];
#endif
long	cur_time;
int	users;	

char	*help_0 =  "cpustat HELP screen";
char	*help_stats[] = {
	"When in full screen mode, to change the display type:",
	"	s : Display cpu statistics only",
	"	c : Display cpu states only",
#ifdef vax
	"	v : Display vector processor states only",
#endif /* vax */
	"	d : Display default mode (stats) and states if less than 8 cpus",
	"	q : Quit this program",
	"	h : Display this help screen",
	"	Any other character refreshes the screen",
	0
	};

char	*help_state[] = {
	"The options for cpustat are:",
	" 	-f : display cpu statistics and state on full screen",
	"	-s : display only cpu statistics information",
	"	-c : display only cpu state information",
#ifdef vax
	"	-v : display only vector processor state information",
#endif /* vax */
	"	-h : display this help information",
	"	interval: interval in seconds before refreshing display",
	"	count: count of intervals before exiting the program",
	"If no option is used, all information will be displayed line by line",
	0
	};

char	*help_info[] = {
	"The Statistics fields are:",
	"	us%% : %% time spent in user mode",
	"	ni%% : %% time spent in nice mode",
	"	sy%% : %% time spent in system mode",
	"	id%% : %% time spent idle by the cpu",
	"	csw : number of context switches",
	"	sys: number of system calls",
	"	trap: number of traps",
	"	intr: number of device interrupts",
	"	ipi: number of inter processor interrupts",
	"	ttyin: number of input tty characters",
	"	ttyout: number of output tty characters",
	"The Cpu States fields are:",
	"	cpuid: Unique identifier of the cpu",
	"	state: Cpu state: B - Boot cpu, R - running, T - TB needs invalidation,",
	"		P - paniced, D - disable soft errors, S - cpu stopped",
	"	ipi-mask: Inter processor interrupt mask: P - panic, R - console print",
	"		S - schedule, T - TB invalidation, H - stop cpu",
	"	proc: if the cpu has an associated process (Y/N)",
	"	pid:  process id of the running process",
	0
	};
char 	*help_1 = "To get out of the help screen, type any character";
#ifdef vax
char	*cmd = "Commands: <c,d,v,h,q,s>";  /* commands available from within
					  * the full display screen */
#else
char	*cmd = "Commands: <c,d,h,q,s>";  /* commands available from within
					  * the full display screen */
#endif /* vax */

struct	stats {		/* structure to internally hold all statistics */
	float	st_usr;
	float	st_nice;
	float	st_sys;
	float	st_idle;
	int	st_csw;
	int	st_calls;
	int	st_trap;
	int	st_intr;
	int	st_ipi;
	int	st_ttyin;
	int	st_ttyout;
} stats;

struct  cpu_states {	/* structure to internally hold all states */
	char		cps_state[15];
	char		cps_ipi[15];
	char		cps_proc;
	short		cps_pid;
} cpu_states;

char	scale_data();
int	done();
extern int errno;

main(argc,argv)
int argc;
char *argv[];
{

	register int i;
	struct cpudata *pcpu;
	if(--argc > 0) {
		if (argv[1][0] == '-') {
			if(get_opts(argv[1])) {
				usage();
				exit(1);
			}
		argc--;
		argv++;
		}
		if(argc--)
			interval = atoi(argv[1]);
		if(argc)
			iter = atoi((++argv)[1]);
	}
	nlist(kernfile,namelist);
	if(namelist[C_AVENRUN].n_type == 0) {
		printf("Namelist could not be found for %s\n",kernfile);
		exit(1);
	}
	if ( (kmem = open(kmemfile,0)) == 0 ) {
		printf("Cannot open %s\n",kmemfile);
		exit(1);
	}

#ifdef vax
	if(namelist[C_VPMASK].n_value != 0) {
	    lseek(kmem, (long)namelist[C_VPMASK].n_value, 0);
	    read(kmem, &vpmask, sizeof(vpmask));
	}
	else {
	    vpmask = 0;
	}

	if(namelist[C_VPTOTAL].n_value != 0) {
	    lseek(kmem, (long)namelist[C_VPTOTAL].n_value, 0);
	    read(kmem, &vptotal, sizeof(vptotal));
	}
	else {
	    vptotal = 0;
	}
#endif /* vax */

	for(i=0; i<MAXCPU; i++) {
		lseek(kmem, (long)namelist[C_CPUDATA].n_value + 4*i, 0);
		read(kmem, &pcpu, sizeof (pcpu));
		if (pcpu) {
			activecpu = activecpu | (1<<i);
			total_cpus++;
		}
	}

	if( (cpudata_p0 = (struct cpudata *)calloc(total_cpus,sizeof(struct cpudata)) ) == NULL) {
		printf("No memory to allocate cpudata structures\n");
		exit(1);
	}
	if( (cpudata_p1 = (struct cpudata *)calloc(total_cpus,sizeof(struct cpudata)) ) == NULL) {
		printf("No memory to allocate cpudata structures\n");
		exit(1);
	}

#ifdef vax
	if(vptotal > 0) {
	    if( (vpdata_p0 = 
		    (struct vpdata *)calloc(total_cpus,
					    sizeof(struct vpdata)) ) == NULL) {
		printf("No memory to allocate vpdata structures\n");
		exit(1);
	    }
	    if( (vpdata_p1 = 
		    (struct vpdata *)calloc(total_cpus,
					    sizeof(struct vpdata)) ) == NULL) {
		printf("No memory to allocate vpdata structures\n");
		exit(1);
	    }
	}
#endif /* vax */

	signal(SIGINT,done);
	signal(SIGQUIT,done);
	display();
	/* NO RETURN */
}

get_opts(ptr)
char *ptr;
{
	char c;
	int errflg = 0;
	
	while (c = *ptr++) {
		switch(c) {
		case 'f':
			fflg++;
			break;
		case 'c':
			cflg++;
			deflt = 0;
			break;
		case 's':
			sflg++;
			deflt = 0;
			break;
#ifdef vax
		case'v':
			vflg++;
			deflt = 0;
			break;
#endif /* vax */
		case 'h':
			print_help();
			exit(0);
		case '-':
			break;
		default:
			errflg++;
		 
		}
	} 

	if(errflg)
		return(1);
	else
		return(0);
}

/* This is the routine which controls all the display. It loops for ever
 * updating the display at requested intervals.
 */

display()
{
	struct timeval tintv;
	int mask,tin;
	char c;
	int hdr = 0;

	if(interval == 0)
		tintv.tv_sec = DEF_INTERVAL;
	else
		tintv.tv_sec = interval;
	tintv.tv_usec = 0;

	if(fflg) {   /* if full screen option, set up the headers */
		gethostname(hname,HOSTLEN);
		init_screen();
		if(sflg || cflg || vflg) {
			if(sflg)
				setup_statistics();
			if(cflg)
				setup_state();
			if(vflg && vptotal)
				setup_vecstate();
		} else {  /* if only -f option used, try to display all info */
			setup_statistics();
			if( total_cpus <=8 )
				setup_state();
			if( vptotal && (total_cpus <= 6 ))
				setup_vecstate();
		}
	}
	for(;;) {
		update_info();  /* pick up the latest data from /dev/kmem */
		if(fflg) {
			standout();  /* start highligting display */
#ifdef vax
			mvprintw(YZERO,25,"%4.2f %4.2f %4.2f",loadav[0],loadav[1],loadav[2]);
#endif vax
#ifdef mips
			mvprintw(YZERO,25,"%4.2f %4.2f %4.2f",FIX_TO_DBL(loadav[0]),FIX_TO_DBL(loadav[1]),FIX_TO_DBL(loadav[2]));
#endif mips
			mvprintw(YZERO,45,ctime(&cur_time));
			move (YZERO,70);
			if(users != -1) {
				if(users == 1)
					addstr("1 user");
				else
					printw("%d users",users);
			}
			standend(); /* end highlighting display */
			if(sflg || cflg || vflg ) {
				if(sflg)
					upd_statistics_scrn();
				if(cflg) 
					upd_state_scrn();
				if(vflg)
					upd_vec_scrn();
			} else {
				upd_statistics_scrn();
				if( total_cpus <=8 ) 
					upd_state_scrn();
				if( total_cpus <= 6 )
					upd_vec_scrn();
			}
			mvprintw(LINES-1,XZERO,cmd);
			refresh();  /* put the stuff up on the screen */
		} else {
			if(sflg || cflg || vflg) {
				if (cflg) {
					if( (hdr == 0) || (sflg) ) {
						printf(statistics_hdr);
						printf("\n");
					}
					pr_statistics();
				}
				if(sflg) {
					if( (hdr == 0) || (cflg) )
						printf("\n");
						printf("%s\n",state_hdr);
					pr_state();
				}
#ifdef vax
				if((vflg != 0) && (vptotal != 0)) {
					if( (hdr == 0) || (vflg) ) {
						printf("\n");
						printf("%s\n", vp_hdr);
					}
					pr_vecstate();
				}
#endif /* vax */
				if(hdr == 0)
					hdr = 19/total_cpus; /* to print header*/
				else
					hdr--;
			} else {
				printf(statistics_hdr);
				printf("\n");
				pr_statistics();
				printf("%s\n",state_hdr);
				pr_state();
#ifdef vax
				if(vptotal > 0) {
					printf("%s\n", vp_hdr);
					pr_vecstate();
				}
#endif /* vax */
			}
			printf("\n");
		}
		if(--iter == 0)
			done();
		if(fflg) {  /* wait for any user input for full screen option */
			tin = 1;	/* mask for stdin for select() */
			/* pick up the user typed character within the desired
			 * time interval */
			errno = 0;
			mask = select(2, &tin, (int *)0, (int *)0, &tintv);

			if( !errno && mask && tin) {
				int saved;
				c = getchar();
				switch(c) {

				case 'q':
					done();

				case 'h':
					display_help();
					saved = tintv.tv_sec;

					/* display the help message for a max. 
					 * of 10 mins. before going back to the
					 * original display */

					tintv.tv_sec = 10 * 60; /* max. 10 mins */
					tin = 1;
					errno = 0;
					mask = select(2,&tin,(int *)0, (int *)0,&tintv);
					if(!errno && mask && tin)
						c = getchar();
					tintv.tv_sec = saved;

					/* setup the screen as was before 
					 * putting up the help screen */

					setup_init();
					if(sflg || cflg || vflg) {
						if(sflg)
							setup_statistics();
						if(cflg)
							setup_state();
						if(vflg && vptotal)
							setup_vecstate();
					} else {
						setup_statistics();
						if( total_cpus <=8 )
							setup_state();
						if(vptotal&&(total_cpus <= 6))
							setup_vecstate();
					}
					break;

				case 'c':
					if(sflg || vflg || (!cflg) ) {
						deflt = 0;
						cflg = 1;
						sflg = 0;
						vflg = 0;	
						setup_init();
						setup_state();
					}
					break;

#ifdef vax
				case 'v':
					if(sflg || cflg || (!vflg) ) { 
						deflt = 0;
						vflg = 1;
						cflg = 0;
						sflg = 0;
						setup_init();
						if(vptotal) 
							setup_vecstate();
					}
					break;
#endif /* vax */

				case 's':
					if(cflg || vflg || (!sflg) ) {
						deflt = 0;
						sflg = 1;
						cflg = 0;
						vflg = 0;
						setup_statistics();
					}
					break;
				case 'd':  /* display the default option 
					    * ie. only f flag */
					if(!deflt) {
						deflt = 1;
						vflg = cflg = sflg = 0;
						setup_statistics();
						if(total_cpus <= 8)
							setup_state();
						if(vptotal&&(total_cpus <= 6))
							setup_vecstate();
					}
				default:   /* any other charcter will refresh
					    * screen with latest data */

					break;  
				}
			}
		} else  {
			if(interval == 0)
				done();
			else
				sleep(interval);
		}
	}
}

	/* initializes the screen */

init_screen()
{
	initscr();	/* init curses package */
	crmode();	/* set terminal into cbreak mode */
	noecho();       /* set terminal into no-echo mode */
	return(0);
}

	/* set up the headers for displaying cpu state information */
setup_vecstate()
{
#ifdef vax
	register int y;

	if(vptotal == 0) {
		return(0);
	}

	if(cflg || sflg || deflt)
		y = YTWO+(2 * total_cpus)+3;
	else {
		y = YTWO;
		setup_init();
	}
	standout(); 
	mvprintw(y,XZERO,vp_hdr);
	standend(); 
#endif /* vax */
	return(0);
}

	/* set up the headers for displaying cpu state information */
setup_state()
{
	register int y;
	if(vflg || sflg || deflt)
		y = YTWO+total_cpus+2;
	else {
		y = YTWO;
		setup_init();
	}
	standout(); 
	mvprintw(y,XZERO,state_hdr);
	standend(); 
	return(0);
}

	/* set up the headers for displaying cpu statistics */
setup_statistics()
{
	setup_init();
	standout();
	mvprintw(YTWO,XZERO,statistics_hdr);
	standend();
	return(0);
}

setup_init()
{
	clear();
	standout();
	mvprintw(YZERO,0,hname);
	standend();
	return(0);
}

/* update_info() picks up the latest statistics from the kernel */

update_info()
{
	register int i,t,j;
	register struct cpudata *save_p0, *save_p1;
#ifdef vax
	struct vpdata *save_v0, *save_v1;
#endif /* vax */
	struct cpudata *cpu_ptr;

#define X(fld) t = save_p0->fld[j]; save_p0->fld[j] -= save_p1->fld[j];save_p1->fld[j] = t
#define Y(fld) t = save_p0->fld; save_p0->fld -= save_p1->fld; save_p1->fld = t

#ifdef vax
#define VY(fld) t = save_v0->fld; save_v0->fld -= save_v1->fld; save_v1->fld = t
#endif /* vax */


	if(fflg) {
		lseek(kmem,(long)namelist[C_AVENRUN].n_value,0);
		read(kmem,loadav,sizeof(loadav));
		time(&cur_time);
		users = user_count();
	}
	save_p0 = cpudata_p0;
	save_p1 = cpudata_p1;
#ifdef vax
	if(vptotal > 0) {
		save_v0 = vpdata_p0;
		save_v1 = vpdata_p1;
	}
#endif /* vax */
	for(i=0; i<MAXCPU; i++) {
		if(activecpu & (1<<i)) {
			lseek(kmem,(long)namelist[C_CPUDATA].n_value + (4*i),0);
			read(kmem,&cpu_ptr,4);
			lseek(kmem, (long)cpu_ptr,0);
			read(kmem,save_p0,sizeof(struct cpudata));
#ifdef vax
			if((vptotal > 0) && (vpmask & (1<<i)) &&
						(save_p0->cpu_vpdata)) {
				lseek(kmem, (long)save_p0->cpu_vpdata, 0);
				read(kmem, save_v0, sizeof(struct vpdata));
				VY(vpd_ccsw);
				VY(vpd_ecsw);
				VY(vpd_success);
				VY(vpd_failed);
			}
#endif /* vax */
			for(j=0; j<CPUSTATES; j++)  {
				X(cpu_cptime);
			}
			Y(cpu_ttyin);
			Y(cpu_ttyout);
			Y(cpu_switch);
			Y(cpu_trap);
			Y(cpu_syscall);
			Y(cpu_intr);
			Y(cpu_ip_intr);
			save_p0++;
			save_p1++;
#ifdef vax
			save_v0++;
			save_v1++;
#endif /* vax */
		}
	}

	return(0);
}

upd_statistics_scrn()
{
	register int i;
	register struct cpudata *cpu_ptr;
	cpu_ptr = cpudata_p0;
	for(i=0; i<total_cpus; i++) {
		format_statistics(cpu_ptr);
		mvprintw(YTHREE+i,XZERO," %d  %4.1f %4.1f %4.1f %4.1f ",cpu_ptr->cpu_num,stats.st_usr,stats.st_nice,stats.st_sys,stats.st_idle);
		mvprintw(YTHREE+i,XZERO+24,printbuf+24,stats.st_csw,stats.st_calls,stats.st_trap,stats.st_intr,stats.st_ipi,stats.st_ttyin,stats.st_ttyout);
		cpu_ptr++;
	}
	return(0);
}

format_statistics(cpu_ptr)
register struct cpudata *cpu_ptr;
{
	register i;
	double time;
	static char tmp_buf[] = " %4d   ";

	printbuf[0] = NULL;  /* initialize the print buffer */
	time = 0;
	for(i=0; i<CPUSTATES; i++)
		time += cpu_ptr->cpu_cptime[i];
	if(time == 0.0)
		time = 1;
	stats.st_usr = (100.0 * cpu_ptr->cpu_cptime[CP_USER])/time;
	stats.st_nice = (100.0 * cpu_ptr->cpu_cptime[CP_NICE])/time;
	stats.st_sys = (100.0 * cpu_ptr->cpu_cptime[CP_SYS])/time;
	stats.st_idle = (100.0 * cpu_ptr->cpu_cptime[CP_IDLE])/time;
	strcat(printbuf," %4.1f %4.1f %4.1f %4.1f ");
	tmp_buf[4] = scale_data(cpu_ptr->cpu_switch,&stats.st_csw);
	strcat(printbuf,tmp_buf);
	tmp_buf[4] = scale_data(cpu_ptr->cpu_syscall,&stats.st_calls);
	strcat(printbuf,tmp_buf);
	tmp_buf[4] = scale_data(cpu_ptr->cpu_trap,&stats.st_trap);
	strcat(printbuf,tmp_buf);
	tmp_buf[4] = scale_data(cpu_ptr->cpu_intr,&stats.st_intr);
	strcat(printbuf,tmp_buf);
	tmp_buf[4] = scale_data(cpu_ptr->cpu_ip_intr,&stats.st_ipi);
	strcat(printbuf,tmp_buf);
	tmp_buf[4] = scale_data(cpu_ptr->cpu_ttyin,&stats.st_ttyin);
	strcat(printbuf,tmp_buf);
	tmp_buf[4] = scale_data(cpu_ptr->cpu_ttyout,&stats.st_ttyout);
	strcat(printbuf,tmp_buf);
	
	return(0);
}

char
scale_data(input,output)
unsigned input;
int *output;
{
	if(input >= 1048576) {
		*output = input/1048576;
		return('m'); }
	if(input >= 1024) {
		*output = input/1024;
		return('k');
	}
	*output = input;
	return(' ');
}


pr_statistics()
{
	register int i;
	register struct cpudata *cpu_ptr;
	cpu_ptr = cpudata_p0;
	for(i=0; i<total_cpus; i++) {
		printf(" %d ",cpu_ptr->cpu_num);
		format_statistics(cpu_ptr);
		printf(printbuf,stats.st_usr,stats.st_nice,stats.st_sys,stats.st_idle,stats.st_csw,stats.st_calls,stats.st_trap,stats.st_intr,stats.st_ipi,stats.st_ttyin,stats.st_ttyout);
		cpu_ptr++;
		printf("\n");
	}
	return(0);
}

done()
{
	if(fflg) {
		mvprintw(LINES-1,0,"\n");
		refresh();
		endwin();
	}
	free((char *)cpudata_p0);
	free((char *)cpudata_p1);
	exit(0);
}

#ifdef vax
char * get_vp_state();
#endif /* vax */

upd_vec_scrn()
{
#ifdef vax
	register int i,y;
	register struct cpudata *cpu_ptr;
	register struct vpdata *vp_ptr;
	struct proc tmp_proc;

	if(vptotal == 0) {
		return(0);
	}

	if(cflg || sflg || deflt)
		y = YTHREE+(2 * total_cpus)+3;
	else
		y = YTHREE;
	cpu_ptr = cpudata_p0;
	vp_ptr = vpdata_p0;

	for(i=0; i<total_cpus; i++) {
	    if(cpu_ptr->cpu_vpdata) {
		if (vp_ptr->vpd_proc) {
		    lseek(kmem,(long)vp_ptr->vpd_proc,0);
		      read(kmem,&tmp_proc,sizeof(struct proc));
		    sprintf(printbuf,
		      "%3d    %8s    %8d    %8d    %8d    %8d   %8d",
		      cpu_ptr->cpu_num, get_vp_state(vp_ptr->vpd_state),
		      tmp_proc.p_pid, vp_ptr->vpd_ccsw, vp_ptr->vpd_ecsw,
		      vp_ptr->vpd_success, vp_ptr->vpd_failed);
		} else {
		    sprintf(printbuf,
		      "%3d    %8s    %8x    %8d    %8d    %8d   %8d",
		      cpu_ptr->cpu_num, get_vp_state(vp_ptr->vpd_state),
		      vp_ptr->vpd_proc, vp_ptr->vpd_ccsw, vp_ptr->vpd_ecsw,
		      vp_ptr->vpd_success, vp_ptr->vpd_failed);
		}


		mvprintw(y+i,XZERO,printbuf);
	    }
	    else {
		mvprintw(y+i,XZERO,"%3d",cpu_ptr->cpu_num);
		clrtoeol();
	    }
	    cpu_ptr++;
	    vp_ptr++;
	}
#endif /* vax */
	return(0);
}

pr_vecstate()
{
#ifdef vax
	register int i,y;
	register struct cpudata *cpu_ptr;
	register struct vpdata *vp_ptr;
	struct proc tmp_proc;

	cpu_ptr = cpudata_p0;
	vp_ptr = vpdata_p0;
	for(i=0; i<total_cpus; i++) {
	    if(cpu_ptr->cpu_vpdata) {
		if (vp_ptr->vpd_proc) {
		    lseek(kmem,(long)vp_ptr->vpd_proc,0);
		      read(kmem,&tmp_proc,sizeof(struct proc));
		    printf("%3d    %8s    %8d    %8d    %8d    %8d   %8d\n",
		      cpu_ptr->cpu_num, get_vp_state(vp_ptr->vpd_state),
		      tmp_proc.p_pid, vp_ptr->vpd_ccsw, vp_ptr->vpd_ecsw,
		      vp_ptr->vpd_success, vp_ptr->vpd_failed);
		} else {
		    printf("%3d    %8s    %8d    %8d    %8d    %8d   %8d\n",
		      cpu_ptr->cpu_num, get_vp_state(vp_ptr->vpd_state),
		      vp_ptr->vpd_proc, vp_ptr->vpd_ccsw, vp_ptr->vpd_ecsw,
		      vp_ptr->vpd_success, vp_ptr->vpd_failed);
		}

	    }
	    else {
		printf("%3d",cpu_ptr->cpu_num);
	    }
	    cpu_ptr++;
	    vp_ptr++;
	}
#endif /* vax */
	return(0);
}

#ifdef vax
char	vp_state_string[9];

char * 
get_vp_state(s)
int s;
{
	if(s == VPD_ABSENT) {
		return("ABSENT");
	}

	vp_state_string[0] = '\0';

	if(s & VPD_ALIVE) {
		strcat(vp_state_string, "OK");
	}
	if(s & VPD_DEAD) {
		strcat(vp_state_string, *vp_state_string ? ",DEAD" : "DEAD");
	}
	if(s & VPD_ENABLED) {
		strcat(vp_state_string, *vp_state_string ? ",ENA" : "ENA");
	}
	if(s & VPD_DISABLED) {
		strcat(vp_state_string, *vp_state_string ? ",DIS" : "DIS");
	}
	return(vp_state_string);
}
#endif /* vax */

upd_state_scrn()
{
	register int i,y;
	register struct cpudata *cpu_ptr;
	if(sflg || deflt)
		y = YTHREE+total_cpus+2;
	else
		y = YTHREE;
	cpu_ptr = cpudata_p0;
	for(i=0; i<total_cpus; i++) {
		mvprintw(y+i,XZERO," %d ",cpu_ptr->cpu_num);
		format_state(cpu_ptr);
		mvprintw(y+i,XZERO+6,cpu_states.cps_state);
		mvprintw(y+i,XZERO+16,cpu_states.cps_ipi);
		mvprintw(y+i,XZERO+33,"%c",cpu_states.cps_proc);
		if(cpu_states.cps_pid != -1) {
			mvprintw(y+i,XZERO+40,"%d",cpu_states.cps_pid);
			clrtoeol();
		} else
			clrtoeol();
		cpu_ptr++;
	}
	return(0);
}

format_state(cpu_ptr)
register struct cpudata *cpu_ptr;
{

	register int i;
	struct proc tmp_proc;

	cpu_states.cps_state[0] = NULL;
	if(cpu_ptr->cpu_state & CPU_BOOT)
		strcat(cpu_states.cps_state,"B");
	else
		strcat(cpu_states.cps_state," ");
	if(cpu_ptr->cpu_state & CPU_RUN)
		strcat(cpu_states.cps_state,"R");
	else
		strcat(cpu_states.cps_state," ");
	if(cpu_ptr->cpu_state & CPU_TBI)
		strcat(cpu_states.cps_state,"T");
	else
		strcat(cpu_states.cps_state," ");
	if(cpu_ptr->cpu_state & CPU_PANIC)
		strcat(cpu_states.cps_state,"P");
	else
		strcat(cpu_states.cps_state," ");
	if(cpu_ptr->cpu_state & CPU_SOFT_DISABLE)
		strcat(cpu_states.cps_state,"D");
	else
		strcat(cpu_states.cps_state," ");
	if(cpu_ptr->cpu_state & CPU_STOP)
		strcat(cpu_states.cps_state,"S");
	else
		strcat(cpu_states.cps_state," ");
	cpu_states.cps_ipi[0] = NULL;
	if(cpu_ptr->cpu_int_req & IPIMSK_PANIC)
		strcat(cpu_states.cps_ipi,"P");
	if(cpu_ptr->cpu_int_req & IPIMSK_PRINT)
		strcat(cpu_states.cps_ipi,"R");
	else
		strcat(cpu_states.cps_ipi," ");
	if(cpu_ptr->cpu_int_req & IPIMSK_SCHED)
		strcat(cpu_states.cps_ipi,"S");
	else
		strcat(cpu_states.cps_ipi," ");
	if(cpu_ptr->cpu_int_req & IPIMSK_TBFLUSH)
		strcat(cpu_states.cps_ipi,"T");
	else
		strcat(cpu_states.cps_ipi," ");
	if(cpu_ptr->cpu_int_req & IPIMSK_KDB)
		strcat(cpu_states.cps_ipi,"K");
	else
		strcat(cpu_states.cps_ipi," ");
	if(cpu_ptr->cpu_int_req & IPIMSK_STOP)
		strcat(cpu_states.cps_ipi,"H");
	else
		strcat(cpu_states.cps_ipi," ");
	if(cpu_ptr->cpu_noproc) {
		cpu_states.cps_proc = 'N';
		cpu_states.cps_pid = -1;
	} else {
		cpu_states.cps_proc = 'Y';
		if(cpu_ptr->cpu_proc != NULL) {
			lseek(kmem,(long)cpu_ptr->cpu_proc,0);
			read(kmem,&tmp_proc,sizeof(struct proc));
			cpu_states.cps_pid = tmp_proc.p_pid;
		} else
			cpu_states.cps_pid = -1;
	}
	
	return(0);
}

pr_state()
{
	register int i;
	register struct cpudata *cpu_ptr;
	cpu_ptr = cpudata_p0;
	for(i=0; i<total_cpus; i++) {
		printf(" %d ",cpu_ptr->cpu_num);
		format_state(cpu_ptr);
		printf(st_format,cpu_states.cps_state,cpu_states.cps_ipi,cpu_states.cps_proc);
		if(cpu_states.cps_pid != -1)
			printf("%d",cpu_states.cps_pid);
		cpu_ptr++;
		printf("\n");
	}
	return(0);
}

display_help()
{
	register int i,j;
	register char **hlp_ptr;
	clear();
	mvprintw(YZERO,COLS/2-strlen(help_0)/2,help_0);
	hlp_ptr = help_stats;
	for(i=0; hlp_ptr[i]; i++)
		mvprintw(i,XZERO,hlp_ptr[i]);
	hlp_ptr = help_info;
	for(j=0; hlp_ptr[j]; j++)
		mvprintw(i+j,XZERO,hlp_ptr[j]);
	mvprintw(i+j,XZERO,help_1);
	refresh();
	return(0);
}

print_help()
{
	register int i;
	register char **hlp_ptr;
	usage();
	hlp_ptr = help_state;
	for(i=0; hlp_ptr[i];i++)
		printf("%s\n",hlp_ptr[i]);
	hlp_ptr = help_info;
	for(i=0; hlp_ptr[i];i++)
		printf("%s\n",hlp_ptr[i]);
	return(0);
}

usage()
{
	printf("Usage: cpustat [-fcsh] [interval [count]]\n");
	return(0);
}

/*
 *	int user_count()
 *		Return a count of the number of users on.  Returns -1 on
 *		error
 *
 *	int first;
 *		-1 on error; 1 on first time; 0 otherwise
 *
 *	time_t mtime;
 *		last time file was modified
 *
 *	FILE *strm;
 *		stream for /etc/utmp (And file descriptor for fstat)
 *
 *	int count;
 *		last calculated count of number of users on.  Only
 *		recalculated when /etc/utmp is modified
 */


user_count ()
{
    static int  first = 1;
    static time_t  mtime;
    static FILE *strm;
    static int  count;
    struct utmp utmp;
    struct stat statbuf;

/*
 *  If we already had an error once, don't even bother
 */
    if (first == -1)
	return (1);

/*
 *  The first time through the loop, open the file.  If there is an
 *  error; give up
 */
    if (first)
    {
	strm = fopen ("/etc/utmp", "r");
	if (strm == NULL)
	{
	    first = -1;
	    return (-1);
	}
    }

/*
 *  Find last modifed time of file
 */
    if (fstat (strm -> _file, &statbuf))
    {
	first = -1;
	return (-1);
    }

/*
 *  If this is the first time, or the file has been modified, calculate
 *  the number of users on (also clear the first time flag)
 */
    if (first || mtime != statbuf.st_mtime)
    {
	first = 0;
	mtime = statbuf.st_mtime;

	rewind (strm);
	count = 0;
	while (fread (&utmp, sizeof (utmp), 1, strm))
	{
	    if (utmp.ut_name[0] != '\0')
		count++;
	}
    }

    return (count);
}
