#ifndef lint
static char *sccsid = "@(#)u_main.c	4.1      ULTRIX 7/2/90";
#endif

/* file: u_main.c -- shell for ansi_to_ps translator */

/************************************************************************
 *                                                                      *
 *      COPYRIGHT  (c)  DIGITAL  EQUIPMENT CORPORATION 1985,            *
 *            1986, 1987,1988, 1989.   ALL RIGHTS RESERVED.             *
 *                                                                      *
 *      THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE           *
 *      USED AND COPIED ONLY IN ACCORDANCE  WITH THE TERMS OF           *
 *      SUCH  LICENSE  AND  WITH  THE  INCLUSION OF THE ABOVE           *
 *      COPYRIGHT  NOTICE.  THIS SOFTWARE OR ANY OTHER COPIES           *
 *      THEREOF   MAY  NOT  BE  PROVIDED  OR  OTHERWISE  MADE           *
 *      AVAILABLE  TO  ANY  OTHER  PERSON.  NO  TITLE  TO AND           *
 *      OWNERSHIP  OF  THE  SOFTWARE  IS  HEREBY TRANSFERRED.           *
 *                                                                      *
 *      THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE           *
 *      WITHOUT  NOTICE  AND SHOULD  NOT BE  CONSTRUED  AS A            *
 *      COMMITMENT  BY  DIGITAL EQUIPMENT CORPORATION.                  *
 *                                                                      *
 *      DIGITAL  ASSUMES  NO RESPONSIBILITY  FOR THE  USE  OR           *
 *      RELIABILITY  OF ITS SOFTWARE ON EQUIPMENT THAT IS NOT           *
 *      SUPPLIED BY DIGITAL.                                            *
 ************************************************************************/


/*
 * Modification History
 *
 * 13 Nov 89	Adrian Thoms
 *	Merged in the argument parsing code and item list building
 *	code from the old ansi xlator
 *	Deleted the string constants for the preamble which now
 *	lives in a file.
 *
 */
/************************************************************************
 * Translator options
 * ------------
 * 
 *  -F <page_size>	a,a3,a4,a5,b,b4,b5,executive,ledger,legal,letter
 * 			default is a4
 *  -O <orientation>	landscape,portrait
 * 			default is portrait
 *  -v <vmsize>		0 (1Mb), 1 (256k), 2 (470000b)
 * 			default is 1Mb vm
 *  -e			send <CSI>20h, setting linefeed/newline
 *  -s			inhibit final showpage - needed for multi-page printing
 *  -m <8f|7f|8g|7g>	select output mode
 *  -g			select GL(/GR) only output mode
 * 			default is full output mode
 *  -h 			select sixel format - sixelfont or hexstring
 * 			default is sixelfont
 *  -R <resource string>info on (one) pre-loaded resource
 * 			info on up to MAXRESOURCES resources can be passed
 *  -x			don't reset postscript state on entry
 *  -z			don't reset postscript state on exit
 * 
 *  The order of the options is unimportant.
 * 
 ***************************************************************************/


#include	<stdio.h>
#include	<strings.h>
#include	"portab.h"
#include	"argstrings.h"
#include	"page_sizes.h"

#include	"trn$.hc"
#include	"cpsys.hc"
#include	"caglobal.hc"

CONST BYTE str_time_date_and_version [] = {"\
% Date of this preamble: Tue May  2 11:51:18 1989\n\
%    .ident	/LPS_TRN V3.1-57/  \n\
"};

char str_prologue [30000] ;

static int	lfnl_flag;
static char	lfnl_str[] = {0x9b,'2','0','h'};

#define	OBUFSIZE	8192
#define	IBUFSIZE	2048

#define trn$k_sheet_size_a5 12          
#define trn$k_sheet_size_b4 13
#define trn$k_sheet_size_b5 14
#define trn$k_sheet_size_exec 15

ITEM	itemlist [20];
TRN$K_ANSI_SPECIAL	trn$k_ansi_special;
PL	trn$_resource_table_ptr;


