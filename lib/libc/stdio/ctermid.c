#ifndef lint
static	char	*sccsid = "@(#)ctermid.c	4.1	(ULTRIX)	7/3/90";
#endif

/*	Modification History						
 *									
 *	001 - Mark A. Parenti						
 *		Add call to getsysinfo() to get controlling terminal device.
 *		If no controlling terminal and in POSIX mode, return NULL
 *		string.
 *
 *	002 - Mark A. Parenti
 *		Change progenv to short because kernel only copyout's a short.
 *
 */
/*LINTLIBRARY*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/exec.h>

extern char *strcpy();
static char res[L_ctermid];

char *
ctermid(s)
register char *s;
{
short	progenv;
dev_t	t_dev;

	if( getsysinfo(GSI_PROG_ENV, &progenv, sizeof(progenv), 0, 0, 0) < 1 )
		progenv = A_BSD;
	if( (getsysinfo(GSI_TTYP, &t_dev, sizeof(dev_t), 0, 0, 0) < 1) && 
			(progenv == A_POSIX) ){
		return (strcpy(s != NULL ? s : res, ""));
	}
	else {
		return (strcpy(s != NULL ? s : res, "/dev/tty"));
	}
}
