/*
 *	setld.h -
 *		master header file for the setld family of utilties
 *
 *	SCCSID	"@(#)setld.h	4.2 (ULTRIX) 11/14/90"
 *
 *			Copyright (c) 1989 by
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.
 *								
 *	This software is furnished under a license and may be used and
 *	copied  only  in accordance with the terms of such license and
 *	with the  inclusion  of  the  above  copyright  notice.   This
 *	software  or  any  other copies thereof may not be provided or
 *	otherwise made available to any other person.  No title to and
 *	ownership of the software is hereby transferred.		
 *								
 *	The information in this software is subject to change  without
 *	notice  and should not be construed as a commitment by Digital
 *	Equipment Corporation.					
 *								
 *	Digital assumes no responsibility for the use  or  reliability
 *	of its software on equipment which is not supplied by Digital.
 *
 *	000	2-feb-1989	Chas. Bennett
 *	001	14-jun-1989	ccb
 *		add defines and typedefs and externs needed to implement
 *		C coded dependency checking
 *	002	24-jul-1989	ccb
 *		remove "include <sys/dir.h>"
 *		Lint, depord debugging/qualification
*/

#define	NAMELEN		128	/* max length of subset names */
#define	DATELEN		12	/* length of date strings */
#define	CODELEN		3	/* length of rev code strings */
#define	STRINGLEN	256	/* length of arbitrary strings */

typedef char	CodeT[CODELEN+1];	/* code info type */
extern char	*CodeSet();		/* set a code object to a value */

typedef char	DateT[DATELEN+1];	/* external date representation */
extern char	*DateFormat();		/* Format time_t to i_date */

typedef	unsigned	FlagsT;		/* flag representation */
extern FlagsT		FlagsScan();	/* xlate from string */

typedef char	NameT[NAMELEN+1];	/* Name Type */
extern char	*NameGetPcode();	/* get pcode from name */
extern char	*NameGetVcode();	/* get vcode from name */
extern char	*NameScan();		/* set a NameT from a string */
extern char	*NameSet();		/* obs. alias for NameScan() */

typedef char	PathT[MAXPATHLEN+1];
extern char	*PathSet();		/* assign value */
extern char	*PathStripExt();	/* product extension-less copy */

typedef char	StringT[STRINGLEN+1];
extern char	*StringCopy();		/* copy a string, strict */
extern char	*StringSet();		/* assign value */
extern char	*StringToken();		/* strtok(3) interface */
extern char	*StringUnquote();	/* remove quotes from a string */

typedef char	FtypeT;		/* File Type storage type */
extern FtypeT	FtypeFormat();	/* xlate mode to type char */


/*	TYPEDEF MI DataTypes
*/
typedef FILE	MiT;	/* Master Inventory Pointer */
typedef struct	MiRecT	/* Master Inventory record definition */
{
	struct MiRecT	*mi_next;	/* for making chains */
	FlagsT		mi_flags;	/* flag info */
	PathT		mi_path;	/* path name */
	NameT		mi_subset;	/* subset name */
} MiRecT;

/*	TYPEDEF INV DataTypes
*/

/* internal data access constants
*/
#define	I_NEXT		0x0001
#define	I_FLAGS		0x0002
#define	I_SIZE		0x0004
#define	I_SUM		0x0008
#define	I_UID		0x0010
#define	I_GID		0x0020
#define	I_PERM		0x0040
#define	I_DATE		0x0080
#define	I_REV		0x0100
#define	I_TYPE		0x0200
#define	I_PATH		0x0400
#define	I_LINKTO	0x0800
#define	I_SUB		0x1000
#define	I_RDEV		0x2000
#define	I_VALL		0x0FF4
#define	I_VDATA		(I_SIZE|I_SUM)

/* bit definitions for i_flags
*/
#define	INVPRECEDENCE	0x0001
#define	INVVOLATILE	0x0002
#define INVVOLITILE	INVVOLATILE	/* backwards.... */
#define	INVMASK		0x0003

