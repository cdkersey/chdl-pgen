CXXFLAGS ?= -std=c++11 -g
TESTS = cgen-0 cgen-1 cgen-2 cgen-3 cgen-4 type-0
LDLIBS = ../bscotch.a

tests: $(TESTS)

type-0: type-0.cpp
cgen-0: cgen-0.cpp
cgen-1: cgen-1.cpp
cgen-2: cgen-2.cpp
cgen-3: cgen-3.cpp
cgen-4: cgen-4.cpp

clean:
	$(RM) $(TESTS) *~
