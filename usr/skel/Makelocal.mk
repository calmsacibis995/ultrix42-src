#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

FILESX = .cshrc .login 

FILES = .mailrc .profile .project

install:
	-if [ ! -d ${DESTROOT}/usr/skel ]; \
	then \
		mkdir ${DESTROOT}/usr/skel; \
		chmod 755 ${DESTROOT}/usr/skel; \
	else \
		true; \
	fi
	for i in ${FILES}; do \
		(install -c -m 744 ../$$i ${DESTROOT}/usr/skel/$$i); done

	for i in ${FILESX}; do \
		(install -c -m 755 ../$$i ${DESTROOT}/usr/skel/$$i); done

include $(GMAKERULES)
