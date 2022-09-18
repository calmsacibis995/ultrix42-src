
/*	@(#)msiport.h	4.3	(ULTRIX)	9/4/90	*/

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************
 *
 *
 *   Facility:	Systems Communication Architecture
 *		Mayfair Storage Interconnect Port Driver
 *
 *   Abstract:	This module contains Mayfair Storage Interconnect Port
 *		Driver( MSI ) constants; data structure, port command,
 *		and register definitions; and macros.
 *
 *   Creator:	Todd M. Katz	Creation Date:	December 06, 1988
 *
 *   Modification History:
 *
 *   05-Jul-1990	Pete Keilty
 *	Changed ST_NACK_IS_ACK from 0x8080 to 0x8008 false NACK indicator.
 *
 *   29 May 90 -- chet 
 *	Changed Setup_dmap macro to allow unmapped io addresses
 *
 *   06-Feb-1990	Pete Keilty
 *	Changed Rreceive_abort check to only look for the low two bits of
 *	the REQ/ACK field as described by DSSI spec. 
 *
 *   11-Jul-1989	Pete Keilty
 *	Added new macro Xsii_busy( pccb ) for checking DSCR_OUT and
 *	Msiisr3 state bits.
 *
 *   9-Jul-1989         Stephen W. Bailey       SWBXXXX
 *      Added bit definitions and reference macro for Msiisr3 (the SII Main
 *      Control Diagnostic Register) necessary to fix race conditions with ILP,
 *      TLP and packet STATUS word for packets in the IL.
 *
 *   16-Jun-1989	Pete Keilty
 *	Add the new system smp locking primitives.
 *
 *   10-Jun-1989	Pete Keilty
 *	Add IPL raising to Xfp_started and Rfp_started macro's.
 *
 *   13-May-1989	Todd M. Katz		TMK0002
 *	Synchronize GVPBDDB( GVP Buffer Descriptor Database Header ) template
 *	to the updated structure definition required for full SMP support.
 *
 *   16-Mar-1989	Todd M. Katz		TMK0001
 *	1. It is not necessary to specially mark MSIB buffers being
 *	   re-transmitted.  Therefore, eliminate the constants MSI_BM_RETRY and
 *	   MSI_RETRY; the macros Retry_msib() and Set_retry(); and use of
 *	   Set_retry() within the macro Xretry_xmt().
 *	2. Modify the macro Xdelay_xmts() to reset the framelength of all local
 *	   RETDAT packets to the size of just the overhead portion of a RETDAT
 *	   packet.  This is the value which must be within this field when the
 *	   packet is not actually being transmitted.
 *	3. Modify the macro Xfp_timer_started() to appropriately set the IPL.
 */

/* MSI Constants.
 */
					/* Console Logging Format Codes	     */
				 	/* 0 - No optional information logged*/
				 	/* 1 - CI PPD special format code    */
				 	/* 2 - CI PPD special format code    */
				 	/* 3 - CI PPD special format code    */
				 	/* 4 - CI PPD special format code    */
				 	/* 5 - Rem port station addr logged  */
#ifdef KERNEL
#include "../h/ansi_compat.h"
#else
#include <ansi_compat.h>
#endif

#define	CF_LPORT		 6	/* Local port station address logged */
#define	CF_REGS			 7	/* Port registers logged	     */
#define	CF_PKT			 8	/* MSI packet fields logged	     */
#define	CF_PKT2			 9	/* MSI packet/cmdblock fields logged */

					/* Device Attention Logging Modifiers*/
#define	LOG_NOREGS		 0	/* Do not log device registers       */
#define	LOG_REGS		 1	/* Log device registers		     */

					/* MSI Informational Event Codes     */
/* MSI informational events are always local port specific.  There are
 * currently no remote port or path specific MSI informational events.
 * msi_log_devattn(), msi_console_log(), msl_cli[], and msi_cltab[][] must be
 * updated to reflect new additions.
 *
 * The following MSI informational events are local port specific but may NOT
 * have the local port crash severity modifier( ESM_LPC ) applied:
 *
 *	LPORT_INIT, LPORT_REINIT
 *
 * NOTE: Local port specific MSI informational events are never candidates for
 *	 application of the local port crash severity modifier( ESM_LPC ).
 *
 * NOTE: Choose an appropriate console formatting code( CF ) which includes
 *	 displaying of the local port station address when updating msi_cli[]
 *	 with a new local port specific CI informational event.
 */
#define	I_LPORT_INIT	( PDI | 0x01 )	/* Initial local port initialization */
#define	I_LPORT_REINIT	( PDI | 0x02 )	/* Local MSI port initialization     */

					/* MSI Warning Event Codes	     */
/* No codes are defined because MSI currently never defines any warning events.
 */

					/* MSI Remote Error Event Codes	     */
/* MSI remote error events are always remote port specific.  There are
 * currently no local port or path specific MSI remote error events.
 * msi_console_log(), msi_log_packet(), msi_clre[], and msi_cltab[][] must be
 * updated to reflect new additions.
 *
 * The following MSI remote error events are remote port specific:
 *
 *	RPORTSTATE
 *
 * NOTE: Choose an appropriate console formatting code( CF ) which includes 
 *	 displaying of the remote port station address when updating msi_clre[]
 *	 with a new MSI remote error event.
 */
#define	RE_RPORTSTATE	( PDRE | 0x01 )	/* Remote port in invalid state	     */

					/* MSI Error Event Codes	     */
/* MSI error events are either local port or path specific.  There are
 * currently no remote port specific MSI error events.  msi_log_packet(),
 * msi_console_log(), msi_cle[], and msi_cltab[][] must be updated to reflect
 * new additions.
 *
 * The following MSI error events are local port specific but may NOT have the
 * local port crash severity modifier( ESM_LPC ) applied:
 *
 *	INVSRCADDR
 *
 * The following MSI error events are path specific but may NOT have the path
 * crash severity modifier( ESM_PC ) applied:
 *
 *	NOPATH, RPORTSTATE
 *
 * Path specific information is always displayed by default during console
 * logging of path specific MSI error events.  The local port station address
 * is always displayed by default during console logging of local port specific
 * MSI error events.
 *
 * NOTE: Path and local port specific MSI error events are NEVER candidates for
 *	 application of the path( ESM_PC ) or local port( ESM_LPC ) crash
 *	 severity modifiers.
 *
 * NOTE: When updating msi_cle[] with a new MSI error event bear in mind the
 *	 information displayed by default when choosing an appropriate console
 *	 formatting code( CF ).
 */
#define	E_NOPATH	( PDE | 0x01 )	/* Closing vc due to exhausted retrys*/
#define	E_RPORTSTATE	( PDE | 0x02 )	/* Remote port in invalid state	     */
#define	E_INVSRCADDR	( PDE | 0x03 )	/* Invalid source port station addr  */

					/* MSI Severe Error Event Codes	     */
/* MSI severe error events are either local port or path specific.  There are
 * currently no remote port specific MSI severe error events.
 * msi_console_log(), msi_clse[], and msi_cltab[][] must be updated to reflect
 * new additions.  Either msi_log_devattn() or msi_log_packet() must be updated
 * when a new local port specific MSI severe error event is added, depending
 * upon the nature of the event.  msi_log_packet() must be updated when a new
 * path specific MSI severe error event is added.  msi_crash_lport() also
 * requires updating when the new addition is a candidate for application of
 * the local port crash severity modifier( ESM_LPC ).
 *
 * The following MSI severe error events are local port specific and may have
 * the local port crash severity modifier( ESM_LPC ) applied:
 *
 *	MFQE, INVBNAME, INVBSIZE, BUSERROR, SWA, IMODE, TMODE
 *
 * The following MSI severe error events are local port specific but may NOT
 * have the local port crash severity modifier( ESM_LPC ) applied:
 *
 *	BADPORTNUM
 *
 * The following MSI severe error events are path specific but may NOT have the
 * path crash severity modifier( ESM_PC ) applied:
 *
 *	UNKOPCODE, INVOPCODE, INVRPKTSIZE, OSEQMSG
 *
 * Path specific information is always displayed by default during console
 * logging of path specific MSI severe error events.  The local port station
 * address is always displayed by default during console logging of local port
 * specific MSI severe error events.
 *
 * NOTE: Path specific MSI severe error events are NEVER candidates for
 *	 application of the path crash severity modifier( ESM_PC ).  Not all
 *	 local port specific MSI severe error events are candidates for
 *	 application of the local port crash severity modifier( ESM_LPC ).
 *
 * NOTE: When updating msi_clse[] with a new MSI severe error event bear in
 *	 mind the information displayed by default when choosing an appropriate
 *	 console formatting code( CF ).
 */
#define	SE_BADPORTNUM   ( PDSE | 0x01 )	/* Invalid port number encountered   */
#define	SE_UNKOPCODE    ( PDSE | 0x02 )	/* Unknown opcode in packet	     */
#define	SE_INVOPCODE    ( PDSE | 0x03 )	/* Invalid opcode in packet	     */
#define	SE_INVRPKTSIZE  ( PDSE | 0x04 )	/* received invalid sized packet     */
#define	SE_OSEQMSG      ( PDSE | 0x05 )	/* Out-of-sequenced message received */
#define	SE_MFQE         ( PDSE | 0x06 )	/* Message free queue exhausted	     */
#define	SE_INVBNAME	( PDSE | 0x07 )	/* Invalid local buffer name	     */
#define	SE_INVBSIZE	( PDSE | 0x08 )	/* Local buffer length violation     */
#define	SE_BUSERROR	( PDSE | 0x09 )	/* DSSI bus error reported	     */
#define	SE_SWA		( PDSE | 0x0A )	/* SII chip selected with attention  */
#define	SE_IMODE	( PDSE | 0x0B )	/* SII chip selected non-DSSI device */
#define	SE_TMODE	( PDSE | 0x0C )	/* SII chip selected by non-DSSI dev */

					/* MSI Fatal Error Event Codes	     */
/* MSI fatal error events are always local port specific.  There are currently
 * no path or remote port specific MSI fatal error events.  msi_log_initerr(),
 * msi_clfe[], and msi_cltab[][] must be updated to reflect new additions. 
 *
 * The following NSI fatal error events are local port specific but may NOT
 * have the local port crash severity modifier( ESM_LPC ) applied:
 *
 *	INIT_ZEROID, INIT_NOMEM, INIT_NOPTES
 *
 * The local port station address is displayed by default during console
 * logging of all MSI fatal error events.
 *
 * NOTE: Local port specific MSI fatal error events may occur only during
 *	 initial probing of local MSI ports.
 *
 * NOTE: Local port specific MSI fatal error events are never candidates for
 *	 application of the local port crash severity modifier( ESM_LPC ).
 *
 * NOTE: Local port specific MSI fatal error events are always assigned the
 *	 console logging format code CF_NONE.  They never display variable
 *	 information.  This is because currently all such events are fully
 *	 logged by msi_log_initerr().  msi_console_log() is never invoked to
 *	 log them to the console.
 */
#define	FE_INIT_ZEROID	( PDFE | 0x01 )	/* Init - zero system id number      */
#define	FE_INIT_NOMEM	( PDFE | 0x02 )	/* Init - insufficient memory        */
#define	FE_INIT_NOPTES	( PDFE | 0x03 )	/* Init - insufficient ptes	     */

					/* Port Driver Panic Strings	     */
