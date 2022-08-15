# 
# A very simple makefile to do simple tasks. No automake.
#
PACKAGE_VERSION="0.0.3"

all: 
	@echo "Nothing to do with 'all'."

install:
	#install -m0755 pip-*[-a-z] utilityfunctions.sh $(DESTDIR)/usr/bin/
	#install -m0755 pip-*.1 $(DESTDIR)/usr/share/man/man1/

dist: clean
	VERSION=${PACKAGE_VERSION}; \
	DIRNAME=$$(basename ${PWD}); \
	cd ..; \
	tar cvzf $${DIRNAME}-$${VERSION}.tar.gz $${DIRNAME}

clean:
	rm -rvf tmp
