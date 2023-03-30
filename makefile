# Makefile for PPFC project
# Using SDL2 library

# Compiler
CC = g++

# Compiler flags
CFLAGS = -std=c++11 -Wall -O1 -g

# Include directories
IDIR = -Ilib/x86_64-w64-mingw32/include/SDL2

# Library directories
LDIR = -Llib/x86_64-w64-mingw32/lib

# Libraries to link
LIBS = -lSDL2main -lSDL2

# Source files
SRCS = src/cartridge.cpp \
       src/cpu.cpp \
       src/keyboard.cpp \
       src/main.cpp \
       src/mapper.cpp \
       src/memory.cpp \
       src/ppfc.cpp \
       src/ppu.cpp \
       src/screen.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Target
TARGET = ppfc.exe

# Build rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDIR) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) $(IDIR) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