#define	PANIC_PCCBFB	"msi\t- invalid pccb fork block\n"
#define	PANIC_REQLPC	"msi\t- panic requested on all local port failures\n"
#define	PANIC_RETDAT	"msi\t- invalid transmit fork process retdat packet\n"
#define	PANIC_RFPFB	"msi\t- invalid receive fork process fork block\n"
#define	PANIC_UNKCF	"msi\t- unknown console logging formatting code\n"
#define	PANIC_UNKCODE	"msi\t- unknown/invalid event code\n"
#define	PANIC_UNKLPC	"msi\t- unknown/invalid local port crash reason\n"
#define	PANIC_XFPFB	"msi\t- invalid transmit fork process fork block\n"
#define	PANIC_XFP_TIMER	"msi\t- broken transmit fork process timer\n"

					/* Timer/Random Number Generator     */
					/*  Constants			     */
#define	RANDOM_BASE	     69069	/* Random retry delay timer base num */
#define	TIMER_UNITS		 1	/* XFP_TIMER is 10 msec intrval timer*/

					/* Transmit retry constants	     */
#define	IMMED_RETRYS		 8	/* Maximum number immediate retries  */
#define	MAX_RETRYS	       512	/* Maximum number retries	     */

/* MSI Port Specific Constants.
 */
					/* Functionality Bit Mask	     */
					/* Supported port states	     */
#define	STATE_ENABLED	0x80000000	/*  Enabled port state		     */
					/* Send    - Receive supported pkts  */
#define	MSG_MSG		0x02000000	/*  MSG	     MSG		     */
#define	CNF_SNTDAT	0x00800000	/*  CNF	     SNTDAT		     */
#define	RETDAT_DATREQ0	0x00080000	/*  RETDAT   DATREQ0		     */
#define	RETDAT_DATREQ1	0x00040000	/*  RETDAT   DATREQ1		     */
#define	RETDAT_DATREQ2	0x00020000	/*  RETDAT   DATREQ2		     */
#define	IDREQ_ID	0x00010000	/*  IDREQ    ID			     */
#define	SND_RST		0x00000800	/*  RST				     */
#define	SND_STRT	0x00000400	/*  STRT			     */
#define	PORT_FCN_MASK	0x828F0C00	/* Full bit mask		     */

					/* Miscellaneous Constants	     */
#define	ADDR_SHIFT		 2	/* Nbits to >> when comp thread addrs*/
#define	MAX_CABLES		 1	/* Maximum cable number		     */

					/* Packet Field Size Constants	     */
					/*  ( Quadword sized entities )	     */
#define	MAX_DATA_SIZE	      4096	/* Maximum size of transfered data   */
#define	MAX_DG_SIZE	      4096	/* Max size of datagram text( 4089 ) */
#define	MAX_MSG_SIZE	      4096	/* Max size of seq msg text( 4089 )  */

					/* Sequence Number Constants	     */
					/* ( Sequence nums occupy bits: 9-11)*/
#define	SEQNO_INCR	0x00000200	/* Sequence number incrementor	     */
#define	SEQNO_OVERFLOW	0x00001000	/* Sequence number overflow bit      */

/* MSI Register Definitions.
 */
struct sii_regs
{
	u_long sii_msidr0;	/* MSI Diag. Register 0			*/
	u_long sii_msidr1;	/* MSI Diag. Register 1			*/
	u_long sii_msidr2;	/* MSI Diag. Register 2			*/
	u_long sii_msicsr;	/* MSI Control and Status register.	*/
	u_long sii_msiid;	/* MSI ID register.			*/
	u_long sii_msislcs;	/* MSI Selector control/status 		*/
	u_long sii_msidestat;	/* MSI Selection detection status	*/
	u_long sii_msitr;	/* MSI Timeout Register.		*/
	u_long sii_msidata;	/* MSI Data register			*/
	u_long sii_msidmctlr;	/* MSI DMA control register		*/
	u_long sii_msidmlotc;	/* MSI DMA length to xfer		*/
	u_long sii_msidmaddrl;	/* MSI DMA address pointer		*/
	u_long sii_msidmaddrh;	/* MSI DMA address pointer		*/
	u_long sii_msidmabyte;	/* MSI DMA initial byte			*/
	u_long sii_msistlp;	/* MSI Short Target List Pointer	*/
	u_long sii_msiltlp;	/* MSI Long Target List Pointer		*/
	u_long sii_msiilp;	/* MSI Initiator List Pointer		*/
	u_long sii_msicr;	/* MSI (DSSI) Controll Register		*/
	u_long sii_msisr;	/* MSI (DSSI) Status Register		*/
	u_long sii_msidstat;	/* Data interupt control Register	*/
	u_long sii_msicomm;	/* MSI Command Register			*/
	u_long sii_msidcr;	/* MSI Diag. Control Register		*/
	u_long sii_mscccr;	/* MSI Clock Control Register		*/
	u_long sii_msiisr0;	/* MSI Internal State Register 0	*/
	u_long sii_msiisr1;	/* MSI Internal State Register 1	*/
	u_long sii_msiisr2;	/* MSI Internal State Register 2	*/
	u_long sii_msiisr3;	/* MSI Internal State Register 3	*/
};

struct siibuf
{
	u_long siibuf_msirb[32768];	/* MSI Buffer RAM		      */
};

					/* Control/Status Register Mask Bits */
#define	MSICSR_IE	0x00000001	/* Enable interrupts		     */
#define	MSICSR_PCE	0x00000002	/* Report parity errors		     */
#define	MSICSR_SLE	0x00000004	/* Allow SII to respond to selections*/
#define	MSICSR_HPM	0x00000010	/* SII is operating on arbitrated bus*/

					/* Diagnostic Control Reg Mask Bits  */
#define	MSIDCR_PRE	0x00000004	/* Port enable( start bus drivers )  */

					/* DSSI Control Register Mask Bits   */
#define	MSIDSCR_CH0	0x00000001	/* Device at port 0 is in DSSI mode  */
#define	MSIDSCR_CH1	0x00000002	/* Device at port 1 is in DSSI mode  */
#define	MSIDSCR_CH2	0x00000004	/* Device at port 2 is in DSSI mode  */
#define	MSIDSCR_CH3	0x00000008	/* Device at port 3 is in DSSI mode  */
#define	MSIDSCR_CH4	0x00000010	/* Device at port 4 is in DSSI mode  */
#define	MSIDSCR_CH5	0x00000020	/* Device at port 5 is in DSSI mode  */
#define	MSIDSCR_CH6	0x00000040	/* Device at port 6 is in DSSI mode  */
#define	MSIDSCR_CH7	0x00000080	/* Device at port 7 is in DSSI mode  */
#define	MSIDSCR_ALLCH	0x000000FF	/* Devs at all ports are in DSII mode*/
#define	MSIDSCR_OUT	0x00004000	/* Enable output transmissions	     */
#define	MSIDSCR_DSE	0x00008000	/* Enable DSSI mode in SII chip	     */
#define	MSIDSCR_SOUT	0x0000C0FF	/* Bit mask settings to start output */

					/* DSSI Status Register Mask Bits    */
#define	MSIDSSR_LST	0x00000002	/* SII lost arbitration		     */
#define	MSIDSSR_SIP	0x00000004	/* Selection in progress 	     */
#define	MSIDSSR_SWA	0x00000008	/* SII chip selected with attention  */
#define	MSIDSSR_TGT	0x00000010	/* SII chip operating as target	     */
#define	MSIDSSR_DST	0x00000020	/* SII chip selected by another	     */
#define	MSIDSSR_CON	0x00000040	/* SII chip connected to another     */
#define	MSIDSSR_SCH	0x00000080	/* SII chip left DSSI mode	     */
#define	MSIDSSR_LDN	0x00000100	/* List element done		     */
#define	MSIDSSR_BUF	0x00000200	/* Target is not in DSSI mode	     */
#define	MSIDSSR_TZ	0x00000400	/* Target ptr 0( receive list empty )*/
#define	MSIDSSR_OBC	0x00000800	/* Tmo/Target disconn/DSSI bus reset */
#define	MSIDSSR_BER	0x00001000	/* Bus error			     */
#define	MSIDSSR_RST	0x00002000	/* DSSI bus was reset		     */
#define	MSIDSSR_CI	0x00008000	/* Composite error bit		     */

#ifdef __mips
#define	MSIDSSR_FERRS	0x00001208	/* DSSI status reg fatal error bits  */
#else
#define	MSIDSSR_FERRS	0x00001288	/* DSSI status reg fatal error bits  */
#endif /* __mips */
					/* ID Register Mask Bits	     */
#define	MSIIDR_BUSID	0x00000007	/* Complement - processor DSSI bus ID*/
#define	MSIIDR_IO	0x00008000	/* Specify processor DSSI bus ID     */

					/* Timeout Register Mask Bits	     */
#define	MSITR_ITV	0x0000000E	/* DSSI initiator tmo interval value */
					/*  ( 14 * 200 usec = 2800 usec )    */
#define	MSITR_TTV	0x000000C0	/* DSSI target timeout interval value*/
					/*  ( 12 * 200 usec = 2400 usec )    */
#define	MSITR_ENA	0x00008000	/* Enable DSSI target & initator tmrs*/

					/* SII Command Register Mask Bits    */
#define	SIICOM_SIIRESET	0x00000080	/* Reset SII chip		     */

                                        /* SII Main Control Diagnostic       */
                                        /* Register Mask Bits                */
#define MSIISR3_WR0    0x00000180       /* SII writing status in IL          */
#define MSIISR3_WR1    0x00000188       /* SII writing status in IL or TL    */
#define MSIISR3_STMASK 0x000003FF       /* MAIN, XFER and LCTRL state bits   */

/* MSI Port Command Definitions.
 */
typedef	struct	_msih	{		/* MSI Port Header		     */
    u_char	opcode;			/* Operation code( packet type )     */
#define	DG		 1		/*  Datagram			     */
#define	MSG		 2		/*  Message			     */
#define	CNF		 3		/*  Confirm			     */
#define	IDREQ		 5		/*  Identification Request	     */
#define	RST		 6		/*  Reset			     */
#define	STRT		 7		/*  Start			     */
#define	DATREQ0		 8		/*  Data Request @ Priority 0	     */
#define	DATREQ1		 9		/*  Data Request @ Priority 1	     */
#define	DATREQ2		10		/*  Data Request @ Priority 2	     */
#define	ID		11		/*  Identification		     */
#define	SNTDAT		16		/*  Sent Data			     */
#define	RETDAT		17		/*  Return Data			     */
    struct	{			/* Operation code modifier flags     */
	u_char	lp		:  1;	/*  Last packet flag		     */
	u_char	ns		:  3;	/*  Sending sequence number	     */
	u_char  m		:  3;	/*  Packet size multiple	     */
	u_char  pkt_size	:  1;	/*  Packet size( always 0 )	     */
#define	Freset	pkt_size		/*   Force reset		     */
#define	Dsa	pkt_size		/*   Default start address	     */
#define	Pformat	pkt_size		/*   Packing format		     */
					/* Opcode modifier flag bit masks    */
#define	MSIH_LP		0x00000100	/*  ( used when performance matters )*/
#define	MSIH_NS		0x00000E00	/*  ( origin is beginning of header )*/
#define	MSIH_M		0x00007000
					/* Opcode modifier flag offsets	     */
#define	MSIH_NS_OFFSET		 9
#define	MSIH_M_OFFSET		12
    } flags;
} MSIH;

