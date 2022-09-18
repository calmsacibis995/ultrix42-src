#ifndef lint
static char *sccsid_h = "@(#)globals.h	4.6      ULTRIX 	3/13/91";
#endif
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
n ************************************************************************/

/**********************************************************************
*
*    MODIFICATION HISTORY
*
* 03-Mar-91 - Adrian Thoms
*	Got rid of harmless (but untidy) extra : in printserver template
*	Put printserver help back in synch with template
*	TO BE DONE: get the templates into printcap format and avoid
*	the duplication in the help messages!!!!
*
* 25-Feb-91	Adrian Thoms
*	Fixed numerous spelling errors and corrected trademark usage.
*
* 04-Oct-90 - Adrian Thoms (thoms@wessex)
*	printserver template fix: added pp field for lpr -p support
*
* 13 Nov 89	Adrian Thoms (for Daren Seymour)
*	Changed default value for uv
*	
*    07-Jan-89  David Gray (gray)
*    Modified help message for "sd", "af", and "lf" to reflect changes
*    made to the way the files are created.
*    Modified to "xs" help message, bits to set are represented in octal
*    instead of hex. Since all other references are made in octal, this
*    makes much more sense.
*
*    November 11, 1988
*    by David J. Gray
*
*    Changed default entries for la75 and ln03r     
*
*    July 13, 1988
*    by: David J. Gray
*
*    Changed the wording of some of the help messages. There were
*    problems due to references to non-existent documents.
*
*    Added the LA120 to the printers available list, and the LP29
*    to the same list.
*
*    May 21, 1988
*    by: David J. Gray
*
*    Changed some of the default values for printers so that the
*    printers will work with factory settings.
*    ie., 8bit no parity and either 4800 or 9600 baud.
*
*    December 7, 1987  (Pearl Harbor Day - 12/7/41)
*
*    By: David J. Gray
*    Modified help messages to provide more verbose explanations to 
*    hopefully provide more useful information.
*    -> better explanation on printcap parameters
*    -> short description on each printer
*    -> more detailed information on use
*
*  8-Jul-88 Dave Maxwell (EUEG)
*    Added entries/help info for new postscript capabilities:
*	ct - connection typ
*	ps - postscript/non postscript mode
*	uv - Ultrix version
*	Da - Data type
*	Dl - Device Control Module file
*	It - Input tray
*	Lf - Layup to postscript translator
*	Lu - layup Definition file
*	Ml - Message log
*	Nu - Number up
*	Or - Orientation
*	Ot - Output tray
*	Ps - Page size
*	Sd - Sheet Size default
*	Si - Sides
*	Ss - Sheet size
*	Ul - Upper page limit
*	Xf - translator dispatch program
*    
*   Added lps40 template to printer list
*    
*  20-Jul-88 Dave Maxwell (EUEG)
*	changed lps40 to printserver
*
*************************************************************************/

/***************************************
*  global variables for lprsetup program
***************************************/

char    progname[] = "lprsetup";/* name of program 			*/
char    pnum[LEN];		/* printer number we are working on	*/
char    pname[LEN];		/* printer name		 		*/
char	longname[LEN];		/* printer name and synonyms		*/
char    ptype[LEN];		/* printer type		 		*/
char    symbolname[LEN];	/* symbol just read in; this needs to be*/
				/* large since is also used to hold the	*/
				/* value of the new symbol. 		*/
char    printertype[LEN];       /* name of printer type to get help on  */
	/* was char bp[1024]; */
char	jhp[1024];
char    *bp = jhp;			 /* return from tgetent	 		*/
char	oldfilter[LEN];			 /* print filter before modify		*/
char    printercomments[LINES][COLUMNS];  /* to store comments for printcap file  */
int     numcomments;		     /* actual number of comments	    */

/*
 * This structure holds symbols from the PRINTCAP file.
 * The used flag indicates whether the symbol is in
 * use or not, if so, it changes from NO to YES.
 */

struct table    tab[] =
{
    "af", "/usr/adm/lpacct", STR, NO, 0, 0,
    "br", "none", INT, NO, 0, 0,
    "cf", "none", STR, NO, 0, 0,
    "ct", "none", STR, NO, 0, 0,
    "df", "none", STR, NO, 0, 0,
    "dn", "/usr/lib/lpd", STR, NO, 0, 0,
    "du", "0", INT, NO, 0, 0,
    "fc", "none", INT, NO, 0, 0,
    "ff", "\\f", STR, NO, 0, 0,
    "fo", "off", BOOL, NO, 0, 0,
    "fs", "none", INT, NO, 0, 0,
    "gf", "none", STR, NO, 0, 0,
    "ic", "off", BOOL, NO, 0, 0,
    "if", "none", STR, NO, 0, 0,
    "lf", "/usr/adm/lperrs", STR, NO, 0, 0,
    "lo", "lock", STR, NO, 0, 0,
    "lp", "/dev/lp", STR, NO, 0, 0,
    "mc", "20", INT, NO, 0, 0,
    "mx", "1000", INT, NO, 0, 0,
    "nc", "off", BOOL, NO, 0, 0,
    "nf", "none", STR, NO, 0, 0,
    "of", "none",  STR, NO, 0, 0,
    "op", "none", STR, NO, 0, 0,
    "os", "none", STR, NO, 0, 0,
    "pl", "66", INT, NO, 0, 0,
    "pp", "usr/lib/lpdfilters/ln01pp", STR, NO, 0, 0,
    "ps", "non_PS", STR, NO, 0, 0,
    "pw", "132", INT, NO, 0, 0,
    "px", "none", INT, NO, 0, 0,
    "py", "none", INT, NO, 0, 0,
    "rf", "none", STR, NO, 0, 0,
    "rm", "none", STR, NO, 0, 0,
    "rp", "none", STR, NO, 0, 0,
    "rs", "off", BOOL, NO, 0, 0,
    "rw", "off", BOOL, NO, 0, 0,
    "sb", "off", BOOL, NO, 0, 0,
    "sc", "off", BOOL, NO, 0, 0,
    "sd", "/usr/spool/lpd", STR, NO, 0, 0,
    "sf", "off", BOOL, NO, 0, 0,
    "sh", "off", BOOL, NO, 0, 0,
    "st", "none", STR, NO, 0, 0,
    "tf", "none", STR, NO, 0, 0,
    "tr", "none", STR, NO, 0, 0,
    "ts", "none", STR, NO, 0, 0,
    "uv", "4.0", STR, NO, 0, 0,
    "vf", "none", STR, NO, 0, 0,
    "xc", "none", INT, NO, 0, 0,
    "xf", "none", STR, NO, 0, 0,
    "xs", "none", INT, NO, 0, 0,
    "Da", "none", STR, NO, 0, 0,
    "Dl", "/usr/lib/lpdfilters/dcl.a", STR, NO, 0, 0,
    "It", "none", STR, NO, 0, 0,
    "Lf", "/usr/lib/lpdfilters/layup", STR, NO, 0, 0,
    "Lu", "none", STR, NO, 0, 0,
    "Ml", "none", STR, NO, 0, 0,
    "Nu", "none", INT, NO, 0, 0,
    "Or", "portrait", STR, NO, 0, 0,
    "Ot", "none", STR, NO, 0, 0,
    "Ps", "none", STR, NO, 0, 0,
    "Sd", "none", STR, NO, 0, 0,
    "Si", "none", STR, NO, 0, 0,
    "Ss", "none", STR, NO, 0, 0,
    "Ul", "none", INT, NO, 0, 0,
    "Xf", "xlator_call", STR, NO, 0, 0,
    0, 0, 0, 0, 0, 0
};

char h_af[] =
{"\n\
The 'af' parameter is the name of the accounting file used to		\n\
keep track of the number of pages printed by each user for each		\n\
printer.  The name of the accounting file should be unique for		\n\
each printer on your system. The information stored in the		\n\
accounting file is used by the pac(8) program. Note this file		\n\
must be owned by `daemon'. It should be if it was created using		\n\
this script. The `af' parameter is not applicable for remote 		\n\
printer entries.							\n\
\n\
When the accounting file is created, intermediate directories  \n\
will be created as neccessary. \n\
"};

