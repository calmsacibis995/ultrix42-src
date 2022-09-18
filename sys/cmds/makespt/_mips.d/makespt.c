#ifndef lint
static	char	*sccsid = "@(#)makespt.c	1.4	(ULTRIX)	3/8/89";
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

/*
 *
 * Written by rr
 *
 *			Modification History
 * 7/18/88	jaa
 *		recognize key field in spt.s
 *
 *
 * 12/24/87	jaa
 *		added dynamic memory allocation
 */

#include <stdio.h>
#include <mips/pte.h>

#define NUM_ARGS 100
#define MAX_NAME 80
#define MAXLINE 132

#define SYSMAP         "Sysmap"
#define COMM           ".comm _%s,%d\n"
#define GLOBL          ".globl _%s\n.set _%s,0x%x\n"
#define GLOBL0         ".globl _%s\n.set _%s,0\n"
#define SYSSIZE        ".globl _Syssize\n.set _Syssize,0x%x\n"
#define SYSPTSIZE      ".data\n.align 2\n.globl _sysptsize\n_sysptsize:.long	%d\n"

char *malloc();
void bcopy(), exit();

main()
{

	char mapname[MAX_NAME], virtual[MAX_NAME], sizebuf[MAX_NAME], 
		key[MAX_NAME], newbuf[MAX_NAME];
	char *expr_args[NUM_ARGS];
        char *str, *cp;
	int i, j, num_args, size = 0, totalsize = 0;
	unsigned int v = 0x80000000;
	struct work {
		char lin[MAXLINE];
		struct work *nxt;
	} *hdcomm, *hdglobl, *gptr, *cptr;

	bzero((char *)expr_args, sizeof(char *)*NUM_ARGS);
	if((str = malloc(MAXLINE)) == NULL){
		perror("malloc");
		exit(1);
	}

	if((gptr = (struct work *)malloc(sizeof(struct work))) == NULL){
	      perror("malloc");
	      exit(2);
        }

	if((cptr = (struct work *)malloc(sizeof(struct work))) == NULL){
	      perror("malloc");
	      exit(2);
        }

	hdglobl = gptr;
	hdcomm = cptr;

	while(gets(str)){
	/*
	 * throw out any empty lines, comments and leading white space
	 */
	      cp = str;
	      if(*cp == '\t' || *cp == ' ')
	              while(*cp == '\t' || *cp == ' ') 
			++cp;
	      if(*cp == '#' || *cp == '\0')
	              continue;
	      if(sscanf(cp, "%s\t%s\t%s\t%[^\n]", 
	      key, mapname, virtual, sizebuf) == EOF){
		perror("scanf");
		exit(3);
	      }
	      /* 
	       * look for SYSMAP macro lines only 
	       * this allows any file to be included in spt.s
	       */
	      if(strcmp(key, "SPT"))
		continue;
	      for(i = 0, j = 0; sizebuf[i]; i++)
		if(sizebuf[i] != ' ' && sizebuf[i] != '\t')
			newbuf[j++] = sizebuf[i];
	      newbuf[j] = 0;
	      num_args = make_args(&expr_args[1], newbuf);
	      size = expr_main(num_args+1, expr_args);
	      if (size & 1)
	              size++;	/* clsize roundup! */
	      if (size) {
		      if (strcmp(mapname, SYSMAP) == 0) {
			/* do extern first so pte's are contiguous */
 			      (void)sprintf(cptr->lin, SYSPTSIZE, size);
			      if((cptr->nxt = (struct work *)malloc(sizeof(struct work))) == NULL){
				      perror("malloc");
				      exit(4);
			      }
			      cptr = cptr->nxt;
		      }
 		      (void)sprintf(cptr->lin, COMM, mapname, size*sizeof(struct pte));
		      if((cptr->nxt = (struct work *)malloc(sizeof(struct work))) == NULL){
			      perror("malloc");
			      exit(5);
		      }
		      cptr = cptr->nxt;
	      }
	      else {
		      /* hack for now - take out */
 		      (void)sprintf(gptr->lin, GLOBL0, mapname, mapname);
		      if((gptr->nxt = (struct work *)malloc(sizeof(struct work))) == NULL){
			      perror("malloc");
			      exit(6);
		      }
		      gptr = gptr->nxt;
		      /* hack for now - take out */
	      }

	      (void)sprintf(gptr->lin, GLOBL, virtual, virtual, v);
	      if((gptr->nxt = (struct work *)malloc(sizeof(struct work))) == NULL){
		      perror("malloc");
		      exit(7);
	      }

	      gptr = gptr->nxt;
	      v += 512*size;
	      totalsize += size;
	      (void)fflush(stdin);
	}                /* end while */

	if(gptr == (struct work *)NULL)
	      if((gptr = (struct work *)malloc(sizeof(struct work))) == NULL){
		      perror("malloc");
		      exit(8);
	      }

	/* set the Syssize for the SLR */
	(void)sprintf(gptr->lin, SYSSIZE, totalsize);
	gptr->nxt = (struct work *)NULL;

	(void)fflush(stdin);
	for(cptr = hdcomm; cptr != (struct work *)NULL; cptr = cptr->nxt)
	     (void)printf("%s", cptr->lin);
	
	(void)fflush(stdout);
	for(gptr = hdglobl; gptr != (struct work *)NULL; gptr = gptr->nxt)
	        (void)printf("%s", gptr->lin);

	(void)fflush(stdout);
	exit(0);
}

make_args(args,string)
register char **args;
register char *string;
{
	register int i = 0;
	register int j = 0;
	char buf[BUFSIZ];
	register char *ptr = buf;
	bzero(buf,BUFSIZ);
	while (*string) {
		if (*string != ' ') *ptr++ = *string;
		string++;
	}
	ptr = buf;
	while (*ptr) {
		j = strspn(ptr,"0123456789");
		if (j>0) {
			args[i] = malloc((unsigned)j+1);
			bcopy(ptr,args[i],j);
			args[i][j] = '\0';
			i++;
			ptr += j;
		}
		j = strspn(ptr,"(+-*/%)");
		if (j==0) goto out;
again:
		args[i] = malloc((unsigned)2);
		args[i][0] = *ptr;
		args[i][1] = '\0';
		i++;
		ptr++;
		if (j>1) {	/* one special char at a time */
			j--;
			goto again;
		}
	}
out:
	return(i);
}

