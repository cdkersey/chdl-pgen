#include "../asm-macro.h"

#include "testgen.h"

using namespace pgen;

static void swaplt(var &a, var &b) {
  var lt(bit());
  var new_a(u(32)), new_b(u(32));

  lt = (a < b);
  sel(new_a, lt)(b)(a);
  sel(new_b, lt)(a)(b);

  a = new_a;
  b = new_b;
}

static void sort4() {
  
  function("sort4");

  static_var("out", sa(u(32), 4));
  
  label("entry");

  var a(u(32)), b(u(32)), c(u(32)), d(u(32));
  var next_out(sa(u(32), 4));

  a = arg(u(32));
  b = arg(u(32));
  c = arg(u(32));
  d = arg(u(32));

  swaplt(a, c);
  swaplt(b, d);

  label("stage1");

  swaplt(a, b);
  swaplt(c, d);

  label("stage2");

  swaplt(b, c);

  build(next_out)(a)(b)(c)(d);

  store("out", next_out);
  
  ret();
}

static void bmain() {
  function("bmain");

  label("entry");
  var i(u(32));
  i = lit(u(32), 0);


  label("loop");
  
  spawn("sort4")
    (lit(u(32), 10))
    (i + lit(u(32), 5))
    (i + lit(u(32), 1))
    (i + lit(u(32), 4));
  
  i = i + lit(u(32), 1);
  br(i == lit(u(32), 100))("loop")("exit");

  label("exit");
  ret();
}

using namespace std;

void sort(if_prog *pp) {
  if_prog &p(*pp);
  asm_prog a(p);

  init_macro_env(a);

  sort4();
  bmain();
  
  finish_macro_env();
}

REGISTER_TEST(sort, sort);
