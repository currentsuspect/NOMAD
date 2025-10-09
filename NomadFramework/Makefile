# Nomad Framework Makefile

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -fPIC
INCLUDES = -Iinclude
LIBS = -lpthread

# Source files
SOURCES = $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

# Targets
LIBRARY = libnomadframework.a
TEST_EXECUTABLE = test_framework
EXAMPLE_EXECUTABLE = nomad_example

.PHONY: all clean test example

all: $(LIBRARY) $(TEST_EXECUTABLE) $(EXAMPLE_EXECUTABLE)

$(LIBRARY): $(OBJECTS)
	ar rcs $@ $^

$(TEST_EXECUTABLE): test_framework.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< -L. -lnomadframework $(LIBS)

$(EXAMPLE_EXECUTABLE): examples/NomadExample.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< -L. -lnomadframework $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

test: $(TEST_EXECUTABLE)
	./$(TEST_EXECUTABLE)

example: $(EXAMPLE_EXECUTABLE)
	./$(EXAMPLE_EXECUTABLE)

clean:
	rm -f $(OBJECTS) $(LIBRARY) $(TEST_EXECUTABLE) $(EXAMPLE_EXECUTABLE)

install: $(LIBRARY)
	cp $(LIBRARY) /usr/local/lib/
	cp -r include/nomad /usr/local/include/

help:
	@echo "Available targets:"
	@echo "  all      - Build library, test, and example"
	@echo "  test     - Build and run tests"
	@echo "  example  - Build and run example"
	@echo "  clean    - Remove build artifacts"
	@echo "  install  - Install library and headers"
	@echo "  help     - Show this help message"