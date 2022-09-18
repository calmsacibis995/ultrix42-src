#ifndef lint
static	char	*sccsid = "@(#)getboot.c	4.2  (ULTRIX)        7/17/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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


/************************************************************************
 *
 * Name: getboot.c
 *
 * Modification History
 * 
 * July 05, 1990 - Pete Keilty
 *	Made changes for booting on 6xxx systems from XMI controllers.
 * 	If rpb.r1 low nibble is zero booting from XMI else BI.
 *
 * July 14, 1989 - Alan Frechette
 *	Added boot support for RIGEL (VAX_6400).
 *
 * May 02, 1989 - Alan Frechette
 *	Changes to deal with new unique "cpu" handling for both
 *	vax and mips architectures.
 *
 * Feb 12, 1989 - Alan Frechette
 *	New sizer code which supports multiple architectures.
 *	Based on the original V3.0 sizer by Tungning Cherng.
 *	Added in boot support for the VAX_60 and C_VAXSTAR.
 *
 ***********************************************************************/

#include <ctype.h>
#include "sizer.h"

/****************************************************************
*    getboot							*
*								*
*    Get the boot stuff for the system disk.			*
****************************************************************/
getboot ()
{
#ifdef vax
	long offset;
	int cpusub, cpusubsub;
	char tctlr, line[80],cmdfile[50],askfile[50],bootfile[50];
	FILE *fpin,*fpdef,*fpask;
	struct rpb rpb;

	/* Read the rpb boot structure from kernel memory */
    	if(nl[NL_rpb].n_type == N_UNDF)
		quitonerror(-7);
	offset = reset_anythg (NL_rpb);
	lseek(kmem,offset,0);
	read(kmem,&rpb,sizeof(rpb));
	cmdfile[0] = askfile[0] = bootfile[0] = NULL;

	switch (CPU)
	{
	case VAX_780:
		switch (rpb.devtyp)
		{
		case BTD$K_UDA:
			strcpy(cmdfile,"/usr/sys/780cons/ubara.cmd");
			break;

		case BTD$K_MB:
			strcpy(cmdfile,"/usr/sys/780cons/mbahp.cmd");
			break;

		case BTD$K_HSCCI:
			strcpy(cmdfile,"/usr/sys/780cons/cira.cmd");
			break;
		default:
			exit(2);  /* no need to update */
		}
		strcpy(askfile,"/usr/sys/780cons/askboo.cmd");
		strcpy(bootfile,"/usr/sys/780cons/defboo.cmd");
		break;

	case VAX_8600:
		switch (rpb.devtyp)
		{
		case BTD$K_UDA:
			strcpy(cmdfile,"/usr/sys/8600cons/ubara.com");
			break;
		
		case BTD$K_MB:
			strcpy(cmdfile,"/usr/sys/8600cons/mbahp.com");
			break;

		case BTD$K_HSCCI:
			strcpy(cmdfile,"/usr/sys/8600cons/cira.com");
			break;
		default:
			exit(2);  /* no need to update */
		}
		strcpy(askfile,"/usr/sys/8600cons/askboo.com");
		strcpy(bootfile,"/usr/sys/8600cons/defboo.com");
		break;

	case VAX_730:
		switch (rpb.devtyp)
		{
		case BTD$K_UDA:
			strcpy(cmdfile,"/usr/sys/730cons/ubara.cmd");
			break;

		case BTD$K_DQ:
			strcpy(cmdfile,"/usr/sys/730cons/ubaidc.cmd");
			break;
		default:
			exit(2);  /* no need to update */
		}
		strcpy(askfile,"/usr/sys/730cons/askboo.cmd");
		strcpy(bootfile,"/usr/sys/730cons/defboo.cmd");
		break;

	case VAX_750:
		switch (rpb.devtyp)
		{
		case BTD$K_HSCCI:
			strcpy(cmdfile,"/usr/sys/750cons/cira.cmd");
			strcpy(askfile,"/usr/sys/750cons/askboo.cmd");
			strcpy(bootfile,"/usr/sys/750cons/defboo.cmd");
			break;
		default:
			exit(2);  /* no need to update */
		}
		break;

	case VAX_8200:
		switch (rpb.devtyp)
		{
		case BTD$K_HSCCI:
			strcpy(cmdfile,"/usr/sys/8200cons/cira.cmd");
			strcpy(askfile,"/usr/sys/8200cons/askboo.cmd");
			strcpy(bootfile,"/usr/sys/8200cons/defboo.cmd");
			break;
		default:
			exit(2);  /* no need to update */
		}
		break;

	case VAX_6200:
	case VAX_6400:
		printf("\
After the processor is halted, follow the steps listed below \n\
to update console boot defaults:\n\n\
1. Set the key switch on the front panel to the position labeled 'Update'.\n\n\
2. At the console mode prompt, enter the following commands:\n");

			switch (rpb.devtyp)
		{
		case BTD$K_HSCCI:
		    if(( rpb.bootr1 & 0x0f ) == 0 ) {
		        printf("\t>>> set boot default /xmi:%x /node:%x /r5:10008 du%x\n",
		        rpb.bootr1>>4, rpb.bootr2, rpb.unit);
		        printf("\t>>> set boot ask /xmi:%x /node:%x /r5:1000b du%x\n",
		        rpb.bootr1>>4, rpb.bootr2, rpb.unit);
		    } else {
		        printf("\t>>> set boot default /xmi:%x /bi:%x /node:%x /r5:10008 du%x\n",
		        rpb.bootr1>>4, rpb.bootr1 & 0x0f, rpb.bootr2, rpb.unit);
		        printf("\t>>> set boot ask /xmi:%x /bi:%x /node:%x /r5:1000b du%x\n",
		        rpb.bootr1>>4, rpb.bootr1 & 0x0f, rpb.bootr2, rpb.unit);
		    }
		    break;
		default:
		    if(( rpb.bootr1 & 0x0f ) == 0 ) {
			printf("\t>>> set boot default /xmi:%x /r5:10008 du%x\n",
				rpb.bootr1>>4, rpb.unit);
			printf("\t>>> set boot ask /xmi:%x /r5:1000b du%x\n",
				rpb.bootr1>>4, rpb.unit);
		    } else {
			printf("\t>>> set boot default /xmi:%x /bi:%x /r5:10008 du%x\n",
				rpb.bootr1>>4, rpb.bootr1 & 0x0f, rpb.unit);
			printf("\t>>> set boot ask /xmi:%x /bi:%x /r5:1000b du%x\n",
				rpb.bootr1>>4, rpb.bootr1 & 0x0f, rpb.unit);
		    }
		    break;
		}
		printf("\n\
3. Set the key switch back to its original position.\n\n\
4. Now you can boot the default system device to multi-user mode by\n\
   entering:\n\
	>>> b \n\n\
   Or, you can boot the default system device to single-user mode by\n\
   entering:\n\
	>>> b ask \n\n\
* Make sure the TK50 console tape is in the drive before booting the system.\n");
		exit(0);

	case VAX_3400:
		printf("\
After the processor is halted, follow the steps listed below \n\
to update console boot default:\n\n\
At the console mode prompt, enter the following command:\n");
		switch (rpb.devtyp)
		{ 
		case BTD$K_SII:
			printf("\t>>> set boot dia%x\n",rpb.unit);
			break;
		default:
			printf("\t>>> set boot dua%x\n",rpb.unit);
			break;
		}
		printf("\n\
After the update, you can boot the default system device to multi-user\n\
mode by entering:\n\
	>>> b\n");
		exit(0);
	
	case VAX_60:
	case C_VAXSTAR:
		printf("\
After the processor is halted, follow the steps listed below \n\
to update console boot default:\n\n\
At the console mode prompt, enter the following command:\n");
		if(rpb.unit >= 100)
			rpb.unit /= 100;
		tctlr = 'a' + rpb.ctrllr - 1;
		switch (rpb.devtyp)
		{ 
		case BTD$K_SII:
			printf("\t>>> set boot dk%c%x00\n",tctlr,rpb.unit);
			break;
		case BTD$K_KA420_DISK:
			printf("\t>>> set boot dk%c%x00\n",tctlr,rpb.unit);
			break;
		default:
			break;
		}
		printf("\n\
After the update, you can boot the default system device to multi-user\n\
mode by entering:\n\
	>>> b\n");
		exit(0);

	default:
		break;;
	}

	if(strlen(cmdfile) == 0 || strlen(bootfile) == 0 ||
			strlen(askfile) == 0)
		exit(0);

	if((fpin=fopen(cmdfile,"r")) == NULL)
	{
		fprintf(stderr,"Cannot open (%s).\n", cmdfile);
		fprintf(stderr,"Cannot continue the console update.\n");
		quitonerror(-8);
	}

	if((fpdef=fopen(bootfile,"w")) == NULL)
	{
		fprintf(stderr,"Cannot open (%s).\n", bootfile);
		fprintf(stderr,"Cannot continue the console update.\n");
		quitonerror(-8);
	}

	if((fpask=fopen(askfile,"w")) == NULL)
	{
		fprintf(stderr,"Cannot open (%s).\n", askfile);
		fprintf(stderr,"Cannot continue the console update.\n");
		quitonerror(-8);
	}

	while(fgets(line,80,fpin) != NULL)
	{
		switch (CPU)
		{
		case VAX_780 :
		    switch (rpb.devtyp)
		    {
			case BTD$K_HSCCI:
			    if (strncmp(line,"DEPOSIT R1",10)==0)
			    {
				fprintf(fpdef,"%s",line);
				fprintf(fpask,"%s",line);
				sprintf(line,"DEPOSIT R2 %x   ! HSC UNIT\n",rpb.bootr2);
				toupcase(line);
				fprintf(fpdef,"%s",line);
				fprintf(fpask,"%s",line);
				sprintf(line,"DEPOSIT R3 %x   ! DISK UNIT\n",rpb.bootr3);
				toupcase(line);
				fprintf(fpdef,"%s",line);
				fprintf(fpask,"%s",line);
				continue;
			    }
			    if(strncmp(line,"DEPOSIT R5 1000B",16) == 0)
			    {
				fprintf(fpask,"%s",line);
				sprintf(line,"DEPOSIT R5 10008");
				strcat(line,"   ! BOOT ULTRIX TO MULTI USER\n");
				fprintf(fpdef,"%s",line);
				continue;
			    }
			    fprintf(fpdef,"%s",line);
			    fprintf(fpask,"%s",line);
			    break;

			default: 
			    if(strncmp(line,"!R1",3) == 0)
			    {
				sprintf(line,"DEPOSIT R1 %x",rpb.bootr1);
				strcat(line,"   ! TR LEVEL OF UNIBUS\n");
			    }
			    if(strncmp(line,"!R3",3) == 0)
			    {
				toupcase(line);
				sprintf(line,"DEPOSIT R3 %x",rpb.bootr3);
				strcat(line,"   ! PLUG # OF SYSTEM DISK\n");
			    }
			    if(strncmp(line,"DEPOSIT R5 1000B",16) == 0)
			    {
				fprintf(fpask,"%s",line);
				sprintf(line,"DEPOSIT R5 10008");
				strcat(line,"   ! BOOT ULTRIX TO MULTI USER\n");
				fprintf(fpdef,"%s",line);
				continue;
			    }
			    fprintf(fpdef,"%s",line);
			    fprintf(fpask,"%s",line);
			    break;
			}
			break;

		case VAX_8600:
		    switch (rpb.devtyp)
		    {
			case BTD$K_HSCCI:
			    if (strncmp(line,"DEPOSIT R0",10)==0)
			    {
				fprintf(fpdef,"%s",line);
				fprintf(fpask,"%s",line);
				sprintf(line,"DEPOSIT R1 %x   ! TR number of CI780 \n",rpb.bootr1);
				toupcase(line);
				fprintf(fpdef,"%s",line);
				fprintf(fpask,"%s",line);
				sprintf(line,"DEPOSIT R2 %x   ! HSC UNIT\n",rpb.bootr2);
				toupcase(line);
				fprintf(fpdef,"%s",line);
				fprintf(fpask,"%s",line);
				sprintf(line,"DEPOSIT R3 %x   ! DISK UNIT\n",rpb.bootr3);
				toupcase(line);
				fprintf(fpdef,"%s",line);
				fprintf(fpask,"%s",line);
				continue;
			    }
			    if(strncmp(line,"DEPOSIT R5 1000B",16) == 0)
			    {
				fprintf(fpask,"%s",line);
				sprintf(line,"DEPOSIT R5 10008");
				strcat(line,"   ! BOOT ULTRIX TO MULTI USER\n");
				fprintf(fpdef,"%s",line);
				continue;
			    }
			    fprintf(fpdef,"%s",line);
			    fprintf(fpask,"%s",line);
			    break;

			default: 
			    if(strncmp(line,"!R1",3) == 0)
			    {
				sprintf(line,"DEPOSIT R1 %x",rpb.bootr1);
				strcat(line,"   ! TR LEVEL OF UNIBUS\n");
				toupcase(line);
			    }
			    if(strncmp(line,"!R3",3) == 0)
			    {
				sprintf(line,"DEPOSIT R3 %x",rpb.bootr3);
				strcat(line,"   ! PLUG # OF SYSTEM DISK\n");
				toupcase(line);
			    }
			    if(strncmp(line,"DEPOSIT R5 1000B",16) == 0)
			    {
				fprintf(fpask,"%s",line);
				sprintf(line,"DEPOSIT R5 10008");
				strcat(line,"   ! BOOT ULTRIX TO MULTI USER\n");
				toupcase(line);
				fprintf(fpdef,"%s",line);
				continue;
			    }
			    fprintf(fpdef,"%s",line);
			    fprintf(fpask,"%s",line);
			    break;
			}
			break;

		case VAX_730:
			if(strncmp(line,"D/G/L 2",7) == 0)
			{
				fprintf(fpdef,"%s",line);
				fprintf(fpask,"%s",line);
				fprintf(fpdef,"D/G/L 3 %x\n",rpb.bootr3);
				fprintf(fpask,"D/G/L 3 %x\n",rpb.bootr3);
				continue;
			}
			if(strncmp(line,"D/G/L 5 1000B",13) == 0)
			{
				fprintf(fpask,"%s",line);
				sprintf(line,"D/G/L 5 10008\n");
				fprintf(fpdef,"%s",line);
				continue;
			}
			fprintf(fpdef,"%s",line);
			fprintf(fpask,"%s",line);
			break;

		case VAX_750:  /* HSCCI */
			if(strncmp(line,"D/G 1",5) == 0)
			{
				fprintf(fpdef,"%s",line);
				fprintf(fpask,"%s",line);
				sprintf(line,"D/G 2 %x\n",rpb.bootr2);
				toupcase(line);
				fprintf(fpdef,"%s",line);
				fprintf(fpask,"%s",line);
				sprintf(line,"D/G 3 %x\n",rpb.bootr3);
				toupcase(line);
				fprintf(fpdef,"%s",line);
				fprintf(fpask,"%s",line);
				continue;
			}
			if(strncmp(line,"D/G 5 1000B",11) == 0)
			{
				fprintf(fpask,"%s",line);
				sprintf(line,"D/G 5 10008\n");
				fprintf(fpdef,"%s",line);
				continue;
			} 
			fprintf(fpdef,"%s",line);
			fprintf(fpask,"%s",line);
			break;

		case VAX_8200:	/* HSCCI */  	
			if(strncmp(line,"D/G 0",5) == 0)
			{
				fprintf(fpdef,"%s",line);
				fprintf(fpask,"%s",line);
				sprintf(line,"D/G 1 %x\n",rpb.bootr1);
				toupcase(line);
				fprintf(fpdef,"%s",line);
				fprintf(fpask,"%s",line);
				sprintf(line,"D/G 2 %x\n",rpb.bootr2);
				toupcase(line);
				fprintf(fpdef,"%s",line);
				fprintf(fpask,"%s",line);
				sprintf(line,"D/G 3 %x\n",rpb.bootr3);
				toupcase(line);
				fprintf(fpdef,"%s",line);
				fprintf(fpask,"%s",line);
				continue;
			}
			if(strncmp(line,"D/G 5 1000B",11) == 0)
			{
				fprintf(fpask,"%s",line);
				sprintf(line,"D/G 5 10008\n");
				fprintf(fpdef,"%s",line);
				continue;
			}
			fprintf(fpdef,"%s",line);
			fprintf(fpask,"%s",line);
			break;
		}
	}
	fclose(fpdef);
	fclose(fpask);
	fclose(fpin);
#endif vax
}

/****************************************************************
*    toupcase							*
*								*
*    Convert a line to upper case.  				*
****************************************************************/
toupcase(line)
char line[];
{
	int i;
	for (i=0; line[i]!='\0'; i++)
		if (islower(line[i]))
			line[i]=_toupper(line[i]);
	line[i]='\0';
}
