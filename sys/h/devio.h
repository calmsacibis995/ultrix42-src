#ifndef	DEVIO_INCLUDE
#define	DEVIO_INCLUDE	1
/*
 * @(#)devio.h	4.10	(ULTRIX)	1/25/91
 */

/************************************************************************
 *									*
 *			Copyright (c) 1986-1989 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * devio.h
 *
 * Modification history
 *
 * Common structures and definitions for device drivers and ioctl
 *
 * 14-Jan-91 - Brian Nadeau
 *	Added DEV_TA91
 *
 * 08-Jan-91 - Robin Miller
 *	Added density codes for TLZ04 (RDAT) and TZK08 (Exabyte).
 *
 * 07-Jan-91 - Brian Nadeau
 *	Change name of the RAH72 to RA71
 *
 * 02-Nov-90 - Brian Nadeau
 *	Added DEV_RA72, DEV_RAH72
 *
 * 11-Sep-90 - Robin Miller
 *	Added DEV_3_ED2S definition for RX26 Extra Density 2.88MB diskette.
 *	Also added DEV_5_DD2S for RX33 Double Density 2 sided diskette.
 *
 * 30-Jul-90 - Robin Miller
 *	Added DEV_RRD42, DEV_RX26, and DEV_RZ25 defines.
 *
 * 16-Jul-90  - Janet Schank
 *      Added DEV_RZ23L define.
 *
 * 05-Jul-90 - Pete Keilty
 *	Added two new bus types DEV_BICI & DEV_XMICI.
 *	This defines bus and interconnect for booting.
 *
 * 20-May-90 - Bill Dallas
 *	Added new densities and QIC devices
 *
 * 03-May-90  - Paul Grist
 *      Added define for mdc driver (for Tim).
 *
 * 23-Jan-90  - Janet Schank
 *      Added define for TLZ04 (RDAT) device.
 *
 * 15-Dec-89  - Alan Frechette
 *	Added defines for the different graphic console devices.
 *
 * 30-Nov-89  - Tim Burke
 *	Added DEV_TF70L, DEV_RF73, DEV_RFH31, DEV_RFH72, DEV_RF73 and
 *	DEV_RFH73  definitions.
 *
 * 17-Oct-89  - Janet Schank / Art Zemon
 *	Added support for the TZ05, RZ24, and RZ57.
 *
 * 17-Oct-89 - Tim Burke
 * 	Added data structure devgeom which is used in the DEVGETGEOM ioctl.
 *	Added 3 new tape densities: DEV_38000_CP, DEV_76000BPI, DEV_76000_CP.
 *
 * 27-Sep-89	Debby Haeck
 *	merged PU and ISIS pools
 *
 * 22-Sep-89 - Janet L. Schank
 *      Added defines for DEV_RZxx, DEV_TZxx, and removed TZ88.
 *
 * 17-Aug-1989		David E. Eiche		DEE0074
 *	Change the name of the HSX50 (previously known as the BSA, HSB50,
 *	and KSB50) to be KDM70 to conform to the current MSCP specification.
 *	Add a definition for the XMI bus.
 *
 * 20-Jun-89 - Fred Canter
 *	Added DEV_RZ56.
 *
 * 18-Jun-89 - Fred Canter
 *	Added RZxx and TZxx defines for unknown SCSI disk/tape.
 *	Removed TZ88.
 *
 * 14-Jun-89 - Tim Burke
 *	Changed the name TQL70 to TQK7L and TBL70 to TBK7L to conform to 
 *	name change.  Added DEV_LOADER to be used in the tape categoty_stat
 *	field if a media loader is present.
 *
 * 11-Jun-89 - Fred Canter
 *	Added media changed and density information to devget structure.
 *	So softpc application can make better use of the floppy drive.
 *
 * 23-May-89 - Tim Burke
 *	Added DEV_38000BPI to represent the TA90 tape density.
 *
 * 05-May-89 - Tim Burke
 *	Merged 3.1 changes into pu and isis pools.
 *
 * 21-Feb-89 - Tim Burke
 *	Add TQL70, TBL70, TF85, HSX50, and HSC40 string definitions.
 *
 * 18-Nov-88 - Darrell Dunnuck (for Tim Burke)
 *	Added Firefox fc.c driver support.
 *
 * 17-Sep-88 - Ricky Palmer
 *	Updated PMAX SCSI support.
 *	
 * 30-Aug-1988		David E. Eiche		DEE0055
 *	Change name of HSB50 to be KSB50 to match current MSCP
 *	specification.  Add RF72 and KRQ50 string definitions.
 *
 * 01-Aug-88 - Ricky Palmer
 *	Added PMAX SCSI support.
 *
 * 27-Jul-1988		David E. Eiche		DEE0048
 *	Added DEV_UNKBUS for devices whose bus type cannot
 *	be determined.
 *
 * 17-Jul-1988		David E. Eiche		DEE0045
 *	Added strings for KRU50 and TBK50 interfaces and RF31, TF30
 *	and TF70 devices.  Also sorted interface and device name
 *	definitions to make maintenance easier.
 *
 * 14-Jul-88 - George Mathew
 *	Added RX23 name string
 *
 * 20-Jun-88 - Fred Canter
 *	Added RZ55 name string.
 *
 * 19-May-88 - Fred Canter
 *	Added SCSI controller and device names.
 *
 * 17-Apr-88 - Ricky Palmer
 *
 *	Added DEV_DSSC and DEV_MSI for MSI support.
 *
 * 20-Feb-88 - Tim Burke
 *
 *      Added DEV_DHB32, a 16 line BI bus terminal mux.
 *
 * 28-Sep-87 - Ricky Palmer
 *
 *	Added new "rctlr_num" field to "devget" structure.
 *
 * 17-May-87 - Ricky Palmer
 *
 *	Updated field for tu78/ta78.
 *
 * 19-Mar-87 -- Fred Canter
 *
 *	Added DEV_XOS for X in kernel special device.
 *
 * 10-Mar-87 - rsp (Ricky Palmer)
 *
 *	Added defines for cx series of controllers, updated dh defines.
 *
 * 14-Jan-87 - Robin
 *
 *	Added rqdx4, rd35
 *
 *  6-Jan-87 -	Fred Canter
 *
 *	Minor changes to some comments.
 *
 *  4-Mar-86 -	Ricky Palmer
 *
 *	Created original file and its contents. V2.0
 *
 * 13-Jun-86 - Jim Woodward
 *
 *	Fix to uba reset and drivers.
 *
 * 11-Jul-86 - Ricky Palmer
 *
 *	Added adpt, nexus fields to basic devget structure. V2.0
 *
 *  5-Aug-86 - Fred Canter
 *
 *	Added defines needed for devioctl support in VAXstation
 *	2000 device drivers.
 *	Changed RD3X to RD32.
 *
 * 6-Aug-86 - Robin Lewis
 *
 *	Added tape density for tk70 and added device entries for
 *	tk70, ra70, ra90, rv80, tu82
 *
 * 7-Aug-86 - Ricky Palmer
 *
 *	Added defines for VT3?? series of terminals. V2.0
 *
 * 27-Aug-86 -- Fred Canter
 *	Bug fix: removed the comma after DEV_MOUSE and DEV_TABLET.
 */

