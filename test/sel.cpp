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

void sel(pgen::if_prog *pp) {
  using namespace pgen;

  // Initialize the assembler.
  if_prog &p(*pp);
  asm_prog a(p);
  init_macro_env(a);
  
  // The assembly program.
  function("bmain");
  
  var i(u(32));
  var done(bit());

  label("init");
  i = lit(u(32), 0);

  label("loop");

  var i_lsbs(u(2)), rom_val(u(32));
  i_lsbs = load(i, lit(u(5), 0), 2);

  sel(rom_val, i_lsbs)
    (lit(u(32), 0xabcdef00))
    (lit(u(32), 0xbcdef000))
    (lit(u(32), 0xcdef0000))
    (lit(u(32), 0xdef00000));
  
  spawn("print_hex")(rom_val);
  
  i = i + lit(u(32), 1);
  done = (i == lit(u(32), 10));
  br(done)("loop")("exit");

  label("exit");
  ret();

  finish_macro_env();
}

REGISTER_TEST(sel, sel);
