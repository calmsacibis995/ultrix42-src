/*	@(#)parser.c	4.1				7/2/90				      */
/*
 * Program parser.c,  Module 
 *
 *									
 *			Copyright (c) 1985 by			
 *		Digital Equipment Corporation, Maynard, MA
 *			All rights reserved.		
 *						
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Digital Equipment
 * Corporation.
 *
 * Digital assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by Digital.
 *
 *
 * Program parser.c,  Module 
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 *
 * 1.00 10-Jul-1985
 *     DECnet Ultrix V1.0
 *
 */

/*******************************************************************************
**
**
**	parser.c
**
**
**	Run time parser for upars system
**
**
*******************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>
#include "upars.h"
#include "utables.h"



int	 _u_parse();
void	 _u_debug0();
void	 _u_debug1();
void	 _u_debug2();

int	 __u_parse();





extern	void	 _u_init();
extern	void	 _u_blankson();
extern	void	 _u_blanksoff();
extern	void	 _u_text();
extern	void	 _u_binary();
extern unsigned char	 _u_getchar();
extern	void	 _u_copy();
extern unsigned char	*_u_nextchar();
extern u_BOOL	 _u_incr();



u_BOOL	 u_parse_flag;


extern unsigned char	*u_cc;	/* master buffer pointer */
extern short	 u_left;	/* number of characters left in buffer */
extern unsigned char	 u_text[]; /* Latest token matched. Ends with '\0' for */
				/* easy copys and compares . . . */
extern short	 u_length;	/* length of token in u_text, minus '\0' */



#define	u_REJECT	u_ok = FALSE



/*
**	SCCS def
*/

static	char	 PARSER[] = "      @(#)parser.c	4.1                 7/2/90";



static jmp_buf	u_envp;




void	 _u_debug0(p, status)
_utbl_entry	*p;
u_BOOL		*status;
{
	if (!GET_DEBUG(p))
		return;

	*status = TRUE;
	if (isprint((char)*u_cc))
		fprintf(stderr, "\nInput is '%c'\t0x%x\t%d", *u_cc, *u_cc, *u_cc);
	else
		fprintf(stderr, "\nInput is ''  \t0x%x\t%d", *u_cc, *u_cc, *u_cc);
	if (GET_TYPE(p))
		fprintf(stderr, "\n%s", get_keyword(p->type));
	else
		fprintf(stderr, "\nTable Subroutine Call=======>");

	if (GET_ARG(p))
		fprintf(stderr, "\t%d", p->arg);
	else
		fprintf(stderr, "\t");
}



void	 _u_debug1(p, ok, status)
_utbl_entry	*p;
u_BOOL	 	 ok;	
u_BOOL		*status;
{
	char	 temp_buffer[20];

	if (!GET_DEBUG(p))
		return;
	
	*status = ok;

	if (!GET_TYPE(p))
		fprintf(stderr, "==============>");

	if (*status)
		fprintf(stderr, "\t succeeded");
	else
		fprintf(stderr, "\t failed\t");

}



static void	 _u_debug2(p, ok, status)
_utbl_entry	*p;
u_BOOL		 ok;
u_BOOL		*status;
{
	char	temp_buffer[16];

	if (!GET_DEBUG(p))
		return;



	if (GET_ACTION(p))
		if (*status && ok)
			fprintf(stderr, "\taction succeeded");
		else if (*status && !ok)
			fprintf(stderr, "\taction failed");
		else
			fprintf(stderr, "\taction cancelled");
	else
		fprintf(stderr, "\t no action");

	*status &= ok;

	*temp_buffer = '\0';
	if (*status)
	{
		if (GET_EXIT(p))
			fprintf(stderr, "\t transition is $EXIT\n\n");
		else if (GET_FAIL(p))
			fprintf(stderr, "\t transition is $FAIL\n\n");
		else if (GET_RETURN(p))
			fprintf(stderr, "\t transition is $RETURN\n");
		else if (GET_ERROR(p))
			fprintf(stderr, "\t $ERROR returns %d\n\n", p->arg);
		else
			fprintf(stderr, "\t transition to next state\n");
	}
	
	else if (GET_LAST(p))
		fprintf(stderr, "\t *** last transition fails\n\n");
	else
		fprintf(stderr, "\t transition falls through \n");
}


int	_u_parse(buffer, buflen, tbl_ptr)
char	*buffer;
_utbl_entry	*tbl_ptr;
short	 buflen;
{
	int	k;

	if ((k = setjmp(u_envp)) != 0)
		return (k);

	k = __u_parse(buffer, buflen, tbl_ptr);
	return (k);
}



int	__u_parse(buffer,  buflen, tbl_ptr)
char	*buffer;
_utbl_entry	*tbl_ptr;
short	 buflen;
{
	_utbl_entry	*p;
	unsigned short	 arg;
	short	 token;
	u_BOOL		u_ok;
	u_BOOL		u_status;

	p = tbl_ptr;
	_u_init(buffer, buflen);

	while (p != NULL) 
	{
		u_ok = TRUE;

		_u_debug0(p, &u_status);

		if (GET_TYPE(p))
		{
			token = p->type;
			if (u_parse_flag)
		 	{
				if (GET_ARG(p))
					arg = p->arg;
				if(!_u_nlex(token, arg))
					u_REJECT;
			}
			else if (!_u_tlex(token))
				u_REJECT;
		}
		else
			if (!__u_parse(u_cc, u_left, p->sub))
				u_REJECT;


		_u_debug1(p, u_ok, &u_status);

		if (u_ok)
			if (GET_ACTION(p))
				if (GET_PARAM(p))
					if (!(p->action(p->param)))
						u_REJECT;
					else;
				else if (!(p->action()))
					u_REJECT;

		_u_debug2(p, u_ok, &u_status);

		if (u_ok)
		{
			if (GET_MASKADDR(p))
				if (GET_MASK(p))
					*(p->mask_addr) |=  p->mask;
				else 
					_u_copy(p->mask_addr);

			if (GET_FAIL(p))
				return (FALSE);

			if (GET_EXIT(p))
				longjmp(u_envp, TRUE);

			if (GET_RETURN(p))
				return (TRUE);

			if (GET_ERROR(p))
				longjmp(u_envp, p->arg);

			p = p->label;
		}

		else if (GET_LAST(p))
			return(FALSE);

		else
			p = p->fail;
	} 

	fprintf(stderr, "*****UPARS Runtime ERROR - state not terminated\n");
	exit(1);
 

}




int	_u_dork()
{
	fprintf(stderr,  "*****UPARS Runtime ERROR: ");
	fprintf(stderr,  "Transition to non_existant action routine\n");
	exit(1);
}
