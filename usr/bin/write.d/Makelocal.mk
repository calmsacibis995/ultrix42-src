#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	write

OBJS=	write.o

write:	write.o
write.o:	write.c

install:
	$(INSTALL) -c -g tty -m 2755 -s write $(DESTROOT)/usr/bin/write
	$(RM) $(DESTROOT)/bin/write
	$(LN) -s ../usr/bin/write $(DESTROOT)/bin/write

include $(GMAKERULES)
