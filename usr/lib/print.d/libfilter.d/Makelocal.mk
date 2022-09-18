# @(#)Makelocal.mk	4.1      ULTRIX 	10/16/90

# Makefile for libfilter.d
#
#
# Author:	Adrian Thoms
# Date:		26th September 1990
# Modification History:
#

include $(GMAKEVARS)

ARFILE=libfilter.a


HFILES =	filetype.h
OBJS =		guesser_new.o fdtype_precomp.o file_magic.o fregex.o
CFILES =	mtabinit.c fregex.c file.c filetype.c fdtype.c


MTABINIT_OBJS =	mtabinit.o fregex.o file.o filetype.o fdtype_nocomp.o


MAGIC_OBJS =	mtabinit.o fregex.o magic.o filetype.o \
		file_magic.o fdtype_precomp.o


LDFLAGS=${CFLAGS}

# test program for new file guesser
guesser_new: guesser_new.c fdtype_precomp.o fregex.o file_magic.o
	$(LDCMD) $(CFLAGS) -g -DTESTING ../guesser_new.c fdtype_precomp.o fregex.o file_magic.o

mtabinit: ${MTABINIT_OBJS}
	${LDCMD} ${MTABINIT_OBJS}

magic: ${MAGIC_OBJS}
	${LDCMD} ${MAGIC_OBJS}

file_magic.c: mtabinit magic.print
	./mtabinit -im ../magic.print > $@ || rm $@

magic.o:	file.c
	@rm -f file.o
	${CCCMD} -DHAVE_MAGIC ../file.c
	mv file.o $@

fdtype_precomp.o:	fdtype.c
	@rm -f fdtype.o
	${CCCMD} -DHAVE_MAGIC ../fdtype.c
	mv fdtype.o $@

fdtype_nocomp.o:	fdtype.c
	@rm -f fdtype.o
	${CCCMD} ../fdtype.c
	mv fdtype.o $@

file_magic.o:	file_magic.c
	${CCCMD} file_magic.c


guesser_new.o:	guesser_new.c
mtabinit.o:	mtabinit.c
file.o:		file.c
file_magic.o:	file_magic.c
filetype.o:	filetype.c
fregex.o:	fregex.c


install:
	@echo "Nothing to install"

include $(GMAKERULES)