typedef	struct	_msicnf	{		/* CNF Packet Format		     */
    struct _msih ph;			/* Port header			     */
    u_short	 xctid[ 4 ];		/* Transaction identifier	     */
} MSI_CNF;

typedef	struct	_msidatreq {		/* DATRQE0/DATREQ1/DATREQ2 Pkt Format*/
    struct _msih ph;			/* Port header			     */
    u_short	 xctid[ 4 ];		/* Transaction identifier	     */
    u_short	 length[ 2 ];		/* Transaction length		     */
    u_short	 sbname[ 2 ];		/* Send buffer name		     */
    u_short	 sboff[ 2 ];		/* Send buffer offset		     */
    u_short	 rbname[ 2 ];		/* Receive buffer name		     */
    u_short	 rboff[ 2 ];		/* Receive buffer offset	     */
} MSI_DATREQ;

typedef	struct	_msidg	{		/* DG Packet Format		     */
    struct _msih ph;			/* Port header( also CI PPD length ) */
    u_short	 mtype;			/* CI PPD datagram type		     */
    u_char	 text[ MAX_DG_SIZE ];	/* Datagram text		     */
} MSI_DG;
#define	DG_OVHD	( sizeof( struct _msih ) + sizeof( u_short ))

typedef	struct _msiid	{		/* ID Packet Format		     */
    struct _msih	ph;		/* Port header			     */
    u_short		xctid[ 4 ];	/* Transaction identifier	     */
    struct _msi_portid	id;		/* Local port identification info    */
} MSI_ID;
#define	Port_state	id.portinfo.sys_state.port_state
#define	Port_type	id.port_type

typedef	struct	_msiidreq {		/* IDREQ Packet Format		     */
    struct _msih ph;			/* Port header			     */
    u_short	 xctid[ 4 ];		/* Transaction identifier	     */
} MSI_IDREQ;

typedef	struct	_msilretdat {		/* Local RETDAT Packet Format	     */
    struct _msih ph;			/* Port header			     */
    u_short	 xctid[ 4 ];		/* Transaction identifier	     */
    u_short	 rbname[ 2 ];		/* Receive buffer name		     */
    u_short	 rboff[ 2 ];		/* Receive buffer offset	     */
    u_short			: 16;
					/* Beginning of computation section  */
    u_short	 length[ 2 ];		/*  Transaction length		     */
    u_short	 sbname[ 2 ];		/*  Send buffer name		     */
    u_short	 sboff[ 2 ];		/*  Send buffer offset		     */
    u_short	 rcvbname[ 2 ];		/*  Receive buffer name		     */
    u_short	 rcvboff[ 2 ];		/*  Receive buffer offset	     */
} MSI_LRETDAT;
#define	LRETDAT_CSSIZE	( 10 * sizeof( u_short ))

typedef	struct	_msimsg	{		/* MSG Packet Format		     */
    struct _msih ph;			/* Port header( CI PPD length )	     */
    u_short	 mtype;			/* CI PPD sequenced message type     */
    u_char	 text[ MAX_MSG_SIZE ];	/* Message text			     */
} MSI_MSG;
#define	MSG_OVHD	( sizeof( struct _msih ) + sizeof( u_short ))

typedef	struct	_msiretdat	{	/* RETDAT Packet Format		     */
    struct _msih ph;			/* Port header			     */
    u_short	 xctid[ 4 ];		/* Transaction identifier	     */
    u_short	 rbname[ 2 ];		/* Receive buffer name		     */
    u_short      rboff[ 2 ];		/* Receive buffer offset	     */
    u_char	 data[ MAX_DATA_SIZE ]; /* Data				     */
} MSI_RETDAT;
#define	RETDAT_OVHD ( sizeof( struct _msih ) + ( 8 * sizeof( u_short )))

typedef	struct	_msirst	{		/* RST Packet Format		     */
    struct _msih ph;			/* Port header			     */
    u_short	 xctid[ 4 ];		/* Transaction identifier	     */
} MSI_RST;

typedef	struct	_msisntdat	{	/* SNTDAT Packet Format		     */
    struct _msih ph;			/* Port header			     */
    u_short	 xctid[ 4 ];		/* Transaction identifier	     */
    u_short	 rbname[ 2 ];		/* Receive buffer name		     */
    u_short      rboff[ 2 ];		/* Receive buffer offset	     */
    u_char	 data[ MAX_DATA_SIZE ]; /* Data				     */
} MSI_SNTDAT;
#define	SNTDAT_MINSIZE ( sizeof( struct _msih ) + ( 8 * sizeof( u_short )))

typedef	struct	_msistrt	{	/* STRT Packet Format		     */
    struct _msih ph;			/* Port header			     */
    u_short	 xctid[ 4 ];		/* Transaction identifier	     */
    u_short	 strt_addr[ 2 ];	/* Start address		     */
} MSI_STRT;

typedef union	_msipacket	{	/* MSI Packet Formats		     */
    struct _msih	ph;		/*  Common to all packet formats     */
    struct _msicnf	cnf;		/*  CNF				     */
    struct _msidatreq   datreq;		/*  DATREQ0/DATREQ1/DATREQ2	     */
    struct _msidg	dg;		/*  DG				     */
    struct _msiid	id;		/*  ID				     */
    struct _msiidreq	idreq;		/*  REQID			     */
    struct _msilretdat	lretdat;	/*  Local form of REDAT		     */
    struct _msimsg	msg;		/*  MSG				     */
    struct _msiretdat	retdat;		/*  RETDAT			     */
    struct _msirst	rst;		/*  RST				     */
    struct _msisntdat	sntdat;		/*  SNTDAT			     */
    struct _msistrt	strt;		/*  STRT			     */
} MSIPACKET;

/* MSI Data Structure Definitions.
 */
typedef	struct _msibh	{		/* MSI Buffer Header	  	     */
    struct _msibq *flink;		/* Queue pointers		     */
    struct _msibq *blink;
    u_short	   size;		/* Size of buffer		     */
    u_char	   type;		/* Structure type		     */
    u_char	   opt;			/* Optional field 		     */
					/*  Deallocate MSIB following xfer   */
#define	MSI_BM_DBUF	0x01		/*   "Byte Mask"		     */
#define	MSI_DEALLOC_BUF	DEALLOC_BUF	/*   "Longword" Mask		     */
					/*  VC required			     */
#define	MSI_BM_VC	0x02		/*   "Byte Mask"		     */
#define	MSI_VC		0x02000000	/*   "Longword" Mask		     */
    u_long	    retdat_size;	/* RETDAT packet size( bytes )	     */
    u_char	    msibhpad;		/* Unused( present for alignment )   */
    u_char	    rport_addr;		/* Remote port station address	     */
    u_short	    framelength;	/* Length of frame to transmit	     */
} MSIBH;

typedef struct _msib	{		/* MSI Buffer			     */
    struct _msibh     bh;		/* Buffer Header		     */
    union  _msipacket pkt;		/* Packet			     */
} MSIB;
#define	MSIB_RCVOVHD	3		/* MSIB Receive Packet Overhead	     */
					/*  ( rport_addr, framelength )	     */
#define	Blink		bh.blink
#define	Flink		bh.flink
#define	Framelength	bh.framelength
#define	Msibhpad	bh.msibhpad
#define	Opt		bh.opt
#define	Retdat_size	bh.retdat_size
#define	Rport_addr	bh.rport_addr
#define	Size		bh.size
#define	Type		bh.type

#define	Cnf		pkt.cnf
#define	Datreq		pkt.datreq
#define	Dg		pkt.dg
#define	Id		pkt.id
#define	Idreq		pkt.idreq
#define	Lretdat		pkt.lretdat
#define	Msg		pkt.msg
#define	Ph		pkt.ph
#define	Retdat		pkt.retdat
#define	Rst		pkt.rst
#define	Sntdat		pkt.sntdat
#define	Strt		pkt.strt

/* MSI Port Specific Data Structure Definitions.
 */
typedef struct _msicmdblk	{	/* MSI Command Block		     */
    u_short	thread;			/* Address of next command block     */
    u_short	status;			/* Status of current transaction     */
#define	ST_DNE		0x8000		/*  Block owned by port driver	     */
#define	ST_RST		0x0080		/*  DSSI reset during transaction    */
#define	ST_TMO		0x0040		/*  Timeout during transaction	     */
#define	ST_XSM		0x0020		/*  Checksum mismatch( target )	     */
#define	ST_BPH		0x0010		/*  Target in bad phase( initiator ) */
#define	ST_STT		0x0008		/*  Target not ret ACK( initiator )  */
#define	ST_PHS		0x0004		/*  Unexpected bus phase( initiator )*/
#define	ST_DSA		0x0002		/*  Error in command bytes( target ) */
#define	ST_PAR		0x0001		/*  Parity error detected	     */
					/*  Summary statuses		     */
#define	ST_SUCCESS     ((u_short)ST_DNE)/*   Successful transaction	     */
#define	ST_NACK_IS_ACK ((u_short)0x8008)/*   NACK is really ACK( initiator ) */
    u_short	command;		/* Command word			     */
#define	CMD_IE		0x8000		/*  Interrupt when transaction done  */
#define	CMD_DSTID	0x0007		/*  ID of target( initiator )	     */
    u_short	opcode;			/* Command operation code	     */
#define	CMDOPC_REQACK	0x3000		/*  REQ/ACK offset value of field    */
#define	CMDOPC_GROUPC	0x00E0		/*  Group code value of field	     */
#define	CMDOPC_OPCODE	0x30E0		/*  Command operation code to use    */
    u_char	dst;			/* Destination port station address  */
    u_char	src;			/* Source port station address	     */
    u_short	length;			/* Frame length( ie- size of data )  */
} MSICMDBLK;

typedef struct _msixmtlnk	{	/* MSI Transmit Data Segment Link    */
    u_short	len;			/* Length of next segment	     */
#define	LINK		0x8000		/*  Data segment follows the next one*/
    u_short	addr;			/*  Nxt data segmnt addr( bits 17:02)*/
} MSIXMTLNK;

typedef	struct _siibuf	{		/* SII RAM Transmit/Receive Buffer   */
    struct _siibq    *flink;		/* Queue pointers		     */
    struct _siibq    *blink;
    u_short	     size;		/* Size of buffer		     */
    u_char	     type;		/* Structure type		     */
    u_char	     opt;		/* Optional field 		     */
    struct _msib     *save_msib;	/* Saved MSIB address		     */
    u_short	     cmdblkaddr;	/* MSI command block SII RAM address */
    u_short	     xpktaddr;		/* Transmit data block SII RAM addr  */
					/*  ( Transmit SII RAM buffer ONLY ) */
    u_long			: 32;	/* ( Pad for Quadword alignment )    */
    struct _msicmdblk cmdblk;		/* Buffer MSI command block	     */
    union	{			/* Transmit/Receive specific portion */
	struct	{			/*  Transmit SII RAM buffer portion  */
	    struct _msixmtlnk link;	/*   Link to xmt data block segment  */
	    union  _msipacket pkt;	/*   Transmit data block	     */
	} xmt;
	struct	{			/*  Receive SII RAM buffer portion   */
	    union  _msipacket pkt;	/*   Receive data block		     */
	} rcv;
    } un;
#define	Xlink	un.xmt.link
#define	Rpkt	un.rcv.pkt
#define	Xpkt	un.xmt.pkt
} SIIBUF;

