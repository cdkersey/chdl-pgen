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

void bmain() {
  // Function bmain() : spawn 5 threads in tmain instance.
  function("bmain");
  var i(u(32));

  label("entry");

  i = lit(u(32), 0);
  
  label("loop");
  spawn("tmain")(i);

  label("loop2");
  i = i + lit(u(32), 1);
  br(i == lit(u(32), 5))("loop")("exit");
  
  label("exit");
  ret();
}

void tmain() {
  function("tmain");
  label("foo");
  var i(u(32)), j(u(32));
  i = arg(u(32));
  i = i * i;

  j = lit(u(32), 0);
  
  label("loop");
  j = j + lit(u(32), 1);
  br(j == lit(u(32), 3))("loop")("loopx");

  label("loopx");
  j = lit(u(32), 0);
  br(i == lit(u(32), 0))("loop2")("exit");

  label("loop2");
  i = i - lit(u(32), 1);
  br("loop");
  
  label("exit");
  ret();
}

int main(int argc, char **argv) {
  if_prog p;
  asm_prog a(p);
  init_macro_env(a);

  bmain();
  tmain();
  
  finish_macro_env();

  // print(std::cout, p); 
  gen_prog(std::cout, p);

  return 0;
}
