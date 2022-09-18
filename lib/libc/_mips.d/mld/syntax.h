/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: syntax.h,v 2010.2.1.3 89/11/29 14:29:58 bettina Exp $ */
/* Niceties for programming in C */

#define false 0
#define true 1
typedef int boolean;
#define private static

/* Useful for passing procedures as arguments */
typedef int (*procedure)();

#include <stdio.h>

char *malloc(/* unsigned */);
char *realloc(/* char *, unsigned */);
free(/* char * */);
int atoi(/* char * */);
double atof(/* char * */);
#ifdef SYSV
#define rindex strrchr
#define index strchr
#define bzero(s, n) memset((s), 0, (n))
char *memset(/* char *, char, int */);
#else
void bzero(/* char *, int */);
#endif
char *index(/* char *, char */);
char *rindex(/* char *, char */);
char *strcpy(/* char *, char * */);
int strcmp(/* char *, char * */);
char *strcat(/* char *, char * */);
exit(/* int */);
FILE *fopen(/* char *, char * */);
#define FROM_BEGINNING 0
int fseek(/* FILE *, long, int */);
int getw(/* FILE * */);
int putw(/* int, FILE * */);
int bcmp(/* char *, char * */);
int fclose(/* FILE * */);
fprintf(/* FILE *, char *, ... */);
fputs(/* char *, FILE * */);
int fstat(/* int, struct stat *buf */);
printf(/* char *, ... */);
#ifdef SYSV
int sprintf(/* char *, char *, ... */);
#else
char *sprintf(/* char *, char *, ... */);
#endif
qsort(/* char *, int, int, int(*)() */);
int getuid();
int getgid();
int geteuid();
int getegid();
char *ctime(/* long * */);
long time(/* long * */);

/* Anybody who wants to indicate an error status without exiting immediately
   may set this */
extern int exit_status;
