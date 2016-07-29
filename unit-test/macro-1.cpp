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
  static_var("a", a(u(32), 64));

  label("entry");
  //var xyzzy(struct_type().add_field("b", u(8)).add_field("a", u(4))), y(u(32)),
  //  z(u(16));
  var y(u(32));
  // xyzzy = lit(u(12), 0);
  y = lit(u(32), 123);
  // z = lit(u(16), 0xabcd);

  label("start");
  var x_val(u(32)), x_plus_1(u(32));

  x_val = load("x");
  x_plus_1 = x_val + lit(u(32), 1);
  y = y - x_val;

  bool found = false;
  
  // xyzzy = repl(xyzzy, "b", load(xyzzy, "b") + lit(u(8), 1));
  // xyzzy = repl(xyzzy, "a", load(xyzzy, "a") - lit(u(4), 1));
  // z = repl(z, lit(u(32), 0), load(xyzzy, "a"), lit(u(32), 4));

  store("a", load(x_val, lit(u(32), 0), 6), y);
  var a_val(u(32));
  a_val = load("a", load(x_val, lit(u(32), 0), 6));
  store("x", x_plus_1);

  br("start");
}

int main(int argc, char **argv) {
  if_prog p;
  asm_prog a(p);
  init_macro_env(a);

  bmain();
  
  finish_macro_env();

  print(std::cout, p);

  std::ofstream out_chdl("macro-1.chdl.cpp");
  gen_prog(out_chdl, p);

  std::ofstream out_sim("macro-1.sim.cpp");
  gen_prog_cpp(out_sim, p);

  return 0;
}
