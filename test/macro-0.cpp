#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

#include "../type.h"
#include "../if.h"
#include "../cgen.h"
#include "../cgen-cpp.h"
#include "../break_cycles.h"
#include "../asm-macro.h"

#include "testgen.h"

void macro0(pgen::if_prog *pp) {
  using namespace pgen;

  // Initialize the assembler.
  if_prog &p(*pp);
  asm_prog a(p);
  init_macro_env(a);
  
  // The assembly program.
  function("bmain");
  label("entry");
  
  var x(u(32)), y(u(32));
  var done(bit());

  x = lit(u(32), 0); // Initialize counter.
  y = lit(u(32), 1); // Random seed.

  label("loop");
  spawn("print_hex")(y);
  y = lit(u(32), 1103515245) * y + lit(u(32), 12345);
  done = (x == lit(u(32), 100));
  x = x + lit(u(32), 1);
  br(done)("loop")("exit");

  label("exit");
  ret();

  finish_macro_env();
}

REGISTER_TEST(macro0, macro0);
