# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
INC = -Iinclude

# Linker flags and libraries
LDFLAGS =
LIBS = -lssl -lcrypto -lsqlite3 -lpthread

# Project name
TARGET = file_crypter

# --- Automatic File Discovery ---
# Find all .cpp files in src and its subdirectories
SRCS = $(shell find src -name '*.cpp')

# Generate object file names by replacing src/ with obj/ and .cpp with .o
OBJS = $(patsubst src/%.cpp, obj/%.o, $(SRCS))

# --- Rules ---
all: $(TARGET)

# Link the program
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

# Compile source files into object files
# This rule creates the object directories (e.g., obj/crypto/) first
obj/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

# Clean up build files
clean:
	@echo "Cleaning up..."
	rm -rf obj $(TARGET)

# Phony targets
.PHONY: all clean