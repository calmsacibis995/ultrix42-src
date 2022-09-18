/* --------------------------------------------------------- */
/* | Copyright (c) 1986, 1989 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                                  | */
/* --------------------------------------------------------- */
/* $Header: u_outformat.h,v 2010.2.1.5 89/11/29 22:39:11 bettina Exp $ */
/* ref.		date		reference				*/
/* !01		07oct85		buffering output			*/
#include	<stdio.h>
#define MIN(a,b) ( ((a) > (b))  ? (b) : (a) )
#define		BUFFLEN 100		/* maximum length of format 	*/
					/*	before expansion	*/
					/*				*/
					/* U-code output formats	*/
#define	UCW_ASCII	1		/* ascii output?		*/
#define	UCW_BIN		2		/* binary output		*/
#define	UCW_OCTAL	4		/* octal output?		*/
#define	UCW_ECHO_OCTAL	8		/* octal output from btou?	*/
int	Ucw_out_type;			/* type of output requested	*/
					/*				*/
int	Ucw_verbose;			/* verbose output?		*/
FILE	*Ucw_binout;			/* binary output file (open)!01	*/
FILE	*Ucw_ascout;			/* ascii output file (fopen)	*/
					/*				*/
struct Uw_if {				/* output format for each type	*/
	int	If_nbytes;		/*  # of bytes to read (doesn't	*/
					/*	include opcode or val)	*/
	int	If_hasval;		/*  if it has a value		*/
	} ;				/*				*/
					/*				*/
					/* U-code output formats	*/
enum					/*				*/
  Uw_otype { Uwof_init,			/* the init statement		*/
	     Uwof_comm,			/* the comment statement	*/
	     Uwof_lca,			/* the lca statement		*/
	     Uwof_ldc,			/* the ldc statement		*/
	     Uwof_normal, 		/* normal format of printf 	*/
					/*	format and pointers	*/
	     Uwof_undef 		/* still working ones of this 	*/
					/*	type			*/
	     } ;			/*				*/
					/*				*/
struct Uw_of {				/* output format for each type	*/
	enum Uw_otype 	Of_type;	/*    the output format type to	*/
					/*		choose		*/
	int		Of_nbytes;	/* number of bytes		*/
	int		Of_hasval;	/* set if it has a constant	*/
	char		*Of_format;	/*    a "printf" style output	*/
					/*		format string	*/
	} ;				/*				*/
					/*				*/
					/*				*/
extern struct Uw_of Uw_of[];		/* the output format data	*/
					/*				*/
