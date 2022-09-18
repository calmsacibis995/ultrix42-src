#ifdef ultrix
#ifdef lint
static char *sccsid = "@(#)lup_io.c	4.1	(ULTRIX)	7/2/90";
#endif lint
#endif ultrix

/***	
 ***	lup_io.c --
 ***	
 ***	Handle input and output for the Page Layup Definition File Translator.
 ***	
 ***	N.Batchelder, 2/9/87.
 ***/

# include "lup_def.h"
# include "ctype.h"

/* 
 * These variables are the interface to the caller that was provided at
 * startup time.
 */

 private	int	(*getr)();
 private int	user_arg_g;
 private int	(*putr)();
 private int	user_arg_p;

/* 
 * This is the jump buffer created in main.c which we jump back to if
 * something goes wrong.
 */

extern  jmp_buf	abort_jmp;

/* 
 * These macros simplify the calling of getr and putr. If one fails, they jump
 * back to return an error.
 */

# define GetR(a,b,c)	{ \
				int	s = (*getr)(a,b,c); \
				if (!(s & STS$M_SUCCESS)) { \
					longjmp(abort_jmp, s); \
				} \
			}

# define PutR(a,b,c)	{ \
				int	s = (*putr)(a,b,c); \
				if (!(s & STS$M_SUCCESS)) { \
					longjmp(abort_jmp, s); \
				} \
			}

/* 
 * These are our variables to keep track of what the caller's io routines have
 * given us, etc.
 */

 private char	*ibuf;		/* Input buffer */
 private int	ilen;		/* Length of input buffer */
 private char	*iptr;		/* Next char to read from ibuf */
 private flag	ieof;		/* Have we hit end-of-file on input? */

 private char	*obuf;		/* Output buffer */
 private int	olen;		/* Length of stuff in output buffer */
 private int	omax;		/* Length of buffer itself */
 private char	*optr;		/* Next char to write in obuf */

/* 
 * This flag is needed to support both the LPS symbiont and the CPS symbiont.
 * Their treatment of RMS record conversion to \n\r differs in that the LPS
 * symbiont has an initial \n where the CPS symbiont does not. When the LPS
 * symbiont no longer exists, this variable and all references to it can go
 * away.
 */

 private	flag	firstchar;

/* 
 * This is our line counter for error messages.
 */

int	inputline;

/* 
 * start_io:
 * 
 * Initialize the input output interface to the caller.
 */

start_io(gr, uag, pr, uap)
int	(*gr)();
int	uag;
int	(*pr)();
int	uap;
{
	/* 
	 * Save the functions and values away in our permanent variables.
	 */

	getr = gr;
	user_arg_g = uag;
	putr = pr;
	user_arg_p = uap;

	/* 
	 * Get the first output buffer.
	 */

	omax = 0;
	PutR(&omax, &obuf, user_arg_p);
	optr = obuf;
	olen = 0;

	/* 
	 * Initialize the input mechanism.
	 */

	ilen = 0;
	ieof = false;
	inputline = 0;

	firstchar = true;	/* Junk this when no more LPS symbiont */
}

/* 
 * getline:
 * 
 * Read a line from the input, but squeeze out the whitespace (all of it),
 * lowercase everything, and remove comments. True is returned if there is
 * actually some new stuff in the buffer, false is returned when end of file
 * has been reached and there is no more data. The line will be terminated
 * by \0, real line terminators having been stripped.
 */

flag
getline(buf)
char	*buf;
{
	flag	done = false;
	flag	incomment = false;
	char	*uptr;			/* Pointer into user's buffer */
	char	c;
# ifdef DEBUGIO
	char	cebuf[200], *ceptr = cebuf;
# endif

	if (ieof) {
		return false;
	}

	uptr = buf;
# ifdef DEBUGIO
	strcpy(cebuf, "(LINE: ");
	ceptr = cebuf + strlen(cebuf);
# endif DEBUGIO

	for (;; firstchar = false) {
		if (ilen == 0) {
			/* 
			 * Fill up the input buffer again.
			 */

			GetR(&ilen, &ibuf, user_arg_g);
			if (ilen == 0) {
				ieof = true;
				*uptr = '\0';
				inputline++;
# ifdef DEBUGIO
				strcpy(ceptr, "\\n) print flush");
				putline(cebuf);
# endif
				return (uptr > buf);
			}
			iptr = ibuf;
		}

		/* 
		 * Pick the next character up from the input buffer.
		 */
		
		c = *iptr++;
		ilen--;
# ifdef DEBUGIO
		switch (c) {

		case '\n':
			*ceptr++ = '\\';
			*ceptr++ = '\\';
			*ceptr++ = 'n';
			break;

		case '\r':
			*ceptr++ = '\\';
			*ceptr++ = '\\';
			*ceptr++ = 'r';
			break;

		case '\\':
		case '(':
		case ')':
			*ceptr++ = '\\';
		default:
			*ceptr++ = c;
			break;
		}
# endif

		/* 
		 * If it is an end of line, terminate the buffer and return
		 * it. Note that if a line ends with "\n\r", the \n will
		 * terminate the first line, and then the \r at the beginning
		 * of the second will be ignored.
		 */
		
		if (c == '\n') {
# ifdef DEBUGIO
			strcpy(ceptr, "\\n) print flush");
			putline(cebuf);
			strcpy(cebuf, "(LINE: ");
			ceptr = cebuf + strlen(cebuf);
# endif 
			/* 
			 * The LPS symbiont passes us an extra linefeed as the
			 * first character in the stream. We want to not count
			 * this character so that both the CPS and LPS
			 * symbiont will work properly. When the LPS symbiont
			 * no longer exists, this first character flag stuff
			 * can go away.
			 */
			
			if (!firstchar) {
				inputline++;
			}
			if (uptr > buf) {
				*uptr = '\0';
				return true;
			}
			incomment = false;
			continue;
		}

		/* 
		 * If it is a comment character, then make a note of the fact
		 * that we are now in a comment.
		 */
		
		if (c == '!') {
			incomment = true;
			continue;
		}

		/* 
		 * If we aren't in a comment, and it isn't a whitespace
		 * character, then put it in the user's buffer.
		 */
		
		if (!incomment && c != ' ' && c != '\t' && c != '\r') {
			*uptr++ = tolower(c);
		}
	}
}

/* 
 * putline:
 * 
 * Write a line to the output. The line should be terminated by \0, real
 * line terminators will be affixed here.
 */

putline(buf)
char	*buf;
{
	int	ulen = strlen(buf);

	if (olen + ulen > omax) {
		/* 
		 * The output buffer is full, so write it out.
		 */

		PutR(&olen, &obuf, user_arg_p);
		optr = obuf;
		olen = 0;
	}

	strcpy(optr, buf);
	optr += ulen;
	*optr++ = '\n';
	olen += ulen + 1;
}

/* 
 * flush_io:
 * 
 * Make sure the last bit of output has been sent.
 */

flush_io()
{
	if (olen > 0) {
		PutR(&olen, &obuf, user_arg_p);
	}
}

/* end of lup_io.c */
