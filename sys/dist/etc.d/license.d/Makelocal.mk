#	Makelocal.mk -
#		sys/dist/etc.d/license.d Makefile
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

#	000	02-mar-1989	ccb
#	New.

include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/etc

AOUT=	license

OBJS=	license.o

license.o:	license.c

install:	license
	$(INSTALL) -c -m 754 -s license $(DESTROOT)/etc/license

include $(GMAKERULES)
