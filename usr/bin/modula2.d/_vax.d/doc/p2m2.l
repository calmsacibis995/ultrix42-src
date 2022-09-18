.\"#@(#)p2m2.l	4.1	Ultrix	7/17/90
.TH P2M2 l "1 February 1984"
.SH NAME
p2m2 \- Pascal to Modula-2 conversion tool
.SH SYNOPSIS
.B p2m2
[ -h ] [ -s ] name
.SH DESCRIPTION
.I P2m2
is a tool for assisting with the conversion of programs from Berkeley Pascal to
Modula-2.
It does not perform a complete translation, since there are some
language features in Pascal that either do not exist in Modula-2 (e.g.,
.B goto
statements) or are too difficult to convert
(e.g., write statements).
It also does not necessarily produce a \*(lqgood\*(rq Modula-2 program,
since it does not restructure a program into modules.
.PP
In spite of these disclaimers, it is possible to use
.I p2m2
to rapidly convert a substantial amount of Pascal software to Modula-2.
.PP
Files are converted one at a time.
A file name must end with \*(lq.p\*(rq for a Pascal program or separate compilation
unit,
or with \*(lq.h\*(rq for a set of definitions or external specifications for a
separate compilation unit.
A file called \*(lqname.p\*(rq will produce a program or implementation module called
\*(lqname\*(rq in a file called \*(lqname.mod\*(rq.
A program module will be generated if the file contains a Pascal program.
A file called \*(lqname.h\*(rq will produce a definition module called name in
a file called \*(lqname.def\*(rq.
.PP
The 
.B \-h
flag is specified when a separate compilation unit is converted.
The option causes
.I p2m2
to scan the corresponding \*(lq.h\*(rq file for procedure parameter definitions and
insert those in the generated \*(lq.mod\*(rq file.
This flag is useful because Berkeley Pascal prohibits parameters from
appearing in the procedure definition if there is an external declaration of
the procedure.
.PP
The
.B \-s
flag outputs reserved words in upper case, in accordance with the Modula-2
report.
By default,
.I p2m2
generates reserved words in lower case.
.SH "METHOD"
As
.I p2m2
parses the Pascal program, it copies white space (comments, spaces, new lines)
to the output.
It outputs tokens and identifiers, rearranging them as necessary.
The result is a program that is formatted approximately the same
as the original.
.PP
The names of procedures, functions, variables, types, and constants defined
in \*(lq.h\*(rq files are exported from the definition module.
The names are exported unqualified, to simulate the global naming that
takes place in Berkeley Pascal.
(Although the Modula-2 report requires global exports to be qualified,
the DEC Modula-2 compiler permits unqualified exports.)
An include directive is changed to a comment, but causes the named module
to be imported.
.SH "UNHANDLED DIFFERENCES"
The following is a partial list of differences between Pascal and Modula-2
that are not handled:
.LP
.I "Forward declarations"
are not necessary, since Modula-2 allows procedures to be defined after they
are used.
.LP
.I "Goto statements and label declarations"
are not supported.
Many goto statements may be avoided by using the
.B loop
and
.B exit
or
.B return
statements.
.LP
.I "I/O and files"
are different, including write statements.
See
.IR mod (l)
and the standard module
.I io.def
for details.
.LP
.I "Procedure parameters"
are supported differently.
Since Modula-2 supports procedure variables (which may be passed as parameters),
the syntax for formal procedure parameters is similar to other formal
parameters.
.LP
.I "Function return values"
are done through the return statement, not by assignment to the function name.
Assignments to functions are marked with the comment (*!return!*) to allow
easy editting in most cases.
.SH FILES
.ta 1.1i
file.p	Pascal main program or separate compilation unit
.br
file.h	Pascal header file
.br
file.mod	Program or implementation module
.br
file.def	Definition module
.br
p2m2.temp	Output of first pass of p2m2
.SH "SEE ALSO"
N. Wirth,
.IR "Programming in Modula-2" ,
Springer-Verlag, New York, 1982.
.SH DIAGNOSTICS
Error messages are written to standard output.
In addition, comments are inserted in the generated modules to mark
places where the translation failed.
Such comments are of the form (*! ... !*).
.SH AUTHORS
Benjamin C. Pierce
.br
Michael L. Powell
.br
Digital Equipment Corporation
.br
Western Research Laboratory
.br
4410 El Camino Real
.br
Los Altos, CA  94022
.br
Mail: powell@decwrl.csnet or {decvax,ucbvax}!decwrl!powell
.PP
Software and documentation is
Copyright 1984, Digital Equipment Corporation,
Maynard, Massachusetts.
All rights reserved.
This software is provided under license agreement and must be kept confidential.
.SH LIMITATIONS
This is an experimental tool, and thus no warranties are expressed or
implied about its conformance to the definition of the Modula-2 language
or about its proper functioning.
