#ifndef lint
static	char	*sccsid = "@(#)licheck.c	4.1	(ULTRIX)	7/2/90";
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
#include <stdio.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
char	*user = "root";
extern char	**environ;
struct	passwd *pwd,*getpwnam();
char	*crypt();
char	*getpass();
char	*getenv();

main()
{
FILE *console;
char *password;
/*******************	Print out licensing warning *********************/
prtmsg();
	pwd=getpwnam(user);
	setpriority(PRIO_PROCESS, 0, -2);
	password = getpass("To continue please enter the superuser password:");
	if (strcmp(pwd->pw_passwd, crypt(password, pwd->pw_passwd)) != 0) 
		{
		fprintf(stderr, "Sorry\n");
		console = fopen("/dev/console", "w");
		if (console != NULL) 
			{
			fprintf(console, "NETINSTALL BADSU: %s %s\r\n",getlogin(),ttyname(2));
			fclose(console);
			}
		exit(2);
		}
	exit(0);
}
prtmsg()
{
fprintf(stdout,"\007\007\007\n\007\007\007\n\007\007\007\n");
fprintf(stdout,"\t**************************************************************\n");
fprintf(stdout,"\t***********            LICENSING NOTICE              *********\n");
fprintf(stdout,"\t***********                  FROM                    *********\n");
fprintf(stdout,"\t***********       DIGITAL EQUIPMENT CORPORATION      *********\n\n");
fprintf(stdout,"\tDigital Equipment Corporation and most software vendors\n");
fprintf(stdout,"\tdistribute software products under Software License Agreements\n");
fprintf(stdout,"\twhich govern the installation and use of their software.\n\n");
fprintf(stdout,"\tYOU are responsible for ensuring that the correct Software\n");
fprintf(stdout,"\tLicenses have been obtained BEFORE completing this procedure.\n\n");
fprintf(stdout,"\tDescriptions of License Options available for specific Digital\n");
fprintf(stdout,"\tEquipment Corporation products may be found in the applicable\n");
fprintf(stdout,"\tSoftware Product Descriptions (SPD).  \n\n");
fprintf(stdout,"\tShould you have questions concerning any of the above, please\n");
fprintf(stdout,"\tcontact your local Digital Equipment Representative, or call\n");
fprintf(stdout,"\t1-800-DIGITAL for assistance. \n\n");
}

