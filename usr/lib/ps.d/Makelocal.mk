#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

AOUTS=	ps_img ps_plot ps_text ps_ascii

OBJS=	ps_img.o ps_plot.o ps_text.o ps_ascii.o

DESTDIR=$(DESTROOT)/usr/lib/ps

LOADLIBES=-lm

install:
	-@if [ ! -d $(DESTDIR) ]; then \
		$(ECHO) "$(MKDIR) $(DESTDIR)"; \
		$(MKDIR) $(DESTDIR); \
		$(ECHO) "$(CHMOD) 755 $(DESTDIR)"; \
		$(CHMOD) 755 $(DESTDIR); \
		$(ECHO) "$(CHOWN) root $(DESTDIR)"; \
		$(CHOWN) root $(DESTDIR); \
		$(ECHO) "$(CHGRP) system $(DESTDIR)"; \
		$(CHGRP) system $(DESTDIR); \
	else \
		true; \
	fi
	$(INSTALL) -c -s -m 755 ps_img $(DESTDIR)/ps_img
	$(INSTALL) -c -s -m 755 ps_plot $(DESTDIR)/ps_plot
	$(INSTALL) -c -s -m 755 ps_text $(DESTDIR)/ps_text
	$(INSTALL) -c -s -m 755 ps_ascii $(DESTDIR)/ps_ascii

ps_img:		ps_img.o
ps_plot:	ps_plot.o
ps_text:	ps_text.o
ps_ascii:	ps_ascii.o
ps_img.o:	ps_img.c img_pro.h
ps_plot.o:	ps_plot.c plot_pro.h
ps_text.o:	ps_text.c
ps_ascii.o:	ps_ascii.c ascii_pro.h

#
#	PostScript prolog filter setup
#
img_pro.h: img_pro.ps
	sed -e 's/.*/"&",/' ../img_pro.ps > img_pro.h
plot_pro.h: plot_pro.ps
	sed -e 's/.*/"&",/' ../plot_pro.ps > plot_pro.h
ascii_pro.h: ascii_pro.ps
	sed -e 's/.*/"&",/' ../ascii_pro.ps > ascii_pro.h

include $(GMAKERULES)
