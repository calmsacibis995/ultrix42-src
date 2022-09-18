#ident "$Header: prom_entry.s,v 1.1 87/08/18 16:24:10 mdove Exp $"
/*	@(#)prom_entry.s	4.2	(ULTRIX)	10/9/90
 *
 * Copyright 1985 by MIPS Computer Systems, Inc.
 *
 * prom_entry.s -- interface to prom entry points
 *
 * Revision History:
 * 
 * Oct 09, 1990 -- Joe Szczypek
 * 	Added TURBOchannel ROM support.
 */

#include "../machine/mips/regdef.h"
#include "../machine/mips/asm.h"
#include "../machine/mips/cpu.h"
#include "../machine/mips/entrypt.h"

/*
 * Prom entry points
 */

/*
 * Return control to prom entry points
 *
 * RESET	transferred to on hardware reset, configures MIPS boards,
 *		runs diags, check for appropriate auto boot action in
 *		"bootmode" environment variable and performs that action.
 *
 * EXEC		called to utilize prom to boot new image.  After the booted
 *		program returns control can either be returned to the
 *		original caller of the exec routine or to the prom monitor.
 *		(to return to the original caller, the new program must
 *		not destroy any text, data, or stack of the parent.  the
 *		new programs stack continues on the parents stack.
 *
 * RESTART	re-enter the prom command parser, do not reset prom state
 *
 * REINIT	reinitialize prom state and re-enter the prom command parser
 *
 * REBOOT	check for appropriate bootmode and perform, no configuration
 *		or diags run
 *
LEAF(_prom_autoboot)
XLEAF(prom_autoboot)
	li	v0,PROM_AUTOBOOT
	j	v0
	END(_prom_autoboot)

LEAF(_prom_reset)
XLEAF(prom_reset)
	li	v0,PROM_RESET
	j	v0
	END(_prom_reset)

LEAF(_prom_exec)
XLEAF(prom_exec)
	li	v0,PROM_EXEC
	j	v0
	END(_prom_exec)
*/

LEAF(_prom_restart)
XLEAF(prom_restart)
	li	v0,PROM_RESTART
	j	v0
	END(_prom_restart)

/*
LEAF(_prom_reinit)
XLEAF(prom_reinit)
	li	v0,PROM_REINIT
	j	v0
	END(_prom_reinit)

LEAF(_prom_reboot)
XLEAF(prom_reboot)
	li	v0,PROM_REBOOT
	j	v0
	END(_prom_reboot)
 */
/*
 * these routines access prom "stdio" routines, and may be used
 * by standalone programs that would like to use prom io redirection
 */
LEAF(_prom_lseek)
	li	v0,PROM_LSEEK
	j	v0
	END(_prom_lseek)

/*
LEAF(_prom_getchar)
	li	v0,PROM_GETCHAR
	j	v0
	END(_prom_getchar)

LEAF(_prom_putchar)
	li	v0,PROM_PUTCHAR
	j	v0
	END(_prom_putchar)
*/

LEAF(_prom_gets)
	li	v0,PROM_GETS
	j	v0
	END(_prom_gets)

/*
LEAF(_prom_puts)
	li	v0,PROM_PUTS
	j	v0
	END(_prom_puts)
*/

LEAF(_prom_printf)
	li	v0,REX_MAGIC
	lw	v1,rex_magicid
	beq	v0,v1,1f
	li	v0,PROM_PRINTF
	j	v0
1:	j	rex_printf
	END(_prom_printf)

LEAF(prom_getenv)
	li	v0,PROM_GETENV
	j	v0
	END(prom_getenv)

LEAF(prom_setenv)
	li	v0,PROM_SETENV
	j	v0
	END(prom_setenv)

