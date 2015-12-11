OBJ=$(patsubst %.cc, %.o, $(wildcard *.cc))
DEPS=$(patsubst %.cc, %.d, $(wildcard *.cc))
CXXFLAGS=-O3 -g -Wall
TARGET=asmtool

all: $(DEPS) $(TARGET)

-include $(DEPS)

$(TARGET): ${OBJ}
	g++ -o $@ ${OBJ}

%.d: %.cc
	g++ -MM -c $(CXXFLAGS) $< > $@

clean:
	rm -f $(TARGET) ${OBJ} $(DEPS)

.PHONY: clean