/* Basic amount of storage for "interface" and "device" below */
#define DEV_SIZE	0x08		/* Eight bytes			*/

/* DEV_UGH uprintf macro for driver backward compatibility */
#define DEV_UGH(x,y,z)	uprintf("%s: unit# %d: %s\n",x,y,z)

/* Structure for DEVIOCGET ioctl - device get status command */
struct	devget	{
	short	category;		/* Category			*/
	short	bus;			/* Bus				*/
	char	interface[DEV_SIZE];	/* Interface (string)		*/
	char	device[DEV_SIZE];	/* Device (string)		*/
	short	adpt_num;		/* Adapter number		*/
	short	nexus_num;		/* Nexus or node on adapter no. */
	short	bus_num;		/* Bus number			*/
	short	ctlr_num;		/* Controller number		*/
	short	rctlr_num;		/* Remote controller number	*/
	short	slave_num;		/* Plug or line number		*/
	char	dev_name[DEV_SIZE];	/* Ultrix device pneumonic	*/
	short	unit_num;		/* Ultrix device unit number	*/
	unsigned soft_count;		/* Driver soft error count	*/
	unsigned hard_count;		/* Driver hard error count	*/
	long	stat;			/* Generic status mask		*/
	long	category_stat;		/* Category specific mask	*/
};

