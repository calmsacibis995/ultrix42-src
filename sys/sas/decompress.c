/*
 * decompress.c
 */

#ifndef lint
static char *sccsid = "@(#)decompress.c	4.1	(ULTRIX)	7/2/90";
#endif lint

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
/************************************************************************
 *
 *			Modification History
 *
 *	Paul Shaughnessy, 17-Feb-86
 *	Seperated decompress algorithm from file compress.c. Added
 *	parameters to allow decompress to return only the number
 *	of bytes requested by the caller. Also added code to process
 *	the input if it is not in compressed format. Added buffering
 *	to speed up input and output. 
 *
 ***********************************************************************/

#define BITS	16			/* BITS mod 1024 must equal 0 */
#define HSIZE	69001			/* 95% occupancy */
#define	MAXBUF	5120			/* max size for i/o bufs */
#define BLKSZ	512			/* disk block size */

/*
 * a code_int must be able to hold 2**BITS values of type int, and also -1
 */

typedef long int	code_int;
typedef long int	count_int;

typedef	unsigned char	char_type;

char_type magic_header[] = { "\037\235" };	/* 1F 9D */

/* Defines for third byte of header */
#define BIT_MASK	0x1f
#define BLOCK_MASK	0x80

/* Masks 0x40 and 0x20 are free.  I think 0x20 should mean that there is
 * a fourth header byte (for expansion).
 */

#define INIT_BITS 9			/* initial number of bits/code */

/*
 * compress.c - File compression ala IEEE Computer, June 1984.
 *
 * Authors:	Spencer W. Thomas	(decvax!harpo!utah-cs!utah-gr!thomas)
 *		Jim McKie		(decvax!mcvax!jim)
 *		Steve Davies		(decvax!vax135!petsd!peora!srd)
 *		Ken Turkowski		(decvax!decwrl!turtlevax!ken)
 *		James A. Woods		(decvax!ihnp4!ames!jaw)
 *		Joe Orost		(decvax!vax135!petsd!joe)
 */

#include <ctype.h>
#ifdef KERNEL
#include "../h/types.h"
#else KERNEL
#include <sys/types.h>
#endif KERNEL

int n_bits;				/* number of bits/code */
int maxbits = BITS;			/* user settable max # bits/code */
code_int maxcode;			/* maximum code, given n_bits */
code_int maxmaxcode = 1 << BITS;	/* should NEVER generate this code */
# define MAXCODE(n_bits)	((1 << (n_bits)) - 1)

/*
 * One code could conceivably represent (1<<BITS) characters, but
 * to get a code of length N requires an input string of at least
 * N*(N-1)/2 characters.  With 5000 chars in the stack, an input
 * file would have to contain a 25Mb string of a single character.
 * This seems unlikely.
 */

count_int htab [HSIZE];
unsigned short codetab [HSIZE];
#define codetabof(i)	codetab[i]
count_int hsize = HSIZE;
count_int fsize;

#define tab_prefix(i)	codetabof(i)		/* prefix code for this entry */
#define tab_suffix(i) ((char_type *)(htab))[i]    /* last char in this entry */
#define stack	      ((char_type *)&tab_suffix(1 << BITS))

code_int free_ent = 0;			/* first unused entry */
code_int getcode();


/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
int block_compress = BLOCK_MASK;
int clear_flg = 0;

/*
 * the next two codes should not be changed lightly, as they must not
 * lie within the contiguous general code space.
 */

#define FIRST	257	/* first free entry */
#define	CLEAR	256	/* table clear output code */

int count  = 0;		/* number of times char is outputted */
int index  = 0;		/* position in input buffer */
/*
 * Global variables used by decompress()
 */

char_type *stackp;
int finchar;
code_int code, oldcode, incode;

int first_time = 1;			/* first_time     = true */
char_type magic_fil1, magic_fil2;	/* temp hold for file magic num */
char_type data_buf[MAXBUF];		/* input buffer */
char_type *buf1p = data_buf;		/* pointer to begining of input buf */
char_type *buf2p;			/* pointer to input_buffer */
char out_hold_buf[1024];		/* output buffer */