char h_br[] =
{"\n\
The 'br' parameter specifies the baud rate for the printer.		\n\
The baud rate is dependent upon the printer hardware.			\n\
Consult your printer hardware manual for the correct baud rate.		\n\
The 'br' parameter is only applicable for tty devices, ie. serial.      \n\
This parameter has no effect on printers connected to the console       \n\
port or to printers connected on a parallel port.		       \n\
"};

char h_cf[] =
{"\n\
The 'cf' parameter specifies the output filter for the		   \n\
cifplot data filter.						     \n\
\n\
Printer filter entries can either be an executable program	      \n\
or a single line which is interpreted by the shell.		    \n\
"};

char h_ct[] =
{"\n\
The 'ct' parameter specifies the type of connection to the printer.       \n\
Choices are dev, lat, remote, and network. 				\n\
"};

char h_df[] =
{"\n\
The 'df' parameter specifies the output filter for the TeX data 	\n\
filter (DVI format).						    \n\
\n\
Printer filter entries can either be an executable program	      \n\
or a single line which is interpreted by the shell.		    \n\
"};

char h_dn[] =
{"\n\
The 'dn' parameter specifies the name of the daemon program to		\n\
invoke each time a print request is made to the printer.  The		\n\
default daemon name is '/usr/lib/lpd', and should not be		\n\
changed.  The 'dn' parameter is available here so that the		\n\
system may support multiple line printer daemons.			\n\
"};

char h_du[] =
{"\n\
The 'du' parameter specifies the daemon UID used by the printer 	\n\
spooler programs.  The default value, (0) should not be changed.	\n\
The 'du' parameter is available here so that printer daemons,	   \n\
other than /usr/lib/lpd, may be used.				   \n\
"};

char h_fc[] =
{"\n\
The 'fc' parameter specifies which terminal flag bits to clear		\n\
when initializing the printer line.  Normally, all of the bits		\n\
should be cleared (fc=0177777 octal) before calling 'fs'.  Refer	\n\
to the discussion of 'sg_flags' in tty(4) of the Ultrix Reference	\n\
Pages, or see help message for the 'fs' parameter. \n\
"};

char h_ff[] =
{"\n\
The 'ff' parameter is the string to send as a form feed to the		\n\
printer.  The default value for this parameter is '\\f'.		\n\
"};

char h_fo[] =
{"\n\
The boolean parameter 'fo' specifies whether a form feed will		\n\
be printed when the device is first opened.  This is in addition	\n\
to the normal form feed which is printed by the driver when the		\n\
device is opened.  To suppress ALL printer induced form feeds,		\n\
use the 'sf' flag, in addition to the 'fo' flag.			\n\
"};

char h_fs[] =
{"\n\
The 'fs' parameter specifies which terminal flag bits to set		\n\
when initializing the printer line.  Normally, all of the bits		\n\
should be cleared (using fc=0177777 octal) and then 'fs' should be	\n\
used to set the specified bits.  Refer to the discussion of		\n\
'sg_flags' in tty(4) of the Ultrix Reference Pages.		\n\
A short discussion on each bit is listed below:	      	        \n\
									\n\
ALLDELAY     0177400    Delay algorithm selection		       \n\
BSDELAY      0100000    Select Backspace delays (not implemented)       \n\
BS0	     0							  \n\
BS1	     0100000						    \n\
VTDELAY      0040000    Select form-feed and vertical tab delays	\n\
FF0	     0							  \n\
FF1	     0100000						    \n\
CRDELAY      0030000    Select carriage-return delay		    \n\
CR0	     0							  \n\
CR1	     0010000						    \n\
CR2	     0020000						    \n\
CR3	     0030000						    \n\
TBDELAY      0060000    Select tab delays			       \n\
TAB0	     0							  \n\
TAB1	     0002000						    \n\
TAB2	     0004000						    \n\
XTABS	     0006000						    \n\
NLDELAY      0001400    Select new-lines delay			  \n\
NL0	     0							  \n\
NL1	     0000400						    \n\
NL2	     0001000						    \n\
NL3	     0001400						    \n\
EVENP	     0000200    Even parity allowed on input (most terminals)   \n\
ODDP	     0000100    Odd parity allowed on input		     \n\
RAW	     0000040    Raw mode: wake up on all characters; 8bit interface\n\
CRMOD	     0000020    Map CR into LF; echo LF or CR as CR-LF	  \n\
ECHO	     0000010    Echo (full duplex)			      \n\
LCASE	     0000004    Map upper case to lower on input		\n\
CBREAK       0000002    Return each character as soon as it is typed    \n\
TANDEM       0000001    Automatic flow control			  \n\
"};

char h_gf[] =
{"\n\
The 'gf' parameter specifies the graph data filter (plot(3X) format). 	\n\
\n\
Printer filter entries can either be an executable program	      \n\
or a single line which is interpreted by the shell.		    \n\
"};

char h_ic[] =
{"\n\
The 'ic' parameter is the driver that supports (nonstandard)	    \n\
ioctl to an independent printout.				       \n\
"};

char h_if[] =
{"\n\
The 'if' parameter is the name of a filter which does 			\n\
accounting.  Filters which can be used for if include:			\n\
\n\
	Filter name:	  		Description:			\n\
	------------	  		------------			\n\
	/usr/lib/lpdfilters/lpf	  	line printer filter 		\n\
					   (LP25, LP26, LP27, LP29)	\n\
					   (LG01, LA210, LQP02, LQP03)	\n\
	/usr/lib/lpdfilters/lqf	  	letter quality filter 		\n\
					   (LQP02, LQP03)		\n\
	/usr/lib/lpdfilters/ln01of   	LN01 Laser Printer filter 	\n\
	/usr/lib/lpdfilters/ln03of   	LN03 Laser Printer filter 	\n\
	/usr/lib/lpdfilters/ln03of   	LN03S Laser Printer filter 	\n\
	/usr/lib/lpdfilters/lcg01of   	LCG01 Color Printer		\n\
	/usr/lib/lpdfilters/lj250of   	LJ250 DEColorwriter filter 	\n\
\n\
If an accounting file is specified with the 'af' parameter then this    \n\
filter is used instead of the one specified by the 'of' parameter       \n\
to process the print job.					       \n\
\n\
Printer filter entries can either be an executable program	      \n\
or a single line which is interpreted by the shell.		    \n\
"};

char h_lf[] =
{"\n\
The 'lf' parameter is the logfile where errors are reported.		\n\
The default logfile, if one is not specified, is '/dev/console'.	\n\
If you have more than one printer on your system, you should give 	\n\
each logfile a unique name.						\n\
\n\
When the error log file is created, intermediate directories  \n\
will be created as neccessary. \n\
"};

char h_lo[] =
{"\n\
The 'lo' parameter is the name of the lock file used by the		\n\
printer daemon to control printing the jobs in each spooling		\n\
directory.  The default value, 'lock', should not be changed.		\n\
This parameter is available for use by 'other' printer daemons.	 \n\
"};

char h_lp[] =
{"\n\
The 'lp' parameter is the name of the special file to open for   	\n\
output.  The default value is '/dev/lp', which is for a parallel	\n\
printer. Additional parallel printers should have names like: lp1,      \n\
lp2, lp3, ..., etc.,. For serial printers, a terminal line should       \n\
have a name like /dev/tty01, /dev/tty02, ..., etc.		      \n\
"};

char h_mc[] =
{"\n\
The 'mc' parameter is the maximum number of copies allowed.		\n\
This parameter is used in conjunction with the lpr -#n option.	  \n\
See lpr(1) in the Ultrix Reference Pages.			     \n\
"};

char h_mx[] =
{"\n\
The 'mx' parameter specifies the maximum allowable filesize      	\n\
(in BUFSIZ blocks) printable by each user.  Specifying mx=0     	\n\
removes the filesize restriction entirely. If this parameter	    \n\
is not specified, the default value of 1000 blocks is assumed.	  \n\
The 'mx' parameter effects the size of the job on the machine	   \n\
that the lpr command was executed.				      \n\
"};

char h_nc[] =
{"\n\
The 'nc' parameter does not allow control characters in the	     \n\
output file.							    \n\
"};

char h_nf[] =
{"\n\
The 'nf' parameter specifies a ditroff filter. 				\n\
"};

