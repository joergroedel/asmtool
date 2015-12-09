OBJ=parse.o helper.o asmfile.o
CXXFLAGS=-O3 -g

all: parse

clean:
	rm -f parse ${OBJ}

parse: ${OBJ}
	g++ -o parse ${OBJ}

.PHONY: clean
