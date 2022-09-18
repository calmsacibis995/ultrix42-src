#ifndef lint
static char sccsid[]  =  "@(#)uerfdbc.c	4.2   (ULTRIX)   10/16/90";
#endif lint

#include <stdio.h>
#include "generic_dsd.h"

struct std_item {
	struct std_item *next;
	char *name;
	int   id;
	int   type;
	int   class;
	int   size;
	char *label;
	int   label_type;
	struct std_index *map_index;	/* pointer to index list */
	int   map_index_count;
	struct std_reg_field *map_reg;	/* pointer to register_field list */
	int   map_reg_count;
	char *doc;	/* comments */
};

struct std_index {
	struct std_index *next;
	unsigned long value;	/* (position in list, for old style assignment) */
	char *name;
	char *label;
	char *label_plain;		/* this one doesn't have convert_toupper done */
};

struct std_reg_field {
	struct std_reg_field *next;
	int   class;
	int   size;
	char *label;
	int   label_type;
	struct std_reg_field_index *map_std_reg_field_index;
	int   map_std_reg_field_index_count;
	char *doc;
};

struct std_reg_field_index {
	struct std_reg_field_index *next;
	unsigned long value;
	char *label;
};

struct std_segment {
	struct std_segment *next;
	int type;	/* ID field 1 (e.g., CDS, ADS,...) */
	int type_id;	/* ID field 2 (e.g., 65, 67, 15, ...) */
	char *name;
	char *label;
	char *doc;
	struct std_segment_element *elements;
	int elements_count;
};

struct std_segment_element {
	struct std_segment_element *next;
	char *name;
	int id;	/* id of the std_item with name. */
};

struct os_item {
	struct os_item *next;
	int   id;
	char *name;
	int type;
	int class;
	int size;
	char *doc;
	struct os_index *map;
	int map_count;
};

struct os_index {
	struct os_index *next;
	unsigned long os_value;
	unsigned long std_value;
	char * std_name;
};

	/* for translating strings into values */
extern struct dx_struct {
	int value;
	char *string;
} df_table[], dt_table[], dc_table[], seg_table[];

char *ep_malloc(), *malloc();
char *strpbrk();
char *readline();
unsigned long find_id_of_index();
extern char *std_h_fixed1[], *std_h_fixed2[], *std_h_fixed3[];
extern char *os_h_fixed1[], *os_h_fixed2[];
char *encode_type();

	/* current pointers */
struct std_item *sitem, *last_sitem, *first_sitem;
struct std_reg_field *sreg, *last_sreg, *first_sreg;
struct std_segment *sseg, *last_sseg, *first_sseg;
struct os_item *oitem, *last_oitem, *first_oitem;
struct std_segment_element *sse, *last_sse, *first_sse;

FILE *std_h, *os_h, *bin_out;
int lineno = 0; /* current line number of input */
int cnt_std_items, cnt_std_segs, cnt_os_items;

	/* globals set by readline() */
#define LINEBUF_SIZ 800
char linebuf[LINEBUF_SIZ+2];
#define WORD1_SIZ 50
char word1[WORD1_SIZ+2];
char *ls;	/* line start - beginning of line after skipping whitespace */
int indented;	/* indented line ? 1 or 0 */
int reread;	/* just read previous line again */

char *cp, *cp2;

int opt_d = 0;	/* debug option */
int opt_i = 0;	/* use old method of index value assignment (by position) */
int opt_j = 0;	/* issue warning if input index value differs from position */
int opt_auto_assign = 0;/* auto assignment of id numbers */

#define BIN_NAME "ultrix_dsd.bin"
#define STD_H_NAME "std_dsd.h"
#define OS_H_NAME "os_dsd.h"

char *cmd_name;	/* command name used to invoke this program */
char **db_files = 0; /* list of database input files */
char *in_name = 0;/* current input database file name */
FILE *db_fp = 0;	 /* FILE * for current input database file */

	/* for automatic assignment of id numbers */
int next_sitem_id = 1;
#define MAX_SEG_INDEX 7
int next_sseg_type_id[MAX_SEG_INDEX+2]; /* +1 for zero element, +1 safety */
int next_oitem_id = 1;

int uerf_version = 0;	/* version to keep uerf program in sync with database */
int errval = 0; /* error code return value */

main(argc, argv, arge)
int argc;
char **argv, arge;
{
	int same_block = 0; /* for unrecognized line, we only output the
						first unrecognized line in a block of lines */

	init_vars();	/* initialize some vars */
	process_args(argc, argv, arge);
	while(readline()) {
		if(!strlen(ls)) {
			same_block = 0;
			continue;
		}
		if( !strcasecmp(word1, "sitem") ) {
			same_block = 0;
			read_std_item();
		} else if( !strcasecmp(word1, "sseg") ) {
			same_block = 0;
			read_std_segment();
		} else if( !strcasecmp(word1, "oitem") ) {
			same_block = 0;
			read_os_item();
		} else if( !strcasecmp(word1, "version") ) {
			same_block = 0;
			read_version();
		} else {
			if( same_block )
				continue;
			print_where();
			fprintf(stderr, "  Error, Unrecognized keyword or not in an item or segment\n");
			errval = 1;
			same_block = 1;
		}
	}
	check_data();
	output_data();	/* output the data file */
	output_h();		/* output the include files */
	/* output_all(); */
	exit(errval);
}

init_vars()
{
	int i;

	sitem = last_sitem = first_sitem = 0;
	sreg = last_sreg = first_sreg = 0;
	sseg = last_sseg = first_sseg = 0;
	oitem = last_oitem = first_oitem = 0;
	sse = last_sse = first_sse = 0;
	for(i=1; i<= MAX_SEG_INDEX; i++)
		next_sseg_type_id[i] = 1;
		
}

		/* read next line from input file(s) */

		/* INPUT parameter: 
		   if reread is set, re-read the previous line.

		   OUTPUT values:
		   ls points to first non-null char in line.
		   word1 array contains a copy of the first word from line.
		   indented is true if line begins with whitespace.
		   returns pointer to word1 array.
		 */
char *
readline()
{
	if( reread ) {
		reread = 0;
		return(word1);
	}
reget:		/* we 'goto' here only when had comment line, to get a new line */
	strcpy(word1, "");
	ls = 0;
	indented = 0;

		/* get a line into linebuf */
	while( !db_fp || NULL == fgets(linebuf, LINEBUF_SIZ, db_fp) ) {
			/* need to get open next file */
		if( !*db_files )
			return(0);	/* there is no next file */
		in_name = *db_files++;
		lineno = 0;
		if( NULL == (db_fp=fopen(in_name, "r")) ) {
			fprintf(stderr, "Error, Unable to open file %s\n", in_name);
			errval = 1;
		}
	}
	lineno++;
	if(opt_d)
		fprintf(stderr, "File %s, line %d: %s", in_name, lineno, linebuf);
		/* remove trailing newline and whitespace */
	for( cp= &linebuf[strlen(linebuf)-1];
			*cp && cp >= linebuf && 
				(*cp == '\n' || *cp == ' ' || *cp == '\t');
		 cp--)
		*cp = 0;
	if( linebuf[strlen(linebuf)] == '\n' )
	    linebuf[strlen(linebuf)] = '\0';
			/* skip white space */
	for(ls=linebuf; *ls && (*ls == ' ' || *ls == '\t'); ls++ )
		indented=1;
	if( *ls == '#' )	/* comment line ? */
		goto reget;
			/* pick up first word into 'word1' */
	for(	cp=word1, cp2=ls;
			*cp2 && cp-word1 < WORD1_SIZ && *cp2 != ' ' && *cp2 != '\t';
			cp2++)
		*cp++ = *cp2;
	*cp = '\0';
	return(word1);
}

		/* read in the version number of the db */
read_version(){
	if(sscanf(ls, "version %d", &uerf_version) != 1 ) {
		print_where();
		fprintf(stderr, "Error, bad version number\n");
		errval = 1;
	}
}

