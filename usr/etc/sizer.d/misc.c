#ifndef lint
static	char	*sccsid = "@(#)misc.c	4.1  (ULTRIX)        7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright(c) 1987 by				*
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
 * Name: misc.c
 *
 * Modification History
 * 
 * May 21, 1990 - Robin
 *	added code to generate the pseudo device entry to support
 *	presto nvram driver.
 *
 * Dec 15, 1989 - Alan Frechette
 *	Added subroutine getwsinfo().
 *
 * Oct 11, 1989 - Alan Frechette
 *	Fixed a bug in the search_config() subroutine.
 *
 * May 10, 1989 - Alan Frechette
 *	Set pagesize to NBPG in routine getphysmem() to correctly
 *	determine the size of physical memory.
 *
 * Feb 12, 1989 - Alan Frechette
 *	New sizer code which supports multiple architectures.
 *	Restructured this code and cleaned it up considerably.
 *	Based on the original V3.0 sizer by Tungning Cherng.
 *
 ***********************************************************************/

#include <stdio.h>
#include <ctype.h>
#include "sizer.h"

/****************************************************************
*    checksysname						*
*								*
*    Check the system name and if it is not a valid system	*
*    name then prompt the user for a correct system name.	*
****************************************************************/
checksysname(sysname)
char *sysname;
{
	char *ptr;
	char newsysname[100];
	int errorflag;
	int rcode;

	while(1) {
		ptr = sysname;
		errorflag = 0;
		while(*ptr != '\0') {
			if(isdigit(*ptr) || isalpha(*ptr) ||
				*ptr == '_' || *ptr == '.' || *ptr == '-' )
				ptr++;
			else {
				fprintf(stderr,
				"Invalid character %c in system name. ",*ptr);
				fprintf(stderr,"Only use alphanumeric.");
				errorflag = 1;
				break;
			}
		}
		if(errorflag == 0)
			break;
		else {
			fprintf(stdout, "Enter a valid system name: ");
			rcode = scanf("%s", newsysname);
			sysname = &newsysname[0];
			if(rcode < 1 || rcode  == EOF)
				quitonerror(-3);
		}
	}
}

/****************************************************************
*    getconfig_string						*
*								*
*    Search the /install.tmp/.config file for any options,	*
*    pseudo-devices, controllers, etc, to add to our config 	*
*    file.  It's only argument is the type of config file	*
*    entry it is to look for in /install.tmp/.config. When	*
*    it finds a match it adds that line to our config file.	*
****************************************************************/
getconfig_string(type)
char *type;
{
	FILE *fpconfig;
	char line[PATHSIZE],conftype[20];
 	int  i,len,found=0;

	if((fpconfig = fopen("/install.tmp/.config","r")) == NULL) {
 		return(found);
	}
	while(fgets(line,PATHSIZE,fpconfig) != NULL) {
		sscanf(line,"%s",conftype);
		if(strcmp(type,"hardware") == 0) {
			for(i=0; i<HARDTBLSIZE; i++) {
				if(strcmp(conftype,hardtbl[i].typename) == 0) {
					found = 1;
					fprintf(fp,"%s",line);
				}
			}
		}
		else if(strcmp(conftype,type) == 0) {
			found = 1;
			if(strcmp(type,"swap") == 0 
				|| strcmp(type,"dumps") == 0) {
				len = strlen(line);
				line[len-1] = ' ';
				strcat(line," \0");
			}
 			fprintf(fp,"%s",line);
 		}
	}
 	fclose(fpconfig);
	return(found);
}

/****************************************************************
*    search_config						*
*                 						*
*    	Search the /install.tmp/.config file for the given	*
*	string fields.						*
****************************************************************/
search_config(key1,word1)
char key1[];
char word1[];
{
	FILE *fpconfig;
	char line[PATHSIZE];
	char key2[20];
	char word2[30];
	int found=0;

	if((fpconfig = fopen("/install.tmp/.config","r")) == NULL) {
 		return(found);
	}
	while(fgets(line,PATHSIZE,fpconfig) != NULL) {
		sscanf(line,"%s%s",key2,word2);
		if(strcmp(key1,key2) == 0 && strcmp(word1,word2)==0) {
 			found=1;
			break;
 		}
	}
 	fclose(fpconfig);
	return(found);
}

/****************************************************************
*    reset_anythg						*
*								*
*    Go back to the begining of a namelist location.		*
****************************************************************/
long reset_anythg(nlindex)
int nlindex;
{
	long offset;

	offset = lseek(kmem,(long)nl[nlindex].n_value, 0);
	return(offset);
}

