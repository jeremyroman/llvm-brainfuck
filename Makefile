PROGRAM := llvm-brainfuck
OBJECTS := main.o

SHIM := shim.a
SHIM_OBJECTS := shim.o

CC := clang
CXX := clang++
CXXFLAGS := $(shell llvm-config --cppflags) -Wall -Werror -pedantic
LDFLAGS := $(shell llvm-config --ldflags --libs core)

all: $(PROGRAM) $(SHIM)

$(PROGRAM): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(SHIM): $(SHIM_OBJECTS)
	ar rcs $@ $^

clean:
	rm -f $(PROGRAM) $(OBJECTS)

.PHONY: clean all
