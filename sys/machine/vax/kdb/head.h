/*
 * @(#)head.h	4.1	ULTRIX	7/2/90
 */

long	kdb_maxoff;
ADDR	kdb_localval;

struct	nlist *kdb_symtab, *kdb_esymtab;
struct	nlist *kdb_cursym;
struct	nlist *lookup();

struct	exec kdb_filhdr;

long	kdb_var[36];

MAP	kdb_txtmap;
MAP	kdb_datmap;
INT	kdb_fcor;
INT	kdb_fsym;
INT	kdb_signo;

struct user *uptr;

char	*kdb_corfil, *kdb_symfil;

