OBJ=$(patsubst %.cc, %.o, $(wildcard *.cc))
DEPS=$(patsubst %.cc, %.d, $(wildcard *.cc))
CXXFLAGS=-O3 -g -Wall -std=c++11 -flto
TARGET=asmtool
INSTALLDIR ?= $(HOME)/bin/

all: $(DEPS) $(TARGET)

-include $(DEPS)

$(TARGET): ${OBJ}
	g++ -flto -o $@ ${OBJ}

%.d: %.cc
	g++ -MM -c $(CXXFLAGS) $< > $@

install: $(TARGET)
	mkdir -p $(INSTALLDIR)
	install -b -m 755 $(TARGET) $(INSTALLDIR)

clean:
	rm -f $(TARGET) ${OBJ} $(DEPS)

.PHONY: clean

