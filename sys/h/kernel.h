/*	@(#)kernel.h	4.2  (ULTRIX)        9/4/90     */

/*
 * Global variables for the kernel
 */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif


long	rmalloc();

/* 1.1 */
long	hostid;
char	hostname[32];
int	hostnamelen;
char	domainname[32];
int	domainnamelen;

/* 1.2 */
struct	timeval boottime;
struct	timeval time;
struct	timezone tz;			/* XXX */
int	hz;
int	phz;				/* alternate clock's frequency */
int	tick;
int	lbolt;				/* awoken once a second */
int	realitexpire();

#ifdef __vax
extern double	avenrun[];
#endif /* __vax */
#ifdef __mips
#include "../h/fixpoint.h"
extern fix	avenrun[];
#endif /* __mips */

#ifdef GPROF
extern	int profiling;
extern	char *s_lowpc;
extern	u_long s_textsize;
#ifdef __vax
extern	u_short *kcount;
#endif /* __vax */
#ifdef __mips
extern	unsigned int *kcount;
#endif /* __mips */
#endif /* GPROF */