read_std_item()
{
		/* allocate a std_item */
	sitem = (struct std_item *)ep_malloc(sizeof(struct std_item));
	cnt_std_items++;
	if( last_sitem )
		last_sitem->next = sitem;
	last_sitem = sitem;
	if( !first_sitem )
		first_sitem = sitem;
		/* register fields are local to a std_item */
		/* so dont carry over from previous std_items */
	sreg = last_sreg = first_sreg = 0;
		/* clear fields here */
	sitem->next = 0;
	sitem->name = "";
	if( opt_auto_assign )
		sitem->id = next_sitem_id++;
	else
		sitem->id = -1;
	sitem->type = -1;
	sitem->class = -1;
	sitem->size = -1;
	sitem->label = "";
	sitem->label_type = -1;
	sitem->map_index = 0;
	sitem->map_index_count = 0;
	sitem->map_reg = 0;
	sitem->map_reg_count = 0;
	sitem->doc = 0;

	while(readline()) {
		if(!strlen(ls)) { /* null line ends this std item */
			return;
		} else if( !strcasecmp(word1, "POINTER") ) {
		} else if( !strcasecmp(word1, "RANGE") ) {
		} else if( !strcasecmp(word1, "LIST") ) {
		} else if( !strcasecmp(word1, "DEFAULT") ) {
		} else if( !strcasecmp(word1, "ID") ) {
			if( !opt_auto_assign ) /* ignore ID if prog assigns id's */
				if(sscanf(ls, "%*[Ii]%*[Dd] %d", &sitem->id) != 1 ) 
					err("sitem", "ID");
		} else if( !strcasecmp(word1, "NAME") ) {
			sitem->name = ep_malloc(strlen(ls));
			if(sscanf(ls, "%*[Nn]%*[Aa]%*[Mm]%*[Ee] %s", sitem->name) != 1 )
				err("sitem", "NAME");
		} else if( !strcasecmp(word1, "TYPE") ) {
			sitem->type = decode_type(ls+strlen("TYPE"), dt_table);
		} else if( !strcasecmp(word1, "CLASS") ) {
			sitem->class = decode_type(ls+strlen("CLASS"), dc_table);
		} else if( !strcasecmp(word1, "SIZE") ) {
			if(sscanf(ls, "%*[Ss]%*[Ii]%*[Zz]%*[Ee] %d", &sitem->size) != 1 )
				err("sitem", "SIZE");
		} else if( !strcasecmp(word1, "LABEL") ) {
			sitem->label = ep_malloc(strlen(ls));
			if(!copy_quote_str(sitem->label, ls+strlen("LABEL"), 0))
				err("sitem", "LABEL");
		} else if( !strcasecmp(word1, "DISPLAY") ) {
			sitem->label_type = decode_type(ls+strlen("DISPLAY"), df_table);
		} else if( !strcasecmp(word1, "doc") ) {
			sitem->doc = ep_malloc(strlen(ls));
			if(!copy_quote_str(sitem->doc, ls+strlen("doc"), 0))
				err("sitem", "doc");
		} else if( !strcasecmp(word1, "MAP") ) {
			if( sitem->type == DT_SHORT_INDEX || sitem->type == DT_INDEXED ){
				while(readline()) {
					if(!strlen(ls))
						return; /* null line ends this std item */
#ifdef OLD_
					if(word1[0] == ':')
						{ reread=1; break; }
#endif
					{
					char *cp;
					struct std_index *cp_m, *si;
					int tcount;

						/* allocate and link in std_index struct */
					cp_m = (struct std_index *)
								ep_malloc(sizeof(struct std_index));
					if(!sitem->map_index) {
						sitem->map_index = cp_m;
					} else {
						tcount = 0;
							/* get to last one on list */
						for(si=sitem->map_index; si && si->next; si=si->next)
							tcount++;
							/* the following check is unnecessary.
								It was intended as an extra safety
								against programming errors. */
						if( !si || tcount != sitem->map_index_count -1 ) {
						  print_where();
						  errval = 1;
				   		  fprintf(stderr, "  Error, internal map_index loop failure ");
				   		  fprintf(stderr,
									"  tcount=%d map_index_count=%d\n",
				 					tcount, sitem->map_index_count);
				   		  break;
						}
						si->next = cp_m;
					}
					sitem->map_index_count++;
					cp_m->next = 0;
						/* if option 'i', then we use the old method,
							which is to determine the value by the position
							of the entry in the list (first is one, second
							is two, etc.).  Also, we allow, but do not require
							the presence of nor use, an assignment number
							in the line.
						   if NOT option 'i', then we get the number from
							the line itself.  Further, if option 'j',
							we issue a warning if the number in the line
							is not the same as the number that we would
							have assigned using the old method (value by 
							position).  
						 */

						/* pick up and interpret the number, if found. */
						/*   Bump ls past this, and over to the next field */
					cp_m->value = 0;
					sscanf(ls, "%d", &cp_m->value);
					while( (*ls >= '0' && *ls <= '9') || *ls == ' ' ||
							*ls == '\t')
								ls++;

					if( opt_i )
						cp_m->value = (long)sitem->map_index_count;
					else {
						if(!cp_m->value) {
							print_where();
							errval = 1;
							fprintf(stderr, "  Error, missing or bad index value\n");
						}
						if( opt_j && cp_m->value != sitem->map_index_count ){
							print_where();
							fprintf(stderr,
"  warning: index value(%d) inconsistent with input line position(%d)\n",
								cp_m->value, sitem->map_index_count);
						}
					}
					cp_m->name = ep_malloc(strlen(ls));
					if (sscanf(ls, "%s", cp_m->name) != 1)
							{ err("sitem", ls); break;}
					
					cp_m->label = ep_malloc(strlen(ls));
					if( !copy_quote_str(cp_m->label, ls+strlen(cp_m->name), 0) )
						err("sitem", ls);
					cp_m->label_plain = ep_malloc(strlen(ls));
					if( !copy_quote_str(cp_m->label_plain,
										ls+strlen(cp_m->name), 0) )
						err("sitem", ls);
					}
				}
			}
		} else if( !strcasecmp(word1, "field") ) {
reg_field:
						/* start register field */
			sreg = (struct std_reg_field *)
						ep_malloc(sizeof(struct std_reg_field));
			if( last_sreg )
				last_sreg->next = sreg;
			last_sreg = sreg;
			if( !first_sreg )
				first_sreg = sreg;
						/* link into the current standard item */
			if( !sitem ) {
				print_where();
				fprintf(stderr,"Error, no current std item\n");
				errval = 1;
				break;
			}
			if( !sitem->map_reg ) {
				sitem->map_reg = sreg;
				sitem->map_reg_count = 0;
			}
			sitem->map_reg_count++;
	
			sreg->next = 0;			 /* clear fields */
			sreg->class = -1;
			sreg->size = -1;
			sreg->label = "";
			sreg->label_type = -1;
			sreg->map_std_reg_field_index = 0;
			sreg->map_std_reg_field_index_count = 0;
			sreg->doc = 0;

			while(readline()) {
				if(!strlen(ls))
					return;
reg_field_b:
#ifdef REQUIRE_INDENTED
				if( !indented )
					{ reread=1; break; }
#endif
				if( !strcasecmp(word1, "CLASS") ) {
					sreg->class = decode_type(ls+strlen("CLASS"),dc_table);
				} else if( !strcasecmp(word1, "SIZE") ) {
					if(sscanf(ls, "%*[Ss]%*[Ii]%*[Zz]%*[Ee] %d", &sreg->size) != 1 ) 
						err("sitem", "SIZE");
				} else if( !strcasecmp(word1, "LABEL") ) {
					sreg->label = ep_malloc(strlen(ls));
					strcpy(sreg->label, "");
					if(!copy_quote_str(sreg->label, ls+strlen("LABEL"), 0))
						err("sitem", "LABEL");
				} else if( !strcasecmp(word1, "DISPLAY") ) {
						sreg->label_type =
							decode_type(ls+strlen("DISPLAY"), df_table);
				} else if( !strcasecmp(word1, "MAP") ) {
					while(readline()) {
						if(!strlen(ls))
							return;
						if( !strcasecmp(word1, "field") )
							{ reread =1 ; break; }
#ifdef REQUIRE_INDENTED
						if(!indented) /* register map fields must be indented */
							{ reread =1 ; goto reg_field_b; }
#endif
#ifdef OLD_
						if( word1[0] == ':' )
							{ reread =1 ; break; }
#endif
						{
						char *cp;
						struct std_reg_field_index *cp_m, *sr;
						int tcount;

						cp_m = (struct std_reg_field_index *)
								ep_malloc(sizeof(struct std_reg_field_index));
						if(!sreg->map_std_reg_field_index) {
							sreg->map_std_reg_field_index = cp_m;
						} else {
							tcount = 0;
								/* find last on list */
							for(sr=sreg->map_std_reg_field_index;
								sr && sr->next; sr=sr->next)
									tcount++;
				/* the following check is unnecessary.
					It was intended as an extra safety
					against programming errors. */
if( !sr || tcount != sreg->map_std_reg_field_index_count -1 ) {
	print_where();
	errval = 1;
	fprintf(stderr,"  Error, internal map_std_reg_field_index loop failure \n");
	fprintf(stderr,"  tcount=%d map_std_reg_field_index_count=%d\n",
			tcount, sreg->map_std_reg_field_index_count);
 	break;
}
							sr->next = cp_m;
						}
						sreg->map_std_reg_field_index_count++;
						cp_m->next = 0;
									/* pick up value */
						if (sscanf(ls, "%lu", &cp_m->value) != 1)
								{ err("sitem", ls); break;}
						
						cp_m->label = ep_malloc(strlen(ls));
						cp = ls + strspn(ls, "0123456789");
									/* pick up label */
						if( !copy_quote_str(cp_m->label, cp, 0) )
							err("sitem", ls);
						}
					}
				} else if( !strcasecmp(word1, "doc") ) {
					sreg->doc = ep_malloc(strlen(ls));
					if(!copy_quote_str(sreg->doc,ls+strlen("doc"),0))
						err("sitem", "doc");
				} else if( !strcasecmp(word1, "field") ) {
					{ goto reg_field; }
				} else {
					print_where();
					errval = 1;
					fprintf(stderr, "Error, unrecognized reg field line\n");
				}
			}
		} else {
			reread=1; return;
		}
	}
}

