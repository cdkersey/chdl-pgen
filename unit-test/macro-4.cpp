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

type mem_req(int b, int n, int a, int i) {
  type t = struct_type().
    add_field("d", sa(u(b), n)).
    add_field("a", u(a)).
    add_field("mask", u(n)).
    add_field("wr", bit()).
    add_field("llsc", bit()).
    add_field("id", u(i));

  return t;
}

type mem_resp(int b, int n, int i) {
  type t = struct_type().
    add_field("q", sa(u(b), n)).
    add_field("llsc_suc", bit()).
    add_field("wr", bit()).
    add_field("id", u(i));
  return t;
}

void bmain() {
  type req(mem_req(B, N, A, I)), resp(mem_req(B, N, I));
  
  
  // Function bmain() : spawn 10 threads in tmain instance.
  function("bmain");
  var i(u(32));

  label("entry");

  i = lit(u(32), 0);
  
  label("loop");
  spawn("tmain")(i); // Must be last in basic block.

  label("loop2");
  i = i + lit(u(32), 1);
  br(i == lit(u(32), 10))("loop")("exit");
  
  label("exit");
  ret();
}

void tmain() {
  function("tmain");
  static_var("x", u(32), 0);

  var x(u(32)), tid(u(32));
  
  label("entry");
  tid = arg(u(32));
  x = tid;
  x = x + lit(u(32), 1);
  
  label("foo");
  x = x - lit(u(32), 1);
  store("x", tid);
  br(x == lit(u(32), 0))("foo")("exit");
  
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
