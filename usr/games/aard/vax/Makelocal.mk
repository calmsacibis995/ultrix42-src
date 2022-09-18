#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUTS= aard

aard:
#       ddl build currently dead...Ricky Palmer 9/13/84
#       ../ddl/ddlcomp aard < aard.ddl > ddlcomp.out

install:
	install aard ${DESTROOT}/usr/games/lib/aard
	install -c -m 755 frontend ${DESTROOT}/usr/games/aardvark


# All vars are set above.
# Include rules file that will perform operations based on those vars.
include $(GMAKERULES)
