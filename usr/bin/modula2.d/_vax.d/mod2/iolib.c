/*#@(#)iolib.c	4.1	Ultrix	7/17/90*/
/****************************************************************************
 *									    *
 *  Copyright (c) 1984 by						    *
 *  DIGITAL EQUIPMENT CORPORATION, Maynard, Massachusetts.		    *
 *  All rights reserved.						    *
 * 									    *
 *  This software is furnished under a license and may be used and copied   *
 *  only in  accordance with  the  terms  of  such  license  and with the   *
 *  inclusion of the above copyright notice. This software or  any  other   *
 *  copies thereof may not be provided or otherwise made available to any   *
 *  other person.  No title to and ownership of  the  software is  hereby   *
 *  transferred.							    *
 * 									    *
 *  The information in this software is  subject to change without notice   *
 *  and  should  not  be  construed as  a commitment by DIGITAL EQUIPMENT   *
 *  CORPORATION.							    *
 * 									    *
 *  DIGITAL assumes no responsibility for the use  or  reliability of its   *
 *  software on equipment which is not supplied by DIGITAL.		    *
 * 									    *
$Header: iolib.c,v 1.3 84/05/19 11:41:05 powell Exp $
 ****************************************************************************/
#include <stdio.h>
#define MAXSEARCH 10
#define PATHNAME "MODPATH"
#define MAXLINE 1024
char *searchList[MAXSEARCH];
int numSearchEntries = 0;
char pathstring[MAXSEARCH*100];
char *getenv();
InitFiles() {
    char *s; int i;
    s = getenv(PATHNAME);
    if (s!=NULL) {
	while (*s != '\0') {
	    searchList[numSearchEntries] = s;
	    while (*s != '\0' && *s != ':') s++;
	    if (*s!='\0') {
		*s = '\0';
		s++;
	    }
	    numSearchEntries++;
	}
    }
    searchList[numSearchEntries] = ".";
    numSearchEntries++;
/***fprintf(stderr,"Search list\n");
    for (i=0;i<numSearchEntries;i++) {
	fprintf(stderr,"%s\n",searchList[i]);
    }
***/
}
FILE *GetInput() {
    return(stdin);
}
FILE *Open(fn,fnl) char *fn; int fnl;{
    char name[100];
    FILE *f;
    int i;
    
    for (i=0;i<numSearchEntries;i++) {
	strcpy(name,searchList[i]);
	strcat(name,"/");
	strncat(name,fn,fnl);
	f = fopen(name,"r");
	if (f != NULL) return(f);
    }
    return(NULL);
}
Close(f) FILE *f; {
    fclose(f);
}
ReadLine(f,b) FILE *f; char *b; {
    register int i;
    if (fgets(b,MAXLINE,f)) {
	i = 0;
	while (b[i]!='\0') i++;
    } else {
	i = -1;
    }
    return(i);
}
