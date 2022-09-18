#ifndef lint
static	char	*sccsid = "@(#)makpkt.c	4.1	(ULTRIX) 	7/2/90";
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

/* 12-10-87  lea  exip() was dropping last numb at end of string. fixed.  */
/*                processes a string ending with CR then null, correctly. */

#include <stdio.h>
#include <ctype.h>
#include <sas/mop.h>
main(argc,argv)
char	*argv[];
int	argc;
{
struct	netblk nblk;
int	k,i,j,c,n=0;

nblk.srvipadr=exip(argv[1]);
nblk.cliipadr=exip(argv[3]);
nblk.brdcst=exip(argv[4]);
nblk.netmsk=exip(argv[5]);
printf("0x%lx,\n",nblk.srvipadr);
printf("\"%s\",\n",argv[2]);
printf("0x%lx,\n",nblk.cliipadr);
printf("0x%lx,\n",nblk.brdcst);
printf("0x%lx,\n",nblk.netmsk);
exit(0);
}
/************************************************************************/
/*									*/
/*	exip - extract ip address into a long				*/
/************************************************************************/
exip(str)
	char *str;
{
int i=0,j=0,flg=0;
char numb[4];
char temp;
unsigned long	retval=0;
while(str[i] != '\0')
	{
	temp=str[i+1];
	str[i+1]='\0';
	if(isdigit(str[i]))
		{
		if(flg==0)
			{
			flg=1;
			j=0;
			retval <<= 8;
			numb[j++]=str[i];
			}
		else	numb[j++]=str[i];
		}
	else	if(flg==1)
			{
			flg=0;
			numb[j]='\0';
			retval+= atoi(numb);
			}

	i++;
	str[i]=temp;
	if (temp == '\0' && flg==1 )    /* at end of str[], save last numb */
		{
			numb[j]='\0';
			retval+= atoi(numb);
		}
	}
return(retval);
}
