#ifndef lint
static	char	*sccsid = "@(#)finder.c	4.6	(ULTRIX)	10/9/90";
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
#include <ctype.h>
#include <stdio.h>
#include <nlist.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/devio.h>
#include <sys/mtio.h>
#include <sys/dkio.h>
#include <sys/param.h>
#include <machine/cpuconf.h>
#ifdef vax
#include <machine/rpb.h>
#endif vax
#ifdef mips
#include <machine/entrypt.h>
#include <sys/sysinfo.h>
#endif mips

#define 	MAXDEVICES	300	/* maximum number of devs to find  */
#define		MAXDISKS	32	/* maximum number of non-ra disks  */
#define		MAX_RA_DISKS	256	/* maximum number of ra disks      */
#define		MAXTAPES	32	/* maximum number of any tape type */

#define		RA_BASE_MAJOR	60	/* Base major number for ra char dev */

#define SUPPORTED_ROOT  	0	/* Dev supported for root installs   */
#define UNSUPPORTED_ROOT	1	/* Dev unsupported for root installs */
#define DEF_A_PARTITION		32768	/* Size of a default "a" partition   */
#define DEF_B_PARTITION		50160	/* Size of a default "b" partition   */
#define MIN_CAPACITY		(DEF_A_PARTITION + DEF_B_PARTITION)

/*
 * ra disks can occupy a range of major numbers.  For this reason the unit
 * number is held in both the major and minor numbers; not just the upper 5
 * bits of the minor number as before.  Each major number represents 32 disks.
 * For this reason the major number is the base major number added to the
 * div by 32 to determine which "set" if 32 disks is referenced.  The minor
 * number is which disk within the "set" of 32 and is the mod of 32 shifted
 * left by 3 bits because the low 3 bits represent partition.
 */
#define RAMAJOR(index, base)    ((index/32) + base)
#define RAMINOR(index)          ((index%32) * 8)

#define maketape(x,y)	((dev_t)(((x)<<8)  | ((((y&0xfc)<<3) | (y&3)))))

struct d 
{
	char *type;			/* Ultrix name of device */
	int majnum;			/* major number of device */
};


struct d roots [] =			/* disks that can be root devices */
	 {
	"ra",   RA_BASE_MAJOR,	"rz",   56,
#ifdef vax
 	"hp",	4,	"rd",	47,
#endif vax
	"\0",   -1
	 };

struct d installs [] =			/* install devices */
	{
	"ra",   RA_BASE_MAJOR,	"tms",  36,	"rz",   56,
#ifdef vax
	"tu",	5,	"ts",	16,	"mu",	19,	"st",	46,
	"tz",	55,
#endif vax
	"\0",   -1
	};

struct d disks [] =			/* all disk devices */
	{
	"ra",   RA_BASE_MAJOR,	"rz",   56,
#ifdef vax
	"hp",	4,	"rd",	47,
#endif vax
	"\0",   -1
	};


struct f {
	char type[20];			/* device type (TU77,RA60...) */
	char name[20];			/* Ultrix name */
	char interface[20];		/* controller name (HSC70,UDA50) */
	int unit;			/* Unit number assigned by kernel */
	int plug;			/* physical plug number */
	int link;			/* number of controller is linked to */
	int linktype;			/* bus type */
	int trlevel;			/* TR level or BI node number */
	int adpt_num;			/* BI number or SBI number*/
	short rctlr_num;		/* remote controller number,i.e. hsc #*/
};

int FMAX = 0;
int supported_root();
struct f found[MAXDEVICES];		/* devices found */
struct devget dev_info;			/* IOCTL structure */
/*
 * The following is a list of devices which are not supported as root
 * installation devices.
 */
char *unsupported_roots[] = {
	"RX",		/* Floppy - too small			*/
	"RD31",		/* Disk - too small			*/
	"RD32",		/* Disk - too small			*/
	"RD51",		/* Disk - too small			*/
	"RD52",		/* Disk - too small			*/
	"RZ22",		/* Disk - too small			*/
	"ESE20",	/* Limited number of power hits problem	*/
	"RRD",		/* Read-only media			*/
	""		/* Terminating element in the array	*/
};

#ifdef vax
struct rpb rpb;
#endif vax

char *header0, *getenv();
static char header1[]="Selection   Device     ULTRIX     Device       Controller   Controller ";
static char header2[]="            Name       Name       Number       Name         Number     ";
static char header3[]="---------------------------------------------------------------------- ";

