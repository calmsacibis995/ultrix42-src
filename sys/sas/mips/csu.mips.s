#ident "$Header: csu.mips.s,v 1.1 87/08/18 16:23:37 mdove Exp $"
/*		4.2	csu.mips.s
 *
 * Copyright 1985 by MIPS Computer Systems, Inc.
 *
 * csu.s -- standalone io library startup code
 */

/*
 * Revision History:
 *
 * jas - commented out reloading of a2.  environ is never loaded, so
 *       loading a2 with environ results in a2 being loaded with trash.
 */

#include "../../machine/mips/regdef.h"
#include "../../machine/mips/cpu.h"
#include "../../machine/mips/asm.h"
#include "../../machine/mips/entrypt.h"
#include "setjmp.h"
#include "saio.h"

	.text

STARTFRM=	EXSTKSZ			# leave room for fault stack
NESTED(start, STARTFRM, zero)
	la	gp,_gp
	subu	v0,sp,4*4		# leave room for argsaves
/*
	sw	v0,_fault_sp		# small stack for fault handling
*/
	bne	a3,zero,1f		# no return
	la	t0,jb
	sw	ra,JB_PC*4(t0)		# ra and sp to get back to exec'er
	sw	sp,JB_SP*4(t0)
1:
	subu	sp,STARTFRM		# fault stack can grow to here + 16
	sw	zero,STARTFRM-4(sp)	# keep debuggers happy
	sw	a0,STARTFRM(sp)		# home args
	sw	a1,STARTFRM+4(sp)
	sw	a3,retflag		# return or exit flag
	lw	a0,STARTFRM+4(sp)	# copy strings out of prom area
	lw	a0,STARTFRM(sp)		# reload argc, argv, environ
	lw	a1,STARTFRM+4(sp)
/*	lw	a2,environ */		# don't do, it trashes a2.
	jal	main
	lw	v1,retflag
	beq	v1,zero,return
	move	v0,a0
	jal	_exit
	END(start)

LEAF(_exit)
	lw	v1,retflag
	bne	v1,zero,promexit
	move	v0,a0
return:
	la	t0,jb
	lw	ra,JB_PC*4(t0)
	lw	sp,JB_SP*4(t0)
	j	ra

promexit:
	li	ra,+PROM_RESTART
	j	ra
	END(_exit)

	BSS(environ,4)			# environment pointer
	LBSS(retflag,4)			# return or exit flag
	BSS(jb,JB_SIZE*4)		# return jump_buf
