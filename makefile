# Makefile for PPFC project

# Compiler
CC = g++

# Compiler flags
CFLAGS = -std=c++11 -Wall -O1 -g

# Include directories
ifeq ($(OS),Windows_NT)
	IDIR = -Ilib/x86_64-w64-mingw32/include/SDL2
else
	IDIR = -I/usr/local/include
endif

# Library directories
ifeq ($(OS),Windows_NT)
	LDIR = -Llib/x86_64-w64-mingw32/lib
else
	LDIR = -L/usr/local/lib
endif

# Libraries to link
LIBS = -lSDL2 -lpthread

# Source files
SRCS = src/cartridge.cpp \
       src/cpu.cpp \
       src/keyboard.cpp \
       src/main.cpp \
       src/mapper.cpp \
       src/memory.cpp \
       src/ppfc.cpp \
       src/ppu.cpp \
       src/apu.cpp \
       src/screen.cpp \
       src/speaker.cpp \
       src/plugin_save_load.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Target
ifeq ($(OS),Windows_NT)
	TARGET = ppfc.exe
else
	TARGET = ppfc
endif

ifeq ($(OS),Windows_NT)
	FixPath = $(subst /,\,$(addprefix ",$(addsuffix ",$1)))
else
	FixPath = $1
endif

# Build rules
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDIR) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) $(IDIR) -c $< -o $@

clean:
	$(RM) $(call FixPath,$(OBJS)) $(TARGET)

.PHONY: clean

test:
	$(CC) $(CFLAGS) src/test.cpp -o test.exe

audio_test:
	$(CC) $(CFLAGS) src/audio_test.cpp -o audio_test.exe $(IDIR) $(LDIR) $(LIBS)
