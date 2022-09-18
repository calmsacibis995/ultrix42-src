/*	@(#)tables.c	4.1				7/2/90	*/
/*******************************************************************************
/*
 * Program tables.c,  Module 
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
 * Program tables.c,  Module 
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
**	tables.c
**
**	handle UPARS state tables
**
**
**
**	These routines handle the building and output of UPARS state tables.
**
**
*******************************************************************************/




#include <stdio.h>
#include "upars.h"
#include "utables.h"



void	 tbl_dump();
void	 tbl_finish();
void	 tbl_debug();
void	 tbl_fail();
void	 tbl_exit();
void	 tbl_error();
void	 tbl_return();
void	 tbl_last();
void	 tbl_stop();
void	 tbl_key();
void	 tbl_id();
void	 tbl_num();
void	 save_label();


/*
**	SCCS def
*/

static	char	 TABLES[] = "	@(#)tables.c	4.1		7/2/90		";

extern	 FILE	*define_file;
extern	 u_BOOL	 u_parse_flag;

static	 void	 tbl_start();

extern	 char	states_file_name[];
static   FILE	*fd;

static	u_BOOL	 new_state;

static	u_BOOL	 debug_flag;
static	u_BOOL	 exit_flag;
static	u_BOOL	 error_flag;
static	u_BOOL	 return_flag;
static	u_BOOL	 fail_flag;
static	u_BOOL	 labeled;
static	u_BOOL	 label_flag;
static	u_BOOL	 action_flag;


static	int	 arg_type;
static	u_BOOL	 type_sub_flag;	/* TRUE if type is a keyword, FALSE if 
				/* table subroutine call   */
static	int	 mask_type;	/* 0 if no mask, 1 if key, 2 if id, 3 if num */
static	u_BOOL	 mask_addr_flag;
static	int	 param_type;


extern	char	 table_name[];
static	char	 this_label_id[u_NAME_LENGTH];
static	short	 label_num = 0;

/*
**	The compile time parse table.  This is built during the compile,
**	and then compacted and output by tbl_dump at EOF.
*/

typedef struct tbl
 	{
	struct   tbl *next;
	struct	 tbl *back;
	short	 flags;
	char	 this_label[u_NAME_LENGTH];
	short	 type;
	char	 sub[u_NAME_LENGTH];
	long	 arg;
	char	 arg_id[u_NAME_LENGTH];
	char	 label[u_NAME_LENGTH];
	char	 action[u_NAME_LENGTH];
	long	 mask;
	char	 mask_id[u_NAME_LENGTH];
	long	 param;
	char	 param_id[u_NAME_LENGTH];
	char	 mask_addr[u_NAME_LENGTH];
	} tbl_entry;



tbl_entry	*root = NULL;		/* base of table linked list */
tbl_entry	*top = NULL;		/* top of table */


static	u_BOOL	stop_generating = FALSE;     /* If TRUE, don't gen anything */

static	char	flags_head[33] = "\t\tshort\t flags;\n";

static  char	sub_head[33] = "\t\tstruct _utbl\t*sub;\n";

static	char	type_head[33] =  "\t\tshort\t type;\n";

static	char	arg_head[33] = "\t\tunsigned short\t arg;\n";

static	char	label_head[33] = "\t\tstruct _utbl\t*label;\n";

static	char	fail_head[33] = "\t\tstruct _utbl\t*fail;\n";

static	char	action_head[33] = "\t\tint\t(*action)();\n";

static	char	mask_head[33] = "\t\tlong\t mask;\n";

static	char	maskaddr_head[33] = "\t\tlong\t*mask_addr;\n";

static	char	param_head[33] = "\t\tlong\t param;\n";


/*******************************************************************************
**
**	tbl_start
**
**	A new upars state or transition has been reached.  Make a new parse 
**	table entry, **	link it up to the table, and then set all values to 
**	their defaults.
**
*/