/****************************************************************
*    asksysid							*
*								*
*    Ask for the scs_sysid. 					*
****************************************************************/
asksysid()
{
	char buf[60];
	int i,n;
RETRY:
	printf("\nEnter the scs_sysid number: ");
	fflush(stdout);
	gets(buf);
	for (i=0; buf[i]==' ' || buf[i]=='\t'; i++)
		;
	for (n=i; buf[i]!='\0' ;i++)
		if (!(isdigit(buf[i]))) {
			fprintf(stderr,"Invalid scs_sysid %s. ", &buf[i]);
			fprintf(stderr,"Only use integer number.\n"); 
			goto RETRY;
		}
	n=atoi(&buf[n]);
	return(n);
}

/****************************************************************
*    getphysmem							*
*								*
*    Get the size of physical memory.				*
****************************************************************/
getphysmem()
{
	int physmem, memmeg, pagesize;
	long offset;

	/* Read physical memory size from kernel memory */
    	if(nl[NL_Physmem].n_type == N_UNDF)
		quitonerror(-5);
	offset = reset_anythg(NL_Physmem);
	offset = lseek(kmem, offset, 0);
	if(read(kmem, &physmem, sizeof(physmem)) < 0) {
		fprintf(stderr,"Can not size memory!  Assumming 16 meg.\n");
		return(16);
	}
	else {
		/* Convert "physmem" to megs of memory. */
		pagesize = NBPG;	
		memmeg = (physmem * pagesize) / 0x100000;
		if(physmem % (0x100000 / pagesize))
	    		memmeg++;
		return(memmeg);
	}
}

/****************************************************************
*    gettimezone						*
*								*
*    Get time zone.						*
****************************************************************/
gettimezone(tz, dst)
int *tz;
int *dst;
{

	long offset;
	struct timezone tzb;

	/* Read the timezone from kernel memory */
    	if(nl[NL_tz].n_type == N_UNDF)
		quitonerror(-6);
	offset = reset_anythg(NL_tz);
	offset = lseek(kmem, offset, 0);
	read(kmem, &tzb, sizeof(tzb));
	*tz = tzb.tz_minuteswest / 60;
	*dst = tzb.tz_dsttime;
}

/****************************************************************
*    getsysid							*
*								*
*    Get the scs_sysid. 						*
****************************************************************/
getsysid()
{
	long offset;
	short sysid;

	/* Read the scs_sysid from kernel memory */
  	offset = reset_anythg(NL_ci_first_port);
        lseek(kmem, offset, 0);
        read(kmem, &sysid, sizeof(sysid));
	if(offset == 0 || sysid == 0)	
		return(1);
	else
		return(sysid);
}

/************************************************************************
*    get_X               						*
*                        						*
*    Get the workstation device type.					*
*                        						*
*    If found=1 then put option UWS only.				*
*    If found=2 then put option UWS, XOS and pseudo-device xos.		*
************************************************************************/
#define SUBSETS 	"/usr/etc/subsets"
#define UWSLOCK		"UWSXOS"		/* UWSXOS*.lk */
get_X()
{
 	int ws;
	struct direct *dp;
	DIR *dirp;
	char *s;
	int found;
	long offset;

	/* Read the workstation display type from kernel memory */
 	ws = found = 0;
 	offset = reset_anythg (NL_ws_display_type);
 	offset = lseek (kmem, offset, 0);
 	read (kmem, &ws, sizeof (ws));
 
 	if(ws != 0) {
		found=1;
		if ((dirp=opendir(SUBSETS))==NULL)
			return(found);
		for (dp=readdir(dirp); dp!=NULL; dp=readdir(dirp)) {
			if (strncmp(dp->d_name,UWSLOCK,6)==0) {
				s=(char * )strchr(dp->d_name, '.');
				if (s!=NULL && (strcmp(s,".lk")==0)) {
					found=2;
					break;
				}
			}	
		}
		closedir(dirp);
 	}
 	return(found);
}

/************************************************************************
*    getwsinfo               						*
*                        						*
*    Get and display the workstation display info. 			*
*                        						*
************************************************************************/
getwsinfo(tflag)
int tflag;
{
 	int ws=0;
	long offset;

	/* Get the workstation display type */
	if(tflag == 1)
		getsysinfo(GSI_WSD_TYPE, &ws, sizeof(ws));
	/* Get the workstation display units */
	else if(tflag == 2)
		getsysinfo(GSI_WSD_UNITS, &ws, sizeof(ws));
	fprintf(stdout, "%d\n", ws);
}
/****************************************************************
*    getnvram							*
*								*
*    Get the nvram size (prsize).  If its not zero the kernel	*
*    knows about and supports the "presto" nvram device driver  *
****************************************************************/
getnvram()
{
	long offset;
	int nvram_size;

	/* Read nvram size from kernel memory */
    	if(nl[NL_prsize].n_type == N_UNDF)
		return(0);
	/* Read the nvram_size from kernel memory */
  	offset = reset_anythg(NL_prsize);
        lseek(kmem, offset, 0);
        read(kmem, &nvram_size, sizeof(nvram_size));
	if(offset == 0 || nvram_size == 0)	
		return(0);
	else
		return(nvram_size);
}
