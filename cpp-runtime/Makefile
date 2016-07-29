CXXFLAGS += -std=c++11 -fmax-errors=1 -O3

cpp-example: cpp-example.cpp cgen-out.incl
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o cpp-example cpp-example.cpp $(LDLIBS)

clean:
	$(RM) cpp-example *~
