/* @(#)mode.h	4.1      7/17/90 */

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
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
 *
 *   Modification History:
 *
 * 002 - Gary A. Gaudet, Wed Nov  9 10:24:49 EST 1988
 *	 	MIPS portability and bug fixes
 *
 * 001 - Gary Gaudet for Andy Gadsby 09-mar-88
 *	i18n version of csh
 *
 *
 *
 */

/*
 *	UNIX shell
 */

#ifdef pdp11
typedef CHTYPE BOOL;
#else
typedef short BOOL;
#endif

#define BYTESPERWORD	(sizeof (CHTYPE *))
#define	NIL	((CHTYPE *)0)


/* the following nonsense is required
 * because casts turn an Lvalue
 * into an Rvalue so two cheats
 * are necessary, one for each context.
 */
#define Lcheat(a)	(*(int *)&(a))	/* DAG -- found a better way */
#define Rcheat(a)	((int)(a))


/* address puns for storage allocation */
typedef union
{
	struct forknod	*_forkptr;
	struct comnod	*_comptr;
	struct fndnod	*_fndptr;
	struct parnod	*_parptr;
	struct ifnod	*_ifptr;
	struct whnod	*_whptr;
	struct fornod	*_forptr;
	struct lstnod	*_lstptr;
	struct blk	*_blkptr;
	struct namnod	*_namptr;
	CHTYPE	*_bytptr;
} address;


/* heap storage */
struct blk
{
	struct blk	*word;
};

#define	BUFSIZ	128
struct fileblk
{
	int	fdes;
	unsigned flin;
	BOOL	feof;
	unsigned char	fsiz;
	CHTYPE	*fnxt;
	CHTYPE	*fend;
	CHTYPE	**feval;
	struct fileblk	*fstak;
	CHTYPE	fbuf[BUFSIZ];
};

struct tempblk
{
	int fdes;
	struct tempblk *fstak;
};


/* for files not used with file descriptors */
struct filehdr
{
	int	fdes;
	unsigned	flin;
	BOOL	feof;
	unsigned char	fsiz;
	CHTYPE	*fnxt;
	CHTYPE	*fend;
	CHTYPE	**feval;
	struct fileblk	*fstak;
	CHTYPE	_fbuf[1];
};

struct sysnod
{
	CHTYPE	*sysnam;
	int	sysval;
};

/* this node is a proforma for those that follow */
struct trenod
{
	int	tretyp;
	struct ionod	*treio;
};

/* dummy for access only */
struct argnod
{
	struct argnod	*argnxt;
	CHTYPE	argval[1];
};

struct dolnod
{
	struct dolnod	*dolnxt;
	int	doluse;
	CHTYPE	*dolarg[1];
};

struct forknod
{
	int	forktyp;
	struct ionod	*forkio;
	struct trenod	*forktre;
};

struct comnod
{
	int	comtyp;
	struct ionod	*comio;
	struct argnod	*comarg;
	struct argnod	*comset;
};

struct fndnod
{
	int 	fndtyp;
	CHTYPE	*fndnam;
	struct trenod	*fndval;
};

struct ifnod
{
	int	iftyp;
	struct trenod	*iftre;
	struct trenod	*thtre;
	struct trenod	*eltre;
};

struct whnod
{
	int	whtyp;
	struct trenod	*whtre;
	struct trenod	*dotre;
};

struct fornod
{
	int	fortyp;
	struct trenod	*fortre;
	CHTYPE	*fornam;
	struct comnod	*forlst;
};

struct swnod
{
	int	swtyp;
	CHTYPE *swarg;
	struct regnod	*swlst;
};

struct regnod
{
	struct argnod	*regptr;
	struct trenod	*regcom;
	struct regnod	*regnxt;
};

struct parnod
{
	int	partyp;
	struct trenod	*partre;
};

struct lstnod
{
	int	lsttyp;
	struct trenod	*lstlef;
	struct trenod	*lstrit;
};

struct ionod
{
	int	iofile;
	CHTYPE	*ioname;
	CHTYPE	*iolink;
	struct ionod	*ionxt;
	struct ionod	*iolst;
};

struct fdsave
{
	int org_fd;
	int dup_fd;
};


#define		fndptr(x)	((struct fndnod *)x)
#define		comptr(x)	((struct comnod *)x)
#define		forkptr(x)	((struct forknod *)x)
#define		parptr(x)	((struct parnod *)x)
#define		lstptr(x)	((struct lstnod *)x)
#define		forptr(x)	((struct fornod *)x)
#define		whptr(x)	((struct whnod *)x)
#define		ifptr(x)	((struct ifnod *)x)
#define		swptr(x)	((struct swnod *)x)
