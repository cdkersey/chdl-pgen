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

using namespace pgen;

void macro3(if_prog *pp) {

  // Initialize the assembler.
  if_prog &p(*pp);
  asm_prog a(p);
  init_macro_env(a);
  
  // The assembly program.
  function("bmain");
  label("entry");
  
  var x(u(5)), y(u(32));
  var done(bit());

  x = lit(u(5), 0);
  y = lit(u(32), 0);

  label("loop");
  spawn("print_hex")(y);
  var y_array(sa(bit(), 32));
  y_array = y;
  y_array = repl(y_array, x, !load(y_array, x));
  y = y_array;
  
  x = x + lit(u(5), 1);
  br("loop");

  //label("exit");
  //ret();

  finish_macro_env();
}

REGISTER_TEST(macro3, macro3);
