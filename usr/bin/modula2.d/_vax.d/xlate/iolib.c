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
 *									    *
 ****************************************************************************/
#include <stdio.h>
#define	STDINNAME	"SYS$INPUT"
#define	STDOUTNAME	"SYS$OUTPUT"
#define	STDERRNAME	"SYS$ERROR"
#define	MAXNAME		96	/* maximum name length */
#define MAXSTRINGSIZE 	100	/* maximum string length */

/* open specified file.  Name is null or space terminated.
    Returns Unix fd. Can be used for Unix read system call.
*/
int Open(name) char *name; {
    char filename[MAXNAME+1];
    int i;
    int file;
    i = 0;
    while (i < MAXNAME && name[i] != '\0' && name[i] != ' ' ) {
	filename[i] = name[i];
	i = i + 1;
    }
    filename[i] = '\0';
    if (strcmp(filename,STDINNAME)==0) {
        file = 0;
    } else {
	file = open(filename,0);
    }
    return (file);
}

/* create specified file.  Name is null or space terminated.
    Returns stdio buffer pointer. Can be used for fwrite, fprintf.
*/
FILE *Create(name) char *name; {
    char filename[MAXNAME+1];
    int i;
    FILE *file;
    i = 0;
    while (i < MAXNAME && name[i] != '\0' && name[i] != ' ' ) {
	filename[i] = name[i];
	i = i + 1;
    }
    filename[i] = '\0';
    if (strcmp(filename,STDERRNAME)==0) {
        file = stderr;
    } else if (strcmp(filename,STDOUTNAME)==0) {
	file = stdout;
    } else {
	file = fopen(filename,"w");
    }
    return (file);
}

/* formatted write of an integer */
WriteI(file,value,width) FILE *file; int value, width; {
    fprintf(file,"%*d",width,value);
}

/* formatted write of a character */
WriteC(file,value,width) FILE *file; char value; int width; {
    int i;
    /* pad on left */
    for (i=1;i<=width-1;i++) putc(' ',file);
    putc(value,file);
}

/* formatted write of a string */
WriteS(file,value,width) FILE *file; char *value; int width; {
    int i, j;
    i = 0;
    /* find null at end */
    while ((value[i] != '\0')) {
	i = i + 1;
    }
    /* pad on left */
    for (j=0; j<width-i; j++) {
	putc(' ',file);
    }
    if (width < i && width != 0 ) {
	i = width;
    }
    for (j=0; j<i; j++) {
	putc(value[j],file);
    }
}

/* formatted write of end of line */
WriteEOL(file) FILE *file; {
    putc('\n',file);
    fflush(file);
}

/* Unix read system call */
Read(file,buf,len) int file; char *buf; int len; {
    int readlen;
    readlen = read(file,buf,len);
    return(readlen);
}


/* Mult:  perform multiply ignoring overflow */
Mult(i,j) unsigned long i, j;{ return(i*j); }

/* Add:  perform addition ignoring overflow */
Add(i,j) unsigned long i, j;{ return(i+j); }
