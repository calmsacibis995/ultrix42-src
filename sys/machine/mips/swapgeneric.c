/*
 *	@(#)swapgeneric.c	4.3	(ULTRIX)	3/7/91
 *
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 */

/************************************************************************
 *
 *	Modification History: swapgeneric.c
 *
 * 05-Mar-1991 -- Szczypek
 *	Altered TURBOchannel console support to look for boot path 
 *	rather than assume that it's always the second argument.
 *
 * 16-Sep-1990 -- Szczypek
 *	Added TURBOchannel console support.
 *
 * 29-May-1990 -- Robin
 *	Moved gets() to a new file in machine/common so that it
 *	can be used by diskless kernels.
 *
 * 12-Dec-1989 -- burns
 *	fixed to handle booting from multiple controllers for SCSI
 *	on DECsystem 5000.
 *
 * 02-Oct-1989 -- burns
 *	fixed to handle unit numbers greater than 7 for systems other than
 *	DECstation/DECsystem 3100s.
 *
 * 12-Jun-1989 -- gg
 *	Removed variables dmmin, dmmax and dmtext.	
 *
 *************************************************************************/

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/systm.h"
#include "../h/reboot.h"
#include "../../machine/common/cpuconf.h"
#include "../io/tc/tc.h"

/*
 * Generic configuration;  all in one
 */
dev_t	rootdev = NODEV;
dev_t	dumpdev = NODEV;
int	nswap;
struct	swdevt swdevt[] = {
	{ -1,	1,	0 },
	{ 0,	0,	0 },
};
long	dumplo;

/*
 * the boot_data struct is used by the parser routine for mips based
 * systems to place the boot string information.
 */
struct boot_data {
	int	node;
	int	controller;
	int	unit;
	int	partition;
	char	name[128];
};

extern int askme;			 /* set by getargs */
extern struct genericconf genericconf[]; /* set in conf.c */
extern int	cpu;			 /* needed for DS5800 */
extern struct 	tc_slot tc_slot[8];	/* contains TURBOchannel info */
extern int	rex_base;		/* base address of callback table */
extern char	**ub_argv;		/* input parameters */
extern int	ub_argc;		/* input parameter count */

/*
 * This routine has been upgraded to handle disk unit numbers > 7 for
 * systems other than DECstation/DECsystem 3100s. It realy needs a re-write.
 *
 * Now we can handle multiple scsi controllers. It still needs a re-write.
 */
setconf()
{
	register struct genericconf *gc;
	int i, j, major_offset, swaponroot; 
	extern dev_t swapdev;
	char *name_hold = "  ";
	char *cp;
	char bootstr[80];
	int ctlr, bbflag;

	struct boot_data boot_data;

	major_offset = swaponroot = 0;

	if(rex_base) {
		bbflag=0;
		/*
		 * Look for boot path.  Scan arguments looking for one where
		 * second char is a '\'.  If found, this is the boot path.
		 * It is assumed that single digit slot numbers are used,
	  	 * hence the skipping over of the first char.  When found,
		 * set pointer to argument.  Note that no checking is done
	   	 * if the argument has a '-' as its first char since this
	 	 * indicates some sort of switch, or if a NULL is found. 
		 */
		for(i=1;i<ub_argc;i++) {
			if(ub_argv[i][0] != '-' && ub_argv[i][0] != NULL)
				if(ub_argv[i][1] == '/') {
					cp = ub_argv[i];
					break;
				}
		}
		if(strncmp(cp+1,"/rz",3)==0) {
			for(i=0;i<=8;i++) {
				if((strcmp(tc_slot[i].devname,"asc")==0) &&
				((*cp - '0')==tc_slot[i].slot)) {
					ctlr = tc_slot[i].unit;
				}
			}
		}
		i=0;
		for(i=0;i<2;i++)
			bootstr[i] = *(cp+2+i);  
		bootstr[i++]='(';
		bootstr[i++]= ctlr+'0';
		bootstr[i++]=',';
		bootstr[i++]= *(cp+4);
		bootstr[i++]=',';
		for(j=1;j<ub_argc;j++)
			if(ub_argv[j][0]=='-') {
				cp = &ub_argv[j][1];
				while ((*cp != NULL) && (bbflag != 1)) {
				  switch (*cp++) {
				  case 'b':
				        j++;
					while(ub_argv[j][0] == NULL) {
					  j++;
					  if(j >= ub_argc) {
					    printf("bad format\n");
					    rex_rex('h');
					  }
					}
					strcpy(&bootstr[i],ub_argv[j]);
					i=i+strlen(ub_argv[j]);
					j=ub_argc;
					bbflag=1;
					break;
				  }
				}
			}
		if(!bbflag)
			bootstr[i++] = '0';
		bootstr[i++]=')';
		bootstr[i]='\0';
		cp = bootstr;	

	}

	if (rootdev != NODEV)
		goto doswap;

	if (askme || rootdev == NODEV) {
		boot_data.node = 0;
		boot_data.controller = 0;
		boot_data.unit = 0;
		boot_data.partition = 0;
		boot_data.name[3] = NULL;
		boot_data.name[4] = NULL;
		boot_data.name[5] = NULL;
		boot_data.name[6] = NULL;
#if SWAPTYPE == 2
		if(rex_base) {
			if(parser(cp, &boot_data)) {
				goto swap_on_boot;
			}		   
		}
		else {
			if(parser((char *)prom_getenv("boot"), &boot_data)) {
				goto swap_on_boot;
			}		   
		}
#endif

retry:
		boot_data.unit = 0;
		boot_data.partition = 0;
		printf("root device? ");
		gets(boot_data.name);
swap_on_boot:
		for (gc = genericconf; gc->gc_name; gc++)
			if (gc->gc_name[0] == boot_data.name[0] &&
			    gc->gc_name[1] == boot_data.name[1])
				goto gotit;
		goto bad;
gotit:
		if (strlen(boot_data.name) &&
		    boot_data.name[strlen(boot_data.name)-1] == '*') {
                        boot_data.name[strlen(boot_data.name)-1] = '\0';
                        swaponroot++;
                }
		boot_data.unit = 0;
		i = 2;
		if ((boot_data.name[i] >= '0') &&
		    (boot_data.name[i] <= '9')) {
			while (boot_data.name[i] != '\0') {
				boot_data.unit = ((boot_data.unit * 10) +
					(boot_data.name[i] - '0'));
				i++;
			}
			goto found;
		}
		printf("bad/missing unit number\n");
bad:
		printf("bad root specification, use one of:");
		for (gc = genericconf; gc->gc_name; gc++) {
			/*
		 	 * Don't print ra twice !!
		 	 */
			if (strcmp(name_hold, gc->gc_name) != 0)
				printf("%s%%d ", gc->gc_name);
			name_hold = gc->gc_name;
		}
		printf("\n");
		goto retry;
	}
	boot_data.unit = 0;
found:
	major_offset = (boot_data.unit / 32);
	boot_data.unit = (boot_data.unit % 32);
	gc->gc_root = makedev((major(gc->gc_root) + major_offset),
		      (boot_data.unit * 8) + boot_data.partition);
	rootdev = gc->gc_root;
doswap:
	swdevt[0].sw_dev = dumpdev =
	    makedev(major(rootdev), minor(rootdev)+1);
	/* swap size and dumplo set during autoconfigure */
	if (swaponroot) {
		rootdev = dumpdev;
	}
}


