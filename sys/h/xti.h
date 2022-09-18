/*
 * @(#)xti.h	4.1 7/2/90 XTI.H
 *
 *
 */

/* 
 *   History:
 *
 *  03/07/88    mcmenemy Update to Revision 2 (24-feb-88) at GRENOBLE
 *  04/12/88    mcmenemy Update for BL2
 *  08/25/88    mcmenemy Update to Final Draft for XPG 3
 *  12/02/88    mcmenemy Fix error name 
 *  07/14/89    mcmenemy Fix typo in iso structure
 */

/*
 *	The following are the error codes needed by both the kernel
 *	level transport providers and the user level library
 */

#define TBADADDR	1	/* incorrect addr. format */
#define TBADOPT		2	/* incorrect option format */
#define TACCES		3	/* incorrect permissions */
#define TBADF		4	/* illegal transport fd */
#define TNOADDR		5	/* couldn't allocate addr */
#define TOUTSTATE	6	/* out of state */
#define TBADSEQ		7	/* bad call sequence number */
#define TSYSERR		8	/* system error */
#define TLOOK		9	/* event requires attention */
#define TBADDATA	10	/* illegal amount of data */
#define TBUFOVFLW	11	/* buffer not large enough */
#define TFLOW		12	/* flow control */
#define TNODATA		13	/* no data */
#define TNODIS		14 	/* discon_ind not found on q */
#define TNOUDERR	15	/* unitdata error not found */
#define TBADFLAG	16	/* bad flags */
#define TNOREL		17	/* no ord rel found on q */
#define TNOTSUPPORT	18	/* primitive not supported */
#define TSTATECHNG	19	/* state is in process of changing */
#define TNOSTRUCTYPE    20      /* unsupported struct-type requested */
#define TBADNAME        21      /* invalid transport provider name */
#define TBADQLEN        22      /* qlen is zero */
#define TADDRBUSY       23      /* address in use */

/*
 *	The following are the events returned by t_look
 */

#define T_LISTEN	0x0001	/* connection indication received */
#define T_CONNECT	0x0002	/* connect confirmation received */
#define T_DATA		0x0004	/* normal data received */
#define T_EXDATA	0x0008	/* expedited data received */
#define T_DISCONNECT 	0x0010	/* disconnect received */
#define T_UDERR		0x0040	/* datagram error indication */
#define T_ORDREL	0x0080	/* orderly release indication */
#define T_GODATA        0x0100  /* sending norma data is again possible */
#define T_GOEXDATA      0x0200  /* sending expedited data is again possible */
#define T_EVENTS	0x03ff	/* event mask */


/*
 *	The following are the flag definitions needed by the
 *	user level library routines
 */

#define T_MORE		0x001	/* more data */
#define T_EXPEDITED	0x002	/* expedited data */
#define T_NEGOTIATE	0x004	/* set opts */
#define T_CHECK		0x008	/* check opts */
#define T_DEFAULT	0x010	/* get default opts */
#define T_SUCCESS	0x020	/* successful */
#define T_FAILURE	0x040	/* failure */

/*
 * XTI error return
 */

extern int t_errno;


/*
 *	Protocol specific service limits
 */

struct t_info {
	long addr;	/* size of protocol address */
	long options;	/* size of protocol options */
	long tsdu;	/* size of max transport service data unit */
	long etsdu;	/* size of max expedited tsdu */
	long connect;	/* max data for connection primitives */
	long discon;	/* max data for disconnect primitives */
	long servtype;	/* provider service type */
};

/*
 * specific service limit definitions
 */

#define T_NOLIMIT              -1
#define T_NOTSUPPORTED         -2


/*
 *	Service type defines
 */

#define T_COTS		01	/* connection oriented transport service */
#define T_COTS_ORD	02	/* connection oriented w/ orderly release */
#define T_CLTS		03	/* connectionless transport service */


/*
 *	netbuf structure
 */

struct netbuf {
	unsigned int maxlen;
	unsigned int len;
	char *buf;
};


/*
 *	t_bind - format of the address and options arguments of bind
 */

struct t_bind {
	struct netbuf	addr;
	unsigned 	qlen;
};


/*
 *	options management structure
 */

struct t_optmgmt {
	struct netbuf	opt;
	long		flags;
};


/*
 *	disconnect structure
 */

struct t_discon {
	struct netbuf	udata;		/* user data */
	int 		reason;		/* reason code */
	int 		sequence;	/* sequence number */
};


/*
 *	call structure
 */

struct t_call {
	struct netbuf	addr;		/* address */
	struct netbuf	opt;		/* options */
	struct netbuf	udata;		/* user data */
	int		sequence;	/* sequence number */
};


/*
 *	datagram structure
 */

struct t_unitdata {
	struct netbuf	addr;		/* address */
	struct netbuf	opt;		/* options */
	struct netbuf 	udata;		/* user data */
};


