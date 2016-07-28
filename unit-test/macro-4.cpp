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
  // Function bmain() : spawn 10 threads in tmain instance.
  function("bmain");
  var i(u(32));

  label("entry");

  i = lit(u(32), 0);
  
  label("loop");
  call("tmain")(i); // Must be last in basic block.

  label("loop2");
  i = i + lit(u(32), 1);
  br(i == lit(u(32), 10))("loop")("exit");
  
  label("exit");
  ret();
}

void tmain() {
  function("tmain");
  static_var("x", u(32), 0);

  label("foo");
  store("x", arg(u(32)));
  
  
  label("exit");
  ret();
}

int main(int argc, char **argv) {
  using namespace std;

  if_prog p;
  asm_prog a(p);
  init_macro_env(a);

  bmain();
  tmain();

  finish_macro_env();

  print(std::cout, p);

  ofstream chdl_out("macro-4.chdl.cpp");
  gen_prog(chdl_out, p);

  ofstream cpp_out("macro-4.sim.cpp");
  gen_prog_cpp(cpp_out, p);

  return 0;
}
