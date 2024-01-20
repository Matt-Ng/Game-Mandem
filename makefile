# Compiler settings - Can change to clang++ if desired
CXX = g++
SDL = -framework SDL2

# Compiler flags
CXXFLAGS = -Wall -Wextra -std=c++17 -F /Library/Frameworks
LDFLAGS = -framework SDL2 -F /Library/Frameworks -I ~/Library/Frameworks/SDL2.framework/Headers


# Build target executable:
TARGET = gameboy

# Source files
SOURCES = gameboy.cc cpu.cc memory.cc interrupt.cc timer.cc cartridge.cc ppu.cc

# Object files
OBJECTS = $(SOURCES:.cc=.o)

# Header files
HEADERS = cpu.hh memory.hh interrupt.hh timer.hh cartridge.hh ppu.hh

# Default target
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

# Individual source files
gameboy.o: gameboy.cc cpu.hh memory.hh interrupt.hh timer.hh cartridge.hh ppu.hh

cpu.o: cpu.cc cpu.hh memory.hh interrupt.hh timer.hh

memory.o: memory.cc memory.hh cartridge.hh

interrupt.o: interrupt.cc interrupt.hh memory.hh

timer.o: timer.cc timer.hh memory.hh interrupt.hh

cartridge.o: cartridge.cc cartridge.hh

ppu.o: ppu.cc ppu.hh memory.hh interrupt.hh

# Clean target
clean:
	rm -f $(TARGET) $(OBJECTS)

# Prevent make from doing something with a file named clean
.PHONY: clean