decompress(out_buf, bytes_requested)
char *out_buf;
int  bytes_requested;
 {

	int  total_count = 0;		/* total # chars bcopied to out_buf */
	int  num_want;			/* temp hold */
	char *obuf1p = out_hold_buf;	/* pointer to beginning of output buf */
	char *obuf2p;			/* pointer to output buffer */
	char *oindexp = out_buf;	/* pointer to current location in out_buf */

	stackp = stack;

	/*
	 * A number of variables have to be initialized one time
	 * only.
	 */

	if (first_time) {
		maxbits = BITS;
		maxmaxcode = 1 << maxbits;
		maxcode = MAXCODE(n_bits = INIT_BITS);

		/*
		 * Fill input buffer.
		 */

		if (read(buf1p, MAXBUF) != MAXBUF)
			return(-1);
		first_time = 0;		/* first_time = false */

		if (read_compress((char_type *)&magic_fil1, 1) == -1)
			return(-1);
		if (read_compress((char_type *)&magic_fil2, 1) == -1)
			return(-1);
		/*
	 	 * check magic number of file to verify compression was
	 	 * performed.
	 	 */
	
		if ((magic_fil1 != (magic_header[0] & 0xFF))
 		|| (magic_fil2 != (magic_header[1] & 0xFF))) {
			printf("Image not compressed.\n");
			return(-1);
		}
		else {			/* File was in compressed format */

			if (read_compress((char_type *)&maxbits, 1) == -1)
				return(-1);
			block_compress = maxbits & BLOCK_MASK;
			maxbits &= BIT_MASK;
			maxmaxcode = 1 << maxbits;
			if(maxbits > BITS) {
				printf("Input file compressed with %d bits, can only handle %d bits\n", maxbits, BITS);
			return(-1);
			}

			/*
 		 	 * Initialize the first 256 entries in the table.
 		 	 */

			for ( code = 255; code >= 0; code-- ) {
				tab_prefix(code) = 0;
				tab_suffix(code) = (char_type)code;
    			}
			free_ent = ((block_compress) ? FIRST : 256 );

			finchar = oldcode = getcode();

			/*
			 * place the first character of the input file
			 * into the output buffer.
			 */

			out_hold_buf[count++] = (char)finchar;
		}
	} /* end first_time */

	while ( (code = getcode()) > -1 ) {
		if ( (code == CLEAR) && block_compress ) {
			for ( code = 255; code >= 0; code-- )
				tab_prefix(code) = 0;
			clear_flg = 1;
			free_ent = FIRST - 1;
			if ( (code = getcode ()) == -1 ) /* O, death! */
				break;
		}
		incode = code;

		/*
	 	 * Special case for KwKwK string.
	 	 */

		if ( code >= free_ent ) {
			*stackp++ = finchar;
			code = oldcode;
		}

		/*
		 * Generate output characters in reverse order
		 */

		while ( code >= 256 ) {
			*stackp++ = tab_suffix(code);
			code = tab_prefix(code);
		}
		*stackp++ = finchar = tab_suffix(code);

		/*
		 * Generate the new entry.
		 */

		if ( (code=free_ent) < maxmaxcode ) {
			tab_prefix(code) = (unsigned short)oldcode;
			tab_suffix(code) = finchar;
			free_ent = code+1;
		}

		/*
		 * Remember previous code.
		 */
		oldcode = incode;

		/*
		 * And put chars into the local output buffer in forward order
		 */

		do {
			out_hold_buf[count++] = *--stackp;

			/*
		 	 * if the number of characters sitting in the output
		 	 * buffer, exceeds the boundary...
		 	 */

			if (count >= BLKSZ) {

				/*
			  	 * copy a disk blocks worth of data from the local
			 	 * output buffer into out_buf and update total_count.
			 	 */

				bcopy(obuf1p, oindexp, BLKSZ);
				total_count += BLKSZ;

				/*
			 	 * update current location in out_buf
			 	 */

				oindexp += BLKSZ;

				/*
			 	 * fix up the local output buffer to start where
			 	 * the user supplied out_buf left off. Since the
			 	 * bcopy above only copies a disk blocks worth
			 	 * of information, and the local output buffer is
			 	 * 2 * BLKSZ in length, a bcopy from BLKSZ index
			 	 * in the output buffer into the beginning of the
			 	 * output buffer will accomplish the task.
			 	 */

				obuf2p = &out_hold_buf[BLKSZ];
				bcopy(obuf2p,obuf1p, BLKSZ);
				count -= BLKSZ;
			}

			/*
		 	 * Copy only the number of bytes needed to fill request.
		 	 * Must update all the pointers and buffers, just incase
		 	 * we are called again to pick up where we left off.
		 	 */

			if (total_count + count >= bytes_requested) {
				num_want = bytes_requested - total_count;
				bcopy(obuf1p, oindexp, num_want);
				/*
				 * Remove characters from output buffer
				 */
				obuf2p = &out_hold_buf[num_want];
				bcopy(obuf2p, obuf1p, 1024 - num_want);
				count -= num_want;
				total_count += num_want;
				/*
				 * Must clear out stackp incase we are
				 * called again.
				 */
				if (stackp != stack)
				do
					out_hold_buf[count++] = *--stackp;
				while (stackp > stack);
				goto complete;
			}
		} while (stackp > stack);
	}

complete:
	/*
	 * If anything went wrong, including the read, return a -1 to
	 * the caller. If all went well, return the total_number of bytes
	 * transfered from the local output buffer into the user supplied
	 * out_buf.
	 */

	if (code == -1)
		return(-1);
	return(total_count);
}