char h_of[] =
{"\n\
The 'of' parameter specifies the output filter to be used with		\n\
the printer.  Output filters are used to filter text data to the	\n\
printer device when accounting is not used or when all text data	\n\
must be passed through a filter.  Filters which can be used for		\n\
output include:								\n\
\n\
	Filter name:	  		Description:			\n\
	------------	  		------------			\n\
	/usr/lib/lpdfilters/lpf	  	line printer filter 		\n\
					   (LP25, LP26, LP27, LP29 )	\n\
					   (LG01, LA210, LQP02, LQP03)	\n\
	/usr/lib/lpdfilters/lqf	  	letter quality filter 		\n\
					   (LQP02, LQP03)		\n\
	/usr/lib/lpdfilters/la75of      LA75 Dot Matrix Printer filter  \n\
	/usr/lib/lpdfilters/ln01of   	LN01 Laser Printer filter 	\n\
	/usr/lib/lpdfilters/ln03of   	LN03 Laser Printer filter 	\n\
	/usr/lib/lpdfilters/ln03of   	LN03S Laser Printer filter 	\n\
	/usr/lib/lpdfilters/lj250of     LJ250 Ink Jet Printer filter    \n\
	/usr/lib/lpdfilters/lcg01of     LCG01 Ink Jet Printer filter    \n\
	/usr/lib/lpdfilters/lg02of      LG02 Line Printer filter	\n\
	/usr/lib/lpdfilters/lg31of      LG31 Line Printer filter	\n\
	/usr/lib/lpdfilters/lpscomm     PrintServer network filter      \n\
\n\
This filter is used as the output filter unless an accounting file      \n\
is specified with the 'af' pareameter, in which case the filter	 \n\
specified with the 'if' parameter is used as the output filter	  \n\
as well as keeping track of accounting information.		     \n\
\n\
If you are setting up a PrintServer, the output filter requires         \n\
non-standard arguments.  Replace the dummy nodename with the node       \n\
name of your PrintServer followed by '%%U %%H %%J' (This causes your       \n\
Nodename, Username Hostname and Job-id to be passed to the filter.)    \n\
"};

char h_op[] =
{"\n\
The 'op' parameter specifies the object port on a LAT terminal server.	\n\
This parameter must be specified in uppercase.				\n\
"};

char h_os[] =
{"\n\
The 'os' parameter specifies the object service on a LAT server.	\n\
(NOT USED)							      \n\
"};

char h_pl[] =
{"\n\
The 'pl' parameter specifies the page length in lines.  The 		\n\
default page length is 66 lines.					\n\
"};

char h_pp[] =
{"\n\
The 'pp' parameter specifies the print command filter		   \n\
replacement.  Filters currently available include:			\n\
\n\
	Filter name:	  		Description:			\n\
	------------	  		------------			\n\
	/usr/lib/lpdfilters/ln01pp	LN01 Laser Printer filter	\n\
\n\
This filter replaces 'pr' as specified by the lpr -p command.	   \n\
See lpr(1) in the Ultrix Reference Pages.				     \n\
"};

char h_ps[] =
{"\n\
The 'ps' parameter specifies the mode in which the daemon runs.  It       \n\
selects between non-postscript and specific postscript printers.  Choices \n\
are non_PS and LPS.                                                       \n\
"};

char h_pw[] =
{"\n\
The 'pw' parameter specifies the page width in characters.  The		\n\
default page width is 132 characters, although a page width of		\n\
80 characters is more useful for letter quality printers, whose		\n\
standard paper size is 8 1/2\" x 11\".					\n\
"};

char h_px[] =
{"\n\
The 'px' parameter specifies the page width in pixels.		  \n\
"};

char h_py[] =
{"\n\
The 'py' parameter specifies the page length in pixels.		 \n\
"};

char h_rf[] =
{"\n\
The 'rf' parameter specifies the filter for printing FORTRAN style      \n\
text files.							     \n\
\n\
Printer filter entries can either be an executable program	      \n\
or a single line which is interpreted by the shell.		    \n\
"};

char h_rm[] =
{"\n\
The 'rm' parameter specifies the machine name for a remote printer.     \n\
This parameter should only appear in the printcap entries for remote    \n\
printers.							       \n\
\n\
In order for a remote machine to accept a printer request the name of   \n\
the machine making the request must appear in either the /etc/hosts.equiv\n\
file or the /etc/hosts.lpd file on the remote system, ie., the system    \n\
that the printer resides on.					     \n\
\n\
A remote machine name in /etc/hosts.equiv allows root on the remote      \n\
machine to have root priviledges on the host. This is a little over      \n\
for just printing needs and is dangerous for security reasons.	   \n\
\n\
A remote machine name in the /etc/hosts.lpd file allows users on the     \n\
remote machine to use the printer on the host, however, root does not    \n\
have any priviledges.  A '*' on the first line of /etc/hosts.lpd allows   \n\
all remote systems to use its printers.				  \n\
"};

char h_rp[] =
{"\n\
The 'rp' parameter specifies the remote printer name argument.	  \n\
The name specified must be one of the recognized names for the	  \n\
printyer on the remote machine.					 \n\
\n\
In order for a remote machine to accept a printer request the name of   \n\
the machine making the request must appear in either the /etc/hosts.equiv\n\
file or the /etc/hosts.lpd file on the remote system, ie., the system    \n\
that the printer resides on.					     \n\
\n\
A remote machine name in /etc/hosts.equiv allows root on the remote      \n\
machine to have root priviledges on the host. This is a little over      \n\
for just printing needs and is dangerous for security reasons.	   \n\
\n\
A remote machine name in the /etc/hosts.lpd file allows users on the     \n\
remote machine to use the printer on the host, however, root does not    \n\
have any priviledges. A '*' on the first line of /etc/hosts.lpd allows   \n\
all remote systems to use its printers.				  \n\
"};

char h_rs[] =
{"\n\
The 'rs' parameter restricts the remote users to those with	     \n\
local accounts.							 \n\
"};

char h_rw[] =
{"\n\
The boolean parameter 'rw' specifies that the printer is to be		\n\
opened for both reading and writing.  Normally, the printer is		\n\
opened for writing only.						\n\
"};

char h_sb[] =
{"\n\
The boolean 'sb' parameter specifies a short banner consisting		  \n\
of one line only.						       \n\
"};

char h_sc[] =
{"\n\
The boolean 'sc' parameter suppresses multiple copies.  This is	 \n\
equivalent to setting the 'mc' parameter to 1.			  \n\
"};

char h_sd[] =
{"\n\
The 'sd' parameter specifies the spooling directory where files		\n\
are queued before they are printed.  Each spooling directory		\n\
should be unique. All printcap entries must specify a spooling	  \n\
directory, both local and remote. 					\n\
\n\
When the spooling directory is created, intermediate directories  \n\
will be created as neccessary. \n\
"};

char h_sf[] =
{"\n\
The boolean parameter 'sf' suppresses all printer induced form    	\n\
feeds, except those which are actually in the file.  The 'sf'		\n\
flag, in conjunction with 'sh', is useful when printing a letter	\n\
on a single sheet of stationery.					\n\
"};

char h_sh[] =
{"\n\
The boolean parameter 'sh' suppresses printing of the normal		\n\
burst page header.  This often saves paper, in addition to being	\n\
useful when printing a letter on a single sheet of stationary.		\n\
"};

char h_st[] =
{"\n\
The 'st' parameter specifies the status file name. The default name     \n\
is 'status'. The status file is located in the spooling directory.      \n\
The current status of the printer is written in this file.	      \n\
"};

char h_tf[] =
{"\n\
The 'tf' parameter specifies a troff data filter.		       \n\
"};

char h_tr[] =
{"\n\
The 'tr' parameter specifies a trailing string to print when		\n\
the spooling queue empties.  It is generally a series of form    	\n\
feeds, or sometimes an escape sequence, to reset the printer		\n\
to a known state.							\n\
"};

char h_ts[] =
{"\n\
The 'ts' parameter specifies a LAT terminal server node name.		\n\
This parameter must be specified in uppercase.				\n\
"};

char h_uv[] =
{"\n\
The 'uv' parameter specifies the Ultrix version and is used by the daemon \n\
to decide how to work out the connection type and whether to expand %   \n\
escapes.  There are two version arguments: 3.0 and 4.0.  If 3.0 is used   \n\
the daemon uses rm rm lp etc. to work out the connection in a backwardly  \n\
compatable way; if 4.0 is used, the connection type is taken from ct and  \n\
any % escapes are expanded.                                               \n\
"};

