CXXFLAGS ?= -std=c++11 -g -Wfatal-errors
TESTS = cgen-0 cgen-1 cgen-2 cgen-3 cgen-4 cgen-5 cgen-6 cgen-7 cgen-8 \
        cgen-9 type-0 asm-0
LDLIBS = ../bscotch.a

tests: $(TESTS)

type-0: type-0.cpp
cgen-0: cgen-0.cpp
cgen-1: cgen-1.cpp
cgen-2: cgen-2.cpp
cgen-3: cgen-3.cpp
cgen-4: cgen-4.cpp
cgen-5: cgen-5.cpp
cgen-6: cgen-6.cpp
cgen-7: cgen-7.cpp
cgen-8: cgen-8.cpp
cgen-9: cgen-9.cpp
asm-0: asm-0.cpp

clean:
	$(RM) $(TESTS) *~