read_std_segment()
{
	int state_reading_elements = 0;	/* true while reading elements */
	sseg = (struct std_segment *)ep_malloc(sizeof(struct std_segment));
	cnt_std_segs++;
	if( last_sseg )
		last_sseg->next = sseg;
	last_sseg = sseg;
	if( !first_sseg )
		first_sseg = sseg;
		/* clear fields here */
	sseg->next = 0;
	sseg->type = -1;
	sseg->type_id = -1;
	sseg->name = "";
	sseg->label = "";
	sseg->doc = "";
	sseg->elements = 0;
	sseg->elements_count = 0;
	state_reading_elements = 0;
	while(readline()) {
		char tbuf[12];
		if(!strlen(ls))
			return;
		if( !indented && strcasecmp(word1, "ELEMENTS") )
			state_reading_elements = 0;	/* no longer reading elements */
		if( !strcasecmp(word1, "ID") ) {
			if( opt_auto_assign ) {
				if(sscanf(ls, "%*[Ii]%*[Dd] %10s", tbuf) != 1 ) 
					err("sseg", "ID");
			} else {
				if(sscanf(ls, "%*[Ii]%*[Dd] %10s %d", tbuf, &sseg->type_id) != 2 ) 
					err("sseg", "ID");
			}
			sseg->type = decode_type(tbuf, seg_table);
			if( sseg->type > MAX_SEG_INDEX ) {
				fprintf(stderr, "Error, sseg->type (%d) too large\n", sseg->type);
				errval = 1;
			} else if( opt_auto_assign ) {
				sseg->type_id = next_sseg_type_id[sseg->type]++;
			}
		} else if( !strcasecmp(word1, "NAME") ) {
			sseg->name = ep_malloc(strlen(ls));
			if(sscanf(ls, "%*[Nn]%*[Aa]%*[Mm]%*[Ee] %s", sseg->name) != 1 )
				err("sseg", "NAME");
		} else if( !strcasecmp(word1, "LABEL") ) {
			sseg->label = ep_malloc(strlen(ls));
			if(!copy_quote_str(sseg->label, ls+strlen("LABEL"), 0))
				err("sseg", "LABEL");
		} else if( !strcasecmp(word1, "ELEMENTS") ) {
			state_reading_elements = 1;
		} else if( !strcasecmp(word1, "doc") ) {
			sseg->doc = ep_malloc(strlen(ls));
			if(!copy_quote_str(sseg->doc, ls+strlen("doc"), 1))
				err("sseg", "doc");
		} else {
			if( state_reading_elements ) {
					/* pick up each word from the line */
				for(cp=ls; *cp; ) {
					struct std_segment_element *se;
					int len;
					sse = (struct std_segment_element *)
							ep_malloc(sizeof(struct std_segment_element ));
					sseg->elements_count++;
					if( !sseg->elements )
						sseg->elements = sse;
					else {
						/* find last element */
						for(se=sseg->elements; se && se->next; se=se->next)
							;
						se->next = sse;
					}
					sse->next = 0;
					len = strcspn(cp," \t");	/* word ends at whitespace */
					sse->name = ep_malloc(len+1);
					strncpy(sse->name, cp, len);
					sse->name[len] = '\0';
					cp+=len;
					while( *cp && (*cp == ' ' || *cp == '\t' ))
						cp++;
				}
			} else {
				reread=1; return;
			}
		}
	}
}

read_os_item()
{
	oitem = (struct os_item *)ep_malloc(sizeof(struct os_item));
	cnt_os_items++;
	if( last_oitem )
		last_oitem->next = oitem;
	last_oitem = oitem;
	if( !first_oitem )
		first_oitem = oitem;
		/* clear fields here */
	oitem->next = 0;
	if( opt_auto_assign )
		oitem->id = next_oitem_id++;
	else
		oitem->id = -1;
	oitem->name = "";
	oitem->type = -1;
	oitem->class = -1;
	oitem->size = -1;
	oitem->doc = 0;
	oitem->map = 0;
	oitem->map_count = 0;
	while(readline()) {
		if(!strlen(ls)) {
			return;
		} else if( !strcasecmp(word1, "POINTER") ) {
		} else if( !strcasecmp(word1, "RANGE") ) {
		} else if( !strcasecmp(word1, "LIST") ) {
		} else if( !strcasecmp(word1, "DEFAULT") ) {
		} else if( !strcasecmp(word1, "ID") ) {
			if( !opt_auto_assign )
				if(sscanf(ls, "%*[Ii]%*[Dd] %d", &oitem->id) != 1 ) 
					err("sitem", "ID");
		} else if( !strcasecmp(word1, "NAME") ) {
			oitem->name = ep_malloc(strlen(ls));
			if(sscanf(ls, "%*[Nn]%*[Aa]%*[Mm]%*[Ee] %s", oitem->name) != 1 )
				err("sitem", "NAME");
		} else if( !strcasecmp(word1, "TYPE") ) {
			oitem->type = decode_type(ls+strlen("TYPE"), dt_table);
		} else if( !strcasecmp(word1, "CLASS") ) {
			oitem->class = decode_type(ls+strlen("CLASS"), dc_table);
		} else if( !strcasecmp(word1, "SIZE") ) {
			if(sscanf(ls, "%*[Ss]%*[Ii]%*[Zz]%*[Ee] %d", &oitem->size) != 1 )
				err("sitem", "SIZE");
		} else if( !strcasecmp(word1, "doc") ) {
			oitem->doc = ep_malloc(strlen(ls));
			if(!copy_quote_str(oitem->doc, ls+strlen("doc"), 0))
				err("sitem", "doc");
		} else if( !strcasecmp(word1, "MAP") ) {
			if( oitem->type == DT_SHORT_INDEX || oitem->type == DT_INDEXED ||
				oitem->type == DT_TINY_INDEX ){
				while(readline()) {
					if(!strlen(ls))
						return;
#ifdef OLD_
					if(word1[0] == ':')
						{ reread=1; break; }
#endif
					{
					char *cp;
					struct os_index *cp_m, *oi;
					int tcount;

					cp_m = (struct os_index *)
								ep_malloc(sizeof(struct os_index));
					if(!oitem->map) {
						oitem->map = cp_m;
					} else {
						for(oi=oitem->map; oi && oi->next; oi=oi->next)
							;
						if( !oi ) {
							print_where();
			   				fprintf(stderr, "  Error, internal map loop failure ");
							errval = 1;
			   				break;
						}
						oi->next = cp_m;
					}
					oitem->map_count++;
					cp_m->next = 0;
					cp_m->std_value = 0;
					cp_m->std_name = ep_malloc(strlen(ls));
					if(sscanf(ls, "%lu %s", &cp_m->os_value,cp_m->std_name)!=2)
							{ err("oitem", ls); break;}
					}
				}
			}
		} else {
			reread=1; return;
		}
	}
}