static	 void	tbl_start()
{
	int	 	 i;
	tbl_entry	*p;

	if (stop_generating)
		return;

	if (!(p = (tbl_entry *)malloc(sizeof(tbl_entry))))
	{
		errprint("Fatal error - table table overflow");
		exit(1);
	}

	if (root == NULL)
	{
		root = p;
		p->back = NULL;
	}
	else
		top->next = p;

	if (new_state)
	{
		p->back = NULL;
		new_state = FALSE;
	}
	else
		p->back = top;

	p->next = NULL;
	top = p;

	debug_flag = label_flag = action_flag = mask_addr_flag = fail_flag = 
       		return_flag = exit_flag = error_flag = FALSE;
	arg_type = mask_type = param_type = IS_NOTHIN;

	strcpy(p->sub, "u_fool");
	strcpy(p->mask_id, "0");
	strcpy(p->arg_id, "0");
	strcpy(p->action, "_u_dork");
	strcpy(p->mask_addr, "NULL");
	strcpy(p->label, "u_fool");
	strcpy(p->param_id, "0");

	p->type = p->arg = p->mask = p->param  = 0;
}


/*******************************************************************************
**
**	tbl_state
**
**	called by the upars parser whenever the parsing of a particular
**	state is finished.  Fill in a label, if the user put a label on
**	the state, and then string back the (user's or default) label
**	back in the table, to satisfy forward references.
*/

void	 tbl_state()
{
	tbl_entry	*p;
	char		 label_temp[u_NAME_LENGTH];

	if (stop_generating)
		return;

	if (!labeled)
	{
		strcpy(this_label_id, table_name);
		sprintf(label_temp, "%d", label_num++);
		strcat(this_label_id, label_temp);
		labeled = TRUE;
	}

	for (p = top; p != NULL; p = p->back)
		if (!GET_FAIL(p) && !GET_EXIT(p) && !GET_RETURN(p) &&
		    !GET_LABEL(p))
			strcpy(p->label, this_label_id);
		
	new_state = TRUE;
}

/*******************************************************************************
**
**	tbl_finish
**
**	Called whenever the parsing of a upars transition is finished.
**	Packs flag bits into flag word of the table, and labels the
**	transition with either the internally generated or user generated
**	label.
*/

void	 tbl_finish()
{
	char	 label_temp[u_NAME_LENGTH];

	top->flags = type_sub_flag | label_flag << LABEL_POS | 
		     action_flag << ACTION_POS | 
		     param_type << PARAM_POS | 
                     mask_addr_flag << MASKADDR_POS | 
                     mask_type << MASK_POS | 
		     debug_flag << DEBUG_POS | 
		     arg_type << ARG_POS |
		     return_flag << RETURN_POS |
		     exit_flag << EXIT_POS | 
		     error_flag << ERROR_POS |
		     fail_flag << FAIL_POS ; 

	if (!labeled)
	{
		strcpy(top->this_label, table_name);
		sprintf(label_temp, "%d", label_num++); 
		strcat(top->this_label, label_temp);
	}
	else
		strcpy(top->this_label, this_label_id);
	
	labeled = FALSE;
}


/*******************************************************************************
**
**	tbl_stop, tbl_debug, tbl_exit, tbl_error, tbl_fail, tbl_return,
**	tbl_last.
**
**
**	These provide data abstraction and independence to the table
**	generation routines.  Their use is kinda obvious.  tbl_stop
**	is used to stop generating tables whenever a yacc compiler error
**	is reached.  All the others set values for individual upars
**	transactions.
*/



void	 tbl_stop()
{
	stop_generating = TRUE;
}



void	 tbl_debug()
{
	debug_flag = TRUE;
}


void	 tbl_exit()
{
	exit_flag = TRUE;
}


void	 tbl_error()
{
	error_flag = TRUE;
}


void	 tbl_fail()
{
	fail_flag = TRUE;
}


void	 tbl_return()
{
	return_flag = TRUE;
}


void	 tbl_last()
{
	top->flags = top->flags | (TRUE << LAST_POS);
}




/*******************************************************************************
**
**	tbl_key, tbl_id, tbl_num
**
**	These provide semantic checking and table input to/from the
**	parser when arguments of a upars transition are being parsed.
**	Depending upon which arg (argument number), certain types
** 	(keyword, id, or number) are allowed as arguments.  The
**	proper compile-time table values are set, or errors returned.
*/

	
	


void	 tbl_key(arg, key)
int	 arg;
short	 key;
{

	switch (arg)
	{
	    case TYPE:
		type_sub_flag = TRUE;
		tbl_start();
		top->type = key;
		break;

	    case ARG:
		arg_type = IS_KEY;
		top->arg = key;
		break;

	    case MASK:
		mask_type = IS_KEY;
		top->mask = key;
		break;

	    case PARAMETER:
		param_type = IS_KEY;
		top->param = key;
		break;

	    default:
		errprint("Keyword is illegal argument");
	}
}







