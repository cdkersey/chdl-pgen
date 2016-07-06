#include <iostream>

#include <vector>
#include <string>
#include <sstream>

#include "../type.h"
#include "../if.h"
#include "../cgen.h"
#include "../break_cycles.h"
#include "../asm.h"

using namespace bscotch;
using namespace std;

int main(int argc, char **argv) {
  // Initialize the assembler.
  if_prog p;
  asm_prog a(p);
  
  // The assembly program.
  a.function("bmain");
  a.static_var(u(32), "x", 0);

  a.label("entry");
  a.val(u(32), 0,     VAL_CONST).   const_arg(0);
  a.val(u(32), 1, VAL_LD_STATIC).static_arg("x");
  a.val(u(32), 2,       VAL_ADD).         arg(0).  arg(1);
  a.val(3,        VAL_ST_STATIC).static_arg("x").  arg(2);
  a.br().target("entry");
  
  print(cout, p);
  // gen_prog(cout, p);

  return 0;
}