int	fin, fout;
int	i, vm_size, sgr;
UBYTE	*co, *p;
BOOL	set_vm_size, noshowlastpage;
BOOL    no_prologue_with_job;
UBYTE	orientation;
UBYTE	paper_size;
UBYTE	output_mode;
UBYTE	image_comp;
UBYTE	vid[8]="V3.1-57";
BYTE	reset_ent, reset_ex;
UBYTE	output_prologue_only;

#define MAXRESOURCES	20
#define ENTRYLEN	322

static PAGE_SIZE *page_size = NULL;

static char	*resource_ptr[MAXRESOURCES];
static int	rescount = 0;
static BYTE	(*resource_table) [ENTRYLEN];
static struct entry_ptr_type {
	BYTE *curptr;
	BYTE *endptr;
};

static void fatal();

char *program_name;			/* name we were called with */

typedef int opaque;


/*----------*/
get_xlbuf (length, buf, user_arg_g)
short int	*length;
PUB		*buf;
int		user_arg_g;
{
	static unsigned char	ibuf [IBUFSIZE];
	int		status;

    /* If lfnl_flag TRUE, send a 4-byte buffer to the caller that contains
    	<CSI>20h */
    if (lfnl_flag != 0)  {
    	*length = 4;
    	*buf = (PUB) lfnl_str;
    	lfnl_flag = 0;
    	return;
    	}

	status = read (user_arg_g, ibuf, IBUFSIZE);
	if (status <= 0) {
	    *length = 0;
	} else {
	    *length = status;
	}

	*buf = ibuf;
}

/*----------*/
put_xlbuf (length, buf, user_arg_p)
short int	*length;
PUB		*buf;
int		user_arg_p;
{
	static unsigned char	putbuf [OBUFSIZE];

	if (*length) {
	    write (user_arg_p, *buf, *length);
	}

	*buf = putbuf;
	*length = OBUFSIZE;
}

/*****************************************************************
 * initialise pointers for resource table entry
 *****************************************************************/

static void
init_entry(i,e_ptr)
int i;
struct entry_ptr_type *e_ptr;
{
	e_ptr-> curptr = &resource_table[i][0];           /* start of entry */
	e_ptr-> endptr = &resource_table[i][0] + ENTRYLEN;/* limit of entry */
}

/*****************************************************************
 * add string to entry preceded by length byte
 *****************************************************************/

static void 
add_string(e_ptr,str)
struct entry_ptr_type *e_ptr;
char * str;
{
	unsigned len = strlen(str);
	
	*e_ptr->curptr++ = len;
	(void) strcpy(e_ptr->curptr,str);
	e_ptr->curptr += len;
	if (e_ptr->curptr > e_ptr->endptr)    /* check for end of buffer */
		fatal("Resource table overflow - entry too long");
}

/*****************************************************************
 * parse resource strings and add entries to resource table
 *
 * resource string format: T<res type>N<res name>V<version>
 *
 * resource table format : <num of entries> (1 byte)
 *			   <entry 1>        (322 bytes)
 *			      .
 *			      .
 *			   <entry n>        (322 bytes)
 *
 * where <entry> is      : <length of res type>  (1 byte)
 *		 	   <res type>            (max 32 bytes)
 *                         <length of res name>  (1 byte)
 *		 	   <res name>            (max 255 bytes)
 *                         <length of version>   (1 byte)
 *		           <version>             (max 32 bytes)
 *****************************************************************/

