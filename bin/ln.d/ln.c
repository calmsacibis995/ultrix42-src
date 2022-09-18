#ifndef lint
static	char	*sccsid = "@(#)ln.c	4.1	(ULTRIX)	7/2/90";
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

/*
 * ln.c
 *
 *	03-Nov-83	mah1: Correct message suppression.
 *
 *	17-Feb-84	mah. Change sccsid format to match
 *			ueg's sccsid format.
 *
 *	7-Apr-89 	chetal
 *			Added [-i] option to make it POSIX compatible.
 *			Changed -f option to mean force rather than 
 *			'suppress error messages'.
 *			Removed unnecessary 'goto' code.
 *			Added getopt().
 */

/*
 * ln
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>

#define NO	0
#define YES	1

struct	stat stb;
int	fflag;		/* force flag set? */
int	sflag;
int    	iflag;		/* Ask for confirmation if overwriting existing file */
char	name[BUFSIZ];
char	*rindex();
extern	int errno;
extern 	int optind;
extern  char *optarg;

main(argc, argv)
     int argc;
     register char **argv;
{
  register int i, r, c;

  while((c = getopt(argc, argv, "ifs")) != EOF) {
    switch(c) {
    case 'i': iflag++; break;
    case 'f': fflag++; break;
    case 's': sflag++; break;
    }
  }
  
  if (optind == argc)
    usage();
  else if (optind == (argc - 1)) {
    argv[argc] = ".";
    argc++;
  }
  
  if (sflag == 0 && ((argc - optind) > 2)) {
    if (stat(argv[argc-1], &stb) < 0)
      usage();
    if ((stb.st_mode&S_IFMT) != S_IFDIR) 
      usage();
  }
  r = 0;
  for(i = 0; i < argc-optind-1; i++)
    r |= linkit(argv[optind+i], argv[argc-1]);
  exit(r);
}



extern int	link(), symlink();

linkit(from, to)
     char *from, *to;
{
  char *tail;
  int (*linkf)() = sflag ? symlink : link;
  
  /* is target a directory? */
  if (sflag == 0 && stat(from, &stb) >= 0
      && (stb.st_mode&S_IFMT) == S_IFDIR) {
    printf("%s is a directory\n", from);
    return (1);
  }
  if ((stat(to, &stb) >= 0) && ((stb.st_mode&S_IFMT) == S_IFDIR)) {
    tail = rindex(from, '/');
    if (tail == 0)
      tail = from;
    else
      tail++;
    sprintf(name, "%s/%s", to, tail);
    to = name;
  }
  /* Check if it already exists, if -[if] flag given */
  if(access(to,F_OK) == 0) {
    if (fflag) {
      if (unlink(to) < 0) {
	perror(to);
	return(1);
      }
    }
    if (iflag && !fflag) {
      printf("\"%s\" already exists; do you want to overwrite this? [yn] (n)", to);
      if (query(NO) == NO)
	return(0);
      if (unlink(to) < 0) {
	perror(to);
	return(1);
      }
    }
  }
  if ((*linkf)(from, to) < 0) {
    if (errno == EEXIST)
      perror(to);
    else if (access(from, 0) < 0)
      perror(from);
    else
      perror(to);
    return (1);
  }
  return (0);
}


/*
 * usage message
 */
usage()
{
  fprintf(stderr, "Usage:\tln [ -fsi ] name1 [ name2 ]\nor:\tln [ -fsi ] name ...  directory\n");
  exit(1);
}

/*
 * Solicit response
 */
query(fallback)
int fallback;
{
  register int i, b;

  i = b = getchar();
  while( b != '\n' && b != EOF)
    b = getchar();
  if (i == '\n')
    return(fallback);
  switch(i) {
  case 'y':
  case 'Y':
    return(YES);
    break;
  case 'n':
  case 'N':
  default:
    return(NO);
    break;
  }
}


