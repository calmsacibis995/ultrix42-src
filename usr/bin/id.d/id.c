#ifndef LINT
static char *sccsid = "@(#)id.c	4.1	(ULTRIX)	7/17/90";
#endif LINT

/*
 * Modification History
 *
 *	April 4, 1989  Pradeep Chetal (chetal)
 *	  - Added [-gnru] options
 *	    Made posix compatible.
 */


#include <stdio.h>
#include <pwd.h>
#include <grp.h>

extern int optind;
extern char *optarg;

int gflag, uflag, rflag, nflag;
char *progName;

main(argc, argv)
int argc;
char **argv;
{
  int c, errflag = 0;
  int uid, gid, euid, egid;
  static char stdbuf[BUFSIZ];
  char *cp;
  extern char *rindex();
  struct passwd *getpwuid();
  struct group *getgrgid();

  setbuf (stdout, stdbuf);
  progName = (cp = rindex(argv[0],'/')) ? ++cp : argv[0];

  while ((c = getopt(argc,argv,"grnu")) != EOF) {
    switch(c) {
    case 'g': 
      uflag ? errflag++ : gflag++ ; break;
    case 'u': 
      gflag ?  errflag++ : uflag++; break;
    case 'n': 
      nflag++; break;
    case 'r': 
      rflag++; break;
    default:  
      usage();
    }
  }
  
  if (errflag)
    usage();

  /* -r and/or -n have to be used with one of -u *or* -g */
  if ((rflag || nflag) && !uflag && !gflag)
    usage();

  uid = getuid();
  gid = getgid();
  euid = geteuid();
  egid = getegid();

  setpwent();
  setgrent();
  
  if (!gflag && !uflag && !rflag && !nflag) {	/* no options, default */
    printf("uid=%d(%s)", uid, getpwuid(uid)->pw_name);
    printf(" gid=%d(%s)", gid, getgrgid(gid)->gr_name);
    if (uid != euid)
      printf(" euid=%d(%s)", euid, getpwuid(euid)->pw_name);
    if (gid != egid)
      printf(" egid=%d(%s)", egid, getgrgid(egid)->gr_name);
    putchar ('\n');
    exit(0);
  }
  
  /* Else print only what is asked for */
  if (nflag) {
    printf("%s\n", 
	 uflag ? (rflag ? getpwuid(uid)->pw_name : getpwuid(euid)->pw_name)
	       : (rflag ? getgrgid(gid)->gr_name : getgrgid(egid)->gr_name));
  } else {
    printf("%d\n",
	 uflag ? (rflag ? uid : euid)
	       : (rflag ? gid : egid));
  }
  exit(0);
}

/*
 * Usage error message
 */
usage()
{
  fprintf(stderr, "usage: %s [-{g|u}[rn]]\n", progName);
  exit(1);
}