char h_vf[] =
{"\n\
The 'vf' parameter specifies a raster image filter.		     \n\
Filters currently available include:					\n\
\n\
	Filter name:	  		Description:			\n\
	------------	  		------------			\n\
	/usr/lib/lpdfilters/ln01vf	LN01 Laser Printer filter	\n\
\n\
Note that the following raster image filters are specified using	\n\
the 'if' or 'of'parameter:						\n\
	/usr/lib/lpdfilters/la75of      LA75 Dot Matrix Printer filter  \n\
	/usr/lib/lpdfilters/ln03of   	LN03S Laser Printer filter 	\n\
	/usr/lib/lpdfilters/lcg01of   	LCG01 Color Printer filter 	\n\
	/usr/lib/lpdfilters/lj250of   	LJ250 DEColorwriter filter 	\n\
	/usr/lib/lpdfilters/lg02of    	LG02 Line Printer filter   	\n\
	/usr/lib/lpdfilters/lg31of    	LG31 Line Printer filter   	\n\
\n\
Printer filter entries can either be an executable program	      \n\
or a single line which is interpreted by the shell.		    \n\
"};

char h_xc[] =
{"\n\
The 'xc' parameter specifies the local mode bits to clear		\n\
when the terminal line is first opened.  Refer to the			\n\
discussion of the local mode word in tty(4) of the Ultrix		\n\
Reference Pages.  xc#0177777 clears all bits. See help 			\n\
message for the 'xs' parameter.	      \n\
"};

char h_xf[] =
{"\n\
The 'xf' parameter specifies the pass-thru filter name.	 		\n\
This routine is used when output is already formatted for		\n\
printing and does not require special filtering.			\n\
\n\
Printer filter entries can either be an executable program	      \n\
or a single line which is interpreted by the shell.		    \n\
"};

char h_xs[] =
{"\n\
The 'xs' parameter specifies the local mode bits to set			\n\
when the terminal line is first opened.  Refer to the		 	\n\
discussion of the local mode word in tty(4) of the			\n\
Ultrix Reference Pages. Normally the bits are first 			\n\
cleared using the 'xc' parameter xc#0177777. A short          \n\
discussion on each of the local mode bits is presented below:	     \n\
\n\
LCRTBS     000001    Backspace on erase rather than echoing erase       \n\
LPRTERA    000002    Printing terminal erase mode		       \n\
LCRTERA    000004    Erase character echoes as backspace-space-backspace\n\
LTILDE     000010    Convert ~ to ` on output (for Hazeltine terminals) \n\
LMDMBUF    000020    Start/Stop output when carrier drops (unimplemented)\n\
LLITOUT    000040    Suppress output translations - 8 bit	       \n\
LTOSTOP    000100    Send SIGTOC fro background output		  \n\
LFLUSHO    000200    Output is being flushed			    \n\
LNOHANG    000400    Do not send hangup when carrier drops	      \n\
LETXACK    001000    Diablo style buffer hacking (unimplemented)	\n\
LCRTKIL    002000    BS-space-BS erase entire line on line kill	 \n\
LCTLECH    010000    Echo input control chars as ^X, delete as ^?       \n\
LPENDIN    020000    Retype pending input at next read or input char    \n\
LDECCTQ    040000    Only ^Q restores output after ^S, like DEC systems \n\
LNOFLSH    100000    Flush output on receipt of 'suspend character'     \n\
"};

char h_Da[] =
{"\n\
The 'Da' parameter specifies the default DATA TYPE used by the daemon.    \n\
Choices are: ansi, ascii, postscript, regis, and tek.  If a data type\n\
other than postscript is specified, a translator is invoked by the daemon \n\
to convert the files in the job to postscript.                            \n\
This parameter is only valid with postscript printers.                    \n\
"};

char h_Dl[] =
{"\n\
The 'Dl' parameter specifies the DEVICE CONTROL MODULE library file.      \n\
This parameter is only valid with postscript printers.                    \n\
"};

char h_It[] =
{"\n\
The 'It' parameter specifies the default INPUT TRAY.  Choices are bottom, \n\
lcit, middle and top.                                                     \n\
An error will occur if the specified INPUT TRAY is not available on the   \n\
printer.                                                                  \n\
This parameter is only valid with postscript printers.                    \n\
"};

char h_Lf[] =
{"\n\
The 'Lf' parameter specifies the LAYUP TO POSTSCRIPT translator program.  \n\
This parameter is only valid with postscript printers.                    \n\
"};

char h_Lu[] =
{"\n\
The 'Lu' parameter specifies the default LAYUP DEFINITION FILE.           \n\
This parameter is only valid with postscript printers.                    \n\
"};

char h_Ml[] =
{"\n\
The 'Ml' parameter specifies the default action to be taken with user     \n\
errors produced by the PrintServer.  Choices are keep and ignore.         \n\
This parameter is only valid with postscript printers.                    \n\
"};

char h_Nu[] =
{"\n\
The 'Nu' parameter specifies the default NUMBER UP value (ie number of    \n\
pages per sheet).  The value must be in the range [1-100].                \n\
This parameter is only valid with postscript printers.                    \n\
"};

char h_Or[] =
{"\n\
The 'Or' parameter specifies the default ORIENTATION.  Choices are        \n\
portrait and landscape.                                                   \n\
This parameter is only valid with postscript printers.                    \n\
"};

char h_Ot[] =
{"\n\
The 'Ot' parameter specifies the default OUTPUT TRAY. Choices are face-up,\n\
lcos, lower, side, top and upper.                                         \n\
An error will occur if the specified OUTPUT TRAY is not available on the  \n\
printer.                                                                  \n\
This parameter is only valid with postscript printers.                    \n\
"};

char h_Ps[] =
{"\n\
The 'Ps' parameter specifies the default PAGE SIZE.  Choices are a,       \n\
letter, a3, a4, a5, b, ledger, b4, b5, executive and legal.      \n\
An error will occur if the specified PAGE SIZE is not available on the    \n\
printer.                                                                  \n\
This parameter is only valid with postscript printers.                    \n\
"};

char h_Sd[] =
{"\n\
The 'Sd' parameter specifies the default SHEET SIZE value.  It differs    \n\
from the Ss parameter in that if the specified sheet size is not available\n\
then no error will occur and the job will be printed on whatever sheet    \n\
size is available.  This parameter is overriden by the Ss parameter. If Sd\n\
is not specified, the translators assume a-sized paper so it              \n\
should always be used in countries where the standard paper size is other \n\
than a.  Choices are a, letter,a3, a4, a5, b, ledger, b4, b5, executive  \n\
and legal.                                                                \n\
This parameter is only valid with postscript printers.                    \n\
"};

char h_Si[] =
{"\n\
The 'Si' parameter specifies the default SIDES option.  Choices are 1,    \n\
one_sided_duplex, 2, two_sided_duplex, tumble, two_sided_tumble,          \n\
one_sided_duplex, one_sided_tumble, two_sided_simplex.                    \n\
An error will occur if the specified SIDES option is not available on the \n\
printer.                                                                  \n\
This parameter is only valid with postscript printers.                    \n\
"};

char h_Ss[] =
{"\n\
The 'Ss' parameter specifies the default SHEET SIZE.  Choices are a,      \n\
letter,a3, a4, a5, b, ledger, b4, b5, executive and legal.                \n\
An error will occur if the specified SHEET SIZE is not available on the   \n\
printer.                                                                  \n\
This parameter is only valid with postscript printers.                    \n\
"};

char h_Ul[] =
{"\n\
The 'Ul' parameter specifies the default UPPER PAGE LIMIT value.  The     \n\
must be in the range [1-10000]                                            \n\
This parameter is only valid with postscript printers.                    \n\
"};

char h_Xf[] =
{"\n\
The 'Xf' parameter specifies the translator dispatch program.             \n\
This parameter is only valid with postscript printers.                    \n\
"};

