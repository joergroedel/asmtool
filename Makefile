OBJ=$(patsubst %.cc, %.o, $(wildcard *.cc))
CXXFLAGS=-O3 -g
TARGET=asmtool

all: $(TARGET)

$(TARGET): ${OBJ}
	g++ -o $@ ${OBJ}

clean:
	rm -f $(TARGET) ${OBJ}

.PHONY: clean