typedef	FILE	InvT;	/* inventory pointer */
typedef struct InvRecT	/* inventory record definition */
{
	struct InvRecT	*i_next;	/* for making chains */

	/* setld specific atrributes
	*/
	PathT		 i_path;	/* name */
	PathT		 i_ref;		/* referent */
	DateT		 i_date;	/* last rev date */
	FtypeT		 i_type;	/* type in [lsdf] */
	FlagsT		 i_vflags;	/* verification flags */
	FlagsT		 i_flags;	/* attribute flags */
	CodeT		 i_rev;		/* rev level */
	NameT		 i_subset;	/* subset */
	unsigned	 i_sum;		/* checksum */

	/* stat(2) derived attributes
	*/
	short		 i_gid;		/* group id */
	dev_t		 i_dev;		/* device info */
	ino_t		 i_ino;		/* inode info */
	u_short		 i_mode;	/* file access mode */
	short		 i_nlink;	/* link count */
	dev_t		 i_rdev;	/* dev info for dev files */
	off_t 		 i_size;	/* size in bytes */
	short		 i_uid;		/* user id */
} InvRecT;

extern InvT	*InvInit();		/* Initialize InvT from FILE * */
extern InvT	*InvOpen();		/* Inventory Open Routine */
extern InvRecT	*InvRead();		/* Inventory Read Routine */
extern InvRecT	*InvRecAppend();	/* append inventory lists */
extern InvRecT	*InvRecCopy();		/* copy a record */
extern InvRecT	*InvRecInsertAlpha();	/* insert record, alpha on i_path */
extern InvRecT	*InvRecNew();		/* allocate storage */
extern char	*InvRecSetPath();	/* set path */
extern char	*InvRecSetRef();	/* set referent */
extern char	*InvRecSetRev();	/* set revision code */
extern char	*InvRecSetSubset();

/*	dependency data structure
 *		dependencies are stored as links in a list
 *
*/

#define	DEP_REQUIRE	0x0001	/* subset required (default) */
#define	DEP_ADVERSE	0x0002	/* subset absent asserted */
#define	DEP_PRODTYP	0x0004	/* product level dependency */
#define	DEP_SUBSTYP	0x0008	/* subset level dependency */

#define	DEPS_OPEN	0	/* dependency ordering closure not required */
#define	DEPS_CLOSED	1	/* dependency ordering closure required */

typedef struct DepT {
	struct DepT	*d_next;
	NameT		d_value;
	FlagsT		d_flags;
} DepT;

extern DepT	*DepAppend();		/* append routine */
extern DepT	*DepCreate();		/* create named */
extern void	DepFree();		/* free a single DepT */
extern DepT	*DepListCopy();		/* copy a list of DepTs */
extern char	*DepListFormat();	/* convert to string */
extern void	DepListFree();		/* Free DepT storage */
extern DepT	*DepListMerge();	/* merge dependency lists */
extern DepT	*DepMember();		/* list membership predicate */
extern DepT	*DepNew();		/* allocate storage */
extern DepT	*DepListPromote();	/* promote from subset to prod */
extern DepT	*DepScan();		/* convert from StringT */


/*	x.ctrl definitions and typedefs
*/


/* ctrlerr error codes
*/
#define	CTRL_OK		0
#define	CTRL_EOF	1	/* no more records */
#define	CTRL_FOPEN	2	/* cannot open ctrl file */
#define	CTRL_CPTN	3	/* control file corrupt */
#define	CTRL_NCODE	4	/* total # of error codes */

typedef struct CtrlT
{
	struct CtrlT	*c_next;	/* link pointer */
	PathT		c_dpath;	/* db pathname */
	DIR		*c_dir;		/* directory pointer */
	PathT		c_fpath;	/* ctrl file pathanme */
	FILE		*c_fp;		/* ctrl file pointer */
	FlagsT		c_flags;	/* ctrl stream flags */
} CtrlT;

#define	SUB_NORM	0x0001	/* no rm subset flag */
#define	SUB_OPT		0x0002	/* optional subset flag */

extern CtrlT	*CtrlCreate();		/* create routine */
extern char	*CtrlGetFpath();	/* get fpath value */
extern CtrlT	*CtrlNew();		/* allocte CtrlT storage */
extern CtrlT	*CtrlOpen();		/* open routine */
extern char	*CtrlSetFpath();	/* set fpath field */

/* ctrl file variable status bits
*/
#define	CVAR_NAME	0x0001		/* name OK */
#define	CVAR_DESC	0x0002		/* desc OK */
#define	CVAR_NVOLS	0x0004		/* nvols OK */
#define	CVAR_MTLOC	0x0008		/* mtloc OK */
#define	CVAR_DEPS	0x0010		/* deps OK */
#define	CVAR_FLAGS	0x0020		/* flags OK */
#define	CVAR_ALL	0x003f		/* all bits */
#define	CVAR_BITS	6		/* number of flags to check */

