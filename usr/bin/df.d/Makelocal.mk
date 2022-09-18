#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	df

OBJS=	df.o

df:	df.o
df.o:	df.c

install:
	$(INSTALL) -c -s -m 755 df $(DESTROOT)/usr/bin/df
	$(RM) $(DESTROOT)/bin/df
	$(LN) -s ../usr/bin/df $(DESTROOT)/bin/df

include $(GMAKERULES)
