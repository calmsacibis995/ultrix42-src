/*
 *	@(#)uqppd.h	4.1	(ULTRIX)	7/2/90
 */


/************************************************************************
 *									*
 *			Copyright (c) 1987 - 1989 by			*
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
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 *	Modification History
 *
 *	20-Jul-1989 - map (Mark A. Parenti)
 *		Change communications area for:
 *			1) Support of XMI port
 *			2) Support of SSP scratchpad ECO
 *		Switch order of pcinfo structure to provide better
 *		alignment for XMI.
 *
 *	14-Mar-1988 - Larry Cohen
 *		Remove all references to ra_info.
 *
 *	15-Feb-1988 - map
 *		Removed ACCEPT_REQUEST buffers from port_info structure
 */

#define TENSEC	(1000)

/* Macros
 */


						/* Shorthand notations */
#define	Lpinfo		pccb->lpinfo.pd.uq
#define	Pccb		pccb->pd.uq
#define Bh		bhp->pd.uq.bh
#define	Pos_to_ppdh( uqbp )	\
	( UQPPDH * )(( u_char * )uqbp + sizeof(UQH) + sizeof(SCSH) - sizeof(UQPPDH))
#define	UQ_header( scsbp )	\
	( UQH * )(( u_char * )scsbp - sizeof( UQH ))
#define	SCS_msg_header(uqbp)	\
	( SCSH * )(( u_char * )uqbp + sizeof( UQH ))
#define	Store_connid( connid )	\
	*( u_long * )&connid
#define	Load_connid( connid ) \
	*( u_long *)&connid

/*
 * UQ Communications Area
 */

typedef struct _uqca {
	struct {
		volatile unsigned long	rsvd:16;
		volatile unsigned long	scp_size:16; /* Size of scratchpad */
	} ca_scp1;
	volatile unsigned long	ca_scp_add;	/* Address of scratchpad */
	struct {
		union {
			    struct {
				volatile unsigned long	rsvd:24; /* Reserved */
				volatile unsigned long 	bdp:8;  /* Buffered data path for purge */
		            }uq;
			    struct {
				volatile unsigned long pfn:16;
				volatile unsigned long flags:4;
				volatile unsigned long psi:4;
				volatile unsigned long rsvd:8;
			    } xmi;
			}ca_busdep_un;
	}ca_busdep;
	volatile unsigned short	ca_cmdint; /* command queue transition interrupt flag */
	volatile unsigned short	ca_rspint; /* response queue transition interrupt flag */
	volatile unsigned long	ca_rspdsc[NRSP];/* response descriptors */
	volatile unsigned long	ca_cmddsc[NCMD];/* command descriptors 	*/
} UQCA;

#define	ca_ringbase	ca_rspdsc[0]
#define ca_bdp		ca_busdep.ca_busdep_un.uq.bdp
#define	ca_xmi		ca_busdep.ca_busdep_un.xmi
#define	ca_scp_size	ca_scp1.scp_size

#define	UQ_SCP_SIZE	0x40	/* Size of controller scratchpad	*/
typedef struct _uqscp {
	volatile char scrpad[UQ_SCP_SIZE];
} UQ_SCP;

/*
 * MSCP packet info
 */
typedef	struct uq_phdr {
	u_long	uqp_msglen  : 16; /* length of application message	*/
	u_long	uqp_credits : 4; /* credits 				*/
	u_long	uqp_msgtype : 4; /* message type 			*/
	u_long	uqp_cid     : 8; /* connection id 			*/
}UQPPDH;


typedef	struct	_uqh	{	/* UQ port header			*/
	struct	_uqbq	*flink;	/* Port buffer queue pointers		*/
	struct	_uqbq	*blink;
	long	ua;		/* Address used in ring entry for this	*/
				/* packet				*/
} UQH;

typedef	struct	_app_buf {
	union {
	MSCP_MAXBUF _mscp_buf;	/* Max of MSCP buffers			*/
	CONN_REQ    _con_req;	/* Biggest SCS buffer			*/
	}un;
} APP_BUF;

typedef	struct	uq_buf {	/* UQ port message buffer		*/
	struct	_uqh	uqh;	/* UQ specific info			*/
	struct	_scsh	scsh;	/* SCS header				*/
	APP_BUF app_buf;	/* Max of MSCP and SCS buffers		*/
} UQBUF;

typedef	struct	_uq {
	volatile struct	_uqca	uqca;	/* Communications area		*/
	UQBUF	uq_buf[NBUF]; 	/* Message buffers			*/
} UQ;


/* 	Port Info Block
 */
struct	port_info {


	UQ	uq;		/* Rings and buffers			*/
	PCCB	*pc_ptr;	/* Pointer to PCCB			*/
};

