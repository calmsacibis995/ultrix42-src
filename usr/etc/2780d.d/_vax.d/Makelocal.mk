#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=	2780d

OBJS=	2780d.o nmatch.o

2780d.o:	2780d.c
nmatch.o:	nmatch.c

install:
	$(INSTALL) -c -s -m 700 -o root 2780d $(DESTROOT)/usr/etc/2780d
	$(RM) $(DESTROOT)/etc/2780d
	$(LN) -s ../usr/etc/2780d $(DESTROOT)/etc/2780d

include $(GMAKERULES)
