#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

FILES = apl basker.b basker.i basker.r bocklin bodoni.b bodoni.i \
	bodoni.r chess clarendon cm.b cm.i cm.r countdown cyrillic \
	delegate.b delegate.i delegate.r fix gacham.b gacham.i \
	gacham.r graphics greek h19 hebrew meteor.b meteor.i \
	meteor.r mona nonie.b nonie.i nonie.r oldenglish otimes.b \
	otimes.i otimes.r pip playbill script seal shadow sign \
	stare.b stare.i stare.r times.b times.i times.r ugramma \
	wumpus xx

DESTLIST=${DESTROOT}/usr/lib/fontinfo

install:
	@for i in ${FILES}; do \
		$(INSTALL) -c ../$$i ${DESTLIST}/$$i; \
	done

include $(GMAKERULES)
