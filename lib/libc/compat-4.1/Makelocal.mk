#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS=	ftime.o gtty.o nice.o pause.o rand.o stty.o \
	times.o vlimit.o vtimes.o getpw.o 

ftime.o:	ftime.c
gtty.o:		gtty.c
nice.o:		nice.c
pause.o:	pause.c
rand.o:		rand.c
stty.o:		stty.c
times.o:	times.c
vlimit.o:	vlimit.c
vtimes.o:	vtimes.c
getpw.o:	getpw.c

include ../Makelocal_$(MACHINE).mk

include $(GMAKERULES)