/**************************************************************************/
/* FINDER:								  */
/*		This routine finds all the devices that can be used as    */
/*	installation devices,root devices, or all disks.  It searches for */
/*	devices in the previously defined in the structures.  A system    */
/*	call, using mknod, is issued to create a device special file for  */
/*	each device.  The special file is then opened and an ioctl call   */
/*	is issued to the device.  Information returned is stored in the   */
/*	'found' structure and is printed after the device list has been   */
/*	exhausted.							  */
/* 									  */
/**************************************************************************/
/*  Modification History						  */
/*									  */	
/*  000 - July,   1986	- Bob Fontaine created				  */
/*									  */
/*  001 - August, 1986  - Bob Fontaine					  */	
/*   		Added the message for boot command.			  */
/*  									  */
/*  002 - August 14, 1986 -  Tungning Cherng				  */
/*		Added the -f flag to reduce the redundant check in order  */
/*			to have a faster output.		          */
/*									  */	
/*  003 - Mar 9, 1987 - Tungning Cherng					  */
/*		Added the CVAX cpu support.				  */
/*									  */	
/*  004 - July 9, 1987 - Tungning Cherng				  */
/*		RD51,RD52 and RD31 not for system disk, only data disk.	  */
/*									  */	
/*  005 - June,7,1988 - Tungning cherng					  */
/* 		Supported the C_VAXSTAR(VAX420), and VAX8820.	  	  */
/* 		Supported the RZ23 disk, RRDxx cdrom, and TZ30 tape.	  */
/*  									  */
/*  006 - May 11, 1989 - Tim Burke					  */
/*		Allow more than 32 "ra" type disks.  Change major	  */
/*		number of "ra"						  */
/*									  */
/*  007 - May 31, 1989 - Jon Wallace					  */
/*		New User interface that allows paging through disk	  */
/*		selections of more than 16 disks.			  */
/*									  */
/*  008 - Jun 27, 1989 - Jon Wallace					  */
/*		Added ULTRIX name to user menu, and discontinued	  */
/*		TA90 support per Tim Burke.				  */
/*									  */
/*  009 - Sep 13, 1989 - Tim Burke                                        */
/*              Added the supported_root routine.  This determines if     */
/*              the disk qualifies as a supported system disk.            */
/*									  */
/*  010 - Nov 09, 1989 - Jon Wallace					  */
/*		Added -w option that looks to see if the console	  */
/*		device is graphic or not				  */
/*									  */
/*  011 - Jul 05, 1909 - Pete Keilty					  */
/*		Added new DEV_BICI and DEV_XMICI defines.		  */
/*		This informs which bus the CI is on and booting from.	  */
/*									  */
/*  012 - Aug 31, 1990 - Jon Wallace					  */
/*		Added new rom support for DS5000 machines with code	  */
/*		from Joe Szczypek					  */
/*									  */
/*  013 - Sep 07, 1990 - Ali Rafieymehr					  */
/*		Added support for the VAX9000.				  */
/*									  */
/*  014 - Oct 09, 1990 - Joe Szczypek					  */
/*		Corrected new ROM support.				  */
/*									  */
/**************************************************************************/