/************************************************
*  This structure holds the correct printcap
*  values for various printers, other values
*  -> default.  The first member of the structure
*  holds the printer name.
*************************************************/
struct 
{
    char *name;
    struct nameval entry[80];
} printer[] = 
{
    /* LA50 - Serial */
    {
    	"la50",
	"uv", "4.0",
	"br", "4800",
	"fc", "0177777",
	"fs", "023",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/lpf",
	"pl", "66",
	"pw", "80",
	"xc", "0177777",
	"xs", "044000",
	"xf", "/usr/lib/lpdfilters/xf",
	0, 0
    },
    /* LA75 - Serial */
    {
    	"la75",
	"uv", "4.0",
	"br", "4800",
	"fc", "0177777",
	"fs", "023",
	"if", "/usr/lib/lpdfilters/la75of",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/la75of",
	"pl", "66",
	"pw", "80",
	"xc", "0177777",
	"xs", "044000",
	"xf", "/usr/lib/lpdfilters/xf",
	0, 0
    },
    /* LA100 - Serial */
    {
    	"la100",
	"uv", "4.0",
	"br", "4800",
	"fc", "0177777",
	"fs", "023",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/lpf",
	"pl", "66",
	"pw", "80",
	"xc", "0177777",
	"xs", "044000",
	"xf", "/usr/lib/lpdfilters/xf",
	0, 0
    },
    /* LA120 - Serial */
    {
    	"la120",
	"uv", "4.0",
	"br", "4800",
	"fc", "0177777",
	"fs", "023",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/lpf",
	"pl", "66",
	"pw", "80",
	"xc", "0177777",
	"xs", "044000",
	"xf", "/usr/lib/lpdfilters/xf",
	0, 0
    },
    /* LA210 - Serial */
    {
    	"la210",
	"uv", "4.0",
	"br", "4800",
	"fc", "0177777",
	"fs", "023",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/lqf",
	"pl", "66",
	"pw", "80",
	"xc", "0177777",
	"xs", "044000",
	"xf", "/usr/lib/lpdfilters/xf",
	0, 0
    },
    /* LCG01 - Serial */
    {
    	"lcg01",
	"uv", "4.0",
	"br", "9600",
	"fc", "0177777",
	"fs", "03",
	"mx", "0",
	"mc", "20",
	"if", "/usr/lib/lpdfilters/lcg01of",
	"xf", "/usr/lib/lpdfilters/xf",
	"pl", "66",
	"pw", "80",
	"rw", "on",
	"sh", "on",
	"xc", "0177777",
	"xs", "044000",
	0, 0
    },
    /* LG01 - Parallel */
    {
    	"lg01",
	"lp", "/dev/lp",
	"mx", "0",
	"pl", "66",
	"pw", "80",
	"of", "/usr/lib/lpdfilters/lpf",
	"xf", "/usr/lib/lpdfilters/xf",
	0, 0
    },
    /* LG02 - Parallel or Serial*/
    {
	"lg02",
	"uv", "4.0",
        "br", "9600",
	"fc", "0177777",
	"fs", "03",
	"if", "/usr/lib/lpdfilters/lg02of",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/lg02of",
	"pl", "66",
	"pw", "80",
	"xc", "0177777",
	"xs", "044000",
	"xf", "/usr/lib/lpdfilters/xf",
	0, 0
    },
    /* LG31 - Serial */
    {
	"lg31",
	"uv", "4.0",
        "br", "9600",
	"fc", "0177777",
	"fs", "03",
	"if", "/usr/lib/lpdfilters/lg31of",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/lg31of",
	"pl", "66",
	"pw", "80",
	"xc", "0177777",
	"xs", "044000",
	"xf", "/usr/lib/lpdfilters/xf",
	0, 0
    },
    /* LJ250 - Serial */
    {
    	"lj250",
	"uv", "4.0",
	"br", "4800",
	"fc", "0177777",
	"fs", "03",
	"mc", "20",
	"mx", "0",
	"if", "/usr/lib/lpdfilters/lj250of",
	"xf", "/usr/lib/lpdfilters/xf",
	"pl", "66",
	"pw", "80",
	"rw", "on",
	"sh", "on",
	"xc", "0177777",
	"xs", "044000",
	0, 0
    },
    /* LN01 - Parallel */
    {
    	"ln01",
	"fc", "0177777",
	"fs", "023",
	"lp", "/dev/lp",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/ln01of",
	"pl", "66",
	"pp", "/usr/lib/lpdfilters/ln01pp",
	"pw", "80",
	0, 0
    },
    /* LN01S - Parallel */
    {
        "ln01s",
	"fc", "0177777",
	"fs", "023",
	"lp", "/dev/lp",
	"of", "/usr/lib/lpdfilters/ln01of",
	"mx", "0",
	"pl", "66",
	"pp", "/usr/lib/lpdfilters/ln01pp",
	"pw", "80",
	"vf", "/usr/lib/lpdfilters/ln01vf",
	0, 0
    },
    /* LN03 - Serial */
    {
    	"ln03",
	"uv", "4.0",
	"br", "4800",
	"fc", "0177777",
	"fs", "03",
	"if", "/usr/lib/lpdfilters/ln03of",
	"of", "/usr/lib/lpdfilters/ln03of",
	"xf", "/usr/lib/lpdfilters/xf",
	"mx", "0",
	"pl", "66",
	"pw", "80",
	"rw", "on",
	"xc", "0177777",
	"xs", "044000",
	0, 0
    },
    /* LN03S - Serial */
    {
    	"ln03s",
	"uv", "4.0",
	"br", "4800",
	"fc", "0177777",
	"fs", "03",
	"if", "/usr/lib/lpdfilters/ln03of",
	"mc", "20",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/ln03of",
	"xf", "/usr/lib/lpdfilters/xf",
	"pl", "66",
	"pw", "80",
	"rw", "on",
	"xc", "0177777",
	"xs", "044000",
	0, 0
    },
    /* LN03R - Serial */
    {
    	"ln03r",
	"uv", "4.0",
	"br", "9600",
	"fc", "0177777",
	"fs", "03",
	"if", "/usr/lib/lpdfilters/ln03rof",
	"mc", "20",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/ln03rof",
	"xf", "/usr/lib/lpdfilters/xf",
	"pl", "66",
	"pw", "80",
	"rw", "on",
	"xc", "0177777",
	"xs", "044000",
	0, 0
    },
    /* LP25 - Parallel */
    {
    	"lp25",
	"lp", "/dev/lp",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/lpf",
	0, 0
    },
    /* LP26 - Parallel */
    {
    	"lp26",
	"lp", "/dev/lp",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/lpf",
	0, 0
    },
    /* LP27 - Parallel */
    {
    	"lp27",
	"lp", "/dev/lp",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/lpf",
	0, 0
    },
    /* LP29 - Parallel */
    {
    	"lp29",
	"lp", "/dev/lp",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/lpf",
	0, 0
    },
    /* LQP02 - Serial */
    {
    	"lqp02",
	"uv", "4.0",
	"br", "4800",
	"fc", "0177777",
	"fs", "023",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/lqf",
	"pl", "66",
	"pw", "80",
	"rw", "on",
	0, 0
    },
    /* LQP03 - Serial */
    {
    	"lqp03",
	"uv", "4.0",
	"br", "4800",
	"fc", "0177777",
	"fs", "023",
	"mx", "0",
	"of", "/usr/lib/lpdfilters/lqf",
	"pl", "66",
	"pw", "80",
	"rw", "on",
	0, 0
    },
    /* LVP16 - Serial */
    {
	"lvp16",
	"uv", "4.0",
	"br", "9600",
	"fc", "0177777",
	"fs", "03",
	"mx", "0",
	"if", "/usr/lib/lpdfilters/xf",
	"rw", "on",
	"sh", "on",
	"xf", "/usr/lib/lpdfilters/xf",
	 0,    0
    },

    /* PRINTSERVER - Network */
    {
        "printserver",
        "uv", "4.0",
        "ct", "network",
	"pp", "pr_call %x %D %F %O %W %L %T",
        "ps", "LPS",
        "Dl", "/usr/lib/lpdfilters/lps_v3.a",
        "Da", "ascii",
        "Sd", "a",
        "of", "/usr/lib/lpdfilters/lpscomm NODE  %U %H %J",
         0,    0
    },

    /* REMOTE SYSTEM FOR PRINTERS */
    {
	"remote",
	0, 0
    },
    /* UNKNOWN PRINTERS */
    {
	"unknown",
	0, 0
    },
    /* NULL */
    {
	0, 0, 0
    }
};

