/*
 * io.c
 */

#ifndef lint
static char *sccsid = "@(#)io.c	4.2	ULTRIX	10/9/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
 * Revision History:							*
 * Oct 09, 1990 - Joe Szczypek						*
 *	Added TURBOchannel support					*
 ************************************************************************/
#include "../h/param.h"
#include "../h/gnode_common.h"
#include "../fs/ufs/ufs_inode.h"
#include "../h/gnode.h"
/*
#include "../h/dir.h"
*/
#include "saio.h"
#ifdef vax
#include "vax/vmb.h"
#include "../machine/vax/rpb.h"
#endif vax

#ifdef notdef
	if (newunit != -1) {
		oldunit = ((struct rpb *)(vmbinfo->rpbbas))->unit;
		if (oldunit != newunit) {
			status = drvinit(newunit);
			if ((status & 1) == 0) {
				printf("Init error: %s\n", geterr(status));
				drvinit(oldunit);	/* switch back */
				return (-1);
			}
		}
	}
}

#endif notdef

int	errno;
extern	mode;
extern	prom_io;

#ifdef mips
extern  int rex_base;
extern  int ub_argc;
extern  char **ub_argv;
#endif mips

devread(io)
        register struct iob *io;
{
#ifdef vax
	int qio_status;
	int bc, i;

	if (mode & ROM_BOOT)
		bc=512;			/* ROM driver can only read 512 */
	else
		bc=io->i_cc;

	for (i = 0; i*bc < io->i_cc; i++) {
	    qio_status = 
		qio(PHYSMODE, IO$_READLBLK, io->i_bn+i, bc, (int)io->i_ma+(i*512));
	    if ((qio_status & 1) == 0)
			break;
	}
	if (qio_status & 1) {
		errno = 0;
        	return (io->i_cc);
	} else {
#ifdef SECONDARY
		printf("Read error: bn = %d, %s\n",
			io->i_bn, geterr(qio_status));
#endif SECONDARY
		io->i_error = EIO;
		return (-1);
	}
#endif vax
#ifdef mips
	int i;
	char *cp;

	if (rex_base) {
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
				   if(ub_argv[i][0] == '0') { 
				     i = ub_argc;
				     break;
				   } else {
				     io->i_bn += dctob(ub_argv[i]);
				     i = ub_argc;
				     break;
				   }
				 }
			}
	        }
		return(rex_bootread(io->i_bn, io->i_ma, io->i_cc));
	}
	else {
		_prom_lseek(prom_io, io->i_bn*512, 0);
		return (_prom_read(prom_io, io->i_ma, io->i_cc));
	}
#endif mips
}

/*
 * Functional Discription:
 * 	Convert an ASCII string of decimal characters to a binary number.
 *
 * Inputs:
 *	pointer to string
 *
 * Outputs:
 *	value (int)
 *
 */

#ifdef mips
dctob(str)
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
			hexnum = hexnum * 10 + (*str - 48);
/*		else if (*str >= 'a' && *str <= 'f')
			hexnum = hexnum * 10 + (*str - 87);
		else if (*str >= 'A' && *str <= 'F')
			hexnum = hexnum * 10 + (*str - 55);
*/
	}
	return (hexnum);
}
#endif mips

