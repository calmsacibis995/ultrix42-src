#	Makelocal.mk -
#		sys/dist/hosts.d Makefile
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

#	000	02-mar-1989	ccb
#	New.

include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/usr/hosts

AOUT=	makehosts

OBJS=	makehosts.o

makehosts.o:	makehosts.c

install:
	$(INSTALL) -s -c -m 700 makehosts $(DESTROOT)/usr/hosts/MAKEHOSTS

include $(GMAKERULES)
