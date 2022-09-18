#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS=	stime.o strcatn.o strcmpn.o strcpyn.o ulimit.o utime.o

stime.o:		stime.c
strcatn.o:		strcatn.c
strcmpn.o:		strcmpn.c
strcpyn.o:		strcpyn.c
ulimit.o:		ulimit.c
utime.o:		utime.c

include ../Makelocal_$(MACHINE).mk

include $(GMAKERULES)
