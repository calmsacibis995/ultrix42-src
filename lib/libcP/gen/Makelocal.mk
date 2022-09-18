# @(#)Makelocal.mk	4.1	ULTRIX	7/3/90
#
# Dan Smith  28-Feb-90
# Include files which are affected by ANSI changes to fseek/ftell.
#

include $(GMAKEVARS)

# Files shared with generic libc version of stdio
STDIOSRC= filbuf.c fseek.c ftell.c fgetpos.c fsetpos.c
# fgetpos and fsetpos are here to get proper fseek() and ftell()

# Files shared with generic libc
GENSRC=_asctime.c assert.c crypt.c ctime.c strftime.c tzset.c tzs.h
# assert has to be here to get proper abort()
# ctime needed to get proper tzset()
# _asctime and strftime needed to get proper tzset()

# Files shared with System V library
VSRC=abort.c

# All non-local object files
GENOBJS=_asctime.o abort.o assert.o crypt.o ctime.o strftime.o tzset.o \
        fseek.o ftell.o filbuf.o fgetpos.o fsetpos.o

# Local object files
# OBJS=

include ../Makelocal_$(MACHINE).mk

all:	$(GENOBJS)

_asctime.o:	_asctime.c tzs.h
abort.o:	abort.c
assert.o:	assert.c
crypt.o:	crypt.c
ctime.o:	ctime.c tzs.h
strftime.o:	strftime.c
tzset.o:	tzset.c tzs.h
filbuf.o:	filbuf.c
fseek.o:	fseek.c
ftell.o:	ftell.c
fgetpos.o:	fgetpos.c
fsetpos.o:	fsetpos.c

$(GENSRC):
	$(RM) $@
	ln -s ../../../libc/gen/$@ $@

$(VSRC):
	$(RM) $@
	ln -s ../../../libcV/gen/$@ $@

$(STDIOSRC):
	$(RM) $@
	ln -s ../../../libc/stdio/$@ $@


include $(GMAKERULES)
