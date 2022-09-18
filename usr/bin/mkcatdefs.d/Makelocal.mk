# @(#)Makelocal.mk	4.1 (ULTRIX) 7/17/90

include $(GMAKEVARS)

CDEFINES  = -D_BLD
DESTLIST = $(DESTROOT)/usr/bin
LOADLIBES = -li

SOURCES	= mkcatdefs.c catio.c symtab.c
OBJECTS	= mkcatdefs.o catio.o symtab.o
HEADERS	= mesg.h nl_types.h standards.h 

all:		mkcatdefs
mkcatdefs:		$(OBJECTS)
		$(CC) $(LDFLAGS) -o mkcatdefs $(OBJECTS) $(LOADLIBES)

$(OBJECTS):	$(HEADERS)

mkcatdefs.o:	mkcatdefs.c
catio.o:	catio.c
symtab.o:	symtab.c

install:	
	$(INSTALL) -c -m 555 -o root -g system -s mkcatdefs $(DESTROOT)/usr/bin

include $(GMAKERULES)
