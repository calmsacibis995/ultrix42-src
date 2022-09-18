#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	units

OBJS=	units.o

units:	units.o
units.o:	units.c

install:
	$(INSTALL) -c -s units $(DESTROOT)/usr/bin/units
	$(INSTALL) -c ../units.lib $(DESTROOT)/usr/lib/units

include $(GMAKERULES)
