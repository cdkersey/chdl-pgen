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

using namespace bscotch;

void bmain() {
  function("bmain");

  static_var("x", u(32), 0);

  var i(u(32));
  
  label("entry");

  i = lit(u(32), 0);
  
  label("loop");
  var sub_enable(load(i, lit(u(5), 0))), n_sub_enable(!sub_enable);

  store("x", load("x") - i); pred(sub_enable);
  store("x", load("x") + i); pred(n_sub_enable);

  i = i + lit(u(32), 1);

  br("loop");
}

int main(int argc, char **argv) {
  if_prog p;
  asm_prog a(p);
  init_macro_env(a);

  bmain();
  
  finish_macro_env();

  print(std::cout, p);

  std::ofstream out_chdl("macro-5.chdl.cpp");
  gen_prog(out_chdl, p);

  std::ofstream out_sim("macro-5.sim.cpp");
  gen_prog_cpp(out_sim, p);

  return 0;
}