void	 tbl_id(arg, id)
int	 arg;
char	*id;
{
	
	switch (arg)
	{
	    case SUBROUTINE:
		type_sub_flag = FALSE;
		tbl_start();
		strcpy(top->sub, id);
		break;

	    case ARG:
		arg_type = IS_ID;
		strcpy(top->arg_id, id);
		break;

	    case LABEL:
		label_flag = TRUE;
		strcpy(top->label, id);
		break;
	
	    case ACTION:
		action_flag = TRUE;
		strcpy(top->action, id);
		break;

	    case MASK:
		mask_type = IS_ID;
		strcpy(top->mask_id, id);
		break;

	    case MASK_ADDR:
		mask_addr_flag = TRUE;
		strcpy(top->mask_addr, id);
		break;

	    case PARAMETER:
		param_type = IS_ID;
		strcpy(top->param_id, id);
		break;


	    default:
		errprint("Illegal argument");
	}
}






void	 tbl_num(arg, num)
int	 arg;
long	 num;
{

	switch (arg)
	{
	    case ARG:
		arg_type = IS_NUM;
		top->arg = num;
		break;

	    case MASK:
		mask_type = IS_NUM;
		top->mask = num;
		break;

	    case PARAMETER:
		param_type = IS_NUM;
		top->param = num;
		break;

	    default:
		errprint("Illegal argument");
	}
}


/*******************************************************************************
**
**	save_label
**
**	Called by the parser whenever a a state label is encountered.
**	The label is saved for tbl_state, above.
*/


void	 save_label(label)
char	*label;
{
	strcpy(this_label_id, label);
	labeled = TRUE;
}





/*******************************************************************************
**
**	tbl_dump
**
**	Called at the end of a upars compile.  If no errors, 
**	it goes through the compile-time parse tables and
**	writes it out to the default state_table file, compacting it as
**	it goes.
**
*/


void	 tbl_dump()
{
	tbl_entry	*p;
	int		 i;

	if (stop_generating)
		return;


	if ((fd = fopen(states_file_name, "w")) == NULL)
	{
		perror("Fatal Error - Could not open %s\n", states_file_name);
		exit(1);
	}


	for (p = root; p != NULL; p = p->next)
	{
		fprintf(define_file, "_utbl_entry\t%s; \n", p->this_label);
	}

	fprintf(define_file, "_utbl_entry\tu_fool;\n");
	fprintf(define_file, "int\t_u_dork();\n");
	fclose(define_file);



	for (p = root; p != NULL; p = p->next)
	{
		fprintf(fd, "\n_utbl_entry  %s = {", p->this_label);
		fprintf(fd, "0x%x", p->flags);

		fprintf(fd, ", %d", p->type);
		fprintf(fd, ", &%s", p->sub);

		switch (GET_ARG(p))
		{
		    case IS_KEY:
			fprintf(fd, ", %s, ", get_keyword(p->arg));
			break;

		    case IS_ID:
			fprintf(fd, ", %s", p->arg_id);
			break;

		    default:
			fprintf(fd, ", %ld", p->arg);
			break;
		};
	
		fprintf(fd, ", &%s", p->label);

		if (!GET_LAST(p))
			fprintf(fd, ", &%s", p->next->this_label); 
		else
			fprintf(fd, ", &u_fool");
			
		fprintf(fd, ", %s", p->action);

		switch(GET_MASK(p))
		{
		    case IS_KEY:
			fprintf(fd, ", %s", get_keyword(p->mask));
			break;
			
		    case IS_ID:
			fprintf(fd, ", %s", p->mask_id);
			break;

	       	    default:
			fprintf(fd, ", %ld", p->mask);
			break;
		};

		if (GET_MASKADDR(p))
			fprintf(fd, ", &%s", p->mask_addr);
		else
			fprintf(fd, ", NULL");

		switch(GET_PARAM(p))
		{
		    case IS_KEY:
			fprintf(fd, ", %d", p->param);
			break;
			
		    case IS_ID:
			fprintf(fd, ", %s", p->param_id);
			break;

		    default:
			fprintf(fd, ", %ld", p->param);
			break;
		};


		fprintf(fd, "};\n");
	}

	fclose(fd);
}

