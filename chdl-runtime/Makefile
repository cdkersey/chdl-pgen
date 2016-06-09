CXXFLAGS = -std=c++11
LDLIBS = -lchdl

chdl-example: chdl-example.cpp cgen-out.incl
	$(CXX) -o chdl-example $(CXXFLAGS) chdl-example.cpp $(LDLIBS)

clean:
	$(RM) chdl-example *~
