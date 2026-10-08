# ==============================================================================
# Makefile for cxx_sqlite3 library
# High level wrapper for sqlite3 with modern C++ (C++11 and beyond)
# (C) Copyright Yoel Monsalve 2019-2026.
# ==============================================================================

CC       ?= gcc
CXX      ?= g++
AR       ?= ar

# Directories
INC_DIR     := include
SRC_DIR     := src
WSQLITE_DIR := wsqlite

# Platform detection
ifeq ($(OS),Windows_NT)
    PLATFORM  := Windows
    BUILD_DIR ?= wbuild
    BIN_EXT   := .exe
    LDFLAGS   += -L$(WSQLITE_DIR)/lib -lsqlite3
else
    PLATFORM  := Linux
    BUILD_DIR ?= build
    BIN_EXT   :=
    HAVE_SYSTEM_SQLITE := $(shell printf 'int main(){}' | gcc -lsqlite3 -x c - -o /dev/null 2>/dev/null && echo 1 || echo 0)
    ifeq ($(HAVE_SYSTEM_SQLITE),1)
        LDFLAGS += -lsqlite3
    else
        # Find system runtime library to link against via build symlink
        SYSTEM_SQLITE_SO := $(firstword $(wildcard /usr/lib/*/libsqlite3.so.0 /usr/lib/libsqlite3.so.0 /lib/*/libsqlite3.so.0 /lib/libsqlite3.so.0))
        ifneq ($(SYSTEM_SQLITE_SO),)
            USE_SO_SYMLINK := 1
            LDFLAGS += -L$(BUILD_DIR) -lsqlite3 -Wl,-rpath,'$$ORIGIN'
        else
            USE_EMBEDDED_SQLITE := 1
        endif
    endif
endif

# Compiler and Preprocessor Flags
CPPFLAGS := -I$(INC_DIR) -I$(WSQLITE_DIR)/include
CXXFLAGS ?= -std=c++11 -Wall -Wextra -O2
CFLAGS   ?= -std=c99 -Wall -Wextra -O2

# Dependency tracking flags (-MMD -MP)
DEPFLAGS = -MMD -MP

# Source files and targets
LIB_SRCS := $(SRC_DIR)/sqlite.cpp
LIB_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(LIB_SRCS))

# If using embedded SQLite C source fallback
ifeq ($(USE_EMBEDDED_SQLITE),1)
    SQLITE_C_OBJ := $(BUILD_DIR)/sqlite3_c.o
    EXTRA_OBJS   += $(SQLITE_C_OBJ)
endif

TEST_SRCS := $(sort $(wildcard test*.cpp))
TEST_BINS := $(patsubst %.cpp,$(BUILD_DIR)/%$(BIN_EXT),$(TEST_SRCS))

# All object files for dependency generation
ALL_OBJS := $(LIB_OBJS) $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(TEST_SRCS)) $(EXTRA_OBJS)
DEPS     := $(ALL_OBJS:.o=.d)

.PHONY: all test clean clean_all dirs
.SECONDARY:

.DEFAULT_GOAL := all

all: dirs test

dirs:
	@mkdir -p $(BUILD_DIR)

# Symlink rule for Linux runtime libsqlite3 if -dev package is not present
ifeq ($(USE_SO_SYMLINK),1)
$(BUILD_DIR)/libsqlite3.so: | dirs
	@ln -sf $(SYSTEM_SQLITE_SO) $@
SYMLINK_DEP := $(BUILD_DIR)/libsqlite3.so
endif

# Embedded sqlite3.c compilation rule (if needed)
$(BUILD_DIR)/sqlite3_c.o: $(WSQLITE_DIR)/src/sqlite3.c | dirs
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $@

# Library module objects
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | dirs
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $@

# Test objects
$(BUILD_DIR)/%.o: %.cpp | dirs
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $@

# Test binaries
$(BUILD_DIR)/%$(BIN_EXT): $(BUILD_DIR)/%.o $(LIB_OBJS) $(EXTRA_OBJS) $(SYMLINK_DEP)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

test: $(TEST_BINS)
	@echo "Running tests in $(BUILD_DIR)..."
	@for bin in $(TEST_BINS); do \
		echo "Executing $$bin:"; \
		$$bin || exit 1; \
	done

clean:
	-rm -rf $(BUILD_DIR)
	-rm -f *.o test1 test1.exe

clean_all: clean
	-rm -rf build wbuild

-include $(DEPS)