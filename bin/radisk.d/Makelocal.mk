#  @(#)Makelocal.mk	4.1	ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=radisk

OBJS=radisk.o

radisk.o:	radisk.c

install:
	$(INSTALL) -s -c radisk $(DESTROOT)/bin/radisk
	$(RM) $(DESTROOT)/etc/radisk
	$(LN) -s ../bin/radisk $(DESTROOT)/etc/radisk

include $(GMAKERULES)
