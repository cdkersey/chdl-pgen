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

static void func() {
  function("func");
  label("bb0");

  label("bb1");

  label("bb2");
  ret();
}

static void bmain() {
  function("bmain");

  global_var("g", u(32), 0);
  global_var("a", a(u(32), 8));
  static_var("x", u(32), 0);

  var i(u(32));
  
  label("entry");

  i = lit(u(32), 0);
  
  label("loop");
  var sub_enable(load(i, lit(u(5), 0))), n_sub_enable(!sub_enable);

  store("x", load("x") - i); pred(sub_enable);
  store("x", load("x") + i); pred(n_sub_enable);

  store("g", load("g") + lit(u(32), 1));
  store("a", load(i, lit(u(5), 0), 3), -i);
  load("a", load(i, lit(u(5), 1), 3));

  spawn("func");

  i = i + lit(u(32), 1);

  br("loop");
}

void global(if_prog *pp) {
  if_prog &p(*pp);
  asm_prog a(p);
  init_macro_env(a);

  bmain();
  func();
  
  finish_macro_env();
}

REGISTER_TEST(global, global);