err(state, line)
char *state, *line;
{
	if( !strcasecmp(state, "sitem") ) {
		print_where();
		errval = 1;
		fprintf(stderr, "  Error, error in std-item %s(%d)\n",
			sitem->name, sitem->id);
	} else if( !strcasecmp(state, "sseg") ) {
		print_where();
		errval = 1;
		fprintf(stderr, "  Error, error in std-segment %s(%d-%d) \n",
			sseg->name, sseg->type, sseg->type_id);
	} else if( !strcasecmp(state, "oitem") ) {
		print_where();
		errval = 1;
		fprintf(stderr, "  Error, error in os-item %s(%d)\n",
			oitem->name, oitem->id);
	}
}


struct dx_struct dt_table[] = {
  { DT_SHORT, "SHORT" },
  { DT_LONG, "LONG" },
  { DT_STRING, "STRING" },
  { DT_SHORT_INDEX, "SHORT_INDEX" },
  { DT_INDEXED, "INDEXED" },
  { DT_SHORT_REGISTER, "SHORT_REGISTER" },
  { DT_REGISTER, "REGISTER" },
  { DT_DATE, "DATE" },
  { DT_BYTE_VECTOR, "BYTE_VECTOR" },
  { DT_COUNTED_SHORT_VECTOR, "COUNTED_SHORT_VECTOR" },
  { DT_COUNTED_LONG_VECTOR, "COUNTED_LONG_VECTOR" },
  { DT_ADDR_CNT_VECTOR, "ADDR_CNT_VECTOR" },
  { DT_TINY, "TINY" },
  { DT_TINY_INDEX, "TINY_INDEX" },
  { DT_ASCIZ, "ASCIZ" },
  { DT_BIT_VECTOR, "BIT_VECTOR" },
  { DT_SHORT_VECTOR, "SHORT_VECTOR" },
  { DT_LONG_VECTOR, "LONG_VECTOR"},
  { DT_VMS_TIME, "VMS_TIME"},
  { 0, ""}
};

struct dx_struct dc_table[] = {
  { DC_INTEGER, "INTEGER" },
  { DC_FLOAT, "FLOAT" },
  { DC_CODED, "CODED" },
  { DC_CHARACTER, "CHARACTER" },
  { DC_TIME, "TIME" },
  { DC_COMPOSITE, "COMPOSITE" },
  { DC_FILLER, "FILLER" },
  { DC_BYTES, "BYTES" },
  { DC_BITS, "BITS" },
  { 0, ""}
};

struct dx_struct df_table[] = {
  { DF_DEFAULT, "NIL"},
  { DF_DECIMAL, "DECIMAL" },
  { DF_HEX, "HEX" },
  { DF_OCTAL, "OCTAL" },
  { DF_DATE_TIME, "DATE_TIME" },
  { DF_E_TIME, "E_TIME" },
  { DF_HEX_DUMP, "HEX_DUMP" },
  { 0, ""}
};

struct dx_struct seg_table[] = {
	{ 1, "EIS"},          /* Event Identification Segment */
	{ 2, "DIS"},          /* Device Information Segment */
	{ 3, "SDS"},          /* Supporting Data Segment */
	{ 4, "CDS"},          /* Correlating Data Segment */
	{ 5, "ADS"},          /* Additional Data Segment */
	{ 6, "SIS"},          /* Summary Information Segment */
	{ 7, "CIS"},          /* Configuration Information Segment */
		/* note: if extending this table, MAX_SEG_NUM must be adjusted */
  { 0, ""}
};


	/* lookup string in table to get value */
decode_type(s, table)
char *s;
struct dx_struct *table;
{
	int len;
	struct dx_struct *dt;

	while(*s && (*s == ' ' || *s == '\t'))
		s++;
	for( dt=table; len=strlen(dt->string); dt++ ) {
		if( !strncasecmp(s, dt->string, len) &&
			(s[len] == '\0' || s[len] == ' ' ||
			 s[len] == '\t' || s[len] == '\n')) {
				return(dt->value);
			}
	}
	print_where();
	errval = 1;
	fprintf(stderr, "  Error, unable to decode %s\n", s);
	return(-1);
}

	/* lookup value in table to get string */
char *
encode_type(a, table)
int a;
struct dx_struct *table;
{
	int len;
	struct dx_struct *dt;

	for( dt=table; len=strlen(dt->string); dt++ ) {
		if( a == dt->value )
			return(dt->string);
	}
	/* fprintf(stderr, "  Error, unable to encode %d\n", a); */
	return("<UNKNOWN>");
}

	/* copy the quoted string from s to d , without the quotes */
	/* Allow case of only whitespace present. */
	/* Return true if ok, false if error */
copy_quote_str(d, s, nil_flg)
char *d, *s;
int nil_flg;
{
	char quote_char;
	*d = '\0';
	while(*s && (*s == ' ' || *s == '\t'))
		s++;
	if( !*s )
		return(1);
	if( *s != '"' && *s != '\'') {
				/* special case.  NILL is found once in the 
					vmslisp-generated file and we want to match
					that file, so we need this line here. */
		if( !strncasecmp(s, "NILL", 4) ) {
			strcpy(d, "NILL");
			return(1);
		}
		if( !strncasecmp(s, "NIL", 3) ) {
			if( nil_flg )
				strcpy(d, "NIL");
			return(1);
		}
		return(0);
	}
	quote_char = *s;
	s++;
	while(*s && *s != quote_char && *s != '\n') {
		if( *s == '\\' )
			s++;
		*d++ = *s++;
	}
	if( *s && *s == quote_char)
		return(1);
	else
		return(0);
}

	/* this function dumps the database */
	/* It was used when developing this prog to check that
		the database was parsed correctly, prior to writing
		the code to output the .h and bin files. */
	/* It may no longer serve a useful purpose. */
output_all()
{
	int tcount =0;
	struct std_index *si;
	struct std_reg_field *sr;
	struct std_reg_field_index *sri;
	struct std_segment_element *sse;
	struct os_index *oi;

	printf("#std_items=%d #std_segs=%d #os_items=%d\n",
			cnt_std_items, cnt_std_segs, cnt_os_items);

	for(sitem = first_sitem; sitem; sitem = sitem->next) {
		tcount++;
		printf("std item %s (%d) type %d\n",
			sitem->name, sitem->id, sitem->type);
		printf("  class %d, size %d\n", sitem->class, sitem->size);
		printf("  label_type %d, label=%s\n", sitem->label_type, sitem->label);
		printf("  documentation=%s\n", sitem->doc);
		if (sitem->type == DT_SHORT_INDEX || sitem->type == DT_INDEXED) {
			for(si=sitem->map_index; si ; si=si->next) {
				printf("      %lu  %s  %s\n",
					si->value, si->name, si->label);
			}
		}
		if (sitem->type == DT_SHORT_REGISTER || sitem->type == DT_REGISTER) {
			for(sr=sitem->map_reg; sr; sr=sr->next) {
				printf("      %s, size %d, class %d, l_type %d\n      %s\n",
					sr->label? sr->label: "",
					sr->size, sr->class, sr->label_type,
					sr->doc? sr->doc: "");
				for(sri=sr->map_std_reg_field_index; sri; sri=sri->next)
					printf("         %lu \"%s\"\n", sri->value, sri->label);
			}
		}
	}
	printf("count = %d    confirm count = %d\n", cnt_std_items, tcount);
	for(sseg = first_sseg; sseg; sseg = sseg->next) {
		printf("std segment %s (%d %d) %s  count %d\n",
			sseg->name, sseg->type, sseg->type_id,
			sseg->label, sseg->elements_count);
		printf("    doc=%s\n", sseg->doc);
		for(sse=sseg->elements; sse; sse=sse->next) {
			printf("         %d %s\n", sse->id, sse->name);
		}
	}
	for(oitem = first_oitem; oitem; oitem = oitem->next) {
		printf("os item %s (%d)  type %d, class %d, size %d cnt %d\n",
			oitem->name, oitem->id, oitem->type, oitem->class, oitem->size,
			oitem->map_count);
		printf("    doc=%s\n", oitem->doc);
		for( oi = oitem->map; oi; oi=oi->next)
			printf("    %lu  %lu  %s\n",
				oi->os_value, oi->std_value, oi->std_name);
	}
}

