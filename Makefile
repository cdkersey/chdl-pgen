CXXFLAGS = -std=c++11
LDLIBS = -lchdl
OBJS = if.o type.o cgen.o find_back_edges.o break_cycles.o
HEADERS = cgen.h type.h if.h find_back_edges.h break_cycles.h

bscotch.a : $(OBJS)
	$(AR) q $@ $(OBJS)

cgen.o : cgen.cpp $(HEADERS)
type.o : type.cpp $(HEADERS)
if.o : if.cpp $(HEADERS)
find_back_edges.o : find_back_edges.cpp $(HEADERS)
break_cycles.o : break_cycles.cpp $(HEADERS)

clean:
	$(RM) $(OBJS) bscotch.a
