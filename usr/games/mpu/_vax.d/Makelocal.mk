#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)


all:mpu

mpu:	constants locales locnames mpu.ddl objects routines transitions verbs
#	mv mpu mpu.old
#	../ddl/ddlcomp mpu < mpu.ddl > ddlcomp.out

install:
#	install mpu ${DESTROOT}/usr/games/lib/mpu
#	install frontend ${DESTROOT}/usr/games/mpu

clean:
	rm -f ddlcomp.out

clobber: clean
	rm -f Makefile

sccsinfo:
	sccs info

sccsget:
	sccs get SCCS

.DEFAULT:
	sccs get $<
include $(GMAKERULES)
