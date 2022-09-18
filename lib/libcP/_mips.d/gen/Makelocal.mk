#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS=	tzX.o

tzX.o:		tzX.s

include $(GMAKERULES)