/*
 * read-modify-write routine use special cpu board circuitry to accomplish
 * vme bus r-m-w cycles.  all routines are similar to:
 *	unsigned char
 *	orb_rmw(addr, mask)
 *	unsigned char *addr;
 *	unsigned mask;
 *	{
 *		register unsigned rval;
 *
 *		lockbus();
 *		rval = *addr;
 *		*addr = rval & mask;
 *		unlockbus();
 *		return(rval);
 *	}
LEAF(orw_rmw)
	li	v0,PROM_ORW_RMW
	j	v0
	END(orw_rmw)

LEAF(orh_rmw)
	li	v0,PROM_ORH_RMW
	j	v0
	END(orh_rmw)

LEAF(orb_rmw)
	li	v0,PROM_ORB_RMW
	j	v0
	END(orb_rmw)

LEAF(andw_rmw)
	li	v0,PROM_ANDW_RMW
	j	v0
	END(andw_rmw)

LEAF(andh_rmw)
	li	v0,PROM_ANDH_RMW
	j	v0
	END(andh_rmw)

LEAF(andb_rmw)
	li	v0,PROM_ANDB_RMW
	j	v0
	END(andb_rmw)
 */

/*
 * prom saio entry points
 * (mainly for implementing stdin/stdout for standalones)
 */
LEAF(_prom_open)
	li	v0,PROM_OPEN
	j	v0
	END(_prom_open)

LEAF(_prom_read)
	li	v0,PROM_READ
	j	v0
	END(_prom_read)

LEAF(_prom_write)
	li	v0,PROM_WRITE
	j	v0
	END(_prom_write)

LEAF(_prom_ioctl)
	li	v0,PROM_IOCTL
	j	v0
	END(_prom_ioctl)

LEAF(_prom_close)
	li	v0,PROM_CLOSE
	j	v0
	END(_prom_close)

LEAF(_prom_startcvax)
	li	v0,PROM_STARTCVAX
	j	v0
	END(_prom_startcvax)

/*
 * REX global variables
 */
	BSS(rex_base,4)			# REX base address
	BSS(rex_magicid,4)		# REX magic number

/*
 * ROM Executive Program (REX) callbacks
 *
 * These are presently used only with 3MAX.  They are currently TURBOchannel
 * specific.
 */

LEAF(rex_memcpy)
	lw	v0,rex_base
	lw	v0,REX_MEMCPY(v0)
	j	v0
	END(rex_memcpy)

/*
LEAF(rex_memset)
	lw	v0,rex_base
	lw	v0,REX_MEMSET(v0)
	j	v0
	END(rex_memset)

LEAF(rex_strcat)
	lw	v0,rex_base
	lw	v0,REX_STRCAT(v0)
	j	v0
	END(rex_strcat)

LEAF(rex_strcmp)
	lw	v0,rex_base
	lw	v0,REX_STRCMP(v0)
	j	v0
	END(rex_strcmp)

LEAF(rex_strlen)
	lw	v0,rex_base
	lw	v0,REX_STRLEN(v0)
	j	v0
	END(rex_strlen)

LEAF(rex_strncat)
	lw	v0,rex_base
	lw	v0,REX_STRNCAT(v0)
	j	v0
	END(rex_strncat)

LEAF(rex_strncpy)
	lw	v0,rex_base
	lw	v0,REX_STRNCPY(v0)
	j	v0
	END(rex_strncpy)

LEAF(rex_strncmy)
	lw	v0,rex_base
	lw	v0,REX_STRNCMY(v0)
	j	v0
	END(rex_strncmy)

LEAF(rex_getchar)
	lw	v0,rex_base
	lw	v0,REX_GETCHAR(v0)
	j	v0
	END(rex_getchar)
*/
LEAF(rex_gets)
	lw	v0,rex_base
	lw	v0,REX_GETS(v0)
	j	v0
	END(rex_gets)

LEAF(rex_puts)
	lw	v0,rex_base
	lw	v0,REX_PUTS(v0)
	j	v0
	END(rex_puts)

LEAF(rex_printf)
	lw	v0,rex_base
	lw	v0,REX_PRINTF(v0)
	j	v0
	END(rex_printf)
