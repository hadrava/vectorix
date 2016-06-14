all: lib/libpotrace.a include/potracelib.h

potrace-1.12.tar.gz:
	wget http://potrace.sourceforge.net/download/1.12/potrace-1.12.tar.gz

potrace-1.12: potrace-1.12.tar.gz
	rm -rf $@
	tar -xvf $<

potrace-1.12/src/.libs/libpotrace.a: potrace-1.12
	cd $<; ./configure
	cd $<; make
	touch $@

lib/libpotrace.a: potrace-1.12/src/.libs/libpotrace.a
	mkdir -p lib
	cp $< $@

include/potracelib.h: potrace-1.12/src/potracelib.h
	mkdir -p include
	cp $< $@

clean:
	rm -rf potrace-1.12 lib include

.PHONY: all clean
