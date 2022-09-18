#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib/term

FILES=	example tab300 tab300-12 tab300X tab300s tab300s-12 tab302 \
	tab302-12 tab37 tab382 tab382-12 tab450 tab450-12 tab450-12-8 \
	tab450X tab833 tab833-12 tabdtc tabdtc12 tabipsi tabipsi12 \
	tabitoh tabitoh12 tablpr tabnec tabnec-t tabnec12 tabqume \
	tabqume12 tabtn300 tabx-ecs tabx-ecs12 tabx1700 tabxerox \
	tabxerox12

install:
	$(CD) ..; $(CP) $(FILES) $(DESTROOT)/usr/lib/term

include $(GMAKERULES)