/*
LEAF(rex_sprintf)
	lw	v0,rex_base
	lw	v0,REX_SPRINTF(v0)
	j	v0
	END(rex_sprintf)

LEAF(rex_io_poll)
	lw	v0,rex_base
	lw	v0,REX_IO_POLL(v0)
	j	v0
	END(rex_io_poll)

LEAF(rex_strtol)
	lw	v0,rex_base
	lw	v0,REX_STRTOL(v0)
	j	v0
	END(rex_strtol)

LEAF(rex_signal)
	lw	v0,rex_base
	lw	v0,REX_SIGNAL(v0)
	j	v0
	END(rex_signal)

LEAF(rex_raise)
	lw	v0,rex_base
	lw	v0,REX_RAISE(v0)
	j	v0
	END(rex_raise)

LEAF(rex_time)
	lw	v0,rex_base
	lw	v0,REX_TIME(v0)
	j	v0
	END(rex_time)

LEAF(rex_setjump)
	lw	v0,rex_base
	lw	v0,REX_SETJUMP(v0)
	j	v0
	END(rex_setjump)

LEAF(rex_longjump)
	lw	v0,rex_base
	lw	v0,REX_LONGJUMP(v0)
	j	v0
	END(rex_longjump)
*/
LEAF(rex_bootinit)
	lw	v0,rex_base
	lw	v0,REX_BOOTINIT(v0)
	j	v0
	END(rex_bootinit)

LEAF(rex_bootread)
	lw	v0,rex_base
	lw	v0,REX_BOOTREAD(v0)
	j	v0
	END(rex_bootread)

LEAF(rex_bootwrite)
	lw	v0,rex_base
	lw	v0,REX_BOOTWRITE(v0)
	j	v0
	END(rex_bootwrite)
/*
LEAF(rex_setenv)
	lw	v0,rex_base
	lw	v0,REX_SETENV(v0)
	j	v0
	END(rex_setenv)
*/
LEAF(rex_getenv)
	lw	v0,rex_base
	lw	v0,REX_GETENV(v0)
	j	v0
	END(rex_getenv)
/*
LEAF(rex_unsetenv)
	lw	v0,rex_base
	lw	v0,REX_UNSETENV(v0)
	j	v0
	END(rex_unsetenv)

LEAF(rex_slot_address)
	lw	v0,rex_base
	lw	v0,REX_SLOT_ADDRESS(v0)
	j	v0
	END(rex_slot_address)

LEAF(rex_wbflush)
	lw	v0,rex_base
	lw	v0,REX_WBFLUSH(v0)
	j	v0
	END(rex_wbflush)

LEAF(rex_msdelay)
	lw	v0,rex_base
	lw	v0,REX_MSDELAY(v0)
	j	v0
	END(rex_msdelay)

LEAF(rex_leds)
	lw	v0,rex_base
	lw	v0,REX_LEDS(v0)
	j	v0
	END(rex_leds)

LEAF(rex_clear_cache)
	lw	v0,rex_base
	lw	v0,REX_CLEAR_CACHE(v0)
	j	v0
	END(rex_clear_cache)
*/
LEAF(rex_getsystype)
	lw	v0,rex_base
	lw	v0,REX_GETSYSTYPE(v0)
	j	v0
	END(rex_getsysid)

LEAF(rex_getbitmap)
	lw	v0,rex_base
	lw	v0,REX_GETBITMAP(v0)
	j	v0
	END(rex_getbitmap)
/*
LEAF(rex_disableintr)
	lw	v0,rex_base
	lw	v0,REX_DISABLEINTR(v0)
	j	v0
	END(rex_disableintr)

LEAF(rex_enableintr)
	lw	v0,rex_base
	lw	v0,REX_ENABLEINTR(v0)
	j	v0
	END(rex_enableintr)

LEAF(rex_testintr)
	lw	v0,rex_base
	lw	v0,REX_TESTINTR(v0)
	j	v0
	END(rex_testintr)

LEAF(rex_console_init)
	lw	v0,rex_base
	lw	v0,REX_CONSOLE_INIT(v0)
	j	v0
	END(rex_console_init)
*/
LEAF(rex_halt)
	lw	v0,rex_base
	lw	v0,REX_HALT(v0)
	j	v0
	END(rex_halt)
/*
LEAF(rex_showfault)
	lw	v0,rex_base
	lw	v0,REX_SHOWFAULT(v0)
	j	v0
	END(rex_showfault)

LEAF(rex_gettcinfo)
	lw	v0,rex_base
	lw	v0,REX_GETTCINFO(v0)
	j	v0
	END(rex_gettcinfo)
*/
LEAF(rex_execute_cmd)
	lw	v0,rex_base
	lw	v0,REX_EXECUTE_CMD(v0)
	j	v0
	END(rex_execute_cmd)

LEAF(rex_rex)
	lw	v0,rex_base
	lw	v0,REX_REX(v0)
	j	v0
	END(rex_rex)

