/* 	@(#)vmmac_md.h	4.2	(ULTRIX)	2/14/91 	*/

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 ************************************************************************/
/*
 *
 *   Modification History:
 *
 * 13-Feb-91 -- jmartin
 *	Define SM_PSM_UPDATE correctly for VAX.
 *
 * 10 Feb 88 -- depp
 *	New file to contain machine dependent portion of /sys/h/vmmac.h
 *
 */



/*
 * Check SYSTEM V SMS alignment macro
 *	VAX  -- should be page aligned (1024 byte) and in user space
 */
#define	SM_CHKALIGN(addr,size) ( \
	(int)(addr) & (ctob(CLSIZE) - 1) || \
	(int)(addr) & VA_SPACE || \
	((int)(addr) + (size)) & VA_SPACE \
)

#define SM_PSM_CLEAR(p) { \
       (p)->p_smbeg = (p)->p_smend = (p)->p_smsize = 0; \
       setp0lr(u.u_tsize + u.u_dsize); \
       if ((p)->p_sm != (struct p_sm *) NULL){ \
		KM_FREE((p)->p_sm, KM_SHMSEG); \
		(p)->p_sm = (struct p_sm *) NULL; \
	} \
}

#define SM_PSM_UPDATE(p) { \
	(p)->p_smbeg = (p)->p_sm[0].sm_saddr; \
	(p)->p_smend = (p)->p_sm[(p)->p_smcount - 1].sm_eaddr; \
	(p)->p_smsize = (p)->p_smend - (p)->p_tsize - (p)->p_dsize; \
}
