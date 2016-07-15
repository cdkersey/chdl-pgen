#include <iostream>

#include <vector>
#include <string>
#include <sstream>

#include "../type.h"
#include "../if.h"
#include "../cgen.h"
#include "../break_cycles.h"
#include "../asm-macro.h"

using namespace bscotch;
using namespace std;

int main(int argc, char **argv) {
  // Initialize the assembler.
  if_prog p;
  asm_prog a(p);
  init_macro_env(a);
  
  // The assembly program.
  function("bmain");
  label("entry");
  
  var x(u(32)), y(u(32));
  var done(bit());

  x = 0_c; // Initialize counter.
  y = 0_c; // Random seed.

  label("loop");
  y = 1103515245_c * y + 12345_c;
  done = (x == 100_c);
  x = x + 1_c;
  br(done)("loop")("exit");

  label("exit");
  ret();

  finish_macro_env();
  
  // print(cout, p); 
  gen_prog(cout, p);

  return 0;
}
