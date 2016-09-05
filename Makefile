all:
	clang -Ofast -Wall -dynamiclib -o libspatialhash.dylib spatialhash.c -lgeometry
install:
	mv libspatialhash.dylib /usr/lib
	cp spatialhash.h /usr/local/include
remove:
	rm /usr/lib/libspatialhash.dylib
	rm /usr/local/inclue/spatialhash.h
