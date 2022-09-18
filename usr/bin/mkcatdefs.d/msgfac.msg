$
$ @OSF_COPYRIGHT@
$
$
$ HISTORY
$ $Log:	msgfac.msg,v $
$ Revision 1.4  90/10/07  16:45:35  devrcs
$ 	Added EndLog Marker.
$ 	[90/09/28  14:53:47  gm]
$ 
$ Revision 1.3  90/08/24  13:15:12  devrcs
$ 	fix useage message
$ 	[90/08/08  16:04:22  mbrown]
$ 
$ Revision 1.2  90/05/13  20:40:11  devrcs
$ 	new for OSF/1
$ 	[90/05/03  13:04:13  mbrown]
$ 
$ $EndLog$

$ @(#)msgfac.msg	1.12  com/cmd/msg,3.1,9013 2/2/90 13:34:49
 
$ COMPONENT_NAME: CMDMSG
$
$ FUNCTION: msgfac.msg
$ 
$ ORIGINS: 27
$ 
$ This module contains IBM CONFIDENTIAL code. -- (IBM
$ Confidential Restricted when combined with the aggregated
$ modules for this product)
$ OBJECT CODE ONLY SOURCE MATERIALS
$ (C) COPYRIGHT International Business Machines Corp. 1988, 1989
$ All Rights Reserved
$
$ US Government Users Restricted Rights - Use, duplication or
$ disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
$
$ (Copyright statements and/or associated legends of other
$ companies whose code appears in any part of this module must
$ be copied here.)



$ scmc msg file created Mon Feb 20 09:31:47 1989
$ user:  robbie

$quote "         define quote character.

$set MS_DSPCAT        Define initial set#

0               "com/cmd/msg,3.1,8926 6/16/89 13:16:44"

M_CAT_NO_OPEN        "dspcat: Unable to open specified catalog (%s)\n"
M_NO_ID              "dspcat: No message id allowed with -g option."
M_USAGE              "Usage:  dspcat [-g] catname [set#] [msg#]"

$set MS_GENCAT        Redefine set#

M_MSG_0              "Usage: gencat catfile [descfile ...]"
M_MSG_1              "gencat: Unable to open target file: %s \n"
M_MSG_2              "gencat: Unable to realloc()."
M_MSG_3              "gencat: Message text is longer than $len value.\n \t %s\n"
M_MSG_5              "gencat: Invalid set number. \n \t %s\n"
M_MSG_7              "gencat: Bad hex len (the length of a hex number must be either two or four digits.) \n \t %s \n"
M_MSG_8              "gencat: Unexpected newline within quotes.\n \t %s \n"
M_MSG_9              "gencat: Message string longer than NL_TEXTMAX. \n %s \n"
M_MSG_10             "gencat: Unexpected end of string, (no newline or end of quotes)\n \t %s \n"
M_MSG_11             "gencat: Internal error."
M_MSG_12             "gencat: Internal error (bad file position)"
M_MSG_13             "gencat: Unable to load specified catalog. \n \t %s \n"
M_MSG_14             "gencat: Unable to read old catalog file. \n \t %s \n"
M_MSG_15             "gencat: Unable to malloc memory."
M_NOSET	  	     "gencat: No set defined before first message.\n \t %s\n"
M_NOMSG		     "gencat: No messages defined in source file."
M_MSG_18	     "gencat: Symbolic set identifier used. \n \t %s \n"
M_MSG_19	     "gencat: Symbolic message identifier used:\n \t %s \n"
M_ORDER              "gencat:  The message numbers/sets became out of order just after:\n\
 msg:  %d,  set %d\n\
 %s"

$set MS_MKCATDEFS        Redefine set#

0                    "\"com/cmd/msg,3.1,8926 6/16/89 13:16:44\""
M_NO_OPEN       "gencat: Unable to open %s\n"
MKCATUSAGE	"mkcatdefs: Usage: %s [<catname>] <msg_file> [-h]\n"
MNAMTOOLONG	"mkcatdefs: catname too long\n"
MKCATOPN	"mkcatdefs: Cannot open %s\n"
INVTAG		"mkcatdefs: invalid syntax name %s\n"
INVLDSET	"mkcatdefs: set # %d already assigned or sets no in ascending sequence\n"
ZEROINV		"mkcatdefs: %s is an invalid identifier\n"
MULTOPN		"mkcatdefs: name %s used more than once\n"
MULTNR		"mkcatdefs: message id %s already assigned to identifier\n"
NOTASC		"mkcatdefs: source messages not in ascending sequence\n"
READERRS	"mkcatdefs: There were read errors on file %s\n"
WRITERRS	"mkcatdefs: There were write errors on file %s\n"
ERRFND		"mkcatdefs: Errors found: no %s created\n"
HCREAT		"mkcatdefs: %s created\n"
NOSYMB		"mkcatdefs: No symbolic identifiers; no %s created\n"
NOHDR	        "mkcatdefs: no %s created\n"

$set MS_DSPMSG        Define initial set#

M_DSPMSG       "Usage: dspmsg [-s setno] <catname> <msgno> ['default' arg ... ]"

$set MS_RUNCAT        Define initial set#

M_RUNCAT       "Usage: runcat catname srcfile [catfile]"
