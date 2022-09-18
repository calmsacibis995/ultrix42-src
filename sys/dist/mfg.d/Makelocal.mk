#	Makelocal.mk -
#		sys/dist/mfg.d Makefile
#
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
#
#	000	02-mar-1989	ccb
#	New.

include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/usr/sys/dist
TODIR=$(DESTLIST)

SCRIPTS= genboots getimages make.sas buildroot
MACHDEP= buildmini
AOUTS= fsmrg mksastape rxintlv zeero
OBJS= fsmrg.o mksastape.o rxintlv.o zeero.o

fsmrg:		fsmrg.o
mksastape:	mksastape.o
rxintlv:	rxintlv.o
zeero:		zeero.o

fsmrg.o:	fsmrg.c
mksastape.o:	mksastape.c
rxintlv.o:	rxintlv.c
zeero.o:	zeero.c


install:
	@for i	in $(SCRIPTS); \
	do \
		echo "$(INSTALL) -c -m 755 ../$$i $(TODIR)/$$i"; \
		$(INSTALL) -c -m 755 ../$$i $(TODIR)/$$i; \
	done
	@for i in $(MACHDEP); \
	do \
		echo "$(INSTALL) -c -m 755 ../$${i} $(TODIR)/$$i"; \
		$(INSTALL) -c -m 755 ../$${i} $(TODIR)/$$i; \
	done
	@for i in $(AOUTS); \
	do \
		echo "$(INSTALL) -c -s -m 755 $$i $(TODIR)/$$i"; \
		$(INSTALL) -c -s -m 755 $$i $(TODIR)/$$i; \
	done

include $(GMAKERULES)