/* MSI Macros.
 */
					/* Buffer Name Field Macros
					 */
#define	Bindex( bname )		bname[ 0 ]
#define	Bkey( bname )		bname[ 1 ]

					/* Double Mapping Macros
					 */
#define	Check_bname( bname, bdp )					\
    (( Bindex( bname ) <= ( lscs.max_msibds - 1 ))	  &&		\
     ( Bkey( bname ) == ( bdp = ( msi_bddb->bdt + Bindex( bname )))->key ) &&\
      bdp->valid )
#define	Check_bsize( bdp, dminfo )					\
    (( dminfo.ssize == 0 ) || ( bdp->bsize >= ( dminfo.ssize + dminfo.Sboff )))
#define	Setup_dmap( bdp, dminfo ) {					\
    register u_char	*toaddr;					\
    register u_long	nptes;						\
    register struct pte	*fptep, *tptep;					\
    dminfo.Sboff += ( U_short( *bdp ) & PGOFSET );			\
    if (( U_short( *bdp ) & 0x4000 )) {					\
	dminfo.Saddr = ( u_char * )((u_long)bdp->bpte & ~PGOFSET) + dminfo.Sboff ; \
    }									\
    else if (( unsigned )bdp->bpte >= ( unsigned )Sysmap &&		\
       ( unsigned )bdp->bpte <  ( unsigned )( Sysmap + sysptsize )) {	\
	dminfo.Saddr = ( u_char * )sptetosva( bdp->bpte ) + dminfo.Sboff; \
    } else {								\
	for( toaddr = ( u_char * )dminfo.dmap_baddr,			\
	      nptes = rbtop( dminfo.ssize + ( dminfo.Sboff & PGOFSET )),\
	      fptep = ( bdp->bpte + btop( dminfo.Sboff )),		\
	      tptep = dminfo.dmap_bpteaddr;				\
	     nptes-- > 0;						\
	     toaddr += NBPG ) {						\
	    *( u_long * )tptep++					\
		= ((*( u_long * )fptep++ & PG_PFNUM ) | dminfo.protopte ); \
	    Tbis( toaddr );						\
	}								\
	dminfo.Saddr = ( u_char * )( dminfo.dmap_baddr +		\
				     ( dminfo.Sboff & PGOFSET ));	\
    }									\
}

					/* Event Logging Macros
					 */
#define	Clog_ftab( tab, event )						\
    ( tab[ Eseverity( event )][ Esubclass( event )].ftable + Ecode( event) - 1)
#define	Clog_maxcode( tab, event )					\
    ( tab[ Eseverity( event )][ Esubclass( event )].max_code )
#define	Clog_tabcode( tab, event )	( Clog_ftab( tab, event )->fcode )
#define	Clog_tabmsg( tab, event )	( Clog_ftab( tab, event )->msg )
#define	Elmsicmdblk( p )		(( struct msi_cmdblk * )p )
#define	Elmsicommon( elp )		( &elp->el_body.elmsi.msicommon )
#define	Elmsidattn( elp )						\
    ( &elp->el_body.elmsi.msitypes.msidattn )
#define	Elmsilcommon( elp )						\
    ( &elp->el_body.elmsi.msitypes.msilpkt.msilcommon )
#define	Elmsiregs( p )			(( struct msi_regs * )p )

					/* Initialization Macros
					 */
#define	Init_lpdinfo( pccb ) {						\
    U_long( pccb->Lpidinfo.port_type[ 0 ]) = HPT_SII;			\
    U_long( pccb->Lpidinfo.portinfo.port_fcn[ 0 ]) = PORT_FCN_MASK;	\
    pccb->Lpidinfo.portinfo.sys_state.reset_port			\
			= Scaaddr_lob( pccb->lpinfo.addr );		\
    pccb->Lpidinfo.portinfo.sys_state.port_state = PS_ENAB;		\
    pccb->Lpidinfo.portinfo.port_fcn_ext.maxbodylen			\
			= sizeof( struct _msipacket );			\
}
#define	Init_portinfo( tpi ) {						\
    Flushq( tpi->xretryq )						\
    tpi->xretrys = 0;							\
    tpi->xretry_timer = 0;						\
    tpi->xseqno = 0;							\
    tpi->rseqno = 0;							\
    tpi->rpstatus.dip = 0;						\
    tpi->rpstatus.path = 0;						\
    tpi->rpstatus.vc = 0;						\
}

					/* Local Port Status Bit Macros
					 */
#define	Lpstatus_active( pccb )	(*( u_long * )&pccb->Lpstatus & MSI_ACTIVE )
#define	Lpstatus_rfork( pccb )	(*( u_long * )&pccb->Lpstatus & MSI_RFORK )
#define	Lpstatus_xfork( pccb )	(*( u_long * )&pccb->Lpstatus & MSI_XFORK )
#define	Clear_rfork( pccb ) {						\
    *( u_long * )&pccb->Lpstatus &= ~MSI_RFORK;				\
}
#define	Clear_xfork( pccb ) {						\
    *( u_long * )&pccb->Lpstatus &= ~MSI_XFORK;				\
}
#define	Set_rfork( pccb ) {						\
    *( u_long * )&pccb->Lpstatus |= MSI_RFORK;				\
}
#define	Set_xfork( pccb ) {						\
    *( u_long * )&pccb->Lpstatus |= MSI_XFORK;				\
}

					/* MSIB Field Macros
					 */
#define	Dealloc_msib( msibp )	( msibp->Opt & MSI_BM_DBUF )
#define	Free_msib( msibp )	( Dealloc_msib( msibp ) == 0 )
#define	Vc_msib( msibp )	( msibp->Opt & MSI_BM_VC )
#define	Set_discard( msibp ) {						\
    msibp->Opt |= MSI_BM_DBUF;						\
}
#define	Scaaddr_lob( addr )	(*( u_char * )&addr )
#define	Set_vc( msibp ) {						\
    msibp->Opt |= MSI_BM_VC;						\
}

					/* MSIB Header Formating Macros
					 */
#define	Format_cnf( msibp, xid ) {					\
    Move_xctid( xid, msibp->Cnf.xctid )					\
}
#define	Format_id( msibp, xid ) {					\
    Move_xctid( xid, msibp->Id.xctid )					\
    msibp->Id.id = pccb->Lpidinfo;					\
}
#define	Format_idreq( msibp ) {						\
    Zero_xctid( msibp->Idreq.xctid )					\
}
#define	Format_msib( msibp, size, type ) {				\
    U_long( msibp->Size ) = ( u_long )( size );				\
    msibp->Type = ( u_char )( type );					\
    U_long( msibp->Msibhpad ) = 0;					\
}
#define	Format_msibh( msibp, rport, framelength, flags ) {		\
    U_long( msibp->Size ) |= ( u_long )( flags );			\
    msibp->Rport_addr = ( u_char )( rport );				\
    msibp->Framelength = ( framelength );				\
}
#define	Format_msih( msibp, opc ) {					\
    U_short( msibp->Ph.opcode ) = ( u_short )( opc );			\
}
#define	Format_msippdh( msibp, ppdtype ) {				\
    (( CIPPDH * )&msibp->Ph )->mtype = ppdtype;				\
}
#define	Format_rst( msibp ) {						\
    Zero_xctid( msibp->Rst.xctid )					\
}
#define	Format_strt( msibp, saddr ) {					\
    Zero_xctid( msibp->Strt.xctid )					\
    Copy_long( &saddr, &msibp->Strt.strt_addr[ 0 ])			\
}
#define	Move_xctid( from, to ) {					\
    U_short( to[ 0 ]) = U_short( from[ 0 ]);				\
    U_long( to[ 1 ]) = U_long( from[ 1 ]);				\
    U_short( to[ 3 ]) = U_short( from[ 3 ]);				\
}
#define	Reset_msib( msibp ) {						\
    msibp->Opt = 0;							\
    U_long( msibp->Msibhpad ) = 0;					\
}
#define	Zero_xctid( xctid ) {						\
    U_short( xctid[ 0 ]) = 0;						\
    U_long( xctid[ 1 ]) = 0;						\
    U_short( xctid[ 3 ]) = 0;						\
}

					/* MSIB Header Positioning Macros
					 */
#define	Ppd_to_pd( pccb, cippdbp )					\
    (( MSIB * )(( u_char * )cippdbp - pccb->lpinfo.Ppd_ovhd ))
#define	Scs_to_pd( pccb, scsbp )					\
    (( MSIB * )(( u_char * )scsbp - pccb->lpinfo.Pd_ovhd ))

					/* MSIB Queue Manipulation Macros
					 */
/* SMP: If the specified queue requires locking it must be locked EXTERNAL
 *	to macro invocation.
 */
#define	Flushq( q ) {							\
    register msibq	*msibp;						\
    while(( msibp = q.flink ) != &q ) {					\
	Remove_msib( Msibp )						\
	( void )msi_dealloc_pkt( Msibp );				\
    }									\
}
/* SMP: The PCCB specific COMQH is locked allowing exclusive access to the
 *	corresponding high priority command queue.
 */
#define	Flush_comqh( pccb ) {						\
    Lock_comqh( pccb )							\
    Flushq( pccb->Comqh )						\
    Unlock_comqh( pccb )						\
}
/* SMP: The PCCB specific COMQL is locked allowing exclusive access to the
 *	corresponding low priority command queue.
 */
#define	Flush_comql( pccb ) {						\
    Lock_comql( pccb )							\
    Flushq( pccb->Comql )						\
    Unlock_comql( pccb )						\
}
/* SMP: The PCCB specific DFREEQ is locked allowing exclusive access to the
 *	corresponding datagram free queue.
 */
#define	Flush_dfreeq( pccb ) {						\
    Lock_dfreeq( pccb )							\
    Flushq( pccb->Dfreeq )						\
    Unlock_dfreeq( pccb )						\
}
/* SMP: The PCCB specific MFREEQ is locked allowing exclusive access to the
 *	corresponding message free queue.
 */
#define	Flush_mfreeq( pccb ) {						\
    Lock_mfreeq( pccb )							\
    Flushq( pccb->Mfreeq )						\
    Unlock_mfreeq( pccb )						\
}
/* SMP: If the specified queue requires locking it must be locked EXTERNAL
 *	to macro invocation.
 */
#define	Insert_msib( msibp, q ) {					\
    Insert_entry((( msibq * )msibp )->flink, q )			\
}
/* SMP: The PCCB specific COMQH is locked allowing exclusive access to the
 *	corresponding high priority command queue.
 */
#define	Insert_comqh( pccb, msibp ) {					\
    Lock_comqh( pccb )							\
    Insert_msib( msibp, pccb->Comqh )					\
    Unlock_comqh( pccb )						\
}
/* SMP: The PCCB specific COMQL is locked allowing exclusive access to the
 *	corresponding low priority command queue.
 */
#define	Insert_comql( pccb, msibp ) {					\
    Lock_comql( pccb )							\
    Insert_msib( msibp, pccb->Comql )					\
    Unlock_comql( pccb )						\
}
/* SMP: The PCCB specific DFREEQ is locked allowing exclusive access to the
 *	corresponding datagram free queue.
 */
