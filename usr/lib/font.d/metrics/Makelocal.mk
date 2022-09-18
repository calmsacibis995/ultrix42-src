#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

#
# This makefile is for PostScript AFM files
#
AFMDIR = $(DESTROOT)/usr/lib/font/metrics

AFM1 = \
	encodefont.ps \
	avantgarde_book.afm \
	avantgarde_book_isolatin1.afm \
	avantgarde_bookoblique.afm \
	avantgarde_bookoblique_isolatin1.afm \
	avantgarde_demi.afm \
	avantgarde_demi_isolatin1.afm \
	avantgarde_demioblique.afm \
	avantgarde_demioblique_isolatin1.afm \
	courier.afm \
	courier_bold.afm \
	courier_bold_isolatin1.afm \
	courier_boldoblique.afm \
	courier_boldoblique_isolatin1.afm \
	courier_isolatin1.afm \
	courier_oblique.afm \
	courier_oblique_isolatin1.afm \
	helvetica.afm \
	helvetica_bold.afm \
	helvetica_bold_isolatin1.afm \
	helvetica_boldoblique.afm \
	helvetica_boldoblique_isolatin1.afm \
	helvetica_isolatin1.afm \
	helvetica_oblique.afm \
	helvetica_oblique_isolatin1.afm \
	lubalingraph_book.afm \
	lubalingraph_book_isolatin1.afm \
	lubalingraph_bookoblique.afm \
	lubalingraph_bookoblique_isolatin1.afm
AFM2= \
	lubalingraph_demi.afm \
	lubalingraph_demi_isolatin1.afm \
	lubalingraph_demioblique.afm \
	lubalingraph_demioblique_isolatin1.afm \
	newcenturyschlbk_bold.afm \
	newcenturyschlbk_bold_isolatin1.afm \
	newcenturyschlbk_bolditalic.afm \
	newcenturyschlbk_bolditalic_isolatin1.afm \
	newcenturyschlbk_italic.afm \
	newcenturyschlbk_italic_isolatin1.afm \
	newcenturyschlbk_roman.afm \
	newcenturyschlbk_roman_isolatin1.afm \
	souvenir_demi.afm \
	souvenir_demi_isolatin1.afm \
	souvenir_demiitalic.afm \
	souvenir_demiitalic_isolatin1.afm \
	souvenir_light.afm \
	souvenir_light_isolatin1.afm \
	souvenir_lightitalic.afm \
	souvenir_lightitalic_isolatin1.afm \
	symbol.afm \
	times_bold.afm \
	times_bold_isolatin1.afm \
	times_bolditalic.afm \
	times_bolditalic_isolatin1.afm \
	times_italic.afm \
	times_italic_isolatin1.afm \
	times_roman.afm \
	times_roman_isolatin1.afm

install:
	@-if [ ! -d $(DESTROOT)/usr/lib/font ]; then \
		mkdir $(DESTROOT)/usr/lib/font \
	else true; \
	fi
	@-if [ ! -d $(AFMDIR) ]; then \
		mkdir $(AFMDIR); \
	else true; \
	fi
	@for i in $(AFM1); do \
		($(ECHO) "$(INSTALL) -c -m 644 ../$$i $(AFMDIR)/$$i"; \
		$(INSTALL) -c -m 644 ../$$i $(AFMDIR)/$$i); \
	done
	@for i in $(AFM2); do \
		($(ECHO) "$(INSTALL) -c -m 644 ../$$i $(AFMDIR)/$$i"; \
		$(INSTALL) -c -m 644 ../$$i $(AFMDIR)/$$i); \
	done

include $(GMAKERULES)