main(argc,argv)
int argc;
char *argv[];
{

        int answer, anschk, fflag=0, rflag=0;
        int COUNTER=0, ERR_FLAG=0, PAGE_FLAG=0;
        char ans[8], line[128];
        register int i;
        FILE *fp1, *fp2;
        char *path1="/tmp/finder.tab";
        char *path2="/tmp/finder.dev";
        header0=getenv("ROUTINE");

	switch(argv[1][1])
	{

		case 'i':
			getinstalls();
			break;

		case 'r':
			getroots();
			rflag++;
			break;

		case 'd':
			getdisks();
			break;

		case 'f':
			/* to reduce redundant check, show the previous form */
			fflag++;
			break;

		case 'w':
			/* find out if the machine has graphics capability */
			getwscons();
			exit(0);

		default:
			fprintf(stderr,"usage: finder -[irdfw]\n");
			break;
	}

	if(FMAX == 0 && fflag==0)
	{
		exit(-1);
	}

	if (fflag==0)
	{
		if ((fp1=fopen(path1,"w"))==NULL)
		{
			fprintf(stderr,"open %s failed\n",path1);
			exit(-1);
		}

		if ((fp2=fopen(path2,"w"))==NULL)
		{
			fprintf(stderr,"open %s failed\n",path2);
			exit(-1);
		}

		for(i=0;i<FMAX;i++)
		{
			fprintf(fp1,"   %2d       %-10s ",i+1,found[i].type);
			fprintf(fp1,"%3s%-3d     ",found[i].name,found[i].unit);
			fprintf(fp1," %3d         ",found[i].plug);
			fprintf(fp1,"%-10s   %3d\n",found[i].interface,found[i].link);
			fprintf(fp2,"%s %s %d\n",found[i].type,found[i].name,found[i].unit);
		}
		fclose(fp1);
		fclose(fp2);
	}

	if ((fp1=fopen(path1,"r"))==NULL)
	{
		fprintf(stderr,"read %s failed\n",path1);
		exit(-1);
	}

	if ((fp2=fopen(path2,"r"))==NULL)
	{
		fprintf(stderr,"read %s failed\n",path2);
		exit(-1);
	}

	answer = 0;
	for (;;)
	{
		fprintf(stderr, "\n%s TABLE \n\n", header0);
                Headeroutine();
                for (COUNTER=0, FMAX=0; fgets(line, sizeof(line), fp1)!=NULL; COUNTER++, FMAX++)
                {
                        if (COUNTER==16)
                        {
                                fprintf(stderr, "%s\n", header3);
                                ERR_FLAG=Multipage_routine(ans, ERR_FLAG);
                                answer=atoi(gets(ans));
                                if ( *ans == '\0' )
                                {
                                        COUNTER=0;
                                        fprintf(stderr, "\n\n%s TABLE \n\n", header0); 
                                        Headeroutine();
                                }
                                else
                                {
                                        PAGE_FLAG=1;
                                        break;
                                }
                        }
                        fprintf(stderr,"%s",line);
                }

                if (PAGE_FLAG == 0)
                {
                        fprintf(stderr, "%s\n", header3);
                        ERR_FLAG=Unipage_routine(ERR_FLAG, FMAX);
                        answer=atoi(gets(ans));
                        if ( *ans == '\0' )
                        {
                                if ( FMAX <= 16 )
                                        ERR_FLAG=1;
                                else
                                        ERR_FLAG=0;

                                PAGE_FLAG=0;
                                (void) rewind(fp1);
                                continue;
                        }
                }

                anschk=choicechk(ans);
                if(FMAX < answer || answer <= 0 || anschk == 0)
                        ERR_FLAG=1;
                else
                {
			answer--;
			break;
                }

                PAGE_FLAG=0;
                (void) rewind(fp1);

        }
        fclose(fp1);

	for (i=0; fgets(line,sizeof(line),fp2)!=NULL && i!=answer; i++)
		;
	printf("%s",line);
	fclose(fp2);	

	if (rflag)
		showboot(answer);

	exit(0);
}

/**************************************************************************/
Headeroutine()                          /* User Interface headers/titles */
{
        fprintf(stderr, "%s\n", header1);
        fprintf(stderr, "%s\n", header2);
        fprintf(stderr, "%s\n", header3);
}

/**************************************************************************/
Unipage_routine(ERR_FLAG, FMAX)         /* Single screen User interface */
        int     ERR_FLAG, FMAX;
{
        if (ERR_FLAG == 1)
                fprintf(stderr,"\nSelect a number between 1 - %d : ", FMAX);
        else
                fprintf(stderr,"\nEnter your choice: ");

        return(0);
}

/*****************************************************************************/
Multipage_routine(ans, ERR_FLAG)        /* Multiple screen User interface */
        char    ans[8];
        int     ERR_FLAG;
{
        if (ERR_FLAG == 1)
        {
                fprintf(stderr,"\nYour previous choice, '%s', is not a valid selection.", ans);
                fprintf(stderr,"\nEnter another choice OR press RETURN for next screen: ");
        }
        else
                fprintf(stderr,"\nEnter your choice OR press RETURN for next screen: ");

        return(0);
}