#define	Insert_dfreeq( pccb, msibp ) {					\
    Lock_dfreeq( pccb )							\
    Insert_msib( msibp, pccb->Dfreeq )					\
    Unlock_dfreeq( pccb )						\
}
/* SMP: The PCCB specific MFREEQ is locked allowing exclusive access to the
 *	corresponding message free queue.
 */
#define	Insert_mfreeq( pccb, msibp ) {					\
    Lock_mfreeq( pccb )							\
    Insert_msib( msibp, pccb->Mfreeq )					\
    Unlock_mfreeq( pccb )						\
}
/* SMP: If the queue on which the MSIB is located requires locking it must be
 *	locked EXTERNAL to macro invocation.
 */
#define	Remove_msib( msibp ) {						\
    Remove_entry((( msibq * )msibp )->flink )				\
}
/* SMP:
 */
#define	Remove_comqh( pccb, msibp ) {					\
    if( pccb->Comqh.flink != &pccb->Comqh ) {				\
	Lock_comqh( pccb )						\
	msibp = pccb->Comqh.flink;					\
	Remove_msib( Msibp )						\
	Unlock_comqh( pccb )						\
    } else {								\
	msibp = NULL;							\
    }									\
}
/* SMP:
 */
#define	Remove_comql( pccb, msibp ) {					\
    if( pccb->Comql.flink != &pccb->Comql ) {				\
	Lock_comql( pccb )						\
	msibp = pccb->Comql.flink;					\
	Remove_msib( Msibp )						\
	Unlock_comql( pccb )						\
    } else {								\
	msibp = NULL;							\
    }									\
}
/* SMP: 
 */
#define	Remove_dfreeq( pccb, msibp ) {					\
    if( pccb->Dfreeq.flink != &pccb->Dfreeq ) {				\
	Lock_dfreeq( pccb )						\
	msibp = pccb->Dfreeq.flink;					\
	Remove_msib( Msibp )						\
	Unlock_dfreeq( pccb )						\
    } else {								\
	msibp = NULL;							\
    }									\
}
/* SMP:
 */
#define	Remove_mfreeq( pccb, msibp ) {					\
    if( pccb->Mfreeq.flink != &pccb->Mfreeq ) {				\
	Lock_mfreeq( pccb )						\
	msibp = pccb->Mfreeq.flink;					\
	Remove_msib( Msibp )						\
	Unlock_mfreeq( pccb )						\
    } else {								\
	msibp = NULL;							\
    }									\
}
					/* MSIH Field Macros
					 */
#define	Lp_msih( ph )	(*( u_long * )&ph & MSIH_LP )
#define	M_msih( ph )	((*( u_long * )&ph & MSIH_M ) >> MSIH_M_OFFSET )
#define	Ns_msih( ph )	(*( u_long * )&ph & MSIH_NS )
#define	Set_lp( ph ) {							\
    *( u_long * )&ph |= MSIH_LP;					\
}


					/* Miscellaneous Macros
					 */
#define	Copy_long( from, to ) {						\
    *( u_short * )to = *( u_short * )from;				\
    *(( u_short * )to + 1 ) = *(( u_short * )from + 1 );		\
}
#define	Incr_seqno( sn ) {						\
    if(( sn = ( long )sn + SEQNO_INCR ) & SEQNO_OVERFLOW ) {		\
	sn = 0;								\
    }									\
}
#define	Quad_align( p )	((( u_long )p + 0x00000007 ) & ~0x00000007 )

					/* Remote Port Status Bit Macros
					 */
#define	Rpstatus_dip( ppi )	(*( u_long * )&ppi->rpstatus & MSI_RPDIP )
#define	Rpstatus_path( ppi )	(*( u_long * )&ppi->rpstatus & MSI_RPPATH )
#define	Rpstatus_vc( ppi )	(*( u_long * )&ppi->rpstatus & MSI_RPVC )
#define	Clear_rpdip( ppi ) {						\
    *( u_long * )&ppi->rpstatus &= ~MSI_RPDIP;				\
}
#define	Set_rpdip( ppi ) {						\
    *( u_long * )&ppi->rpstatus |= MSI_RPDIP;				\
}
#define	Set_rppath( ppi ) {						\
    *( u_long * )&ppi->rpstatus |= MSI_RPPATH;				\
}

					/* Shorthand Notations
					 */
#define	Msibp		(( MSIB * )msibp )
#define	Siibp		(( SIIBUF * )siibp )
#define	Comqh		pd.msi.comqh
#define	Comqh_lk	pd.msi.comqh_lk
#define	Comql		pd.msi.comql
#define	Comql_lk	pd.msi.comql_lk
#define	Dfreeq		pd.msi.dfreeq
#define	Dfreeq_lk	pd.msi.dfreeq_lk
#define	Dg_ovhd		pd.msi.dg_ovhd
#define	Dg_size		pd.msi.dg_size
#define	Errlogopt	pd.msi.errlogopt
#define	Lpcinfo		pd.msi.lpcinfo
#define	Lpidinfo	pd.msi.lpidinfo
#define	Lpstatus	pd.msi.lpstatus
#define	Lretdat_cssize	pd.msi.lretdat_cssize
#define	Max_datreq_size	pd.msi.max_datreq_size
#define	Max_dg_size	pd.msi.max_dg_size
#define	Max_id_size	pd.msi.max_id_size
#define	Max_idreq_size	pd.msi.max_idreq_size
#define	Max_msg_size	pd.msi.max_msg_size
#define	Max_sntdat_size	pd.msi.max_sntdat_size
#define	Mfreeq		pd.msi.mfreeq
#define	Mfreeq_lk	pd.msi.mfreeq_lk
#define	Min_datreq_size	pd.msi.min_datreq_size
#define	Min_dg_size	pd.msi.min_dg_size
#define	Min_id_size	pd.msi.min_id_size
#define	Min_idreq_size	pd.msi.min_idreq_size
#define	Min_msg_size	pd.msi.min_msg_size
#define	Min_sntdat_size	pd.msi.min_sntdat_size
#define	Msg_ovhd	pd.msi.msg_ovhd
#define	Msg_size	pd.msi.msg_size
#define	Msicomm		Siiregptrs.msicomm
#define	Msicsr		Siiregptrs.msicsr
#define	Msidcr		Siiregptrs.msidcr
#define	Msidscr		Siiregptrs.msidscr
#define	Msidssr		Siiregptrs.msidssr
#define	Msidstat	Siiregptrs.msidstat
#define	Msiisr3 	Siiregptrs.msiisr3	
#define	Msiidr		Siiregptrs.msiidr
#define	Msiilp		Siiregptrs.msiilp
#define	Msitr		Siiregptrs.msitr
#define	Msitlp		Siiregptrs.msitlp
#define	Pd_ovhd		pd.msi.pd_ovhd
#define	Perport		pd.msi.perport
#define	Pkt_size	pd.msi.pkt_size
#define	Ppd_ovhd	pd.msi.ppd_ovhd
#define	Randomseed	pd.msi.randomseed
#define	Rbusy		pd.msi.rbusy
#define	Rdmap		pd.msi.rdmap
#define	Rforkb		pd.msi.rforkb
#define	Retdat_ovhd	pd.msi.retdat_ovhd
#define	Rfp_lk		pd.msi.rfp_lk
#define	Rpinfo		pd.msi.rpinfo
#define	Rpslogmap	pd.msi.rpslogmap
#define	Save_dssr	pd.msi.save_dssr
#define	Save_dstat	pd.msi.save_dstat
#define	Siibuffer	pd.msi.siibuffer
#define	Siiregptrs	pd.msi.siiregptrs
#define	Siiregs		pd.msi.siiregs
#define	Xbusy		pd.msi.xbusy
#define	Xdmap		pd.msi.xdmap
#define	Xforkb		pd.msi.xforkb
#define	Xfp_lk		pd.msi.xfp_lk
#define	Xfree		pd.msi.xfree

#define	Init_comqh_lock( pccb ) {					\
    lockinit( &(( pccb )->Comqh_lk ), &lock_msicomqh_d );		\
}
#define	Init_comql_lock( pccb ) {					\
    lockinit( &(( pccb )->Comql_lk ), &lock_msicomql_d );		\
}
#define	Init_dfree_lock( pccb ) {					\
    lockinit( &(( pccb )->Dfreeq_lk ), &lock_msidfreeq_d );		\
}
#define	Init_mfree_lock( pccb ) {					\
    lockinit( &(( pccb )->Mfreeq_lk ), &lock_msimfreeq_d );		\
}
#define	Init_rfp_lock( pccb ) {						\
    lockinit( &(( pccb )->Rfp_lk ), &lock_msirfp_d ); 			\
}
#define	Init_xfp_lock( pccb ) {						\
    lockinit( &(( pccb )->Xfp_lk ), &lock_msixfp_d );			\
}
#define	Lock_comqh( pccb ) {						\
    smp_lock( &(( pccb )->Comqh_lk ), LK_RETRY ); 			\
}
#define	Lock_comql( pccb ) {						\
    smp_lock( &(( pccb )->Comql_lk ), LK_RETRY );			\
}
#define	Lock_dfreeq( pccb ) {						\
    smp_lock( &(( pccb )->Dfreeq_lk ), LK_RETRY );			\
}
#define	Lock_mfreeq( pccb ) {						\
    smp_lock( &(( pccb )->Mfreeq_lk ), LK_RETRY ); 			\
}
#define	Lock_rfp( pccb ) {						\
    smp_lock( &(( pccb )->Rfp_lk ), LK_RETRY );				\
}
#define	Lock_xfp( pccb ) {						\
    smp_lock( &(( pccb )->Xfp_lk ), LK_RETRY );				\
}
#define	Test_rfp_lock( pccb )	(( pccb )->Rfp_lk.l_lock )
#define	Test_xfp_lock( pccb )	(( pccb )->Xfp_lk.l_lock )
#define	Unlock_comqh( pccb ) {						\
    smp_unlock( &(( pccb )->Comqh_lk ));				\
}
#define	Unlock_comql( pccb ) {						\
    smp_unlock( &(( pccb )->Comql_lk )); 				\
}
#define	Unlock_dfreeq( pccb ) {						\
    smp_unlock( &(( pccb )->Dfreeq_lk ));				\
}
#define	Unlock_mfreeq( pccb ) {						\
    smp_unlock( &(( pccb )->Mfreeq_lk )); 				\
}
#define	Unlock_rfp( pccb ) {						\
    smp_unlock( &(( pccb )->Rfp_lk )); 					\
}
#define	Unlock_xfp( pccb ) {						\
    smp_unlock( &(( pccb )->Xfp_lk )); 					\
}

/* MSI Port Specific Macros.
 */

					/* SIIBUF Header Formating Macros
					 */
#define	Interrupt_disable( siibp ) {					\
    siibp->cmdblk.command &= ( u_short )CMD_DSTID;			\
}
#define	Interrupt_enable( siibp ) {					\
    (( SIIBUF * )siibp )->cmdblk.command |= ( u_short )CMD_IE;		\
}

					/* SIIBUF Queue Manipulation Macros
					 */
#define	Insert_siib( siibp, q ) {					\
    Insert_entry((( siibq * )siibp )->flink, q )			\
}
#define	Remove_siib( siibp ) {						\
    Remove_entry((( siibq * )siibp )->flink )				\
}
					/* SIIBUF Thread Macros
					 */
