CXXFLAGS += -std=c++11 -g -fmax-errors=1
LDLIBS += -lchdl -pthread

chdl-example: chdl-example.cpp cgen-out.incl print_hex.h
	$(CXX) -o chdl-example $(CXXFLAGS) chdl-example.cpp $(LDLIBS)

clean:
	$(RM) chdl-example *~
