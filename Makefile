# Makefile

# C compiler
CC  = gcc
# C preprocessor 
CPP =
# C++ compiler 
CXX = g++
CFLAGS   = -I$(BUILD_INC_DIR) --std=c99 -O1
# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CXXFLAGS = -I$(BUILD_INC_DIR) --std=c++11 -O1
# C++ preprocessor flags
CPPFLAGS =
# library flags
LDFLAGS  = -lsqlite3
# define flags
DFLAGS   =

.DELETE_ON_ERROR:
.PHONY: all pre test clean_all
.DEFAULT_GOAL: $(tests)

headers := std_version.hpp exception.hpp throwable.hpp sqlite.hpp object.hpp
modules := sqlite.cpp
tests   := test1

test: $(tests)

test_obj := $(addsuffix .o,$(test))
obj      := sqlite.o

tell:
	@echo $(test_obj)

$(filter %.o,$(test_obj)): %.o: %.cpp $(modules) $(headers)
	$(CXX) -o $@ -c $< $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(DFLAGS)

$(filter %.o,$(obj)): %.o: %.cpp $(modules) $(headers)
	$(CXX) -o $@ -c $< $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(DFLAGS)

test%: test%.o $(obj)
	$(CXX) -o $@ $@.o $(obj) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(DFLAGS)

clean:
	-rm *.o
	-rm $(tests)