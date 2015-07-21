all:
	@echo "Specify OpenCV version:"
	@echo "make v-2.4.11"
	@echo "make v-3.0.0"

v-%: lib-% include-% opencv-%.zip

include-%: opencv-%
	cp -r $</include $@
	cp -r $</modules/*/include/* $@/
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

.PHONY: all clean v-3.0.0 v-2.4.11
