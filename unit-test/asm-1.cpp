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
  a.val(u(32), 0,     VAL_CONST).   const_arg(0);
  a.val(u(32), 1,     VAL_CONST).   const_arg(0);

  a.label("loop");
  a.val(u(32), 2,     VAL_CONST).const_arg(1103515245);
  a.val(u(32), 3,     VAL_CONST).const_arg(12345);
  a.val(u(32), 4,       VAL_MUL).arg(1).arg(2);
  a.val(u(32), 1,       VAL_ADD).arg(3).arg(4);
  a.val(u(32), 5,     VAL_CONST).const_arg(100);
  a.val(u(32), 6,       VAL_XOR).arg(0).arg(5);
  a.val(bit(), 7, VAL_OR_REDUCE).arg(6);
  a.val(bit(), 7,       VAL_NOT).arg(7);
  a.val(u(32), 8,     VAL_CONST).const_arg(1);
  a.val(u(32), 0,       VAL_ADD).arg(0).arg(8);
  a.br(7).target("loop").target("exit");

  a.label("exit");
  a.val(void_type(), 9, VAL_CONST).const_arg(0);
  a.val(void_type(), 10, VAL_RET).arg(9);

  a.assemble_func();
  
  print(cout, p); 
  gen_prog(cout, p);

  return 0;
}
