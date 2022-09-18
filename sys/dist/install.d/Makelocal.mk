#	Makelocal.mk
#		sys/dist/install.d Makefile
#
#  @(#)Makelocal.mk	4.2  ULTRIX  7/17/90
#
#	002	05-Jul-1990	ccb
#	arrange for install of install.mup
#	001	18-Apr-1989	jon
#	removed MACHINE dependancy in MACHDEP loop
#
#	000	02-mar-1989	ccb
#	New.

include $(GMAKEVARS)

TODIR= $(DESTROOT)/usr/sys/dist
DESTLIST= $(TODIR)

SUBDIRS= finder.d

INSTALLSCRIPTS= .miniprofile .rootprofile .updprofile \
	ask_filesys ask_part install.2 sysupd install.mup

MACHDEP= install.1 install.3

CINCLUDES=-I. -I../..  -I$(SRCROOT)/usr/include
AOUTS= btd delet_part gethost log size_part
OBJS= btd.o delet_part.o gethost.o log.o size_part.o

btd:		btd.o
delet_part:	delet_part.o
gethost:	gethost.o
size_part:	size_part.o
log:		log.o

btd.o:		btd.c
delet_part.o:	delet_part.c
gethost.o:	gethost.c
size_part.o:	size_part.c
log.o:		log.c

install:
	@for i in $(INSTALLSCRIPTS); \
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