#define	Siiaddr( pccb, addr )						\
    ( u_short )((( u_char * )addr - pccb->Siibuffer ) >> ADDR_SHIFT )
#define	Thread_cmdblk( headp, siibp ) {					\
    (( SIIBUF * )( headp )->blink )->cmdblk.thread			\
			= (( SIIBUF * )( siibp ))->cmdblkaddr;		\
}
#define	Term_thread( headp ) {						\
    (( SIIBUF * )( headp )->blink )->cmdblk.thread = 0;			\
}
#define	Unthread_cmdblk( siibp ) {					\
    (( SIIBUF * )siibp->blink )->cmdblk.thread = siibp->cmdblk.thread;	\
}

/* Receive Fork Process Macros.
 */
					/* Fork Process Control Macros
					 */
#define	Rstart_rfp( pccb ) {						\
    register u_long	unlock;						\
    if( !Test_pccb_lock( pccb )) {					\
	Lock_pccb( pccb )						\
	unlock = 1;							\
    } else {								\
	unlock = 0;							\
    }									\
    if( Lpstatus_rfork( pccb ) == 0 ) {					\
	Set_rfork( pccb )						\
	Kfork( &pccb->Rforkb, msi_rfp, pccb )				\
    }									\
    if( unlock ) {							\
	Unlock_pccb( pccb )						\
    }									\
}
#define	Rfp_started( pccb, save_ipl ) {					\
    save_ipl = Splscs();						\
    Lock_pccb( pccb )							\
    Lock_rfp( pccb )							\
    if( Lpstatus_rfork( pccb )) {					\
	Clear_rfork( pccb )						\
	Unlock_pccb( pccb )						\
	if( Lpstatus_active( pccb ) == 0 || pccb->Rbusy == NULL ) {	\
	    Unlock_rfp( pccb )						\
	    ( void )splx( save_ipl );					\
	    return;							\
	}								\
    } else {								\
	( void )panic( PANIC_RFPFB );					\
    }									\
}

					/* MSIB Field Macros
					 */
#define	Rappl_length( msibp )	( msibp->Framelength - sizeof( CIPPDH ))

					/* MSIB Formating Macros
					 */
#define	Rformat_msib( siibp, msibp ) {					\
    ( void )bcopy(( u_char * )&siibp->cmdblk.src,			\
		  ( u_char * )&msibp->Rport_addr,			\
		  ( siibp->cmdblk.length + MSIB_RCVOVHD ));		\
}
#define	Rset_cippd_len( msibp ) {					\
    (( CIPPDH * )&msibp->Ph )->length					\
		= ( msibp->Framelength - sizeof( struct _msih ));	\
}

					/* MSIB Packet Transmission Macros
					 */
#define	Rqueue_cnf( pccb, msibp, siibp ) {				\
    Format_msibh( msibp, siibp->cmdblk.src, sizeof( MSI_CNF ), MSI_VC )	\
    Format_msih( msibp, CNF )						\
    Format_cnf( msibp, siibp->Rpkt.sntdat.xctid )			\
    Insert_comqh( pccb, msibp )						\
}
#define Rqueue_id( pccb, msibp, siibp ) {				\
    Format_msibh( msibp, siibp->cmdblk.src, sizeof( MSI_ID ), 0 )	\
    Format_msih( msibp, ID )						\
    Format_id( msibp, siibp->Rpkt.idreq.xctid )				\
    Insert_comqh( pccb, msibp )						\
}
#define	Rqueue_lretdat( pccb, msibp, siibp ) {				\
    Format_msibh( msibp, siibp->cmdblk.src, pccb->Retdat_ovhd, MSI_VC )	\
    msibp->Retdat_size = (( M_msih( siibp->Rpkt.ph ) + 1 ) * 512 );	\
    Format_msih( msibp, RETDAT )					\
    Move_xctid( siibp->Rpkt.datreq.xctid, msibp->Lretdat.xctid )	\
    Copy_long( &siibp->Rpkt.datreq.rbname[ 0 ], &msibp->Lretdat.rbname[ 0 ])\
    Copy_long( &siibp->Rpkt.datreq.rboff[ 0 ], &msibp->Lretdat.rboff[ 0 ])\
    ( void )bcopy(( u_char * )&siibp->Rpkt.datreq.length[ 0 ],		\
		  ( u_char * )&msibp->Lretdat.length[ 0 ],		\
		  pccb->Lretdat_cssize );				\
    Insert_comql( pccb, msibp )						\
}

					/* Reception Completion Macros
					 */
#define	Rfinish_receive( pccb, msibp ) {				\
    if( msibp->Ph.opcode == MSG ) {					\
	Rset_cippd_len( msibp )						\
	( void )scs_msg_rec( pccb,					\
			     ( SCSH * )msibp->Msg.text,			\
			     Rappl_length( msibp ));			\
    } else if( msibp->Ph.opcode == DG ) {				\
	Rset_cippd_len( msibp )						\
	if( msibp->Dg.mtype == SCSDG ) {				\
	    ( void )scs_dg_rec( pccb,					\
				( SCSH * )msibp->Dg.text,		\
				Rappl_length( msibp ));			\
	} else {							\
	    ( void )cippd_receive( pccb,				\
				   ( CIPPDH * )&msibp->Ph,		\
				   ( u_long )msibp->Dg.mtype );		\
	}								\
    } else {								\
	( void )cippd_receive( pccb,					\
			       ( CIPPDH * )&msibp->Ph,			\
			       CNFE_ID_REC );				\
    }									\
}
#define	Rincr_rseqno( pccb, tpi ) {					\
    Incr_seqno( tpi->rseqno )						\
}
#define	Rlog_invsrcaddr( pccb, siibp ) {				\
    register siibq	*rbusy_cache = pccb->Rbusy;			\
    pccb->Rbusy = NULL;							\
    Unlock_rfp( pccb )							\
    Lock_pccb( pccb )							\
    Lock_rfp( pccb )							\
    pccb->Rbusy = rbusy_cache;						\
    pccb->Errlogopt.portnum = Siibp->cmdblk.src;			\
    ( void )msi_log_packet( pccb,					\
			    NULL,					\
			    &siibp->cmdblk,				\
			    &siibp->Rpkt.ph,				\
			    siibp->cmdblk.length,			\
			    E_INVSRCADDR,				\
			    LPORT_EVENT );				\
    Unlock_pccb( pccb )							\
}
#define	Rproc_error( pccb, siibp, msibp, event, lpc, rq ) {		\
    register PB		*pb;						\
    u_long		cport = siibp->cmdblk.src;			\
    siibq		*rbusy_cache = pccb->Rbusy;			\
    pccb->Rbusy = NULL;							\
    Unlock_rfp( pccb )							\
    while(( msibp = rq.flink ) != &rq ) {				\
	Remove_msib( Msibp )						\
	Rfinish_receive( pccb, Msibp )					\
    }									\
    Lock_pccb( pccb )							\
    if( lpc == 0 ) {							\
	if(( pb = cippd_get_pb( pccb, ( SCSH * )&cport, NO_BUF ))) {	\
	    Lock_pb( pb )						\
	} else {							\
	    pccb->Errlogopt.portnum = cport;				\
	}								\
	if( pb || event != E_RPORTSTATE ) {				\
	    ( void )msi_log_packet( pccb,				\
				    pb,					\
				    NULL,				\
				    &siibp->Rpkt.ph,			\
				    siibp->cmdblk.length,		\
				    event,				\
				    PATH_EVENT );			\
	}								\
	if( pb ) {							\
	    if( pb->pinfo.state != PS_PATH_FAILURE ) {			\
		( void )cippd_crash_pb( pccb, pb, E_PD, 0, NULL );	\
	    }								\
	    Unlock_pb( pb )						\
	}								\
    }									\
    Lock_rfp( pccb )							\
    pccb->Rbusy = rbusy_cache;						\
    if( lpc ) {								\
	pccb->Lpstatus.optlpcinfo = 1;					\
	pccb->Lpcinfo.pkth = &Siibp->Rpkt.ph;				\
	pccb->Lpcinfo.pktsize = Siibp->cmdblk.length;			\
	pccb->Lpcinfo.pport_addr = cport;				\
	( void )msi_crash_lport( pccb, event, NULL );			\
	pccb->Lpstatus.optlpcinfo = 0;					\
    }									\
    Unlock_pccb( pccb )							\
}

					/* SIIBUF Header Formating Macros
					 */
#define	Rreset_cmdblk( siibp ) {					\
    U_short( siibp->cmdblk.status ) = 0;				\
    U_long( siibp->cmdblk.command ) = 0;				\
    Interrupt_enable( siibp )						\
}
					/* SIIBUF Queue/List Manipulation
					 * Macros
					 */
#define	Rinsert_rbusy( pccb, siibp ) {					\
    Insert_siib( siibp, *pccb->Rbusy )					\
}

					/* SIIBUF Reception Status Macros
					 */
#define	Rcheck_datreq( pccb, siibp, tpi, msibp, event, lpc ) {		\
    if( Rcheck_vc( pccb, siibp, tpi )) {				\
	Rincr_rseqno( pccb, tpi )					\
	if( Rcheck_pktlen( siibp, Min_datreq_size, Max_datreq_size )) {	\
	    Remove_mfreeq( pccb, msibp )				\
	    if( Msibp == NULL ) {					\
		u_long	tport = siibp->cmdblk.src;			\
		Rget_free_msg( pccb, tport, msibp )			\
		if( Msibp == NULL ) {					\
		    lpc = 1;						\
		    event = SE_MFQE;					\
		}							\
	    }								\
	} else {							\
	    event = SE_INVRPKTSIZE;					\
	}								\
    } else {								\
	Rcheck_oseqmsg( pccb, siibp, tpi, event )			\
    }									\
}
#define	Rcheck_dg( pccb, siibp, msibp, event ) {			\
    if( Rcheck_pktlen( siibp, Min_dg_size, Max_dg_size )) {		\
	Remove_dfreeq( pccb, msibp )					\
    } else {								\
	event = SE_INVRPKTSIZE;						\
    }									\
}
#define	Rcheck_id( pccb, siibp, msibp, event ) {			\
    if( Rcheck_pktlen( siibp, Min_id_size, Max_id_size )) {		\
	if( Rcheck_penable( siibp->Rpkt.id.Port_state )) {		\
	    if( siibp->Rpkt.id.Port_type[ 0 ] != HPT_SII ) {		\
		Remove_dfreeq( pccb, msibp )				\
	    }								\
	} else {							\
	    event = E_RPORTSTATE;					\
	}								\
    } else {								\
	event = SE_INVRPKTSIZE;						\
    }									\
}
#define	Rcheck_idreq( pccb, siibp, msibp, event ) {			\
    if( Rcheck_pktlen( siibp, Min_idreq_size, Max_idreq_size )) {	\
	Remove_dfreeq( pccb, msibp )					\
    } else {								\
	event = SE_INVRPKTSIZE;						\
    }									\
}
#define	Rcheck_msg( pccb, siibp, tpi, msibp, event, lpc ) {		\
    if( Rcheck_vc( pccb, siibp, tpi )) {				\
	Rincr_rseqno( pccb, tpi )					\
	if( Rcheck_pktlen( siibp, Min_msg_size, Max_msg_size )) {	\
	    Remove_mfreeq( pccb, msibp )				\
	    if( Msibp == NULL ) {					\
		u_long	tport = siibp->cmdblk.src;			\
		Rget_free_msg( pccb, tport, msibp )			\
		if( Msibp == NULL ) {					\
		    lpc = 1;						\
		    event = SE_MFQE;					\
		}							\
	    }								\
	} else {							\
	    event = SE_INVRPKTSIZE;					\
	}								\
    } else {								\
	Rcheck_oseqmsg( pccb, siibp, tpi, event )			\
    }									\
}
#define	Rcheck_oseqmsg( pccb, siibp, tpi, event ) {			\
    if( Rpstatus_vc( tpi )) {						\
	register u_long pseqno = Ns_msih( siibp->Rpkt.ph );		\
	Incr_seqno( pseqno )						\
	if( tpi->rseqno != pseqno ) {					\
	    event = SE_OSEQMSG;						\
	}								\
    }									\
}
#define	Rcheck_penable( port_state )					\
    ( port_state == PS_ENAB || port_state == PS_ENAB_MAINT )
