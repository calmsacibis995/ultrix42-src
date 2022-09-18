# @(#)Makelocal.mk	4.1	ULTRIX	7/2/90

include $(GMAKEVARS)

SUBDIRS=tabset

DESTLIST=$(DESTROOT)/etc

all:	termcap

termcap: ../reorder ../termcap.src
	$(RM) termcap.src
	$(CP) ../termcap.src termcap.src
	ex - termcap.src < ../reorder

install:
	$(INSTALL) -c -m 644 termcap $(DESTROOT)/etc/termcap

include $(GMAKERULES)
