#	Makelocal.mk -
#		sys/dist/install.d/finder.d Makefile
#
#  @(#)Makelocal.mk	4.1	(ULTRIX)	7/2/90
#
#	002	16-Oct-1989	jon
#	added geometry program
#
#	001	18-Apr-1989	jon
#	removed MACHINE dependency
#
#	000	02-mar-1989	ccb
#	New

include $(GMAKEVARS)

TODIR= $(DESTROOT)/usr/sys/dist
DESTLIST= $(TODIR)

AOUTS= geometry finder
OBJS= geometry.o finder.o

finder:	finder.o
finder.o:	finder.c

geometry: geometry.o
geometry.o:	geometry.c

install:
	@for i in $(AOUTS); \
	do \
		$(INSTALL) -c -s -m 755 $${i} $(TODIR)/$${i}; \
	done

include $(GMAKERULES)
