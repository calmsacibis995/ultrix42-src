#ifdef ultrix
#ifdef lint
static char *sccsid = "@(#)lup_def.h	4.1	(ULTRIX)	7/2/90";
#endif lint
#endif ultrix

/***	
 ***	cps$lup_def.h --
 ***	
 ***	Miscellaneous preprocessor stuff for the Page Layup File Translator.
 ***	
 ***	N.Batchelder, 2/9/87.
 ***/

# include <stdio.h>
# include <setjmp.h>
# ifdef vms
# include ssdef
# include stsdef
# endif

# ifdef ultrix
# define SS$_NORMAL	1
# define STS$M_SUCCESS	1
# endif
# define private	static
# define public		/**/
# define fast		register
# define elif		else if
# define any		int

# define strequal(s,t)	(!strcmp(s,t))

/* 
 * Some nicer boolean stuff.
 */

typedef char	flag;

# define true	(0 == 0)
# define false	(0 == 1)

# ifdef strequal(a,b)
# undef strequal
# endif
# define strequal(a,b)	(!strcmp((a),(b)))