/* Get status definitions for category word (category) */
#define DEV_TAPE	0x00		/* Tape category		*/
#define DEV_DISK	0x01		/* Disk category		*/
#define DEV_TERMINAL	0x02		/* Terminal category		*/
#define DEV_PRINTER	0x03		/* Printer category		*/
#define DEV_SPECIAL	0x04		/* Special category		*/

/* Get status definitions for bus word (bus) */
#define DEV_UB		0x00		/* Unibus bus			*/
#define DEV_QB		0x01		/* Qbus bus			*/
#define DEV_MB		0x02		/* Massbus bus			*/
#define DEV_BI		0x03		/* BI bus			*/
#define DEV_CI		0x04		/* CI bus			*/
#define DEV_NB		0x05		/* No Bus (single board VAX CPU)*/
#define DEV_MSI		0x06		/* MSI bus			*/
#define DEV_SCSI        0x07            /* SCSI bus                     */
#define DEV_XMI		0x08		/* XMI bus			*/
#define DEV_BICI	0x09		/* CI on BI bus			*/
#define DEV_XMICI	0x0A		/* CI on XMI bus		*/
#define	DEV_UNKBUS	0xff		/* Unknown bus type		*/

/* Definition for any unsupported/unknown interface or device */
#define DEV_UNKNOWN	"UNKNOWN"	/* Unknown interface/device	*/

/* Definitions for interface character array (interface) */
#define DEV_AIO 	"AIO"		/* AIO disk controller		*/
#define DEV_CXAB16	"CXAB16"	/* CX(AB)16 terminal mux.	*/
#define DEV_DEBNT	"DEBNT" 	/* DEBNT network/tape controller*/
#define DEV_DHB32	"DHB32"		/* DHB32 terminal mux.          */
#define DEV_DHQVCXY	"DHQVCXY"	/* DH(QV)11/CXY08 terminal mux. */
#define DEV_DHU11	"DHU11" 	/* DHU11 terminal mux.		*/
#define DEV_DMB32	"DMB32" 	/* DMB32 terminal mux.		*/
#define DEV_DMF32	"DMF32" 	/* DMF32 terminal mux.		*/
#define DEV_DMZ32	"DMZ32" 	/* DMZ32 terminal mux.		*/
#define DEV_DSSC	"DSSC"		/* DSSC MSI controller		*/
#define DEV_DZ11	"DZ11"		/* DZ11 terminal mux.		*/
#define DEV_DZ32	"DZ32"		/* DZ32 terminal mux.		*/
#define DEV_DZQ11	"DZQ11" 	/* DZQ11 terminal mux.		*/
#define DEV_DZV11	"DZV11" 	/* DZV11 terminal mux.		*/
#define DEV_HSC40	"HSC40" 	/* HSC40 intelligent controller */
#define DEV_HSC50	"HSC50" 	/* HSC50 intelligent controller */
#define DEV_HSC60	"HSC60" 	/* HSC60 intelligent controller */
#define DEV_HSC70	"HSC70" 	/* HSC70 intelligent controller */
#define DEV_HSC90	"HSC90" 	/* HSC90 intelligent controller */
#define DEV_IDC 	"IDC"		/* IDC integral disk controller */
#define DEV_KDA50	"KDA50" 	/* KDA50 disk controller	*/
#define DEV_KDB50	"KDB50" 	/* KDB50 disk controller	*/
#define DEV_KDM70	"KDM70" 	/* KDM70 disk/tape controller   */
#define DEV_KFBTA	"KFBTA" 	/* KFBTA disk controller	*/
#define DEV_KFQSA	"KFQSA" 	/* KFQSA disk controller	*/
#define DEV_KLESI	"KLESI" 	/* KLESI disk/tape controller	*/
#define DEV_KRQ50	"KRQ50" 	/* KRQ50 disk controller	*/
#define DEV_KRU50	"KRU50" 	/* KRU50 disk controller	*/
#define DEV_KSB50	"KSB50" 	/* KSB50 intelligent controller */
#define DEV_LAT 	"LAT"		/* LAT terminal server		*/
#define DEV_MF_SLU      "MF_SLU"        /* Mipsfair serial line controller  */
#define DEV_RH		"RH"		/* RH disk controller		*/
#define DEV_RK711	"RK711" 	/* RK711 disk controller	*/
#define DEV_RLU211	"RLU211"	/* RLU211 disk controller	*/
#define DEV_RLV211	"RLV211"	/* RLV211 disk controller	*/
#define DEV_RQDX1	"RQDX1" 	/* RQDX1 disk controller	*/
#define DEV_RQDX2	"RQDX2" 	/* RQDX2 disk controller	*/
#define DEV_RQDX3	"RQDX3" 	/* RQDX3 disk controller	*/
#define DEV_RQDX4	"RQDX4" 	/* RQDX4 disk controller	*/
#define DEV_RRD40	"RRD40" 	/* RRD40 disk controller	*/
#define DEV_RRD42	"RRD42"		/* RRD42 disk controller	*/
#define DEV_RRD50	"RRD50" 	/* RRD50 disk controller	*/
#define DEV_RUX50	"RUX50" 	/* RUX50 disk controller	*/
#define DEV_SCSI_GEN    "SCSI"          /* SCSI generic string          */
#define DEV_TBK70	"TBK70" 	/* TBK70 tape controller	*/
#define DEV_TBL70	"TBK7L" 	/* TBK7L tape ctrlr & loader	*/
#define DEV_TM03	"TM03"		/* TM03 tape formatter		*/
#define DEV_TM32	"TM32"		/* TM32 tape ctlr / device	*/
#define DEV_TM78	"TM78"		/* TM78 tape formatter		*/
#define DEV_TQK50	"TQK50" 	/* TQK50 tape controller	*/
#define DEV_TQK70	"TQK70" 	/* TQK70 tape controller	*/
#define DEV_TQL70	"TQK7L" 	/* TQK7L tape ctrlr & loader	*/
#define DEV_TSU05	"TSU05" 	/* TSU05 tape controller	*/
#define DEV_TSU11	"TSU11" 	/* TSU11 tape controller	*/
#define DEV_TSV05	"TSV05" 	/* TSV05 tape controller	*/
#define DEV_TUK50	"TUK50" 	/* TUK50 tape controller	*/
#define DEV_TUK70	"TUK70" 	/* TUK70 tape controller	*/
#define DEV_TUU80	"TU80"	 	/* TU80 tape controller		*/
#define DEV_UDA50	"UDA50" 	/* UDA50 disk controller	*/
#define DEV_UDA50A	"UDA50A"	/* UDA50 enhanced disk cont.	*/
#define DEV_VCB01	"VCB01" 	/* VCB01 workstation controller */
#define DEV_VCB02	"VCB02" 	/* VCB02 workstation controller */