char *ep_malloc(size)
int size;
{
	char *m;
	if(opt_d)
		fprintf(stderr, "ep_malloc %d\n", size);
	m=malloc(size);
	if(!m) {
		print_where();
		errval = 1;
		fprintf(stderr,"  Error, ep_malloc failed\n");
		exit(3);
	}
	return(m);
}


check_data() {
	check_std_items();
	check_std_segs();
	check_os_items();
}

	/* output the data file (bin file) */
output_data() {
	bin_out = fopen(BIN_NAME, "w");
	if( bin_out == NULL )  {
		fprintf(stderr, "Error, unable to open %s\n", BIN_NAME);
		errval = 1;
		exit(4);
	}
	fprintf(bin_out, "%d\n/*| Start of ultrix_dsd.bin */\n\n3\n\n", uerf_version);
	out_std_items();
	fprintf(bin_out, "\n");
	out_std_segs();
	fprintf(bin_out, "\n");
	out_os_items();
	fprintf(bin_out, "/* End of ultrix_dsd.bin */\n");
}

invalid_sitem(s, str)
struct std_item *s;
char *str;
{
	errval = 1;
	fprintf(stderr, "Error, In std item '%s' (id %d), invalid or missing %s\n",
		s->name, s->id, str);
}

invalid_sseg(s, str)
struct std_segment *s;
char *str;
{
	errval = 1;
	fprintf(stderr, "Error, In std segment '%s' (id %s %d), invalid or missing %s\n",
		s->name, encode_type(s->type,seg_table), s->type_id, str);
}

invalid_oitem(s, str)
struct os_item *s;
char *str;
{
	errval = 1;
	fprintf(stderr, "Error, In os item '%s' (id %d), invalid or missing %s\n",
		s->name, s->id, str);
}

	/* data checks on std_items,
		and any necessary references to other structures */
	/* Also, adjust things such as upper/lower case. */
check_std_items(){
	struct std_index *si;
	struct std_reg_field *sr;
	struct std_reg_field_index *sri;
	int need_nl, i;

	for(sitem = first_sitem; sitem; sitem = sitem->next) {
		if( !sitem->name || !strlen(sitem->name) ) {
			invalid_sitem(sitem, "NAME");
			sitem->name = " ";
		}
		convert_tolower(sitem->name);
		if( sitem->id <= 0)
			invalid_sitem(sitem, "ID");
		if( sitem->type <= 0)
			invalid_sitem(sitem, "TYPE");
		if( sitem->class <= 0)
			invalid_sitem(sitem, "CLASS");
		if( sitem->size <= 0)
			invalid_sitem(sitem, "SIZE");
				/* it's okay to omit display when type is indexed */
		if( sitem->label_type <= 0) {
			if( sitem->type != DT_SHORT_INDEX &&
				sitem->type != DT_INDEXED &&
				sitem->type != DT_TINY_INDEX ) {
					invalid_sitem(sitem, "DISPLAY");
			} else {
				sitem->label_type = DF_DEFAULT;
			}
		}
		if( !sitem->label || !strlen(sitem->label) ) {
			/* it's okay to omit the LABEL */
				/* this has been concluded because labels were found
					to be omitted from sti_rdch_stat2(old id 333),
					rv_slave2(old id 449), cierrcode(old id 517) */
			/* invalid_sitem(sitem, "LABEL"); */
			sitem->label = " ";
		}
		convert_toupper(sitem->label);
		if( !sitem->doc || !strlen(sitem->doc) ) {
			sitem->doc = "nil";
		}
		switch(sitem->type){
			case DT_SHORT_INDEX:
			case DT_INDEXED:
				for(si=sitem->map_index; si; si=si->next) {
					if( !si->name || !strlen(si->name) ) {
						invalid_sitem(sitem, "index name");
						si->name = " ";
					}
					if( si->value <=0 ) {
						invalid_sitem(sitem, "index value");
					}
					convert_tolower(si->name);
					if( !si->label || !strlen(si->label) ) {
						invalid_sitem(sitem, "index label");
						si->label = " ";
					}
					convert_toupper(si->label);
						/* label_plain is directly derived from label */
					if( !si->label_plain || !strlen(si->label_plain) )
						si->label_plain = " ";
				}
				break;


			case DT_SHORT_REGISTER:
			case DT_REGISTER:
				for(sr=sitem->map_reg; sr; sr=sr->next) {
					if( !sr->label || !strlen(sr->label) ) {
						/* it is okay to omit a register label */
						/* invalid_sitem(sitem, "register LABEL"); */
						sr->label = " ";
					}
					convert_toupper(sr->label);
					if( !sr->doc || !strlen(sr->doc) )
						sr->doc = "nil";
					if( sr->class == DC_CODED ) {
						for(sri=sr->map_std_reg_field_index;
							sri; sri=sri->next) {
							if( !sri->label  || !strlen(sri->label) ) {
							  invalid_sitem(sitem, 
										"register MAP missing label");
							  sri->label = " ";
							}
							convert_toupper(sri->label);
							/* don't check sri->value since anything is valid */
						}
					}
					if( sr->class <= 0 ) {
						invalid_sitem(sitem, "register CLASS");
						fprintf(stderr, "  register LABEL=%s\n", sr->label);
					}
					if( sr->size <= 0 ) {
						invalid_sitem(sitem, "register SIZE");
						fprintf(stderr, "  register LABEL=%s\n", sr->label);
					}
						/* it's okay to omit display if class is coded */
					if( sr->label_type <= 0 ) {
						if( sr->class == DC_CODED || sr->class == DC_FILLER ) {
							sr->label_type = DF_DEFAULT;
						} else {
							invalid_sitem(sitem, "register DISPLAY");
							fprintf(stderr, "  register LABEL=%s\n", sr->label);
						}
					}
				}
				break;

		}
	}
		/* now check for duplicate id numbers */
	for(sitem = first_sitem; sitem; sitem = sitem->next) {
		struct std_item *s2;
		if( sitem->id <= 0 )
			continue;
		for(s2 = sitem->next; s2; s2 = s2->next) {
			if(sitem->id == s2->id) {
			  errval = 1;
			  fprintf(stderr,
			  "Error, Duplicate std item id number (%d) in std items '%s' and '%s'\n",
			  sitem->id, sitem->name, s2->name);
			}
		}
	}
}

	/* data checks on std_segs,
		and any necessary references to other structures */
	/* Also, adjust things such as upper/lower case. */
check_std_segs()
{
	struct std_segment_element *sse;

	for(sseg = first_sseg; sseg; sseg = sseg->next) {
		if( !sseg->name || !strlen(sseg->name) ) {
			sseg->name = " ";
			invalid_sseg(sseg, "NAME");
		}
		convert_toupper(sseg->name);
		if( sseg->type_id <= 0 ) {
			invalid_sseg(sseg, "ID number");
		}
		if( sseg->type <= 0 ) {
			invalid_sseg(sseg, "ID type");
		}
		if( !sseg->label || !strlen(sseg->label) ) {
			/* it's okay to omit label */
			/* invalid_sseg(sseg, "LABEL"); */
			sseg->label = " ";
		}
		convert_toupper(sseg->label);
		if( !sseg->doc || !strlen(sseg->doc) ) {
			sseg->doc = " ";
		}
		for(sse=sseg->elements; sse; sse=sse->next) {
			if( !sse->name || !strlen(sse->name) ) {
				invalid_sseg(sseg, "ELEMENT");
				sse->name = " ";
			}
			convert_tolower(sse->name);
			sse->id = find_id_of_sitem(sse->name);
			if( !sse->id ) {
				errval = 1;
				fprintf(stderr,
					"Error, In std segment '%s' (id %s %d), no std item named '%s'\n",
					sseg->name, encode_type(sseg->type,seg_table),
					sseg->type_id, sse->name);
			}
		}
	}
		/* now check for duplicate id numbers */
	for(sseg = first_sseg; sseg; sseg = sseg->next) {
		struct std_segment *s2;
		if(sseg->type <= 0 || sseg->type_id <= 0)
			continue;
		for(s2 = sseg->next; s2; s2 = s2->next) {
			if(sseg->type == s2->type && sseg->type_id == s2->type_id) {
				errval = 1;
				fprintf(stderr,
						"Error, Duplicate std segment id type (%s) and number (%d)\n",
						encode_type(sseg->type,seg_table), sseg->type_id);
				fprintf(stderr, " in std segments '%s' and '%s'\n",
						sseg->name, s2->name);
			}
		}
	}
}

	/* data checks on os_items,
		and any necessary references to other structures */
	/* Also, adjust things such as upper/lower case. */
