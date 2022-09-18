/*	@(#)upars.y	4.1				7/2/90        */
/*
 * Program upars,  Module 
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
 * Program upars,  Module 
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
**	upars.y
**
**	upars compiler yacc state definitions and main routines.
**
**
**
**	This hack defines the syntax of upars, the version of LIB$TPARS
**	available under Ultrix.
**
*/


%token	INIT_STATE
%token	STATE
%token	DEBUG
%token	IDENTIFIER
%token	INTEGER
%token	COLON
%token	BINARY	
%token	TEXT
%token	PERCENTS
%token	COMMA
%token  BANG
%token	SEMICOLON
%token  RBRACE
%token  LBRACE
%token	PARSE	
%token	KEYWORD
%token	EXIT
%token	RETURN
%token  ERROR	
%token  FAIL


%%

upars_program :	
		keyword_table state_tables ;

keyword_table :
		parse keywords PERCENTS = 
		{
		keyword_dump();
		}

	    |	parse PERCENTS =
		{
		keyword_dump();
		};

parse :
		parse_head COMMA DEBUG =
		{
			u_debug = TRUE;
		}

	     |  parse_head = 
		{
			u_debug = FALSE;
		};

parse_head:
		PARSE TEXT  =
		{
			u_parse_flag = FALSE;
			key_init(u_parse_flag);
		}

	    |   PARSE BINARY  =
		{
			u_parse_flag = TRUE;		
			key_init(u_parse_flag);
		};

keywords :
		keywords keyword
	    |   keyword ;

keyword :
		key_head INTEGER = 
		{
			add_keyword(id_, NULL, (short)yyintval);
		}

	    |   key_head IDENTIFIER  =
		{
			if (!u_parse_flag)
				errprint("Illegal keyword in text parse");
			else
				add_keyword(id_, yytext, 0);
		};

key_head:
		KEYWORD IDENTIFIER  =
		{
			make_id(yytext);
		} ;


state_tables :
		states PERCENTS  =
		{
			tbl_dump();
			pgm_dump();
		}


states :
		states state
	    |   state ;

state :
		label st
	    |   st ;

st :
		state_head transitions RBRACE  =
		{
			tbl_last();
		}

state_head:
		STATE LBRACE  =
		{
			tbl_state();
		}

label :
	  	label_head COLON  =
		{
			save_label(id_);
		} ;

label_head:
		IDENTIFIER  =
		{
			make_id(yytext);
		} ;

transitions :
		transitions transition
	    |   transition ;

transition:
		types args SEMICOLON  =
		{
			if ($2 == COMMA)
				errprint("Missing argument");
			else
			{
				if (u_debug)
					tbl_debug();
				tbl_finish();
			}
		}

	    |   types SEMICOLON  =
		{
			if (u_debug)
				tbl_debug();
			tbl_finish();
		}

types :
		type arg  =
		{
			if (!u_parse_flag && ($1 != ERROR))
				errprint("Missing comma");
			else 
			{
				if ($2 == INTEGER)
					tbl_num(ARG, yyintval);

				else if (!(offset = search_keyword(yytext)))
					tbl_id(ARG, yytext);
				else
					tbl_key(ARG, offset);
			}
		}
	
	    |   type ;

type:
		IDENTIFIER  =
		{
			if (!(offset =search_keyword(yytext)))
				errprint("No keyword in transition");
			else
				tbl_key(TYPE, offset);
		}

	    |   BANG IDENTIFIER =
		{
			if (search_keyword(yytext))
				errprint("Keyword is target of table subroutine call");
			else
				tbl_id(SUBROUTINE, yytext);
		} 
		
	    |	ERROR  =
		{
			tbl_key(TYPE, search_keyword(yytext));
			tbl_error();
			$$ = ERROR;
		};

args : 
		args1  COMMA DEBUG  =
		{
			tbl_debug();
			$$ = COMMA + 1;
		}

	    |   args1 ;

args1 :
		args2 COMMA arg  =
		{
			if ($3 == INTEGER)
				tbl_num(PARAMETER, yyintval);

			else
				tbl_id(PARAMETER, id_);

			$$ = COMMA + 1;
		}

	    |   args2 COMMA  =
		{
			$$ = COMMA;
		}

	    |   args2 ;

args2 :
		args3 COMMA arg  =
		{
			if ($3 != IDENTIFIER)
				errprint("Mask Address must be an identifier");
		
			else if (search_keyword(id_))
				errprint("Mask Address is a keyword");

			else
				tbl_id(MASK_ADDR, id_);
			
			$$ = COMMA + 1;
		}

	    |   args3 COMMA  =
		{
			$$ = COMMA;
		}

	    |   args3 ;

