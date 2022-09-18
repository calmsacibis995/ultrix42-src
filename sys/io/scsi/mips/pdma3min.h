/*
 * @(#)pdma3min.h	4.2	(ULTRIX)	2/26/91
 */

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
  * pdma3min.h	
  *
  * Modification History
  *
  ***********************************************************************/

/* 3min related ... */
#define	BASE_IOASIC	0x1c040000		/* motherboard IO-ASIC */
#define BASE_IOASIC_K1	0xbc040000	
#define TC0_IOASIC	0x10040000		/* FIX - this is slot 0 base */
#define TC1_IOASIC	0x14040000		/* FIX - slot 1 base addr.  */
#define TC2_IOASIC	0x18040000		/* FIX - slot 2 base addr.  */

#define SSR_MASK        0x00020200    /* DMA read, enable, C94 /RESET */
#define SSR_DMADIR      0x00040000    /* Set this bit for write, zero for read */
#define SSR_DMAENB	0x00020000    /* Set this bit to enable DMA */

#define	SIR_MASK	0x000e0204
#define SCSI_SLOT_DATA	0x0000000e		/* must write upon init */

#define	DB0_O		0x000000c0		/* offset to data buffer 0 */
#define	DB1_O		0x000000d0		/* offset to data buffer 1 */
#define	DB2_O		0x000000e0		/* offset to data buffer 2 */
#define	DB3_O		0x000000f0		/* offset to data buffer 3 */
#define SSR_O           0x00000100    /* IOASIC System support reg. */
#define	SIR_O           0x00000110		/* IOASIC Interrupt reg. */
#define SIMR_O          0x00000120		/* IOASIC Int. mask reg. */
#define SCSI_DMASLOT_O	0x00000170		/* IOASIC SCSI DMA slot reg. */
#define	SCSI_CTRL_O	0x000001b0		/* new SCSI control register */
#define SCSI_DATA0_O	0x000001c0		/* new data register-1 of 2 */
#define SCSI_DATA1_O	0x000001d0		/* 2 of 2 */

#define AND_O		0x00001000		/* and with TC data */
#define OR_O		0x00002000		/* or with TC data */

#define IOA_ADDRMASK	0x1ffffffc
#define CREG_BUSG_M	0x00000003		/* Byte usage mask (SCSI_CTRL */
#define CREG_DMA_M	0x00000004		/* Direction mask (1=write) */

#define IOA_S_DMAP_O	0x00000000
#define	IOA_S_DMABP_O	0x00000010

#define BASE_SCSI	0x1c300000		/* motherboard 53C94 */
#define TC0_SCSI	0x10300000		/* FIX - slot 0 guess */
#define TC1_SCSI	0x14300000		/* FIX - slot 1 guess */
#define TC2_SCSI	0x18300000		/* FIX - slot 2 guess */

/*#define PMAZ_BA_CFG3 ASC_C3_T8 |  ASC_C3_ALTDMA  old ioasic */
#define PMAZ_BA_CFG3 	0

#define	SCSI_DRDY	0x00000004
#define SCSI_C94	0x00000200
#define SCSI_MERR	0x00020000
#define SCSI_OERR	0x00040000
#define SCSI_DBPL	0x00080000

#define DATEP_INCOMPLETE	0
#define DATEP_COMPLETE		1
#define DATEP_RETRY		2

#ifndef REVERSEM
typedef struct ioa_ssr
    {
    unsigned char leds;
    unsigned      lance_reset:1;
    unsigned      C94_reset:1;
    unsigned      rtc_reset:1;
    unsigned      scc_reset:1;
    unsigned      reserved_1:1;
    unsigned      txdis:2;
    unsigned      diagdn:1;
    unsigned      lance_dma:1;
    unsigned      scsi_dma:1;
    unsigned      scsi_dir:1;
    unsigned      reserved_2:5;
    unsigned      com2_rxdma:1; 
    unsigned      com2_txdma:1;
    unsigned      com1_rxdma:1;
    unsigned      com1_txdma:1;
    } IOA_SSR;

typedef struct ioa_sir
    {
    unsigned pbno:1;		/*SIR<0>*/
    unsigned pbnc:1;
    unsigned scsi_drdy:1;	/*SIR<2>*/
    unsigned reserved_1:1;
    unsigned pswarn:1;		/*SIR<4>*/
    unsigned toy:1;
    unsigned scc0:1;
    unsigned scc1:1;
    unsigned ether:1;		/*SIR<8>*/
    unsigned scsi_c94:1;
    unsigned nrmod:1;
    unsigned reserved_2:1;
    unsigned cpuiowt:1;		/*SIR<12>*/
    unsigned reserved_3:1;
    unsigned nvr:1;
    unsigned reserved_4:1;
    unsigned lance_merr:1;	/*SIR<16>*/
    unsigned scsi_merr:1;	/*SIR<17>*/
    unsigned scsi_oerr:1;	/*SIR<18>*/
    unsigned scsi_dbpl:1;	/*SIR<19>*/
    unsigned reserved_5:4;	/*SIR<23:20 >*/
    unsigned com2_oerr:1;	/*SIR<24>*/
    unsigned com2_hp:1;
    unsigned com2_merr:1;
    unsigned com2_pe:1;
    unsigned com1_oerr:1;	/*SIR<28>*/
    unsigned com1_hp:1;
    unsigned com1_merr:1;	/*SIR<30>*/
    unsigned com1_pe:1;		/*SIR<31>*/
    } IOA_SIR;

