/*
 *    laps.h	Local Area Printing Service definitions
 *    v1.00	27-Jan-1986
 *
 *    Edit History
 *    V3.0-36   03-Jun-1988 10:35  CGI PCO 322 - new status message block
 *          type for handling returnstatus
 *    V3.0-33   13-May-1988 11:00  CGI PCO 321 - add capability for informing
 *          client of resources loaded by server at boot time.
 *          Change protocol version to 3.0   
 *    V1.0-01   19-Feb-1987 17:00  MJD PCO 18 - add IO$M_INTERRUPT capability.
 *    V1.0-00   04-Feb-1987 09:12  MJD Add protocol compatability for LAPS V1.1.
 */

typedef unsigned char FIELD8;		/* 8 bit protocol field */
typedef unsigned short FIELD16;		/* 16 bit protocol field */
typedef unsigned long FIELD32;		/* 32 bit protocol field */

/*
 * LAPS message header
 */
struct msg_hdr {			/* message header */
	FIELD8	msg_type;		/* type of message */
	FIELD8	msg_blkcnt;		/* number of blocks in message */
	FIELD16 msg_len;		/* length of data in message */
};
#define MSG_HDR_LEN sizeof (struct msg_hdr)

/*
 * LAPS block header
 */
struct blk_hdr {			/* block header */
	FIELD8	blk_type;		/* type of block */
	FIELD8	blk_len[2];		/* length of data in block */
};
#define BLK_HDR_LEN sizeof (struct blk_hdr)

/*
 * LAPS protocol version
 */
struct pro_ver {			/* protocol version */
	FIELD8	pro_major;		/* major version number */
	FIELD8	pro_minor;		/* minor version number */
	FIELD8	pro_edit;		/* edit number */
	FIELD8	pro_user_mod;		/* user modification number */
};
#define PRO_VER_LEN sizeof (struct pro_ver)

/* 
 * LAPS Version V0.3 (alias V1.0) 
 */
#define PRO_VER_MAJOR_LO 0
#define PRO_VER_MINOR_LO 3
#define PRO_VER_EDIT_LO	 0
#define PRO_VER_USER_LO	 0

/*  
 * LAPS Version V3.0              
 */
#define PRO_VER_MAJOR_HI 3
#define PRO_VER_MINOR_HI 0
#define PRO_VER_EDIT_HI  0 
#define PRO_VER_USER_HI  0

/*
 * Condition record header
 */
struct cond_hdr {
	FIELD8	routing_info;	/* message routing info */
	FIELD8	mbz_1[3];	/* must be zero */
	FIELD32	act_code;	/* ??? */
	FIELD32	msg_code;	/* ??? */
	FIELD8	argc;		/* argument count */
	FIELD8	mbz_2[3];	/* must be zero */
};

/*
 * Condition record routing info flags
 */
#define LAPS_ROU_INTERNAL	(1 << 0)
#define LAPS_ROU_OPERATOR	(1 << 1)
#define LAPS_ROU_STA_MGR	(1 << 2)
#define LAPS_ROU_USER		(1 << 3)
#define LAPS_ROU_ERR_LOG	(1 << 4)
#define LAPS_ROU_RESOURCE	(1 << 5)
/*
 * Condition record action codes
 */
#define LAPS_ACTIGNORE		1
#define LAPS_ACTINFORM		2
#define LAPS_ACTSTOPTASK	3
#define LAPS_ACTSTOPJOB		4
#define LAPS_ACTABORT		5
#define LAPS_ACTPAUSE		6
#define LAPS_ACTRESUME		7
#define LAPS_ACTRECOVER		8

/*
 * Define the MESSAGE TYPE values
 */
#define LMT_CONNECT		1
#define LMT_DATA		2
#define LMT_CONTROL		3
#define LMT_EOJ			4
#define LMT_STATUS		5    
#define LMT_RESOURCE_FAULT	6
#define LMT_RESOURCE_STATUS	7
           
/*
 * Current LAPS message size max
 */
#define MAX_DAT_SIZE		1024
#define MAX_COND_REC_SIZE	512
#define LAPS_BUF_SIZE (MSG_HDR_LEN + (3 * BLK_HDR_LEN) + LBL_DAT_DESC + LBL_DAT_TYPE + MAX_DAT_SIZE)
#define INTERRUPT_BUF_SIZE 16
/*
 * Define the values found in the BLOCK TYPE block header field
 * and the maximum length of the various fields in bytes
 */

/*
 * CONNECT message blocks
 */
#define LBT_CON_HI_VER		1	/* highest supported protocol version */
#define LBT_CON_DATA		2 	/* additional connection data */
#define LBT_CON_LO_VER		3	/* lowest supported protocol version */
#define LBT_CON_PRINTER		4	/* printer to use for this job */

/*
 * DATA message blocks
 */
#define LBT_DAT_DESC		1	/* print or resource data? */
#define	LBL_DAT_DESC		1
#define LBT_DAT_TYPE		2  	/* what type of data or resource? */
#define	LBL_DAT_TYPE		2
#define LBT_DAT_DATA		3 	/* the data itself */
#define LBL_DAT_DATA		MAX_DAT_SIZE
#define LBT_DAT_MSGFLGS		4	/* message flags (eod for now...) */
#define LBL_DAT_MSGFLGS		1

