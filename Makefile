OBJ=asmtool.o helper.o asmfile.o
CXXFLAGS=-O3 -g

all: asmtool

clean:
	rm -f parse ${OBJ}

asmtool: ${OBJ}
	g++ -o asmtool ${OBJ}

.PHONY: clean
