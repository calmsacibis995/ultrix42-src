/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
static char	sccsid[] = "@(#)iconv.c	4.1	(ULTRIX)	7/17/90";
#endif /* lint */

/*
 * iconv -- international conversion utility
 *
 * iconv [-d] -f fromcode -t tocode [file..]
 *
 * Convert files, or stdin, from codeset from code to codeset tocode
 *
 */

#include <stdio.h>
#include <locale.h>

extern int optind;
extern char *optarg ;
int delete = 0;		/* delete any invalid characters */

main(argc, argv)
int argc;
char *argv[];
{	int c;
	char *fromcode,*tocode ;
	FILE *cfp, *ifp;
	FILE *openconv();

	while((c = getopt(argc, argv, "df:t:")) != EOF)
		switch (c) {
		case 'd':		/* delete invalid characters */
			delete = 1;
			break;
		case 'f':
			fromcode = optarg ;
			break ;
		case 't':
			tocode = optarg ;
			break ;
		}

	/* conversion table must be specified */

	if (fromcode == (char *) 0 || tocode == (char *) 0 )
		usage() ;
	/*
	 * open and read in the conversion table 
	 */
	if ((cfp = openconv(fromcode,tocode)) == (FILE *)NULL)
		exit(1);
	if (readconv(cfp))
		exit(1);

	/*
	 * process the list of files or stdin if no names given
	 */
	if (argc - optind) {
		/*
		 * skip to file list
		 */
		while (optind < argc)
			if ((ifp = fopen(argv[optind++], "r")) == (FILE *)NULL)
				fprintf(stderr, "iconv: cannot open %s\n", argv[optind]);
			else {
				convert(ifp);
				fclose(ifp);
			}
	} else
		convert(stdin);
	exit(0);
}
		
usage()
{
	fputs("usage: iconv [-d] -f fromcode -t tocode [file..]\n", stderr);
	exit(1);
}

/* 
 * now open the conversion table which lives in ICONV  by default
 * usr/lib/intln/conv/fromname_toname.
 * By setting ICONV the user can overide the default. If open fails
 * print a nice message an exit
 */

FILE *
openconv(from, to)
char *from, *to;
{	char *intl;
	char fname[BUFSIZ];
	char *getenv();
	FILE *fp;
	
	if ((intl = getenv("ICONV")) == (char *)NULL)
		intl = "/usr/lib/intln";
	strcpy(fname, intl);
	strcat(fname, "/conv/");
	strcat(fname, from);
	strcat(fname, "_");
	strcat(fname, to);
	if ((fp = fopen(fname, "r")) == (FILE *)NULL)
		fprintf(stderr, "iconv: cannot open conversion table %s\n", fname);
	return fp;
}

/*
 * read the conversion file specified. The format of this is defined as
 * follows:
 *	Lines beginning with a # are comments.
 *	A \ escapes the next character if the following three charaters are
 *	octal digits this is taken to be an octal constant. 
 *	White space is ignored.
 *	Each line is expected to contain two string an input and output token
 */

readconv(cfp)
FILE *cfp;
{	char buf[BUFSIZ];
	int line = 0;
	register char *ftok, *stok;
	register char *cp;
	char *strtok();
	int error = 0;

	while ((cp = fgets(buf, BUFSIZ, cfp)) != NULL) {
		line++;
		if (buf[0] == '#')
			continue;

		/*
		 * got a valid line, break it into a pair of tokens 
		 */
		if ((ftok = strtok(cp, "\n\t ")) == NULL)
			continue;
		parse(ftok);
		if ((stok = strtok(NULL, "\n\t ")) == NULL) {
			fprintf(stderr, "iconv: missing output string at line %d\n", line);
			error++;
			continue;
		}
		parse(stok);

		/*
		 * save the tokens for use by convert
		 */
		saveconv(ftok, stok);
	}
	return error;
}

/*
 * parse a token and convert \ sequences to binary value in situ
 * NOTE: octal constants MUST consist of exactly 3 octal digits.
 */

