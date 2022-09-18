/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: nlist.c,v 2010.2.1.3 89/11/29 14:29:56 bettina Exp $ */

/*
 * Author	Mark I. Himelstein
 * Module	nlist
 * Program	nlist
 * Date:	Friday May 9 1986
 * Purpose	symbol table access of binaries
 */

#include	<stdio.h>
#include	"a.out.h"
#include	"ldfcn.h"

#ifdef SELFTEST

main (argc, argv)
char **argv;
{
    struct nlist *nl;

    if (argc < 3) {
	printf ("Usage: nlist textfile symbolname [symbolname ..]\n");
	exit (1);
    } /* if */

    nl = (struct nlist *) calloc(argc , sizeof(struct nlist));
    --argc;
    --argc;
    while (--argc >= 0)
	nl[argc].n_name = argv[argc + 2];
    
    nlist(argv[1], nl);
    for (; nl && nl->n_name != 0 && nl->n_name[0] != 0 ; nl++)
	printf ("%s 0x%x %d\n", nl->n_name, nl->n_value, nl->n_type);
    exit (0);
} /* main */
#endif SELFTEST


nlist (filename, nlbase)

char	*filename;
struct nlist *nlbase;

{
    struct nlist *nl;
    LDFILE	*ldptr;
    char	*name;
    pSYMR	psym;
    SYMR	asym;
    int		isymMax;		/* max # of regular symbols */
    int		iextMax;		/* max # of externals */
    int		iMax;			/* total # of wanted syms + externals */
    int		iBase;			/* base index of sym or extern wanted */
    int		iextBase;		/* base index of externs for f5 sorts */
    int		isym;			/* index through symbols in file */
    char	*ldgetname();
    int         unfound = 0;

    for (nl = nlbase; nl && nl->n_name && nl->n_name[0]; nl++) {
#ifdef BSD
	if (nl->n_name[0] == '_')
	    nl->n_name++;
#endif
	nl->n_type = 0;
	nl->n_value = 0;
        unfound++;
    } /* for */

    if ((ldptr = ldopen (filename, ldptr)) == NULL) {
	return -1;
    } /* if */

    if (ISARCHIVE(ldptr->type)) {
	return -1;
    } /* if */

#if 0
    for (nl = nlbase; nl && nl->n_name && nl->n_name[0]; nl++) {

#ifdef BSD
	if (nl->n_name[0] == '_')
	    nl->n_name++;
#endif
#endif

    if( PSYMTAB(ldptr) == 0)
        return -1;  /* no symbols, leave the types set 0 from above.
                Call this 'unsuccessful'  */


	isymMax = SYMHEADER(ldptr).isymMax;
	iextMax = SYMHEADER(ldptr).iextMax;

	/* determine range of symbols to read */
	iMax = isymMax + iextMax;
	iBase = isymMax;

	psym = &asym;

	for (isym = iBase; isym < iMax; isym++) {

	    /* read a symbol */
	    if (ldtbread (ldptr, isym, psym) != SUCCESS) {
		return -1;
	    } /* if */

	    if (psym->st == stBlock && psym->index != indexNil) {
		isym = psym->index - 1;
		continue;
	    } else if (isym < isymMax && psym->st != stStatic &&
		psym->st != stStaticProc) {
		/* statics allowed, throw away other locals */
		continue;
	    } /* if */

            for (nl = nlbase; nl && nl->n_name && nl->n_name[0]; nl++) {

                    /* Set the name and isym, take the first of equal names */
                    if(nl->n_type == 0  &&
                        strcmp(name=ldgetname (ldptr, psym), nl->n_name) == 0) {
#if 0
	    /* set the name and isym */
	    if (strcmp(name = ldgetname (ldptr, psym), nl->n_name) == 0) {
#endif
		nl->n_value = psym->value;
		nl->n_type = psym->st;
			unfound--;
                        if( unfound == 0) {
			    ldclose(ldptr);
			    return 0;
			}
	    } /* if */

	} /* for */
    } /* for */
    ldclose (ldptr);
#if 0
    return 0;
#else
    return unfound;
#endif

} /* nlist */
