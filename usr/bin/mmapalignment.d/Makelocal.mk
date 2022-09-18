#  @(#)Makelocal.mk	4.1  ULTRIX  9/10/90

include $(GMAKEVARS)

AOUT=	mmapalignment

OBJS=	mmapalignment.o

mmapalignment:		mmapalignment.o
mmapalignment.o:	mmapalignment.c

install:
	$(INSTALL) -c -s mmapalignment $(DESTROOT)/usr/bin/mmapalignment
	$(RM) $(DESTROOT)/bin/mmapalignment
	$(LN) -s ../usr/bin/mmapalignment $(DESTROOT)/bin/mmapalignment

include $(GMAKERULES)