parse(str)
char *str;
{	register char *sp;
	register int octal, nextc, c;
	char *index();

	if (sp = index(str, '\\')) {
		/*
		 * string has escapes in it, must start copying from sp
		 */
		str = sp;
		while (c = *sp++ & 0377)
			if (c == '\\') {
				/*
				 * carefully (! )process octal constant
				 */
				nextc = *sp & 0377;
				if (nextc >= '0' && nextc <= '3') {
				    octal = (nextc - '0') << 6;
				    nextc = sp[1] & 0377;
				    if (nextc >= '0' && nextc <= '7') {
					octal |= (nextc - '0') << 3;
					nextc = sp[2] & 0377;
					if (nextc >= '0' && nextc <= '7') {
					    octal |= (nextc - '0');
					    sp += 3;
					    *str++ = octal;
					} else
					   continue;
				    } else
					continue;
				} else {
				    /*
				     * literal next character, watch \<nul>
				     */
				    if (nextc) {
					*str++ = nextc;
					sp++;
				    } else
					break;
				}
					
					
			} else
			 	*str++ = c;
		/*
		 * nul terminate shorter string
		 */
		*str = '\0';
			
	}
}

/*
 * simple function to allocate space for and save a copy of a string
 */

char *
strsave(cp)
char *cp;
{	char *malloc();
	char *mp;

	if ((mp = malloc(strlen(cp) + 1))  == NULL) {
		fprintf(stderr, "iconv: malloc failed");
		exit(1);
	}
	strcpy(mp, cp);
	return mp;
}

/*
 * the data structures used for the conversion. 
 *
 * We assume that the majority of conversion is 1 - N. For these cases we
 * have a 256 item array containing for each input character the ouput 
 * string (if any), if this character can introduce a N -M sequence then
 * a pointer is set pointing to a linked list of all strings which
 * have this charater as a lead in.
 */

struct multi {			/* multi byte to multi bytes sequence */
	char *input;
	int   inplen;
	char *output;
	struct multi *next;
};
	
struct first {			/* structure of first characters */
	char *output;
	struct multi *multi;
} first[256] = { 0 };

/*
 * save the conversion strings, special casing the 1 - N case
 */

saveconv(inp, out)
char *inp, *out;
{	register int c;
	struct multi *ptr;

	c = *inp & 0377;
	if (inp[1] == '\0') {
		/*
	 	 * 1 - N mapping
		 */
		if (first[c].output) {
			fprintf(stderr, "iconv: output string already defined for %s\n", inp);
			return;
		}
		first[c].output = strsave(out);
	} else {
		/*
		 * N - M case, place in linked list off first character 
		 */
		if ((ptr = (struct multi *)malloc(sizeof(struct multi))) == (struct multi *)NULL) {
			fprintf(stderr, "iconv: malloc failed");
			exit(1);
		}
		ptr->input = strsave(inp + 1);
		ptr->inplen = strlen(inp) - 1;
		ptr->output = strsave(out);
		insert(ptr, &first[c].multi);
	}
}

/*
 * insert the N - M conversion at the head of the list.
 */
insert(item, head)
register struct multi *item, **head;
{	

	if (*head == (struct multi *)NULL) {
		*head = item;
		item->next = (struct multi *)NULL;
	} else {
		item->next = *head;
		*head = item;
	}
}


/*
 * actually convert the input file, 1 - N mappings are handled inline for 
 * speed and domulti handles the N - M case
 */
convert(ip)
FILE *ip;
{	char buf[BUFSIZ];
	register char *cp;
	register int c, count;

	while ((cp = fgets(buf, BUFSIZ, ip)) != NULL)
		while (c = *cp++ & 0377) {
			if (first[c].multi && (count = domulti(c, cp))) {
				cp += count;
				continue;
			} 
			/*
			 * if not a N - M case output the string for the single
			 * character OR else the original if no conversion
			 * defined
			 */
			if (first[c].output)
				fputs(first[c].output, stdout);
			else if ( ! delete )
				putchar(c);
		}
}

/*
 * scan the N - M char list hanging off the head pointed to by c
 * take care to find the LONGEST match and tell the caller how
 * many characters we matched on
 */

domulti(c, cp)
int c;
char *cp;
{	struct multi *ptr;
	int maxlen = 0;
	char *out = NULL;

	ptr = first[c].multi;
	while (ptr) {
		if (strncmp(cp, ptr->input, ptr->inplen) == 0)
			if (ptr->inplen > maxlen) {
				maxlen = ptr->inplen;
				out = ptr->output;
			}
		ptr = ptr->next;
	}
	/*
	 * if we reach here either no match was found or a match
	 * of count length was found
	 */
	if (maxlen) 
		fputs(out, stdout);
	return (maxlen);
}
