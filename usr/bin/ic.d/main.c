/************************************************************************
 *									*
 *			Copyright (c) 1987,1988 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *		            Bull, France				*
 *			   Siemens AG, FR Germany			*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under license and may be used and	*
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

#ifndef lint
static char Sccsid[] = "@(#)main.c	4.1	(ULTRIX)	7/17/90";
#endif

#include "ic.h"
#include <signal.h>

/*
 * definitions of global variables
 */
i_dbhead i_hdr;				/* header of the data base */
char *outname = (char *)0;		/* name of output file */

sym *cod_anc	= (sym *)0;		/* name of code table */
sym *prp_anc	= (sym *)0;		/* name of the property table */
sym *col_anc	= (sym *)0;		/* name of current collation table */
sym *frm_anc	= (sym *)0;		/* name of current format table */
sym *cnv_anc	= (sym *)0;		/* name of current conversion */

int  verbose	= 0;			/* verbosity level */

/*
 * main() -- run cpp before yyparse, evaluate parameters.
 */

main(argc, argv)
int argc;
char **argv;
{
	char **argp;
	void usage();
	extern int yynerrs;		/* yacc error count */

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGNOID, "main(%d, %x)", argc, argv);
#endif

	for (argp = argv; ++argp && *argp && **argp == '-'; /*EMPTY*/)
	{
		switch ((*argp)[1])
		{
		case 'C': case 'D': case 'E':
		case 'I': case 'P': case 'U':
			break;

		case 'v':		/* verbose option */
			verbose++;
			break;

		case 'o':		/* renamed output file */
			if ((*argp)[2] != '\0')
				outname = &((*argp)[2]);
			else if (*(argp + 1) != (char *)0)
				outname = *++argp;
			else
				usage(argv[0]);
			break;

		default:
			usage(argv[0]);
			break;

		}
	}

	if (argp[0] && argp[1])
		usage(argv[0]);

	if (argp && *argp && !freopen(*argp, "r", stdin))
		perror(*argp), exit(1);

	if (cpp(argv))
		perror("C preprocessor"), exit(1);

#ifdef EBUG
	if (yac_dbg)
#ifdef YYDEBUG
	{
		extern int yydebug;

		yydebug = 1;
	}
#else
	dbg_prt(DBGTIN, "-DYYDEBUG not defined when compiled");
#endif
	/*
	 * if we are debugging we want ALL information
	 */
	if (yac_dbg || lex_dbg || fct_dbg || cod_dbg || frm_dbg || cnv_dbg || val_dbg)
	{
		extern FILE *dbgfp;
		setbuf(dbgfp, (char *)0);
	}
#endif
	
	exit(yyparse() != 0 || yynerrs != 0);
}

static void
usage(name)
register char *name;
{
	fputs("usage: ", stderr);
	fputs(name, stderr);
	fputs(" [C preprocessor options] [-v] [source]\n", stderr);
	exit(1);
}

/*
 * ic_init -- initialize globals
 */
void
ic_init()
{
	int onintr();
#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "ic_init()");
#endif

	/*
	 * initilialize header for data base:
	 */
	i_hdr.i_magic  = I_MAGIC; i_hdr.i_nbspl = 0; i_hdr.i_nblet = 0;

	i_hdr.i_flags = 0;
	i_hdr.i_dbtab = (long)sizeof(i_dbhead);
	i_hdr.i_dblsz = 0L; i_hdr.i_nbdbl = 0;

	i_hdr.i_prtab = 0L; i_hdr.i_prhsz = 0L;
	i_hdr.i_prtsz = 0L; i_hdr.i_nbrpr = 0;

	i_hdr.i_cltab = 0L; i_hdr.i_clhsz = 0L;
	i_hdr.i_cltsz = 0L; i_hdr.i_nbrcl = 0;

	i_hdr.i_cvtab = 0L; i_hdr.i_cvhsz = 0L;
	i_hdr.i_nbrcv = 0;

	i_hdr.i_sttab = 0L; i_hdr.i_sthsz = 0L;
	i_hdr.i_nbrst = 0;

	/*
	 * trap signals:
	 */
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, onintr);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, onintr);
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, onintr);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, onintr);
}

/*
 * i_end -- finishup clearing away no longer used stuff
 */