#define	Rcheck_pktlen( siibp, min, max )				\
    ( siibp->cmdblk.length >= pccb->min && siibp->cmdblk.length <= pccb->max )
#define	Rcheck_sntdat( pccb, siibp, tpi, msibp, event, lpc ) {		\
    register MSIBD	*bdp;						\
    if( Rcheck_vc( pccb, siibp, tpi )) {				\
	Rincr_rseqno( pccb, tpi )					\
	if( Rcheck_pktlen( siibp, Min_sntdat_size, Max_sntdat_size )) {	\
	    if( Check_bname( siibp->Rpkt.sntdat.rbname, bdp )) {	\
		pccb->Rdmap.ssize = siibp->cmdblk.length - SNTDAT_MINSIZE; \
		Copy_long( &siibp->Rpkt.sntdat.rboff[ 0 ], &pccb->Rdmap.Sboff)\
		if( Check_bsize( bdp, pccb->Rdmap )) {			\
		    Setup_dmap( bdp, pccb->Rdmap )			\
		    Remove_mfreeq( pccb, msibp )			\
		    if( Msibp == NULL ) {				\
			u_long	tport = siibp->cmdblk.src;		\
			Rget_free_msg( pccb, tport, msibp )		\
			if( Msibp == NULL ) {				\
			    lpc = 1;					\
			    event = SE_MFQE;				\
			}						\
		    }							\
		} else {						\
		    lpc = 1;						\
		    event = SE_INVBSIZE;				\
		}							\
	    } else {							\
		lpc = 1;						\
		event = SE_INVBNAME;					\
	    }								\
	} else {							\
	    event = SE_INVRPKTSIZE;					\
	}								\
    } else {								\
	Rcheck_oseqmsg( pccb, siibp, tpi, event )			\
    }									\
}
#define	Rcheck_vc( pccb, siibp, tpi )					\
    ( Rpstatus_vc( tpi ) && tpi->rseqno == Ns_msih( siibp->Rpkt.ph ))
#define	Rget_free_msg( pccb, port, msibp ) {				\
    register siibq	*siibp;						\
    register SCSH	*scsbp;						\
    Lock_xfp( pccb )							\
    Remove_mfreeq( pccb, msibp )					\
    if( Msibp == NULL && ( siibp = pccb->Xbusy ) && Siibp->save_msib ) {\
	do	{							\
	    if( Siibp->cmdblk.dst == port ) {				\
		Msibp = Siibp->save_msib;				\
		if( Vc_msib( Msibp ) && Free_msib( Msibp )) {		\
		    if(( scsbp = msi_alloc_msg( pccb ))) {		\
			Set_discard( Msibp )				\
			Msibp = Scs_to_pd( pccb, scsbp );		\
		    } else {						\
			Msibp = NULL;					\
		    }							\
		    break;						\
		} else {						\
		    Msibp = NULL;					\
		}							\
	    }								\
	}	while(( siibp = siibp->flink ) != pccb->Xfree );	\
    }									\
    Unlock_xfp( pccb )							\
}
#define	Rinvalid_srcaddr( pccb, siibp )					\
    ( siibp->cmdblk.src >= MSI_MAXNUM_PORT )
#define	Rreceive_abort( pccb, siibp )					\
    ( ( siibp->cmdblk.opcode & CMDOPC_GROUPC ) != CMDOPC_GROUPC	  ||	\
      ( siibp->cmdblk.opcode & CMDOPC_REQACK ) != CMDOPC_REQACK	  ||	\
      siibp->cmdblk.dst != Scaaddr_lob( pccb->lpinfo.addr ) ||		\
      siibp->cmdblk.length < 2				    ||		\
      Rinvalid_srcaddr( pccb, siibp ))
#define	Rreceive_good( pccb, siibp )					\
    ( siibp->cmdblk.status == ST_SUCCESS &&				\
      !Rreceive_abort( pccb, siibp ))

/* Transmit Fork Process Macros.
 */
					/* Fork Process Control Macros
					 */
#define	Xstart_xfp( pccb ) {						\
    register u_long	unlock;						\
    if( !Test_pccb_lock( pccb )) {					\
	Lock_pccb( pccb )						\
	unlock = 1;							\
    } else {								\
	unlock = 0;							\
    }									\
    if( Lpstatus_xfork( pccb ) == 0 ) {					\
	Set_xfork( pccb )						\
	Kfork( &pccb->Xforkb, msi_xfp, pccb )				\
    }									\
    if( unlock ) {							\
	Unlock_pccb( pccb )						\
    }									\
}
#define	Xstart_xfp_timer( pccb ) {					\
    if( !pccb->Lpstatus.timer ) {					\
	pccb->Lpstatus.timer = 1;					\
	( void )timeout( msi_xfp_timer, ( u_char * )pccb, 1 );		\
    }									\
}
#define	Xfp_started( pccb, save_ipl ) {					\
    save_ipl = Splscs();						\
    Lock_pccb( pccb )							\
    Lock_xfp( pccb )							\
    if( Lpstatus_xfork( pccb )) {					\
	Clear_xfork( pccb )						\
	Unlock_pccb( pccb )						\
	if( Lpstatus_active( pccb ) == 0 || pccb->Xbusy == NULL ) {	\
	    Unlock_xfp( pccb )						\
	    ( void )splx( save_ipl );					\
	    return;							\
	}								\
    } else {								\
	( void )panic( PANIC_XFPFB );					\
    }									\
}
#define	Xfp_timer_started( pccb, save_ipl ) {				\
    save_ipl = Splscs();						\
    Lock_xfp( pccb )							\
    if( pccb->Lpstatus.timer ) {					\
	pccb->Lpstatus.timer = 0;					\
	if( Lpstatus_active( pccb ) == 0 ) {				\
	    Unlock_xfp( pccb )						\
	    ( void )splx( save_ipl );					\
	    return;							\
	}								\
    } else {								\
	( void )panic( PANIC_XFP_TIMER );				\
    }									\
}

					/* MSIB/SIIBUF Transmit Status Macros
					 */
#define Xsii_busy( pccb )						\
    ( *pccb->Msidscr & MSIDSCR_OUT || 					\
    ( *pccb->Msiisr3 & MSIISR3_WR0 ) == MSIISR3_WR0 ) 
#define	Xabort_xmt( tpi, msibp )					\
    (( Vc_msib( msibp ) && !Rpstatus_vc( tpi ))		||		\
     ( Rpstatus_path( tpi ) == 0 &&					\
       msibp->Ph.opcode != IDREQ &&					\
       msibp->Ph.opcode != ID ))
#define	Xfalse_nack( pccb, siibp )					\
    ( siibp->cmdblk.status == ST_NACK_IS_ACK &&				\
      *pccb->Msiilp != siibp->cmdblkaddr )
#define	Xignore_xmtfail( pccb, siibp, tpi )				\
    ( siibp->cmdblk.status == 0 &&					\
      !Rpstatus_vc( tpi )	&&					\
      siibp->Xpkt.ph.opcode == IDREQ )
#define	Xcheck_lretdat( pccb, msibp, event ) {				\
    register MSIBD	*bdp;						\
    if( Check_bname( msibp->Lretdat.sbname, bdp )) {			\
	if( U_long( msibp->Lretdat.length[ 0 ]) > msibp->Retdat_size ) {\
	    pccb->Xdmap.ssize = msibp->Retdat_size;			\
	} else {							\
	    pccb->Xdmap.ssize = U_long( msibp->Lretdat.length[ 0 ]);	\
	    Set_lp( msibp->Ph )						\
	}								\
	pccb->Xdmap.Sboff = U_long( msibp->Lretdat.sboff[ 0 ]);		\
	if( Check_bsize( bdp, pccb->Xdmap )) {				\
	    Setup_dmap( bdp, pccb->Xdmap )				\
	} else {							\
	    event = SE_INVBSIZE;					\
	}								\
    } else {								\
	event = SE_INVBNAME;						\
    }									\
}

					/* SIIBUF Header Formating Macros
					 */
#define	Xformat_cmdblk( pccb, siibp, msibp ) {				\
    siibp->cmdblk.status = 0;						\
    siibp->cmdblk.command = ( u_short )msibp->Rport_addr;		\
    siibp->cmdblk.opcode = CMDOPC_OPCODE;				\
    siibp->cmdblk.dst = msibp->Rport_addr;				\
    siibp->cmdblk.src = Scaaddr_lob( pccb->lpinfo.addr );		\
    siibp->cmdblk.length = msibp->Framelength;				\
}
#define	Xformat_siib( siibp, msibp ) {					\
    ( void )bcopy(( u_char * )&msibp->Ph,				\
		   ( u_char * )&siibp->Xpkt.ph,				\
		   msibp->Framelength );				\
}
#define	Xformat_xlink( siibp, msibp ) {					\
    siibp->Xlink.addr = siibp->xpktaddr;				\
    siibp->Xlink.len = msibp->Framelength;				\
}
#define	Xreset_cmdblk( siibp ) {					\
    siibp->cmdblk.status = 0;						\
    Interrupt_disable( siibp )						\
}
#define	Xset_xseqno( dpi, siibp ) {					\
    U_long( siibp->Xpkt.ph ) |= dpi->xseqno;				\
    Incr_seqno( dpi->xseqno )						\
}

					/* SIIBUF Queue/List Manipulation
					 * Macros
					 */
#define	Xflush_xbusy( pccb ) {						\
    register siibq	*siibp;						\
    register MSIB	*msibp;						\
    for( siibp = pccb->Xbusy;						\
	 ( msibp = Siibp->save_msib );					\
	 siibp = siibp->flink ) {					\
	( void )msi_dealloc_pkt( msibp );				\
	Siibp->save_msib = NULL;					\
    }									\
    pccb->Xbusy = siibp;						\
}
#define	Xfree_siib( pccb, siibp ) {					\
    siibp->save_msib = NULL;						\
    pccb->Xbusy = siibp->flink;						\
}
#define	Xinsert_xbusy( pccb, siibp ) {					\
    Insert_siib( siibp, *pccb->Xbusy )					\
}
#define	Xinsert_xfree( pccb, siibp ) {					\
    Insert_siib( siibp, *pccb->Xfree )					\
}
#define	Xinsert_xretryq( tpi, msibp ) {					\
    Insert_msib( msibp, tpi->xretryq )					\
}
#define	Xreset_xbusy( pccb, siibp ) {					\
    do	{								\
	Xreset_cmdblk( Siibp )						\
    }	while(( siibp = siibp->flink ) != pccb->Xfree );		\
}

					/* Transmit Completion Macros
					 */
