#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=   mv

OBJS=   mv.o

mv.o:   mv.c

#	take care with mv; install uses it!

install:
	$(RM) $(DESTROOT)/bin/mv; $(CP) mv $(DESTROOT)/bin/mv; \
		chmod 755 $(DESTROOT)/bin/mv; \
		$(STRIP) $(DESTROOT)/bin/mv

include $(GMAKERULES)