check_os_items()
{
	struct os_index *oi;

	for(oitem = first_oitem; oitem; oitem = oitem->next) {
		if( !oitem->name || !strlen(oitem->name) ) {
			invalid_oitem(oitem, "NAME");
			oitem->name = " ";
		}
		if( oitem->id <= 0 )
			invalid_oitem(oitem, "ID");
		if( oitem->type <= 0 )
			invalid_oitem(oitem, "TYPE");
		if( oitem->class <= 0 )
			invalid_oitem(oitem, "CLASS");
		if( oitem->size <= 0 )
			invalid_oitem(oitem, "SIZE");
		if( !oitem->doc || !strlen(oitem->doc) )
			oitem->doc = "nil";
		convert_tolower(oitem->name);
		for( oi = oitem->map; oi; oi=oi->next) {
			if( !oi->std_name || !strlen(oi->std_name) )
				oi->std_name = " ";
			convert_tolower(oi->std_name);
			oi->std_value = find_id_of_index(oi->std_name);
			if( !oi->std_value ) {
				errval = 1;
				fprintf(stderr,
					"Error, In os item %s (id %d), no std_item_index %s\n",
					oitem->name, oitem->id, oi->std_name);
			}
		}
	}
		/* now check for duplicate id numbers */
	for(oitem = first_oitem; oitem; oitem = oitem->next) {
		struct os_item *o2;
		if( oitem->id <= 0 )
			continue;
		for(o2 = oitem->next; o2; o2 = o2->next) {
			if(oitem->id == o2->id) {
			  errval = 1;
			  fprintf(stderr,
			  "Error, Duplicate os item id number (%d) in os items '%s' and '%s'\n",
			  oitem->id, oitem->name, o2->name);
			}
		}
	}
}

	/* output to bin file for std_items */
out_std_items()
{
	struct std_index *si;
	struct std_reg_field *sr;
	struct std_reg_field_index *sri;
	int need_nl, i;

	fprintf(bin_out, "1.2 %d\n", cnt_std_items);
	for(sitem = first_sitem; sitem; sitem = sitem->next) {
		need_nl = 0;
		fprintf(bin_out, "%d %s %d ", sitem->id, sitem->name, sitem->type);
		switch(sitem->type){
			case DT_BYTE_VECTOR:
			case DT_COUNTED_SHORT_VECTOR:
			case DT_COUNTED_LONG_VECTOR:
			case DT_ADDR_CNT_VECTOR:
				fprintf(bin_out, "%d %d |%s|\n",
						sitem->size, sitem->label_type, sitem->label);
				break;

			case DT_SHORT_INDEX:
			case DT_TINY_INDEX:
			case DT_INDEXED:
				fprintf(bin_out, "%d |%s| %d",
					sitem->label_type, sitem->label, sitem->map_index_count);
				need_nl = 1;
				/* print 13 entries for first line, 15 thereafter */
				for(i=13,si=sitem->map_index; si; si=si->next) {
					need_nl = 1;
					fprintf(bin_out,
						" %lu %s |%s|", si->value, si->name, si->label);
					if( !--i ) {
						fprintf(bin_out, "\n");
						i = 15;
						need_nl = 0;
					}
				}
				break;

			case DT_SHORT_REGISTER:
			case DT_REGISTER:
				fprintf(bin_out, "%d |%s| %d\n",
					sitem->label_type, sitem->label, sitem->map_reg_count);
				for(sr=sitem->map_reg; sr; sr=sr->next) {
					fprintf(bin_out, "%d %d %d |%s|",
						sr->size, sr->class, sr->label_type, sr->label);
					need_nl = 1;
					if( sr->class == DC_CODED ) {
						fprintf(bin_out,
							" %d", sr->map_std_reg_field_index_count);
						for(i=12,sri=sr->map_std_reg_field_index;
							sri; sri=sri->next) {
							need_nl = 1;
							fprintf(bin_out,
								" %lu |%s|", sri->value, sri->label);
							if( !--i ) {
								fprintf(bin_out, "\n");
								i = 12;
								need_nl = 0;
							}
						}
					}
					if( need_nl ) {
						fprintf(bin_out, "\n");
						need_nl = 0;
					}
				}
				break;

			default:
				fprintf(bin_out, "%d |%s|\n", sitem->label_type, sitem->label);
				break;
		}
		if( need_nl )
			fprintf(bin_out, "\n");
	}
}

	/* output to bin file for std_segs */
out_std_segs()
{
	struct std_segment_element *sse;

	fprintf(bin_out, "2.1 %d\n", cnt_std_segs);
	for(sseg = first_sseg; sseg; sseg = sseg->next) {
		fprintf(bin_out, "%d %d |%s|", sseg->type, sseg->type_id, sseg->label);
		fprintf(bin_out, " %d", sseg->elements_count);
		for(sse=sseg->elements; sse; sse=sse->next) {
			fprintf(bin_out, " %d", sse->id);
		}
		fprintf(bin_out, "\n");
	}
}

	/* output to bin file for os_items */
out_os_items()
{
	int i;
	struct os_index *oi;

	fprintf(bin_out, "4.1 %d\n", cnt_os_items);
	for(oitem = first_oitem; oitem; oitem = oitem->next) {
		fprintf(bin_out, "%d %d", oitem->id, oitem->type);
		switch(oitem->type) {
			case DT_BYTE_VECTOR:
			case DT_COUNTED_SHORT_VECTOR:
			case DT_COUNTED_LONG_VECTOR:
			case DT_ADDR_CNT_VECTOR:
			case DT_ASCIZ:
			case DT_BIT_VECTOR:
			case DT_SHORT_VECTOR:
			case DT_LONG_VECTOR:
				fprintf(bin_out, " %d", oitem->size);
				break;

			case DT_TINY_INDEX:
			case DT_SHORT_INDEX:
			case DT_INDEXED:
				fprintf(bin_out, " %d", oitem->map_count);
				for( i=50, oi = oitem->map; oi; oi=oi->next) {
					fprintf(bin_out, " %lu %lu", oi->os_value, oi->std_value);
					if(!--i) {
						fprintf(bin_out, "\n");
						i=50;
					}
				}
				break;

			default:
				break;
		}
		fprintf(bin_out, "\n");
	}
}

	/* output the 2 include files */
