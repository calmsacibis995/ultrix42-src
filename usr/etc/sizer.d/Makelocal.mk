#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=	sizer

OBJS=	sizer.o tables.o getboot.o getroot.o getcpu.o getfloat.o \
	getconfig.o getconfig_utils.o misc.o

HFILES=	sizer.h

sizer.o:		sizer.c
tables.o:		tables.c
getboot.o:		getboot.c
getroot.o:		getroot.c
getcpu.o:		getcpu.c
getfloat.o:		getfloat.c
getconfig.o:		getconfig.c
getconfig_utils.o:	getconfig_utils.c
misc.o:			misc.c

$(OBJS):	$(HFILES)

install:
	$(INSTALL) -s -c -m 744 sizer $(DESTROOT)/usr/etc/sizer
	$(RM) $(DESTROOT)/etc/sizer
	$(LN) -s ../usr/etc/sizer $(DESTROOT)/etc/sizer

include $(GMAKERULES)
