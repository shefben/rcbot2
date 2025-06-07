# Makefile for Source Engine Plugin

# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++11 -shared -fPIC -Wall -Wextra -Wno-unused-parameter -D LINUX -DNDEBUG -DTARGET_POSIX -D__x86_64__
# Add -m32 for 32-bit builds if necessary, depending on the game
# CXXFLAGS += -m32

# Source files
SOURCES := main.cpp

# Include paths - Adjust these to your Source SDK location
SDK_INCLUDE_DIR := ./public
TIER1_INCLUDE_DIR := ./public/tier1
COMMON_INCLUDE_DIR := ./public/common

INCLUDES := -I$(SDK_INCLUDE_DIR) -I$(TIER1_INCLUDE_DIR) -I$(COMMON_INCLUDE_DIR)

# Output
OUTPUT_DIR := ./
OUTPUT_NAME := my_plugin.so # Change this to your desired plugin name

# Object files
OBJECTS := $(SOURCES:.cpp=.o)

# Targets
all: $(OUTPUT_DIR)/$(OUTPUT_NAME)

$(OUTPUT_DIR)/$(OUTPUT_NAME): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(OUTPUT_DIR)/$(OUTPUT_NAME)

.PHONY: all clean