/* VAXstar/CVAXstar/PVAX/Firefox device name definitions */
#define DEV_VS_SLU	"VS_SLU"	/* Serial line controller	*/
#define DEV_FF_SLU	"FF_SLU"	/* Serial line ctrlr firefox	*/
#define DEV_VS_DISK	"VS_DISK"	/* Disk controller		*/
#define DEV_VS_TAPE	"VS_TAPE"	/* TZK50 tape controller	*/
#define DEV_VS_NI	"VS_NI" 	/* Ethernet controller		*/
#define	DEV_VS_SCSI	"VS_SCSI"	/* SCSI device controller	*/

#define DEV_TM_SLE	"TM_SLE"	/*				*/

/* Definitions for device character array (device) */
#define DEV_ESE20	"ESE20"		/* ESE20 disk drive		*/
#define DEV_ESE25       "ESE25"         /* ESE25 disk drive             */
#define DEV_MOUSE	"VSXXXAA"	/* Graphics serial mouse	*/
#define DEV_R80 	"R80"		/* R80 disk drive		*/
#define DEV_RA60	"RA60"		/* RA60 disk drive		*/
#define DEV_RA70	"RA70"		/* RA70 disk drive		*/
#define DEV_RA71	"RA71"		/* RA71 disk drive		*/
#define DEV_RA72	"RA72"		/* RA72 disk drive		*/
#define DEV_RA80	"RA80"		/* RA80 disk drive		*/
#define DEV_RA81	"RA81"		/* RA81 disk drive		*/
#define DEV_RA82	"RA82"		/* RA82 disk drive		*/
#define DEV_RA90	"RA90"		/* RA90 disk drive		*/
#define DEV_RA92	"RA92"		/* RA92 disk drive		*/
#define DEV_RAMDISK	"RAMDISK"	/* RAM memory disk		*/
#define DEV_RC25	"RC25"		/* RC25 disk drive		*/
#define DEV_RC25F	"RC25F"		/* RC25 fixed disk drive	*/
#define DEV_RD31	"RD31"		/* RD31 disk drive		*/
#define DEV_RD32	"RD32"		/* RD32 disk drive		*/
#define DEV_RD33	"RD33"		/* RD33 disk drive		*/
#define DEV_RD51	"RD51"		/* RD51 disk drive		*/
#define DEV_RD52	"RD52"		/* RD52 disk drive		*/
#define DEV_RD53	"RD53"		/* RD53 disk drive		*/
#define DEV_RD54	"RD54"		/* RD54 disk drive		*/
#define DEV_RF30	"RF30"		/* RF30 disk drive		*/
#define DEV_RF31	"RF31"		/* RF31 disk drive		*/
#define DEV_RFH31	"RFH31"		/* RFH31 disk drive		*/
#define DEV_RF71	"RF71"		/* RF71 disk drive		*/
#define DEV_RF72	"RF72"		/* RF72 disk drive		*/
#define DEV_RFH72	"RFH72"		/* RFH72 disk drive		*/
#define DEV_RF73	"RF73"		/* RF73 disk drive		*/
#define DEV_RFH73	"RFH73"		/* RFH73 disk drive		*/
#define DEV_RK07	"RK07"		/* RK07 disk drive		*/
#define DEV_RL02	"RL02"		/* RL02 disk drive		*/
#define DEV_RM03	"RM03"		/* RM03 disk drive		*/
#define DEV_RM05	"RM05"		/* RM05 disk drive		*/
#define DEV_RM80	"RM80"		/* RM80 disk drive		*/
#define DEV_RP05	"RP05"		/* RP05 disk drive		*/
#define DEV_RP06	"RP06"		/* RP06 disk drive		*/
#define DEV_RP07	"RP07"		/* RP07 disk drive		*/
#define DEV_RV20	"RV20"		/* RV20 tape drive		*/
#define DEV_RV60	"RV60"		/* RV60 tape drive		*/
#define DEV_RX18	"RX18"		/* RX18 disk drive		*/
#define DEV_RX23	"RX23"		/* RX23 disk drive		*/
#define DEV_RX26	"RX26"		/* RX26 disk drive		*/
#define DEV_RZ23L	"RZ23L"		/* RZ23L disk drive		*/
#define DEV_RX33	"RX33"		/* RX33 disk drive		*/
#define DEV_RX35	"RX35"		/* RX33 disk drive		*/
#define DEV_RX50	"RX50"		/* RX50 disk drive		*/
#define DEV_RZ22	"RZ22"		/* RZ22 disk drive		*/
#define DEV_RZ23	"RZ23"		/* RZ23 disk drive		*/
#define DEV_RZ24        "RZ24"          /* RZ24 disk drive              */
#define DEV_RZ25	"RZ25"		/* RZ25 disk drive		*/
#define DEV_RZ55	"RZ55"		/* RZ55 disk drive		*/
#define DEV_RZ56	"RZ56"		/* RZ56 disk drive		*/
#define DEV_RZ57        "RZ57"          /* RZ57 disk drive              */
#define	DEV_RZxx	"RZxx"		/* Unknown SCSI disk drive	*/
#define DEV_SVS00	"SVS00"		/* SVS00 tape drive		*/
#define DEV_TA78	"TA78/9"	/* TA78/TA79 tape drive 	*/
#define DEV_TA79	"TA79"		/* TA79 tape drive		*/
#define DEV_TA81	"TA81"		/* TA81 tape drive		*/
#define DEV_TA90	"TA90"		/* TA90 tape drive		*/
#define DEV_TA91	"TA91"		/* TA91 tape drive		*/
#define DEV_TABLET	"VSXXXAB"	/* Graphics tablet		*/
#define DEV_TE16	"TE16"		/* TE16 tape drive		*/
#define DEV_TF30	"TF30"		/* TF30 tape drive		*/
#define DEV_TF70	"TF70"		/* TF70 tape drive		*/
#define DEV_TF70L	"TF70L"		/* TF70 tape drive & loader	*/
#define DEV_TF85	"TF85"		/* TF85 tape drive		*/
#define DEV_TK50	"TK50"		/* TK50 tape drive		*/
#define DEV_TK70	"TK70"		/* TK70 tape drive		*/
#define DEV_TRACE	"TRACE" 	/* TRACE special device 	*/
#define DEV_TS05	"TS05"		/* TS05 tape drive		*/
#define DEV_TS11	"TS11"		/* TS11 tape drive		*/
#define DEV_TU45	"TU45"		/* TU45 tape drive		*/
#define DEV_TU77	"TU77"		/* TU77 tape drive		*/
#define DEV_TU78	"TU78/9"	/* TU78/TU79 tape drive 	*/
#define DEV_TU80	"TU80"		/* TU80 tape drive		*/
#define DEV_TU81	"TU81"		/* TU81 tape drive		*/
#define DEV_TU81E	"TU81E" 	/* TU81E tape drive		*/
#define DEV_TLZ04       "TLZ04"         /* TLZ04 tape drive             */
#define DEV_TZ05	"TZ05"		/* CSS TZ05 tape drive		*/
#define DEV_TZ07	"TZ07"		/* CSS TZ07 tape drive		*/
#define DEV_TZK08	"TZK08"		/* Exabytes 8MM			*/
#define DEV_TZK10	"TZK10"		/* TZK10 QIC format tape	*/
#define DEV_TZ30	"TZ30"		/* TZ30 tape drive		*/
#define	DEV_TZxx	"TZxx"		/* Unknown SCSI tape drive	*/
#define DEV_TZQIC	"TZQIC"		/* A WHATever QIC tape drive	*/
#define DEV_TZ9TRK	"TZ9TRK"	/* A WHATever 9trk tape drive	*/
#define DEV_TZ8MM	"TZ8MM"		/* A WHATever 8 MM tape drive	*/
#define DEV_TZRDAT	"TZRDAT"	/* A WHATever RDAT tape drive	*/
#define DEV_VR100	"VR100" 	/* VR100 terminal		*/
#define DEV_VR260	"VR260" 	/* VR260 terminal		*/
#define DEV_VR290	"VR290" 	/* VR290 terminal		*/
#define DEV_VT100	"VT100" 	/* VT100 terminal		*/
#define DEV_VT101	"VT101" 	/* VT101 terminal		*/
#define DEV_VT102	"VT102" 	/* VT102 terminal		*/
#define DEV_VT125	"VT125" 	/* VT125 terminal		*/
#define DEV_VT220	"VT220" 	/* VT220 terminal		*/
#define DEV_VT240	"VT240" 	/* VT240 terminal		*/
#define DEV_VT241	"VT241" 	/* VT241 terminal		*/
#define DEV_VT320	"VT320" 	/* VT320 terminal		*/
#define DEV_VT330	"VT330" 	/* VT330 terminal		*/
#define DEV_VT340	"VT340" 	/* VT340 terminal		*/
#define DEV_XOS 	"XOS"		/* X in kernel special device	*/
#define DEV_ECRM	"Buffer"	/* ECRM SCSI-Buf/PelBox/Autokon */

