#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	chgrp

OBJS=	chgrp.o

chgrp:	chgrp.o
chgrp.o:	chgrp.c

install:
	$(INSTALL) -c -s -m 4755 chgrp $(DESTROOT)/usr/bin/chgrp
	$(RM) $(DESTROOT)/bin/chgrp
	$(LN) -s ../usr/bin/chgrp $(DESTROOT)/bin/chgrp

include $(GMAKERULES)
