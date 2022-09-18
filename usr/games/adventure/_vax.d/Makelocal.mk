#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

OBJS=    done.o init.o io.o main.o save.o subr.o vocab.o wizard.o

LDFLAGS=-n      # should be -i on small machines, -n on a vax

AOUT=adventure

done.o:		done.c
init.o:		init.c
io.o:		io.c
main.o:		main.c
save.o:		save.c
subr.o:		subr.c
vocab.o:	vocab.c
wizard.o:	wizard.c

install:
	install -c adventure $(DESTROOT)/usr/games/adventure

include $(GMAKERULES)