/*************************************************
*  This structure maps a command character string
*  against an integer code
*************************************************/
struct cmdtyp   cmdtyp[] =
{
    "?", HELP,
    "add", ADD,
    "delete", DELETE,
    "exit", QUIT,
    "help", HELP,
    "list", LIST,
    "modify", MODIFY,
    "no", NO,
    "print", PRINT,
    "quit", QUIT,
    "view", VIEW,
    "yes", YES,
    "printer?", PRINTER_INFO,
    "none", NO,
    0, 0
};
/******************************************
*  the following arrays hold the help text
*  associated with each function and are
*  referred to specifically by the function.
******************************************/
char h_help[] =
{"\n\
This program helps you to set up your printers.  			\n\
									\n\
Select a command from the Main Command Menu by typing one of the	\n\
commands listed (or just the first letter of the command)		\n\
and pressing RETURN.  You will then be prompted to enter		\n\
the number of the printer (for example 0 or 1).  Then you will be	\n\
prompted for other responses, depending on the command. 		\n\
									\n\
For all prompts, a default response value is given in \[ \].  To use	\n\
the default, just press RETURN.  You can always enter '?' for help.	\n\
									\n\
Some knowledge of the printer and its installation is required 		\n\
in order to correctly use this program.  Refer to the passage		\n\
in the System Management Guide for general requirements.		\n\
Chapter 4 - The Print System					    \n\
									\n\
Most of the setup activities require you to enter values for symbols	\n\
in the file /etc/printcap.  Refer to printcap(5) in the			\n\
Ultrix Reference Pages for a summary of the possible symbols	\n\
for /etc/printcap.  This program provides a detailed explanation	\n\
of each printcap symbol in response to '?' after a symbol name		\n\
has been specified and a value for the symbol is prompted.		\n\
									\n\
The Available commands are:					     \n\
									\n\
    add      -> add a new printcap entry (local and remote)	     \n\
    delete   -> delete a printcap entry (local and remote)	      \n\
    modify   -> modify a printcap entry (local and remote)	      \n\
    view     -> view the contents of the printcap file		  \n\
    exit     -> exit or quit lprsetup				   \n\
    quit     -> quit or exit lprsetup				   \n\
    ?	     -> ask for help  (at any prompt)			   \n\
    help     -> ask for help  (at any prompt)			   \n\
									\n\
Commands may be abbreviated to their first letter.		      \n\
Any command can be quickly exited by typing a Control-D		\n\
\n\
"};

char h_doadd[] =
{"\n\
Enter a printer name, ie., the name by which you want to identify       \n\
the printer through the lpr command, example: lpr -Pprintername.	\n\
Lprsetup uses an internal numbering scheme from 0 to 99. The	    \n\
next available number is the default name, you may chose the	    \n\
default by simply pressing RETURN or enter any other alphanumeric	\n\
name that is appropriate. Lprsetup will always assign at least	  \n\
two printer synonyms: the default number and 'lp default number', plus  \n\
any others you specify, both here and later when specifying synonyms.   \n\
If the default number was 1, the two names would be '1' and 'lp1'.      \n\
This printer could then be identified with 'lpr -P1' or 'lpr -Plp1'     \n\
If you only have one printer or are entering the first of many		\n\
printers, the first one will have a printer number of 0. This is	\n\
recognized as your system's default printer and will have an	    \n\
additional name of just 'lp'. This means if you use 'lpr' without       \n\
specifiying a specific printer this is the one it will use.	     \n\
"};

char h_dodel[] =
{"\n\
This section deletes an existing printer.  				\n\
Enter the name of the printer.  The name must be one of the printer's	\n\
synonyms.  The name of the printer that you enter must match 		\n\
that of a printer in the /etc/printcap file in order to delete it.	\n\
\n\
The entry is removed from the printcap file, the spooling direcory      \n\
is deleted, along with the accounting and error log files, if used.     \n\
"};

char h_domod[] =
{"\n\
This section modifies one or more existing printcap entries.  		\n\
Enter the name of the printer.  The name must be one of the printer's   \n\
synonyms.  The name of the printer that you enter must match 		\n\
that of a printer in the /etc/printcap file in order to modify it.	\n\
You will then be prompted to name the symbol in /etc/printcap which 	\n\
you wish to change.  You may instead enter 'p' to print all of the      \n\
current printcap parameters; 'l' to list all of the possible printcap    \n\
symbols and defaults;  or enter 'q' (for quit) to return to the  	\n\
main Command menu.      						\n\
"};

char h_symsel[] =
{"\n\
Enter the name of the printcap symbol you wish to modify.  Other	\n\
valid entries are:							\n\
	'q'     to quit (no more changes)				\n\
	'p'     to print the symbols you have specified so far		\n\
	'l'     to list all of the possible symbols and defaults	\n\
The names of the printcap symbols are:					\n\
\n\
"};

char h_synonym[] =
{"\n\
Enter alternate names for this printer.  Some examples include	\n\
'draft',  'letter', and 'LA-75 Companion Printer'.  If the name		\n\
contains blanks or tabs it is assumed to be last. You can enter as many \n\
alternate names	for a printer as you like, but the total length of 	\n\
the line containing the names must be less than 80 characters.  	\n\
After entering a synonym, you will be prompted again.  If you do not 	\n\
wish to enter any more synonyms, press RETURN to continue.		\n\
									\n\
Each synonym (including	the printer number) identifies the printer 	\n\
to the print system.  For example, if you enter the synonym 'draft'	\n\
for this printer, then the command 'lpr -Pdraft files ...' 		\n\
will print files on this printer.					\n\
"};

char h_type[] =
{"\n\
Printers are listed by type and only those supported by DIGITAL. 	\n\
These printers have some default values	already included in the 	\n\
setup program.  							\n\
									\n\
Other printers can be set up by using 'unknown' and then 		\n\
responding to the prompts, using values similar to those for 		\n\
supported printers.							\n\
									\n\
Responding 'remote' allows you to designate a remote system		\n\
for printing.  In this case, only four printcap entries are required:	\n\
rm (name of the remote system), rp (name of the printer on the		\n\
remote system), sd (pathname of the spooling directory on		\n\
this system, and lp (the local line printer device - which is always    \n\
null) the lp parameter must be present in order to print to a remote    \n\
printer.								\n\
									\n\
Responding 'printer?' allows you to enter a mode where more information \n\
can be requested for each printer type. In this mode you will be	\n\
prompted to enter the same printer types as above. Information about    \n\
the printer and the default printcap entries for that printer will be   \n\
displayed. Enter 'quit' to return to the prompt to select the printer   \n\
type being added.						       \n\
									\n\
When specifying printer type full command names and printer names       \n\
must be used.							   \n\
									\n\
The default printer type is 'unknown'.				  \n\
\n\
"};

char h_default[] =
{"\n\
Enter a new value, or press RETURN to use the default. 			\n\
"};

char h_addcmnts[] =
{"\n\
Add Comments allows you to add comments to the printcap file while      \n\
still running lprsetup. The comments will be insert directly above      \n\
the printcap entry in the printcap file. The default is not to enter    \n\
any comments so simply press RETURN when prompted if you do not want    \n\
to enter comments. Any other response will assume that you do. At the   \n\
'# ' prompt enter your comment, press RETURN at the '# ' prompt to      \n\
exit.								   \n\
\n\
"};

char h_printype[] =
{"\n\
Printer Type allows you to request more information on a specific       \n\
printer type. Specify one of the supported printers, 'remote', or	\n\
'unknown' at the prompt. Type 'quit' to exit this mode. More	    \n\
information about that printer is displayed, including the default      \n\
printcap settings.						      \n\
									\n\
The Supported printers are:					     \n\
\n\
"};

char isnotused[] =
{"feature is not used with 						\n\
the parallel line printer interface.  You would only			\n\
specify this symbol for a printer which is connected via		\n\
a serial terminal line. 						\n\
"};

/*****************************************************
* This structure maps a printer type character string
* against an integer code
******************************************************/

struct prnttyp    prnttyp[] =
{
     "?", PTHELP,
     "help", PTHELP,
     "quit", PTQUIT,
     "exit", PTQUIT,
     "la50", LA50,
     "la75", LA75,
     "la100", LA100,
     "la120", LA120,
     "la210", LA210,
     "lcg01", LCG01,
     "lg01", LG01,
     "lg02", LG02,
     "lg31", LG31,
     "lj250", LJ250,
     "ln01", LN01,
     "ln01s", LN01S,
     "ln03", LN03,
     "ln03s", LN03S,
     "ln03r", LN03R,
     "lp25", LP25,
     "lp26", LP26,
     "lp27", LP27,
     "lp29", LP29,
     "lqp02", LQP02,
     "lqp03", LQP03,
     "lvp16", LVP16,
     "printserver", PRINTSERVER,
     "remote", REMOTE,
     "unknown", NOTKNOWN,
      0, 0
};