static void
build_resource_table()
{
	int	i;
	char	*type, *name, *vers;
	struct entry_ptr_type entry_ptr;   /* pointers to current entry */
	char tmpbuf[ENTRYLEN];
	register BYTE	*res_buf;



#ifdef DEBUG
	int len,len2,len3;
	BYTE *ptr;
#endif
	res_buf = (BYTE *)malloc(ENTRYLEN * rescount + 1);

	trn$_resource_table_ptr = (PL)res_buf;

	*res_buf++ = (BYTE)rescount;             /* num of entries in table */

	resource_table = (BYTE	(*) [ENTRYLEN])res_buf;

	for(i=0; i< rescount; i++)  {

		(void) strcpy(tmpbuf,resource_ptr[i]);

		if (((type  = strtok(resource_ptr[i]," ")) == NULL) ||
		    ((name  = strtok(NULL," "))== NULL) ||
		    ((vers  = strtok(NULL," ")) == NULL) ||
			     (strtok(NULL," ") != NULL))
			fatal("invalid resource string:%s", tmpbuf);
		
		init_entry(i,&entry_ptr);

		add_string(&entry_ptr,type);
		add_string(&entry_ptr,name);
		add_string(&entry_ptr,vers);

# ifdef DEBUG
		fprintf(stderr,"num entries %u\n",*(char *)resource_table);
		fprintf(stderr,"type %u ",(len=resource_table[i][0]));
		fprintf(stderr,"%1.*s\n",len,&resource_table[i][1]);
		fprintf(stderr,"name %u ",(len2=resource_table[i][len+1]));
		fprintf(stderr,"%1.*s\n",len2,&resource_table[i][len+2]);
		fprintf(stderr,"vers %u ",(len3=resource_table[i][len+len2+2]));
		fprintf(stderr,"%1.*s\n",len3,&resource_table[i][len+len2+3]);
# endif

	}
}

/*****************************************************************
 * add item to item list
 *****************************************************************/

static void
add_item(code, address)
int code;
opaque address;
{
	static int cur_i = 0;

	itemlist[cur_i].code = code;
	itemlist[cur_i++].address = (int)address;
}

/*****************************************************************
 * Build the item list
 *****************************************************************/

static void
build_item_list()
{
	/* add default items */
	
	trn$k_ansi_special.reset_entry = reset_ent;
	trn$k_ansi_special.reset_exit = reset_ex;

	add_item(trn$_ansi_special, (opaque)&trn$k_ansi_special);
	add_item(trn$_page_orientation, (opaque)orientation);
	add_item(trn$_seven_bit, (opaque)output_mode);
	add_item(trn$_image_compression, (opaque)image_comp);
	add_item(trn$_page_height, (opaque)page_size->sz_height);
	add_item(trn$_page_width, (opaque)page_size->sz_width);

	if (noshowlastpage)  {
		add_item(trn$_page_fragment, (opaque)trn$k_page_fragment);
	}

	if (set_vm_size) {
		add_item(trn$_vmsize, (opaque)vm_size);
	}

	switch (image_comp)
	    {
		case 0: 
		    add_item(trn$_image_compression, trn$k_image_readhexstring);
		    break;
		case 1:
		    add_item(trn$_image_compression, trn$k_image_sixel_font);
		    break;
		case 2:
		    add_item(trn$_image_compression, trn$k_image_readsixelstring);
		    break;
	    }
	if (rescount)
		add_item(trn$_resource_table, (opaque)trn$_resource_table_ptr);

	add_item(trn$_end_of_list, (opaque)0);
}

#ifdef DEBUG
print_item_list()
{
	int i = 0;
	while (itemlist[i].code != trn$_end_of_list) {
		fprintf(stderr,"code %d address %d\n"
			,itemlist[i].code,itemlist[i].address);
		i++;
	}

}
#endif

/*****************************************************************
 * fatal error - print message on stderr and exit
 *****************************************************************/


static int select_output_mode(arg)
char *arg;
{
	if	((*arg == '8') && (*(arg+1) == 'f'))
	{
		return trn$k_eight_bit;
	}

	if	((*arg == '8') && (*(arg+1) == 'g'))
	{
		return trn$k_gl_gr_only;
	}

	if	((*arg == '7') && (*(arg+1) == 'f'))
	{
		return trn$k_seven_bit;
	}

	if	((*arg == '7') && (*(arg+1) == 'g'))
	{
		return trn$k_gl_only;
	}

}

