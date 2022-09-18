.\"#@(#)mod.l	4.1	Ultrix	7/17/90
.TH MOD l "1 February 1984"
.SH NAME
mod \- Modula-2 compiler
.SH SYNOPSIS
.B mod
[ options ] name ...
.SH DESCRIPTION
.I Mod
is a Modula-2 compiler.
It compiles one or more Modula-2 program or implementation modules.
Definition modules are not compiled.
In the absence of options, it will compile all specified modules and link
them together into an executable file called
.I a.out.
.PP
Each program or implementation module must be in a separate file with a name
ending with \*(lq.mod\*(rq.
Each definition module must be in a separate file called \*(lqmodule.def\*(rq,
where \*(lqmodule\*(rq is the name of the module.
Object files ending with \*(lq.o\*(rq compiled with
.I mod
or some
other compiler may be specified.
.PP
File name arguments ending with \*(lq.pcd\*(rq and \*(lq.s\*(rq are assumed to be pcode
and assembly language files, respectively,
and are translated and assembled into object files.
.PP
The following options are available:
.TP 6
.B \-c
Create object files but do not link them together.
.TP 6
.B \-g
Generate additional symbol table information for the debugger
.IR dbx (1).
.TP 6
.B \-i
Ignore the fact that there are errors in some of the modules
and continue compiling the rest of them.
.TP 6
.BR \-m "flags"
Perform intermodule checking.
If an out-of-date module is encountered, recompile it using the
specified \*(lqflags\*(rq.
The flags are separated by commas or spaces, and must be quoted if spaces
are used.
.TP 6
.B \-n
Write out what will happen when the same command is entered
without the \*(lq-n\*(rq option.
.TP 6
.BR \-o " name"
Create an executable file called \*(lqname\*(rq instead of the default \*(lqa.out\*(rq.
.TP 6
.B \-pg
Set up object files for profiling by
.IR  gprof (1).
.TP 6
.B \-r
Retain pcode and assembly language files in the current directory after
compilation.
.TP 6
.B \-s
Use standard conventions for reserved word case, cardinal data type, and strings
(See Extensions, below).
.TP 6
.B \-sc
Use standard conventions for cardinal data type (See Extensions, below).
.TP 6
.B \-sk
Use standard conventions for reserved word case (See Extensions, below).
.TP 6
.B \-ss
Use standard conventions for string constants (See Extensions, below).
.TP 6
.B \-u
Convert all identifiers and reserved words to upper case (i.e., ignore
the case of identifiers and reserved words on input).
.TP 6
.B \-v
Print out messages saying what is happening during compilation.
.TP 6
.B \-C
Generate runtime checks for illegal pointers, subrange and index bounds,
and variant record tags.
.TP 6
.BR \-D "directory"
Use the specified directory for the phases of the compiler and the
location of the standard definition modules and libraries.
.TP 6
.B \-L
While performing intermodule checking, ignore references to modules
not specified.
(This is useful when checking modules to be placed in a library).
.TP 6
.B \-M
Perform intermodule checking, but do not recompile if inconsistencies
are found.
.TP 6
.BR \-N name
While performing intermodule checking, ignore references to the module
\*(lqname\*(rq.
(This is useful when the module \*(lqname\*(rq is not a Modula-2 module.)
This option may occur multiple times.
.TP 6
.B \-O
Perform code optimizations.
.TP 6
.B \-P
Stop after generating pcode in a file ending with \*(lq.pcd\*(rq.
.TP 6
.B \-S
Stop after generating assembly language in a file ending with \*(lq.s\*(rq.
.SH "LIBRARY MODULES"
By default, an import of a global module will cause the compiler to look for
the definition module first
in the working directory and then in the standard library directory.
The standard library modules are automatically linked with the program.
.PP
The default may be overridden to specify other directories of definition modules
using the MODPATH environment variable.
MODPATH is set to a sequence of directory names separated by colons.
Those directories will be searched in the order specified to find any definition
module.
The corresponding object files or libraries are specified when linking.
The MODPATH environment variable may be set by the user
in .login or in .modpath in the working directory.
If the file \*(lq.modpath\*(rq exists in the working directory, the 
.B mod
command will use its first line as the value of the MODPATH variable.
.PP
The following modules are provided by this implementation of Modula-2.
Note that system, memory, io, and bitoperations are builtin modules;
definition modules for them are provided for documentation purposes only.
Only strings and parameters are actually implemented in Modula-2.
.TP
system
.br
Builtin system module.  Contains types like word, address, etc., and process
routines.
.TP
memory
.br
Builtin storage module.  Sets up pointers properly for runtime checks.
Contains ALLOCATE and DEALLOCATE.
.TP
io
.br
Builtin I/O module that provides formatted read and write similar to
.IR scanf (3)
and
.IR printf (3).
.TP
bitoperations
.br
Builtin bit manipulation module.
Performs operations such as shift, exclusive or, etc., on integer operands.
.TP
math
.br
Mathematical functions.
Interface to the C math library.
.TP
parameters
.br
Accesses command line parameters and environment variables.
.TP
strings
.br
Compares, assigns, and concatenates strings.
.TP
unix
.br
Defines some Unix system calls and C library routines.
.TP
Storage
.br
Standard storage module, for compatibility with standard Modula-2.
Contains ALLOCATE and DEALLOCATE.
.SH "DIFFERENCES AND EXTENSIONS"
This implementation of Modula-2 has compiled and run Wirth's Modula-2 compiler
(as modified by Cambridge University for the VAX) with only minor changes
to make that compiler more portable.
However, the definition of the language has been relaxed in some areas.
For the most part, these changes are upward compatible.
.PP
The following is an incomplete list of differences between this compiler and
Wirth's compiler:
.PP
Reserved words and standard identifiers are recognized in any case,
not just in upper case.
Thus, case variations of reserved words may not be used for identifiers.
This feature is disabled by the
.B -sk
option.
.PP
Cardinal and non-negative subranges that do not exceed MAXINT are considered
to be subranges of integer and are compatible with integers.
Subranges that exceed MAXINT are compatible with cardinal and non-negative
subranges.
This feature is disabled by the
.B -sc
option.
.PP
A builtin module called
.I io
provides formatted input and output.
The
.I Readf
and
.I Writef
routines can accept any number of parameters, so long as their types
correspond properly with the format string.
Supported formats include: for integer and cardinal,
.BR d ,
.BR x ,
and
.BR o ;
for real,
.B g
(output only),
.BR f ,
and
.BR e ;
for longreal,
.B G
(output only),
.BR F ,
and
.BR E ;
for char,
.BR c ;
and
for string (array of char),
.B s
and
[] (input only).
.PP
No import of
.I allocate
or
.I deallocate
is required to use new and dispose if the standard memory allocation routines
are desired.
Programs that desire checking will normall import allocate and deallocate
from memory, rather than storage.
.PP
The sizes returned by
.I size
and
.I tsize
and expected by
.IR allocate ,
.I deallocate
and
.I newprocess
are in units of bits.
.PP
The
.I system
module includes the type
.IR byte ,
which is analogous to
.IR word ,
as well as appropriate related constants.
There is also a function cputime, which returns the accumulated program
CPU time in milliseconds.
.PP
There is a standard type called
.I longreal
that stores a double precision real value.
A standard function longfloat converts cardinals, integers, or reals 
to longreal.
.PP
Additional standard procedures include:
.TP
min(a,b)
Returns the smaller of two cardinal, integer, real, or longreal values.
.TP
max(a,b)
Returns the larger of two cardinal, integer, real, or longreal values.
.TP
assert(condition[,message])
Aborts the program (with the optional message)
if the condition is false.
.TP
number(a)
Returns the number of elements in the specified array.
.TP
first(type)
Returns the smallest legal value of the specified type.
.TP
last(type)
Returns the largest legal value of the specified type.
.PP
Definition modules are not compiled.
.PP
Escape sequences may be placed in strings to specify non-printing characters.
E.g., \en, \et, \er, \ef, \eb, \e\e, \e', and \e" mean
linefeed, tab, carriage return, form feed, backspace, backslash,
single quote, and double quote, respectively.
In addition a \e followed by up to three octal digits specifies the
the character whose ASCII code is the octal value.
A single (double) quote also may be put in a string delimited with
single (double) quotes by specifying two single (double) quotes.
This feature is disabled by the
.B -ss
option.
.PP
The interface to Unix is through a module called
.I unix
rather than the
.I system
module.
The
.I unixcall
procedure is handled for compatibility with the Cambridge compiler,
but is not recommended.
.PP
Additional keywords are recognized in certain contexts.
These keywords are prefixed by @ to avoid conflicting with valid
identifiers.
.TP
Pointer attributes
Attributes may be specified between the keywords
.I pointer
and
.I to
in order to change the default assumptions of Modula-2 pointer with checking.
Recognized attributes are:
.br
.ta 2i
@nocheck	Modula-2 pointer, no checking
.br
@c	C/malloc pointer, no checking
.br
@pascal	Pascal pointer, Pascal checking
.TP
Size and alignment
The size and alignment of data types may be specified preceding any
type specification.
The size and alignment multiples are in bits.
For example,
.br
    type Register = @align 2 @size 4 [-8..7];
.br
defines a type that occupies 4 bits aligned on a multiple of two bits.
See Using Modula-2 with Unix C and Berkeley Pascal for more details.
.TP
Exports
Exports from a definition module are assumed qualified whether the export
statement says qualified or not.
Unqualified exports are permitted if the @unqualified keyword is used.
Multiple export statements are permitted, but they must occur next to
each other.
.TP
External variables and procedures
A procedure or variable may be accessed by C and Pascal routines using
its unqualified name if the @external attribute
occurs between the keyword procedure and the name of the procedure or
precedes the variable declaration, respectively.
See Using Modula-2 with Unix C and Berkeley Pascal for more details.
.TP
Uncounted open arrays
Open array parameters appear as two parameters, the address of the array and
the number of element, to non-Modula-2 programs.
If necessary, the count may be omitted by placing the attribute @nocount
between the keywords
.I array
and
.I of
in the open array declaration.
See Using Modula-2 with Unix C and Berkeley Pascal for more details.
.SH FILES
.ta 2.6i
file.mod	Program or implementation module
.br
file.def	Definition module
.br
file.pcd	Pcode (-P or -r)
.br
file.s	Assembly code (-S or -r)
.br
/usr/local/lib/mod/mod2.0	Modula-2 compiler front-end
.br
/usr/local/lib/mod/mod2.1	Modula-2 compiler back-end
.br
/usr/local/lib/mod/mod2.2	Intermodule checker
.br
/usr/local/lib/mod/*.def	Standard definition modules
.br
/usr/local/lib/mod/modlib	Default library
.br
/tmp/modNNNNNN.pcd	Temporary Pcode file
.br
/tmp/modNNNNNN.s	Temporary assembly code file
.SH "SEE ALSO"
N. Wirth,
.IR "Programming in Modula-2" ,
Springer-Verlag, New York, 1982.
.SH DIAGNOSTICS
All error messages suppress subsequent compilation phases.
Error messages ending with a question mark are internal errors, and
probably represent compiler bugs.
When pointer checking is enabled in a running Modula-2 program,
segmentation faults may be generated by the pointer validation test.
These are intentional and should be considered as invalid pointer messages.  
The compiler runs with runtime checks enabled, and may produce core dumps.
Report problems to the author.
.SH AUTHOR
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
This is an experimental compiler, and thus no warranties are expressed or
implied about its conformance to the definition of the Modula-2 language
or about its proper functioning.
We will endeavor to report and fix bugs, but users should be aware that this
compiler is not a supported product.
