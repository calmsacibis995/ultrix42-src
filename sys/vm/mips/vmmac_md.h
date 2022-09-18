/* 	@(#)vmmac_md.h	4.1	(ULTRIX)	7/2/90 	*/

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 ************************************************************************/
/*
 *
 *   Modification History:
 *
 * 30-Nov-89 -- jmartin
 *	Support for coprocessor access to process virtual memory.
 *
 * 10 Feb 89 -- depp
 *	New file to contain machine dependent portion of /sys/h/vmmac.h
 *
 */


/*
 * Check SMS alignment macro
 *	MIPS -- should be page of PTEs aligned (4 Mb) and in KUSEG space
 */
#define	SM_CHKALIGN(addr,size) ( \
	(int)(addr) & (ctob(NPTEPG) - 1) || \
	!IS_KUSEG(addr) || \
	!IS_KUSEG((int)(addr) + (size)) \
)

#define SM_PSM_CLEAR(p) { \
       (p)->p_smbeg = (p)->p_smend = (p)->p_smsize = 0; \
       if ((p)->p_sm != (struct p_sm *) NULL){ \
		KM_FREE((p)->p_sm, KM_SHMSEG); \
		(p)->p_sm = (struct p_sm *) NULL; \
	} \
}

/*
 * For shared segments (both text and SYSTEM V shared memory):
 * Since the mips only has to account for a single global page table, 
 * and no local page tables, these routines are NULL.
 */
#define	sm_cpdirty(p,s) (p,s)
#define vinitsmpt(p,sp) (p,sp)
#define dirtysm(sp,smp) (sp,smp,0)
#define distsmpte(sp,smp,dpte,cm) (sp,smp,dpte,cm)
#define distpte(xp,tp,dpte) (xp,tp,dpte)

#define clear_dev_tlbs(segment, vpn, pagecount, type)			       \
((type)==CSYS || (pagecount)==1 || panic("clear_dev_tlbs")		       \
,(type)==CSYS								       \
	? (((struct proc *)(segment))->p_dev_VM_maint &&		       \
		(*((struct proc *)(segment))->p_dev_VM_maint)(PDEVCMD_ALL, 0)) \
	: ((type)==CDATA						       \
		? (((struct proc *)(segment))->p_dev_VM_maint &&	       \
			(*((struct proc *)(segment))->p_dev_VM_maint)	       \
				(PDEVCMD_ONE,				       \
				 dptov((struct proc *)(segment), (vpn))))      \
		: ((type)==CSMEM &&					       \
			sm_clear_dev_tlbs((struct smem *)(segment), (vpn))))   \
,1)

#define release_dev_VM_maint(p)						\
	((p)->p_dev_VM_maint && ((*(p)->p_dev_VM_maint)(PDEVCMD_ALL, 0)	\
				 ,(p)->p_dev_VM_maint = (int(*)())NULL))
