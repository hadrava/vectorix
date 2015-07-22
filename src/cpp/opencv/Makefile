all:
	@echo "Specify OpenCV version:"
	@echo "make opencv-3.0.0.zip include-3.0.0 lib-3.0.0"
	@echo "make opencv-2.4.11.zip include-2.4.11 lib-2.4.11"

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

.PHONY: all clean
