#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

#include "../type.h"
#include "../asm-macro.h"

#include "testgen.h"

using namespace pgen;

static int clog2(unsigned long x) {
  int add = (x & (x - 1)) ? 1 : 0;
  
  for (int i = 63; i != -1; --i)
    if (x & (1ull << i)) return i + add;

  return 0;
}

static void sum_func() {
  function("sum");
  static_var("a", a(sa(u(32), 4), 16));
  static_var("s", u(32), 0);
  static_var("initialized", bit(), 0);
  bcast_var("s_fwd", u(32));

  label("entry");
  var i(u(4));
  i = arg(u(4));
  stall(i != lit(u(4), 0) & !load("initialized"));
  br(i == lit(u(4), 0))("stage0")("init");

  label("stage0");
  var vals(sa(u(32), 4));
  vals = load("a", i);

  store("initialized", lit(bit(), 1));

  label("stage1");
  var a(u(32)), b(u(32));
  a = load(vals, lit(u(2), 0)) + load(vals, lit(u(2), 1));
  b = load(vals, lit(u(2), 2)) + load(vals, lit(u(2), 3));
  
  label("stage2");
  var ab(u(32)), sval(u(32)), sval_fwd(u(32)), sval_fwd_valid(bit());

  sval_fwd = load("s_fwd");
  sval_fwd_valid = bcast_valid("s_fwd");
  
  ab = a + b;
  sval = load("s");

  label("stage3");
  var finalsum(u(32)), sval_sel(u(32));

  sel(sval_sel, sval_fwd_valid)(sval)(sval_fwd);
  finalsum = ab + sval_sel;

  store("s", finalsum);
  store("s_fwd", finalsum);

  ret();

  {
  label("init");
  var i(u(4));
  i = lit(u(4), 0);

  var a(u(32)), b(u(32)), c(u(32)), d(u(32));
  a = lit(u(32), 0);
  b = lit(u(32), 1);
  c = lit(u(32), 2);
  d = lit(u(32), 3);

  label("iloop");
  var v(sa(u(32), 4));
  build(v)(a)(b)(c)(d);
  store("a", i, v);

  a = a + lit(u(32), 4);
  b = b + lit(u(32), 4);
  c = c + lit(u(32), 4);
  d = d + lit(u(32), 4);
  
  i = i + lit(u(4), 1);
  br(i == lit(u(4), 0))("iloop")("stage0");
  }
}

static void bmain() {
  function("bmain");

  label("entry");
  var i(u(4));
  i = lit(u(4), 0);

  label("loop");
  spawn("sum")(i);
  
  i = i + lit(u(4), 1);
  br(i == lit(u(4), 0))("loop")("exit");

  label("exit");
  ret();
}

using namespace std;

void sum(if_prog *pp) {
  if_prog &p(*pp);
  asm_prog a(p);

  init_macro_env(a);

  sum_func();
  bmain();
  
  finish_macro_env();
}

REGISTER_TEST(sum, sum);
