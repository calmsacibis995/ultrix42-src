/*
 * ultrixload.c
 */
#ifndef lint
static char *sccsid = "@(#)ultrixload.c	4.1	ULTRIX	7/2/90";
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
#include "vax/vmb.h"
#include "../h/param.h"
#include <a.out.h>

extern edata;
extern entry;
int bn;

struct desc {
    union {
	char pad[512];
	struct {
   	   struct exec x;		/* a.out image header */
	   int nblks;			/* num of 512 byte blocks on medium */
	   int vmbblks;			/* size of VMB in 512 byte blocks */
	   int compressed;		/* 1 = compressed, 0 = not compressed */
	} d
    } un
};

struct desc desc;
int	volume = 1;
char	*medium;
extern	mode;
extern	struct	vmb_info *vmbinfo;

/*
 * Functional Discription:
 *	This is the main of the `ultrixload' image.  It loads either
 *	vmb.exe or a standalone kernel from a non-file structured device.
 *	The layout of the boot device is:
 *	
 *		ultrixload	(variable # of 512 byte blocks)
 *		tape mark	(only if TK50 tape)
 *		descriptor blk	(one 512 byte block - see struct desc above)
 *		vmb.exe		(variable # of 512 byte blocks - optional)
 *		vmunix		(variable # of 512 byte blocks - compressed
 *				 if not a TK50)
 *		
 *		Note: vmunix can span multiple media
 *		
 * Inputs:
 *	R11 and R10 are preserved
 *
 * Outputs:
 *	stop on error
 *	otherwise, start loaded image
 *
 */
main () {
	register        howto, devtype;		/* howto=r11, devtype=r10 */
	register int    i, resid;
	char   *addr;
	int	(*READ)();
	extern	int decompress();

	/*
 	 * calculate the lbn of the descriptor block
 	 */
	bn=(((((int)&edata - (int)&entry) | 0x1ff) + 1) + 512)/512;
	desc.un.d.nblks = 1000000; /* make sure early reads don't fail */
	if (mode & TK50_BOOT)
		if (skip(bn+1)) {		/* skip loader size + TM */
			printf("tape position lost\n");
			stop();
		}
	/*
	 * Read the descriptor
	 */
	if (read((char *)&desc, sizeof desc) != sizeof desc) {
		printf("Error reading descriptor block\n");
		stop(); 
	}
	/*
	 * If we got here via system ROM then load VMB and reboot
	 */
	if (mode & ROM_BOOT) {
		/*
		 * If theres no VMB, tell the user and stop
		 */
		if (desc.un.d.vmbblks == 0) {
			printf("There is no VMB on this medium.  Use BOOT 1/1.\n");
			stop();
		}
		if (read((char*) 0x200, desc.un.d.vmbblks*512) == desc.un.d.vmbblks*512) {
			start_vmb();
			stop();			/* should never return */
		} else {
			printf("vmb.exe load failed\n");
			stop();
		}
	}
	bn += desc.un.d.vmbblks;
	if (mode & TK50_BOOT) {
		if (skip(desc.un.d.vmbblks)) {
			printf("tape position lost\n");
			stop();
		}
	}
	if (desc.un.d.compressed)
		READ = decompress;
	else 
		READ = read;
	printf("\nUltrixload (using VMB version %d)\n\n", vmbinfo->vmbvers);
	switch (desc.un.d.nblks) {
	case 800:
		medium="RX50";
		break;
	case 512:
		medium="TU58";
		break;
	case 494:
		medium="RX01";
		break;
	default:
		medium="VOL";
		break;
	}
	if (desc.un.d.x.a_magic != 0407 && desc.un.d.x.a_magic != 0413 && desc.un.d.x.a_magic != 0410) {
		printf("Bad magic number in image header.\n");
		stop();
	}
	printf("Sizes:\n");
	printf("text = %d\n", desc.un.d.x.a_text);
	printf("data = %d\n", desc.un.d.x.a_data);
	printf("bss  = %d\n", desc.un.d.x.a_bss);
	if (desc.un.d.x.a_magic == 0407) {
		i = desc.un.d.x.a_text + desc.un.d.x.a_data; 
		addr = (char *) desc.un.d.x.a_text;
	} else {
		i = (desc.un.d.x.a_text | 0x3ff) + 1;
		resid = i - desc.un.d.x.a_text;
		addr = (char *) (i + resid);
	}
	if ((*READ)((char *) 0, i) != i)
		goto shread;
	if (desc.un.d.x.a_magic != 0407) {
		bcopy(desc.un.d.x.a_text,i,resid);
		desc.un.d.x.a_data -= resid;
		if ((*READ)(addr, desc.un.d.x.a_data) != desc.un.d.x.a_data)
			goto shread;
	}
	addr += desc.un.d.x.a_data;
	desc.un.d.x.a_bss += 128 * 512;			/* slop */
	for (i = 0; i < desc.un.d.x.a_bss; i++)
		*addr++ = 0;
	if (mode & TK50_BOOT) {
		printf("\nRewinding tape ...\n");
		qio(PHYSMODE,IO$_REWIND,0,0,0);
	}
	desc.un.d.x.a_entry &= 0x7fffffff;
	printf ("Starting at 0x%x\n\n", desc.un.d.x.a_entry);
	(*((int (*) ()) desc.un.d.x.a_entry)) (vmbinfo);
	stop();
shread: 
	printf("short read\n");
	stop();
}