args3 :
		args4 COMMA arg  =
		{
			if ($3 == INTEGER)
				tbl_num(MASK, yyintval);
/*
			else if (offset = search_keyword(id_)) 
				errprint("Keyword used as mask");
*/
			else
				tbl_id(MASK, id_);
		
			$$ = COMMA + 1;
		}

	    |   args4 COMMA  =
		{
			$$ = COMMA ;
		}

	    |   args4 ;

args4:	
		args5 COMMA arg  = 
		{
			if ($3 != IDENTIFIER)
				errprint("Action must be an identifier");
			else
			{
				if (search_keyword(id_))
					errprint("Keyword used as action");
				else
					tbl_id(ACTION, id_);
			}
			
			$$ = COMMA + 1;
		}

	    |   args5 COMMA  =
		{
			$$ = COMMA;
		}
	    |   args5 ;

args5:
		COMMA arg  =
		{
			if ($2 != IDENTIFIER)
				errprint("Label must be identifier");
			else
			{
				if (search_keyword(id_))
					errprint("Keyword used as label");
				else
					tbl_id(LABEL, id_);
			}

			$$ = COMMA + 1;
		}

	     | 	COMMA EXIT  =
		{
			tbl_exit();
			$$ = COMMA + 1;
		}

	     |	COMMA RETURN  =
		{
			tbl_return();
			$$ = COMMA + 1;
		}

	     | 	COMMA FAIL  =
		{
			tbl_fail();
			$$ = COMMA + 1;
		}

	    |   COMMA  =
		{
			$$ = COMMA;
		} ;

arg :
		IDENTIFIER  = 
		{
			make_id(yytext);
			$$ = IDENTIFIER;
		}

	    |	INTEGER  =
		{
			$$ = INTEGER;
		} ;


%%

/*
 * Program y.tab.c,  Module 
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
 * Program y.tab.c,  Module 
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 * 1.00 
 *     DECnet Ultrix V1.1
 *
 *
 */



/*******************************************************************************
**
**	y.tab.c
**
**	This is the output of the yacc parser generator, that is,
**	the parser of the upars compiler.
**
*/


#include <stdio.h>
#include <fcntl.h>
#include "upars.h"
#include "lex.yy.c"

u_BOOL	 u_parse_flag;		/* TRUE for byte, FALSE for text */
u_BOOL	 u_debug;		/*TRUE for global run-time debug */



int	 num_errs = 0;		/*Number of compiler errors*/

long	yyintval;		/* latest integer value, put there by */
				/* lex.yy.c . . .               */
static char id_[u_NAME_LENGTH];	/* Holds value of latest identifier. */
				/* 32 char ids, extra for \0 */

/*
**	SCCS def
*/

static  char 	 MAIN[] = "    @(#)upars.y	4.1 			7/2/90    " ;


static	short	 offset;

#define	BUFF_SIZE	256
static	char	 buffer[BUFF_SIZE];

FILE	*output_file;
FILE	*input_file;

/*
**	Command line options
**	Note that MAX_PATH_NAME is defined for this implementation of UNIX
**	to be 1024, but adding 5k to upars to handle a boundary condition
**	that won't even fit on the screen seems a tad ridiculous . . .
*/

#define MAX_PATH_NAME 256 



char	 input_file_name[MAX_PATH_NAME] = 	"stdin";
char	 output_file_name[MAX_PATH_NAME] = 	"u_parser.c";
char	 keyword_file_name[MAX_PATH_NAME] = 	"keyword_table.txt";
char	 defines_file_name[MAX_PATH_NAME] = 	"u_DEFINES.h";
char	 states_file_name[MAX_PATH_NAME] = 	"u_STATES.h";
char	 table_name[MAX_PATH_NAME] = 		"u_table_";

#define	o_INPUT_FILE		0
#define o_OUTPUT_FILE		o_INPUT_FILE + 1
#define o_KEYWORD_FILE		o_OUTPUT_FILE + 1
#define o_DEFINES_FILE		o_KEYWORD_FILE + 1
#define o_STATES_FILE		o_DEFINES_FILE + 1
#define o_TABLE_NAME		o_STATES_FILE + 1

#define NUM_OPTIONS		o_TABLE_NAME + 1

static u_BOOL	option_set[NUM_OPTIONS] = 
		{
			FALSE,
			FALSE,
			FALSE,
			FALSE,
			FALSE,
			FALSE
		};

static char	*option_string[NUM_OPTIONS] = 
		{
			input_file_name,
			output_file_name,
			keyword_file_name,
			defines_file_name,
			states_file_name,
			table_name
		};


/*******************************************************************************
**
**	yyerror
**
**	Since this implementation of yacc seems to be missing yyerror in
**	liby.c, we supply one.  Print out stuff, tell the table handler
**	to stop producing parse tables, and reset yacc with yyerrok.
**	yyerror takes care of yacc-detected syntax errors.
*/

			