/*****************************************************
* The following arrays contain help information
* about new printer supported by DIGITAL
******************************************************/

char h_la50 [] =
{"\n\
The desktop LA50 serial printer is compact, lightweight and is \n\
designed for use with Digital's personal computers and video \n\
terminals. A serial printer using dot-matrix technology, the LA50 \n\
Personal Printer can be used for applications that require both \n\
text and graphics output. Shown below are the default printcap \n\
parameters: \n\
\n\
         uv = 4.0  \n\
	 br = 4800  \n\
	 fc = 0177777  \n\
	 fs = 023  \n\
	 of = /usr/lib/lpdfilters/lpf  \n\
	 mx = 0  \n\
	 pl = 66  \n\
	 pw = 80  \n\
	 xc = 0177777  \n\
	 xs = 044000  \n\
	 xf = /usr/lib/lpdfilters/xf \n\
\n\
"};

char h_la75 [] =
{"\n\
The LA75 is a versatile and reliable high-speed dot-matrix \n\
serial printer that incorporates numerous state-of-the-art \n\
printing and user-oriented features. Designed specifically \n\
for office environments. The LA75 prints in any one of five \n\
modes - draft, memo, near-letter quality, letter quality, \n\
and bit-map graphics. Shown below are the default printcap \n\
parameters: \n\
\n\
         uv = 4.0  \n\
	 br = 4800  \n\
	 fc = 0177777  \n\
	 fs = 023  \n\
	 if = /usr/lib/lpdfilters/la75of  \n\
	 mx = 0  \n\
	 of = /usr/lib/lpdfilters/la75of  \n\
	 pl = 66  \n\
	 pw = 80  \n\
	 xc = 0177777  \n\
	 xs = 044000  \n\
	 xf = /usr/lib/lpdfilters/xf \n\
\n\
"};

char h_la100 [] =
{"\n\
\n\
The Letterwriter 100 (la100) is a multi-talented, wide carriage, \n\
tabletop serial printer/terminal. The LA100's features include \n\
multiple maximum speeds of 240 cps draft quality and 30 cps near \n\
letter quality, and a variety of dynamically selectable character \n\
fonts and bit map graphics. Shown below are the default printcap \n\
parameters: \n\
\n\
         uv = 4.0  \n\
	 br = 4800  \n\
	 fc = 0177777  \n\
	 fs = 023  \n\
	 mx = 0  \n\
	 of = /usr/lib/lpdfilters/lpf  \n\
	 pl = 66  \n\
	 pw = 80  \n\
	 xc = 0177777  \n\
	 xs = 044000  \n\
	 xf = /usr/lib/lpdfilters/xf \n\
\n\
"};

char h_la120 [] =
{"\n\
The DecWriter III (LA120) is a serial, dox-matrix, pedestal-mounted \n\
printer that combines bi-directional printing and a 1K character \n\
buffer to achieve a maximum throughput of 180 characters per second.\n\
Shown below are the default printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 br = 4800  \n\
	 fc = 0177777  \n\
	 fs = 023  \n\
	 mx = 0  \n\
	 of = /usr/lib/lpdfilters/lpf  \n\
	 pl = 66  \n\
	 pw = 80  \n\
	 xc = 0177777  \n\
	 xs = 044000  \n\
	 xf = /usr/lib/lpdfilters/xf \n\
\n\
"};

char h_la210[] =
{"\n\
The LA210 Letterprinter is a multimode, dot-matrix, serial desktop \n\
printer that is compatible with Digital computers, International \n\
Business Machines (IBM), and IBM-compatible personal computers. \n\
Shown below are the default printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 br = 4800  \n\
	 fc = 0177777  \n\
	 fs = 023  \n\
	 mx = 0  \n\
	 of = /usr/lib/lpdfilters/lqf  \n\
	 pl = 66  \n\
	 pw = 80  \n\
	 xc = 0177777  \n\
	 xs = 044000  \n\
	 xf = /usr/lib/lpdfilters/xf \n\
\n\
"};

char h_lcg01 [] =
{"\n\
The LCG01 is a color ink jet printer. It has a pallet of 216 colors \n\
and a resolution of 154 dots per inch. Shown below are the default \n\
printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 br = 9600  \n\
	 fc = 0177777  \n\
	 fs = 03  \n\
	 mx = 0  \n\
	 mc = 20  \n\
	 if = /usr/lib/lpdfilters/lcg01of  \n\
	 pl = 66  \n\
	 pw = 80  \n\
	 rw = on  \n\
	 sh = on  \n\
	 xc = 0177777  \n\
	 xs = 044000  \n\
	 xf = /usr/lib/lpdfilters/xf \n\
\n\
"};

char h_lj250 [] =
{"\n\
The LJ250 is a color ink jet printer designed for the desktop. It \n\
has a pallet of 256 colors and a resolution of 180 dots per inch. \n\
Shown below are the default printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 br = 4800  \n\
	 fc = 0177777  \n\
	 fs = 03  \n\
	 mc = 20  \n\
	 mx = 0  \n\
	 if = /usr/lib/lpdfilters/lj250of  \n\
	 pl = 66  \n\
	 pw = 80  \n\
	 rw = on  \n\
	 sh = on  \n\
	 xc = 0177777  \n\
	 xs = 044000  \n\
	 xf = /usr/lib/lpdfilters/xf \n\
\n\
"};

char h_lg01 [] =
{"\n\
The LG01 is a 600 line per minute parallel impact printer. It \n\
prints 600 lpm is draft mode and 280 lpm in correspondence mode. \n\
It can handle 7 & 8 bit character sets, the Digital multinational,\n\
ORC-A, and ORC-B character sets. Shown below are the default \n\
printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 lp = /dev/lp  \n\
	 mx = 0  \n\
	 pl = 66  \n\
	 pw = 80  \n\
	 of = /usr/lib/lpdfilters/lpf  \n\
	 xf = /usr/lib/lpdfilters/xf \n\
\n\
"};

char h_lg02 [] =
{"\n\
The LG02 represents a new standard of reliability for 600 lines \n\
per minute impact printers. It has both a serial and parallel \n\
interface. The LG02 can print in both a text mode and a \n\
plot mode. In text it prints draft quality at 600 lpm and 280 lpm \n\
correspondence quality. It can print in both portrait and landscape \n\
In plot mode, the LG02 can plot a vector, logo, barcode, block \n\
character, or sixel graphic in medium resolution. Shown below \n\
are the default printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 lp = /dev/lp  \n\
	 if = /usr/lib/lpdfilters/lg02of  \n\
	 mx = 0  \n\
	 of = /usr/lib/lpdfilters/lg02of  \n\
	 pl = 66  \n\
	 pw = 80  \n\
	 xf = /usr/lib/lpdfilters/xf \n\
\n\
"};

char h_lg31 [] =
{"\n\
The LG31 is a 300 lpm version of the LG02, however, with only \n\
a serial interface. It can print in both a text mode and a plot \n\
mode. Text others a variety of pitches and font sizes while plot \n\
offers vectors, logos, bar codes, block characters and the ability \n\
to print sixel data. Shown below are the default printcap parameters: \n\
\n\
         uv = 4.0  \n\
         br = 9600 \n\
	 fc = 0177777  \n\
	 fs = 023  \n\
	 if = /usr/lib/lpdfilters/lg31of  \n\
	 mx = 0  \n\
	 of = /usr/lib/lpdfilters/lg31of  \n\
	 pl = 66  \n\
	 pw = 80  \n\
	 xc = 0177777  \n\
	 xs = 044000  \n\
	 xf = /usr/lib/lpdfilters/xf \n\
\n\
"};

char h_ln01 [] =
{"\n\
The LN01 laser printer is a non-impact page printer that is ideal \n\
for applications where print quality and graphics are important. \n\
The LN01 prints one page at a time, up to a maximum of 12 pages \n\
per minute. The printing resolution is 300 X 300 dots per inch. \n\
Shown below are the default printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 fc = 0177777  \n\
	 fs = 023  \n\
	 lp = /dev/lp  \n\
	 mx = 0  \n\
	 of = /usr/lib/lpdfilters/ln01of  \n\
	 pl = 66  \n\
	 pp = /usr/lib/lpdfilters/ln01pp  \n\
	 pw = 80  \n\
\n\
"};

