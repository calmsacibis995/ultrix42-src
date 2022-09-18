/************************************************************************/
/* Usage: delet_part <delete partitions> <available partitions>		*/
/*									*/
/* Modification History:						*/
/*									*/
/* 000 - August 26, 1986 - Tung-Ning Cherng created			*/
/************************************************************************/

#ifndef lint
static char *sccsid = "@(#)delet_part.c	4.1 (ULTRIX) 7/2/90";
#endif

#include <stdio.h>
main(argc, argv)
int argc;
char **argv;
{
	char avail_p;
	char delet_p, n;
	int i,j,k;
	if (argc !=3)
	{
		fprintf(stderr,"%s argument wrong !!! \n",argv[0]);
		exit(1);
	}
	delet_p=(char )alloc_bit(argv[1]);
	avail_p=(char )alloc_bit(argv[2]);
	avail_p ^= ( avail_p & delet_p);
	for (i=0,j=97; i<8;i++, j++)
	{
		n=01;
		n <<= i;
		if (avail_p & n)
			printf("%c ",j);
		}
	printf("\n");
}
		
alloc_bit(part)
char part[];
{
	char charbit;
	int i;
	charbit='\0';
	for (i=0; part[i]!=0; i++)
	{
		switch (part[i])
		{
		case ' ':  	
				break;
		case 'a':  	
				charbit |= 01;
			   	break;
		case 'b':
				charbit |= 02;
				break;
		case 'c':
				charbit |= 04;
				break;
		case 'd':
				charbit |= 010;
				break;
		case 'e':
				charbit |= 020;
				break;
		case 'f':
				charbit |= 040;
				break;
		case 'g':
				charbit |= 0100;
				break;
		case 'h':
				charbit |= 0200;
				break;
		default:
				fprintf(stderr,"No such partition !!!");
				exit(1);
		}
	}
	return(charbit);
}		
		