#define	AT_CONTROLLER	0
#define	AT_UNIT		1
#define	AT_PARTITION	2

parser(str, boot_data)
char *str;
struct boot_data *boot_data;
{
	int ret = 0, i;
	char *frn, *bck, *cp, *index();
	int where_are_we, unit, mod;

	/*
	 * Check the sanity of the boot string
	 */
	if(str == NULL)
		return(0);
	if(index(str, '(') && index(str, ')') && (index(str, '(') < index(str,')')))
		ret = 1;
	/*
	 * Put the two character device name into the boot_data struct
	 */
	if(ret && strncmp(str, "rz(", 3) == 0) {
		bcopy("rz", &boot_data->name[0], 3);
		ret = 1;
	}
	if(ret && strncmp(str, "ra(", 3) == 0) {
		bcopy("ra", &boot_data->name[0], 3);
		ret = 1;
	}
	if(ret && strncmp(str, "du(", 3) == 0) {	/* for DS5800 */
		bcopy("ra", &boot_data->name[0], 3);
		ret = 1;
	}
	if(ret && strncmp(str, "rf(", 3) == 0) {
		/* RF drives on DSSI are MSCP devices and are therefore
		 * ra devices to ULTRIX.
		 */
		bcopy("ra", &boot_data->name[0], 3);
		ret = 1;
	}

	/*
	 * Position to the 'meat' of the boot string
	 */
	if(ret == 1) {
		char *subval;
		frn = index(str,'(');
		bck = index(str,')');

		if(frn + 1 == bck) {
			boot_data->name[2] = '0';
			return(ret);
		}
		subval = frn + 1;
		*bck = '\0';
			

		/*
		 * Convert alpha partition number to numeric
		 */
		for (i = 0; i < strlen(subval); i++) {
			if (subval[i] >= 'A' && subval[i] <= 'H')
				subval[i] = subval[i] - 'A' + '0';
			if (subval[i] >= 'a' && subval[i] <= 'h')
				subval[i] = subval[i] - 'a' + '0';
		}

		/*
		 * Parse the 'controller,unit,partition'. We begin at the first character
		 * after the open paren.
		 */
		where_are_we = AT_CONTROLLER;
		while (*subval != '\0') {
			/*
			 * Here we decide if we need to process the 'controller'
			 * section of the boot string. Basically, it's ignored
			 * for all systems but those with multiple scsi controllers.
			 * So if we see that we are not dealing with an 'rz' type disk,
			 * we just skip over the controller section to the unit.
			 */
			if ((where_are_we == AT_CONTROLLER) && (boot_data->name[1] != 'z')) {
				while((*subval != ',') && (*subval != '\0'))
					subval++;
				if (*subval == ',') {
					where_are_we++;
					subval++;
					continue;
				}
				if (*subval == '\0')
					return(ret);
			}
			if (*subval == ',') {
				where_are_we++;
				subval++;
				continue;
			}
			if (*subval <= '0' && *subval >= '9') {
				ret = 0;
				return(ret);
			}
			switch (where_are_we) {
			      case AT_CONTROLLER:
				boot_data->controller =
					((boot_data->controller * 10) + (*subval - '0'));
				break;
			      case AT_UNIT:
				boot_data->unit =
					((boot_data->unit * 10) + (*subval - '0'));	
				break;
			      case AT_PARTITION:
				boot_data->partition =
					((boot_data->partition * 10) + (*subval - '0'));
				break;
			      default:
				ret = 0;
				return(ret);
			}
			subval++;
		}
		/*
		 * Now finish building up the device name
		 */
		unit = (boot_data->controller * 8) + boot_data->unit;
		i = 2;
		if (mod = (unit / 100)) {
			boot_data->name[i++] = ('0' + mod);
			unit -= (mod * 100);
		}
		if (mod = (unit / 10)) {
			boot_data->name[i++] = ('0' + mod);
			unit -= (mod * 10);
		}
	        boot_data->name[i] = ('0' + unit);
	}
	return(ret);
}
	
