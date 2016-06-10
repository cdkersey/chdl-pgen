CXXFLAGS = -std=c++11
LDLIBS = -lchdl
OBJS = if.o type.o cgen.o
HEADERS = cgen.h type.h if.h

bscotch.a : $(OBJS)
	$(AR) q $@ $(OBJS)

cgen.o : cgen.cpp $(HEADERS)
type.o : type.cpp $(HEADERS)
if.o : if.cpp $(HEADERS)

clean:
	$(RM) $(OBJS) bscotch.a
