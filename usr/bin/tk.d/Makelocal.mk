#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	tk

OBJS=	tk.o

tk:	tk.o
tk.o:	tk.c

install:
	$(INSTALL) -c -s tk $(DESTROOT)/usr/bin/tk

include $(GMAKERULES)