typedef struct CtrlRecT
{
	struct CtrlRecT	*ct_next;	/* link pointer */
	NameT		ct_subset;	/* subset name */
	StringT		ct_name;	/* name string */
	StringT		ct_desc;	/* description */
	unsigned	ct_dcnt;	/* diskette count */
	int		ct_dvol;	/* disk volume (unused) */
	unsigned	ct_tvol;	/* tape volume */
	int		ct_tloc;	/* tape location */
	DepT		*ct_deps;	/* dependencies */
	FlagsT		ct_flags;	/* subset flags */
} CtrlRecT;

extern char	*CtrlErrorString();	/* translate error code */
extern CtrlRecT	*CtrlRead();		/* read routine */
extern CtrlRecT	*CtrlRecAppend();	/* list management */
extern CtrlRecT	*CtrlRecCopy();		/* copy routine */
extern char	*CtrlRecGetSubset();	/* get subset info */
extern CtrlRecT	*CtrlRecNew();		/* new instance */
extern CtrlRecT	*CtrlRecOrderInsert();	/* insert a CtrlRecT in a list */
extern void	CtrlRecPrintList();	/* debug print */
extern DepT	*CtrlRecSetDeps();	/* set deps field */
extern char	*CtrlRecSetDesc();	/* set desc field */
extern char	*CtrlRecSetDiskInfo();	/* set dcnt, dvol fields */
extern FlagsT	CtrlRecSetFlags();	/* set flags field */
extern char	*CtrlRecSetName();	/* set name field */
extern char	*CtrlRecSetPath();	/* set path field */
extern char	*CtrlRecSetSubset();	/* set subset field */
extern char	*CtrlRecSetTapeInfo();	/* set tape fields */


/*	Product Data Structures
 *
*/

#define	PROD_COMP	0x0001	/* product subsets compressed */
#define	PROD_DEPORD	0x0002	/* dependency ordered */

typedef FILE	ProdT;	/* probably unnecessary */
typedef struct ProdRecT
{
	struct ProdRecT	*p_next;	/* link */
	CtrlRecT	*p_cp;		/* control chain for subsets */
	CodeT		p_pcode;	/* product code */
	CodeT		p_vcode;	/* version code */
	FlagsT		p_flags;	/* flags */
	DepT		*p_deps;		/* dependency codes */
} ProdRecT;

extern ProdRecT	*ProdRecAddCtrl();	/* merge control record in prod list */
extern ProdRecT	*ProdRecAppend();	/* append a prodrect to a list */
extern ProdRecT	*ProdRecFindBySubset();	/* find prod ctrl belongs in */
extern char	*ProdRecGetName();	/* get product name */
extern ProdRecT	*ProdRecInsDepOrder();	/* insert product by dependencies */
extern ProdRecT	*ProdRecNew();		/* Create new product record */
extern ProdRecT *ProdRecOrderByDeps();	/* Order list by dependency */
extern void	ProdRecPrintList();	/* Print list */


/*	Assign Structure -
 *		used for parsing assignment statements as they
 *	appear in some of the data files
*/

typedef struct AssignT
{
	struct AssignT	*a_next;	/* link */
	StringT		a_name;		/* variable name */
	StringT		a_val;		/* variable value */
} AssignT;

extern char	*AssignGetName();
extern char	*AssignGetVal();	/* get value field */
extern AssignT	*AssignScan();		/* cvt string to AssignT */

/*%	FUNCTION TYPE DECLARATIONS
*/

extern u_short	CheckSum();	/* checksum */
extern InvRecT	*FVerify();	/* verify a file */
extern MiT	*MiInit();	/* Initialize MiT from FILE * */
extern MiRecT	*MiRead();	/* MI Read Routine */
extern char	*PermString();	/* xlate 0755 -> rwxr-xr-x, etc */
extern InvRecT	*StatToInv();	/* Make InvRecT from stat(2) data */
extern char	*emalloc();	/* error checked malloc */


/*	Miscellaneous macros
*/
#define	PERM(X)	((X)&~S_IFMT)	/* get perms from mode */

/*	Miscellaneous libc function declarations
*/
extern void	bcopy();	/* bstring(3) */
extern void	exit();		/* exit(3) */
extern char	*fgets();	/* gets(3) */
extern void	free();		/* malloc(3) */
extern char	*malloc();	/* malloc(3) */

