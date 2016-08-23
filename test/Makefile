CXXFLAGS ?= -std=c++11 -g -Wfatal-errors
TEST_OBJS = asm-0.o asm-1.o asm-2.o macro-0.o macro-1.o macro-2.o macro-3.o \
            macro-4.o macro-5.o localspawn.o sel.o sum.o sort.o
LDFLAGS = -L../
LDLIBS = -lchdl-pgen

testgen: testgen.o $(TEST_OBJS)
	$(CXX) $(LDFLAGS) -o testgen testgen.o $(TEST_OBJS) $(LDLIBS)

testgen.o : testgen.cpp
asm-0.o : asm-0.cpp
asm-1.o : asm-1.cpp
asm-2.o : asm-2.cpp
macro-0.o : macro-0.cpp
macro-1.o : macro-1.cpp
macro-2.o : macro-2.cpp
macro-3.o : macro-3.cpp
macro-4.o : macro-4.cpp
macro-5.o : macro-5.cpp
localspawn.o : localspawn.cpp
sel.o : sel.cpp
sum.o : sum.cpp
sort.o : sort.cpp

clean:
	$(RM) $(TEST_OBJS) testgen.o testgen *.s *.chdl.cpp *.sim.cpp