#define	Xdiscard_msib( pccb, msibp ) {					\
    if( Free_msib( msibp )) {						\
	Xfree_msib( pccb, msibp )					\
    } else {								\
	( void )msi_dealloc_pkt( msibp );				\
    }									\
}
#define	Xfree_msib( pccb, msibp ) {					\
    if( Vc_msib( msibp )) {						\
	Reset_msib( msibp )						\
	Insert_mfreeq( pccb, msibp )					\
    } else {								\
	Reset_msib( msibp )						\
	Insert_dfreeq( pccb, msibp )					\
    }									\
}
#define	Xnext_lretdat( pccb, msibp ) {					\
    register u_long	ssize;						\
    ssize = msibp->Framelength - pccb->Retdat_ovhd;			\
    if(( long )( U_long( msibp->Lretdat.length[ 0 ]) -= ssize ) > 0 ) {	\
	msibp->Framelength = pccb->Retdat_ovhd;				\
	U_long( msibp->Lretdat.sboff[ 0 ]) += ssize;			\
	U_long( msibp->Lretdat.rcvboff[ 0 ]) += ssize;			\
	Copy_long( &msibp->Lretdat.rcvboff[ 0 ], &msibp->Lretdat.rboff[ 0 ])\
	Insert_comql( pccb, msibp )					\
    } else {								\
	( void )panic( PANIC_RETDAT );					\
    }									\
}
#define	Xproc_pathfail( pccb, siibp, msibp, tpi, port, xq ) {		\
    register PB		*pb;						\
    u_long		cport = port;					\
    Clear_rpdip( tpi )							\
    tpi->xretrys = 0;							\
    pccb->Xbusy = NULL;							\
    Unlock_xfp( pccb )							\
    Lock_pccb( pccb )							\
    if(( pb = cippd_get_pb( pccb, ( SCSH * )&cport, NO_BUF ))) {	\
	Lock_pb( pb )							\
	( void )msi_log_packet( pccb,					\
				pb,					\
				&Siibp->cmdblk,				\
				&Siibp->Xpkt.ph,			\
				Siibp->cmdblk.length,			\
				E_NOPATH,				\
				PATH_EVENT );				\
	if( pb->pinfo.state != PS_PATH_FAILURE ) {			\
	    ( void )cippd_crash_pb( pccb, pb, E_PD, 0, NULL );		\
	}								\
	Unlock_pb( pb )							\
    }									\
    Lock_xfp( pccb )							\
    Unlock_pccb( pccb )							\
    do	{								\
	if( Siibp->cmdblk.dst == port ) {				\
	    Msibp = Siibp->save_msib;					\
	    Xdiscard_msib( pccb, Msibp )				\
	    Siibp->save_msib = NULL;					\
	    if( pccb->Xbusy ) {						\
		register siibq *prev_siibp = siibp->blink;		\
		Unthread_cmdblk( Siibp )				\
		Remove_siib( Siibp )					\
		Xinsert_xbusy( pccb, siibp )				\
		Thread_cmdblk( Siibp, Siibp )				\
		Thread_cmdblk( pccb->Xbusy, pccb->Xbusy )		\
		siibp = prev_siibp;					\
	    }								\
	} else if( pccb->Xbusy == NULL ) {				\
	    pccb->Xbusy = siibp;					\
	}								\
    }	while(( siibp = siibp->flink ) != pccb->Xfree );		\
    if( pccb->Xbusy == NULL ) {						\
	pccb->Xbusy = pccb->Xfree;					\
    }									\
    for( msibp = xq.flink; msibp != &xq; msibp = msibp->flink ) {	\
	if( Msibp->Rport_addr == port ) {				\
	    register msibq	*prev_msibp = msibp->blink;		\
	    Remove_msib( Msibp )					\
	    ( void )msi_dealloc_pkt( Msibp );				\
	    msibp = prev_msibp;						\
	}								\
    }									\
}
#define	Xretdat_error( pccb, msibp, event, xq ) {			\
    siibq		*cache_xbusy = pccb->Xbusy;			\
    pccb->Xbusy = NULL;							\
    Unlock_xfp( pccb )							\
    Lock_pccb( pccb )							\
    Lock_xfp( pccb )							\
    pccb->Xbusy = cache_xbusy;						\
    pccb->Lpstatus.optlpcinfo = 1;					\
    pccb->Lpcinfo.pkth = &Msibp->Ph;					\
    pccb->Lpcinfo.pktsize = sizeof( MSI_LRETDAT );			\
    pccb->Lpcinfo.pport_addr = Msibp->Rport_addr;			\
    ( void )msi_crash_lport( pccb, event, NULL );			\
    pccb->Lpstatus.optlpcinfo = 0;					\
    Unlock_pccb( pccb )							\
    ( void )msi_dealloc_pkt( Msibp );					\
    while(( msibp = xq.flink ) != &xq ) {				\
	Remove_msib( Msibp )						\
	( void )msi_dealloc_pkt( Msibp );				\
    }									\
}
#define	Xtransmit_done( pccb, msibp, tpi, xq ) {			\
    tpi->xretrys = 0;							\
    if( Free_msib( msibp )) {						\
	if( msibp->Ph.opcode != RETDAT || Lp_msih( msibp->Ph )) {	\
	    Xfree_msib( pccb, msibp )					\
	} else {							\
	    Xnext_lretdat( pccb, msibp )				\
	}								\
    } else {								\
	if( msibp->Ph.opcode == MSG ) {					\
	    Insert_msib( msibp, xq )					\
	} else {							\
	    if( msibp->Ph.opcode == IDREQ ) {				\
		Set_rppath( tpi )					\
	    }								\
	    ( void )msi_dealloc_pkt( msibp );				\
	}								\
    }									\
}

					/* Transmit Retry Control Macros
					 */
/* Compute Random Delay Interval
 *
 * A random delay interval between 10 and 100 msecs in 10 msec units is
 * picked in exponentially decreasing order of probability( ie- 10 msec is
 * chosen 1/2 of the time, 20 msec 1/4 of the time, etc... ).  A simple
 * linear congruential random number generator is employed by executing the
 * following equation up to 10 times or until the sign bit of the longword size
 * "seed" has been set:
 *
 *			seed = ( seed * base ) + 1;
 *
 * The number of times this computation takes place represents the number of
 * 10 msecs to delay.
 */
#define	Xcompute_delay( seed, delay ) {					\
    for( delay = 1; delay <= 10; delay++ ) { 				\
	seed = ( seed * RANDOM_BASE ) + 1;				\
	if(( long )seed < 0 ) {						\
	    break;							\
	}								\
    }									\
    delay *= TIMER_UNITS;						\
}
#define	Xdelay_xmts( pccb, siibp, msibp, port, tpi ) {			\
    register u_long	cached_xseqno = 0;				\
    Set_rpdip( tpi )							\
    pccb->Xbusy = NULL;							\
    do	{								\
	if( Siibp->cmdblk.dst == port ) {				\
	    Msibp = Siibp->save_msib;					\
	    if( Vc_msib( msibp ) && cached_xseqno == 0 ) {		\
		tpi->xseqno = Ns_msih( Siibp->Xpkt.ph );		\
		cached_xseqno = 1;					\
	    }								\
	    if( Msibp->Ph.opcode == RETDAT ) {				\
		msibp->Framelength = pccb->Retdat_ovhd;			\
	    }								\
	    Xinsert_xretryq( tpi, msibp )				\
	    Siibp->save_msib = NULL;					\
	    if( pccb->Xbusy ) {						\
		register siibq *prev_siibp = siibp->blink;		\
		Unthread_cmdblk( Siibp )				\
		Remove_siib( Siibp )					\
		Xinsert_xbusy( pccb, siibp )				\
		Thread_cmdblk( Siibp, Siibp )				\
		Thread_cmdblk( pccb->Xbusy, pccb->Xbusy )		\
		siibp = prev_siibp;					\
	    }								\
	} else if( pccb->Xbusy == NULL ) {				\
	    pccb->Xbusy = siibp;					\
	}								\
    }	while(( siibp = siibp->flink ) != pccb->Xfree );		\
    if( pccb->Xbusy == NULL ) {						\
	pccb->Xbusy = pccb->Xfree;					\
    }									\
    Xcompute_delay( pccb->Randomseed, tpi->xretry_timer )		\
    Xstart_xfp_timer( pccb )						\
}
/* Initial seed chosen to be unique by stuffing in local port number in
 * lower order byte
 */
#define	Xinit_generator( pccb ) {					\
    pccb->Randomseed = (( time.tv_sec & ~0xff ) +			\
			Scaaddr_lob( pccb->lpinfo.addr ));		\
}
#define	Xretry_xmt( pccb, siibp, msibp, tpi, xq ) {			\
    if( ++tpi->xretrys > IMMED_RETRYS ) {				\
	register u_long	port = Siibp->cmdblk.dst;			\
	if( tpi->xretrys <= MAX_RETRYS ) {				\
	    Xdelay_xmts( pccb, siibp, Msibp, port, tpi )		\
	} else {							\
	    Xproc_pathfail( pccb, siibp, msibp, tpi, port, xq )		\
	}								\
    }									\
}

/* GVP( Generic VaxPort ) Emulation Data Structure Definitions and Macros 
 */
typedef	struct	_gvpbd	{		/* GVP Buffer Descriptor	     */
    u_short	  boff	: 9;		/* Offset into page of buffer        */
    u_short		: 6;		/* MBZ	 			     */
    u_short	  valid	: 1;		/* Buffer Descriptor valid flag      */
#define	GVPBD_VALID	0x8000
    u_short	  key;			/* Key( sequence number )	     */
    u_long	  bsize;		/* Size of buffer in bytes	     */
    struct pte	  *bpte;		/* Virtual address of buffer PTE     */
    struct _gvpbd *next_free_bd;	/* Next free buffer descriptor	     */
} GVPBD;

					/* GVP Buffer Descriptor Database    */
typedef	struct	_gvpbddb {		/*  Header			     */
    GVPBD	*free_bd;		/* Free GVP BD list head	     */
    GVPBD	*bdt;			/* Buffer Descriptor table pointer   */
    u_short	size;			/* Size of data structure	     */
    u_char	type;			/* Structure type		     */
    u_char		:  8;
    u_long		: 32;		/* Align on octaword boundary MUSTBE */
} GVPBDDB;

					/* Shorthand Notations for Contants
					 */
#define	DYN_MSIDG	DYN_GVPDG
#define	DYN_MSIMSG	DYN_GVPMSG
#define	MSIBD_VALID	GVPBD_VALID

					/* Shorthand Notations for Declared
					 * Data Structures
					 */
#define	msi_bddb	gvp_bddb

					/* Shorthand Notations for Data
					 * Structures and Data Structure Fields
					 */
#define	max_msibds	max_gvpbds
#define	_msibd		_gvpbd
#define	MSIBD		GVPBD
#define	_msibddb	_gvpbddb
#define	MSIBDDB		GVPBDDB