/*
 * CONTROL message blocks
 */
#define LBT_CTL_CMD		1	/* action for server to take */
#define	LBL_CTL_CMD		1    	    
/*
 * EOJ message blocks
 */
#define LBT_EOJ_REASON		1 	/* disposition of circuit termination */
#define LBL_EOJ_REASON		1
#define LBT_EOJ_DATA		2	/* additional eoj data */
#define LBL_EOJ_DATA		4

/*
 * STATUS message blocks
 */
#define LBT_STA_PAGE_CNT	1	/* physical page count so far */
#define	LBL_STA_PAGE_CNT	4
#define LBT_STA_JOB_STATE	2	/* print device status */
#define LBL_STA_JOB_STATE	2
#define LBT_STA_CON_STATE	3	/* LAPS connection status */    
#define LBL_STA_CON_STATE	2
#define LBT_STA_PRO_VER		4	/* protocol version for this circuit */
#define	LBL_STA_PRO_VER		PRO_VER_LEN
#define LBT_STA_JOB_ID		5	/* unique server assigned job id */
#define LBL_STA_JOB_ID		4
#define LBT_STA_DATA		6	/* additional status data */
#define LBL_STA_DATA		4
#define LBT_STA_UPLINE_DATA	7	/* ISS generated data, passed to client */
    
#define LBT_STA_LO_VER		8	/* lowest server supported protocol version */

#define LBT_STA_HI_VER		9	/* highest server supported protocol version */

#define LBT_STA_RES_AVAIL       11      /* a resource loaded into server at boot */
#define LBL_STA_RES_AVAIL       322     /* maximum value of resource data */
                   
#define LBT_STA_RES_COUNT       12      /* how many resources loaded into server at boot */
#define LBL_STA_RES_COUNT       1       /* field for maximum number of resources loaded */
#define LBV_STA_RES_COUNT       255     /* maximum number of resources loaded */

#define LBT_STA_RETURNSTATUS    13      /* additional status data (returnstatus) */

/*
 * RESOURCE_FAULT message blocks
 */
#define LBT_RF_NAME_TYPE	1   	/* descriptive resource id requested */
#define LBT_RF_NAME		2     	/* file spec of resource requested */
#define LBT_RF_TYPE		3     	/* what type of resource */
#define LBT_RF_LO_VER		4	/* lowest suppported protocol ver */
#define LBT_RF_HI_VER		5	/* highest suppported protocol ver */
    
/*
 * RESOURCE_STATUS message blocks
 */
#define LBT_RS_DISPOSITION	1  	/* state of current resource fault */
#define LBL_RS_DISPOSITION	1
#define LBT_RS_STATUS		2   	/* additional disposition data */
#define LBL_RS_STATUS		4
/*
 *  no substitution currently allowed
 *
 * #define LBT_RS_SUB_FILE	3
 * #define LBT_RS_SUB_SPEC	4
 */
#define LBT_RS_LO_VER		5	/* lowest suppported protocol ver */
#define LBT_RS_HI_VER		6	/* highest suppported protocol ver */
#define LBT_RS_RES_VER		7 	/* protocol ver for this transaction */

/*
 * Define the possible values for the LBT_DAT_MSGFLGS block in the DATA 
 * message
 */
#define LBV_DAT_EOD		2

/*
 * Define the possible values for the LBT_DAT_DESC block in the DATA 
 * message
 */
#define LBV_DAT_PRINT_DATA	1
#define LBV_DAT_RESOURCE_DATA	2
             
/*
 * Define the possible values for the LBT_CTL_CMD block in the CONTROL 
 * message
 */
#define LBV_CTL_INTERRUPT	1

/*
 * Define the possible values for the LBT_EOJ_REASON block in the EOJ 
 * message
 */
#define LBV_EOJ_OK		1            
#define LBV_EOJ_ABNORMAL	2

/*
 * Define the possible values for the LBT_STA_CON_STATE block in the STATUS
 * message
 */               

#define LBV_STA_C_ACCEPTED	1
#define LBV_STA_C_PENDING	2
#define LBV_STA_C_REJECTED	3 
#define LBV_STA_C_RELEASED	4
#define LBV_STA_C_JOB_ABORT	5    
#define LBV_STA_C_PROT_ERROR	6
#define LBV_STA_C_BAD_PROT	7
#define LBV_STA_C_COMPLETED	8
#define LBV_STA_C_STOP_STREAM	9
#define LBV_STA_C_DISABLED	10

/*
 * Define the possible values for the LBT_STA_JOB_STATE block in a STATUS
 * message
 */

#define LBV_STA_J_ONLINE	1
#define LBV_STA_J_OFFLINE	2
#define LBV_STA_J_DATA_ERROR	3

/*
 * Define the possible values for the LBT_RS_DISPOSITION block in a  
 * RESOURCE_STATUS message
 */

#define LBV_RS_EOS		1
#define LBV_RS_NOT_FOUND	2
/*
 *  no substitution currently allowed
 *
 * #define LBV_RS_SUBSTITUTE	3
 */

#define LBV_RS_PROT_ERROR	4
#define LBV_RS_BAD_VER		5
#define LBV_RS_AVAILABLE	6
