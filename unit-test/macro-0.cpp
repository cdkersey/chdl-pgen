#include <iostream>

#include <vector>
#include <string>
#include <sstream>

#include "../type.h"
#include "../if.h"
#include "../cgen.h"
#include "../break_cycles.h"
#include "../asm-macro.h"

int main(int argc, char **argv) {
  using namespace bscotch;

  // Initialize the assembler.
  if_prog p;
  asm_prog a(p);
  init_macro_env(a);
  
  // The assembly program.
  function("bmain");
  label("entry");
  
  var x(u(32));//, y(u(32));
  var done(bit());

  x = lit(u(32), 0); // Initialize counter.
  // y = lit(u(32), 1); // Random seed.

  label("loop");
  // y = lit(u(32), 1103515245) * y + lit(u(32), 12345);
  done = (x == lit(u(32), 100));
  x = x + lit(u(32), 1);
  br(done)("loop")("exit");

  label("exit");
  ret();

  finish_macro_env();
  
  print(std::cout, p); 
  gen_prog(std::cout, p);

  return 0;
}
