/************************************************************************/
/*								 	*/
/* Usage: size_part <partitions> <size number>                          */
/*								 	*/
/*	This program will compare the requested partitions whether are  */
/* larger than the minimun required size.			    	*/
/*       The program looks for /tmp/ptable which come from            	*/
/* "/etc/chpt -q /dev/rra?c > /tmp/ptable ".	  	      		*/
/*	The input has two arguments: the fisrt one is for partitions,	*/
/* the second one is required minimun size.		                */
/*								 	*/
/************************************************************************/
/* Modification history:					 	*/
/*								 	*/
/* 000 - August 26, 1986 - Tungning Cherng created			*/
/************************************************************************/

#ifndef lint
static char *sccsid = "@(#)size_part.c	4.1 (ULTRIX) 7/2/90";
#endif

#include <stdio.h>
main(argc, argv)
int argc;
char **argv;
{
	FILE *fp;
	int i,j,k;
	char c;
	char line[80];
	char ssize[8];
	int nsize;
	int SIZE;
	char *ptable="/tmp/ptable";
	if (argc != 3)
	{
		fprintf(stderr,"%s: argument wrong !!! \n",argv[0]);
		exit(1);
	}
	if ((fp=fopen(ptable, "r"))==NULL)
	{
		fprintf(stderr, "Cannot open ptable\n");
		exit(1);
	}
	SIZE=atoi(argv[2]);
	for (i=0; argv[1][i]!=0; i++)
		if (argv[1][i]!=' ')
		{
			rewind(fp);
			while (fgets(line,sizeof(line),fp)!=NULL)
			{
				if (argv[1][i]!=line[4])
					continue;
				for (j=5; line[j++]==' '; )
					;
				for (; line[j++]!=' '; )
					;
				for (; line[j++]==' '; )
					;
				for (; line[j++]!=' '; )
					;
				for (; line[j]==' ';j++ )
					;
				for (k=0; line[j]!=' ';j++)
					ssize[k++]=line[j];	
				ssize[k]=0;
				nsize=atoi(ssize);
				if (nsize > SIZE)
					printf("%c ",argv[1][i]);
			}
		}
	printf("\n");
}
				
				
