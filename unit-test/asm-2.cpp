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

  a.label("entry");
  a.val(u(32), 0,     VAL_ARG);
  a.val(u(5),  1,   VAL_CONST).const_arg(0);
  a.val(bit(), 1,     VAL_LD_IDX).arg(0).arg(1);
  a.br(1).target("a").target("b");

  a.label("a");
  a.val(u(32), 2, VAL_CONST).const_arg(0x11111111);
  a.val(u(32), 2, VAL_XOR).arg(0).arg(2);
  a.br().target("cont");

  a.label("b");
  a.val(u(32), 2, VAL_CONST).const_arg(0x22222222);
  a.val(u(32), 2, VAL_XOR).arg(0).arg(2);
  a.br().target("cont");

  // Do something else for a block.
  a.label("cont");
  a.val(u(32), 3,     VAL_CONST).const_arg(1);
  a.val(u(32), 3,     VAL_ADD).arg(0).arg(3);

  a.label("user");
  a.val(u(32), 4,     VAL_CONST).const_arg(1);
  a.val(u(32), 4,     VAL_ADD).arg(2).arg(4);

  a.label("exit");
  a.val(void_type(), 10, VAL_RET).arg(0);

  a.assemble_func();
  
  print(cout, p); 
  // gen_prog(cout, p);

  return 0;
}
