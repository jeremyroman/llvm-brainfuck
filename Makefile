PROGRAM := llvm-brainfuck
OBJECTS := main.o

CXX := clang++
CXXFLAGS := $(shell llvm-config --cppflags)
LDFLAGS := $(shell llvm-config --ldflags --libs core)

all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(PROGRAM) $(OBJECTS)

.PHONY: clean all
