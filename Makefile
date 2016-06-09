CXXFLAGS = -std=c++11
LDLIBS = -lchdl
OBJS = if.o type.o cgen.o

bscotch.a : $(OBJS)
	$(AR) q $@ $(OBJS)

cgen.o : cgen.cpp
type.o : type.cpp
if.o : if.cpp

clean:
	$(RM) $(OBJS) bscotch.a
