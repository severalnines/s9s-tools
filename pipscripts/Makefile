# 
# A very simple makefile to do simple tasks. No automake.
#
PACKAGE_VERSION="0.0.2"

all: 
	@echo "Nothing to do with 'all'."

install:
	install -m0755 pip-* $(DESTDIR)/usr/bin

dist: clean
	VERSION=${PACKAGE_VERSION}; \
	DIRNAME=$$(basename ${PWD}); \
	cd ..; \
	tar cvzf $${DIRNAME}-$${VERSION}.tar.gz $${DIRNAME}

clean:
	rm -rvf tmp