int	yyerror()
{
	fprintf(stderr, "Upars Syntax Error at line %d", yylineno);
	fprintf(stderr, "  Current token  %s\n", yytext);
	tbl_stop();
	num_errs++;
	yyerrok;
}



/*******************************************************************************
**
**	errprint
**
**	Given a string to print, and a semantic error to flag, errprint
**	takes cars of business.  Tell the table module to stop producing
**	parse tables, also.
*/

int	errprint(string)
char	*string;
{
	fprintf(stderr,"Upars Error at line %d:  %s\n", yylineno, string);
	tbl_stop();
	num_errs++;
}



/*******************************************************************************
**
**	make_id
**
**	Whenever a state in the parse tables (above, if you can read yacc)
**	needs to make an identifier, it has made_id stick it into the 
** 	global id_.
*/


void	 make_id(id)
char	*id;
{
	strcpy(id_, id);
}



/*******************************************************************************
**
**	pgm_dump
**
**	When the '%%' between the upars states and the user's action
**	routines is encountered in a upars program, in the state called
**	"state_tables", above, pgm_dump is called to open up the output
**	file, whose name defaults to "u_parser.c".  It puts some default
**	includes into the file, and then copies the user's action 
**	routines and main program, written in C, to the file.
**
*/

void	pgm_dump()
{
	if ((output_file = fopen(output_file_name, "w")) == NULL)
	{
		perror(output_file_name);
		exit(1);
	}

	fprintf(output_file, "#include \"%s\"\n", defines_file_name);
	fprintf(output_file, "\n\#define u_KEYWORD_TABLE    \"%s\"\n",
		keyword_file_name);

	for (;;)
	{
		if (fgets(buffer, BUFF_SIZE, stdin) == NULL)
			break;
		if (fputs(buffer, output_file) == NULL)
		{
			perror (output_file_name);
			exit(1);
		}
	}
}



/*******************************************************************************
**
**	save_arg
**
**	save_arg checks if a command-line option is kosher, and if so,
**	substitutes "string" for the default option (usually, a file name);
**	"arg_string" is the actual command line option , like "-k", and
**	is only passed to stick in the error message if it's bogus.
**
*/


static void	save_arg(arg, arg_string, string)
int	 arg;
char	*arg_string;
char	*string;
{
	if (option_set[arg])
	{
		fprintf(stderr, "duplicate command line option:  %s\n", arg_string);
		exit(1);
	}

	option_set[arg] = TRUE;
	strcpy(option_string[arg], string);

}

/*******************************************************************************
**
**	main
**
**
**	The main routine of the upars compiler merely checks out the command
**	line options, opens the input file, and lets yacc do the parsing
**	of the upars program.  If it passes, main writes the #include
**	of the upars parse tables at the end of the upars parser that
**	has already been generated by pgm_dump, above, and that's it.
**
*/


	

void main(argc, argv)
int argc;
char	*argv[];
{

	while (--argc > 0)
	{
		if ((argc == 1) && (*argv[argc] != '-'))
			save_arg(o_INPUT_FILE, argv[argc], argv[argc]);

		else if ((argc != 1) && (*argv[argc-1] != '-'))
			save_arg(o_INPUT_FILE, argv[argc], argv[argc]);
		
		else if ((*argv[argc] == '-') && (*argv[argc-1] == '-'))
		{
			fprintf(stderr, "Missing command line option\n");
			exit(1);
		}


		else switch (*++argv[--argc])
		{
		    case 'o':
			save_arg(o_OUTPUT_FILE, --argv[argc], argv[argc+1]);
			break;
		
		    case 'k':
			save_arg(o_KEYWORD_FILE, --argv[argc], argv[argc+1]);
			break;
		
		    case 'd':
			save_arg(o_DEFINES_FILE, --argv[argc], argv[argc+1]);
			break;

		    case 's':
			save_arg(o_STATES_FILE, --argv[argc], argv[argc+1]);
			break;
		
		    case 't':
			save_arg(o_TABLE_NAME, --argv[argc], argv[argc+1]);
			break;

		    default:
			fprintf(stderr, "unknown option:  %s\n", --argv[argc]);
			exit(1);
		}
	}

	if (option_set[o_INPUT_FILE])
	{
		if ((input_file = fopen(input_file_name, "r")) == NULL)
		{
			perror(input_file_name);
			exit(1);
		}
		_iob[0] = *input_file;
	}

	yyparse();
	if (num_errs)
		{
		printf("%d compilation errors\n", num_errs);
		printf("No tables generated.\n");
		exit(1);
		}

	else
	{

		fprintf(output_file, "#include \"%s\"\n", states_file_name);
		fclose(output_file);

		printf ("Compilation successful\n");
	}
}