output_h() {
	char **cp;
	struct std_index *si;

	std_h = fopen(STD_H_NAME, "w");
	if( std_h == NULL )  {
		errval = 1;
		fprintf(stderr, "Error, Unable to open %s\n", STD_H_NAME);
		exit(4);
	}
	os_h = fopen(OS_H_NAME, "w");
	if( os_h == NULL )  {
		errval = 1;
		fprintf(stderr, "Error, Unable to open %s\n", OS_H_NAME);
		exit(4);
	}
	fprintf(std_h, "#define UERF_VERSION %d\n", uerf_version);
	fprintf(os_h, "#define UERF_VERSION %d\n", uerf_version);
	print_block(std_h, std_h_fixed1);
	fprintf(std_h, "/* sitem id codes */\n");
	for(sitem = first_sitem; sitem; sitem = sitem->next) {
		fprintf(std_h, "#define DD$%-*s %d	/* %s */\n",
			pr_width(sitem->name),
			sitem->name, sitem->id, sitem->doc);
	}
	fprintf(std_h, "\n/* Standard coded field definitions */\n");
	for(sitem = first_sitem; sitem; sitem = sitem->next) {
		switch(sitem->type) {
			case DT_SHORT_INDEX:
			case DT_TINY_INDEX:
			case DT_INDEXED:
				fprintf(std_h, "/* Coded values for %s */\n", sitem->name);
				for(si=sitem->map_index; si; si=si->next) {
					fprintf(std_h, "#define %-*s %lu	/* %s */\n",
						pr_width2(si->name),
						si->name, si->value, si->label_plain);
			}
		}
	}
	print_block(std_h, std_h_fixed2);
	fprintf(std_h, "/* sseg id codes */\n");
	for(sseg = first_sseg; sseg; sseg = sseg->next) {
		fprintf(std_h, "#define DD$%-*s %d	/* %s */\n",
			pr_width(sseg->name),
			sseg->name, sseg->type_id, sseg->doc);
	}
	print_block(std_h, std_h_fixed3);
	print_block(os_h, os_h_fixed1);
	fprintf(os_h, "/* oitem id codes */\n");
	for(oitem = first_oitem; oitem; oitem = oitem->next) {
		fprintf(os_h, "#define OS$%-*s %d	/* %s */\n",
			pr_width(oitem->name),
			oitem->name, oitem->id, oitem->doc);
	}
	print_block(os_h, os_h_fixed2);
}


	/* upper case to lower case */
convert_tolower(cp)
char *cp;
{
	if(!cp)
		return;
	for(; *cp; cp++) {
		if( *cp >= 'A' && *cp <= 'Z' )
			*cp = *cp - ('A' - 'a');
	}
}

	/* lower case to upper case */
convert_toupper(cp)
char *cp;
{
	if(!cp)
		return;
	for(; *cp; cp++) {
		if( *cp >= 'a' && *cp <= 'z' )
			*cp = *cp + ('A' - 'a');
	}
}


/* return id number of std item with given name */
/* return 0 if not found */
find_id_of_sitem(name)
char *name;
{
	if(!name || !*name || (name[0]==' ' && name[1]=='\0') )
		return(0);
	for(sitem = first_sitem; sitem; sitem = sitem->next) {
		if(!strcmp(name, sitem->name))
			return(sitem->id);
	}
	return(0);
}

	/* go through the std items, looking for name in
        the index values.  return the value of name */
		/* return 0 if failure */
unsigned long
find_id_of_index(name)
char *name;
{
	struct std_index *si;

	if(!name || !*name || (name[0]==' ' && name[1]=='\0') )
		return((unsigned long)0);
	for(sitem = first_sitem; sitem; sitem = sitem->next) {
		if( sitem->type != DT_SHORT_INDEX &&
			sitem->type != DT_INDEXED &&
			sitem->type != DT_TINY_INDEX )
				continue;
		for(si=sitem->map_index; si; si=si->next) {
			if(!strcmp(name, si->name))
				return(si->value);
		}
	}
	return((unsigned long)0);
}

/* return the print width for this string */
/*  I had to try to figure out the rule used by the vaxlisp system */
pr_width(name)
char *name;
{
	int len = strlen(name);
	if( len < 12 )
		return(12);
	else if( len < 16 )
		return(16);
	else if( len < 20 )
		return(20);
	else
		return(24);
}

pr_width2(name)
char *name;
{
	int len = strlen(name);
	if( len < 15 )
		return(15);
	else if( len < 19 )
		return(19);
	else if( len < 23 )
		return(23);
	else
		return(27);
}

print_block(fp, block)
FILE *fp;
char *block[];
{
char **cp;
for( cp=block; **cp; cp++ )
	fprintf(fp, *cp);
}

char *std_h_fixed1[] = {
 "/*\n",
 "**	.title Standard DSD definitions\n",
 "**	.ident / 1.14 /\n",
 "**\n",
 "**\n",
 "**	  File:	std_dsd.h\n",
 "** Description:	Standard DSD definitions\n",
 "**	Author:	Luis Arce\n",
 "**	  Date:	8-Oct-1986\n",
 "**\n",
 "**\n",
 "**	Copyright 1986, Digital Equipment Corporation\n",
 "**\n",
 "**\n",
 "**++\n",
 "**	These definitions were generated by the DSD Editor / Compiler\n",
 "**--\n",
 "*/\n",
 "\n",
 "\n",
 "\n",
 "#define STD_DSD_FILE_FORMAT 2\n",
 "\n",
 "/* Miscellaneous definitions needed for the DSD access functions */\n",
 "\n",
 "#define DD$HEADER_BYTES 8		/* Size of standard header */\n",
 "#define DD$MAX_SEGMENT_ELEMENTS 512	/* Max elements excluding the header */\n",
 "#define DD$VALID_CODE_SIZE 2		/* Size in bits of a validity code */\n",
 "\n",
 "#define DD$ELEMENT_COUNT        ctx->segment_DSD_ptr->COUNT\n",
 "#define DD$VALID_BITS(COUNT)    ( (COUNT + 1) * DD$VALID_CODE_SIZE)\n",
 "#define DD$VALID_BYTES(COUNT)   ( (DD$VALID_BITS(COUNT) +7) / 8)\n",
 "\n",
 "\0"
};

char *std_h_fixed2[] = {
 "#ifndef ES$EIS\n",
 "\n",
 "/* Standard segment type:  codes */\n",
 "#define ES$EIS          1	/* Event Identification Segment */\n",
 "#define ES$DIS          2	/* Device Information Segment */\n",
 "#define ES$SDS          3	/* Supporting Data Segment */\n",
 "#define ES$CDS          4	/* Correlating Data Segment */\n",
 "#define ES$ADS          5	/* Additional Data Segment */\n",
 "#define ES$SIS          6	/* Summary Information Segment */\n",
 "#define ES$CIS          7	/* Configuration Information Segment */\n",
 "#endif  ES$EIS\n",
 "\n",
 "\0"
};

