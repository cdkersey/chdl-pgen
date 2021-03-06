CXXFLAGS ?= -std=c++11 -g -Wfatal-errors -fPIC
LDFLAGS ?=
LDLIBS ?=
PREFIX ?= /usr/local
OBJS = if.o type.o cgen.o find_back_edges.o break_cycles.o asm.o asm-macro.o \
       prevent_deadlock.o cgen-cpp.o convert_phis.o remove_copies.o
HEADERS = cgen.h type.h if.h find_back_edges.h break_cycles.h asm.h asm-macro.h\
       prevent_deadlock.h cgen-cpp.h convert_phis.h remove_copies.h

all : libchdl-pgen.a libchdl-pgen.so

install:
	cp libchdl-pgen.so $(PREFIX)/lib
	mkdir -p $(PREFIX)/include/chdl/pgen
	cp *.h $(PREFIX)/include/chdl/pgen

libchdl-pgen.so : $(OBJS)
	$(CXX) -o libchdl-pgen.so $(LDFLAGS) -shared $(OBJS) $(LDLIBS)

libchdl-pgen.a : $(OBJS)
	$(AR) q $@ $(OBJS)

asm.o : asm.cpp $(HEADERS)
asm-macro.o : asm.cpp $(HEADERS)
cgen.o : cgen.cpp $(HEADERS)
cgen-cpp.o : cgen-cpp.cpp $(HEADERS)
type.o : type.cpp $(HEADERS)
if.o : if.cpp $(HEADERS)
remove_copies.o : remove_copies.cpp $(HEADERS)
convert_phis.o : convert_phis.cpp $(HEADERS)
find_back_edges.o : find_back_edges.cpp $(HEADERS)
break_cycles.o : break_cycles.cpp $(HEADERS)
prevent_deadlock.o : prevent_deadlock.cpp $(HEADERS)

clean:
	$(RM) $(OBJS) libchdl-pgen.a libchdl-pgen.so
