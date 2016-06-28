OS=`uname``uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/'`
GCC=`gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/'`

PACKEGE_NAME=XLOG_$(OS)_Gcc$(GCC)_X86
all:
	cd ./log4cplus-1.0.3;./configure && make;
	cd ./xlog;make
	cd ./test;make
clean:
	cd ./log4cplus-1.0.3;make clean
	cd ./xlog;make clean
	cd ./test;make clean
binzip:
	mkdir -p release
	mkdir -p release/include
	mkdir -p release/$(OS)_Gcc$(GCC)
	cd ./release;rm -rf $(PACKEGE_NAME).zip
	cp -r ./ChangeLog.txt ./release/
	cp -r ./xlog/lib/* ./release/$(OS)_Gcc$(GCC)/
	cp -r ./xlog/ILog.h ./release/include/
	cp -r ./xlog/LogManager.h ./release/include/
	cd ./release;zip -r $(PACKEGE_NAME).zip *
	
