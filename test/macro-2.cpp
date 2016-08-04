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

using namespace bscotch;

static void bmain() {
  // Function bmain() : spawn 10 threads in tmain instance.
  function("bmain");
  var i(u(32));

  label("entry");

  i = lit(u(32), 0);
  
  label("loop");
  spawn("tmain")(i);

  label("loop2");
  i = i + lit(u(32), 1);
  br(i == lit(u(32), 10))("loop")("exit");
  
  label("exit");
  ret();
}

static void tmain() {
  function("tmain");
  label("foo");
  var i(u(32)), j(u(32));
  i = arg(u(32));
  spawn("print_hex")(i);
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

void macro2(if_prog *pp) {
  if_prog &p(*pp);
  asm_prog a(p);
  init_macro_env(a);

  bmain();
  tmain();

  finish_macro_env();
}

REGISTER_TEST(macro2, macro2);
