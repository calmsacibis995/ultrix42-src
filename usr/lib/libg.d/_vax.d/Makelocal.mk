#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
include $(GMAKEVARS)

all: libg.a

#
# Note: libg.a is really a .o in disguise.  This forces the loader to
# put -lg in the symbol table, which dbx then expects to see.  It's very
# squirrely, since nobody actually uses libg.a for anything, but it also
# shouldn't be changed lightly.
#
libg.a:	dbxxx.s
	$(AS) ../dbxxx.s -o libg.a

pretools tools1 tools2: libg.a
pretools tools1 tools2 install:
	$(INSTALL) -c libg.a ${DESTROOT}/usr/lib

include $(GMAKERULES)
