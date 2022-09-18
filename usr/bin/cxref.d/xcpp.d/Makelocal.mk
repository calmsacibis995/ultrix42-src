#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	xcpp

OBJS=	cpp.o cpy.o

$(AOUT): $(OBJSMISC)

CDEFINES=-Dunix=1 -DCXREF -DFLEXNAMES

cpp.o: cpp.c
cpy.o: cpy.y

install:
	$(INSTALL) -c -s xcpp $(DESTROOT)/usr/lib/xcpp

include $(GMAKERULES)