char *std_h_fixed3[] = {
 "\n",
 "/* STD-RECORD id codes */\n",
 "\n",
 "/********** STRUCTURE DEFINITIONS FOR THE STANDARD DSD TABLES **********/\n",
 "\n",
 "\n",
 "	/**** Structure definition for the standard segment header ****/\n",
 "\n",
 "typedef struct\n",
 "  {\n",
 "  short			type;\n",
 "  short			subtype;\n",
 "  short			version;\n",
 "  short			length;\n",
 "  DD$BYTE VALID_byte[DD$MAX_SEGMENT_ELEMENTS * DD$VALID_CODE_SIZE / 8];\n",
 "  }\n",
 " DD$STD_HEADER, *DD$STD_HEADER_PTR;\n",
 "\n",
 "\n",
 "	/**** Structure definition for std item codes array ****/\n",
 "\n",
 "typedef struct \n",
 "  {\n",
 "  long			CODE;\n",
 "  long			LABEL_IX;	/* index to DD$DSP_LABELS */\n",
 "  }\n",
 " DD$STD_CODES_DSD, *DD$STD_CODES_DSD_PTR;\n",
 "\n",
 "\n",
 "	/**** Structure definition for std item register array ****/\n",
 "\n",
 "typedef struct\n",
 "  {\n",
 "  DD$BYTE		SIZE;		/* bits in field		*/\n",
 "  DD$BYTE		TYPE;		/* field type (int, coded etc)	*/\n",
 "  short			COUNT;		/* for coded fields only	*/\n",
 "  long			CODE_IX;	/* index to DD$STD_CODES	*/\n",
 "  long			LABEL_IX;	/* index to DD$DSP_LABELS	*/\n",
 "  }\n",
 " DD$STD_REGS_DSD, *DD$STD_REGS_DSD_PTR;\n",
 "\n",
 "\n",
 "	/**** Structure definition for std item array ****/\n",
 "\n",
 "typedef struct\n",
 "  {\n",
 "  short			ID;\n",
 "  DD$BYTE		TYPE;		/* field type (int, coded etc)	*/\n",
 "  DD$BYTE		FILLER1;        /* filler                       */\n",
 "  short			COUNT;		/* for coded or vectors only	*/\n",
 "  short 		FILLER2;        /* filler                       */\n",
 "  long			INDEX;		/* index to DD$STD_CODES/REGS	*/\n",
 "  long			LABEL_IX;	/* index to DD$DSP_LABELS	*/\n",
 "  }\n",
 " DD$STD_ITEMS_DSD, *DD$STD_ITEMS_DSD_PTR;\n",
 "\n",
 "\n",
 "	/**** Structure definition for std segment items array ****/\n",
 "\n",
 "typedef struct\n",
 "  {\n",
 "  long			ITEM_IX;	/* index to DD$STD_ITEMS	*/\n",
 "  long			ITEM_OFFSET;	/* item offset in seg		*/\n",
 "  }\n",
 " DD$STD_SEG_ITEMS_DSD, *DD$STD_SEG_ITEMS_DSD_PTR;\n",
 "\n",
 "\n",
 "	/**** Structure definition for std segment array ****/\n",
 "\n",
 "typedef struct\n",
 "  {\n",
 "  short			TYPE;\n",
 "  short			SUBTYPE;\n",
 "  short			COUNT;		/* count of items in seg	*/\n",
 "  short			STR_OFFSET;	/* offset to string area in seg	*/\n",
 "  long			LABEL_IX;	/* index to DD$DSP_LABELS	*/\n",
 "  long			SEG_ITEM_IX;	/* index to DD$STD_SEG_ITEMS	*/\n",
 "  }\n",
 " DD$STD_SEGS_DSD, *DD$STD_SEGS_DSD_PTR;\n",
 "\n",
 "\n",
 "\n",
 "	/**** Structure definition for std record array ****/\n",
 "\n",
 "typedef struct\n",
 "  {\n",
 "  short			TYPE;\n",
 "  short			SUBTYPE;\n",
 "  short			VERSION;\n",
 "  short			TAILOR_ID;\n",
 "  short			COUNT;		/* count of segs in rec		*/\n",
 "  short                 FILLER1;        /* filler                       */\n",
 "  long			SEG_IX;		/* index to DD$STD_SEGS		*/\n",
 "  }\n",
 " DD$STD_RECORD_DSD, *DD$STD_RECORD_DSD_PTR;\n",
 "\n",
 "\n",
 "	/**** Standard Data Structure Definition Context Structure ****/\n",
 "\n",
 "typedef struct\n",
 "  {					/* Table access function info	*/\n",
 "  int			CTX_type;	/* Not for use by application	*/\n",
 "  int			curr_item;	/* Not for use by application	*/\n",
 "  int			curr_field;	/* Not for use by application	*/\n",
 "						/* Data_record info	*/\n",
 "  DD$STD_RECORD_DSD_PTR	rec_DSD_ptr;	/* pointer to curr record DSD	*/\n",
 "						/* Data-segment info	*/\n",
 "  DD$STD_HEADER_PTR	segment_ptr;	/* Pointer to curr data-segment */\n",
 "  DD$STD_SEGS_DSD_PTR	segment_DSD_ptr;/* Pointer to curr segment DSD	*/\n",
 "  int			segment_VALID_code;\n",
 "						/* Data-item info	*/\n",
 "  DD$BYTE		*item_ptr;	/* Pointer to curr data-item	*/\n",
 "  DD$STD_ITEMS_DSD_PTR	item_DSD_ptr;	/* Pointer to curr item DSD	*/\n",
 "  int			item_VALID_code;\n",
 "\n",
 "						/* Register field info	*/\n",
 "  DD$BYTE		field_position;	/* Bits to right of field	*/\n",
 "  DD$STD_REGS_DSD_PTR	field_DSD_ptr;	/* Pointer to curr field DSD	*/\n",
 "						/* Coded item info	*/\n",
 "  DD$STD_CODES_DSD_PTR	code_DSD_ptr;	/* pointer to curr codes DSD	*/\n",
 "						/* Application infor	*/\n",
 "  long			user_1;		/* Reserved for application	*/\n",
 "  long			user_2;		/* Reserved for application	*/\n",
 "  long			user_3;		/* Reserved for application	*/\n",
 "}\n",
 " DD$STD_DSD_CTX, *DD$STD_DSD_CTX_PTR;\n",
 "\n",
 "\n",
 "\n",
 "/********** STRUCTURE DEFINITIONS FOR DISPLAY DSD TABLES **********/\n",
 "\n",
 "\n",
 "	/**** Structure definition for the label DSD array ****/\n",
 "\n",
 "typedef struct\n",
 "  {\n",
 "  DD$BYTE		TYPE;		/* display type (hex, dec etc)	*/\n",
 "  DD$BYTE		COUNT;		/* length of label		*/\n",
 "  short                 FILLER1;        /* filler                       */\n",
 "  long			STRINGS_IX;	/* index to strings array	*/\n",
 "  }\n",
 " DD$DSP_LABELS, *DD$DSP_LABELS_PTR;\n",
 "\n",
 "\n",
 "/* End of std_dsd.h */\n",
 "\0"
};

char *os_h_fixed1[] = {
 "/*\n",
 "**	.title Operating system DSD definitions\n",
 "**	.ident / 1.14 /\n",
 "**\n",
 "**\n",
 "**	  File:	ultrix_dsd.h\n",
 "** Description:	Operating system DSD definitions\n",
 "**	Author:	Luis Arce\n",
 "**	  Date:	8-Oct-1986\n",
 "**\n",
 "**\n",
 "**	Copyright 1986, Digital Equipment Corporation\n",
 "**\n",
 "**\n",
 "**++\n",
 "**	These definitions were generated by the DSD Editor / Compiler\n",
 "**--\n",
 "*/\n",
 "\n",
 "\n",
 "\n",
 "#define OS_DSD_FILE_FORMAT 2\n",
 "\n",
 "\0"
};

char *os_h_fixed2[] = {
 "\n",
 "/********** STRUCTURE DEFINITIONS FOR THE OS DSD TABLES **********/\n",
 "\n",
 "	/**** Structure definition for an os data-item codes DSD ****/\n",
 "\n",
 "typedef struct\n",
 "  {\n",
 "  long			OS_CODE;\n",
 "  long		        STD_CODE;\n",
 "  }\n",
 " DD$OS_CODES_DSD, *DD$OS_CODES_DSD_PTR;\n",
 "\n",
 "	/**** Structure definition for an os data-item DSD ****/\n",
 "\n",
 "typedef struct\n",
 "  {\n",
 "  short			ID;		/* not same as index or std	*/\n",
 "  DD$BYTE		TYPE;		/* item type (long, coded etc)	*/\n",
 "  DD$BYTE               FILLER1;        /* filler                       */\n",
 "  short			COUNT;		/* for coded or vectors		*/\n",
 "  short                 FILLER2;        /* filler                       */\n",
 "  long			CODE_IX;	/* index to DD$OS_CODSE_DSD	*/\n",
 "  }\n",
 " DD$OS_ITEMS_DSD, *DD$OS_ITEMS_DSD_PTR;\n",
 "\n",
 "\n",
 "\n",
 "/* End of ultrix_dsd.h */\n",
 "\0"
};

process_args(argc, argv, arge)
int argc;
char **argv, **arge;
{
	char *cp, c;

	cmd_name = *argv++;
	for(;*argv;argv++){
		cp = *argv;
		if(*cp++ != '-')
			break;
		while(c= *cp++){
			switch(c) {
				case 'a':
					opt_auto_assign = 1;
					break;
				case 'd':
					opt_d = 1;
					break;
				case 'i':
					opt_i = 1;
					break;
				case 'j':
					opt_j = 1;
					break;
				default:
					errval = 1;
					fprintf(stderr, "Error, Unknown option: '%c'\n", c);
				case 'h':
printf("Usage: %s options filenames\n", cmd_name);
printf("  Option a = assign id numbers automatically\n");
printf("  Option i = use old method of index value assignment (by position)\n");
printf("  Option j = issue warning if given index values differs from position\n");
					break;
			}
		}
	}
	db_files = argv;
}

print_where()
{
	fprintf(stderr, "File %s, line %d: %s\n", in_name, lineno, linebuf);
}
