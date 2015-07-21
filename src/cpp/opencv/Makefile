all: lib-3.0.0 include-3.0.0 lib-2.4.11 include-2.4.11

include-%: opencv-%
	cp -r $</include $@
	rm -f $@/CMakeLists.txt

lib-%: build-%
	cp -r $</lib $@

build-%: opencv-%
	rm -rf $@
	mkdir $@
	cd $@; cmake ../$<
	cd $@; make

opencv-%: opencv-%.zip
	unzip $<

opencv-%.zip:
	wget https://github.com/Itseez/opencv/archive/$*.zip -O $@

clean:
	rm -rf lib-* include* build-* opencv-*/

.PHONY: all clean