/* Definitions for stat longword (stat) */
#define DEV_BOM 	0x01		/* Beginning-of-medium (BOM)	*/
#define DEV_EOM 	0x02		/* End-of-medium (EOM)		*/
#define DEV_OFFLINE	0x04		/* Offline			*/
#define DEV_WRTLCK	0x08		/* Write locked 		*/
#define DEV_BLANK	0x10		/* Blank media			*/
#define DEV_WRITTEN	0x20		/* Write on last operation	*/
#define DEV_CSE 	0x40		/* Cleared serious exception	*/
#define DEV_SOFTERR	0x80		/* Device soft error		*/
#define DEV_HARDERR	0x100		/* Device hard error		*/
#define DEV_DONE	0x200		/* Operation complete		*/
#define DEV_RETRY	0x400		/* Retry			*/
#define DEV_ERASED	0x800		/* Erased			*/

/* Definitions for category_stat longword (category_stat) */
/* TAPES:	*/
#define DEV_TPMARK	0x01		/* Unexpected tape mark 	*/
#define DEV_SHRTREC	0x02		/* Short record 		*/
#define DEV_RDOPP	0x04		/* Read opposite		*/
#define DEV_RWDING	0x08		/* Rewinding			*/
#define DEV_800BPI	0x10		/* 800 bpi tape density 	*/
#define DEV_1600BPI	0x20		/* 1600 bpi tape density	*/
#define DEV_6250BPI	0x40		/* 6250 bpi tape density	*/
#define DEV_6666BPI	0x80		/* 6666 bpi tape density	*/
#define DEV_10240BPI	0x100		/* 10240 bpi tape density	*/
#define DEV_38000BPI	0x200		/* 38000 bpi tape density	*/
#define DEV_LOADER	0x400		/* Media loader present		*/
#define DEV_38000_CP	0x800		/* 38000 bpi compacted density	*/
#define DEV_76000BPI	0x1000		/* 76000 bpi tape density	*/
#define DEV_76000_CP	0x2000		/* 76000 bpi compacted density	*/