/*
 * 	unitdata error structure
 */

struct t_uderr {
	struct netbuf	addr;		/* address */
	struct netbuf 	opt;		/* options */
	long		error;		/* error code */
};


/*
 *	The following are structure types used when dynamically
 *	allocating the above structures via t_alloc().
 */

#define T_BIND_STR     	1	/* struct t_bind */
#define T_OPTMGMT_STR	2	/* struct t_optmgmt */
#define T_CALL_STR     	3	/* struct t_call */
#define T_DIS_STR      	4	/* struct t_discon */
#define T_UNITDATA_STR	5	/* struct t_unitdata */
#define T_UDERROR_STR	6	/* struct t_uderr */
#define T_INFO_STR     	7	/* struct t_info */


/*
 *	The following bits specify which fields of the above
 *	structures should be allocated by t_alloc().
 */

#define T_ADDR		0x01	/* address */
#define T_OPT		0x02	/* options */
#define T_UDATA		0x04	/* user data */
#define T_ALL		0x07	/* all the above */


/*
 *	The following are the states for the user
 */

#define T_UNBND		1	/* unbound */
#define T_IDLE		2	/* idle */
#define T_OUTCON	3	/* outgoing connection pending */
#define T_INCON		4	/* incoming connection pending */
#define T_DATAXFER	5	/* data transfer */
#define T_OUTREL	6	/* outgoing release pending */
#define T_INREL		7	/* incoming release pending */


/*
 *	Defines for setting protocol options
 */

#define T_YES		1
#define T_NO		0
#define T_UNUSED       -1
#define T_NULL          0
#define T_ABSREQ	0x8000

/* SPECIFIC ISO OPTION AND MANAGEMENT PARAMETERS */

/* definition of the ISO transport classes */

#define T_CLASS0	0
#define T_CLASS1	1
#define T_CLASS2	2
#define T_CLASS3	3
#define T_CLASS4	4


/* definition of the priorites */

#define T_PRITOP	0
#define T_PRIHIGH	1
#define T_PRIMID	2
#define T_PRILOW	3
#define T_PRIDFLT	4


/* definition of the protection levels */

#define T_NOPROTECT		1
#define T_PASSIVEPROTECT	2
#define T_ACTIVEPROTECT		4

/* default length of TPDU */

#define T_LTPDUDFLT     128


/*
 * rate structure
 */

struct rate {
  long targetvalue;
  long minacceptvalue;
};

/*
 * reqvalue structure
 */

struct reqvalue {
  struct rate called;
  struct rate calling;
};

/*
 * thrpt structure
 */

struct thrpt {
  struct reqvalue maxthrpt;
  struct reqvalue avgthrpt;
};

/*
 * management structure
 */

struct management {
  short dflt;              /* T_YES , default values are valid */
  int ltpdu;		   /* max. length of tpdu */
  short reastime;	   /* reassignment time */
  char class;		   /* principal class */
  char altclass;	   /* alternative class */
  char extform;		   /* extended format */
  char flowctrl;	   /* flow control */
  char checksum;	   /* checksum */
  char netexp;		   /* network expedited data */
  char netrecptcf;	   /* receipt confirmation */
};

/*
 * ISO connection oriented options
 */

struct isoco_options {
  struct thrpt throughput;     /* throughput */
  struct reqvalue transdel;    /* transit delay */
  struct rate reserrorrate;     /* residual error rate */
  struct rate transffailprob;  /* transfer failure probability */
  struct rate estfailprob;     /* connection establ. failure prob. */
  struct rate relfailprob;     /* connection release failure prob. */
  struct rate estdelay;        /* connection establishment delay */
  struct rate reldelay;        /* connection release delay */
  struct netbuf connresil;     /* connection resilience */
  unsigned short protection;   /* protection */
  short priority;	       /* priority */
  struct management mngmt;     /* management parameters */
  char expd;		       /* expedited data */
};


/*
 * ISO connectionless options
 */

struct isocl_options {
  struct rate transdel;	   	/* transit delay */
  struct rate reserrorrate;      /* residual error rate */
  unsigned short protection;    /* protection */
  short priority;		/* priority */
};


            /*  TCP SPECIFIC ENVIRONMENT */

/*
 * TCP precedence levels 
 */

#define T_ROUTINE       0
#define T_PRIORITY      1
#define T_IMMEDIATE     2
#define T_FLASH         3
#define T_OVERRIDEFLASH 4
#define T_CRITIC_ECP    5
#define T_INETCONTROL   6
#define T_NETCONTROL    7

struct secoptions {
  short security;    /* security field */
  short compartment; /* compartment */
  short handling;    /* handling restrictions */
  long tcc;          /* transmission control code */
};

struct tcp_options {
  short precedence;          /* precedence */
  long timeout;              /* abort timeout */
  long max_seg_size;	     /* maximum segment size */
  struct secoptions secopt;  /* TCP security options */
};























