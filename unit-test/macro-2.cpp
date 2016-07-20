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
  // Function bmain() : spawn 100 threads in tmain instance.
  function("bmain");

  label("entry");
  var i(u(32));

  i = lit(u(32), 0);
  
  label("loop");
  spawn("tmain");
  i = i + 1_c;
  br(i == 100_c)("loop")("exit");
  
  label("exit");
  ret();
}

void tmain() {
  function("tmain");
  label("foo");
  ret();
}

int main(int argc, char **argv) {
  if_prog p;
  asm_prog a(p);
  init_macro_env(a);

  bmain();
  tmain();
  
  finish_macro_env();

  print(std::cout, p); 
  gen_prog(std::cout, p);

  return 0;
}