/* 
 * QIC format densities please note that the recording method is serial
 * bit stream serpentine.. Actual number of bytes that can fit on a tape
 * is = ((BPI * LENGTH * TRKS ) / 8) - (the gap size very small )
 * QIC 320 = 345600000 = (16000 *7200' *24)/8 
*/
 
#define DEV_8000_BPI	0x4000		/* QIC-24 9 tracks		*/
#define DEV_10000_BPI   0x8000		/* QIC-120 and 150 15trk and 18trk */
#define DEV_16000_BPI	0x10000		/* QIC-320/525 26 tracks	*/

#define DEV_61000_BPI	0x20000		/* 4mm tape cartridge (TLZ04)	*/
#define DEV_54000_BPI	0x40000		/* 8mm tape cartridge (TZK08)	*/

/* DISKS:	*/
#define DEV_DISKPART	minor(dev)%0x08 /* Disk partition macro 	*/
#define	DEV_DPMASK	0x07		/* Disk partition mask		*/
#define	DEV_MC_COUNT	0x08		/* Bits 16-31, media changed	*/
					/* counter, see comment below.	*/
#define	DEV_3_HD2S	0x10		/* 3.5"  1.44MB HD 2side 18sect */
#define	DEV_3_DD2S	0x20		/* 3.5"  720KB  DD 2side  9sect */
#define	DEV_5_HD2S	0x30		/* 5.25" 1.2MB  HD 2side 15sect */
#define	DEV_5_DD1S	0x40		/* 5.25" 400KB  DD 1side 10sect */
#define	DEV_5_LD2S	0x50		/* 5.25" 360KB  LD 2side  9sect */
#define	DEV_5_DD2S	0x60		/* 5.25" 720KB  DD 2side  9sect */
#define	DEV_3_ED2S	0x70		/* 3.5"  2.88MB ED 2side 36sect */
#define	DEV_X_XXXX	0xf0		/* Unknown/no floppy in drive   */
#define	DEV_DDMASK	0xf0		/* Disk density mask		*/
/*
 * If DEV_MC_COUNT is set, then category_stat bits 16-31 are the media
 * changed counter (unsigned short). This count allows applications to
 * determine if the media has been changed (for removable disks like RX23).
 * NOTE: the count wraps at 65536.
 */