typedef struct ioa_s_dmap {
    unsigned            reserved:5;
    unsigned            addr:27;
    } IOA_S_DMAP ;

typedef struct ioa_s_dmabp
    {
    unsigned            reserved:5;
    unsigned            addr:27;
    } IOA_S_DMABP ;

#else

typedef struct ioa_ssr
    {
    unsigned char leds;
    unsigned      com1_txdma:1;
    unsigned      com1_rxdma:1;
    unsigned      com2_txdma:1;
    unsigned      com2_rxdma:1; 
    unsigned      reserved_2:5;
    unsigned      scsi_dir:1;
    unsigned      scsi_dma:1;
    unsigned      lance_dma:1;
    unsigned      diagdn:1;
    unsigned      txdis:2;
    unsigned      reserved_1:1;
    unsigned      scc_reset:1;
    unsigned      rtc_reset:1;
    unsigned      C94_reset:1;
    unsigned      lance_reset:1;
    } IOA_SSR;

typedef struct ioa_sir
    {

    unsigned com1_pe:1;		/*SIR<31>*/
    unsigned com1_merr:1;	/*SIR<30>*/
    unsigned com1_hp:1;
    unsigned com1_oerr:1;	/*SIR<28>*/
    unsigned com2_pe:1;
    unsigned com2_merr:1;
    unsigned com2_hp:1;
    unsigned com2_oerr:1;	/*SIR<24>*/
    unsigned reserved_5:4;	/*SIR<23:20 >*/
    unsigned scsi_dbpl:1;	/*SIR<19>*/
    unsigned scsi_oerr:1;	/*SIR<18>*/
    unsigned scsi_merr:1;	/*SIR<17>*/
    unsigned lance_merr:1;	/*SIR<16>*/
    unsigned reserved_4:1;
    unsigned nvr:1;
    unsigned reserved_3:1;
    unsigned cpuiowt:1;		/*SIR<12>*/
    unsigned reserved_2:1;
    unsigned nrmod:1;
    unsigned scsi_c94:1;
    unsigned ether:1;		/*SIR<8>*/
    unsigned scc1:1;
    unsigned scc0:1;
    unsigned toy:1;
    unsigned pswarn:1;		/*SIR<4>*/
    unsigned reserved_1:1;
    unsigned scsi_drdy:1;	/*SIR<2>*/
    unsigned pbnc:1;
    unsigned pbno:1;		/*SIR<0>*/
    } IOA_SIR;

typedef struct ioa_s_dmap {
    unsigned            addr:27;
    unsigned            reserved:5;
    } IOA_S_DMAP ;

typedef struct ioa_s_dmabp
    {
    unsigned            addr:27;
    unsigned            reserved:5;
    } IOA_S_DMABP ;

#endif

/*  The following is related to the DAT table design */

typedef struct fragbuf
    {
    char        top[8];
    char        bot[8]; 
    }FRAGBUF;

typedef struct dtent
    {
    unsigned int length;		/* length of table entry */
    char        *addr;		/* pointer to table entry */
    char        *uadr;	     /* pointer to real buffer if local xfer */
    unsigned	*iadr;		/* address for IOASIC */
    char         completed;	/* entry completion status flag */
    char         dir;    /* DMA direction */
    } DTENT;

#define IOASIC_WRITE	0
#define IOASIC_READ	1
#define IOASIC_UNDEF	2

#define MAX_DATTBL_ENT    256

#define DATTBL_SIZ	MAX_DATTBL_ENT*sizeof(DTENT)

extern int printstate;

extern int pmaz_ba_init();
extern int pmaz_ba_setup();
extern int pmaz_ba_start();
extern int pmaz_ba_cont();
extern int pmaz_ba_end();
extern int pmaz_ba_flush();
extern int bcopy();
extern int bzero();
unsigned *ioa_addrcvt ( char * );
void *backcvt( void * );
int frag_buf_load( PDMA * );
flushfifo( struct sz_softc * );
ioasicint( struct sz_softc *, int, int );
int is_local_xfer( PDMA * );
dumptbl( DTENT * );
dumpent( DTENT * );
blddatent( DTENT *, unsigned long, char *, char *, int );
blddattbl( int, PDMA *, unsigned long, char *, int );
int caldatent( char *, long );
flush_fragbuf( PDMA *, unsigned long );
dmapload( struct sz_softc *, int, unsigned int * );
ssrpload( struct sz_softc *, int );
dumphex( char *, unsigned );
int getdbuffer( struct sz_softc *, void * );
int getdbcount( struct sz_softc *, int );
void setscsictrl( struct sz_softc *, int );
int flushdb( struct sz_softc *, int );