/*****************************************************************
 * TAG( getcode )
 *
 * Read one code from the input file.  If EOF, return -1.
 * Inputs:
 * 	input file.
 * Outputs:
 * 	code or -1 is returned.
 */

code_int
getcode() {
    /*
     * On the VAX, it is important to have the register declarations
     * in exactly the order given, or the asm will break.
     */
    register code_int code;
    static int offset = 0, size = 0;
    static char_type buf[BITS];
    register int r_off, bits;
    register char_type *bp = buf;

    if ( clear_flg > 0 || offset >= size || free_ent > maxcode ) {
	/*
	 * If the next entry will be too big for the current code
	 * size, then we must increase the size.  This implies reading
	 * a new buffer full, too.
	 */
	if ( free_ent > maxcode ) {
	    n_bits++;
	    if ( n_bits == maxbits )
		maxcode = maxmaxcode;	/* won't get any bigger now */
	    else
		maxcode = MAXCODE(n_bits);
	}
	if ( clear_flg > 0) {
    	    maxcode = MAXCODE (n_bits = INIT_BITS);
	    clear_flg = 0;
	}
	size = read_compress((char_type *)buf, n_bits);
	if ( size <= 0 )
	    return -1;			/* Read failed !!!! HELP !!!! */
	offset = 0;
	/* Round size down to integral number of codes */
	size = (size << 3) - (n_bits - 1);
    }
    r_off = offset;
    bits = n_bits;
    asm( "extzv   r10,r9,(r8),r11" );
    offset += n_bits;

    return code;
}

/*
 * read_compress controlls the input buffering. When called, two parameters
 * are supplied. buf, is a pointer to a buffer where the caller wants
 * num_of_bytes bytes to go. The input buffer can be any number of BLKSZ's
 * in length. The buffering works by first satisfying the request. Then
 * a check to see if the last request fell into the last BLKSZ chunk of
 * the buffer. If it did, the last BLKSZ chunk of data will be copied
 * into the top of the input buffer and a read request to fill the remainder
 * of the buffer will be issued.
 */

read_compress(buf, num_of_bytes)
char_type *buf;
int num_of_bytes;
{
	int ind;		/* temp index counter */
	int loop_end;		/* index for loop */
	int return_code;	/* number of characters copied to buffer */

	return_code = 0;
	loop_end = index + num_of_bytes;

	/*
	 * Satisfy the request
	 */


	for (ind = index; ind < loop_end; ind++) {
		return_code++;
		*buf++ = data_buf[index++];
	}

	/*
	 * Perform buffering scheme.
	 */

	if (index > MAXBUF - BLKSZ) {
		buf2p = &data_buf[MAXBUF - BLKSZ];
		bcopy(buf2p, buf1p, BLKSZ);
		buf2p = &data_buf[BLKSZ];
		if (read(buf2p, MAXBUF - BLKSZ) != MAXBUF - BLKSZ)
			return(-1);
		index -= (MAXBUF - BLKSZ);
	}
	return(return_code);
}
