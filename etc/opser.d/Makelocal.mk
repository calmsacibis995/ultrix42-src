#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTDIR=$(DESTROOT)/opr

DESTLIST=$(DESTDIR)

AOUT=	opser

OBJS=	opser.o parse.o remote_opser.o

$(OBJS):	opser.h

opser.o:	opser.c
parse.o:	parse.c
remote_opser.o:	remote_opser.c

AOUTS=	is_vaxstar

is_vaxstar:	is_vaxstar.o
is_vaxstar.o:	is_vaxstar.c

install:
	$(INSTALL) -c -s -m 755 opser $(DESTDIR)/opser
	$(INSTALL) -c -m 755 ../backup $(DESTDIR)/backup
	$(INSTALL) -c -m 755 ../dobackup $(DESTDIR)/dobackup
	$(INSTALL) -c -s -m 755 is_vaxstar $(DESTDIR)/is_vaxstar

include $(GMAKERULES)