/* COMM:	*/
#define DEV_MODEM	0x01		/* Line supports modem control	*/
#define DEV_MODEM_ON	0x02		/* Modem control is turned on	*/


/*
 * Defines associated with the DEVGETPT ioctl:
 */

/*
 * Structure for DEVGETGEOM ioctl - used to pass device geometry.
 */
typedef union devgeom {
    struct {
	unsigned long	dev_size;   /* number of blocks in the user area (#pc)*/
	unsigned short	ntracks;    /* number of tracks per cylinder (#nt)    */
	unsigned short	nsectors;   /* number of sectors per track (#ns)      */
	unsigned short	ncylinders; /* total number of cylinders   (#nc)      */
	unsigned long   attributes; /* Device attributes; see defs below      */
    } geom_info;
    unsigned char	pad[124];  /* Allocate space to allow for expansion */	
} DEVGEOMST;

/*
 * Device attributes which are stored in the geom_info.attributes field.
 */
#define DEVGEOM_REMOVE  0x01    /* Removable Media              */

/*
 * Define different types of graphic console devices.
 */

#define	CONSOLE_DTYPE	0
#define	QVSS_DTYPE	1
#define	QDSS_DTYPE	2
#define	SS_DTYPE	3
#define	SM_DTYPE	4
#define	SG_DTYPE        5
#define	LYNX_DTYPE	6
#define	FC_DTYPE	7
#define	FG_DTYPE	8
#define	PMM_DTYPE	9
#define	PMC_DTYPE	10
#define	CFB_DTYPE	11
#define	GA_DTYPE	12
#define	GQ_DTYPE	13
#define	WS_DTYPE	14

#endif /*	DEVIO_INCLUDE */
