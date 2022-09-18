#ifdef lint
static char *sccsid = "@(#)nlist.c	4.1	(ULTRIX)	7/3/90";
#endif lint

#ifdef vax
/* @(#)nlist.c	4.3 (Berkeley) 6/10/83 */
#include <sys/types.h>
#include <a.out.h>
char *malloc();

/*
 * nlist - retreive attributes from name list (string table version)
 */
nlist(name, list)
	char *name;
	struct nlist *list;
{
	register struct nlist *p, *q;
	register n, m, i, nreq;
	int f;
	off_t sa;		/* symbol address */
	off_t ss;		/* start of strings */
	struct exec buf;
	struct nlist *space;
	char *names;
	int maxlen;
	int ret_code = -1;

	for (q = list, nreq = 0; q->n_un.n_name && q->n_un.n_name[0];
								q++, nreq++) {
		q->n_type = 0;
		q->n_value = 0;
		q->n_desc = 0;
		q->n_other = 0;
	}
	f = open(name,0);
	if (f < 0) goto justreturn;
	if (read(f,(char *)&buf, sizeof buf) != sizeof buf) goto justclose;
	if (N_BADMAG(buf) || buf.a_syms <= 0) goto justclose;
	sa = N_SYMOFF(buf);
	ss = sa + buf.a_syms;
	n = buf.a_syms;
	space = (struct nlist *) malloc(n);
	if (space <= 0) goto justclose;
	if (sa != lseek(f, sa, 0)) goto outspace; /* seek to start of symbols */
	if (read(f,(char *)space,n) != n) goto outspace; /* read symbol table */
	if (read(f,(char *)&maxlen,sizeof(int)) != sizeof(int)) goto outspace;
	n = maxlen;
	if (n <= 0) goto outspace;		/* no strings ?? */
	names = malloc(n);			/* 4 bytes + strings */
	if (names <= 0) goto outspace;		/* no mem */
	n -= sizeof(int);
	if (read(f,names+sizeof(int),n) != n) goto outnames; /* read strings */
	n = (int) space + (int) buf.a_syms;
	ret_code = 0;
	for (q = space;(int) q < n; q++) {
		char *nambuf;
		if (q->n_un.n_strx == 0 || q->n_type & N_STAB) continue;
		nambuf = &names[q->n_un.n_strx];
		for (p = list; p->n_un.n_name && p->n_un.n_name[0]; p++) {
			i = 0;
			while (p->n_un.n_name[i]) {
				if (p->n_un.n_name[i] != nambuf[i]) goto cont;
				i++;
			}
			if (nambuf[i]) continue;    /* not at end of string */
			p->n_value = q->n_value;
			p->n_type = q->n_type;
			p->n_desc = q->n_desc;
			p->n_other = q->n_other;
			if (--nreq == 0) goto alldone;
			break;
cont:			continue;
		}
	}
alldone:
outnames:
	free(names);
outspace:
	free(space);
justclose:
	close(f);
justreturn:
	return (ret_code);
}
#endif vax
#ifdef mips
#endif mips
