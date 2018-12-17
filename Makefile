CPP=g++
CPPFLAGS=-g -std=c++11 -I.

all: wpc wpx

libcompiler.a: compiler/tinyast.h compiler/tinyparser.cpp compiler/tinyparser.h compiler/tinytokenizer.cpp compiler/tinytokenizer.h
	$(CPP) $(CPPFLAGS) -c compiler/tinyparser.cpp compiler/tinytokenizer.cpp
	ar -rv libcompiler.a tinyparser.o tinytokenizer.o

libwc.a: wc.h wc.cpp
	$(CPP) $(CPPFLAGS) -c wc.cpp
	ar -rv libwc.a wc.o

wpc: wpc.cpp we.h tinyformat.h libcompiler.a libwc.a
	$(CPP) $(CPPFLAGS) wpc.cpp wc.cpp -o wpc libcompiler.a libwc.a

wpx: wpx.cpp tinyformat.h sb.h libcompiler.a libwc.a
	$(CPP) $(CPPFLAGS) wpx.cpp -o wpx libcompiler.a libwc.a

install: wpc wpx
	cp wpc wpx /usr/local/bin
