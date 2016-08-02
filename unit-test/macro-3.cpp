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

int main(int argc, char **argv) {
  using namespace bscotch;

  // Initialize the assembler.
  if_prog p;
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

  print(std::cout, p);

  std::ofstream chdl_out("macro-3.chdl.cpp");
  gen_prog(chdl_out, p);

  std::ofstream cpp_out("macro-3.sim.cpp");
  gen_prog_cpp(cpp_out, p);

  return 0;
}
