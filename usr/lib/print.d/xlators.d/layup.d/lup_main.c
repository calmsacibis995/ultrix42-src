#ifdef ultrix
#ifdef lint
static char *sccsid = "@(#)lup_main.c	4.1	(ULTRIX)	7/2/90";
#endif lint
#endif ultrix

/***	
 ***	lup_main.c --
 ***	
 ***	The main function for the Page Layup Definition File Translator.
 ***	
 ***	N.Batchelder, 2/9/87.
 ***/

# include "lup_def.h"
# include "lup_errs.h"

/* 
 * This is the jump buffer we jump into if one of the caller-supplied
 * functions craps out.
 */

 jmp_buf	abort_jmp;

/* 
 * trn$layupdef_ps:
 * 
 * This is the entry point into the translator. This interface is specified in
 * the tranlator standard by David Larrick.
 *
 * N.B.:  While the layup file processing is now internalized in the symbiont,
 * the translator interface is still used because this code is shared with
 * Ultrix, KLPS40, and LPS groups!
 */

trn$layupdef_ps(getr, user_arg_g, putr, user_arg_p, itemlist, device_type)
int	(*getr)();
int	user_arg_g;
int	(*putr)();
int	user_arg_p;
any	itemlist;	/* Don't use any items, so we don't care what type */
any	device_type;	/* Don't use this, so we don't care what type */
{
	char	iline[100];
	char	oline[100];
	int	status;
	errcode	xstat;
	errcode	transline();

	/* 
	 * Set up the error jump for aborting.
	 */

	if (status = setjmp(abort_jmp)) {
		return status;
	}

	/* 
	 * Initialize the input and output.
	 */
	
	start_io(getr, user_arg_g, putr, user_arg_p);

	/* 
	 * Put out the teeny prolog we need.
	 */

	prolog();

	/* 
	 * Now we can start translating.
	 */

	while (getline(iline)) {
		xstat = transline(iline, oline);
		if (xstat == Success) {
			putline(oline);
		}
	}

	/* 
	 * Write out the teeny trailer.
	 */

	trailer();

	flush_io();

	return SS$_NORMAL;
}

/* 
 * prolog:
 * 
 * Because all of the parameters have to be set in lps$page-layup-dict, we
 * must explicitly begin it.
 */

prolog()
{
	putline("% output from the Layup Definition File Translator, v2.0");
	putline("lps$page-layup-dict begin");
}

/* 
 * trailer:
 * 
 * Since we began the special dictionary, we must end it.
 */

trailer()
{
	putline("end");
}

/* end of main.c */
