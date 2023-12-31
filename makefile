# Compiler and compiler flags
CXX = g++
CXXFLAGS = -Wall -g

# Define target executable
TARGET = cartridge

# Define object files
OBJ = cartridge.o cpu.o memory.o

# Default target
all: $(TARGET)

# Rule to link object files into the final executable
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

# Rule to compile source files into object files
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $<

# Dependency rules for object files
cartridge.o: cartridge.cc cpu.hh
cpu.o: cpu.cc cpu.hh memory.hh
memory.o: memory.cc memory.hh

# Clean target for cleaning up
clean:
	rm -f $(TARGET) $(OBJ)

# PHONY targets
.PHONY: all clean
