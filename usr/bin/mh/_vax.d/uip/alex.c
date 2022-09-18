/**************************************************************/
/* FILE:	alex.c					      */
/* AUTHOR:	Peter Walburn @ DEC (EUEG) Reading, England.  */
/* DATE:	February 28, 1990			      */
/* CONTENTS:	Address Extraction Utility		      */
/**************************************************************/

static char rcsid[] = "$Id: alex.c,v 1.14 90/11/25 19:04:05 sharpe Exp $";

#include "../h/mh.h"
#include "../h/addrsbr.h"
#include "../h/aliasbr.h"
#include "../h/dropsbr.h"
#include <stdio.h>
#include "../zotnet/mts.h"
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/file.h>
#include <limits.h>

#define MAKESTR(X) (char *)malloc(strlen(X) + 1)
#define MAKESTRUCT(X) (struct X *)malloc(sizeof(struct X))
#define MAKEARGV(X) (char **)malloc(sizeof(char *) * X)
#define NULLFILE (FILE *)NULL

static struct swit  switches[] = {  /* switches is a structure storing */
#define	ADDRSW	0		    /* all the options for 'alex'.     */
    "address string", 0,	    /* It is used for parsing the      */
				    /* command line and also for       */
#define	ALIASW	1		    /* displaying all the options when */
    "alias name", 0,		    /* the -help option is used.       */

#define	COMPSW  2	
    "compress", 0,
#define	NCOMPSW	3
    "nocompress", 0,

#define	FIELDSW	4
    "field name{/name}", 0,
#define NFIELDSW 5
    "nofield name{/name}", 0,

#define GLOBSW  6
    "global", 0,

#define NGLOBSW 7
    "noglobal", 0,

#define	NAMESW	8
    "name name", 0,

#define	QUERYSW	9
    "query", 0,
#define	NQUERYSW 10
    "noquery", 0,

#define	REPLSW	11
    "replace", 0,
#define	NREPLSW	12
    "noreplace", 0,

#define	WIDTHSW	13
    "width n", 0,

#define HELPSW 14
    "help", 0,

    NULL, NULL
};

/*  */

struct fields { 	  /* Structure for storing which fields to search */
    char **priorities;
    struct fields *next;
}; 

struct entry {		  /* Structure for storing aliases */
    char *words;
    char **address_list;
    struct entry *next;
};

struct entry *get_alias ();
struct entry *read_alias_file ();
struct fields *add_field ();

static void del_field (), del_all_fields (), output_aliases (), 
	    add_alias_entry (), compress_alias ();

/* ARGSUSED */

