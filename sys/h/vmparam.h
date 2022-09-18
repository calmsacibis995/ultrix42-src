/*	@(#)vmparam.h	4.2  (ULTRIX)        9/4/90     */

/*
 * Machine dependent constants
 */

#ifdef KERNEL
#include "../h/ansi_compat.h"
#include "../machine/vmparam.h"
#else
#include <ansi_compat.h>
#include <machine/vmparam.h>
#endif

#ifdef __vax
#if defined(KERNEL) && !defined(LOCORE)
int	klseql;
int	klsdist;
int	klin;
int	kltxt;
int	klout;
#endif /* KERNEL && !LOCORE */
#endif /* __vax */

#ifdef __mips
#if defined(KERNEL) && defined(__LANGUAGE_C)
extern int	klseql;
extern int	klsdist;
extern int	klin;
extern int	kltxt;
extern int	klout;
#endif /* KERNEL && !LOCORE */
#endif /* __mips */
