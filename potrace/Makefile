all: lib/libpotrace.a include/potracelib.h

potrace-1.13.tar.gz:
	wget http://potrace.sourceforge.net/download/1.13/potrace-1.13.tar.gz

potrace-1.13: potrace-1.13.tar.gz
	rm -rf $@
	tar -xvf $<

potrace-1.13/src/.libs/libpotrace.a: potrace-1.13
	cd $<; ./configure
	cd $<; make
	touch $@

lib/libpotrace.a: potrace-1.13/src/.libs/libpotrace.a
	mkdir -p lib
	cp $< $@

include/potracelib.h: potrace-1.13/src/potracelib.h
	mkdir -p include
	cp $< $@

clean:
	rm -rf potrace-1.13 lib include

.PHONY: all clean
