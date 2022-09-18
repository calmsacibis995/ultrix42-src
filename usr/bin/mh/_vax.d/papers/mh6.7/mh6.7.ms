.\" @(#)$Id: mh6.7.ms,v 1.1 90/07/20 11:10:48 sharpe Exp $
.\" Standard -ms macros
.\" with the following changes
.ds lq \\*Q
.ds rq \\*U
.DA "April 9, 1990"
.if n \{\
.nr PO 1i
.po 1i
.\}
.EH ''Changes to MH 6.7'%'
.OH ''Changes to MH 6.7'%'
.TL
Changes to
.br
The RAND MH Message Handling System:
.br
UCI version MH 6.7
.AU
John L. Romine
.AI
Computing Support Group
Department of Information and Computer Science
University of California, Irvine
Irvine, CA  92717
.AB
.PP
This document describes the user\-visible and internal changes to the
UCI version of the RAND MH system from MH 6.6
to MH 6.7.  The first part should be of interest to all users
of MH, while the latter part is of interest only to MH
Administrators.
.PP
This document is meant to supplement,
not supersede,
the standard MH User's manual and MH Administrator's manual.
.PP
Comments concerning this documentation should be addressed to the
mailbox \fBBug\-MH@ICS.UCI.EDU\fP, or \fBucbvax!ucivax!bug-mh\fP.
.AE
.SH
ACKNOWLEDGEMENTS
.LP
The \fIMH\fP system described herein is
based on the original RAND \fIMH\fP system.
It has been extensively developed (perhaps too much so) by Marshall T. Rose
and John L. Romine at the University of California, Irvine.
Einar A. Stefferud, Jerry N. Sweet,
and Terry P. Domae provided numerous suggestions
to improve the UCI version of \fIMH\fP.
.PP
Of course,
a large number of people have helped \fIMH\fP along.
The list of \*(lq\fIMH\fP immortals\*(rq is too long to list here.
For this release, numerous \fIMH\-Workers\fP sent in fixes and other
changes.  A handful of courageous \fIMH\-Workers\fP volunteered
to beta\-test these changes; their help is particularly appreciated.
.SH
DISCLAIMER
.LP
The Regents of the University of California wish to make it known that:
.QP
Although each program has been tested by its contributor,
no warranty, express or implied,
is made by the contributor or the University of California,
as to the accuracy and functioning of the program
and related program material,
nor shall the fact of distribution constitute any such warranty,
and no responsibility is assumed by the contributor
or the University of California in connection herewith.
.KS
.SH
CONVENTIONS
.LP
In this document,
certain formatting conventions are adhered to:
.IP
The names of
\s-2UNIX\s+2
commands, such as \fIcomp\fP
are presented in \fIitalics\fP.
.IP
Arguments to programs, such as `msgs' and `\-nobell' are
delimited by single\-quotes.
.IP
Text that should be typed exactly as\-is, such as 
command lines (e.g., \*(lqfolder \-pack\*(rq),
are delimited by double\-quotes.
.IP
\s-2UNIX\s+2
pathnames and envariables,
such as \fB/usr/uci\fP and \fB$SIGNATURE\fP,
are presented in \fBbold font\fP.
.KE
.SH
GENERAL CHANGES
.LP
The author is pleased to announce that there are very few
user\-visible
changes to \fIMH\fP 6.7 from the previous \fIMH\fP 6.6 distribution.
The majority of development was in the form of bug fixes and
slight enhancements.
In addition, this release is slightly faster than the
previous release.
With a few minor exceptions,
it is backward\-compatible with the previous release.
.PP
The changes were made mainly to generalize the source code to
be compatible with a larger range of systems and compilers.
There were many small changes to add declarations for ANSI C compliance.
The System 5 support has been brought up to SYS5 R3, and there is
support for Sun OS 4.0.
.SH
User\-Visible Changes
.LP
Here a quick summary of the
changes that were made which are not backward\-compatible with the
previous release of \fIMH\fP:
.IP repl \w'sortm'u+2n
The `\-format' and `\-noformat' switches
have not been functional since \fIMH\fP 5, and have been removed.
Any users who have these switches in their \fB\&.mh\(ruprofile\fP,
will have to remove them.
.IP sortm
Previously, in most cases \fIsortm\fP would fill\-in any
gaps in the numbering of a folder, 
by renumbering the messages starting with `1'.
This will no longer occur; for this behavior,
use \*(lqfolder \-pack\*(rq.
.PP
.SH
Using Aliases
.LP
A new profile entry `Aliasfile:' has been added.  The
\fIali\fP\^, \fIsend\fP\^, and \fIwhom\fP programs will look for
this profile entry and treat it as they would an argument to
`\-alias'.
This should make it easier for novice \fIMH\fP users to begin
using aliases.
.PP
.SH
Reading Network News & BBoards
.LP
The UCI BBoards facility can read local BBoards, and if compiled
with the `bboards: pop' and `pop: on' options, can also read remote
BBoards using the Post Office Protocol (POP ver. 3).
With this release,
\fIMH\fP can instead be compiled to read the Network News
(i.e., USENET) using the Network News Transfer Protocol (NNTP).
.PP
This capability is enabled by compiling \fIMH\fP with
the `bboards: nntp' and `pop: on' options.
Unfortunately, reading remote BBoards via the POP and reading the Network
News via the NNTP are mutually exclusive options.
.PP
To support the NNTP,
a new module, \fBuip/pshsbr.c\fP, is compiled and loaded into
\fIbbc\fP and \fImsh\fP instead of \fBuip/popsbr.c\fP.
The default BBoard is changed from \*(lqsystem\*(rq to \*(lqgeneral\*(rq
for the NNTP.
.PP
When reading BBoards,
\fIbbc\fP will first look for local BBoards, and then contact the
NNTP server to read the Network News.  The location of the
NNTP server should be specified with the `nntphost:'
entry in the \fBmtstailor\fP file
(see the \fIMH\fP Administrator's Guide for details),
or may be specified on
the command line with the `\-host' switch.
.PP
.SH
Format Strings
.LP
The manual page \fImh\-format\fP\0(5) has
been rewritten to give a better explanation of how to write format strings,
and how they are interpreted by \fIMH\fP.
A line\-by\-line
description of the 
default \fIrepl\fP form file (\fBreplcomps\fP)
is now included in that manual page.
.PP
.KS
Some new format functions were added, and others were augmented:
.IP trim \w'date2local'u+2n
Strips any leading and trailing white\-space from the current string value.
.IP date2local
Will coerce the date to the local timezone.
.IP date2gmt
Will coerce the date to GMT.
.IP divide
Divides the current numeric value by its argument.
This could be useful for
building \fIscan\fP format strings which print large
message sizes in \*(lqKb\*(rq or \*(lqMb\*(rq.
.IP friendly
If the address field cannot be parsed,
this function will return the text of the address header,
instead of a null string.
.IP szone
A flag indicating whether the timezone was explicit
in the date string.
.KE
.SH
PROGRAM CHANGES
.LP
In addition to the general changes mentioned above,
many programs have specific new features added,
either by new switches or by expanded functionality.
Each command's manual page gives complete information about
its new options.  Here is a short summary.
.SH
User Interface Programs
.IP anno \w'prompter'u+2n
Accepts a `\-nodate' switch which inhibits the date annotation,
leaving only the body annotation.
.IP folder
When invoked with the `\-pack' switch
and the new `\-verbose' switch, \fIfolder\fP will
give information about the actions taken to renumber the folder.
.IP
On most systems, \fIfolder\fP can now create any non\-existing
parent folders of a new sub\-folder.
.IP forw
When making digests, \fIforw\fP\^
will put the issue and volume numbers
in addition to the digest list name, in the digest trailer.
.IP inc
Detects NFS write failures, and will
not zero your maildrop in that event.
.IP msh
Supports a variant of the new \fIsortm\fP\^.
.IP prompter
Considers a period on a line by itself to signify end\-of\-file
when the `\-doteof' switch is specified.
.IP repl
The `\-[no]format' switches
have not been used since \fIMH\fP 5 and have been deleted. \fIrepl\fP
will now find filter files in the \fIMH\fP library area.
.IP scan
With the `\-file msgbox' switch, \fIscan\fP\^
can list a \fIpackf\fP\|'d\-format file directly
(without using \fImsh\fP\^).
.IP
Lists messages in reverse order with the `\-reverse' switch.
This should be considered a bug.
.IP sortm
Now has the options:
`\-textfield field', `\-notextfield',
`\-limit days', and `\-nolimit'.
.IP
With these options, \fIsortm\fP can be instructed to sort a
folder based on the contents of an arbitrary header such
as \*(lqsubject\*(rq.
.IP
\fIsortm\fP minimizes renaming messages, and
will no longer arbitrarily pack folders; for this behavior,
use \*(lqfolder \-pack\*(rq.
.IP whatnow
Deletes the draft by renaming it with leading comma, instead
of unlinking it.
.SH
\fIMH\fP Support Programs
.LP
.KS
The following support programs also have changes or enhancements:
.IP mhl \w'rcvtty'u+2n
Will now accept a format string on any component, not just on
addresses and dates.
.KE
.IP popd
Will use \fIshadow\fP passwords if compiled with the
\fBSHADOW\fP option.  It can now also
read UUCP\-style maildrops directly.
.IP rcvtty
If given no arguments, \fIrcvtty\fP will produce a \fBscan\fP
listing as specified by a format string or file; a default
format string is used if one is not specified.
.IP
Before the listing is written to the users terminal, the
terminal's bell is rung and a newline is output.
The `\-nobell' and the `\-nonewline' options
inhibit these functions.
.IP
\fIrcvtty\fP will obey terminal write
notification set by \fImesg\fP.
With the `\-biff' switch, \fIrcvtty\fP will
also obey the mail notification status set by
\fIbiff\fP.  
.IP
On \fBBSD43\fP systems, as with \fIwrite\fP,
\fIrcvtty\fP will be installed set\-group\-id to the group \*(lqtty\*(rq.
.IP slocal
Understands UUCP\-style \*(lqFrom \*(rq lines and will write
output files using this format if appropriate.
Before invoking a delivery program, \fIslocal\fP will strip
such lines unless compiled with the \fBRPATHS\fP option,
in which case it will will convert such lines into
\*(lqReturn\-Path:\*(rq headers.
.IP
\fIslocal\fP has a new result code \*(lqN\*(rq,
for use in \fB.maildelivery\fP files.
With this result code,
\fIslocal\fP will
perform the action only if the message has not been delivered
and the previous action succeeded.  This allows for performing
an action only if multiple conditions are true.
.SH
DOCUMENTATION
.LP
Several of the older \fIMH\fP papers have been difficult to format
because they depended on an older version of
PhDTeX which was
not supplied.  These papers have been updated, and some TeX
library files are supplied in \fBpapers/doclib/\fP, so that
these papers may be generated on any system with TeX.
.PP
Many of the manual pages have been revised to include documentation
of new command options, and some have been expanded to give more detail.
All are now slightly reformatted at installation time
to make them more compatible with programs like \fImakewhatis\fP\^.
.PP
.SH
\fIMH\fP ADMINISTRATION
.LP
This section describes changes in configuring, compiling and
installing \fIMH\fP 6.7 and should not
be of interest to casual \fIMH\fP users.
The \fBREAD\-ME\fP file has been 
considerably revised and expanded to give more detail 
about the configuration and compilation options which
have been included in this release.  Some compilation options
have been removed, and many new options have been added.
.PP
All \fIMH\fP \fBMakefile\fPs have been updated to work around some
incompatibilities introduced in newer versions of \fImake\fP\^.
\fIMH\fP programs will no longer be installed with the sticky\-bit
turned on.
.PP
Reading this section not a substitute for carefully
reading the \fBREAD\-ME\fP file before attempting to compile \fIMH\fP
.PP
.KS
.SH
Bug Fixes
.LP
Some bugs were fixed which in general were not user\-visible:
.IP "address parser" \w'dynamicnmemory'u+2n
Fixed to allow use of the \*(lqAT\*(rq domain, and some
minor bugs were fixed pertaining to address groups.
.IP "date parser"
Improved to accept more forms of illegal dates.  Military
timezones were removed.
.IP "dynamic memory"
Many problems with corruption of the dynamic memory pool
have been fixed.
.IP locking
Will open files for write, if necessary to enable locking.
.IP "nil pointers"
All reported nil pointer problems have been fixed.
.IP replcomps
The \*(lqIn\-Reply\-To:\*(rq header had quotes added
around the date field to comply with RFC822.
.KE
.SH
White Pages
.LP
If \fIMH\fP is compiled with the \fBWP\fP option,
\fIsend\fP recognizes an address between \*(lq<<\*(rq and \*(lq>>\*(rq characters
such as:
.DS
To: << rose \-org psi >>
.DE
to be a name meaningful to a whitepages service.  In order to expand the
name, \fIsend\fP must be invoked interactively
(i.e., not from \fIpush\fP\^).  For each
name, \fIsend\fP will invoke a command called \fIfred\fP\^
in a special mode asking to expand the name.
.PP
To get a copy of the white pages service, contact wpp\-manager@psi.com.
.SH
Configuration Options
.LP
Some configuration options have been added or changed:
.IP cc \w'bbdelivery'u+2n
To specify an alternate C compiler.
.IP ccoptions
Defaults to `\-O'.
.IP bboards
May now be defined as \*(lqon\*(rq,
\*(lqoff\*(rq, \*(lqpop\*(rq, or \*(lqnntp\*(rq.
.IP bbdelivery
Determines whether the bboard delivery agent
and library files should be installed.
.IP lex
To specify an alternate version of \fIlex\fP\^.
.IP mailgroup
If defined, \fIinc\fP will be made set\-group\-id to this group.
.IP sharedlib
For \fBSUN40\fP systems; if \*(lqon\*(rq, makes \fBlibmh.a\fP
into a shared library.
.IP slibdir
The directory where the above shared library should be installed.
.IP sprintf
Set this to \*(lqint\*(rq if that's what your \fIsprintf\fP\0(3) library
routine returns.
.SH
Compilation Options
.LP
For different configurations,
several `\-D' options to \fIcc\fP have been added or changed:
.IP BERK \w'SENDMAILBUG'u+1n
This disables the address and date parsing routines.  If you
want to do much with \fImh\-format\fP\0(5), don't enable this.
.IP BSD43
Will make \fIrcvtty\fP set\-group\-id to the group \*(lqtty\*(rq.
.IP DBM
For sites with a dbm\-style password file (such as with Yellow
Pages), \fIMH\fP will not read the entire passwd file into a cache.
At one site that runs YP on a large passwd file, using this
showed a 6:1 performance improvement.
.IP NETWORK
This option has been deleted.  See \fBSOCKETS\fP.
.IP NOIOCTLH
Tells \fIMH\fP not to include the file \fBsys/ioctl.h\fP.  Use this
if this file is not present on your system.
.IP NTOHLSWAP
On systems with TCP/IP networking,
\fImsh\fP will try to use the \fBntohl()\fP macro from the
file \fBnetinet/in.h\fP to byte\-swap the binary map files
it writes.
.IP SENDMAILBUG
Some versions of \fIsendmail\fP return a \fB451\fP (failure) reply code
when they don't mean to indicate failure.  This option considers
that code to be equivalent to \fB250\fP (OK).
.IP SHADOW
Causes \fIpopd\fP to read the file \fB/etc/shadow\fP for
encrypted passwords instead of \fB/etc/passwd\fP.  Use this if you
have a shadow password file (such as on newer versions of SYSTEM 5).
.IP SOCKETS
Enable this if you are on a non\-BSD system with a
socket interface for TCP/IP networking compatible with 4.2BSD
.SM
UNIX.
.NL
.IP SUN40
Use on Suns running Sun OS 4.0 and later.
.IP SYS5
This option has been updated to refer to SYS5 R3 and later systems.
.IP SYS5DIR
Use this if your system uses \*(lqstruct dirent\*(rq instead of
\*(lqstruct direct\*(rq.  This should be true for systems based
on SYS5 R3 and later.
.IP TYPESIG
Defines the base type for the \fIsignal\fP system call.  This
defaults to \*(lqint\*(rq, but should be defined as \*(lqvoid\*(rq
if appropriate for your system.
.IP WP
Enables support for the White Pages service.
.SH
Installation
.LP
\fIMH\fP will now
explicitly set the protection mode on every file it installs.
.PP
Previously any existing file installed by \fIMH\fP
would be backed up into
the source tree, and then overwritten.
Now, a few system\-dependent files will not be overwritten, and your
changes will have to be merged in by hand.
See the \fBREAD\-ME\fP file for more details.
