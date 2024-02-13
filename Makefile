# Makefile

# C compiler
CC  = gcc
# C preprocessor 
CPP =
# C++ compiler 
CXX = g++
CFLAGS   = -I$(INC_DIR) --std=c99 -O1
# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CXXFLAGS = -I$(INC_DIR) --std=c++11 -O0
# C++ preprocessor flags
CPPFLAGS =
# library flags
LDFLAGS  = -lsqlite3
# define flags
DFLAGS   =

.DELETE_ON_ERROR:
.PHONY: all pre test clean_all
.DEFAULT_GOAL: all

INC_DIR := include
SRC_DIR := src

headers := cpp_version.hpp exception.hpp error_code.hpp throwable.hpp sqlite.hpp functions.hpp basic_types.hpp object.hpp
modules := sqlite.cpp
tests   := test1

all: test

test: $(tests)

tests_obj := $(addsuffix .o,$(tests))
obj       := sqlite.o

tell:
	@echo $(tests_obj)

$(filter %.o,$(tests_obj)): %.o: %.cpp $(SRC_DIR)/$(modules) $(INC_DIR)/*
	$(CXX) -o $@ -c $< $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(DFLAGS)

$(filter %.o,$(obj)): %.o: $(SRC_DIR)/%.cpp $(SRC_DIR)/$(modules) $(INC_DIR)/*
	$(CXX) -o $@ -c $< $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(DFLAGS)

#test%: test%.o $(obj)
#	$(CXX) -o $@ $@.o $(obj) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(DFLAGS)

%: %.o $(obj)
	$(CXX) -o $@ $@.o $(obj) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(DFLAGS)

clean:
	-rm *.o
	-rm $(tests)	