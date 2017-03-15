#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>
#include <map>

#include "../runtime-cpp.h"

#include "./cgen-out.incl"

int main() {
  using namespace std;

  pgen::runtime::init_mem_state();
  
  pgen::runtime::bmain_state_t s;
  pgen::runtime::init_bmain(s);
  s.call.valid = true;
  s.call.live = NULL;
  
  for (unsigned i = 0; i < 10000; ++i) {
    // Should always be ready for the main function to return.
    s.ret.valid = false;
    
    cout << "=== cycle " << i << " ===" << endl;
    pgen::runtime::tick_bmain(s);

    if (s.ret.valid) break;
  }
  
  return 0;
}