static int parse_command_line(argc, argv)
int argc;
char **argv;
{
	static char *options = "F:O:v:eshR:xzm:";
	extern int optind;
	extern char *optarg;
	int opt;
	char	*dummy;

	program_name = argv[0];

	/* parse command line options */

	init_args();

	while ((opt = getopt(argc,argv,options)) != EOF) {
		switch (opt) {

		    case 'F':	/* paper size */
			if (check_arg(optarg,as_page_sizes,&dummy)  !=  0) {
				fatal("invalid argument %s",optarg);
			}
			if (!(page_size=page_size_lookup(dummy))) {
				fatal("not yet implemented %s",optarg);
			}
			break;
		    case 'O':	/* orientation */
			if (check_arg(optarg,as_orientations,&dummy)  !=  0)
			    fatal("invalid argument %s",optarg);
			else switch(optarg[0]) {

			    case 'l':
				orientation = trn$k_page_landscape;
				break;
			    case 'p':
				orientation = trn$k_page_portrait;
				break;
			}
			break;

		    case 'v':	/* vm size */
			if (strlen(optarg) > 1)
			    fatal("invalid argument %s",optarg);
			set_vm_size++;
			switch (optarg[0]) {

			    case '0':
				vm_size = ONE_MEGABYTE;
				break;
			    case '1':
				vm_size = 262143;
				break;
			    case '2':
				vm_size = 470000;
				break;
			    default:
				fatal("invalid argument %s",optarg);
			}
			break;

		    case 'e':	/* nl esc sequence */
			lfnl_flag = TRUE;
			break;

		    case 's':	/* suppress showpage */
			noshowlastpage++;
			break;

		    case 'h':	/* hexstrings for sixels */
			image_comp = trn$k_image_readhexstring;
			break;

		    case 'R':	/* resource string */
			resource_ptr[rescount++] = optarg;
			break;

		    case 'x':	/* don't reset on entry */
			trn$k_ansi_special.reset_entry = trn$k_noreset_entry;
			break;

		    case 'z':	/* don't reset on exit */
			trn$k_ansi_special.reset_exit = trn$k_noreset_exit;
			break;
			
		    case 'm':
			output_mode = select_output_mode(optarg);
			break;
		    default:
			exit(2);

		}
	}
}

/*****************************************************************
 * fatal error - print message on stderr and exit
 *****************************************************************/
/*VARARGS1*/
static void
fatal(msg, a1, a2, a3)
	char *msg;
{
	fprintf(stderr,"%s: ", program_name);
	fprintf(stderr,msg, a1, a2, a3);
	fputc('\n',stderr);
	exit(2);
}


/*----------*/

main (argc, argv)
int     argc;
char    *argv [];
{
    	/* Initialize and set default options. */

	orientation = trn$k_page_portrait;
	paper_size = trn$k_sheet_size_a;
	output_mode = trn$k_eight_bit;
    	set_vm_size = noshowlastpage = FALSE;
    	lfnl_flag = FALSE;
	sgr = 0;
	vm_size = ONE_MEGABYTE-PREAMBLE_SIZE;
    	no_prologue_with_job = FALSE;
    	output_prologue_only = trn$k_script_and_prologue;
    	reset_ent = trn$k_reset_entry;
    	reset_ex = trn$k_reset_exit;
    	image_comp = trn$k_image_readhexstring;


	parse_command_line(argc, argv);

	/************************************************************************/
	/* Translate input stream						*/
	/************************************************************************/



		/* Declare the input and output files. */

		fin = 0; /* stdin */
		fout = 1; /* stdout */

    		/* Build the item list */
	
		build_resource_table();

    		build_item_list();
#ifdef DEBUG
		print_item_list();
#endif

    		/* Do the actual translation */
		trn$ansi_ps (get_xlbuf, fin, put_xlbuf, fout, itemlist);
}
