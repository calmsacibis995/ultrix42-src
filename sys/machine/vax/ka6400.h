/*
 * @(#)ka6400.h	4.2  (ULTRIX)        9/6/90    
 */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/***********************************************************************
 *
 * Modification History:
 *
 * 4-Sep-1990	dlh
 *	added equates for vector processor support
 *
 * 06-Jun-1990	Pete Keilty
 *	Modified xrp_reg padding to 17k same as xmi_reg, because CIKMF
 *	mode space is 17k.
 *
 * 28-Mar-1990  Joe Szczypek
 *      Modified xrp_reg to include new XMP registers.  Added these here
 *      to minimize modifications (particularly new routines/files) needed
 *      to support XMP.  Also added fdal_regs structure to map private space
 *      fdal registers.
 *
 * 08-Dec-1989	Pete Keilty
 *	Modified xrp_reg padding to 16k same as xmi_reg, because CIXCD
 *	mode space is 16k.
 *
 * 7-Sep-88	Tom Kong
 *		Created this file from ka6200.h
 *
 **********************************************************************/

/* starting physical address of XMI node space*/
#define	XMI_START_PHYS 	0x21800000
#define RSSCADDR 0x20140000 	/* Phy addr of SSCBAR (in XMI private space)*/
#define RSSCSIZE 0x200*3	/* Size of the block in pages */

#define MDAADDR 0x21000000
#define MDASIZE 0x200           /* Size of the block in pages */

/*
 * ka6400 processor registers.
 */
struct xrp_reg {
	unsigned int xrp_dtype;
	unsigned int xrp_xbe;
	unsigned int xrp_fadr;
	unsigned int xrp_gpr;
	unsigned int xrp_rcsr; 
	char         xrp_pad1[0x8];
	unsigned int xmp_nscsr0; /* New for Mariah */
        char	     xrp_pad2[0x4]; 
	unsigned int xmp_xcr0;   /* New for Mariah */
	char         xrp_pad3[0x4];
	unsigned int xmp_xfaer0; /* New for Mariah */
	char         xrp_pad4[0x4];
	unsigned int xmp_xbeer0; /* New for Mariah */
	char         xrp_pad5[0x8];
	unsigned int xmp_wfadr0; /* New for Mariah */
	unsigned int xmp_wfadr1; /* New for Mariah */
	char	xrp_pad[17336];
};


/*
 * XMI private space, contains registers for Ka6400
 */
struct rssc_regs {
long	s_sscbar;	/* RSSC base addr reg(SSCBAR) in 0x20140000	*/
long	pad1[3];
long	s_ssccnr;	/* Config reg (SSCCNR) in 0x20140010		*/
long	pad2[3];
long	s_sscbtr;	/* Bus timeout ctrl reg (SSCBTR) in 0x20140020	*/
long	pad3[3];
long	s_oport;	/* Output port (OPORT) in 0x20140030		*/
long	pad4[3];
long	s_iport;	/* Input port (IPORT) in 0x20140040		*/
long	pad5[59];
long	s_crbadr;	/* CREG base adr reg (CRBADR) in 0x20140130	*/
long	s_cradmr;	/* CREG address decode mask reg in 0x20140134	*/
long	pad6[2];
long	s_eebadr;	/* EEPROM base address reg in 0x20140140	*/
long	s_eeadmr;	/* EEPROM address decode mask reg in 0x20140144	*/
long	pad7[6];
long	s_tcr0;		/* Timer 0 control reg		0x20140160	*/
long	s_tir0;		/* Timer 0 interval reg		0x20140164	*/
long	s_tnir0;	/* Timer 0 next interval reg	0x20140168	*/
long	s_tivr0;	/* Timer 0 interrupt vector reg	0x2014016c	*/
long	s_tcr1;		/* Timer 1 control reg		0x20140170	*/
long	s_tir1;		/* Timer 1 interval reg		0x20140174	*/
long	s_tnir1;	/* Timer 1 next interval reg	0x20140178	*/
long	s_tivr1;	/* Timer 1 interrupt vector reg	0x2014017c	*/
long	pad8[0x7c];	
long    s_sscicr;       /* MSSC (XMP) Interval Counter reg  0x201401f8  */
};

struct fdal_regs {
long    fdal0;          /* Failing DAL Register 0       0x21000020      */
long    pad[0x1];
long    fdal1;          /* Failing DAL Register 1       0x21000028      */ 
long    pad1[0x1];
long    fdal2;          /* Failing DAL Register 2       0x21000030      */ 
long    pad2[0x1];
long    fdal3;          /* Failing DAL Register 3       0x21000038      */
};

extern	struct pte RSSCmap[];		/* declared in spt.s	*/
extern	struct rssc_regs rssc[1];	/* declared in spt.s	*/
extern  struct pte MDAmap[];       	/* declared in spt.s    */
extern  struct fdal_regs mda[1];	/* declared in spt.s    */


/*
 * bit definitions of the VINTSR (Vector Interface Error Status 
 * Register) of the Rigel
 * (these definitions were extracted from the XRP (Rigel) CPU Engineering 
 *  Specification REV 1.0 dated 21Dec87)
 */

#define	VINTSR_VECTOR_UNIT_PRESENT	0x001	/* Read-only bit; set if     */
						/* there is a vector module  */
						/* present                   */
#define	VINTSR_VECTOR_UNIT_SERR		0x002	/* Write-one-to-clear bit;   */
						/* set if there is a vector  */
						/* unit soft error           */
#define	VINTSR_VECTOR_UNIT_HERR		0x004	/* Write-one-to-clear bit;   */
						/* set if there is a vector  */
						/* unit hard error           */
#define	VINTSR_VECTL_VIB_SERR		0x008	/* Write-one-to-clear bit;   */
						/* set if there is a VIB     */
						/* (Scalar to Vector         */
						/* Interface Bus) soft error */
#define	VINTSR_VECTL_VIB_HERR		0x010	/* Write-one-to-clear bit;   */
						/* set if there is a VIB     */
						/* hard error                */
#define	VINTSR_CCHIP_VIB_SERR		0x020	/* Write-one-to-clear bit;   */
						/* set if there is a VIB     */
						/* soft error detected by    */
						/* the c-chip                */
#define	VINTSR_CCHIP_VIB_HERR		0x040	/* Write-one-to-clear bit;   */
						/* set if there is a VIB     */
						/* hard error detected by    */
						/* the c-chip                */
#define	VINTSR_BUS_TIMEOUT		0x080	/* Write-one-to-clear bit;   */
						/* set if the c-chip detects */
						/* a bus timeout             */
#define	VINTSR_VECTOR_MODULE_RESET	0x100	/* Read-write by user bit;   */
						/* set to request a vector   */
						/* module reset              */
#define	VINTSR_DISABLE_VECT_INTF	0x200	/* Read-write by user bit;   */
						/* set to diable the vector  */
						/* interface functions of    */
						/* the c-chip                */


/*
 * bit definitions of the SSCBTR (RSSC Bus Timeout Control Register) 
 * of the Rigel
 * (these definitions were extracted from the XRP (Rigel) CPU Engineering 
 *  Specification REV 1.0 dated 21Dec87)
 */
#define	SSCBTR_BTO	0x80000000	/* Write-one-to-clear bit; set by    */
					/* the RSSC to indicate that a CPU   */
					/* transaction has timed out         */
