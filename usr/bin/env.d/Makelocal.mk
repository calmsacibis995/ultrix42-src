# @(#)Makelocal.mk	4.1 ULTRIX 7/17/90

include $(GMAKEVARS)

DESTLIST =	$(DESTROOT)/usr/bin

OBJS =		env.o
AOUT =		env	

env.o:	env.c

install:
	$(INSTALL) -c -s $(AOUT) $(DESTROOT)/usr/bin/$(AOUT)

include $(GMAKERULES)