main (argc, argv)
int	argc;
char   *argv[];
{

    char *cp,
	 **argp,
	 **ap,
	 *folder = NULL,       /* Stores the mail folder to be used. */
	 *maildir, 	       /* Stores full path name for mail folder. */
	 *alias_file = NULL,   /* Stores name of alias file to be used. */
	 *alias_name = NULL,   /* Stores name to be used for alias. */
	 *msgs[MAXARGS],       /* Stores the messages to be searched. */
	 buf[BUFSIZ],	       /* General purpose buffer. */
	 *msg = NULL,	       /* Holds path of message being searched. */
	 name[NAMESZ],	       /* Field name being looked at in message. */
	 *arguments[MAXARGS],
	 *address = NULL,      /* Stores address specified on command line. */
	 *addr1;	       /* Stores full address from field. */

    int msgp = 0,
	compnum,
	count = 0,     /* General purpose index variable. */
	replace = 0,   /* flag which is set if -replace option given. */
	compress = 0,  /* flag set if -compress option given. */
	query = 0,     /* flag set if -query option given. */
	width = 72,    /* used for specifying maximum width of alias line; */
		       /* 72 is default. Can be changed with -width option. */
	global = 0,    /* Used for specifying if all occurrences of an alias */
		       /* in alias file should be updated.  */
	alias_num = 0, /* Counts the number of occurrences of an alias. */
	low_msg,       /* Set to the lowest message in specified sequence. */
	hgh_msg,       /* Set to highest message in specified sequence. */
	found,	       /* Flag set if header field is found. */
	do_default = 1,/* If no field values given, then use default. */
	msgnum,
	state; 	      /* state is used in searching mail messages; it   */
		      /* corresponds to a header field or the body of a */
		      /* mail message. */	

    static struct fields *ref;

    static struct msgs *mp = NULL;     /* Holds info about current , highest */
				       /* and lowest message, etc.	     */
    static struct mailname *mail_name; /* Holds all info about a mail address */

    FILE *in,
	 *out;

    static struct fields *field_vals = NULL; /* Holds header fields to search */
    static struct entry *alias_lists = NULL; /* Holds alias file info */
    static struct entry *new_alias = NULL;   /* Holds new alias info */

    invo_name = r1bindex (argv[0], '/');
    if ((cp = m_find (invo_name)) != NULL) {
	ap = brkstring (cp = getcpy (cp), " ", "\n");
	ap = copyip (ap, arguments);
    }
    else
	ap = arguments;
    (void) copyip (argv + 1, ap);
    argp = arguments;

    while (cp = *argp++) {
	if (*cp == '-')
	    switch (smatch (++cp, switches)) {
		case AMBIGSW: 
		    ambigsw (cp, switches);
		    done (1);
		case UNKWNSW: 
		    adios (NULLCP, "-%s unknown", cp);
		case HELPSW: 
		    (void) sprintf (buf, "%s [+folder] [switches]", invo_name);
		    help (buf, switches);
		    done (1);

		case ADDRSW: 
		    if (!(address = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    address = trimcpy (address); /* Remove white spaces */
		    continue;

		case ALIASW: /* Argument following -alias can be a string or */
			     /* can be "-", to output on stdout.	     */
		    if (!(cp = *argp++))
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    if (*cp == '-') {
			if (*++cp)
			    adios (NULLCP, "missing argument to %s", argp[-2]);
			alias_file = NULL;
		    }
		    else
		        alias_file = cp;
		    continue;
		
		case GLOBSW:
		    global++;
		    continue;

		case NGLOBSW:
		    global = 0;
		    continue;

		case COMPSW: 
		    compress++;
		    continue;

		case NCOMPSW: 
		    compress = 0;
		    continue;

		case FIELDSW: 
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    field_vals = add_field (field_vals,cp);
		    do_default = 0;
		    continue;

		case NFIELDSW: 
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    if (uleq (cp, "all"))
			del_all_fields (field_vals);
		    else
		        del_field (field_vals, cp);
		    continue;

		case NAMESW: 
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    alias_name = cp;
		    continue;

		case QUERYSW:
		    query++;
		    continue;

		case NQUERYSW:
		    query = 0;
		    continue;

		case REPLSW: 
		    replace++;
		    continue;

		case NREPLSW: 
		    replace = 0;
		    continue;

		case WIDTHSW:
		    if (!(cp = *argp++) || *cp == '-')
			adios (NULLCP, "missing argument to %s", argp[-2]);
		    if ((width = atoi (cp)) < 1)
			adios (NULLCP, "bad argument %s %s", argp[-2], cp);
		    continue;
	    }

	if (*cp == '+' || *cp == '@') {
	    if (folder)
		adios (NULLCP, "only one folder at a time!");
	    else
	 	folder = path (cp + 1, *cp == '+' ? TFOLDER : TSUBCWF);
	}
	else
	    msgs[msgp++] = cp;
    }

/* If no field values specified then use default : */
    if (do_default)
	field_vals = add_field (field_vals,"reply-to/sender/from/to");

/* If there is no command-line alias file name, then see if there is a default
 * one provided in the 'Aliasfile: ' profile entry...
 */
    if (alias_file == NULL || *alias_file == '\0')
	alias_file = m_find ("Aliasfile");

/* Ensure that any relative aliasfile is found in the MAIL directory. */
    if (alias_file && *alias_file)
	alias_file = m_mailpath (alias_file);

/* Read in the alias file and store all the lines in alias_lists. */
    if (alias_file && *alias_file && alias_name && *alias_name) 
	alias_lists = read_alias_file (alias_lists, alias_file);

    if (!msgp)
	msgs[msgp++] = "cur";
    if (!folder)
	folder = m_getfolder ();
    maildir = m_maildir (folder);

    if (chdir (maildir) == NOTOK)
	adios (maildir, "unable to change directory to");
    if (!(mp = m_gmsg (folder)))
	adios (NULLCP, "unable to read folder %s", folder);
    if (mp -> hghmsg == 0)
	adios (NULLCP, "no messages in %s", folder);

    for (msgnum = 0; msgnum < msgp; msgnum++) 
	if (!m_convert (mp, msgs[msgnum]))
	    done (1);
    m_setseq (mp);
    low_msg = mp -> lowsel;
    hgh_msg = mp -> hghsel;

/* new_alias is where the new addresses are going to be stored; */
/* If the alias already exists, new_alias points to structure holding all */
/* that alias information. */
    new_alias = get_alias (alias_lists, (alias_name) ? alias_name : "",
			  replace, global, alias_num);

    do {
/* Repeat this for each message in the sequence given on the command line. */

        for (msgnum =low_msg; msgnum <= hgh_msg; msgnum++) {
            if (mp -> msgstats[msgnum] & SELECTED)
	        mp -> lowsel = msgnum;

            if (mp -> lowsel != mp -> curmsg)
                m_setcur (mp, mp -> lowsel);
            m_sync (mp);
            m_update ();

            msg = getcpy (m_name (mp -> lowsel));

/* If addresses are to be taken from header fields, then search through each */
/* specified message for each field in turn. */

            if (!address && (mp -> lowsel == msgnum)) {
                if ((in = fopen (msg, "r")) == NULLFILE)
	            adios (msg, "unable to open");

                ref = field_vals;
                while (ref != (struct fields *)NULL) {
	            if (ref -> priorities[count] == NULLCP) {
		        ref = ref -> next;
		        count = 0;
		    }
		    else {
		        found = 0;
                        for (compnum = 1, state = FLD;;) {
    	                    switch (state = m_getfld (state, name, buf, 
						      sizeof buf, in)) {
	                        case FLD:
	                        case FLDEOF:
	                        case FLDPLUS:
	                            compnum++;
	                            cp = add (buf, NULLCP);
	                            while (state == FLDPLUS) {
	 	                        state = m_getfld (state, name, buf, 
						          sizeof buf, in);
		                        cp = add (buf, cp);
	                            }

/* Compare the current header field to that being sought. The comparison */
/* is case insensitive.							 */

	                            if (ref != (struct fields *)NULL && 
					uleq (name, ref -> priorities[count])) {
				        cp = trimcpy (cp); /* Remove spaces */

/* This next bit strips off unnecessary parts of the address, eg. the  */
/* signature part. mail_name is a structure which holds the whole      */	
/* address, mail_name -> m_mbox holds the actual address.	       */

				        if (global || !alias_num) {    
			                    while (addr1 = getname(cp)) {
			                        if ((mail_name = getm (addr1, 
						    NULLCP, 0, AD_HOST, 
						    NULLCP)) == NULL)
					            adios(NULLCP,"bad address");
			                        add_alias_entry (new_alias, 
							        mail_name -> m_mbox, 
							        query);
				                mnfree (mail_name);
				            }
				        }
					found = 1;
	                            }
	                            free (cp);
	                            if (state != FLDEOF)
		                        continue;
	                            break;
   	        
	                        case BODY:
	                        case BODYEOF:
	                        case FILEEOF:
		                    break;
    
	                        case LENERR:
                                case FMTERR:
		                    adios (NULLCP, 
				          "message format error in component #%d",
			                  compnum);
        
	                        default:
		                    adios(NULLCP,"getfld() returned %d",state);
	                    } /* end of switch */
	                    break;
                        } /* end of for compnum */
			if (found) {
			    ref = ref -> next;
			    count = 0;
			}
			else
		            count++;
	                (void) fseek (in, 0L, 0); /* Go back to beginning of  */
	            }			   /* mail message to search for next */
	        			   /* header field.		      */
                }
                (void) fclose (in);
            }
        }   
        if (address)					  /* Take address  */ 
	    add_alias_entry (new_alias, address, query);  /* from string.  */ 
        if (compress)
	    compress_alias (new_alias); /* compress the aliases if required. */

        if (!alias_file || !alias_name)
            output_aliases (new_alias, stdout, width, alias_name);

        if (alias_name && *alias_name) {
 	    new_alias = new_alias -> next;
            alias_num++;
	    if (new_alias != (struct entry *)NULL)	
	        new_alias = get_alias (new_alias, alias_name, replace, 1, 
				       alias_num);
        }
        else 
	    new_alias = (struct entry *)NULL;

    } while (new_alias != (struct entry *)NULL);

    for (msgnum = low_msg; msgnum <= hgh_msg; msgnum++) /* We've looked at */
	mp -> msgstats[msgnum] &= ~SELECTED;		/* these msgs, so  */
							/* unset selected  */
							/* flag.	   */

    if (alias_file && *alias_file && alias_name && *alias_name) {
        (void) strcpy (buf, m_backup (alias_file));  /* Make a backup of the */
        if (rename (alias_file, buf) == NOTOK)       /* alias file.          */
            adios (buf, "unable to backup %s to", alias_file);

/* Output aliases to alias file. */

	if ((out = fopen (alias_file, "w")) == NULLFILE) 
	    adios (NULLCP, "unable to fopen %s", alias_file);
	output_aliases (alias_lists, out, width, alias_name);
	(void) fclose (out);
    }
    if ((alias_num > 1) && alias_file && *alias_file) {
	(void) fprintf (stderr, "Warning: there are %d aliases with name %s in alias file\n", alias_num, alias_name);
        if (!global)
	    (void) fprintf (stderr, "         only the first occurrence was changed.\n");
    }
}

/**************************************************************************/
/* add_field () adds the string str, to the fields to be searched.        */
/* Inputs: field_vals - a pointer to the head of a linked list of 	  */
/*	                structures storing all header fields to be 	  */
/*			searched.					  */
/*	   str - a string containing the field name to be added to field  */
/*		 structure. it is in the format: field{/field}		  */
/* Outputs: field_vals - pointer to head of linked list is returned;      */
/*			 this is because the head could have been NULL    */
/*			 when first called, and will be changed.	  */
/**************************************************************************/
struct fields *add_field (field_vals, str)
struct fields *field_vals;
char *str;
{
    int loop,
        num_fields = 1;

    char *p1;

    struct fields *elem;
    struct fields *current_field;

    for (loop = 0; str[loop] != '\0'; loop++) {   /* Counts the number of  */
        if (str[loop] == '/')			  /* fields in 'str'.      */
           num_fields++;
    }

    if ((elem = MAKESTRUCT(fields)) == (struct fields *)NULL)
	adios (NULLCP, "unable to allocate storage");
    if ((elem -> priorities = MAKEARGV (num_fields + 1)) == (char **)NULL)
	adios (NULLCP, "unable to allocate storage");

    for (loop = 0; loop < num_fields; loop++) { /* Parse the fields in 'str'. */
        if ((p1 = index(str, '/')) != NULLCP) { /* Split 'str' at '/' and     */
	    *p1 = '\0';				/* store each field in field  */
            p1++;				/* structure.		      */
	}
	str = trimcpy (str);			 /* Remove white spaces. */
	elem -> priorities[loop] = getcpy (str);
        str = p1;
    }
    elem -> priorities[num_fields] = NULLCP;
    elem -> next = (struct fields *)NULL;

    if (field_vals == (struct fields *)NULL)
        field_vals = elem;
    else {
	current_field = field_vals;
	while (current_field -> next != (struct fields *)NULL)
	    current_field = current_field -> next;
	current_field -> next = elem;
    }
    return (field_vals);
}

/************************************************************************/
/* del_field called when the "-nofield str" option is used. 	        */
/* Inputs: field_vals - a pointer to the head of a linked list of 	*/
/*			structures storing header fields to be 		*/
/*			searched 					*/
/*	   str - the field name to be deleted from the field structure. */
/* Outputs: field_vals will be changed.					*/
/* The comparison of 'str' to the list of fields is case insensitive.   */
/************************************************************************/
static void del_field (field_vals, str)
struct fields *field_vals;
char *str;
{
    int count = 0;

    while (field_vals != (struct fields *)NULL) {
	while (field_vals -> priorities[count] != NULLCP) {
	    if (uleq (field_vals -> priorities[count], str))
	        field_vals -> priorities[count][0] = '\0';
	    count++;
	}
	count = 0;
	field_vals = field_vals -> next;
    }
} 

/***************************************************************************/
/* del_all_fields is called when the "-nofield all" option is given on the */
/* command line. This tells "alex" not to search for any previous header   */
/* fields given with the "-field header" command.                          */
/* Inputs: field_vals - a pointer to the head of a linked list of 	   */
/*			structures storing all the header fields to be     */
/*			searched.					   */
/* Outputs: field_vals is changed.					   */
/***************************************************************************/
static void del_all_fields (field_vals)
struct fields *field_vals;
{
    int count;

    while (field_vals != (struct fields *)NULL) {
	count = 0;
        while (field_vals -> priorities[count] != NULLCP)
            field_vals -> priorities[count++][0] = '\0';
	field_vals = field_vals -> next;
    }
}

/************************************************************************/
/* read_alias_file stores all the aliases from the alias file into the  */
/* alias structure.							*/
/* Inputs: alias_lists - a pointer to the head of a linked list of      */
/*			 structures storing each line of alias file.	*/
/*	   file - a string holding the name of the alais file.		*/
/* Outputs: Each structure in the alias_lists linked list will conatin  */
/*	    a line from the alias file.					*/
/************************************************************************/
struct entry *read_alias_file (alias_lists, file)
struct entry *alias_lists;
char *file;
{
    int i,	    /* General purpose loop counter. */
	num_addr;   /* Holds the number of addresses in each alias. */

    char alias_buf[BUFSIZ],  /* Used for reading in each line of file. */
	 *p1,		     /* Used for parsing alais lines. */
	 *alias_line,	     /* String storing the current line from file. */
	 *cp,
	 *whole_line;	     /* Pointer, used for freeing the space allocated */
			     /* to alias_line, at end of function. */

    struct entry *element,
    		 *current_alias;

    FILE *in;

    if ((in = fopen (file, "r")) == NULLFILE) {
	cp = concat ("Create alias file \"", file, "\"? ", NULLCP);
	if (!getanswer (cp))
	    done (1);
	free (cp);
	if ((in = fopen (file, "w+")) == NULLFILE)
	    adios (file, "unable to create");
	fputc ('\n', in);
	rewind(in);
    }
 
    while (fgets (alias_buf, BUFSIZ, in) != NULLCP) { /* Read in a line from */
	alias_line = add (alias_buf, NULLCP);	      /* alias file.	     */

/* If line ends with backslash then it's a continuation line, */
/* so read next line and join them.			      */

        while (continuation_line (alias_line)) {
	    if (fgets (alias_buf, BUFSIZ, in) != NULLCP) 
		alias_line = add (alias_buf, alias_line);
	}

        if ((element = MAKESTRUCT(entry)) == (struct entry *)NULL)
	    adios (NULLCP, "unable to allocate storage");

	whole_line = alias_line;

/* If there is not a ':' in line, then store whole line in element -> words. */
/* This could be a comment line in the alias file. 			     */
	if (*alias_line == ';' || (p1 = index(alias_line, ':')) == NULLCP) {  
	    element -> words = getcpy (alias_line);
	    if ((element -> address_list = MAKEARGV (1)) == (char **)NULL)
	        adios (NULLCP, "unable to allocate storage");
 	    element -> address_list[0] = NULLCP;
	    element -> next = (struct entry *)NULL;	
	}
	else {				
	    num_addr = 0;				
	    for (i = 0; alias_line[i] != '\0'; i++) {	/* Count the number */
	        if (alias_line[i] == ',')		/* of addresses in  */
	            num_addr++;				/* alias.	    */
	    }
	    num_addr++;
	    *p1 = '\0';		/* Store alias name in element -> words */
   	    p1++;
	    if ((element -> words = (char *)malloc(sizeof(char *)
					* (strlen(alias_line) + 1))) == NULLCP)
	        adios (NULLCP, "unable to allocate storage");
	    (void) strcpy (element -> words, alias_line);
            alias_line = p1;

	    if ((element -> address_list = MAKEARGV(num_addr+1))==(char **)NULL)
	        adios (NULLCP, "unable to allocate storage");

/* Parse alias line into separate addresses - split it at ','s */
	    for (i = 0; i < num_addr; i++) {  			
 	        if ((p1 = index(alias_line, ',')) != NULLCP) {
		    *p1 = '\0';
	            p1++;
		}
		element -> address_list[i] = getcpy (alias_line);
	        alias_line = p1;
	    }
 	    element -> address_list[i] = NULLCP;
	    element -> next = (struct entry *)NULL;	
        }
 
        if (alias_lists == (struct entry *)NULL)
	    alias_lists = element;
	else {
            current_alias = alias_lists;
            while (current_alias -> next != (struct entry *)NULL)
                current_alias = current_alias -> next;
            current_alias -> next = element;
	}
	free (whole_line);
    }
    (void) fclose (in);
    return (alias_lists);
}

/*************************************************************************/
/* output_aliases writes the alias structure into 'out' 		 */
/* Inputs: alias_lists - a pointer to the head of a linked list of 	 */
/*			 structures storing all the info for aliases.	 */
/*	   out - The output file (can be stdout).			 */
/*	   width - The maximum width of an alias file line, specified 	 */
/*		   on the command line, or a default of 72.		 */
/*	   name - the alias name that has been changed.			 */
/* Outputs: out - All of the alias info is output to this file.		 */
/*************************************************************************/
static void output_aliases (alias_lists, out, width, name)
struct entry *alias_lists;
FILE *out;
int width;
char *name;
{
    int count = 0,
	column = 1;

    while (alias_lists != (struct entry *)NULL) {    /* Output the alias name */
	(void) fputs (alias_lists -> words, out);    /* or comment line.      */
	if (uleq (alias_lists -> words, name)) {
	    column = strlen (alias_lists -> words) + 1;
	    if (column >= width) {
	        (void) fputs ("\\\n  ", out); /* If line over maximum width, */
	        column = 3;		    /* new line and indent.        */
	    }
	}
	while (alias_lists -> address_list[count] != NULLCP) {
	    if (alias_lists -> address_list[count][0] != '\0') {
		if (count == 0 && name)	   /* Only output a ':' if a name */
		    fputc (':', out);	   /* is specified. 		  */
		if (count != 0)
		    fputc (',', out);
		if (uleq (alias_lists -> words, name)) {
		    column=column+strlen(alias_lists -> address_list[count])+1;
		    if (column >= width - 1) {
		        (void) fputs ("\\\n  ", out);
		        column = strlen(alias_lists -> address_list[count])+2;
		    }
		}
	        (void) fputs (alias_lists -> address_list[count], out);
	    }
	    count++;
	}
	if ((alias_lists -> address_list[0] == NULLCP && 
		alias_lists -> words[0] != '\n') ||
		uleq (alias_lists -> words, name)) 
	    fputc ('\n', out);
				 		/* If output is stdout, then */
	if (out == stdout) 		        /* only 1 alias output.      */
	    alias_lists = (struct entry *)NULL;
	else					/* Else next alias. */
	    alias_lists = alias_lists -> next;
	count = 0;
    }
}

/*************************************************************************/
/* get_alias searches the aliases to see if the alias name exists. If it */
/* does, then returns a pointer to this alias, otherwise it returns a    */
/* pointer to a new alias.						 */
/* When looking for an alias name, the search is case insensitive.	 */ 
/* Inputs: alias_lists - a pointer to the head of a linked list of 	 */
/*			 structures storing alias info.			 */
/*	   name - alias name being searched for.			 */
/*	   replace - flag specifying if alias should be replace if it	 */
/*		     already exists.					 */
/*	   global - flag specifying if all occurrences of the alias	 */
/*		    name should be updated.				 */
/*	   alias_num - number of occurrences of alias already found.	 */
/* Outputs : alias_lists - the pointer to the head of the linked list 	 */
/*			   is returned - it may have been changed.	 */
/*************************************************************************/
struct entry *get_alias (alias_lists, name, replace, global, alias_num)
struct entry *alias_lists;
char *name;
int replace;
int global;
int alias_num;
{
    struct entry *ret_alias;
    struct entry *current_alias;

    int count = 0;

    current_alias = alias_lists;
    if (alias_lists != (struct entry *)(NULL)) {        /* Search for alias */
        while ((alias_lists != (struct entry *)NULL) && /* name.	    */
	      (uleq(trimcpy (alias_lists -> words), name) == 0))
	       alias_lists = alias_lists -> next;
	
        if (alias_lists != (struct entry *)NULL) {  /* If it's found return */
	    if (replace)			    /* pointer to it.	    */
	       while (alias_lists -> address_list[count] != NULLCP)
		    alias_lists -> address_list[count] = NULLCP; 
	    strip_spaces (alias_lists);  /* Remove white spcs from addresses */
	    return (alias_lists);
        }				/* Alias name wasn't found : */
    }	
    if (global && (alias_num > 0) && name && *name) /* If global flag set,  */
 	return ((struct entry *)NULL);		    /* and an occurrence of */
						    /* alias name has been  */
						    /* found, return NULL   */

/* Make up a new structure for alias, and store alias name in structure. */

    if ((ret_alias = MAKESTRUCT(entry)) == (struct entry *)NULL)
	adios (NULLCP, "unable to allocate storage");
    ret_alias -> words = getcpy (name);
    if ((ret_alias -> address_list = MAKEARGV (1)) == (char **)NULL)
	adios (NULLCP, "unable to allocate storage");
    ret_alias -> address_list[0] = NULLCP;
    ret_alias -> next = (struct entry *)NULL;

    if (current_alias == (struct entry *)NULL)
	alias_lists = ret_alias;
    else {
   	while (current_alias -> next != (struct entry *)NULL)
	    current_alias = current_alias -> next;
	current_alias -> next = ret_alias;
    }
    return (ret_alias);
}

/******************************************************************************/
/* add_alias_entry adds the address str into the appropriate alias structure. */
/* Inputs: alias_ent - a pointer to structure storing current alias info.     */
/* 	   str - address to be added to alias.				      */
/*	   query - flag specifying if user is asked interactively whether to  */
/*		   add an address to the alias.				      */
/* Outputs: alias_ent - the new addresses will be added to the structure.     */
/******************************************************************************/
static void add_alias_entry(alias_ent, str, query)
struct entry *alias_ent;
char *str;
int query;
{
    int count = 0,
	reply = OK;

    while (alias_ent -> address_list[count] != NULL)  /* Count the number of */
        count++; 				      /* addresses in alias. */

    if (query)					      /* If query set, then */
        reply = ask (alias_ent -> words, str);	      /* query the user.    */

/* Reallocate the storage space and add the address 'str' to the alias. */
    if (reply == OK) {
        if ((alias_ent -> address_list = (char **)realloc(alias_ent -> 
            address_list, (count + 2) * sizeof (char *))) == (char **)NULL)
	    adios (NULLCP, "unable to allocate storage");
        alias_ent -> address_list[count + 1] = NULLCP;
	alias_ent -> address_list[count] = getcpy (str);
    }
}

/***************************************************************************/
/* ask - this outputs an address on stderr, and asks the user whether s/he */
/* wishes to add this address to the alias.				   */
/* Inputs: name - the alias name being appended to.			   */
/*	   str - the address being appended.				   */
/* Outputs: either OK (0) or NOTOK (-1).				   */
/***************************************************************************/
int ask (name, str)
char *name;
char *str;
{
    (void) fprintf(stderr, "%s\n", str);
    (void) fprintf(stderr, "Add this address to alias %s (y/n) ?", name);
    return (getanswer ("") ? OK : NOTOK);
}

/***************************************************************************/
/* compress_alias - if an address exists in the alias more than once, then */
/* only include it once.						   */
/* Inputs: new_alias - a pointer to the structure containing new alias.	   */
/* Outputs: new_alias is changed.					   */
/***************************************************************************/
static void compress_alias (new_alias)
struct entry *new_alias;
{
    char *str;
    int count,
	num_entries = 0,
	curr;

    while (new_alias -> address_list[num_entries] != NULLCP) /* Count no of  */
	num_entries++;					     /* addresses in */
							     /* alias.	     */
    for (curr = 1; curr < num_entries - 1; curr++) {
        str = new_alias -> address_list[curr-1];      /* If address exists   */
	while (*str == ' ' || *str == '\t')	      /* more than once then */
	    str++;				      /* replace all but 1st */
        for (count = curr; count < num_entries; count++) 	/* with '\0' */
	    if (uleq (str, new_alias -> address_list[count]))
	        new_alias -> address_list[count][0] = '\0';
    }
}

int continuation_line (str)
char *str;
{
    char *p;

    p = str + strlen (str) - 1;
    while (p >= str && isspace (*p))
	p--;

    return (p >= str && *p == '\\' ? 1 : 0);
}

strip_spaces (alias_list)
struct entry *alias_list;
{
    int i;

    char *p;

    if (alias_list != (struct entry *)NULL) {
	alias_list -> words = trimcpy (alias_list -> words);
	for (i = 0; alias_list -> address_list[i] != '\0'; i++) {
	    alias_list -> address_list[i] = 
			  trimcpy (alias_list -> address_list[i]);
	    while (alias_list -> address_list[i][0] == '\\') {
		alias_list -> address_list[i]++;
	        alias_list -> address_list[i] = 
			  trimcpy (alias_list -> address_list[i]);
	    }
	    if (((p = rindex(alias_list -> address_list[i], '\\')) != NULLCP) &&
		(*(p+1) == '\0'))
		*p = '\0';
	}
    }
}
