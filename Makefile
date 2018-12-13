CPP=g++
CPPFLAGS=-std=c++11 -I.

all: wpc wpx

libcompiler.a: compiler/tinyast.h compiler/tinyparser.cpp compiler/tinyparser.h compiler/tinytokenizer.cpp compiler/tinytokenizer.h
	$(CPP) $(CPPFLAGS) -c compiler/tinyparser.cpp compiler/tinytokenizer.cpp
	ar -rv libcompiler.a tinyparser.o tinytokenizer.o

wpc: wpc.cpp we.h wc.h tinyformat.h libcompiler.a
	$(CPP) $(CPPFLAGS) libcompiler.a wpc.cpp -o wpc

wpx: wpx.cpp wc.h tinyformat.h libcompiler.a
	$(CPP) $(CPPFLAGS) libcompiler.a wpx.cpp -o wpx