void
i_end()
{
	extern int yynerrs;	/* yacc error count */
	register sym *sp;
	FILE *dbfp;		/* file pointer for data base */
	bit16 i;
	cl_head colhdr;		/* template collation header	*/
	st_head frmhdr;		/* template format header	*/
	cv_head cnvhdr;		/* template conversion header	*/
	pr_head prphdr;		/* template property header	*/
	long sum;
	long frm_siz;		/* size of format table section	*/
	long cnv_siz;		/* size of conversion table section */
	cod *codp;

#ifdef EBUG
	if (fct_dbg)
		dbg_prt(DBGTIN, "i_end()");
#endif

	/*
	 * check on completeness of information given
	 *	- 1 code table
	 *	- >1 property table	(default table)
	 *	- >1 collation table	(default table)
	 *	- >1 format table	(default table)
	 *	- conversion tables toupper, tolower as code conv's
	 */
	if (cod_anc == (sym *)0)
		bug("i_end1");
	if (prp_anc == (sym *)0)
		bug("i_end2");
	if (sym_find(COL_DEF) == (sym *)0)
		error("default collation table must be given");
	if (sym_find(FRM_DEF) == (sym *)0)
		error("default format table must be given");

	if ((sp = sym_find("toupper")) == (sym *)0)
		error("conversion table toupper is missing");
	else
	{
		if (sp->sym_cnv->cv_type != CNV_COD)
			error("toupper must be a CODE conversion");
	}

	if ((sp = sym_find("tolower")) == (sym *)0)
		error("conversion table tolower is missing");
	else
	{
		if (sp->sym_cnv->cv_type != CNV_COD)
			error("tolower must be a CODE conversion");
	}

	/*
	 * complete data base header:
	 *
	 * size of double letter table is number of double letters times size
	 * of one code
	 */
	i_hdr.i_dblsz = i_hdr.i_nbdbl * sizeof(i_dblt);

	/*
	 * offset to the property tables is double letter offset plus double
	 * letter table size
	 */
	i_hdr.i_prtab = i_hdr.i_dbtab + (long)i_hdr.i_dblsz;

	/*
	 * size of property tables header is number of property tables times the
	 * size of one header
	 */
	i_hdr.i_prhsz = i_hdr.i_nbrpr * sizeof(pr_head);

	/*
	 * size of one property table is size of code set times the size for
	 * one property entry
	 */
	i_hdr.i_prtsz = i_hdr.i_nblet * sizeof(i_char);

	/*
	 * offset to the collation tables is property tables offset plus
	 * property tables size times the number of tables plus property tables
	 * index array size.
	 */
	i_hdr.i_cltab = i_hdr.i_prtab + i_hdr.i_nbrpr * i_hdr.i_prtsz
			+ i_hdr.i_prhsz;

	/*
	 * size of the collation table index is the number of collation tables
	 * times the index size
	 */
	i_hdr.i_clhsz = i_hdr.i_nbrcl * sizeof(cl_head);

	/*
	 * size of one collation table is number of codes times size of the
	 * collation information.
	 */
	i_hdr.i_cltsz = i_hdr.i_nblet * sizeof(coll);

	/*
	 * offset to the format tables is sum of offset to the collation
	 * tables plus size of the collation tables
	 */
	i_hdr.i_sttab = i_hdr.i_cltab + i_hdr.i_clhsz
			+ i_hdr.i_nbrcl * i_hdr.i_cltsz;

	/*
	 * size of the format tables header is size
	 * of one index entry times the number of format tables
	 */
	i_hdr.i_sthsz = i_hdr.i_nbrst * sizeof(st_head);

	/*
	 * size of the whole format table section is header size plus size of
	 * all the format tables
	 */
	frm_siz = i_hdr.i_sthsz;
	for (sp = frm_anc; sp; sp = sp->sym_nxt)
		frm_siz += sp->sym_frm->st_offst;

	/*
	 * offset to the conversion tables is sum of offset to the format
	 * tables plus size of the format tables
	 */
	i_hdr.i_cvtab = i_hdr.i_sttab + frm_siz;

	/*
	 * size of the conversion index table is number of conversions times 
	 * size of one index entry
	 */
	i_hdr.i_cvhsz = i_hdr.i_nbrcv * sizeof(cv_head);

	/*
	 * size of the conversion tables is 
	 *	size of index section
	 *    + number of code conversions * number of codes * size of i_char
	 *    + number of string conversions * number of codes * size of one
	 *	conversion entry
	 */
	cnv_siz = i_hdr.i_cvhsz;
	for (sp = cnv_anc; sp; sp = sp->sym_nxt)
		cnv_siz += (long)sp->sym_cnv->cv_size;

	/*
	 * build size of total file
	 */
	sum = sizeof(i_dbhead)
	      + i_hdr.i_dblsz
	      + i_hdr.i_prhsz + i_hdr.i_prtsz * i_hdr.i_nbrpr
	      + i_hdr.i_clhsz + i_hdr.i_cltsz * i_hdr.i_nbrcl
	      + frm_siz
	      + cnv_siz;

	/*
	 * write the International unix data base
	 */
	if (yynerrs == 0)
	{
		if (verbose)
		{
			fprintf(stderr, "INTLINFO data base %s:\n",
				cod_anc->sym_nam);
			fprintf(stderr, "\t%4d code table entries (%d simple/%d multi-byte).\n",
				i_hdr.i_nblet, i_hdr.i_nbspl, i_hdr.i_nbdbl);
			fprintf(stderr, "\t%4d property table(s).\n",
				i_hdr.i_nbrpr);
			fprintf(stderr, "\t%4d collation table(s).\n",
				i_hdr.i_nbrcl);
			fprintf(stderr, "\t%4d string table(s).\n",
				i_hdr.i_nbrst);
			fprintf(stderr, "\t%4d conversion tables: ",
				i_hdr.i_nbrcv);
			for (sp = cnv_anc; sp; sp = sp->sym_nxt)
				fprintf(stderr, (sp->sym_nxt) ? "%s, "
							      : "%s.\n",
					sp->sym_nam);
			fprintf(stderr, "%ld bytes total length.\n", sum);
		}

		if ((dbfp = fopen(cod_anc->sym_nam, "w")) == (FILE *)0)
			fatal("cannot create data base %s", cod_anc->sym_nam);

		/*
		 * write the data base header
		 */
		if (fwrite(&i_hdr, sizeof(i_dbhead), 1, dbfp) != 1)
			fatal("cannot write data base header");

		/*
		 * write the double letter table
		 */
		for (i = i_hdr.i_nbspl; i < i_hdr.i_nblet; i++)
		{
			codp = codeset[i]->sym_val->val_cod;
			if (fwrite(&codp->cod_rep, sizeof(i_char), 1, dbfp) != 1 || fwrite(&i, sizeof(bit16), 1, dbfp) != 1)
				fatal("cannot write double letter table");
		}

		/*
		 * write the property table index
		 */
		prphdr.pr_offst = (long)i_hdr.i_nbrpr * (long)sizeof(pr_head);
		for (sp = prp_anc; sp; sp = sp->sym_nxt)
		{
			zero(prphdr.pr_name, I_NAML);
			strcpy(prphdr.pr_name, sp->sym_nam);
			if (fwrite(&prphdr, sizeof(pr_head), 1, dbfp) != 1)
				fatal("cannot write collation table index");
			prphdr.pr_offst += (long)i_hdr.i_prtsz;
		}

		/*
		 * write the property tables
		 */
		for (sp = prp_anc; sp; sp = sp->sym_nxt)
			append(sp->sym_prp->pr_name, dbfp);

		/*
		 * write the collation table index
		 */
		colhdr.cl_offst = (long)i_hdr.i_nbrcl * (long)sizeof(cl_head);
		for (sp = col_anc; sp; sp = sp->sym_nxt)
		{
			zero(colhdr.cl_name, I_NAML);
			zero(colhdr.cl_pnam, I_NAML);
			strcpy(colhdr.cl_name, sp->sym_nam);
			strcpy(colhdr.cl_pnam, sp->sym_col->cl_pnam);
			if (fwrite(&colhdr, sizeof(cl_head), 1, dbfp) != 1)
				fatal("cannot write collation table index");
			colhdr.cl_offst += (long)i_hdr.i_cltsz;
		}

		/*
		 * write the collation tables
		 */
		for (sp = col_anc; sp; sp = sp->sym_nxt)
			append(sp->sym_col->cl_name, dbfp);

		/*
		 * write the format table index
		 */
		frmhdr.st_offst = (long)i_hdr.i_nbrst * (long)sizeof(st_head);
		for (sp = frm_anc; sp; sp = sp->sym_nxt)
		{
			zero(frmhdr.st_name, I_NAML);
			strcpy(frmhdr.st_name, sp->sym_nam);
			frmhdr.st_siz = sp->sym_frm->st_offst;
			if (fwrite(&frmhdr, sizeof(st_head), 1, dbfp) != 1)
				fatal("cannot write format table index");
			frmhdr.st_offst += sp->sym_frm->st_offst;
		}

		/*
		 * write the format tables
		 */
		for (sp = frm_anc; sp; sp = sp->sym_nxt)
			append(sp->sym_frm->st_name, dbfp);

		/*
		 * write the conversion table index
		 */
		cnvhdr.cv_offst = (long)i_hdr.i_nbrcv * (long)sizeof(cv_head);
		for (sp = cnv_anc; sp; sp = sp->sym_nxt)
		{
			zero(cnvhdr.cv_name, I_NAML);
			strcpy(cnvhdr.cv_name, sp->sym_nam);
			cnvhdr.cv_type = sp->sym_cnv->cv_type;
			cnvhdr.cv_size = sp->sym_cnv->cv_size;
			if (fwrite(&cnvhdr, sizeof(cv_head), 1, dbfp) != 1)
				fatal("cannot write conversion index");
			cnvhdr.cv_offst += (long)cnvhdr.cv_size;
		}

		/*
		 * write the conversion tables
		 */
		for (sp = cnv_anc; sp; sp = sp->sym_nxt)
			append(sp->sym_cnv->cv_name, dbfp);

		/*
		 * close the data base file
		 */
		fclose(dbfp);

		/*
		 * final check for consistency
		 */
		if (fillen(cod_anc->sym_nam) != sum)
			bug("i_end2");
	}

	/*
	 * remove temporary files
	 */
	tmp_del();

	sym_all_del();

	cod_anc = (sym *)0;
	col_anc = (sym *)0;
	frm_anc = (sym *)0;
	cnv_anc = (sym *)0;

#ifdef EBUG
	/*
	 * close last line.
	 */
	dbg_prt(DBGNOID, "\n");
#endif
}

onintr(signo)
int signo;
{
	fatal("ic terminated by signal %d", signo);
}