/**************************************************************************/
getinstalls()
{

	register int i;
	int retval,times,to,dev;
	char command[50],spec[25],devfile[20];
	struct d *n;
	int num_disks;



	for(n=installs;strcmp(n->type,"\0") != 0;n++)
	{
		/*
		 * There can be more than 32 ra type disks.  Up the limit
		 * of devices to examine in this case.
		 */
		if (strcmp(n->type,"ra") == 0) 
			num_disks = MAX_RA_DISKS;
		else
			num_disks = MAXDISKS;

		for(i=0;i<num_disks;i++)
		{
			sprintf(spec,"%s%d",n->type,i);
			to = 0;
			if(strcmp(n->type,"rz") == 0)
			{
				sprintf(devfile,"r%sa",spec);
				dev = makedev(n->majnum,i*8);
			}
			else if(strcmp(n->type,"ra") == 0) 
			{
				sprintf(devfile,"r%sa",spec);
				dev = makedev(RAMAJOR(i,n->majnum),RAMINOR(i));
			}
			else
			{
				strcpy(devfile,"rmt0h");
				dev = maketape(n->majnum,i);
			}

			mknod(devfile,0020666,dev);
			to = 0;
			to = open(devfile,O_RDONLY | O_NDELAY);
			if(to < 0)
			{
				unlink(devfile);
				continue;
			}
			dev_info.device[0] = '\0';
			if(ioctl(to,DEVIOCGET,(char *)&dev_info) < 0)
			{
				unlink(devfile);
				close(to);
				continue;
			}
			if(strcmp(n->type,"ra") == 0)
				if(strcmp(dev_info.device,DEV_RA60)!= 0)
				{
					unlink(devfile);
					close(to);
					continue;
				}
			if(strcmp(n->type,"rz") == 0)
				if(strncmp(dev_info.device,"RRD",3)!= 0)
				{
					unlink(devfile);
					close(to);
					continue;
				}
			/* 
			 * The following TMSCP tape devices are not supported
			 * for installation media.
			 * RV20, TA90 
		 	 */
			if ((strcmp(dev_info.device,DEV_RV20)==0) ||
				(strcmp(dev_info.device,DEV_TA90)==0))
			{
				unlink(devfile);
				close(to);
				continue;
			}
			strcpy(found[FMAX].name,n->type);
			strcpy(found[FMAX].type,dev_info.device);
			found[FMAX].unit = i;
			found[FMAX].plug = dev_info.slave_num;
			strcpy(found[FMAX].interface,dev_info.interface);
			found[FMAX].link = dev_info.ctlr_num;
			found[FMAX].adpt_num = dev_info.adpt_num;
			FMAX++;
			close(to);
			unlink(devfile);
		}
	}
	return;
}
/***************************************************************************/
getroots()
{

	register int i;
	int retval,times,to,dev;
	char command[25],spec[25],devfile[10];
	struct d *n;
	int num_disks;


	for(n=roots;strcmp(n->type,"\0") != 0;n++)
	{
		/*
		 * There can be more than 32 ra type disks.  Up the limit
		 * of devices to examine in this case.
		 */
		if (strcmp(n->type,"ra") == 0) 
			num_disks = MAX_RA_DISKS;
		else
			num_disks = MAXDISKS;

		for(i=0;i<num_disks;i++)
		{
			sprintf(devfile,"r%s%da",n->type,i);
			if(strcmp(n->type,"ra") == 0) 
			{
				dev = makedev(RAMAJOR(i,n->majnum),RAMINOR(i));
			}
			else 
			{
				dev = makedev(n->majnum,i*8);
			}
			mknod(devfile,0020666,dev);
			to = open(devfile,O_RDONLY | O_NDELAY);
			if(to < 0)
			{
				unlink(devfile);
				continue;
			}
			if((retval=ioctl(to,DEVIOCGET,(char *)&dev_info)) < 0)
			{
				close(to);
				unlink(devfile);
				continue;
			}
			/*
			 * Determine if this device is supported as a
			 * root device.
			 */
			if (supported_root(to, dev_info.device)==SUPPORTED_ROOT)
			{
				strcpy(found[FMAX].name,n->type);
				strcpy(found[FMAX].type,dev_info.device);
				found[FMAX].linktype = dev_info.bus;
				found[FMAX].trlevel = dev_info.nexus_num;
				found[FMAX].unit = i;
				found[FMAX].plug = dev_info.slave_num;
				if(strncmp(dev_info.device,"RM",2) == 0)
					found[FMAX].link = dev_info.bus_num;
				else
					found[FMAX].link = dev_info.ctlr_num;
				found[FMAX].adpt_num = dev_info.adpt_num;
				strcpy(found[FMAX].interface,dev_info.interface);
				found[FMAX].rctlr_num = dev_info.rctlr_num;
				FMAX++;
			}
			close(to);
			unlink(devfile);
		}
	}

}
/***************************************************************************/
getdisks()
{

	register int i;
	int retval,times,to,dev;
	char command[25],spec[25],devfile[10];
	struct d *n;
	int num_disks;


	for(n=disks;strcmp(n->type,"\0") != 0;n++)
	{
		/*
		 * There can be more than 32 ra type disks.  Up the limit
		 * of devices to examine in this case.
		 */
		if (strcmp(n->type,"ra") == 0) 
			num_disks = MAX_RA_DISKS;
		else
			num_disks = MAXDISKS;

		for(i=0;i<num_disks;i++)
		{
			sprintf(devfile,"r%s%da",n->type,i);
			if(strcmp(n->type,"ra") == 0) 
			{
				dev = makedev(RAMAJOR(i,n->majnum),RAMINOR(i));
			}
			else 
			{
				dev = makedev(n->majnum,i*8);
			}
			mknod(devfile,0020666,dev);
			to = open(devfile,O_RDONLY | O_NDELAY);
			if(to < 0)
			{
				unlink(devfile);
				continue;
			}
			if(ioctl(to,DEVIOCGET,(char *)&dev_info) < 0)
			{
				close(to);
				unlink(devfile);
				continue;
			}
			if(strncmp(dev_info.device,"RX",2) != 0
			&& strncmp(dev_info.device,"RRD",3) != 0)
			{
				strcpy(found[FMAX].name,n->type);
				strcpy(found[FMAX].type,dev_info.device);
				found[FMAX].unit = i;
				found[FMAX].plug = dev_info.slave_num;
				strcpy(found[FMAX].interface,dev_info.interface);
				if(strncmp(dev_info.device,"RM",2) == 0)
					found[FMAX].link = dev_info.bus_num;
				else
					found[FMAX].link = dev_info.ctlr_num;
				FMAX++;
			}
			unlink(devfile);
			close(to);

		}
	}

}
/***************************************************************/
getcpu()
{
	int indx,anyint,mem;
#ifdef vax
	mem = open ("/dev/kmem", 0);
	indx = 0;
	indx = lseek(mem,0x80000000,0);
	read(mem,&rpb,sizeof(rpb));
	return(rpb.cpu);
#endif vax
#ifdef mips
	struct save_state installinfo;
	mem = open ("/dev/kmem", 0);
	indx = 0;
	indx = lseek(mem, SST_ADDR, 0);
	read(mem, &installinfo, sizeof(installinfo));
	return(installinfo.cpu);
#endif mips
}
/***************************************************************************/
getwscons()
{
	int n;
	if ((n = open("/dev/console",O_RDONLY | O_NDELAY)) < 0 )
		{
		printf("Can't open /dev/console\n");
		exit(1);
		}
	if(ioctl(n,DEVIOCGET,(char *)&dev_info) < 0)
		{
		printf("devget ioctl failed!\n");
		exit(1);
		}
	close(n);
	if (strcmp(dev_info.device,"VR260") == 0 ||
	    strcmp(dev_info.device,"VR290") == 0 ||
	    strcmp(dev_info.device,"MONO" ) == 0 ||
	    strcmp(dev_info.device,"COLOR") == 0)
		printf("0");
}
/***************************************************************************/
showboot(i)
int i;
{
	int cpu;
	char console_magic[4];
	FILE *fp;
#ifdef vax
	char *syshalt="after the installation software halts the processor: ";
	char *youhalt="after you halt the processor: ";
	char *howhalt="\n\
Wait for the message indicating that the processor can be halted.\n\
To halt the processor,";
	char *step1="^P   	( CTRL/P, to display the console mode prompt )";
	char *step2=">>> H	( H, to halt the processor )";
	char *mic="Insert media labeled 'BOOT 1/1' before entering the boot sequence.";
	char *mictape="Make sure the console TK50 tape is in the drive before entering the boot sequence.";

	fp=fopen("/tmp/showboot","w");
	fprintf(fp,"\nEnter the following boot sequence at the console mode prompt\n");
	cpu = getcpu();
	switch(cpu)
	{
	case MVAX_I:
		fprintf(fp,"%s\n",youhalt);
		fprintf(fp,"\n\t>>> b\n\n");
		fprintf(fp,"\n%s press and release the front panel HALT button.\n", howhalt);
		break;

	case MVAX_II: 
	case VAXSTAR:
		fprintf(fp,"%s\n",syshalt);
		fprintf(fp,"\n\t>>> b dua%x\n\n",found[i].plug);
		break;

	case C_VAXSTAR:
	case VAX_60:
		fprintf(fp,"%s\n",syshalt);
		switch(found[i].linktype)
		{
		case DEV_NB:
			if (strcmp(found[i].interface,"VS_SCSI")==0)
				fprintf(fp,
"\n\t>>> b dk%c%x00\n\n", found[i].link + 'a', found[i].plug);
			else
				fprintf(fp,"\n\t>>> b dua%x\n\n",found[i].plug);
			break;
		default:
			fprintf(fp,"\n\t>>> b dua%x\n\n",found[i].plug);
			break;
		}
		break;

	case VAX_3600:		/* 	Mayfair I	*/
	case VAX_3400:		/* 	Mayfair II	*/
	case VAX_3900:		/* 	Mayfair III	*/
		fprintf(fp,"%s\n",syshalt);
		switch(found[i].linktype)
		{
		case DEV_MSI:
			fprintf(fp,"\n\t>>> b dia%x\n\n",found[i].plug);
			break;
		
		default:
			fprintf(fp,"\n\t>>> b dua%x\n\n",found[i].plug);
			break;
		}
		break;

	case VAX_730:
/* need to fix this when we really know whats going on with 730 bus type */

		fprintf(fp,"%s\n",youhalt);
		fprintf(fp,"\n\t>>> d/g 3 %x\n",found[i].plug);
		if(strcmp(found[i].type,"R80") == 0)
			fprintf(fp,"\n\t>>> @ubaidc.cmd\n\n");
		else
			fprintf(fp,"\n\t>>> @ubara.cmd\n\n");
		fprintf(fp,"%s type:\n%s\n",howhalt,step1);
		break;

	case VAX_750:
		fprintf(fp,"%s\n",youhalt);
		fprintf(fp,"\n%s\n",mic);
		switch(found[i].linktype)
		{
		case DEV_UB:
			fprintf(fp,"\n\t>>> b du%c%x\n\n",
				found[i].link + 'a', found[i].plug);
			break;
		case DEV_MB:
			fprintf(fp,"\n\t>>> b db%c%x\n\n",
				found[i].link + 'a', found[i].plug);
			break;
		case DEV_CI:
			fprintf(fp,"\n\t>>> b/800 dda0\n\n");
			fprintf(fp,"\tboot58> d/g 2 %x\n",found[i].rctlr_num);
			fprintf(fp,"\tboot58> d/g 3 %x\n",found[i].plug);
			fprintf(fp,"\tboot58> @cira.cmd\n");
			break;
		}
		fprintf(fp,"%s type:\n%s\n",howhalt,step1);
		break;

	case VAX_780:
		fprintf(fp,"%s\n",youhalt);
		fprintf(fp,"\n%s\n",mic);
		/* only 1 SBI bus for vax780 */
		if (found[i].linktype == DEV_CI)
			fprintf(fp,"\n\t>>> d r2 %x\n", found[i].rctlr_num); 
		else
			fprintf(fp,"\n\t>>> d r1 %x\n",found[i].trlevel);
		fprintf(fp,"\t>>> d r3 %x\n",found[i].plug); /* unit */
		switch (found[i].linktype)
		{
		case DEV_CI:
			fprintf(fp,"\t>>> @cira.cmd\n\n");
			break;
		case DEV_UB:
			fprintf(fp,"\t>>> @ubara.cmd\n\n");
			break;
		case DEV_MB:
			fprintf(fp,"\t>>> @mbahp.cmd\n\n");
			break;
		}
		fprintf(fp,"%s type:\n%s\n%s\n",howhalt,step1,step2);
		break;
		
	case VAX_8200:
		fprintf(fp,"%s\n",youhalt);
		switch (found[i].linktype)
		{
		case DEV_UB:
		case DEV_BI:
			if (strcmp(found[i].interface,"KFBTA")==0)
				fprintf(fp,
"\n\t>>> b du%x%x\n\n",found[i].trlevel,found[i].plug % 5);
				/* AIO interface */
			else
			{
				fprintf(fp,"\n%s\n",mic);
				fprintf(fp,
"\n\t>>> b du%x%x\n\n",found[i].trlevel,found[i].plug);
			}
			break;
		case DEV_CI:
		case DEV_BICI:
			fprintf(fp,"\n%s\n",mic);
			fprintf(fp,"\n\t>>> b/r5:800 csa1\n\n");
			fprintf(fp,"\tboot58> d/g 1 %x\n",found[i].trlevel);
			fprintf(fp,"\tboot58> d/g 2 %x\n",found[i].link);
			fprintf(fp,"\tboot58> d/g 3 %x\n",found[i].plug);
			fprintf(fp,"\tboot58> @cira.cmd\n");
			break;
		}
		fprintf(fp,"%s type:\n%s\n",howhalt,step1);
		break;

	case VAX_8600:
		fprintf(fp,"%s\n",youhalt);
		fprintf(fp,"\n\t>>> d r1 %x%x\n",found[i].adpt_num,found[i].trlevel);
		if (found[i].linktype == DEV_CI)
			fprintf(fp,"\n\t>>> d r2 %x\n", found[i].rctlr_num); 
		fprintf(fp,"\t>>> d r3 %x\n",found[i].plug);
		switch (found[i].linktype)
		{
		case DEV_UB:
			fprintf(fp,"\t>>> @ubara.com\n\n");
			break;
		case DEV_MB:
			fprintf(fp,"\t>>> @mbahp.com\n\n");
			break;
		case DEV_CI:
			fprintf(fp,"\t>>> @cira.com\n\n");
			break;
		}
		fprintf(fp,"%s type:\n%s\n%s\n",howhalt,step1,step2);
		break;

	case VAX_8800: 
		fprintf(fp,"%s\n",youhalt);
		fprintf(fp,"\n\t>>> d r1 %x%x\n",found[i].adpt_num,found[i].trlevel);
		if (found[i].linktype == DEV_CI || 
		    found[i].linktype == DEV_BICI )
			fprintf(fp,"\n\t>>> d r2 %x\n", found[i].rctlr_num); 
		fprintf(fp,"\t>>> d r3 %x\n",found[i].plug);
		switch (found[i].linktype)
		{
		case DEV_UB:
			fprintf(fp,"\t>>> @ubara.com\n\n");
			break;
		case DEV_BI:
			fprintf(fp,"\t>>> @bdara.com\n\n");
			break;
		case DEV_CI:
		case DEV_BICI:
			fprintf(fp,"\t>>> @bcira.com\n\n");
			break;
		}
		fprintf(fp,"%s type:\n%s\n%s\n",howhalt,step1,step2);
		break;

	case VAX_8820:
		fprintf(fp,"%s\n",youhalt);
		fprintf(fp,"\n\t>>> d r1 %x%x\n",found[i].adpt_num,found[i].trlevel);
		if (found[i].linktype == DEV_CI ||
		    found[i].linktype == DEV_BICI)
			fprintf(fp,"\n\t>>> d r2 %x\n", found[i].rctlr_num); 
		fprintf(fp,"\t>>> d r3 %x\n",found[i].plug);
		switch (found[i].linktype)
		{
		case DEV_BI:
			fprintf(fp,"\t>>> @bdara.cmd\n\n");
			break;
		case DEV_CI:
		case DEV_BICI:
			fprintf(fp,"\t>>> @bcara.cmd\n\n");
			break;
		}
		fprintf(fp,"%s type:\n%s\n%s\n",howhalt,step1,step2);
		break;

	case VAX_6200:
	case VAX_6400:
		fprintf(fp,"%s\n",syshalt);
		fprintf(fp,"\n%s\n",mictape);
		switch(found[i].linktype)
		{
		case DEV_BI:
			fprintf(fp,"\n\t>>> b /xmi:%x /bi:%x du%d\n",
				found[i].adpt_num, found[i].trlevel,
				found[i].plug);
			break;
		case DEV_BICI:
			fprintf(fp,"\n\t>>> b /xmi:%x /bi:%x /node:%x /r5:10008 du%d\n",
				found[i].adpt_num, found[i].trlevel,
				found[i].rctlr_num, found[i].plug);
			break;
		case DEV_XMICI:
			fprintf(fp,"\n\t>>> b /xmi:%x /node:%x /r5:10008 du%d\n",
				found[i].trlevel, found[i].rctlr_num,
				found[i].plug);
			break;
		case DEV_XMI:
			fprintf(fp,"\n\t>>> b /xmi:%x du%d\n",
				found[i].trlevel, found[i].plug);
			break;
		}
		break;

	case VAX_9000:
		fprintf(fp,"%s\n",syshalt);
		fprintf(fp,"\n\t>>> i/k");
		switch(found[i].linktype)
		{
		case DEV_BI:
			fprintf(fp,"\n\t>>> b /xmi:%x /bi:%x /r5:10008 kdb%x\n",
				found[i].adpt_num, found[i].trlevel,
				found[i].plug);
			break;
		case DEV_BICI:
			fprintf(fp,"\n\t>>> b /xmi:%x /bi:%x /node:%x /r5:10008 kdb%x\n",
				found[i].adpt_num, found[i].trlevel,
				found[i].rctlr_num, found[i].plug);
			break;
		case DEV_XMICI:
			fprintf(fp,"\n\t>>> b /xmi:%x /node:%x /r5:10008 ci%x\n",
				found[i].trlevel, found[i].rctlr_num,
				found[i].plug);
			break;
		case DEV_XMI:
			fprintf(fp,"\n\t>>> b /xmi:%x /r5:10008 kdm%x\n",
				found[i].trlevel, found[i].plug);
			break;
		}
		break;

	default:
		fprintf(fp,"\n**** Unsupported processor ****\n");
		break;
	}
	fclose(fp);
	return;
#endif vax

#ifdef mips
	int	wflag = 1;
	char	DISKTYPE[3];
	char	BOOTENVI[20];
	char	BOOTPATH[40];
	char	BOOTCOMM[20];

	fp=fopen("/tmp/showboot","w");
	cpu = getcpu();
	BOOTPATH[0] = NULL;
	switch(cpu)
        {
	case DS_5800:
		strcpy(DISKTYPE, "ra");
		strcpy(BOOTENVI, "setenv bootpath");
		strcpy(BOOTCOMM, "boot");
                switch(found[i].linktype)
                {
		case DEV_BI:
			sprintf(BOOTPATH, "%s(/x%d/b%d,%d,0)vmunix", 
 				DISKTYPE, found[i].adpt_num, 
				found[i].trlevel, found[i].plug);
			break;

		case DEV_BICI:
			sprintf(BOOTPATH, "%s(/x%d/b%d/c%d,%d,0)vmunix", 
 				DISKTYPE, found[i].adpt_num,
				found[i].trlevel, found[i].rctlr_num,
				found[i].plug);
			break;

		case DEV_XMI:
			sprintf(BOOTPATH, "%s(/x%d,%d,0)vmunix", 
 				DISKTYPE,found[i].trlevel,found[i].plug);
			break;
		}
		break;

	case DS_5000:
	case DS_5000_100:
		strcpy(BOOTCOMM, "boot");

		switch(found[i].linktype)
		{
		case DEV_SCSI:
			strcpy(DISKTYPE, "rz");
			break;

		case DEV_MSI:
			strcpy(DISKTYPE, "rf");
			break;

		default:
			strcpy(DISKTYPE, "ra");
			break;
		}

		/*
		 * CONSOLE DEBUG FOR DS_5000 ROMS
		 * found[i].adpt_num |= 0x8000; 
		*/


		getsysinfo(GSI_CONSTYPE,console_magic,sizeof(console_magic));
		if ((cpu==DS_5000) && (strncmp(console_magic,"TCF0",4)))
			{
			strcpy(BOOTENVI, "setenv bootpath");
		  	sprintf(BOOTPATH, "%s(%d,%d,0)vmunix",
				DISKTYPE, found[i].link,found[i].plug);
			}
		else
			{
			strcpy(BOOTENVI, "setenv boot");
		  	sprintf(BOOTPATH, "\"%d/%s%d/vmunix -a\"",
			  	found[i].adpt_num,
			  	DISKTYPE,found[i].plug);
			}
		break;


	case DS_3100:
	case DS_5100:
        case DS_5400:
	case DS_5500:
		strcpy(BOOTENVI, "setenv bootpath");

		if (cpu == DS_3100)
			strcpy(BOOTCOMM, "auto");
		else
			strcpy(BOOTCOMM, "boot");

		switch(found[i].linktype)
		{
		case DEV_SCSI:
			strcpy(DISKTYPE, "rz");
			break;

		case DEV_MSI:
			strcpy(DISKTYPE, "rf");
			break;

		default:
			strcpy(DISKTYPE, "ra");
			break;
		}

		sprintf(BOOTPATH, "%s(%d,%d,0)vmunix",
			DISKTYPE, found[i].link,found[i].plug);

		break;

	default:
		wflag = 0;
		fprintf(fp,"\n**** Unsupported processor ****\n");
		break;
	}

	if (wflag)
	{
	fprintf(fp,"\nIssue the following console commands to set your default bootpath variable\n");
	fprintf(fp,"and to boot your system disk:\n\n");

	fprintf(fp,"\t>> %s %s\n", BOOTENVI,BOOTPATH);
	fprintf(fp,"\t>> %s\n\n", BOOTCOMM);
	}

	fclose(fp);
#endif mips
}
/*****************************************************************************/
choicechk(choice)	/* check "ans" to see if it consists of all digits */
        char choice[8];
{
        int i;

        for (i=0; choice[i] != NULL; i++)
                if (isdigit(choice[i]) == 0)
                        {
                        return(0);
                        break;
                        }
        return(1);
}
/*****************************************************************************
 *
 * supported_root()
 *
 * To determine if the device is supported first look through a list
 * of devices which are explicitly not supported.  If the device is not on
 * that list then check to see if the disk has enough capacity.  Sufficient
 * capacity here is defined as at least the size of the default "a" partition
 * (32768) plus the size of the default "b" partition (50160).
 */
int
supported_root(fd, dv_name)
	int fd;		/* Device file descriptor        */
	char *dv_name;	/* String containing device name */
{
	char *device_name;
	DEVGEOMST devgeom;
	int i;

	for (i = 0; *unsupported_roots[i]; i++) {
		device_name = unsupported_roots[i];
		if ((strncmp(device_name, dv_name, strlen(device_name))) == 0) {
			return(UNSUPPORTED_ROOT);
		}
	}

	if (ioctl(fd, DEVGETGEOM, (char *)&devgeom) < 0) {
		/*
		 * In the event of ioctl failure do not conclude that the
		 * disk is too small.
		 */
		return(SUPPORTED_ROOT);
	}
	if (devgeom.geom_info.dev_size < MIN_CAPACITY) {
		return(UNSUPPORTED_ROOT);
	}
	else {
		return(SUPPORTED_ROOT);
	}
}

