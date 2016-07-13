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
  a.val(u(32), 0,   VAL_ARG);
  a.val(u(5),  1,   VAL_CONST).const_arg(0).const_arg(2);
  a.val(u(2),  2,   VAL_LD_IDX).arg(0).arg(1);
  a.br(2).target("a").target("b").target("c");

  a.label("a");
  a.val(u(32), 2, VAL_CONST).const_arg(0x11111111);
  a.val(u(32), 2, VAL_XOR).arg(0).arg(2);
  a.br().target("g1");

  a.label("b");
  a.val(u(32), 2, VAL_CONST).const_arg(0x22222222);
  a.val(u(32), 2, VAL_XOR).arg(0).arg(2);
  a.br().target("g1");

  a.label("c");
  a.val(u(32), 2, VAL_CONST).const_arg(0x22222222);
  a.val(u(32), 2, VAL_XOR).arg(0).arg(2);
  a.br().target("g5");

  a.label("g1");
  a.val(u(32), 3,     VAL_CONST).const_arg(1);
  a.val(u(32), 3,     VAL_ADD).arg(0).arg(3);
  a.br().target("g2").target("g3");

  a.label("g2");
  a.br().target("g2").target("g4");

  a.label("g3");
  a.br().target("g4");

  a.label("g4");
  a.br().target("g6");

  a.label("g5");
  a.br().target("g6");

  a.label("g6");
  a.br().target("g7").target("g8");

  a.label("g7");
  a.br().target("exit");

  a.label("g8");
  a.br().target("g9");

  a.label("g9");
  a.br().target("g9").target("g10");
  
  a.label("g10");
  a.br().target("exit");
  
  a.label("exit");
  a.val(void_type(), 10, VAL_RET).arg(2);

  a.assemble_func();
  
  print(cout, p); 
  // gen_prog(cout, p);

  return 0;
}
