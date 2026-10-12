# ==============================================================================
# Makefile for cxx_sqlite3 library
# High level wrapper for sqlite3 with modern C++ (C++17 and beyond)
# (C) Copyright Yoel Monsalve 2025-2026.
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

# Optional local configuration file (- means silent; will NOT fail if missing)
-include config.mk

# Compiler and Preprocessor Flags
DEFINES  ?=
CPPFLAGS := -I$(INC_DIR) -I$(WSQLITE_DIR)/include $(DEFINES)
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
CFLAGS   ?= -std=c99 -Wall -Wextra -O2

# Dependency tracking flags (-MMD -MP)
DEPFLAGS = -MMD -MP

# Source files and targets
LIB_SRCS     := $(SRC_DIR)/sqlite.cpp
LIB_OBJS     := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(LIB_SRCS))

# If using embedded SQLite C source fallback
ifeq ($(USE_EMBEDDED_SQLITE),1)
    SQLITE_C_OBJ := $(BUILD_DIR)/sqlite3_c.o
    EXTRA_OBJS   += $(SQLITE_C_OBJ)
endif

# Test directories and sources
TESTS_DIR    := tests
CTEST_DIR    := $(TESTS_DIR)/ctest
DRAFT_DIR    := draft

# Test sources discovery (tests/ctest/*.cpp, tests/*.cpp, and root test*.cpp)
CTEST_SRCS   := $(sort $(wildcard $(CTEST_DIR)/*.cpp))
BARE_SRCS    := $(sort $(wildcard $(TESTS_DIR)/*.cpp))
DRAFT_SRCS   := $(wildcard $(DRAFT_DIR)/*.cpp)
ROOT_SRCS    := $(sort $(wildcard test*.cpp))

CTEST_BINS   := $(patsubst $(CTEST_DIR)/%.cpp,$(BUILD_DIR)/%$(BIN_EXT),$(CTEST_SRCS))
BARE_BINS    := $(patsubst $(TESTS_DIR)/%.cpp,$(BUILD_DIR)/%$(BIN_EXT),$(BARE_SRCS))
#DRAFT_BINS   := $(patsubst $(DRAFT_DIR)/%.cpp,$(BUILD_DIR)/$(DRAFT_DIR)/%.out,$(DRAFT_SRCS))
DRAFT_BINS   := $(DRAFT_SRCS:%.cpp=$(BUILD_DIR)/%.out)
ROOT_BINS    := $(patsubst %.cpp,$(BUILD_DIR)/%$(BIN_EXT),$(ROOT_SRCS))

TEST_BINS    := $(sort $(CTEST_BINS) $(BARE_BINS) $(ROOT_BINS))

# All object files for dependency generation
TEST_OBJS := $(patsubst $(BUILD_DIR)/%$(BIN_EXT),$(BUILD_DIR)/%.o,$(TEST_BINS))
DRAFT_OBJS:= $(DRAFT_BINS:%.out=%.o)
ALL_OBJS  := $(LIB_OBJS) $(TEST_OBJS) $(EXTRA_OBJS) $(DRAFT_OBJS)
DEPS      := $(ALL_OBJS:.o=.d)

.PHONY: all test test_catch2 test_raii clean clean_all dirs
.SECONDARY:

.DEFAULT_GOAL := all

all: dirs test

tell:
	@echo $(DRAFT_BINS)
	@echo $(DEPS)

dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/$(DRAFT_DIR)

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

# Test objects from tests/ctest/, tests/, or root
$(BUILD_DIR)/%.o: $(CTEST_DIR)/%.cpp | dirs
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TESTS_DIR)/%.cpp | dirs
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp | dirs
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $@

# Test binaries
$(BUILD_DIR)/%$(BIN_EXT): $(BUILD_DIR)/%.o $(LIB_OBJS) $(EXTRA_OBJS) $(SYMLINK_DEP)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

#$(BUILD_DIR)/$(DRAFT_DIR)/%$(BIN_EXT).out: $(BUILD_DIR)/%.o $(LIB_OBJS) $(EXTRA_OBJS) $(SYMLINK_DEP)
#	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

test: $(TEST_BINS)
	@echo "Running tests in $(BUILD_DIR)..."
	@for bin in $(TEST_BINS); do \
		echo "Executing $$bin:"; \
		$$bin || exit 1; \
	done

# Dedicated target to run Catch2/ctest suite specifically
test_catch2: $(CTEST_BINS)
	@echo "Running Catch2-style test suite(s)..."
	@for bin in $(CTEST_BINS); do \
		echo "Executing $$bin:"; \
		$$bin || exit 1; \
	done

test_object: $(BUILD_DIR)/test_object$(BIN_EXT)
	@echo "Executing $(BUILD_DIR)/test_object$(BIN_EXT):"
	@$(BUILD_DIR)/test_object$(BIN_EXT)

# Dedicated target to run bare RAII test specifically
test_raii: $(BUILD_DIR)/test_raii$(BIN_EXT)
	@echo "Executing $(BUILD_DIR)/test_raii$(BIN_EXT):"
	@$(BUILD_DIR)/test_raii$(BIN_EXT)

clean:
	-rm -rf $(BUILD_DIR)
	-rm -f *.o test1 test1.exe

clean_all: clean
	-rm -rf build wbuild

-include $(DEPS)
