/*
 * @(#)mode.h	4.1	ULTRIX	7/2/90
 */

#include "machine.h"
/*
 * sdb/adb - common definitions for old srb style code
 */

#define MAXCOM	64
#define MAXARG	32
#define LINSIZ	512
typedef	long	ADDR;
typedef	short	INT;
typedef	int		VOID;
typedef	long int	L_INT;
typedef	float		REAL;
typedef	double		L_REAL;
typedef	unsigned	POS;
typedef	char		BOOL;
typedef	char		CHAR;
typedef	char		*STRING;
typedef	char		MSG[];
typedef	struct map	MAP;
typedef	MAP		*MAPPTR;
typedef	struct bkpt	BKPT;
typedef	BKPT		*BKPTR;


/* file address maps */
struct map {
	L_INT	b1;
	L_INT	e1;
	L_INT	f1;
	L_INT	b2;
	L_INT	e2;
	L_INT	f2;
	INT	ufd;
};

struct bkpt {
	ADDR	loc;
	ADDR	ins;
	INT	count;
	INT	initcnt;
	INT	flag;
	CHAR	comm[MAXCOM];
	BKPT	*nxtbkpt;
};

typedef	struct reglist	REGLIST;
typedef	REGLIST		*REGPTR;
struct reglist {
	STRING	rname;
	int *	roffs;
};

struct state {
	int *kdb_fp, *kdb_ap, *kdb_sp, *kdb_pc;
};