char	tryagain[]="Check the load device.  Hit `Return' to try again.";
char	response[1];


/*
 * Functional Discription:
 *	This routine handles reading from the booted device.  It breaks
 *	the byte count down into 512 byte block chunks.  For tape, EOF
 *	ends the read.
 *
 * Inputs:
 *	address to receive data
 *	byte count which MUST BE a multiple of 512.
 *
 * Outputs:
 *	the requested byte count
 *	otherwise, the requested byte count
 *
 */
read(addr,bc)
	register *addr, bc;
{
	register howto, devtype;		/* howto=r11, devtype=r10 */
	int qio_status, bcr, block_no;

	bcr = 0;
	while (bcr < bc) {
		if (mode & TK50_BOOT)
			block_no = -1;		/* read next block */
		else
			block_no = bn;
		qio_status = qio(PHYSMODE,IO$_READLBLK,block_no,512,(int)addr+bcr);
		if ((mode & TK50_BOOT) && (qio_status == SS$_ENDOFFILE))
			break;
		if (qio_status & 1) {
			bcr += 512;
			bn++;
			if (bn >= desc.un.d.nblks) {
				volume++;
				printf("\nLoad the %s labeled `STANDALONE ULTRIX %s #%d'.\nPress the RETURN key when ready.", medium, medium, volume);
				gets(response);
				if ((init_drive() & 1) == 0)
					return (-1);
				bn = 0;
			}
		} else {
			printf("Read error: bn = %d, %s\n",
				bn, geterr(qio_status));
			if (bn == 0) {
				printf("%s\n", tryagain);
				gets(response);
				if ((init_drive() & 1) == 0)
					return (-1);
			} else
				return (-1);
		}
	}
	return(bc);
}

/*
 * Functional Discription:
 *	This routine acts as an interface to the drvinit routine.  It
 *	communicates with the user and waits for a response.  Inits are
 *	are atempted up to `retries' times to forgive the user if he 
 *	puts a floppy in upside down or worse.
 *
 * Inputs:
 *	none
 *
 * Outputs:
 *	status of call to drvinit
 *
 */
init_drive()
{
	register howto,devtype;
	int qio_status, retries;

	retries = 5;
	while (retries--) {
		qio_status = 0;
		while ((qio_status & 1) == 0) {
			qio_status = drvinit();
			if ((qio_status & 1) == 0) {
				printf("Drive init error: %s\n%s",
					geterr(qio_status), tryagain);
				gets(response);
			}
			else
				return (qio_status);
		}
	}	
	printf("Retries exceeded\n");
	return (qio_status);
}


/*
 * Functional Discription:
 *	This routine provides a skiprecord function for the TK50.
 *	It reads the specified number of records or until EOF, which ever
 *	comes first.
 *
 * Inputs:
 *	number of records to skip 
 *
 * Outputs:
 *	1 = tape position lost
 *	0 = success
 *
 */
skip(records)
int records;
{
	for (; records > 0; records--)
		if ((read((char *)0, 512)) != 512)
			return(1);
	return(0);
}