char h_ln01s [] =
{"\n\
The LN01S offers the same features as the LN01 plus full bit map \n\
graphics. The LN01S comes with 12 resident fonts, an external \n\
controller, full bit map graphics and Trektronix emulation. \n\
Shown below are the default printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 fc = 0177777  \n\
	 fs = 023  \n\
	 lp = /dev/lp  \n\
	 of = /usr/lib/lpdfilters/ln01of  \n\
	 mx = 0  \n\
	 pl = 66  \n\
	 pp = /usr/lib/lpdfilters/ln01pp  \n\
	 pw = 80  \n\
	 vf = /usr/lib/lpdfilters/ln01vf  \n\
\n\
"};

char h_ln03 [] =
{"\n\
The LN03 is a tabletop, non-impact laser printer. It prints at a \n\
maximum speed of 8 pages per minute (text) at a resolution of \n\
300 X 300 dots per inch. It can print sixel graphics with a \n\
resolution of 150 X 150 dots per inch. The LN03 can print in \n\
both portrait and landscape modes. It has a serial interface. \n\
Shown below are the default printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 br = 4800  \n\
	 fc = 0177777  \n\
	 fs = 03  \n\
	 if = /usr/lib/lpdfilters/ln03of  \n\
	 of = /usr/lib/lpdfilters/ln03of  \n\
	 mx = 0  \n\
	 pl = 66  \n\
	 pw = 80  \n\
	 xc = 0177777  \n\
	 xs = 044000  \n\
	 xf = /usr/lib/lpdfilters/xf \n\
\n\
"};

char h_ln03s [] =
{"\n\
The LN03S is a tabletop, non-impact laser printer. It prints at a \n\
maximum speed of 8 pages per minute (text) at a resolution of \n\
300 X 300 dots per inch. Plus full page bit map graphics at a \n\
resolution of 300 X 300 dots per inch. The LN03 can print in \n\
both portrait and landscape modes. It has a serial interface. \n\
Shown below are the default printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 br = 4800  \n\
	 fc = 0177777  \n\
	 fs = 03  \n\
	 if = /usr/lib/lpdfilters/ln03of  \n\
	 mc = 20  \n\
	 mx = 0  \n\
	 of = /usr/lib/lpdfilters/ln03of  \n\
	 pl = 66  \n\
	 pw = 80  \n\
	 rw = on  \n\
	 xc = 0177777  \n\
	 xs = 044000  \n\
	 xf = /usr/lib/lpdfilters/xf \n\
\n\
"};

char h_ln03r [] =
{"\n\
The LN03R is a PostScript, non-impact laser printer. PostScript \n\
(a trademark of Adobe) is a page description language that \n\
allows almost unlimited control of text and graphics. The LN03R \n\
prints at a rate of 8 pages per minute (text) and has a resolution \n\
of 300 X 300 dots per inch. The LN03R can print in both portrait \n\
and landscape modes as well as any other rotation that is desired. \n\
It has a serial interface. Shown below are default printcap \n\
parameters: \n\
\n\
         uv = 4.0  \n\
	 br = 9600  \n\
	 fc = 0177777  \n\
	 fs = 03  \n\
	 if = /usr/lib/lpdfilters/ln03rof  \n\
	 mc = 20  \n\
	 mx = 0  \n\
	 of = /usr/lib/lpdfilters/ln03rof  \n\
	 pl = 66  \n\
	 pw = 80  \n\
	 rw = on  \n\
	 xc = 0177777  \n\
	 xs = 044000  \n\
	 xf = /usr/lib/lpdfilters/xf \n\
\n\
"};

char h_lp25 [] =
{"\n\
The LP25 is a band printer with a parallel interface. It prints \n\
at 300 lines per minute with a 64 character ASCII set and 215 \n\
lines per minute with a 96 character ASCII set. Shown below are \n\
the default printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 lp = /dev/lp  \n\
	 mx = 0  \n\
	 of = /usr/lib/lpdfilters/lpf  \n\
\n\
"};

char h_lp26 [] =
{"\n\
The LP26 is a band printer with a parallel interface. It prints \n\
at 600 lines per minute with a 64 character ASCII set and 445 \n\
lines per minute with a 96 character ASCII set. Shown below are \n\
the default printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 lp = /dev/lp  \n\
	 mx = 0  \n\
	 of = /usr/lib/lpdfilters/lpf  \n\
\n\
"};

char h_lp27 [] =
{"\n\
The LP27 is a band printer with a parallel interface. It prints \n\
up tp 1200 lines per minute with a 64 character ASCII set and up to  \n\
800 lines per minute with a 96 character ASCII set. Shown below are \n\
the default printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 lp = /dev/lp  \n\
	 mx = 0  \n\
	 of = /usr/lib/lpdfilters/lpf  \n\
\n\
"};

char h_lp29 [] =
{"\n\
The LP29 is a band printer with a parallel interface. It prints \n\
up to 2000 lines per minute with a 64 character ASCII set and up to  \n\
1700 lines per minute with a 96 character ASCII set. Shown below are \n\
the default printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 lp = /dev/lp  \n\
	 mx = 0  \n\
	 of = /usr/lib/lpdfilters/lpf  \n\
\n\
"};

char h_lqp02 [] =
{"\n\
The LQP02 is a desktop, full-character impact printer. It uses \n\
daisywheel technology to produce printed copy much like that of \n\
your office typewriter. It is ideal for word processing \n\
applications. Shown below are the default printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 br = 4800  \n\
	 fc = 0177777  \n\
	 fs = 023  \n\
	 of = /usr/lib/lpdfilters/lqf  \n\
	 pl = 66  \n\
	 pw = 80  \n\
\n\
"};

char h_lqp03 [] =
{"\n\
The LQP03 is a compact, full-character, letter-quality desktop \n\
printer designed for office use or business applications. It \n\
offers an expanded character set on a single, 130-petal daisywheel. \n\
Shown below are the default printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 br = 4800  \n\
	 fc = 0177777  \n\
	 fs = 023  \n\
	 of = /usr/lib/lpdfilters/lqf  \n\
	 pl = 66  \n\
	 pw = 80  \n\
\n\
"};

char h_lvp16 [] =
{"\n\
The LVP16 is a six pen plotter that can produce high-quality \n\
color output on both paper and transparencies. It is ideally \n\
suited for the office environment. Shown below are the default \n\
printcap parameters: \n\
\n\
         uv = 4.0  \n\
	 br = 9600  \n\
	 fc = 0177777 \n\
	 fs = 03 \n\
	 if = usr/lib/lpdfilters/xf  \n\
	 mx = 0 \n\
	 rw = on \n\
         sh = on \n\
	 xf = usr/lib/lpdfilters/xf  \n\
\n\
"};

char h_printserver [] =
{"\n\
The PrintServer is a non-impact printer that uses a laser electrophotographic\n\
printing technique.  It uses PrintServer software that allows shared\n\
use by Ethernet connected computers.  The printer interprets data encoded\n\
in the PostScript page description language, giving it capabilities for \n\
both typesetting and graphic images.  The LPS40 PrintServer has a printing \n\
speed of up to 40 pages per minute. \n\
\n\
	uv = 4.0\n\
	ct = network\n\
	pp = pr_call %%x %%D %%F %%O %%W %%L %%T\n\
	ps = LPS\n\
	Dl = /usr/lib/lpdfilters/lps_v3.a\n\
	Da = ascii\n\
	Sd = a\n\
	of = /usr/lib/lpdfilters/lpscomm NODE %%U %%H %%J\n\
\n\
"};

char h_remote [] =
{"\n\
The REMOTE printer is any printer that is connected to a \n\
remote machine. Required information on the local system \n\
are printer synonyms, the name of the remote machine, the \n\
name of the remote printer, and a spooling directory on the \n\
local machine. \n\
\n\
"};

char h_unknown [] =
{"\n\
This is not a known printer, during the setup process you \n\
will be asked a series of questions similar to those for \n\
a known printer. \n\
\n\
"};

/***********************************
*  end of globals.h
***********************************/

