#ifndef lint
static	char	*sccsid = "@(#)mkmachine.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/*
 * creates a shell script that echos the C preprocessor name mips or vax
 * (so far). This allows any machine to run the shell and find out what the
 * target is!!
 */

main()
{
	char *s1 = "#!/bin/sh\n";
	char *s2 = "echo mips\n";
	char *s3 = "echo vax\n";
	char *s4 = "exit 0\n";
	write(1,s1,strlen(s1));
#ifdef mips
	write(1,s2,strlen(s2));
#endif mips
#ifdef vax
	write(1,s3,strlen(s3));
#endif vax
	write(1,s4,strlen(s4));
	exit(0);
}
