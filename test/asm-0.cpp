#include <iostream>
#include <fstream>

#include <vector>
#include <string>
#include <sstream>

#include "testgen.h"

#include "../type.h"
#include "../if.h"
#include "../cgen.h"
#include "../cgen-cpp.h"
#include "../asm.h"

using namespace pgen;
using namespace std;

static void asm0(if_prog *pp) {
  // Initialize the assembler.
  if_prog &p(*pp);
  asm_prog a(p);
  
  // The assembly program.
  a.function("bmain");
  a.static_var(u(32), "x", 0);

  a.label("entry");
  a.val(u(32), 0,     VAL_CONST).   const_arg(1);
  a.val(u(32), 1, VAL_LD_STATIC).static_arg("x");
  a.val(u(32), 2,       VAL_ADD).         arg(0).  arg(1);
  a.val(3,        VAL_ST_STATIC).static_arg("x").  arg(2);
  a.br().target("entry");

  a.assemble_func();
}

REGISTER_TEST(asm0, asm0);
