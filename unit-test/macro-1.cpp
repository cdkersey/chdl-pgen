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
using namespace std;

void bmain() {
  function("bmain");

  static_var("x", u(32), 0);
  static_var("a", a(u(32), 64));

  label("entry");
  var y(u(32));
  y = 123_c;

  label("start");
  var x_val(u(32)), x_plus_1(u(32));

  x_val = load("x");
  x_plus_1 = x_val + 1_c;
  y = y - x_val;

  store("a", load(x_val, 0_c, 6), y);
  var a_val(u(32));
  a_val = load("a", load(x_val, 0_c, 6));
  store("x", x_plus_1);

  br("start");
}

void pipeline() {
  // function("pipeline");
}

int main(int argc, char **argv) {
  if_prog p;
  asm_prog a(p);
  init_macro_env(a);

  bmain();
  pipeline();
  
  finish_macro_env();

  // print(cout, p); 
  gen_prog(cout, p);

  return 0;
}
