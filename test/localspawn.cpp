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

static void bmain() {
  function("bmain");
  var i(u(32));

  label("entry");

  i = lit(u(32), 0);
  
  label("loop");
  br_spawn()("tmain")("loop2");

  label("loop2");
  i = i + lit(u(32), 1);
  br(i == lit(u(32), 10))("loop")("exit");
  
  label("exit");
  ret();

  label("tmain");
  var j(u(32));
  spawn("print_hex")(i);
  i = i * i;

  j = lit(u(32), 0);
  
  label("tloop");
  j = j + lit(u(32), 1);
  br(j == lit(u(32), 3))("tloop")("tloopx");

  label("tloopx");
  j = lit(u(32), 0);
  br(i == lit(u(32), 0))("tloop2")("texit");

  label("tloop2");
  i = i - lit(u(32), 1);
  br("tloop");
  
  label("texit");
  ret();
}

void localspawn(if_prog *pp) {
  if_prog &p(*pp);
  asm_prog a(p);
  init_macro_env(a);

  bmain();
 
  finish_macro_env();
}

REGISTER_TEST(localspawn, localspawn);
